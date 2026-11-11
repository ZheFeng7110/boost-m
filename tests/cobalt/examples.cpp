// mcpplibs.mwrap.boost.cobalt — the official examples (libs/cobalt/example/),
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers and the
// cobalt sources alone. A case cannot define main (which
// <boost/cobalt/main.hpp> does), nor a file one co_main per example: each
// co_main becomes a task<int> (ex_co_main) that ex_cobalt_main runs with
// cobalt::run. echo_server listens on a free port instead of 55555 and
// a harness client exchanges one message with it; delay / delay_op get a
// 10 ms delay as argv[1]; spsc passes 100 thousand values, not 100 million. The output of thread and thread_pool (worker
// threads) is not compared.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/cobalt/config.hpp>
#include <cassert>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.cobalt;
import mcpplibs.mwrap.boost.outcome;
import mcpplibs.mwrap.boost.lockfree;
import mcpplibs.mwrap.boost.signals2;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// ---- loopback harness (boost-m tests): free ports, waiting for a listener,
// and a background peer that drains or echoes, so server-side examples run in
// one process. The fixed ports of the examples (55555, ...) are replaced by
// ex_port_number(). ----
// UDP servers cannot be probed: ex_free_udp_port() marks the case so the
// harness waits briefly instead (and the UDP clients below retry).
inline bool ex_udp = false;
inline bool ex_local = false;  // ex_port is a local (UNIX domain) socket path
inline bool ex_no_probe = false;  // the server accepts one connection only: do not probe it
inline std::string ex_port;
inline unsigned short ex_port_number() { return static_cast<unsigned short>(std::stoi(ex_port)); }
inline std::string ex_free_port() {
    ex_udp = ex_local = ex_no_probe = false;
    boost::asio::io_context c;
    boost::asio::ip::tcp::acceptor a(c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    return std::to_string(a.local_endpoint().port());
}
inline bool ex_try_connect(boost::asio::ip::tcp::socket& s, const std::string& port) {
    boost::system::error_code ec;
    s.connect({boost::asio::ip::make_address("127.0.0.1"), static_cast<unsigned short>(std::stoi(port))}, ec);
    return !ec;
}
// The probe connection stays open (a server would report it closing).
inline void ex_wait_listening(const std::string& port) {
    auto& c = *new boost::asio::io_context;
    for (int i = 0; i < 500; ++i) {
        auto* s = new boost::asio::ip::tcp::socket(c);
        if (ex_try_connect(*s, port)) return;
        delete s;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
// Connects `n` times (retrying while the example is not listening yet),
// optionally sends `send`, and reads until the server closes.
inline void ex_peer_async(std::string port, int n, std::string send = {}) {
    std::thread([port, n, send] {
        for (int k = 0; k < n; ++k) {
            boost::asio::io_context c;
            boost::asio::ip::tcp::socket s(c);
            int tries = 0;
            while (!ex_try_connect(s, port)) {
                if (++tries > 300) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                s = boost::asio::ip::tcp::socket(c);
            }
            boost::system::error_code ec;
            if (!send.empty()) boost::asio::write(s, boost::asio::buffer(send), ec);
            char buf[4096];
            std::size_t got = 0;
            while (!ec) got += s.read_some(boost::asio::buffer(buf), ec);
            // reset before any data: the connection landed in the backlog of
            // the previous server's acceptor as it closed; it does not count
            if (got == 0 && ec == boost::asio::error::connection_reset) --k;
        }
    }).detach();
}
inline std::string ex_free_udp_port() {
    boost::asio::io_context c;
    boost::asio::ip::udp::socket u(c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    ex_udp = true;
    ex_local = false;
    return std::to_string(u.local_endpoint().port());
}
// The probe stays open like the TCP one: on macOS a server's accept reports
// a connection the peer already closed (EINVAL) and the beast servers stop.
inline bool ex_try_connect_local(const std::string& path) {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
    static auto& c = *new boost::asio::io_context;
    auto* s = new boost::asio::local::stream_protocol::socket(c);
    boost::system::error_code ec;
    s->connect(boost::asio::local::stream_protocol::endpoint(path), ec);
    if (ec)
        delete s;
    return !ec;
#else
    (void)path;
    return false;
#endif
}
// Runs a server example in a detached thread (it serves until the process
// exits) and waits until it accepts connections.
inline void ex_serve_async(std::function<void()> server, std::string port) {
    std::thread(std::move(server)).detach();
    if (ex_udp || ex_no_probe) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else if (ex_local) {
        for (int i = 0; i < 500 && !ex_try_connect_local(port); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } else {
        ex_wait_listening(port);
    }
}
// Calls a ported main with its own argv (kept alive: a server runs on).
template <class F>
int ex_call_main(F f, std::vector<std::string> a) {
    if constexpr (std::is_invocable_v<F>) {
        return f();
    } else {
        auto* store = new std::vector<std::string>(std::move(a));
        auto* ptrs = new std::vector<char*>;
        for (auto& x : *store) ptrs->push_back(x.data());
        ptrs->push_back(nullptr);
        return f(int(store->size()), ex_argv_t{ptrs->data()});
    }
}
// ---- harness clients: each prints what it received and returns 0 ----
inline std::string ex_shape(const std::string& got, bool show) {
    if (show) return got;
    return got.empty() ? "(nothing)\n" : (got.back() == '\n' ? "a line\n" : "text\n");
}
// TCP: sends `send`, then reads `n` bytes (n == 0: until the server closes).
// The connection stays open: a server seeing the client leave would report
// it (from its own thread, racing with this output), and some end on it.
inline int ex_tcp_exchange(const std::string& port, const std::string& send, std::size_t n, bool show = true) {
    auto& c = *new boost::asio::io_context;  // kept: see above
    auto& s = *new boost::asio::ip::tcp::socket(c);
    for (int i = 0; !ex_try_connect(s, port); ++i) {  // (an unprobed server may not listen yet)
        if (i == 300) { ex_out << "connect failed\n"; return 1; }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        s = boost::asio::ip::tcp::socket(c);
    }
    boost::system::error_code ec;
    if (!send.empty()) boost::asio::write(s, boost::asio::buffer(send), ec);
    std::string got;
    if (n) {
        got.resize(n);
        got.resize(boost::asio::read(s, boost::asio::buffer(got), ec));
    } else {
        char buf[1024];
        while (!ec) got.append(buf, s.read_some(boost::asio::buffer(buf), ec));
    }
    ex_out << "received: " << ex_shape(got, show);
    return 0;
}
// UDP: sends `send` (retrying while there is no reply) and prints the reply.
inline int ex_udp_exchange(const std::string& port, const std::string& send, bool show = true) {
    boost::asio::io_context c;
    boost::asio::ip::udp::socket u(c, boost::asio::ip::udp::v4());
    boost::asio::ip::udp::endpoint to(boost::asio::ip::make_address("127.0.0.1"),
                                      static_cast<unsigned short>(std::stoi(port)));
    u.non_blocking(true);
    char buf[1024];
    for (int i = 0; i < 300; ++i) {
        boost::system::error_code ec;
        if (i % 20 == 0) u.send_to(boost::asio::buffer(send), to, 0, ec);
        boost::asio::ip::udp::endpoint from;
        std::size_t k = u.receive_from(boost::asio::buffer(buf), from, 0, ec);
        if (!ec) {
            ex_out << "received: " << ex_shape(std::string(buf, k), show);
            return 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ex_out << "no reply\n";
    return 1;
}
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
inline int ex_local_exchange(const std::string& path, const std::string& send, std::size_t n, bool show = true) {
    auto& c = *new boost::asio::io_context;  // kept open, as in ex_tcp_exchange
    auto& s = *new boost::asio::local::stream_protocol::socket(c);
    boost::system::error_code ec;
    s.connect(boost::asio::local::stream_protocol::endpoint(path), ec);
    if (ec) { ex_out << "connect failed\n"; return 1; }
    boost::asio::write(s, boost::asio::buffer(send), ec);
    std::string got(n, '\0');
    got.resize(boost::asio::read(s, boost::asio::buffer(got), ec));
    ex_out << "received: " << ex_shape(got, show);
    return 0;
}
#else
// No local sockets: the examples print that and return; nothing to talk to
// (their output is not compared there).
inline int ex_local_exchange(const std::string&, const std::string&, std::size_t, bool = true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 0;
}
#endif
// A fresh path in the temporary directory (removed first).
inline std::string ex_temp_path(const std::string& name) {
    auto p = std::filesystem::temp_directory_path() / ("boost-m-asio-" + std::to_string(std::random_device{}()) + "-" + name);
    std::filesystem::remove_all(p);
    return p.string();
}
inline std::string ex_local_path(const std::string& name) {
    ex_udp = false;
    ex_local = true;
    return ex_temp_path(name);
}
// An HTTP document root holding index.html.
inline std::string ex_doc_root() {
    auto d = ex_temp_path("www");
    std::filesystem::create_directories(d);
    // binary: the served file is the same octets everywhere (no \r\n on Windows)
    std::ofstream(std::filesystem::path(d) / "index.html", std::ios::binary) << "<html><body>boost-m</body></html>\n";
    return d;
}
// A background TCP echo server of the harness (the upstream of a proxy).
inline std::string ex_harness_echo() {
    auto* c = new boost::asio::io_context;
    auto* a = new boost::asio::ip::tcp::acceptor(*c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    std::string port = std::to_string(a->local_endpoint().port());
    std::thread([a] {
        for (;;) {
            boost::asio::ip::tcp::socket s(a->get_executor());
            a->accept(s);
            std::thread([s = std::move(s)]() mutable {
                char buf[1024];
                boost::system::error_code ec;
                for (;;) {
                    std::size_t k = s.read_some(boost::asio::buffer(buf), ec);
                    if (ec) return;
                    boost::asio::write(s, boost::asio::buffer(buf, k), ec);
                }
            }).detach();
        }
    }).detach();
    return port;
}
// An error_code's message: "Success" for no error. The text of no error is
// the C library's (glibc "Success", musl "No error information", macOS
// "Undefined error: 0", Windows "The operation completed successfully").
inline std::string ex_message(const boost::system::error_code& ec) {
    return ec ? ec.message() : std::string("Success");
}
#if defined(BOOST_ASIO_HAS_PIPE) && !defined(BOOST_ASIO_WINDOWS)
// A descriptor reading `text` then EOF: stands in for the process's stdin
// (STDIN_FILENO) where an example reads it through a stream_descriptor.
inline int ex_stdin_fd(const std::string& text) {
    boost::asio::io_context c;
    boost::asio::readable_pipe r(c);
    boost::asio::writable_pipe w(c);
    boost::asio::connect_pipe(r, w);
    boost::asio::write(w, boost::asio::buffer(text));
    w.close();
    return r.release();
}
#endif
#if defined(DOCTEST_VERSION_MAJOR)  // (not in the oracle)
// Diagnostics for a case that hangs or ends the process (neither leaves a
// doctest report): a case still running after 60 s is named on stderr, and
// on Windows (where a fast-fail such as heap corruption kills the process)
// every case is named before it runs.
struct ex_case_names : doctest::IReporter {
    inline static std::mutex m;
    inline static std::string current;
    inline static std::chrono::steady_clock::time_point since;
    inline static bool reported = false;
    explicit ex_case_names(const doctest::ContextOptions&) {
        static bool started = false;
        if (std::exchange(started, true))
            return;
        std::thread([] {
            for (;;) {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                std::lock_guard<std::mutex> l(m);
                if (!current.empty() && !reported && std::chrono::steady_clock::now() - since > std::chrono::seconds(60)) {
                    std::fprintf(stderr, "case still running after 60 s: %s\n", current.c_str());
                    reported = true;
                }
            }
        }).detach();
    }
    void report_query(const doctest::QueryData&) override {}
    void test_run_start() override {}
    void test_run_end(const doctest::TestRunStats&) override {}
    void test_case_start(const doctest::TestCaseData& d) override {
#if defined(_WIN32)
        std::fprintf(stderr, "case: %s\n", d.m_name);
#endif
        std::lock_guard<std::mutex> l(m);
        current = d.m_name;
        since = std::chrono::steady_clock::now();
        reported = false;
    }
    void test_case_reenter(const doctest::TestCaseData&) override {}
    void test_case_end(const doctest::CurrentTestCaseStats&) override {
        std::lock_guard<std::mutex> l(m);
        current.clear();
    }
    void test_case_exception(const doctest::TestCaseException&) override {}
    void subcase_start(const doctest::SubcaseSignature&) override {}
    void subcase_end() override {}
    void log_assert(const doctest::AssertData&) override {}
    void log_message(const doctest::MessageData&) override {}
    void test_case_skipped(const doctest::TestCaseData&) override {}
};
DOCTEST_REGISTER_LISTENER("ex_case_names", 1, ex_case_names);
#endif
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

// libs/cobalt/example/channel.cpp
namespace ex_channel {
namespace cobalt = boost::cobalt;

// tag::channel_example[]
cobalt::promise<void> producer(cobalt::channel<int> & chan)
{
  for (int i = 0; i < 4; i++)
    co_await chan.write(i);

  chan.close();
}

boost::cobalt::task<int> ex_co_main(int argc, char * argv[])
{
  cobalt::channel<int> c;

  auto p = producer(c);
  while (c.is_open())
    ex_out << co_await c.read() << std::endl;

  co_await p;
  co_return 0;
}
// end::channel_example[]
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: channel") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0
1
2
3
)ex"));
}
}  // namespace ex_channel

// libs/cobalt/example/outcome.cpp
namespace ex_outcome {
using namespace boost;

outcome_v2::awaitables::lazy<int> lazy_func(int x)
{
  co_return x + 1;
}

outcome_v2::awaitables::eager<int> eager_func(int x)
{
  co_return x + 1;
}


boost::cobalt::task<int> ex_co_main(int argc, char * argv[])
{
  [[maybe_unused]] auto lr = co_await lazy_func(10);

  assert(lr == 11);

  [[maybe_unused]] auto er = co_await eager_func(10);
  assert(er == 11);

  co_return 0;
}
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: outcome") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_outcome

// libs/cobalt/example/spsc.cpp
namespace ex_spsc {
namespace cobalt = boost::cobalt;

/// This is a simple class making a lockfree::spsc_queue awaitable. It's not movable, since the spsc_queue isn't.
template<typename T, typename ... Options>
struct awaitable_spsc_queue
{
  boost::lockfree::spsc_queue<T, Options...> queue;

  template<typename ... Args>
  awaitable_spsc_queue(Args && ... args) : queue(std::forward<Args>(args)...) {}

  // if the queue gets destroyed, destroying the awaiters is all we can do.
  ~awaitable_spsc_queue()
  {
    if (auto r = reader.load(); r != nullptr)
      std::coroutine_handle<void>::from_address(r).destroy();

    if (auto w = writer.load(); w != nullptr)
      std::coroutine_handle<void>::from_address(w).destroy();
  }
  // to avoid locks, put the coroutine handles into atomics.
  std::atomic<void*> reader{nullptr}, writer{nullptr};

  // capture the read & write executor
  cobalt::executor read_executor, write_executor;

  // the awaitable to read a value from the queue
  struct read_op
  {
    awaitable_spsc_queue * this_;

    // if reads are available we don't need to suspend
    bool await_ready() {return this_->queue.read_available();}

    // The suspend implementation. We expect the coroutine promise to have an associated executor.
    // If it doesn't resumption will end up on the system_executor
    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h)
    {
      // Capture the read_executor.
      if constexpr (requires {h.promise().get_executor();})
        this_->read_executor = h.promise().get_executor();
      else
        this_->read_executor = cobalt::this_thread::get_executor();

      // Make sure there's only one coroutine awaiting the read
      assert(this_->reader == nullptr);
      // Store the handle of the awaiter.
      this_->reader.store(h.address());
    }
    T await_resume()
    {
      T res;
      // Grab the value from the queue
      this_->queue.pop(res);
      // if a writer is waiting post it to complete on it's thread
      auto w = cobalt::unique_handle<void>::from_address(this_->writer.exchange(nullptr));
      if (w)
        boost::asio::post(this_->write_executor, std::move(w));

      return res;
    }
  };

  // The function to be used with for reading.
  read_op read() {return {this};}

  // the awaitable to write a value from the queue
  struct write_op
  {
    awaitable_spsc_queue * this_;
    // The value to write
    T value;

    // If there is room in the queue we don't need to suspend
    bool await_ready() {return this_->queue.write_available();}

    // The suspend implementation. Similar to the writer one.
    template<typename Promise>
    void await_suspend(std::coroutine_handle<Promise> h)
    {
      // Capture the read_executor.
      if constexpr (requires {h.promise().get_executor();})
        this_->write_executor = h.promise().get_executor();
      else
        this_->write_executor = cobalt::this_thread::get_executor();

      assert(this_->writer == nullptr);
      this_->writer.store(h.address());
    }

    // Resume the coroutine, this is where we do the actual write.
    // The reason is that we suspend when we cannot write because the queue is full.
    void await_resume()
    {
      this_->queue.push(std::move(value));
      // if a writer is waiting post it
      auto r = cobalt::unique_handle<void>::from_address(this_->reader.exchange(nullptr));
      if (r)
        boost::asio::post(this_->read_executor, std::move(r));
    }
  };

  // The function to be used for writing.
  write_op write(T value) {return {this, std::move(value)};}
};

// Dummy thread blasting out values.
cobalt::thread thr(awaitable_spsc_queue<int> & q)
{
  for (int i = 0; i <= 100000; i++)
    co_await q.write(i);

  co_await q.write(-1);
}

boost::cobalt::task<int> ex_co_main(int argc, char * argv[])
{
  awaitable_spsc_queue<int> queue{1024};
  auto t = thr(queue);

  // dummy consumer
  for (auto val = co_await queue.read();
       val >= 0;
       val = co_await queue.read());

  co_await t;
  co_return 0;
}
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: spsc") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_spsc

// libs/cobalt/example/thread.cpp
namespace ex_thread {
namespace cobalt = boost::cobalt;
using boost::system::error_code;

template<typename Signature>
using cchannel = boost::asio::experimental::concurrent_channel<Signature>;

// this is a function doing some CPU heavy work that should be offloaded onto a thread
cobalt::promise<int> cpu_intense_work(int a, int b) {co_return a + b;}

// this channel is used to send a response to completed work
using response_channel = cchannel<void(std::exception_ptr, int)>;
// this channel is used to send a request to a working thread
using request_channel = cchannel<void(error_code, int, int, response_channel * res)>;

// the worker wrapper
cobalt::thread worker(request_channel & work)
{
  while (work.is_open())
  {
    auto [ec, a, b, respond_to] = co_await work.async_receive(boost::asio::as_tuple(cobalt::use_op));
    if (ec) // done, ignore. in our code this is only triggered by closing the channel
      break;

    // to emulate this being like awaiting on the same thread, we also deliver an exception.
    std::exception_ptr ep;
    int res = 0;
    try
    {
      res = co_await cpu_intense_work(a, b);
    }
    catch(...)
    {
      // this way exception get sent to the awaiting coro as if it was a call.
      ep = std::current_exception();
    }
    // send the response. If the channel is closed, the program will terminate!
    co_await respond_to->async_send(ep, res, boost::asio::redirect_error(cobalt::use_op, ec));
  }
}

cobalt::promise<void> work(request_channel & rc, int min_a, int max_a, int b)
{
  response_channel res{co_await cobalt::this_coro::executor};
  for (int a = min_a; a <= max_a; a++)
  {
    // the following two calls offload the work to another thread.
    co_await rc.async_send(error_code{}, a, b, &res, cobalt::use_op);
    int c = co_await res.async_receive(cobalt::use_op); // may throw if working thread has an exception
    ex_printf("The CPU intensive result of adding %d to %d, is %d\n", a, b, c);
  }
}

boost::cobalt::task<int> ex_co_main(int argc, char *argv [])
{
  // a very simple thread pool
  std::vector<cobalt::thread> thrs;
  const std::size_t n = 4u;

  request_channel rc{co_await cobalt::this_coro::executor};
  for (auto i = 0u; i < n; i++)
      thrs.push_back(worker(rc));

  try
  {
    // this is an over simplification, but emulated multiple pieces of
    // code in the single threaded environment offloading work to the thread.
    co_await cobalt::join(
        work(rc, 0, 10, 32),
        work(rc, 10, 20, 22),
        work(rc, 50, 60, -18)
        );

  }
  catch(std::exception & e)
  {
    ex_printf("Completed with exception %s\n", e.what());
  }
  // closing the channel will cause the threads to complete
  rc.close();
  // wait them so they don't leak.
  co_await cobalt::join(thrs);
  co_return 0;
}
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: thread") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(The CPU intensive result of adding 0 to 32, is 32
The CPU intensive result of adding 10 to 22, is 32
The CPU intensive result of adding 50 to -18, is 32
The CPU intensive result of adding 11 to 22, is 33
The CPU intensive result of adding 51 to -18, is 33
The CPU intensive result of adding 1 to 32, is 33
The CPU intensive result of adding 12 to 22, is 34
The CPU intensive result of adding 52 to -18, is 34
The CPU intensive result of adding 2 to 32, is 34
The CPU intensive result of adding 13 to 22, is 35
The CPU intensive result of adding 53 to -18, is 35
The CPU intensive result of adding 3 to 32, is 35
The CPU intensive result of adding 14 to 22, is 36
The CPU intensive result of adding 54 to -18, is 36
The CPU intensive result of adding 4 to 32, is 36
The CPU intensive result of adding 55 to -18, is 37
The CPU intensive result of adding 15 to 22, is 37
The CPU intensive result of adding 5 to 32, is 37
The CPU intensive result of adding 56 to -18, is 38
The CPU intensive result of adding 16 to 22, is 38
The CPU intensive result of adding 6 to 32, is 38
The CPU intensive result of adding 57 to -18, is 39
The CPU intensive result of adding 17 to 22, is 39
The CPU intensive result of adding 7 to 32, is 39
The CPU intensive result of adding 58 to -18, is 40
The CPU intensive result of adding 18 to 22, is 40
The CPU intensive result of adding 8 to 32, is 40
The CPU intensive result of adding 59 to -18, is 41
The CPU intensive result of adding 19 to 22, is 41
The CPU intensive result of adding 9 to 32, is 41
The CPU intensive result of adding 60 to -18, is 42
The CPU intensive result of adding 20 to 22, is 42
The CPU intensive result of adding 10 to 32, is 42
)ex"));
#endif
}
}  // namespace ex_thread

// libs/cobalt/example/thread_pool.cpp
namespace ex_thread_pool {
namespace cobalt = boost::cobalt;
using boost::system::error_code;

// this is a function doing some CPU heavy work that should be offloaded onto a thread_pool
cobalt::promise<int> cpu_intense_work(
    int a, int b,
    boost::asio::executor_arg_t = {}, cobalt::executor = cobalt::this_thread::get_executor())
    // ^set the executor manually. but default it so we can still use it with the thread_local one if present
{
  co_return a + b;
}

cobalt::task<void> work(int min_a, int max_a, int b)
{
  auto exec = co_await cobalt::this_coro::executor;
  for (int a = min_a; a <= max_a; a++)
  {
    // the following two calls offload the work to another thread.
    int c = co_await cpu_intense_work(a, b, boost::asio::executor_arg, exec);
    ex_printf_mt("The CPU intensive result of adding %d to %d, is %d\n", a, b, c);
  }
}
int example_main(int , char * [])
{
  const std::size_t n = 4u;
  boost::asio::thread_pool tp{n};

  // a very simple thread pool

  auto cpl =
      [](std::exception_ptr ep)
      {
          if (ep)
            try
            {
              std::rethrow_exception(ep);
            }
            catch(std::exception & e)
            {
              ex_printf_mt("Completed with exception %s\n", e.what());
            }
          };

  cobalt::spawn(boost::asio::make_strand(tp.get_executor()), work(0, 10, 32),   cpl);
  cobalt::spawn(boost::asio::make_strand(tp.get_executor()), work(10, 20, 22),  cpl);
  cobalt::spawn(boost::asio::make_strand(tp.get_executor()), work(50, 60, -18), cpl);

  // wait them so they don't leak.
  tp.join();
  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: thread_pool") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(The CPU intensive result of adding 50 to -18, is 32
The CPU intensive result of adding 51 to -18, is 33
The CPU intensive result of adding 52 to -18, is 34
The CPU intensive result of adding 53 to -18, is 35
The CPU intensive result of adding 54 to -18, is 36
The CPU intensive result of adding 55 to -18, is 37
The CPU intensive result of adding 56 to -18, is 38
The CPU intensive result of adding 57 to -18, is 39
The CPU intensive result of adding 58 to -18, is 40
The CPU intensive result of adding 59 to -18, is 41
The CPU intensive result of adding 60 to -18, is 42
The CPU intensive result of adding 10 to 22, is 32
The CPU intensive result of adding 11 to 22, is 33
The CPU intensive result of adding 12 to 22, is 34
The CPU intensive result of adding 13 to 22, is 35
The CPU intensive result of adding 14 to 22, is 36
The CPU intensive result of adding 15 to 22, is 37
The CPU intensive result of adding 16 to 22, is 38
The CPU intensive result of adding 0 to 32, is 32
The CPU intensive result of adding 17 to 22, is 39
The CPU intensive result of adding 1 to 32, is 33
The CPU intensive result of adding 18 to 22, is 40
The CPU intensive result of adding 19 to 22, is 41
The CPU intensive result of adding 20 to 22, is 42
The CPU intensive result of adding 2 to 32, is 34
The CPU intensive result of adding 3 to 32, is 35
The CPU intensive result of adding 4 to 32, is 36
The CPU intensive result of adding 5 to 32, is 37
The CPU intensive result of adding 6 to 32, is 38
The CPU intensive result of adding 7 to 32, is 39
The CPU intensive result of adding 8 to 32, is 40
The CPU intensive result of adding 9 to 32, is 41
The CPU intensive result of adding 10 to 32, is 42
)ex"));
#endif
}
}  // namespace ex_thread_pool

// libs/cobalt/example/delay.cpp
namespace ex_delay {
using namespace boost;

// tag::timer_example[]
boost::cobalt::task<int> ex_co_main(int argc, char * argv[]) // <1>
{
  asio::steady_timer tim{co_await asio::this_coro::executor, // <2>
                         std::chrono::milliseconds(std::stoi(argv[1]))}; // <3>
  co_await tim.async_wait(cobalt::use_op); // <4>
  co_return 0; // <5>
}
// end::timer_example[]
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: delay") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back("10");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_delay

// libs/cobalt/example/delay_op.cpp
namespace ex_delay_op {
using namespace boost;

// tag::timer_example[]
struct wait_op final : cobalt::op<system::error_code> // <1>
{
  asio::steady_timer & tim;
  wait_op(asio::steady_timer & tim) : tim(tim) {}
  void ready(cobalt::handler<system::error_code> h ) override // <2>
  {
    if (tim.expiry() < std::chrono::steady_clock::now())
      h(system::error_code{});
  }
  void initiate(cobalt::completion_handler<system::error_code> complete) override // <3>
  {
    tim.async_wait(std::move(complete));
  }
};


boost::cobalt::task<int> ex_co_main(int argc, char * argv[])
{
  asio::steady_timer tim{co_await asio::this_coro::executor,
                         std::chrono::milliseconds(std::stoi(argv[1]))};
  co_await wait_op(tim); // <4>
  co_return 0; //
}
// end::timer_example[]
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: delay_op") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back("10");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_delay_op

// libs/cobalt/example/signals.cpp
namespace ex_signals {
namespace cobalt = boost::cobalt;
namespace signals = boost::signals2;

template<typename Signal>
struct signal_awaitable
{
  using args_type = boost::callable_traits::args_t<typename Signal::signature_type>;


  bool await_ready() { return false; } // < always wait for the signal to fire.
  void await_suspend(std::coroutine_handle<void> h)
  {
    awaited_from.reset(h.address());
    // the handler will get copied, so we can't capture the handle with a unique_ptr
    signal.connect_extended(
        [this, _ = boost::intrusive_ptr<signal_awaitable>(this)
         ](const signals::connection & conn, auto ... args) mutable
        {
          auto aw = std::move(awaited_from);
          conn.disconnect();
          result_cache.emplace(std::move(args)...); // the result_catch lives in the coro frame
          std::move(aw).resume(); // release ownership & resume
        });

  }

  auto await_resume() // return the value
  {
    constexpr std::size_t size = std::tuple_size_v<args_type>;
    if constexpr (size == 1u) // single argument doesn't need a tuple
      return std::get<0u>(*std::move(result_cache));
    else if constexpr (size > 1u) // make a tuple if more than one arg
      return *std::move(result_cache);
    // else return void.
  }

  // capture it for lazy initialization
  Signal & signal;
  // capture the ownership of the awaiting coroutine
  cobalt::unique_handle<void> awaited_from;
  // store the result from the call
  std::optional<args_type> result_cache;

  // to manage shared ownership with an internal counter.
  // If the last gets released before the handler is invoked,
  // the coro just gets destroyed.
  std::size_t use_count{0u};
  friend void intrusive_ptr_add_ref(signal_awaitable * st) {st->use_count++;}
  friend void intrusive_ptr_release(signal_awaitable * st)
  {
    if (st->use_count-- == 1u)
      st->awaited_from.reset();
  }
};

}  // namespace ex_signals (reopened after the operator)
 namespace boost::signals2
{

// make all signals awaitable
template<typename ... Args>
auto operator co_await(boost::signals2::signal<Args...> & sig) -> ::ex_signals::signal_awaitable<boost::signals2::signal<Args...>>
{
  return {sig};
}

}
namespace ex_signals {

cobalt::promise<int> await_signal(signals::signal<void(int)>  & sig)
{
  co_return co_await sig;
}

boost::cobalt::task<int> ex_co_main(int argc, char * argv[])
{

  signals::signal<void(int)> sig;
  auto p = await_signal(sig);
  sig(42);
  auto res = co_await p;
  assert(res == 42);
  co_return 0;
}
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: signals") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_signals

// libs/cobalt/example/echo_server.cpp
namespace ex_echo_server {
namespace cobalt = boost::cobalt;
namespace this_coro = boost::cobalt::this_coro;
//end::decls[]

// tag::echo[]
cobalt::promise<void> echo(cobalt::io::stream_socket socket)
{
  try // <1>
  {
    char data[4096];
    while (socket.is_open()) // <2>
    {
      std::size_t n = co_await socket.read_some(boost::asio::buffer(data)); // <3>
      co_await cobalt::io::write(socket, boost::asio::buffer(data, n)); // <4>
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo: exception: %s\n", e.what());
  }
}
// end::echo[]


// tag::listen[]
cobalt::generator<cobalt::io::stream_socket> listen()
{
  cobalt::io::acceptor acceptor(cobalt::io::endpoint{cobalt::io::tcp_v4, "0.0.0.0", ex_port_number()});
  for (;;) // <1>
  {
    cobalt::io::stream_socket sock = co_await acceptor.accept(); // <2>
    co_yield std::move(sock); // <3>
  }
  co_return cobalt::io::stream_socket{}; // <4>
}
// end::listen[]

// tag::run_server[]
cobalt::promise<void> run_server(cobalt::wait_group & workers)
{
  auto l = listen(); // <1>
  while (true)
  {
    if (workers.size() == 10u)
      co_await workers.wait_one();  // <2>
    else
      workers.push_back(echo(co_await l)); // <3>
  }
}
// end::run_server[]

// tag::main[]
boost::cobalt::task<int> ex_co_main(int argc, char ** argv)
{
  co_await cobalt::with(cobalt::wait_group(), &run_server); // <1>
  co_return 0u;
}
// end::main[]
int example_main(int argc, char* argv[]) { return ex_cobalt_main(argc, argv, &ex_co_main); }

TEST_CASE("example: echo_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_echo_server
