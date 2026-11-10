// mcpplibs.mwrap.boost.asio — the official cpp20 examples (libs/asio/example/cpp20/)
// that run without a network peer, ported mechanically: each program runs as a
// case and its output must equal what the same code printed when built from
// upstream/boost headers alone. The operations/ examples accept three
// connections on a fixed port: here the port is a free one and a background
// peer (the loopback harness below) connects and drains, so they run in one
// process; their 1 s delay between repeated messages is 10 ms here (the
// output does not depend on it). Outputs not compared: parallel_sort (completion order), the
// parallel_group examples that write to the process's stdout/stderr
// descriptors (bypassing the captured stream, completion order varies) and
// priority_scheduler (std::priority_queue does not order equal priorities).
// boost/asio/yield.hpp / unyield.hpp (the stackless-coroutine macros) stay
// where the examples include them; BOOST_ASIO_COMPLETION_TOKEN_FOR(sig) is
// written as the completion_token_for<sig> concept it expands to; boost/asio/coroutine.hpp is included first
// so those in-place includes add only the macros.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/coroutine.hpp>
#include <cassert>
import std;
import mcpplibs.mwrap.boost.asio;

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
            while (!ec) s.read_some(boost::asio::buffer(buf), ec);
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
inline bool ex_try_connect_local(const std::string& path) {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
    boost::asio::io_context c;
    boost::asio::local::stream_protocol::socket s(c);
    boost::system::error_code ec;
    s.connect(boost::asio::local::stream_protocol::endpoint(path), ec);
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
        for (int i = 0; i < 500 && !ex_try_connect_local(port); ++i)  // (a probe that closes: the local servers do not report it)
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
    std::ofstream(std::filesystem::path(d) / "index.html") << "<html><body>boost-m</body></html>\n";
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
#if defined(BOOST_ASIO_HAS_PIPE)
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

// libs/asio/example/cpp20/invocation/completion_executor.cpp
namespace ex_cpp20_invocation_completion_executor {
using boost::asio::ip::tcp;

//----------------------------------------------------------------------

// The logging_executor class implements a minimal executor that satisfies the
// requirements for use as a completion executor. This means it may be bound to
// a handler using bind_executor.
class logging_executor
{
public:
  // All executors must be no-throw equality comparable.
  bool operator==(const logging_executor&) const noexcept = default;

  // All executors must provide a const member function execute().
  void execute(std::invocable auto handler) const
  {
    try
    {
      ex_out << "handler invocation starting\n";
      std::move(handler)();
      ex_out << "handler invocation complete\n";
    }
    catch (...)
    {
      ex_out << "handler invocation completed with exception\n";
      throw;
    }
  }
};

// Confirm that a logging_executor satisfies the executor requirements.
static_assert(boost::asio::execution::executor<logging_executor>);

// Confirm that a logging_executor can be used as a completion executor.
static_assert(std::convertible_to<
    logging_executor, boost::asio::any_completion_executor>);

//----------------------------------------------------------------------

int example_main()
{
  boost::asio::io_context io_context(1);

  // Post a completion handler to be run immediately.
  boost::asio::post(io_context,
      boost::asio::bind_executor(logging_executor{},
        []{ ex_out << "post complete\n"; }));

  // Start an asynchronous accept that will complete immediately.
  tcp::endpoint endpoint(boost::asio::ip::address_v4::loopback(), 0);
  tcp::acceptor acceptor(io_context, endpoint);
  tcp::socket server_socket(io_context);
  acceptor.async_accept(
      boost::asio::bind_executor(logging_executor{},
        [](auto...){ ex_out << "async_accept complete\n"; }));
  tcp::socket client_socket(io_context);
  client_socket.connect(acceptor.local_endpoint());

  // Set a timer to expire immediately.
  boost::asio::steady_timer timer(io_context);
  timer.expires_at(boost::asio::steady_timer::clock_type::time_point::min());
  timer.async_wait(
      boost::asio::bind_executor(logging_executor{},
        [](auto){ ex_out << "async_wait complete\n"; }));

  io_context.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/invocation/completion_executor") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(handler invocation starting
post complete
handler invocation complete
handler invocation starting
async_accept complete
handler invocation complete
handler invocation starting
async_wait complete
handler invocation complete
)ex"));
}
}  // namespace ex_cpp20_invocation_completion_executor

// libs/asio/example/cpp20/operations/c_callback_wrapper.cpp
namespace ex_cpp20_operations_c_callback_wrapper {
void read_input(const char* prompt, void (*cb)(void*, const char*), void* arg)
{
  std::thread(
      [prompt = std::string(prompt), cb, arg]
      {
        ex_out << prompt << ": ";
        ex_out.flush();
        std::string line;
        std::getline(ex_in, line);
        cb(arg, line.c_str());
      }).detach();
}

//------------------------------------------------------------------------------

// This is an asynchronous operation that wraps the C-based API.

// To map our completion handler into a function pointer / void* callback, we
// need to allocate some state that will live for the duration of the
// operation. A pointer to this state will be passed to the C-based API.
template <boost::asio::completion_handler_for<void(std::string)> Handler>
class read_input_state
{
public:
  read_input_state(Handler&& handler)
    : handler_(std::move(handler)),
      work_(boost::asio::make_work_guard(handler_))
  {
  }

  // Create the state using the handler's associated allocator.
  static read_input_state* create(Handler&& handler)
  {
    // A unique_ptr deleter that is used to destroy uninitialised objects.
    struct deleter
    {
      // Get the handler's associated allocator type. If the handler does not
      // specify an associated allocator, we will use a recycling allocator as
      // the default. As the associated allocator is a proto-allocator, we must
      // rebind it to the correct type before we can use it to allocate objects.
      typename std::allocator_traits<
        boost::asio::associated_allocator_t<Handler,
          boost::asio::recycling_allocator<void>>>::template
            rebind_alloc<read_input_state> alloc;

      void operator()(read_input_state* ptr)
      {
        std::allocator_traits<decltype(alloc)>::deallocate(alloc, ptr, 1);
      }
    } d{boost::asio::get_associated_allocator(handler,
          boost::asio::recycling_allocator<void>())};

    // Allocate memory for the state.
    std::unique_ptr<read_input_state, deleter> uninit_ptr(
        std::allocator_traits<decltype(d.alloc)>::allocate(d.alloc, 1), d);

    // Construct the state into the newly allocated memory. This might throw.
    read_input_state* ptr =
      new (uninit_ptr.get()) read_input_state(std::move(handler));

    // Release ownership of the memory and return the newly allocated state.
    uninit_ptr.release();
    return ptr;
  }

  static void callback(void* arg, const char* result)
  {
    read_input_state* self = static_cast<read_input_state*>(arg);

    // A unique_ptr deleter that is used to destroy initialised objects.
    struct deleter
    {
      // Get the handler's associated allocator type. If the handler does not
      // specify an associated allocator, we will use a recycling allocator as
      // the default. As the associated allocator is a proto-allocator, we must
      // rebind it to the correct type before we can use it to allocate objects.
      typename std::allocator_traits<
        boost::asio::associated_allocator_t<Handler,
          boost::asio::recycling_allocator<void>>>::template
            rebind_alloc<read_input_state> alloc;

      void operator()(read_input_state* ptr)
      {
        std::allocator_traits<decltype(alloc)>::destroy(alloc, ptr);
        std::allocator_traits<decltype(alloc)>::deallocate(alloc, ptr, 1);
      }
    } d{boost::asio::get_associated_allocator(self->handler_,
          boost::asio::recycling_allocator<void>())};

    // To conform to the rules regarding asynchronous operations and memory
    // allocation, we must make a copy of the state and deallocate the memory
    // before dispatching the completion handler.
    std::unique_ptr<read_input_state, deleter> state_ptr(self, d);
    read_input_state state(std::move(*self));
    state_ptr.reset();

    // Dispatch the completion handler through the handler's associated
    // executor, using the handler's associated allocator.
    boost::asio::dispatch(state.work_.get_executor(),
        boost::asio::bind_allocator(d.alloc,
          [
            handler = std::move(state.handler_),
            result = std::string(result)
          ]() mutable
          {
            std::move(handler)(result);
          }));
  }

private:
  Handler handler_;

  // According to the rules for asynchronous operations, we need to track
  // outstanding work against the handler's associated executor until the
  // asynchronous operation is complete.
  boost::asio::executor_work_guard<
    boost::asio::associated_executor_t<Handler>> work_;
};

// The initiating function for the asynchronous operation.
template <boost::asio::completion_token_for<void(std::string)> CompletionToken>
auto async_read_input(const std::string& prompt, CompletionToken&& token)
{
  // Define a function object that contains the code to launch the asynchronous
  // operation. This is passed the concrete completion handler, followed by any
  // additional arguments that were passed through the call to async_initiate.
  auto init = [](
      boost::asio::completion_handler_for<void(std::string)> auto handler,
      const std::string& prompt)
  {
    // The body of the initiation function object creates the long-lived state
    // and passes it to the C-based API, along with the function pointer.
    using state_type = read_input_state<decltype(handler)>;
    read_input(prompt.c_str(), &state_type::callback,
        state_type::create(std::move(handler)));
  };

  // The async_initiate function is used to transform the supplied completion
  // token to the completion handler. When calling this function we explicitly
  // specify the completion signature of the operation. We must also return the
  // result of the call since the completion token may produce a return value,
  // such as a future.
  return boost::asio::async_initiate<CompletionToken, void(std::string)>(
      init, // First, pass the function object that launches the operation,
      token, // then the completion token that will be transformed to a handler,
      prompt); // and, finally, any additional arguments to the function object.
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  // Test our asynchronous operation using a lambda as a callback. We will use
  // an io_context to obtain an associated executor.
  async_read_input("Enter your name",
      boost::asio::bind_executor(io_context,
        [](const std::string& result)
        {
          ex_out << "Hello " << result << "\n";
        }));

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_read_input("Enter your name", boost::asio::deferred);

  // Launch our asynchronous operation using a lambda as a callback. We will use
  // an io_context to obtain an associated executor.
  std::move(op)(
      boost::asio::bind_executor(io_context,
        [](const std::string& result)
        {
          ex_out << "Hello " << result << "\n";
        }));

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<std::string> f =
    async_read_input("Enter your name", boost::asio::use_future);

  std::string result = f.get();
  ex_out << "Hello " << result << "\n";
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/c_callback_wrapper") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(a line of input
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter your name: Hello a line of input
Enter your name: Hello 
Enter your name: Hello 
)ex"));
}
}  // namespace ex_cpp20_operations_c_callback_wrapper

// libs/asio/example/cpp20/operations/callback_wrapper.cpp
namespace ex_cpp20_operations_callback_wrapper {
template <typename Callback>
void read_input(const std::string& prompt, Callback cb)
{
  std::thread(
      [prompt, cb = std::move(cb)]() mutable
      {
        ex_out << prompt << ": ";
        ex_out.flush();
        std::string line;
        std::getline(ex_in, line);
        std::move(cb)(std::move(line));
      }).detach();
}

//------------------------------------------------------------------------------

// This is an asynchronous operation that wraps the callback-based API.

// The initiating function for the asynchronous operation.
template <boost::asio::completion_token_for<void(std::string)> CompletionToken>
auto async_read_input(const std::string& prompt, CompletionToken&& token)
{
  // Define a function object that contains the code to launch the asynchronous
  // operation. This is passed the concrete completion handler, followed by any
  // additional arguments that were passed through the call to async_initiate.
  auto init = [](
      boost::asio::completion_handler_for<void(std::string)> auto handler,
      const std::string& prompt)
  {
    // According to the rules for asynchronous operations, we need to track
    // outstanding work against the handler's associated executor until the
    // asynchronous operation is complete.
    auto work = boost::asio::make_work_guard(handler);

    // Launch the operation with a callback that will receive the result and
    // pass it through to the asynchronous operation's completion handler.
    read_input(prompt,
        [
          handler = std::move(handler),
          work = std::move(work)
        ](std::string result) mutable
        {
          // Get the handler's associated allocator. If the handler does not
          // specify an allocator, use the recycling allocator as the default.
          auto alloc = boost::asio::get_associated_allocator(
              handler, boost::asio::recycling_allocator<void>());

          // Dispatch the completion handler through the handler's associated
          // executor, using the handler's associated allocator.
          boost::asio::dispatch(work.get_executor(),
              boost::asio::bind_allocator(alloc,
                [
                  handler = std::move(handler),
                  result = std::string(result)
                ]() mutable
                {
                  std::move(handler)(result);
                }));
        });
  };

  // The async_initiate function is used to transform the supplied completion
  // token to the completion handler. When calling this function we explicitly
  // specify the completion signature of the operation. We must also return the
  // result of the call since the completion token may produce a return value,
  // such as a future.
  return boost::asio::async_initiate<CompletionToken, void(std::string)>(
      init, // First, pass the function object that launches the operation,
      token, // then the completion token that will be transformed to a handler,
      prompt); // and, finally, any additional arguments to the function object.
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  // Test our asynchronous operation using a lambda as a callback. We will use
  // an io_context to specify an associated executor.
  async_read_input("Enter your name",
      boost::asio::bind_executor(io_context,
        [](const std::string& result)
        {
          ex_out << "Hello " << result << "\n";
        }));

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  auto op = async_read_input("Enter your name", boost::asio::deferred);

  // Launch our asynchronous operation using a lambda as a callback. We will use
  // an io_context to obtain an associated executor.
  std::move(op)(
      boost::asio::bind_executor(io_context,
        [](const std::string& result)
        {
          ex_out << "Hello " << result << "\n";
        }));

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<std::string> f =
    async_read_input("Enter your name", boost::asio::use_future);

  std::string result = f.get();
  ex_out << "Hello " << result << "\n";
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/callback_wrapper") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(a line of input
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter your name: Hello a line of input
Enter your name: Hello 
Enter your name: Hello 
)ex"));
}
}  // namespace ex_cpp20_operations_callback_wrapper

// libs/asio/example/cpp20/operations/composed_1.cpp
namespace ex_cpp20_operations_composed_1 {
using boost::asio::ip::tcp;

//------------------------------------------------------------------------------

// This is the simplest example of a composed asynchronous operation, where we
// simply repackage an existing operation. The asynchronous operation
// requirements are met by delegating responsibility to the underlying
// operation.

template <
  boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)>
    CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is void.
  // However, when the completion token is boost::asio::yield_context (used for
  // stackful coroutines) the return type would be std::size_t, and when the
  // completion token is boost::asio::use_future it would be std::future<std::size_t>.
  // When the completion token is boost::asio::deferred, the return type differs for
  // each asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of our underlying asynchronous operation.
{
  // When delegating to the underlying operation we must take care to perfectly
  // forward the completion token. This ensures that our operation works
  // correctly with move-only function objects as callbacks, as well as other
  // completion token types.
  return boost::asio::async_write(socket,
      boost::asio::buffer(message, std::strlen(message)),
      std::forward<CompletionToken>(token));
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "Testing callback\r\n",
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_message(
      socket, "Testing deferred\r\n", boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<std::size_t> f = async_write_message(
      socket, "Testing future\r\n", boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    std::size_t n = f.get();
    ex_out << n << " bytes transferred\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(18 bytes transferred
18 bytes transferred
16 bytes transferred
)ex"));
}
}  // namespace ex_cpp20_operations_composed_1

// libs/asio/example/cpp20/operations/composed_2.cpp
namespace ex_cpp20_operations_composed_2 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This next simplest example of a composed asynchronous operation involves
// repackaging multiple operations but choosing to invoke just one of them. All
// of these underlying operations have the same completion signature. The
// asynchronous operation requirements are met by delegating responsibility to
// the underlying operations.

template <
  boost::asio::completion_token_for<void(boost::system::error_code, std::size_t)>
    CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, bool allow_partial_write,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is void.
  // However, when the completion token is boost::asio::yield_context (used for
  // stackful coroutines) the return type would be std::size_t, and when the
  // completion token is boost::asio::use_future it would be std::future<std::size_t>.
  // When the completion token is boost::asio::deferred, the return type differs for
  // each asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_initiate.
{
  // In addition to determining the mechanism by which an asynchronous
  // operation delivers its result, a completion token also determines the time
  // when the operation commences. For example, when the completion token is a
  // simple callback the operation commences before the initiating function
  // returns. However, if the completion token's delivery mechanism uses a
  // future, we might instead want to defer initiation of the operation until
  // the returned future object is waited upon.
  //
  // To enable this, when implementing an asynchronous operation we must
  // package the initiation step as a function object. The initiation function
  // object's call operator is passed the concrete completion handler produced
  // by the completion token. This completion handler matches the asynchronous
  // operation's completion handler signature, which in this example is:
  //
  //   void(boost::system::error_code error, std::size_t)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments in the lambda capture set. However, we should prefer to
  // propagate them as function call arguments as this allows the completion
  // token to optimise how they are passed. For example, a lazy future which
  // defers initiation would need to make a decay-copy of the arguments, but
  // when using a simple callback the arguments can be trivially forwarded
  // straight through.)
  auto initiation = [](
      boost::asio::completion_handler_for<void(boost::system::error_code, std::size_t)>
        auto&& completion_handler,
      tcp::socket& socket,
      const char* message,
      bool allow_partial_write)
  {
    if (allow_partial_write)
    {
      // When delegating to an underlying operation we must take care to
      // perfectly forward the completion handler. This ensures that our
      // operation works correctly with move-only function objects as
      // callbacks.
      return socket.async_write_some(
          boost::asio::buffer(message, std::strlen(message)),
          std::forward<decltype(completion_handler)>(completion_handler));
    }
    else
    {
      // As above, we must perfectly forward the completion handler when calling
      // the alternate underlying operation.
      return boost::asio::async_write(socket,
          boost::asio::buffer(message, std::strlen(message)),
          std::forward<decltype(completion_handler)>(completion_handler));
    }
  };

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code, std::size_t)>(
      initiation, token, std::ref(socket), message, allow_partial_write);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "Testing callback\r\n", false,
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_message(
      socket, "Testing deferred\r\n", false, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error, std::size_t n)
      {
        if (!error)
        {
          ex_out << n << " bytes transferred\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<std::size_t> f = async_write_message(
      socket, "Testing future\r\n", false, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    std::size_t n = f.get();
    ex_out << n << " bytes transferred\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(18 bytes transferred
18 bytes transferred
16 bytes transferred
)ex"));
}
}  // namespace ex_cpp20_operations_composed_2

// libs/asio/example/cpp20/operations/composed_3.cpp
namespace ex_cpp20_operations_composed_3 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// In this composed operation we repackage an existing operation, but with a
// different completion handler signature. The asynchronous operation
// requirements are met by delegating responsibility to the underlying
// operation.

template <
  boost::asio::completion_token_for<void(boost::system::error_code)> CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_initiate.
{
  // In addition to determining the mechanism by which an asynchronous
  // operation delivers its result, a completion token also determines the time
  // when the operation commences. For example, when the completion token is a
  // simple callback the operation commences before the initiating function
  // returns. However, if the completion token's delivery mechanism uses a
  // future, we might instead want to defer initiation of the operation until
  // the returned future object is waited upon.
  //
  // To enable this, when implementing an asynchronous operation we must
  // package the initiation step as a function object. The initiation function
  // object's call operator is passed the concrete completion handler produced
  // by the completion token. This completion handler matches the asynchronous
  // operation's completion handler signature, which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments in the lambda capture set. However, we should prefer to
  // propagate them as function call arguments as this allows the completion
  // token to optimise how they are passed. For example, a lazy future which
  // defers initiation would need to make a decay-copy of the arguments, but
  // when using a simple callback the arguments can be trivially forwarded
  // straight through.)
  auto initiation = [](
      boost::asio::completion_handler_for<void(boost::system::error_code)>
        auto&& completion_handler,
      tcp::socket& socket,
      const char* message)
  {
    // The async_write operation has a completion handler signature of:
    //
    //   void(boost::system::error_code error, std::size n)
    //
    // This differs from our operation's signature in that it is also passed
    // the number of bytes transferred as an argument of type std::size_t. We
    // will adapt our completion handler to async_write's completion handler
    // signature by using std::bind, which drops the additional argument.
    //
    // However, it is essential to the correctness of our composed operation
    // that we preserve the executor of the user-supplied completion handler.
    // The std::bind function will not do this for us, so we must do this by
    // first obtaining the completion handler's associated executor (defaulting
    // to the I/O executor - in this case the executor of the socket - if the
    // completion handler does not have its own) ...
    auto executor = boost::asio::get_associated_executor(
        completion_handler, socket.get_executor());

    // ... and then binding this executor to our adapted completion handler
    // using the boost::asio::bind_executor function.
    boost::asio::async_write(socket,
        boost::asio::buffer(message, std::strlen(message)),
        boost::asio::bind_executor(executor,
          std::bind(std::forward<decltype(completion_handler)>(
              completion_handler), std::placeholders::_1)));
  };

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      initiation, token, std::ref(socket), message);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "Testing callback\r\n",
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_message(
      socket, "Testing deferred\r\n", boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_message(
      socket, "Testing future\r\n", boost::asio::use_future);

  io_context.run();

  // Get the result of the operation.
  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Message sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_3") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Message sent
Message sent
Message sent
)ex"));
}
}  // namespace ex_cpp20_operations_composed_3

// libs/asio/example/cpp20/operations/composed_4.cpp
namespace ex_cpp20_operations_composed_4 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// In this composed operation we repackage an existing operation, but with a
// different completion handler signature. We will also intercept an empty
// message as an invalid argument, and propagate the corresponding error to the
// user. The asynchronous operation requirements are met by delegating
// responsibility to the underlying operation.

template <
  boost::asio::completion_token_for<void(boost::system::error_code)> CompletionToken>
auto async_write_message(tcp::socket& socket,
    const char* message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_initiate.
{
  // In addition to determining the mechanism by which an asynchronous
  // operation delivers its result, a completion token also determines the time
  // when the operation commences. For example, when the completion token is a
  // simple callback the operation commences before the initiating function
  // returns. However, if the completion token's delivery mechanism uses a
  // future, we might instead want to defer initiation of the operation until
  // the returned future object is waited upon.
  //
  // To enable this, when implementing an asynchronous operation we must
  // package the initiation step as a function object. The initiation function
  // object's call operator is passed the concrete completion handler produced
  // by the completion token. This completion handler matches the asynchronous
  // operation's completion handler signature, which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments in the lambda capture set. However, we should prefer to
  // propagate them as function call arguments as this allows the completion
  // token to optimise how they are passed. For example, a lazy future which
  // defers initiation would need to make a decay-copy of the arguments, but
  // when using a simple callback the arguments can be trivially forwarded
  // straight through.)
  auto initiation = [](
      boost::asio::completion_handler_for<void(boost::system::error_code)>
        auto&& completion_handler,
      tcp::socket& socket,
      const char* message)
  {
    // The post operation has a completion handler signature of:
    //
    //   void()
    //
    // and the async_write operation has a completion handler signature of:
    //
    //   void(boost::system::error_code error, std::size n)
    //
    // Both of these operations' completion handler signatures differ from our
    // operation's completion handler signature. We will adapt our completion
    // handler to these signatures by using std::bind, which drops the
    // additional arguments.
    //
    // However, it is essential to the correctness of our composed operation
    // that we preserve the executor of the user-supplied completion handler.
    // The std::bind function will not do this for us, so we must do this by
    // first obtaining the completion handler's associated executor (defaulting
    // to the I/O executor - in this case the executor of the socket - if the
    // completion handler does not have its own) ...
    auto executor = boost::asio::get_associated_executor(
        completion_handler, socket.get_executor());

    // ... and then binding this executor to our adapted completion handler
    // using the boost::asio::bind_executor function.
    std::size_t length = std::strlen(message);
    if (length == 0)
    {
      boost::asio::post(
          boost::asio::bind_executor(executor,
            std::bind(std::forward<decltype(completion_handler)>(
                completion_handler), boost::asio::error::invalid_argument)));
    }
    else
    {
      boost::asio::async_write(socket,
          boost::asio::buffer(message, length),
          boost::asio::bind_executor(executor,
            std::bind(std::forward<decltype(completion_handler)>(
                completion_handler), std::placeholders::_1)));
    }
  };

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      initiation, token, std::ref(socket), message);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, "",
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op =
    async_write_message(socket, "", boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_message(
      socket, "", boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Message sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_4") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Error: Invalid argument
Error: Invalid argument
Exception: Invalid argument [system:22]
)ex"));
}
}  // namespace ex_cpp20_operations_composed_4

// libs/asio/example/cpp20/operations/composed_5.cpp
namespace ex_cpp20_operations_composed_5 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation automatically serialises a message, using its I/O
// streams insertion operator, before sending it on the socket. To do this, it
// must allocate a buffer for the encoded message and ensure this buffer's
// validity until the underlying async_write operation completes.

template <typename T,
  boost::asio::completion_token_for<void(boost::system::error_code)> CompletionToken>
auto async_write_message(tcp::socket& socket,
    const T& message, CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_initiate.
{
  // In addition to determining the mechanism by which an asynchronous
  // operation delivers its result, a completion token also determines the time
  // when the operation commences. For example, when the completion token is a
  // simple callback the operation commences before the initiating function
  // returns. However, if the completion token's delivery mechanism uses a
  // future, we might instead want to defer initiation of the operation until
  // the returned future object is waited upon.
  //
  // To enable this, when implementing an asynchronous operation we must
  // package the initiation step as a function object. The initiation function
  // object's call operator is passed the concrete completion handler produced
  // by the completion token. This completion handler matches the asynchronous
  // operation's completion handler signature, which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments in the lambda capture set. However, we should prefer to
  // propagate them as function call arguments as this allows the completion
  // token to optimise how they are passed. For example, a lazy future which
  // defers initiation would need to make a decay-copy of the arguments, but
  // when using a simple callback the arguments can be trivially forwarded
  // straight through.)
  auto initiation = [](
      boost::asio::completion_handler_for<void(boost::system::error_code)>
        auto&& completion_handler,
      tcp::socket& socket,
      std::unique_ptr<std::string> encoded_message)
  {
    // In this example, the composed operation's intermediate completion
    // handler is implemented as a hand-crafted function object, rather than
    // using a lambda or std::bind.
    struct intermediate_completion_handler
    {
      // The intermediate completion handler holds a reference to the socket so
      // that it can obtain the I/O executor (see get_executor below).
      tcp::socket& socket_;

      // The allocated buffer for the encoded message. The std::unique_ptr
      // smart pointer is move-only, and as a consequence our intermediate
      // completion handler is also move-only.
      std::unique_ptr<std::string> encoded_message_;

      // The user-supplied completion handler.
      typename std::decay<decltype(completion_handler)>::type handler_;

      // The function call operator matches the completion signature of the
      // async_write operation.
      void operator()(const boost::system::error_code& error, std::size_t /*n*/)
      {
        // Deallocate the encoded message before calling the user-supplied
        // completion handler.
        encoded_message_.reset();

        // Call the user-supplied handler with the result of the operation.
        // The arguments must match the completion signature of our composed
        // operation.
        handler_(error);
      }

      // It is essential to the correctness of our composed operation that we
      // preserve the executor of the user-supplied completion handler. With a
      // hand-crafted function object we can do this by defining a nested type
      // executor_type and member function get_executor. These obtain the
      // completion handler's associated executor, and default to the I/O
      // executor - in this case the executor of the socket - if the completion
      // handler does not have its own.
      using executor_type = boost::asio::associated_executor_t<
          typename std::decay<decltype(completion_handler)>::type,
          tcp::socket::executor_type>;

      executor_type get_executor() const noexcept
      {
        return boost::asio::get_associated_executor(
            handler_, socket_.get_executor());
      }

      // Although not necessary for correctness, we may also preserve the
      // allocator of the user-supplied completion handler. This is achieved by
      // defining a nested type allocator_type and member function
      // get_allocator. These obtain the completion handler's associated
      // allocator, and default to std::allocator<void> if the completion
      // handler does not have its own.
      using allocator_type = boost::asio::associated_allocator_t<
          typename std::decay<decltype(completion_handler)>::type,
          std::allocator<void>>;

      allocator_type get_allocator() const noexcept
      {
        return boost::asio::get_associated_allocator(
            handler_, std::allocator<void>{});
      }
    };

    // Initiate the underlying async_write operation using our intermediate
    // completion handler.
    auto encoded_message_buffer = boost::asio::buffer(*encoded_message);
    boost::asio::async_write(socket, encoded_message_buffer,
        intermediate_completion_handler{socket, std::move(encoded_message),
          std::forward<decltype(completion_handler)>(completion_handler)});
  };

  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      initiation, token, std::ref(socket),
      std::move(encoded_message));
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_message(socket, 123456,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_message(
      socket, std::string("abcdef"), boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Message sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_message(
      socket, 654.321, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Message sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_5") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Message sent
Message sent
Message sent
)ex"));
}
}  // namespace ex_cpp20_operations_composed_5

// libs/asio/example/cpp20/operations/composed_6.cpp
namespace ex_cpp20_operations_composed_6 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation shows composition of multiple underlying operations.
// It automatically serialises a message, using its I/O streams insertion
// operator, before sending it N times on the socket. To do this, it must
// allocate a buffer for the encoded message and ensure this buffer's validity
// until all underlying async_write operation complete. A one second delay is
// inserted prior to each write operation, using a steady_timer.

template <typename T,
  boost::asio::completion_token_for<void(boost::system::error_code)> CompletionToken>
auto async_write_messages(tcp::socket& socket,
    const T& message, std::size_t repeat_count,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_initiate.
{
  // In addition to determining the mechanism by which an asynchronous
  // operation delivers its result, a completion token also determines the time
  // when the operation commences. For example, when the completion token is a
  // simple callback the operation commences before the initiating function
  // returns. However, if the completion token's delivery mechanism uses a
  // future, we might instead want to defer initiation of the operation until
  // the returned future object is waited upon.
  //
  // To enable this, when implementing an asynchronous operation we must
  // package the initiation step as a function object. The initiation function
  // object's call operator is passed the concrete completion handler produced
  // by the completion token. This completion handler matches the asynchronous
  // operation's completion handler signature, which in this example is:
  //
  //   void(boost::system::error_code error)
  //
  // The initiation function object also receives any additional arguments
  // required to start the operation. (Note: We could have instead passed these
  // arguments in the lambda capture set. However, we should prefer to
  // propagate them as function call arguments as this allows the completion
  // token to optimise how they are passed. For example, a lazy future which
  // defers initiation would need to make a decay-copy of the arguments, but
  // when using a simple callback the arguments can be trivially forwarded
  // straight through.)
  auto initiation = [](
      boost::asio::completion_handler_for<void(boost::system::error_code)>
        auto&& completion_handler,
      tcp::socket& socket,
      std::unique_ptr<std::string> encoded_message,
      std::size_t repeat_count,
      std::unique_ptr<boost::asio::steady_timer> delay_timer)
  {
    // In this example, the composed operation's intermediate completion
    // handler is implemented as a hand-crafted function object.
    struct intermediate_completion_handler
    {
      // The intermediate completion handler holds a reference to the socket as
      // it is used for multiple async_write operations, as well as for
      // obtaining the I/O executor (see get_executor below).
      tcp::socket& socket_;

      // The allocated buffer for the encoded message. The std::unique_ptr
      // smart pointer is move-only, and as a consequence our intermediate
      // completion handler is also move-only.
      std::unique_ptr<std::string> encoded_message_;

      // The repeat count remaining.
      std::size_t repeat_count_;

      // A steady timer used for introducing a delay.
      std::unique_ptr<boost::asio::steady_timer> delay_timer_;

      // To manage the cycle between the multiple underlying asynchronous
      // operations, our intermediate completion handler is implemented as a
      // state machine.
      enum { starting, waiting, writing } state_;

      // As our composed operation performs multiple underlying I/O operations,
      // we should maintain a work object against the I/O executor. This tells
      // the I/O executor that there is still more work to come in the future.
      boost::asio::executor_work_guard<tcp::socket::executor_type> io_work_;

      // The user-supplied completion handler, called once only on completion
      // of the entire composed operation.
      typename std::decay<decltype(completion_handler)>::type handler_;

      // By having a default value for the second argument, this function call
      // operator matches the completion signature of both the async_write and
      // steady_timer::async_wait operations.
      void operator()(const boost::system::error_code& error, std::size_t = 0)
      {
        if (!error)
        {
          switch (state_)
          {
          case starting:
          case writing:
            if (repeat_count_ > 0)
            {
              --repeat_count_;
              state_ = waiting;
              delay_timer_->expires_after(std::chrono::milliseconds(10));
              delay_timer_->async_wait(std::move(*this));
              return; // Composed operation not yet complete.
            }
            break; // Composed operation complete, continue below.
          case waiting:
            state_ = writing;
            boost::asio::async_write(socket_,
                boost::asio::buffer(*encoded_message_), std::move(*this));
            return; // Composed operation not yet complete.
          }
        }

        // This point is reached only on completion of the entire composed
        // operation.

        // We no longer have any future work coming for the I/O executor.
        io_work_.reset();

        // Deallocate the encoded message and delay timer before calling the
        // user-supplied completion handler.
        encoded_message_.reset();
        delay_timer_.reset();

        // Call the user-supplied handler with the result of the operation.
        handler_(error);
      }

      // It is essential to the correctness of our composed operation that we
      // preserve the executor of the user-supplied completion handler. With a
      // hand-crafted function object we can do this by defining a nested type
      // executor_type and member function get_executor. These obtain the
      // completion handler's associated executor, and default to the I/O
      // executor - in this case the executor of the socket - if the completion
      // handler does not have its own.
      using executor_type = boost::asio::associated_executor_t<
          typename std::decay<decltype(completion_handler)>::type,
          tcp::socket::executor_type>;

      executor_type get_executor() const noexcept
      {
        return boost::asio::get_associated_executor(
            handler_, socket_.get_executor());
      }

      // Although not necessary for correctness, we may also preserve the
      // allocator of the user-supplied completion handler. This is achieved by
      // defining a nested type allocator_type and member function
      // get_allocator. These obtain the completion handler's associated
      // allocator, and default to std::allocator<void> if the completion
      // handler does not have its own.
      using allocator_type = boost::asio::associated_allocator_t<
          typename std::decay<decltype(completion_handler)>::type,
          std::allocator<void>>;

      allocator_type get_allocator() const noexcept
      {
        return boost::asio::get_associated_allocator(
            handler_, std::allocator<void>{});
      }
    };

    // Initiate the underlying async_write operation using our intermediate
    // completion handler.
    auto encoded_message_buffer = boost::asio::buffer(*encoded_message);
    boost::asio::async_write(socket, encoded_message_buffer,
        intermediate_completion_handler{
          socket, std::move(encoded_message),
          repeat_count, std::move(delay_timer),
          intermediate_completion_handler::starting,
          boost::asio::make_work_guard(socket.get_executor()),
          std::forward<decltype(completion_handler)>(completion_handler)});
  };

  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the composed asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // Create a steady_timer to be used for the delay between messages.
  std::unique_ptr<boost::asio::steady_timer> delay_timer(
      new boost::asio::steady_timer(socket.get_executor()));

  // The boost::asio::async_initiate function takes:
  //
  // - our initiation function object,
  // - the completion token,
  // - the completion handler signature, and
  // - any additional arguments we need to initiate the operation.
  //
  // It then asks the completion token to create a completion handler (i.e. a
  // callback) with the specified signature, and invoke the initiation function
  // object with this completion handler as well as the additional arguments.
  // The return value of async_initiate is the result of our operation's
  // initiating function.
  //
  // Note that we wrap non-const reference arguments in std::reference_wrapper
  // to prevent incorrect decay-copies of these objects.
  return boost::asio::async_initiate<
    CompletionToken, void(boost::system::error_code)>(
      initiation, token, std::ref(socket),
      std::move(encoded_message), repeat_count,
      std::move(delay_timer));
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_messages(socket, "Testing callback\r\n", 5,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_messages(
      socket, "Testing deferred\r\n", 5, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_messages(
      socket, "Testing future\r\n", 5, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Messages sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_6") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Messages sent
Messages sent
Messages sent
)ex"));
}
}  // namespace ex_cpp20_operations_composed_6

// libs/asio/example/cpp20/operations/composed_7.cpp
namespace ex_cpp20_operations_composed_7 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_compose function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation shows composition of multiple underlying operations.
// It automatically serialises a message, using its I/O streams insertion
// operator, before sending it N times on the socket. To do this, it must
// allocate a buffer for the encoded message and ensure this buffer's validity
// until all underlying async_write operation complete. A one second delay is
// inserted prior to each write operation, using a steady_timer.

template <typename T,
  boost::asio::completion_token_for<void(boost::system::error_code)> CompletionToken>
auto async_write_messages(tcp::socket& socket,
    const T& message, std::size_t repeat_count,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_compose.
{
  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the composed asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // Create a steady_timer to be used for the delay between messages.
  std::unique_ptr<boost::asio::steady_timer> delay_timer(
      new boost::asio::steady_timer(socket.get_executor()));

  // To manage the cycle between the multiple underlying asynchronous
  // operations, our implementation is a state machine.
  enum { starting, waiting, writing };

  // The boost::asio::async_compose function takes:
  //
  // - our asynchronous operation implementation,
  // - the completion token,
  // - the completion handler signature, and
  // - any I/O objects (or executors) used by the operation
  //
  // It then wraps our implementation, which is implemented here as a state
  // machine in a lambda, in an intermediate completion handler that meets the
  // requirements of a conforming asynchronous operation. This includes
  // tracking outstanding work against the I/O executors associated with the
  // operation (in this example, this is the socket's executor).
  //
  // The first argument to our lambda is a reference to the enclosing
  // intermediate completion handler. This intermediate completion handler is
  // provided for us by the boost::asio::async_compose function, and takes care
  // of all the details required to implement a conforming asynchronous
  // operation. When calling an underlying asynchronous operation, we pass it
  // this enclosing intermediate completion handler as the completion token.
  //
  // All arguments to our lambda after the first must be defaulted to allow the
  // state machine to be started, as well as to allow the completion handler to
  // match the completion signature of both the async_write and
  // steady_timer::async_wait operations.
  return boost::asio::async_compose<
    CompletionToken, void(boost::system::error_code)>(
      [
        // The implementation holds a reference to the socket as it is used for
        // multiple async_write operations.
        &socket,

        // The allocated buffer for the encoded message. The std::unique_ptr
        // smart pointer is move-only, and as a consequence our lambda
        // implementation is also move-only.
        encoded_message = std::move(encoded_message),

        // The repeat count remaining.
        repeat_count,

        // A steady timer used for introducing a delay.
        delay_timer = std::move(delay_timer),

        // To manage the cycle between the multiple underlying asynchronous
        // operations, our implementation is a state machine.
        state = starting
      ]
      (
        auto& self,
        const boost::system::error_code& error = {},
        std::size_t = 0
      ) mutable
      {
        if (!error)
        {
          switch (state)
          {
          case starting:
          case writing:
            if (repeat_count > 0)
            {
              --repeat_count;
              state = waiting;
              delay_timer->expires_after(std::chrono::milliseconds(10));
              delay_timer->async_wait(std::move(self));
              return; // Composed operation not yet complete.
            }
            break; // Composed operation complete, continue below.
          case waiting:
            state = writing;
            boost::asio::async_write(socket,
                boost::asio::buffer(*encoded_message), std::move(self));
            return; // Composed operation not yet complete.
          }
        }

        // This point is reached only on completion of the entire composed
        // operation.

        // Deallocate the encoded message and delay timer before calling the
        // user-supplied completion handler.
        encoded_message.reset();
        delay_timer.reset();

        // Call the user-supplied handler with the result of the operation.
        self.complete(error);
      },
      token, socket);
}

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_messages(socket, "Testing callback\r\n", 5,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_messages(
      socket, "Testing deferred\r\n", 5, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_messages(
      socket, "Testing future\r\n", 5, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Messages sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_7") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Messages sent
Messages sent
Messages sent
)ex"));
}
}  // namespace ex_cpp20_operations_composed_7

// libs/asio/example/cpp20/operations/composed_8.cpp
namespace ex_cpp20_operations_composed_8 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_compose function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation shows composition of multiple underlying operations,
// using asio's stackless coroutines support to express the flow of control. It
// automatically serialises a message, using its I/O streams insertion
// operator, before sending it N times on the socket. To do this, it must
// allocate a buffer for the encoded message and ensure this buffer's validity
// until all underlying async_write operation complete. A one second delay is
// inserted prior to each write operation, using a steady_timer.

#include <boost/asio/yield.hpp>

template <typename T,
  boost::asio::completion_token_for<void(boost::system::error_code)> CompletionToken>
auto async_write_messages(tcp::socket& socket,
    const T& message, std::size_t repeat_count,
    CompletionToken&& token)
  // The return type of the initiating function is deduced from the combination
  // of:
  //
  // - the CompletionToken type,
  // - the completion handler signature, and
  // - the asynchronous operation's initiation function object.
  //
  // When the completion token is a simple callback, the return type is always
  // void. In this example, when the completion token is boost::asio::yield_context
  // (used for stackful coroutines) the return type would also be void, as
  // there is no non-error argument to the completion handler. When the
  // completion token is boost::asio::use_future it would be std::future<void>. When
  // the completion token is boost::asio::deferred, the return type differs for each
  // asynchronous operation.
  //
  // In C++20 we can omit the return type as it is automatically deduced from
  // the return type of boost::asio::async_compose.
{
  // Encode the message and copy it into an allocated buffer. The buffer will
  // be maintained for the lifetime of the composed asynchronous operation.
  std::ostringstream os;
  os << message;
  std::unique_ptr<std::string> encoded_message(new std::string(os.str()));

  // Create a steady_timer to be used for the delay between messages.
  std::unique_ptr<boost::asio::steady_timer> delay_timer(
      new boost::asio::steady_timer(socket.get_executor()));

  // The boost::asio::async_compose function takes:
  //
  // - our asynchronous operation implementation,
  // - the completion token,
  // - the completion handler signature, and
  // - any I/O objects (or executors) used by the operation
  //
  // It then wraps our implementation, which is implemented here as a stackless
  // coroutine in a lambda, in an intermediate completion handler that meets the
  // requirements of a conforming asynchronous operation. This includes
  // tracking outstanding work against the I/O executors associated with the
  // operation (in this example, this is the socket's executor).
  //
  // The first argument to our lambda is a reference to the enclosing
  // intermediate completion handler. This intermediate completion handler is
  // provided for us by the boost::asio::async_compose function, and takes care
  // of all the details required to implement a conforming asynchronous
  // operation. When calling an underlying asynchronous operation, we pass it
  // this enclosing intermediate completion handler as the completion token.
  //
  // All arguments to our lambda after the first must be defaulted to allow the
  // state machine to be started, as well as to allow the completion handler to
  // match the completion signature of both the async_write and
  // steady_timer::async_wait operations.
  return boost::asio::async_compose<
    CompletionToken, void(boost::system::error_code)>(
      [
        // The implementation holds a reference to the socket as it is used for
        // multiple async_write operations.
        &socket,

        // The allocated buffer for the encoded message. The std::unique_ptr
        // smart pointer is move-only, and as a consequence our lambda
        // implementation is also move-only.
        encoded_message = std::move(encoded_message),

        // The repeat count remaining.
        repeat_count,

        // A steady timer used for introducing a delay.
        delay_timer = std::move(delay_timer),

        // The coroutine state.
        coro = boost::asio::coroutine()
      ]
      (
        auto& self,
        const boost::system::error_code& error = {},
        std::size_t = 0
      ) mutable
      {
        reenter (coro)
        {
          while (repeat_count > 0)
          {
            --repeat_count;

            delay_timer->expires_after(std::chrono::milliseconds(10));
            yield delay_timer->async_wait(std::move(self));
            if (error)
              break;

            yield boost::asio::async_write(socket,
                boost::asio::buffer(*encoded_message), std::move(self));
            if (error)
              break;
          }

          // Deallocate the encoded message and delay timer before calling the
          // user-supplied completion handler.
          encoded_message.reset();
          delay_timer.reset();

          // Call the user-supplied handler with the result of the operation.
          self.complete(error);
        }
      },
      token, socket);
}

#include <boost/asio/unyield.hpp>

//------------------------------------------------------------------------------

void test_callback()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using a lambda as a callback.
  async_write_messages(socket, "Testing callback\r\n", 5,
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_deferred()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the deferred completion token. This
  // token causes the operation's initiating function to package up the
  // operation with its arguments to return a function object, which may then be
  // used to launch the asynchronous operation.
  boost::asio::async_operation auto op = async_write_messages(
      socket, "Testing deferred\r\n", 5, boost::asio::deferred);

  // Launch the operation using a lambda as a callback.
  std::move(op)(
      [](const boost::system::error_code& error)
      {
        if (!error)
        {
          ex_out << "Messages sent\n";
        }
        else
        {
          ex_out << "Error: " << error.message() << "\n";
        }
      });

  io_context.run();
}

//------------------------------------------------------------------------------

void test_future()
{
  boost::asio::io_context io_context;

  tcp::acceptor acceptor(io_context, {tcp::v4(), ex_port_number()});
  tcp::socket socket = acceptor.accept();

  // Test our asynchronous operation using the use_future completion token.
  // This token causes the operation's initiating function to return a future,
  // which may be used to synchronously wait for the result of the operation.
  std::future<void> f = async_write_messages(
      socket, "Testing future\r\n", 5, boost::asio::use_future);

  io_context.run();

  try
  {
    // Get the result of the operation.
    f.get();
    ex_out << "Messages sent\n";
  }
  catch (const std::exception& e)
  {
    ex_out << "Error: " << e.what() << "\n";
  }
}

//------------------------------------------------------------------------------

int example_main()
{
  test_callback();
  test_deferred();
  test_future();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/operations/composed_8") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_peer_async(ex_port, 3);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Messages sent
Messages sent
Messages sent
)ex"));
}
}  // namespace ex_cpp20_operations_composed_8

// libs/asio/example/cpp20/type_erasure/main.cpp
namespace ex_cpp20_type_erasure_main {
#undef LINE_READER_HPP
//
// line_reader.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef LINE_READER_HPP
#define LINE_READER_HPP


class line_reader
{
public:
  virtual ~line_reader() {}

  template <typename CompletionToken>
  auto async_read_line(std::string prompt, CompletionToken&& token)
  {
    return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code, std::string)>(
        [](auto handler, line_reader* self, std::string prompt)
        {
          self->async_read_line_impl(std::move(prompt), std::move(handler));
        }, token, this, prompt);
  }

private:
  virtual void async_read_line_impl(std::string prompt,
      boost::asio::any_completion_handler<void(boost::system::error_code, std::string)> handler) = 0;
};

#endif // LINE_READER_HPP
// ---- end line_reader.hpp ----
// ---- inlined sleep.hpp ----
#undef SLEEP_HPP
//
// sleep.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SLEEP_HPP
#define SLEEP_HPP


void async_sleep_impl(
    boost::asio::any_completion_handler<void(boost::system::error_code)> handler,
    boost::asio::any_io_executor ex, std::chrono::nanoseconds duration);

template <typename CompletionToken>
inline auto async_sleep(boost::asio::any_io_executor ex,
    std::chrono::nanoseconds duration, CompletionToken&& token)
{
  return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code)>(
      async_sleep_impl, token, std::move(ex), duration);
}

#endif // SLEEP_HPP
// ---- end sleep.hpp ----
// ---- inlined stdin_line_reader.hpp ----
#undef STDIN_LINE_READER_HPP
//
// stdin_line_reader.hpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef STDIN_LINE_READER_HPP
#define STDIN_LINE_READER_HPP


class stdin_line_reader : public line_reader
{
public:
  explicit stdin_line_reader(boost::asio::any_io_executor ex);

private:
  void async_read_line_impl(std::string prompt,
      boost::asio::any_completion_handler<void(boost::system::error_code, std::string)> handler) override;

  boost::asio::posix::stream_descriptor stdin_;
  std::string buffer_;
};

#endif
// ---- end stdin_line_reader.hpp ----

boost::asio::awaitable<void> do_read(line_reader& reader)
{
  for (int i = 0; i < 10; ++i)
  {
    ex_out << co_await reader.async_read_line("Enter something: ", boost::asio::use_awaitable);
    co_await async_sleep(co_await boost::asio::this_coro::executor, std::chrono::seconds(1), boost::asio::use_awaitable);
  }
}

int example_main()
{
  boost::asio::io_context ctx{1};
  stdin_line_reader reader{ctx.get_executor()};
  co_spawn(ctx, do_read(reader), boost::asio::detached);
  ctx.run();
    return 0;  // main's implicit return
}
// ---- merged sleep.cpp (the same program) ----
//
// sleep.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


void async_sleep_impl(
    boost::asio::any_completion_handler<void(boost::system::error_code)> handler,
    boost::asio::any_io_executor ex, std::chrono::nanoseconds duration)
{
  auto timer = std::make_shared<boost::asio::steady_timer>(ex, duration);
  timer->async_wait(boost::asio::consign(std::move(handler), timer));
}
// ---- merged stdin_line_reader.cpp (the same program) ----
//
// stdin_line_reader.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


stdin_line_reader::stdin_line_reader(boost::asio::any_io_executor ex)
  : stdin_(ex, ex_stdin_fd(ex_in.str()))
{
}

void stdin_line_reader::async_read_line_impl(std::string prompt,
    boost::asio::any_completion_handler<void(boost::system::error_code, std::string)> handler)
{
  ex_out << prompt;
  ex_out.flush();

  boost::asio::async_read_until(stdin_, boost::asio::dynamic_buffer(buffer_), '\n',
      boost::asio::deferred(
        [this](boost::system::error_code ec, std::size_t n)
        {
          if (!ec)
          {
            std::string result = buffer_.substr(0, n);
            buffer_.erase(0, n);
            return boost::asio::deferred.values(ec, std::move(result));
          }
          else
          {
            return boost::asio::deferred.values(ec, std::string{});
          }
        }
      )
    )(std::move(handler));
}

TEST_CASE("example: cpp20/type_erasure/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(line one
line two
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter something: line one
Enter something: line two
Enter something: )ex"));
}
}  // namespace ex_cpp20_type_erasure_main
