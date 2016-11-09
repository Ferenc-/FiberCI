#include <cstdlib> // for atoi
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/fiber/all.hpp>


void do_accept(boost::asio::io_service& io_service, uint16_t port,
               boost::fibers::asio::yield_context yield) {
    tcp::acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
    
    for (;;) {
        boost::system::error_code ec;
        boost::shared_ptr<session> new_session(new session(io_service));
        acceptor.async_accept(new_session->socket(),
                              boost::fibers::asio::yield[ec]);
        if (! ec) {
            boost::fibers::fiber(boost::bind(&session::go, new_session)).detach();
        }
    }
}



int main(int argc, char* argv[])
{
    try {
        if (argc != 2) {
            std::cerr << "Usage: echo_server <port>\n";
            return 1;
        }
        
        boost::asio::io_service io_service;
        boost::fibers::asio::round_robin ds(io_service);
        boost::fibers::set_scheduling_algorithm(&ds);
        
        boost::fibers::asio::spawn(io_service,
                                   boost::bind(do_accept,
                                               boost::ref(io_service), std::atoi(argv[1]), _1));
        
        io_service.run();
    } catch (std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
}
