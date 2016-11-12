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
#include "utils.hpp"

using boost::asio::ip::tcp;

using socket_ptr = boost::shared_ptr<tcp::socket>;

//------------------------------------------------------------------------------

void session(socket_ptr socket) {
    try {
        for (;;) { // Infinite read loop
            const unsigned max_length = 1024U;
            char data[max_length];
            boost::system::error_code ec;
            std::size_t length = socket->async_read_some(
                    boost::asio::buffer(data),
                    boost::fibers::asio::yield[ec]);
            if (ec == boost::asio::error::eof) {
                break; //connection closed cleanly by peer
            } else if (ec) {
                throw boost::system::system_error(ec); //some other error
            }
            std::cout<< tag() << ": handled: "
                     << std::string(data, length) << std::endl;

            boost::asio::async_write(
                    * socket,
                    boost::asio::buffer(data, length),
                    boost::fibers::asio::yield[ec]);

            if (ec == boost::asio::error::eof) {
                break; //connection closed cleanly by peer
            } else if (ec) {
                throw boost::system::system_error( ec); //some other error
            }
        }
        std::cout<< tag() << ": connection closed" << std::endl;
    } catch (const std::exception& ex) {
        std::cout<< tag() << ": caught exception : " << ex.what() << std::endl;
    }
}

//------------------------------------------------------------------------------

void server(boost::asio::io_service& io_service, tcp::acceptor & acceptor) {
    std::cout<< tag() << ": echo-server started" << std::endl;
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
        std::cout<< tag() << ": caught exception : " << ex.what() << std::endl;
    }
    io_service.stop();
    std::cout<< tag() << ": server stopped" << std::endl;
}

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    try {

        boost::asio::io_service io_service;
        boost::fibers::use_scheduling_algorithm<boost::fibers::asio::round_robin>(io_service);

        std::cout<< "Thread " << thread_names.lookup() << ": started"<< std::endl;

        tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), 9999));
        boost::fibers::fiber(server, std::ref(io_service), std::ref(acceptor)).detach();

        io_service.run();

        std::cout<< tag() << ": io_service returned" << std::endl;
        std::cout<<"Thread " << thread_names.lookup() << ": stopping" << std::endl;
        std::cout << "done." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cout<< thread_names.lookup() << ": caught exception : " << e.what() << std::endl;
    }
    return -1;
}
