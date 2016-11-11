#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/fiber/all.hpp>
#include "round_robin.hpp"
#include "yield.hpp"

using boost::asio::ip::tcp;

using socket_ptr = boost::shared_ptr<tcp::socket>;

/*****************************************************************************
*   thread names
*****************************************************************************/
class ThreadNames {
private:
    std::map<std::thread::id, std::string> names_{};
    const char* next_{ "ABCDEFGHIJKLMNOPQRSTUVWXYZ"};
    std::mutex mtx_{};

public:
    ThreadNames() = default;

    std::string lookup() {
        std::unique_lock<std::mutex> lk( mtx_);
        auto this_id( std::this_thread::get_id() );
        auto found = names_.find( this_id );
        if ( found != names_.end() ) {
            return found->second;
        }
        BOOST_ASSERT( *next_);
        std::string name(1, *next_++ );
        names_[ this_id ] = name;
        return name;
    }
};

ThreadNames thread_names;

/*****************************************************************************
*   fiber names
*****************************************************************************/
class FiberNames {
private:
    std::map<boost::fibers::fiber::id, std::string> names_{};
    unsigned next_{ 0 };
    boost::fibers::mutex mtx_{};

public:
    FiberNames() = default;

    std::string lookup() {
        std::unique_lock<boost::fibers::mutex> lk( mtx_);
        auto this_id( boost::this_fiber::get_id() );
        auto found = names_.find( this_id );
        if ( found != names_.end() ) {
            return found->second;
        }
        std::ostringstream out;
        // Bake into the fiber's name the thread name on which we first
        // lookup() its ID, to be able to spot when a fiber hops between
        // threads.
        out << thread_names.lookup() << next_++;
        std::string name( out.str() );
        names_[ this_id ] = name;
        return name;
    }
};

FiberNames fiber_names;

std::string tag() {
    std::ostringstream out;
    out << "Thread " << thread_names.lookup() << ": "
        << std::setw(8) << "Fiber " << fiber_names.lookup() << std::setw(0);
    return out.str();
}

/*****************************************************************************
*   message printing
*****************************************************************************/
void print_( std::ostream& out) {
    out << '\n';
}

template < typename T, typename... Ts >
void print_( std::ostream& out, T const& arg, Ts const&... args) {
    out << arg;
    print_(out, args...);
}

template < typename... T >
void print( T const&... args ) {
    std::ostringstream buffer;
    print_( buffer, args...);
    std::cout << buffer.str() << std::flush;
}

/****************************************************************************/

void session(socket_ptr socket) {
    try {
        for (;;) {
            const int max_length = 1024;
            char data[max_length];
            boost::system::error_code ec;
            std::size_t length = socket->async_read_some(
                    boost::asio::buffer( data),
                    boost::fibers::asio::yield[ec]);
            if (ec == boost::asio::error::eof) {
                break; //connection closed cleanly by peer
            } else if (ec) {
                throw boost::system::system_error(ec); //some other error
            }
            print(tag(), ": handled: ", std::string(data, length));

            boost::asio::async_write(
                    * socket,
                    boost::asio::buffer( data, length),
                    boost::fibers::asio::yield[ec]);

            if (ec == boost::asio::error::eof) {
                break; //connection closed cleanly by peer
            } else if (ec) {
                throw boost::system::system_error( ec); //some other error
            }
        }
        print(tag(), ": connection closed");
    } catch (const std::exception& ex) {
        print(tag(), ": caught exception : ", ex.what());
    }
}

/****************************************************************************/

void server(boost::asio::io_service& io_service, tcp::acceptor & acceptor) {
    print(tag(), ": echo-server started");
    try {
        for (;;) {
            socket_ptr socket( new tcp::socket(io_service) );
            boost::system::error_code ec;
            acceptor.async_accept(
                    *socket,
                    boost::fibers::asio::yield[ec]);
            if (ec) {
                throw boost::system::system_error( ec); //some other error
            } else {
                boost::fibers::fiber(session, socket).detach();
            }
        }
    } catch (const std::exception& ex) {
        print(tag(), ": caught exception : ", ex.what());
    }
    io_service.stop();
    print( tag(), ": server stopped");
}


/****************************************************************************/

int main(int argc, char* argv[]) {
    try {

        boost::asio::io_service io_service;
        boost::fibers::use_scheduling_algorithm<boost::fibers::asio::round_robin>(io_service);

        print("Thread ", thread_names.lookup(), ": started");

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 9999));
        boost::fibers::fiber(server, std::ref(io_service), std::ref(acceptor)).detach();

        io_service.run();

        print(tag(), ": io_service returned");
        print("Thread ", thread_names.lookup(), ": stopping");
        std::cout << "done." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        print("Exception: ", e.what(), "\n");
    }
    return -1;
}
