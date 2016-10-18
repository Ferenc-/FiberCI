#include <array>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <random>

#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/fiber/all.hpp>


std::string fn(int fiber_id, int work_length) {
        std::cout << "Fiber: #" << fiber_id 
            << " started. Working for " << work_length << " seconds." << std::endl;
        boost::this_fiber::sleep_for( std::chrono::seconds(work_length) );
        std::cout<< "Fiber: #" << fiber_id << " completed." << std::endl;
        return  std::to_string(work_length);
}

void foo() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::normal_distribution<> d(5,2);


    constexpr const unsigned concurrency = 10U;
    std::array<boost::fibers::future<std::string>, concurrency> futures;
    std::array<int, concurrency> works;
    std::generate(works.begin(), works.end(), [&gen, &d](){return std::round(d(gen));} );
    try
    {
        for(uint32_t i=0U; i<concurrency; ++i) {
            boost::fibers::packaged_task<std::string(int, int)> pt(fn);
            futures[i] = pt.get_future();
            boost::fibers::fiber(std::move(pt), i, works[i]).detach(); // launch task on a fiber
        }

        for(auto& e: futures) {
            e.wait(); // wait for it to finish
            //std::cout<<e.get();
        }

    }
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
}

int main()
{
    try
    {
        foo();
        std::cout << "Program done." << std::endl;

        return EXIT_SUCCESS;
    }
	catch ( std::exception const& e)
	{ std::cerr << "exception: " << e.what() << std::endl; }
	catch (...)
	{ std::cerr << "unhandled exception" << std::endl; }
	return EXIT_FAILURE;
}
