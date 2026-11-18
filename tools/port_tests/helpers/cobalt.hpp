// ---- cobalt: running a ported co_main ----
// <boost/cobalt/main.hpp> defines the program's main, which runs the global
// co_main on a fresh io_context. A case cannot have that main (nor a file one
// co_main per example), so each ported co_main is a task<int> (ex_co_main)
// and ex_cobalt_main runs it with cobalt::run, under the default memory
// resource main.hpp's main installs.
template <class CoMain>
int ex_cobalt_main(int argc, char* argv[], CoMain co) {
    boost::cobalt::pmr::unsynchronized_pool_resource root_resource;
    auto* previous = boost::cobalt::this_thread::set_default_resource(&root_resource);
    int rc = boost::cobalt::run(co(argc, argv));
    boost::cobalt::this_thread::set_default_resource(previous);
    return rc;
}
// thread_pool prints from worker threads at once: the captured stream is
// guarded there (the original's printf is thread-safe, an ostringstream not).
inline std::mutex ex_out_mutex;
template <class... A>
int ex_printf_mt(const char* fmt, A... a) {
    std::lock_guard<std::mutex> lock(ex_out_mutex);
    return ex_printf(fmt, a...);
}
