
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
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
