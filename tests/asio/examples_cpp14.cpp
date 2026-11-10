// mcpplibs.mwrap.boost.asio — the official cpp14 examples (libs/asio/example/cpp14/)
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
#include <boost/config.hpp>
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

// libs/asio/example/cpp14/deferred/deferred_1.cpp
namespace ex_cpp14_deferred_deferred_1 {
using boost::asio::deferred;

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  auto deferred_op = timer.async_wait(deferred);

  std::move(deferred_op)(
      [](boost::system::error_code ec)
      {
        ex_out << "timer wait finished: " << ec.message() << "\n";
      }
    );

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(timer wait finished: Success
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_1

// libs/asio/example/cpp14/deferred/deferred_2.cpp
namespace ex_cpp14_deferred_deferred_2 {
using boost::asio::deferred;

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  auto deferred_op = timer.async_wait(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "first timer wait finished: " << ec.message() << "\n";
          timer.expires_after(std::chrono::seconds(1));
          return timer.async_wait(deferred);
        }
      )
    );

  std::move(deferred_op)(
      [](boost::system::error_code ec)
      {
        ex_out << "second timer wait finished: " << ec.message() << "\n";
      }
    );

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_2

// libs/asio/example/cpp14/deferred/deferred_3.cpp
namespace ex_cpp14_deferred_deferred_3 {
using boost::asio::deferred;

template <typename CompletionToken>
auto async_wait_twice(boost::asio::steady_timer& timer, CompletionToken&& token)
{
  return timer.async_wait(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "first timer wait finished: " << ec.message() << "\n";
          timer.expires_after(std::chrono::seconds(1));
          return timer.async_wait(deferred);
        }
      )
    )(
      std::forward<CompletionToken>(token)
    );
}

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  async_wait_twice(
      timer,
      [](boost::system::error_code ec)
      {
        ex_out << "second timer wait finished: " << ec.message() << "\n";
      }
    );

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_3") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_3

// libs/asio/example/cpp14/deferred/deferred_4.cpp
namespace ex_cpp14_deferred_deferred_4 {
using boost::asio::deferred;

template <typename CompletionToken>
auto async_wait_twice(boost::asio::steady_timer& timer, CompletionToken&& token)
{
  return timer.async_wait(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "first timer wait finished: " << ec.message() << "\n";
          timer.expires_after(std::chrono::seconds(1));
          return timer.async_wait(deferred);
        }
      )
    )(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "second timer wait finished: " << ec.message() << "\n";
          return deferred.values(42);
        }
      )
    )(
      std::forward<CompletionToken>(token)
    );
}

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  async_wait_twice(
      timer,
      [](int result)
      {
        ex_out << "result is " << result << "\n";
      }
    );

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_4") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
result is 42
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_4

// libs/asio/example/cpp14/deferred/deferred_5.cpp
namespace ex_cpp14_deferred_deferred_5 {
using boost::asio::deferred;

template <typename CompletionToken>
auto async_wait_twice(boost::asio::steady_timer& timer, CompletionToken&& token)
{
  return timer.async_wait(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "first timer wait finished: " << ec.message() << "\n";
          timer.expires_after(std::chrono::seconds(1));
          return deferred.when(!ec)
            .then(timer.async_wait(deferred))
            .otherwise(deferred.values(ec));
        }
      )
    )(
      deferred(
        [&](boost::system::error_code ec)
        {
          ex_out << "second timer wait finished: " << ec.message() << "\n";
          return deferred.when(!ec)
            .then(deferred.values(42))
            .otherwise(deferred.values(0));
        }
      )
    )(
      std::forward<CompletionToken>(token)
    );
}

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  async_wait_twice(
      timer,
      [](int result)
      {
        ex_out << "result is " << result << "\n";
      }
    );

  // Uncomment the following line to trigger an error in async_wait_twice.
  //timer.cancel();

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_5") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
result is 42
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_5

// libs/asio/example/cpp14/deferred/deferred_6.cpp
namespace ex_cpp14_deferred_deferred_6 {
using boost::asio::append;
using boost::asio::deferred;

template <typename CompletionToken>
auto async_wait_twice(boost::asio::steady_timer& timer, CompletionToken&& token)
{
  return deferred.values(&timer)(
      deferred(
        [](boost::asio::steady_timer* timer)
        {
          timer->expires_after(std::chrono::seconds(1));
          return timer->async_wait(append(deferred, timer));
        }
      )
    )(
      deferred(
        [](boost::system::error_code ec, boost::asio::steady_timer* timer)
        {
          ex_out << "first timer wait finished: " << ec.message() << "\n";
          timer->expires_after(std::chrono::seconds(1));
          return deferred.when(!ec)
            .then(timer->async_wait(deferred))
            .otherwise(deferred.values(ec));
        }
      )
    )(
      deferred(
        [](boost::system::error_code ec)
        {
          ex_out << "second timer wait finished: " << ec.message() << "\n";
          return deferred.when(!ec)
            .then(deferred.values(42))
            .otherwise(deferred.values(0));
        }
      )
    )(
      std::forward<CompletionToken>(token)
    );
}

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  async_wait_twice(
      timer,
      [](int result)
      {
        ex_out << "result is " << result << "\n";
      }
    );

  // Uncomment the following line to trigger an error in async_wait_twice.
  //timer.cancel();

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_6") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
result is 42
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_6

// libs/asio/example/cpp14/deferred/deferred_7.cpp
namespace ex_cpp14_deferred_deferred_7 {
using boost::asio::append;
using boost::asio::deferred;

template <typename CompletionToken>
auto async_wait_twice(boost::asio::steady_timer& timer, CompletionToken&& token)
{
  return deferred.values(&timer)
    | deferred(
        [](boost::asio::steady_timer* timer)
        {
          timer->expires_after(std::chrono::seconds(1));
          return timer->async_wait(append(deferred, timer));
        }
      )
    | deferred(
        [](boost::system::error_code ec, boost::asio::steady_timer* timer)
        {
          ex_out << "first timer wait finished: " << ec.message() << "\n";
          timer->expires_after(std::chrono::seconds(1));
          return deferred.when(!ec)
            .then(timer->async_wait(deferred))
            .otherwise(deferred.values(ec));
        }
      )
    | deferred(
        [](boost::system::error_code ec)
        {
          ex_out << "second timer wait finished: " << ec.message() << "\n";
          return deferred.when(!ec)
            .then(deferred.values(42))
            .otherwise(deferred.values(0));
        }
      )
    | std::forward<CompletionToken>(token);
}

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::steady_timer timer(ctx);
  timer.expires_after(std::chrono::seconds(1));

  async_wait_twice(
      timer,
      [](int result)
      {
        ex_out << "result is " << result << "\n";
      }
    );

  // Uncomment the following line to trigger an error in async_wait_twice.
  //timer.cancel();

  ctx.run();

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/deferred/deferred_7") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(first timer wait finished: Success
second timer wait finished: Success
result is 42
)ex"));
}
}  // namespace ex_cpp14_deferred_deferred_7

// libs/asio/example/cpp14/executors/actor.cpp
namespace ex_cpp14_executors_actor {
using boost::asio::any_io_executor;
using boost::asio::defer;
using boost::asio::post;
using boost::asio::strand;
using boost::asio::system_executor;

//------------------------------------------------------------------------------
// A tiny actor framework
// ~~~~~~~~~~~~~~~~~~~~~~

class actor;

// Used to identify the sender and recipient of messages.
typedef actor* actor_address;

// Base class for all registered message handlers.
class message_handler_base
{
public:
  virtual ~message_handler_base() {}

  // Used to determine which message handlers receive an incoming message.
  virtual const std::type_info& message_id() const = 0;
};

// Base class for a handler for a specific message type.
template <class Message>
class message_handler : public message_handler_base
{
public:
  // Handle an incoming message.
  virtual void handle_message(Message msg, actor_address from) = 0;
};

// Concrete message handler for a specific message type.
template <class Actor, class Message>
class mf_message_handler : public message_handler<Message>
{
public:
  // Construct a message handler to invoke the specified member function.
  mf_message_handler(void (Actor::* mf)(Message, actor_address), Actor* a)
    : function_(mf), actor_(a)
  {
  }

  // Used to determine which message handlers receive an incoming message.
  virtual const std::type_info& message_id() const
  {
    return typeid(Message);
  }

  // Handle an incoming message.
  virtual void handle_message(Message msg, actor_address from)
  {
    (actor_->*function_)(std::move(msg), from);
  }

  // Determine whether the message handler represents the specified function.
  bool is_function(void (Actor::* mf)(Message, actor_address)) const
  {
    return mf == function_;
  }

private:
  void (Actor::* function_)(Message, actor_address);
  Actor* actor_;
};

// Base class for all actors.
class actor
{
public:
  virtual ~actor()
  {
  }

  // Obtain the actor's address for use as a message sender or recipient.
  actor_address address()
  {
    return this;
  }

  // Send a message from one actor to another.
  template <class Message>
  friend void send(Message msg, actor_address from, actor_address to)
  {
    // Execute the message handler in the context of the target's executor.
    post(to->executor_,
      [=, msg=std::move(msg)]() mutable
      {
        to->call_handler(std::move(msg), from);
      });
  }

protected:
  // Construct the actor to use the specified executor for all message handlers.
  actor(any_io_executor e)
    : executor_(std::move(e))
  {
  }

  // Register a handler for a specific message type. Duplicates are permitted.
  template <class Actor, class Message>
  void register_handler(void (Actor::* mf)(Message, actor_address))
  {
    handlers_.push_back(
      std::make_shared<mf_message_handler<Actor, Message>>(
        mf, static_cast<Actor*>(this)));
  }

  // Deregister a handler. Removes only the first matching handler.
  template <class Actor, class Message>
  void deregister_handler(void (Actor::* mf)(Message, actor_address))
  {
    const std::type_info& id = typeid(Message);
    for (auto iter = handlers_.begin(); iter != handlers_.end(); ++iter)
    {
      if ((*iter)->message_id() == id)
      {
        auto mh = static_cast<mf_message_handler<Actor, Message>*>(iter->get());
        if (mh->is_function(mf))
        {
          handlers_.erase(iter);
          return;
        }
      }
    }
  }

  // Send a message from within a message handler.
  template <class Message>
  void tail_send(Message msg, actor_address to)
  {
    // Execute the message handler in the context of the target's executor.
    defer(to->executor_,
      [=, msg=std::move(msg), from=this]
      {
        to->call_handler(std::move(msg), from);
      });
  }

private:
  // Find the matching message handlers, if any, and call them.
  template <class Message>
  void call_handler(Message msg, actor_address from)
  {
    const std::type_info& message_id = typeid(Message);
    for (auto& h: handlers_)
    {
      if (h->message_id() == message_id)
      {
        auto mh = static_cast<message_handler<Message>*>(h.get());
        mh->handle_message(msg, from);
      }
    }
  }

  // All messages associated with a single actor object should be processed
  // non-concurrently. We use a strand to ensure non-concurrent execution even
  // if the underlying executor may use multiple threads.
  strand<any_io_executor> executor_;

  std::vector<std::shared_ptr<message_handler_base>> handlers_;
};

// A concrete actor that allows synchronous message retrieval.
template <class Message>
class receiver : public actor
{
public:
  receiver()
    : actor(system_executor())
  {
    register_handler(&receiver::message_handler);
  }

  // Block until a message has been received.
  Message wait()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.wait(lock, [this]{ return !message_queue_.empty(); });
    Message msg(std::move(message_queue_.front()));
    message_queue_.pop_front();
    return msg;
  }

private:
  // Handle a new message by adding it to the queue and waking a waiter.
  void message_handler(Message msg, actor_address /* from */)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    message_queue_.push_back(std::move(msg));
    condition_.notify_one();
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::deque<Message> message_queue_;
};

//------------------------------------------------------------------------------


using boost::asio::thread_pool;

class member : public actor
{
public:
  explicit member(any_io_executor e)
    : actor(std::move(e))
  {
    register_handler(&member::init_handler);
  }

private:
  void init_handler(actor_address next, actor_address from)
  {
    next_ = next;
    caller_ = from;

    register_handler(&member::token_handler);
    deregister_handler(&member::init_handler);
  }

  void token_handler(int token, actor_address /*from*/)
  {
    int msg(token);
    actor_address to(caller_);

    if (token > 0)
    {
      msg = token - 1;
      to = next_;
    }

    tail_send(msg, to);
  }

  actor_address next_;
  actor_address caller_;
};

int example_main()
{
  const std::size_t num_threads = 16;
  const int num_hops = 50000000;
  const std::size_t num_actors = 503;
  const int token_value = (num_hops + num_actors - 1) / num_actors;
  const std::size_t actors_per_thread = num_actors / num_threads;

  struct single_thread_pool : thread_pool { single_thread_pool() : thread_pool(1) {} };
  single_thread_pool pools[num_threads];
  std::vector<std::shared_ptr<member>> members(num_actors);
  receiver<int> rcvr;

  // Create the member actors.
  for (std::size_t i = 0; i < num_actors; ++i)
    members[i] = std::make_shared<member>(pools[(i / actors_per_thread) % num_threads].get_executor());

  // Initialise the actors by passing each one the address of the next actor in the ring.
  for (std::size_t i = num_actors, next_i = 0; i > 0; next_i = --i)
    send(members[next_i]->address(), rcvr.address(), members[i - 1]->address());

  // Send exactly one token to each actor, all with the same initial value, rounding up if required.
  for (std::size_t i = 0; i < num_actors; ++i)
    send(token_value, rcvr.address(), members[i]->address());

  // Wait for all signal messages, indicating the tokens have all reached zero.
  for (std::size_t i = 0; i < num_actors; ++i)
    rcvr.wait();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/actor") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_cpp14_executors_actor

// libs/asio/example/cpp14/executors/async_1.cpp
namespace ex_cpp14_executors_async_1 {
using boost::asio::bind_executor;
using boost::asio::get_associated_executor;
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// A function to asynchronously read a single line from an input stream.
template <class IoExecutor, class Handler>
void async_getline(IoExecutor io_ex, std::istream& is, Handler handler)
{
  // Track work for the handler's associated executor.
  auto work_ex = boost::asio::prefer(
      get_associated_executor(handler, io_ex),
      execution::outstanding_work.tracked);

  // Post a function object to do the work asynchronously.
  boost::asio::require(io_ex, execution::blocking.never).execute(
      [&is, work_ex, handler=std::move(handler)]() mutable
      {
        std::string line;
        std::getline(is, line);

        // Pass the result to the handler, via the associated executor.
        boost::asio::prefer(work_ex, execution::blocking.possibly).execute(
            [line=std::move(line), handler=std::move(handler)]() mutable
            {
              handler(std::move(line));
            });
      });
}

int example_main()
{
  static_thread_pool io_pool(1);
  static_thread_pool completion_pool(1);

  ex_out << "Enter a line: ";

  async_getline(io_pool.executor(), ex_in,
      bind_executor(completion_pool.executor(),
        [](std::string line)
        {
          ex_out << "Line: " << line << "\n";
        }));

  io_pool.wait();
  completion_pool.wait();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/async_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(first line
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter a line: Line: first line
)ex"));
}
}  // namespace ex_cpp14_executors_async_1

// libs/asio/example/cpp14/executors/async_2.cpp
namespace ex_cpp14_executors_async_2 {
using boost::asio::bind_executor;
using boost::asio::get_associated_executor;
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// A function to asynchronously read a single line from an input stream.
template <class IoExecutor, class Handler>
void async_getline(IoExecutor io_ex, std::istream& is, Handler handler)
{
  // Track work for the handler's associated executor.
  auto work_ex = boost::asio::prefer(
      get_associated_executor(handler, io_ex),
      execution::outstanding_work.tracked);

  // Post a function object to do the work asynchronously.
  boost::asio::require(io_ex, execution::blocking.never).execute(
      [&is, work_ex, handler=std::move(handler)]() mutable
      {
        std::string line;
        std::getline(is, line);

        // Pass the result to the handler, via the associated executor.
        boost::asio::prefer(work_ex, execution::blocking.possibly).execute(
            [line=std::move(line), handler=std::move(handler)]() mutable
            {
              handler(std::move(line));
            });
      });
}

// A function to asynchronously read multiple lines from an input stream.
template <class IoExecutor, class Handler>
void async_getlines(IoExecutor io_ex, std::istream& is, std::string init, Handler handler)
{
  // Track work for the I/O executor.
  auto io_work_ex = boost::asio::prefer(io_ex,
      execution::outstanding_work.tracked);

  // Track work for the handler's associated executor.
  auto handler_work_ex = boost::asio::prefer(
      get_associated_executor(handler, io_ex),
      execution::outstanding_work.tracked);

  // Use the associated executor for each operation in the composition.
  async_getline(io_work_ex, is,
      bind_executor(handler_work_ex,
        [io_work_ex, &is, lines=std::move(init), handler=std::move(handler)]
        (std::string line) mutable
        {
          if (line.empty())
            handler(lines);
          else
            async_getlines(io_work_ex, is, lines + line + "\n", std::move(handler));
        }));
}

int example_main()
{
  static_thread_pool io_pool(1);
  static_thread_pool completion_pool(1);

  ex_out << "Enter text, terminating with a blank line:\n";

  async_getlines(io_pool.executor(), ex_in, "",
      bind_executor(completion_pool.executor(), [](std::string lines)
        {
          ex_out << "Lines:\n" << lines << "\n";
        }));

  io_pool.wait();
  completion_pool.wait();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/async_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(one
two
three
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter text, terminating with a blank line:
Lines:
one
two
three

)ex"));
}
}  // namespace ex_cpp14_executors_async_2

// libs/asio/example/cpp14/executors/bank_account_1.cpp
namespace ex_cpp14_executors_bank_account_1 {
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// Traditional active object pattern.
// Member functions do not block.

class bank_account
{
  int balance_ = 0;
  mutable static_thread_pool pool_{1};

public:
  void deposit(int amount)
  {
    pool_.executor().execute(
        [this, amount]
        {
          balance_ += amount;
        });
  }

  void withdraw(int amount)
  {
    pool_.executor().execute(
        [this, amount]
        {
          if (balance_ >= amount)
            balance_ -= amount;
        });
  }

  void print_balance() const
  {
    pool_.executor().execute(
        [this]
        {
          ex_out << "balance = " << balance_ << "\n";
        });
  }

  ~bank_account()
  {
    pool_.wait();
  }
};

int example_main()
{
  bank_account acct;
  acct.deposit(20);
  acct.withdraw(10);
  acct.print_balance();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/bank_account_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(balance = 10
)ex"));
}
}  // namespace ex_cpp14_executors_bank_account_1

// libs/asio/example/cpp14/executors/bank_account_2.cpp
namespace ex_cpp14_executors_bank_account_2 {
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// Traditional active object pattern.
// Member functions block until operation is finished.

class bank_account
{
  int balance_ = 0;
  mutable static_thread_pool pool_{1};

public:
  void deposit(int amount)
  {
    boost::asio::require(pool_.executor(), execution::blocking.always).execute(
        [this, amount]
        {
          balance_ += amount;
        });
  }

  void withdraw(int amount)
  {
    boost::asio::require(pool_.executor(),
      execution::blocking.always).execute(
        [this, amount]
        {
          if (balance_ >= amount)
            balance_ -= amount;
        });
  }

  int balance() const
  {
    int result = 0;
    boost::asio::require(pool_.executor(), execution::blocking.always).execute(
        [this, &result]
        {
          result = balance_;
        });
    return result;
  }
};

int example_main()
{
  bank_account acct;
  acct.deposit(20);
  acct.withdraw(10);
  ex_out << "balance = " << acct.balance() << "\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/bank_account_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(balance = 10
)ex"));
}
}  // namespace ex_cpp14_executors_bank_account_2

// libs/asio/example/cpp14/executors/fork_join.cpp
namespace ex_cpp14_executors_fork_join {
using boost::asio::static_thread_pool;
namespace execution = boost::asio::execution;

// A fixed-size thread pool used to implement fork/join semantics. Functions
// are scheduled using a simple FIFO queue. Implementing work stealing, or
// using a queue based on atomic operations, are left as tasks for the reader.
class fork_join_pool
{
public:
  // The constructor starts a thread pool with the specified number of threads.
  // Note that the thread_count is not a fixed limit on the pool's concurrency.
  // Additional threads may temporarily be added to the pool if they join a
  // fork_executor.
  explicit fork_join_pool(
      std::size_t thread_count = std::max(std::thread::hardware_concurrency(), 1u) * 2)
    : use_count_(1),
      threads_(thread_count)
  {
    try
    {
      // Ask each thread in the pool to dequeue and execute functions until
      // it is time to shut down, i.e. the use count is zero.
      for (thread_count_ = 0; thread_count_ < thread_count; ++thread_count_)
      {
        threads_.executor().execute(
            [this]
            {
              std::unique_lock<std::mutex> lock(mutex_);
              while (use_count_ > 0)
                if (!execute_next(lock))
                  condition_.wait(lock);
            });
      }
    }
    catch (...)
    {
      stop_threads();
      threads_.wait();
      throw;
    }
  }

  // The destructor waits for the pool to finish executing functions.
  ~fork_join_pool()
  {
    stop_threads();
    threads_.wait();
  }

private:
  friend class fork_executor;

  // The base for all functions that are queued in the pool.
  struct function_base
  {
    std::shared_ptr<std::size_t> work_count_;
    void (*execute_)(std::shared_ptr<function_base>& p);
  };

  // Execute the next function from the queue, if any. Returns true if a
  // function was executed, and false if the queue was empty.
  bool execute_next(std::unique_lock<std::mutex>& lock)
  {
    if (queue_.empty())
      return false;
    auto p(queue_.front());
    queue_.pop();
    lock.unlock();
    execute(lock, p);
    return true;
  }

  // Execute a function and decrement the outstanding work.
  void execute(std::unique_lock<std::mutex>& lock,
      std::shared_ptr<function_base>& p)
  {
    std::shared_ptr<std::size_t> work_count(std::move(p->work_count_));
    try
    {
      p->execute_(p);
      lock.lock();
      do_work_finished(work_count);
    }
    catch (...)
    {
      lock.lock();
      do_work_finished(work_count);
      throw;
    }
  }

  // Increment outstanding work.
  void do_work_started(const std::shared_ptr<std::size_t>& work_count) noexcept
  {
    if (++(*work_count) == 1)
      ++use_count_;
  }

  // Decrement outstanding work. Notify waiting threads if we run out.
  void do_work_finished(const std::shared_ptr<std::size_t>& work_count) noexcept
  {
    if (--(*work_count) == 0)
    {
      --use_count_;
      condition_.notify_all();
    }
  }

  // Dispatch a function, executing it immediately if the queue is already
  // loaded. Otherwise adds the function to the queue and wakes a thread.
  void do_execute(std::shared_ptr<function_base> p,
      const std::shared_ptr<std::size_t>& work_count)
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() > thread_count_ * 16)
    {
      do_work_started(work_count);
      lock.unlock();
      execute(lock, p);
    }
    else
    {
      queue_.push(p);
      do_work_started(work_count);
      condition_.notify_one();
    }
  }

  // Ask all threads to shut down.
  void stop_threads()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    --use_count_;
    condition_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<std::shared_ptr<function_base>> queue_;
  std::size_t use_count_;
  std::size_t thread_count_;
  static_thread_pool threads_;
};

// A class that satisfies the Executor requirements. Every function or piece of
// work associated with a fork_executor is part of a single, joinable group.
class fork_executor
{
public:
  fork_executor(fork_join_pool& ctx)
    : context_(ctx),
      work_count_(std::make_shared<std::size_t>(0))
  {
  }

  fork_join_pool& query(execution::context_t) const noexcept
  {
    return context_;
  }

  template <class Func>
  void execute(Func f) const
  {
    auto p(std::make_shared<function<Func>>(std::move(f), work_count_));
    context_.do_execute(p, work_count_);
  }

  friend bool operator==(const fork_executor& a,
      const fork_executor& b) noexcept
  {
    return a.work_count_ == b.work_count_;
  }

  friend bool operator!=(const fork_executor& a,
      const fork_executor& b) noexcept
  {
    return a.work_count_ != b.work_count_;
  }

  // Block until all work associated with the executor is complete. While it is
  // waiting, the thread may be borrowed to execute functions from the queue.
  void join() const
  {
    std::unique_lock<std::mutex> lock(context_.mutex_);
    while (*work_count_ > 0)
      if (!context_.execute_next(lock))
        context_.condition_.wait(lock);
  }

private:
  template <class Func>
  struct function : fork_join_pool::function_base
  {
    explicit function(Func f, const std::shared_ptr<std::size_t>& w)
      : function_(std::move(f))
    {
      work_count_ = w;
      execute_ = [](std::shared_ptr<fork_join_pool::function_base>& p)
      {
        Func tmp(std::move(static_cast<function*>(p.get())->function_));
        p.reset();
        tmp();
      };
    }

    Func function_;
  };

  fork_join_pool& context_;
  std::shared_ptr<std::size_t> work_count_;
};

// Helper class to automatically join a fork_executor when exiting a scope.
class join_guard
{
public:
  explicit join_guard(const fork_executor& ex) : ex_(ex) {}
  join_guard(const join_guard&) = delete;
  join_guard(join_guard&&) = delete;
  ~join_guard() { ex_.join(); }

private:
  fork_executor ex_;
};

//------------------------------------------------------------------------------


fork_join_pool pool;

template <class Iterator>
void fork_join_sort(Iterator begin, Iterator end)
{
  std::size_t n = end - begin;
  if (n > 32768)
  {
    {
      fork_executor fork(pool);
      join_guard join(fork);
      fork.execute([=]{ fork_join_sort(begin, begin + n / 2); });
      fork.execute([=]{ fork_join_sort(begin + n / 2, end); });
    }
    std::inplace_merge(begin, begin + n / 2, end);
  }
  else
  {
    std::sort(begin, end);
  }
}

int example_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    ex_out << "Usage: fork_join <size>\n";
    return 1;
  }

  std::vector<double> vec(std::atoll(argv[1]));
  std::iota(vec.begin(), vec.end(), 0);

  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(vec.begin(), vec.end(), g);

  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

  fork_join_sort(vec.begin(), vec.end());

  std::chrono::steady_clock::duration elapsed = std::chrono::steady_clock::now() - start;

  ex_out << "sort took ";
  ex_out << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();
  ex_out << " microseconds" << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/fork_join") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_args.push_back("1000");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(sort took 25 microseconds
)ex"));
#endif
}
}  // namespace ex_cpp14_executors_fork_join

// libs/asio/example/cpp14/executors/pipeline.cpp
namespace ex_cpp14_executors_pipeline {
using boost::asio::executor_binder;
using boost::asio::get_associated_executor;
namespace execution = boost::asio::execution;

// An executor that launches a new thread for each function submitted to it.
// This class satisfies the executor requirements.
class thread_executor
{
private:
  // Singleton execution context that manages threads launched by the new_thread_executor.
  class thread_bag
  {
    friend class thread_executor;

    void add_thread(std::thread&& t)
    {
      std::unique_lock<std::mutex> lock(mutex_);
      threads_.push_back(std::move(t));
    }

    thread_bag() = default;

    ~thread_bag()
    {
      for (auto& t : threads_)
        t.join();
    }

    std::mutex mutex_;
    std::vector<std::thread> threads_;
  };

public:
  static thread_bag& query(execution::context_t)
  {
    static thread_bag threads;
    return threads;
  }

  static constexpr auto query(execution::blocking_t)
  {
    return execution::blocking.never;
  }

  template <class Func>
  void execute(Func f) const
  {
    thread_bag& bag = query(execution::context);
    bag.add_thread(std::thread(std::move(f)));
  }

  friend bool operator==(const thread_executor&,
      const thread_executor&) noexcept
  {
    return true;
  }

  friend bool operator!=(const thread_executor&,
      const thread_executor&) noexcept
  {
    return false;
  }
};

// Base class for all thread-safe queue implementations.
class queue_impl_base
{
  template <class> friend class queue_front;
  template <class> friend class queue_back;
  std::mutex mutex_;
  std::condition_variable condition_;
  bool stop_ = false;
};

// Underlying implementation of a thread-safe queue, shared between the
// queue_front and queue_back classes.
template <class T>
class queue_impl : public queue_impl_base
{
  template <class> friend class queue_front;
  template <class> friend class queue_back;
  std::queue<T> queue_;
};

// The front end of a queue between consecutive pipeline stages.
template <class T>
class queue_front
{
public:
  typedef T value_type;

  explicit queue_front(std::shared_ptr<queue_impl<T>> impl)
    : impl_(impl)
  {
  }

  void push(T t)
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);
    impl_->queue_.push(std::move(t));
    impl_->condition_.notify_one();
  }

  void stop()
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);
    impl_->stop_ = true;
    impl_->condition_.notify_one();
  }

private:
  std::shared_ptr<queue_impl<T>> impl_;
};

// The back end of a queue between consecutive pipeline stages.
template <class T>
class queue_back
{
public:
  typedef T value_type;

  explicit queue_back(std::shared_ptr<queue_impl<T>> impl)
    : impl_(impl)
  {
  }

  bool pop(T& t)
  {
    std::unique_lock<std::mutex> lock(impl_->mutex_);
    while (impl_->queue_.empty() && !impl_->stop_)
      impl_->condition_.wait(lock);
    if (!impl_->queue_.empty())
    {
      t = impl_->queue_.front();
      impl_->queue_.pop();
      return true;
    }
    return false;
  }

private:
  std::shared_ptr<queue_impl<T>> impl_;
};

// Launch the last stage in a pipeline.
template <class T, class F>
std::future<void> pipeline(queue_back<T> in, F f)
{
  // Get the function's associated executor, defaulting to thread_executor.
  auto ex = get_associated_executor(f, thread_executor());

  // Run the function, and as we're the last stage return a future so that the
  // caller can wait for the pipeline to finish.
  std::packaged_task<void()> task(
      [in, f = std::move(f)]() mutable
      {
        f(in);
      });
  std::future<void> fut = task.get_future();
  boost::asio::require(ex, execution::blocking.never).execute(std::move(task));
  return fut;
}

// Launch an intermediate stage in a pipeline.
template <class T, class F, class... Tail>
std::future<void> pipeline(queue_back<T> in, F f, Tail... t)
{
  // Determine the output queue type.
  typedef typename executor_binder<F, thread_executor>::second_argument_type::value_type output_value_type;

  // Create the output queue and its implementation.
  auto out_impl = std::make_shared<queue_impl<output_value_type>>();
  queue_front<output_value_type> out(out_impl);
  queue_back<output_value_type> next_in(out_impl);

  // Get the function's associated executor, defaulting to thread_executor.
  auto ex = get_associated_executor(f, thread_executor());

  // Run the function.
  boost::asio::require(ex, execution::blocking.never).execute(
      [in, out, f = std::move(f)]() mutable
      {
        f(in, out);
        out.stop();
      });

  // Launch the rest of the pipeline.
  return pipeline(next_in, std::move(t)...);
}

// Launch the first stage in a pipeline.
template <class F, class... Tail>
std::future<void> pipeline(F f, Tail... t)
{
  // Determine the output queue type.
  typedef typename executor_binder<F, thread_executor>::argument_type::value_type output_value_type;

  // Create the output queue and its implementation.
  auto out_impl = std::make_shared<queue_impl<output_value_type>>();
  queue_front<output_value_type> out(out_impl);
  queue_back<output_value_type> next_in(out_impl);

  // Get the function's associated executor, defaulting to thread_executor.
  auto ex = get_associated_executor(f, thread_executor());

  // Run the function.
  boost::asio::require(ex, execution::blocking.never).execute(
      [out, f = std::move(f)]() mutable
      {
        f(out);
        out.stop();
      });

  // Launch the rest of the pipeline.
  return pipeline(next_in, std::move(t)...);
}

//------------------------------------------------------------------------------


using boost::asio::bind_executor;
using boost::asio::static_thread_pool;

void reader(queue_front<std::string> out)
{
  std::string line;
  while (std::getline(ex_in, line))
    out.push(line);
}

void filter(queue_back<std::string> in, queue_front<std::string> out)
{
  std::string line;
  while (in.pop(line))
    if (line.length() > 5)
      out.push(line);
}

void upper(queue_back<std::string> in, queue_front<std::string> out)
{
  std::string line;
  while (in.pop(line))
  {
    std::string new_line;
    for (char c : line)
      new_line.push_back(std::toupper(c));
    out.push(new_line);
  }
}

void writer(queue_back<std::string> in)
{
  std::size_t count = 0;
  std::string line;
  while (in.pop(line))
    ex_out << count++ << ": " << line << std::endl;
}

int example_main()
{
  static_thread_pool pool(1);

  auto f = pipeline(reader, filter, bind_executor(pool.executor(), upper), writer);
  f.wait();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/pipeline") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(the quick brown fox
jumps over the lazy dog
)ex");  // the program's stdin
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0: THE QUICK BROWN FOX
1: JUMPS OVER THE LAZY DOG
)ex"));
}
}  // namespace ex_cpp14_executors_pipeline

// libs/asio/example/cpp14/executors/priority_scheduler.cpp
namespace ex_cpp14_executors_priority_scheduler {
namespace execution = boost::asio::execution;

namespace custom_props {

  struct priority
  {
    template <typename T>
    static constexpr bool is_applicable_property_v =
      execution::is_executor<T>::value;

    static constexpr bool is_requirable = true;
    static constexpr bool is_preferable = true;

    using polymorphic_query_result_type = int;

    int value() const { return value_; }

    int value_ = 1;
  };

  constexpr priority low_priority{0};
  constexpr priority normal_priority{1};
  constexpr priority high_priority{2};

} // namespace custom_props

class priority_scheduler
{
public:
  // A class that satisfies the Executor requirements.
  class executor_type
  {
  public:
    executor_type(priority_scheduler& ctx) noexcept
      : context_(ctx), priority_(custom_props::normal_priority.value())
    {
    }

    priority_scheduler& query(execution::context_t) const noexcept
    {
      return context_;
    }

    int query(custom_props::priority) const noexcept
    {
      return priority_;
    }

    executor_type require(custom_props::priority pri) const
    {
      executor_type new_ex(*this);
      new_ex.priority_ = pri.value();
      return new_ex;
    }

    template <class Func>
    void execute(Func f) const
    {
      auto p(std::make_shared<item<Func>>(priority_, std::move(f)));
      std::lock_guard<std::mutex> lock(context_.mutex_);
      context_.queue_.push(p);
      context_.condition_.notify_one();
    }

    friend bool operator==(const executor_type& a,
        const executor_type& b) noexcept
    {
      return &a.context_ == &b.context_;
    }

    friend bool operator!=(const executor_type& a,
        const executor_type& b) noexcept
    {
      return &a.context_ != &b.context_;
    }

  private:
    priority_scheduler& context_;
    int priority_;
  };

  executor_type executor() noexcept
  {
    return executor_type(*const_cast<priority_scheduler*>(this));
  }

  void run()
  {
    std::unique_lock<std::mutex> lock(mutex_);
    for (;;)
    {
      condition_.wait(lock, [&]{ return stopped_ || !queue_.empty(); });
      if (stopped_)
        return;
      auto p(queue_.top());
      queue_.pop();
      lock.unlock();
      p->execute_(p);
      lock.lock();
    }
  }

  void stop()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_ = true;
    condition_.notify_all();
  }

private:
  struct item_base
  {
    int priority_;
    void (*execute_)(std::shared_ptr<item_base>&);
  };

  template <class Func>
  struct item : item_base
  {
    item(int pri, Func f) : function_(std::move(f))
    {
      priority_ = pri;
      execute_ = [](std::shared_ptr<item_base>& p)
      {
        Func tmp(std::move(static_cast<item*>(p.get())->function_));
        p.reset();
        tmp();
      };
    }

    Func function_;
  };

  struct item_comp
  {
    bool operator()(
        const std::shared_ptr<item_base>& a,
        const std::shared_ptr<item_base>& b)
    {
      return a->priority_ < b->priority_;
    }
  };

  std::mutex mutex_;
  std::condition_variable condition_;
  std::priority_queue<
    std::shared_ptr<item_base>,
    std::vector<std::shared_ptr<item_base>>,
    item_comp> queue_;
  bool stopped_ = false;
};

int example_main()
{
  priority_scheduler sched;
  auto ex = sched.executor();
  auto prefer_low = boost::asio::prefer(ex, custom_props::low_priority);
  auto low = boost::asio::require(ex, custom_props::low_priority);
  auto med = boost::asio::require(ex, custom_props::normal_priority);
  auto high = boost::asio::require(ex, custom_props::high_priority);
  execution::any_executor<custom_props::priority> poly_high(high);
  prefer_low.execute([]{ ex_out << "1\n"; });
  low.execute([]{ ex_out << "11\n"; });
  low.execute([]{ ex_out << "111\n"; });
  med.execute([]{ ex_out << "2\n"; });
  med.execute([]{ ex_out << "22\n"; });
  high.execute([]{ ex_out << "3\n"; });
  high.execute([]{ ex_out << "33\n"; });
  high.execute([]{ ex_out << "333\n"; });
  poly_high.execute([]{ ex_out << "3333\n"; });
  boost::asio::require(ex, custom_props::priority{-1}).execute([&]{ sched.stop(); });
  sched.run();
  ex_out << "polymorphic query result = " << boost::asio::query(poly_high, custom_props::priority{}) << "\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/executors/priority_scheduler") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(3
333
3333
33
22
2
11
1
111
polymorphic query result = 2
)ex"));
#endif
}
}  // namespace ex_cpp14_executors_priority_scheduler

// libs/asio/example/cpp14/operations/c_callback_wrapper.cpp
namespace ex_cpp14_operations_c_callback_wrapper {
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
template <typename Handler>
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
template <typename CompletionToken>
auto async_read_input(const std::string& prompt, CompletionToken&& token)
{
  // Define a function object that contains the code to launch the asynchronous
  // operation. This is passed the concrete completion handler, followed by any
  // additional arguments that were passed through the call to async_initiate.
  auto init = [](auto handler, const std::string& prompt)
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

TEST_CASE("example: cpp14/operations/c_callback_wrapper") {
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
}  // namespace ex_cpp14_operations_c_callback_wrapper

// libs/asio/example/cpp14/operations/callback_wrapper.cpp
namespace ex_cpp14_operations_callback_wrapper {
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
template <typename CompletionToken>
auto async_read_input(const std::string& prompt, CompletionToken&& token)
{
  // Define a function object that contains the code to launch the asynchronous
  // operation. This is passed the concrete completion handler, followed by any
  // additional arguments that were passed through the call to async_initiate.
  auto init = [](auto handler, const std::string& prompt)
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

TEST_CASE("example: cpp14/operations/callback_wrapper") {
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
}  // namespace ex_cpp14_operations_callback_wrapper

// libs/asio/example/cpp14/operations/composed_1.cpp
namespace ex_cpp14_operations_composed_1 {
using boost::asio::ip::tcp;

//------------------------------------------------------------------------------

// This is the simplest example of a composed asynchronous operation, where we
// simply repackage an existing operation. The asynchronous operation
// requirements are met by delegating responsibility to the underlying
// operation.

template <typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto op = async_write_message(socket,
      "Testing deferred\r\n", boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_1") {
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
}  // namespace ex_cpp14_operations_composed_1

// libs/asio/example/cpp14/operations/composed_2.cpp
namespace ex_cpp14_operations_composed_2 {
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

template <typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto initiation = [](auto&& completion_handler, tcp::socket& socket,
      const char* message, bool allow_partial_write)
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
  auto op = async_write_message(socket,
      "Testing deferred\r\n", false, boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_2") {
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
}  // namespace ex_cpp14_operations_composed_2

// libs/asio/example/cpp14/operations/composed_3.cpp
namespace ex_cpp14_operations_composed_3 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// In this composed operation we repackage an existing operation, but with a
// different completion handler signature. The asynchronous operation
// requirements are met by delegating responsibility to the underlying
// operation.

template <typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto initiation = [](auto&& completion_handler,
      tcp::socket& socket, const char* message)
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
  auto op = async_write_message(socket,
      "Testing deferred\r\n", boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_3") {
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
}  // namespace ex_cpp14_operations_composed_3

// libs/asio/example/cpp14/operations/composed_4.cpp
namespace ex_cpp14_operations_composed_4 {
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

template <typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto initiation = [](auto&& completion_handler,
      tcp::socket& socket, const char* message)
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
  auto op = async_write_message(socket, "", boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_4") {
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
}  // namespace ex_cpp14_operations_composed_4

// libs/asio/example/cpp14/operations/composed_5.cpp
namespace ex_cpp14_operations_composed_5 {
using boost::asio::ip::tcp;

// NOTE: This example requires the new boost::asio::async_initiate function. For
// an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

//------------------------------------------------------------------------------

// This composed operation automatically serialises a message, using its I/O
// streams insertion operator, before sending it on the socket. To do this, it
// must allocate a buffer for the encoded message and ensure this buffer's
// validity until the underlying async_write operation completes.

template <typename T, typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto initiation = [](auto&& completion_handler,
      tcp::socket& socket, std::unique_ptr<std::string> encoded_message)
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
  auto op = async_write_message(socket,
      std::string("abcdef"), boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_5") {
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
}  // namespace ex_cpp14_operations_composed_5

// libs/asio/example/cpp14/operations/composed_6.cpp
namespace ex_cpp14_operations_composed_6 {
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

template <typename T, typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto initiation = [](auto&& completion_handler, tcp::socket& socket,
      std::unique_ptr<std::string> encoded_message, std::size_t repeat_count,
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
  auto op = async_write_messages(socket,
      "Testing deferred\r\n", 5, boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_6") {
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
}  // namespace ex_cpp14_operations_composed_6

// libs/asio/example/cpp14/operations/composed_7.cpp
namespace ex_cpp14_operations_composed_7 {
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

template <typename T, typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto op = async_write_messages(socket,
      "Testing deferred\r\n", 5, boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_7") {
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
}  // namespace ex_cpp14_operations_composed_7

// libs/asio/example/cpp14/operations/composed_8.cpp
namespace ex_cpp14_operations_composed_8 {
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

template <typename T, typename CompletionToken>
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
  // In C++14 we can omit the return type as it is automatically deduced from
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
  auto op = async_write_messages(socket,
      "Testing deferred\r\n", 5, boost::asio::deferred);

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

TEST_CASE("example: cpp14/operations/composed_8") {
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
}  // namespace ex_cpp14_operations_composed_8

// libs/asio/example/cpp14/parallel_group/parallel_sort.cpp
namespace ex_cpp14_parallel_group_parallel_sort {
template <
    typename Executor,
    typename RandomAccessIterator,
    boost::asio::completion_token_for<void()> CompletionToken>
auto parallel_sort(
    Executor executor,
    RandomAccessIterator begin,
    RandomAccessIterator end,
    CompletionToken&& token);

template <
    typename Executor,
    typename RandomAccessIterator>
void parallel_sort_impl(
    Executor executor,
    RandomAccessIterator begin,
    RandomAccessIterator end,
    std::function<void()> continuation)
{
    std::size_t n = end - begin;
    if (n <= 16384)
    {
      boost::asio::post(executor,
          [=]
          {
            std::sort(begin, end);
            continuation();
          }
        );
    }
    else
    {
      boost::asio::experimental::make_parallel_group(
          [=](auto token)
          {
            return parallel_sort(executor, begin, begin + n / 2, token);
          },
          [=](auto token)
          {
            return parallel_sort(executor, begin + n / 2, end, token);
          }
        ).async_wait(
          boost::asio::experimental::wait_for_all(),
          [=](std::array<std::size_t, 2>)
          {
            std::inplace_merge(begin, begin + n / 2, end);
            continuation();
          }
        );
    }
}

template <
    typename Executor,
    typename RandomAccessIterator,
    boost::asio::completion_token_for<void()> CompletionToken>
auto parallel_sort(
    Executor executor,
    RandomAccessIterator begin,
    RandomAccessIterator end,
    CompletionToken&& token)
{
  return boost::asio::async_compose<CompletionToken, void()>(
      [=](auto& self, auto... args)
      {
        if (sizeof...(args) == 0)
        {
          using self_type = std::decay_t<decltype(self)>;
          parallel_sort_impl(executor, begin, end,
              [self = std::make_shared<self_type>(std::move(self))]
              {
                boost::asio::dispatch(
                    boost::asio::append(
                      std::move(*self), 0));
              }
            );
        }
        else
        {
          self.complete();
        }
      },
      token
    );
}

int example_main()
{
  boost::asio::thread_pool pool(4);

  std::vector<int> values(100'000'000);

  std::random_device random_device;
  std::mt19937 rng(random_device());
  std::uniform_int_distribution<int> dist(1, 1'000'000);
  std::generate(values.begin(), values.end(), [&]{ return dist(rng); });

  ex_out << "starting sort\n";

  auto begin = std::chrono::high_resolution_clock::now();

  parallel_sort(
      pool.get_executor(),
      values.begin(),
      values.end(),
      boost::asio::use_future
    ).get();

  auto end = std::chrono::high_resolution_clock::now();

  auto duration = end - begin;
  ex_out << "sort took "
    << std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
    << " microseconds\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/parallel_group/parallel_sort") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(starting sort
sort took 6149431 microseconds
)ex"));
#endif
}
}  // namespace ex_cpp14_parallel_group_parallel_sort

// libs/asio/example/cpp14/parallel_group/ranged_wait_for_all.cpp
namespace ex_cpp14_parallel_group_ranged_wait_for_all {
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor out(ctx, ::dup(STDOUT_FILENO));
  boost::asio::posix::stream_descriptor err(ctx, ::dup(STDERR_FILENO));

  using op_type = decltype(
      out.async_write_some(boost::asio::buffer("", 0))
    );

  std::vector<op_type> ops;

  ops.push_back(
      out.async_write_some(boost::asio::buffer("first\r\n", 7))
    );

  ops.push_back(
      err.async_write_some(boost::asio::buffer("second\r\n", 8))
    );

  boost::asio::experimental::make_parallel_group(ops).async_wait(
      boost::asio::experimental::wait_for_all(),
      [](
          std::vector<std::size_t> completion_order,
          std::vector<boost::system::error_code> ec,
          std::vector<std::size_t> n
      )
      {
        for (std::size_t i = 0; i < completion_order.size(); ++i)
        {
          std::size_t idx = completion_order[i];
          ex_out << "operation " << idx << " finished: ";
          ex_out << ec[idx] << ", " << n[idx] << "\n";
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp14/parallel_group/ranged_wait_for_all") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("first\r\n"
        "operation 0 finished: system:0, 7\n"
        "operation 1 finished: system:0, 8\n", 75)));
#endif
}
}  // namespace ex_cpp14_parallel_group_ranged_wait_for_all

// libs/asio/example/cpp14/parallel_group/wait_for_all.cpp
namespace ex_cpp14_parallel_group_wait_for_all {
#ifdef BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      [&](auto token)
      {
        return in.async_read_some(boost::asio::buffer(data), token);
      },
      [&](auto token)
      {
        return timer.async_wait(token);
      }
    ).async_wait(
      boost::asio::experimental::wait_for_all(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp14/parallel_group/wait_for_all") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
#endif
}
}  // namespace ex_cpp14_parallel_group_wait_for_all

// libs/asio/example/cpp14/parallel_group/wait_for_one.cpp
namespace ex_cpp14_parallel_group_wait_for_one {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      [&](auto token)
      {
        return in.async_read_some(boost::asio::buffer(data), token);
      },
      [&](auto token)
      {
        return timer.async_wait(token);
      }
    ).async_wait(
      boost::asio::experimental::wait_for_one(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp14/parallel_group/wait_for_one") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
}
}  // namespace ex_cpp14_parallel_group_wait_for_one

// libs/asio/example/cpp14/parallel_group/wait_for_one_error.cpp
namespace ex_cpp14_parallel_group_wait_for_one_error {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      [&](auto token)
      {
        return in.async_read_some(boost::asio::buffer(data), token);
      },
      [&](auto token)
      {
        return timer.async_wait(token);
      }
    ).async_wait(
      boost::asio::experimental::wait_for_one_error(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp14/parallel_group/wait_for_one_error") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
}
}  // namespace ex_cpp14_parallel_group_wait_for_one_error

// libs/asio/example/cpp14/parallel_group/wait_for_one_success.cpp
namespace ex_cpp14_parallel_group_wait_for_one_success {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

int example_main()
{
  boost::asio::io_context ctx;

  boost::asio::posix::stream_descriptor in(ctx, ::dup(STDIN_FILENO));
  boost::asio::steady_timer timer(ctx, std::chrono::seconds(5));

  char data[1024];

  boost::asio::experimental::make_parallel_group(
      [&](auto token)
      {
        return in.async_read_some(boost::asio::buffer(data), token);
      },
      [&](auto token)
      {
        return timer.async_wait(token);
      }
    ).async_wait(
      boost::asio::experimental::wait_for_one_success(),
      [](
          std::array<std::size_t, 2> completion_order,
          boost::system::error_code ec1, std::size_t n1,
          boost::system::error_code ec2
      )
      {
        switch (completion_order[0])
        {
        case 0:
          {
            ex_out << "descriptor finished: " << ec1 << ", " << n1 << "\n";
          }
          break;
        case 1:
          {
            ex_out << "timer finished: " << ec2 << "\n";
          }
          break;
        }
      }
    );

  ctx.run();
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)
int example_main() {    return 0;  // main's implicit return
}
#endif // defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR)

TEST_CASE("example: cpp14/parallel_group/wait_for_one_success") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(descriptor finished: asio.misc:2, 0
)ex"));
}
}  // namespace ex_cpp14_parallel_group_wait_for_one_success
