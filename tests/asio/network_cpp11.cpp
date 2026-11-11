// mcpplibs.mwrap.boost.asio — the official C++11 (libs/asio/example/cpp11/) examples that need a network
// peer, run over the loopback interface in one process. A server example
// runs in a background thread on a free port (ex_serve_async) and a harness
// client of the loopback harness below talks to it; the case output is what
// that client received. A client example runs against the ported server
// example it belongs with, started the same way. The expected output is what
// the same code printed when built from upstream/boost headers alone. Fixed
// ports (13, 55555, 54321) are replaced by the free one, "daytime"/"http"
// service names by its number, and output that carries the time of day is
// not compared.
// The local/ examples (UNIX domain sockets; the fd-passing pair also calls
// sendmsg/recvmsg) are compiled on POSIX systems only.
#if !defined(_WIN32)
#include <sys/socket.h>
#include <sys/uio.h>
#endif
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cstdlib>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/asio/detail/handler_tracking.hpp>
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

// libs/asio/example/cpp11/echo/async_tcp_echo_server.cpp
namespace ex_cpp11_echo_async_tcp_echo_server {
using boost::asio::ip::tcp;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            do_write(length);
          }
        });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            do_read();
          }
        });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/echo/async_tcp_echo_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_echo_async_tcp_echo_server

// libs/asio/example/cpp11/echo/blocking_tcp_echo_server.cpp
namespace ex_cpp11_echo_blocking_tcp_echo_server {
using boost::asio::ip::tcp;

const int max_length = 1024;

void session(tcp::socket sock)
{
  try
  {
    for (;;)
    {
      char data[max_length];

      boost::system::error_code error;
      size_t length = sock.read_some(boost::asio::buffer(data), error);
      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      boost::asio::write(sock, boost::asio::buffer(data, length));
    }
  }
  catch (std::exception& e)
  {
    ex_out << "Exception in thread: " << e.what() << "\n";
  }
}

void server(boost::asio::io_context& io_context, unsigned short port)
{
  tcp::acceptor a(io_context, tcp::endpoint(tcp::v4(), port));
  for (;;)
  {
    std::thread(session, a.accept()).detach();
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: blocking_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server(io_context, std::atoi(argv[1]));
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/echo/blocking_tcp_echo_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_echo_blocking_tcp_echo_server

// libs/asio/example/cpp11/echo/async_udp_echo_server.cpp
namespace ex_cpp11_echo_async_udp_echo_server {
using boost::asio::ip::udp;

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : socket_(io_context, udp::endpoint(udp::v4(), port))
  {
    do_receive();
  }

  void do_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(data_, max_length), sender_endpoint_,
        [this](boost::system::error_code ec, std::size_t bytes_recvd)
        {
          if (!ec && bytes_recvd > 0)
          {
            do_send(bytes_recvd);
          }
          else
          {
            do_receive();
          }
        });
  }

  void do_send(std::size_t length)
  {
    socket_.async_send_to(
        boost::asio::buffer(data_, length), sender_endpoint_,
        [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
        {
          do_receive();
        });
  }

private:
  udp::socket socket_;
  udp::endpoint sender_endpoint_;
  enum { max_length = 1024 };
  char data_[max_length];
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: async_udp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;
    auto work = boost::asio::make_work_guard(io_context);
    std::thread t([&](){io_context.run();});

    server s(io_context, std::atoi(argv[1]));

    work.reset();
    t.join();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/echo/async_udp_echo_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_udp_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_udp_exchange(ex_port, "hello")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_echo_async_udp_echo_server

// libs/asio/example/cpp11/echo/blocking_udp_echo_server.cpp
namespace ex_cpp11_echo_blocking_udp_echo_server {
using boost::asio::ip::udp;

enum { max_length = 1024 };

void server(boost::asio::io_context& io_context, unsigned short port)
{
  udp::socket sock(io_context, udp::endpoint(udp::v4(), port));
  for (;;)
  {
    char data[max_length];
    udp::endpoint sender_endpoint;
    size_t length = sock.receive_from(
        boost::asio::buffer(data, max_length), sender_endpoint);
    sock.send_to(boost::asio::buffer(data, length), sender_endpoint);
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: blocking_udp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server(io_context, std::atoi(argv[1]));
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/echo/blocking_udp_echo_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_udp_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_udp_exchange(ex_port, "hello")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_echo_blocking_udp_echo_server

// libs/asio/example/cpp11/echo/blocking_tcp_echo_client.cpp
namespace ex_cpp11_echo_blocking_tcp_echo_client {
using boost::asio::ip::tcp;

enum { max_length = 1024 };

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: blocking_tcp_echo_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::socket s(io_context);
    tcp::resolver resolver(io_context);
    boost::asio::connect(s, resolver.resolve(argv[1], argv[2]));

    ex_out << "Enter message: ";
    char request[max_length];
    ex_in.getline(request, max_length);
    size_t request_length = std::strlen(request);
    boost::asio::write(s, boost::asio::buffer(request, request_length));

    char reply[max_length];
    size_t reply_length = boost::asio::read(s,
        boost::asio::buffer(reply, request_length));
    ex_out << "Reply is: ";
    ex_out.write(reply, reply_length);
    ex_out << "\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/echo/blocking_tcp_echo_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello world
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_echo_blocking_tcp_echo_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter message: Reply is: hello world
)ex"));
}
}  // namespace ex_cpp11_echo_blocking_tcp_echo_client

// libs/asio/example/cpp11/echo/blocking_udp_echo_client.cpp
namespace ex_cpp11_echo_blocking_udp_echo_client {
using boost::asio::ip::udp;

enum { max_length = 1024 };

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: blocking_udp_echo_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    udp::socket s(io_context, udp::endpoint(udp::v4(), 0));

    udp::resolver resolver(io_context);
    udp::resolver::results_type endpoints =
      resolver.resolve(udp::v4(), argv[1], argv[2]);

    ex_out << "Enter message: ";
    char request[max_length];
    ex_in.getline(request, max_length);
    size_t request_length = std::strlen(request);
    s.send_to(boost::asio::buffer(request, request_length), *endpoints.begin());

    char reply[max_length];
    udp::endpoint sender_endpoint;
    size_t reply_length = s.receive_from(
        boost::asio::buffer(reply, max_length), sender_endpoint);
    ex_out << "Reply is: ";
    ex_out.write(reply, reply_length);
    ex_out << "\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/echo/blocking_udp_echo_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello world
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_free_udp_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_echo_blocking_udp_echo_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Enter message: Reply is: hello world
)ex"));
}
}  // namespace ex_cpp11_echo_blocking_udp_echo_client

// libs/asio/example/cpp11/handler_tracking/async_tcp_echo_server.cpp
namespace ex_cpp11_handler_tracking_async_tcp_echo_server {
using boost::asio::ip::tcp;

// Define a helper macro to invoke BOOST_ASIO_HANDLER_LOCATION with the current
// file name, line number, and function name. For the function name, you might
// also consider using __PRETTY_FUNCTION__, BOOST_CURRENT_FUNCTION, or a hand-
// crafted name. For C++20 or later, you may also use std::source_location.
#define HANDLER_LOCATION \
  BOOST_ASIO_HANDLER_LOCATION((__FILE__, __LINE__, __func__))

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    HANDLER_LOCATION;

    do_read();
  }

private:
  void do_read()
  {
    HANDLER_LOCATION;

    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          HANDLER_LOCATION;

          if (!ec)
          {
            do_write(length);
          }
        });
  }

  void do_write(std::size_t length)
  {
    HANDLER_LOCATION;

    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          HANDLER_LOCATION;

          if (!ec)
          {
            do_read();
          }
        });
  }

  tcp::socket socket_;
  enum { max_length = 1024 };
  char data_[max_length];
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    HANDLER_LOCATION;

    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          HANDLER_LOCATION;

          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: async_tcp_echo_server <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/handler_tracking/async_tcp_echo_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_handler_tracking_async_tcp_echo_server

// libs/asio/example/cpp11/allocation/server.cpp
namespace ex_cpp11_allocation_server {
using boost::asio::ip::tcp;

// Class to manage the memory to be used for allocating objects that are
// associated with an execution context (such as services and internal state of
// I/O objects). It contains a single block of memory from which objects are
// monotonically allocated (similar to std::pmr::monotonic_resource). If no
// more space is available it delegates allocation to the global heap.
class context_memory
{
public:
  explicit context_memory(std::size_t preallocated)
    : preallocated_(preallocated),
      next_allocation_(0),
      storage_(new unsigned char[preallocated_])
  {
  }

  ~context_memory()
  {
    delete[] storage_;
  }

  context_memory(const context_memory&) = delete;
  context_memory& operator=(const context_memory&) = delete;

  void* allocate(std::size_t size, std::size_t align)
  {
    // Since this program is single-threaded there is no need to perform any
    // synchronisation when modifying next_allocation_. Use an atomic or other
    // form of synchronisation when using an exeution context from multiple
    // threads.
    std::size_t space = size + align;
    if (next_allocation_ + space < preallocated_)
    {
      void* ptr = storage_ + next_allocation_;
      next_allocation_ += space;
      return std::align(align, size, ptr, space);
    }
    else
    {
      return ::operator new(size);
    }
  }

  void deallocate(void* ptr)
  {
    auto* ucptr = static_cast<unsigned char*>(ptr);
    if (std::less_equal<unsigned char*>{}(storage_, ucptr)
        && std::less<unsigned char*>{}(ucptr, storage_ + preallocated_))
    {
      // Nothing to do.
    }
    else
    {
      ::operator delete(ptr);
    }
  }

private:
  std::size_t preallocated_;
  std::size_t next_allocation_;
  unsigned char* storage_;
};

// The allocator to be associated with the execution context. This allocatoro
// only needs to satisfy the C++11 minimal allocator requirements.
template <typename T>
class context_allocator
{
public:
  using value_type = T;

  explicit context_allocator(context_memory& mem)
    : memory_(mem)
  {
  }

  template <typename U>
  context_allocator(const context_allocator<U>& other) noexcept
    : memory_(other.memory_)
  {
  }

  bool operator==(const context_allocator& other) const noexcept
  {
    return &memory_ == &other.memory_;
  }

  bool operator!=(const context_allocator& other) const noexcept
  {
    return &memory_ != &other.memory_;
  }

  T* allocate(std::size_t n) const
  {
    return static_cast<T*>(memory_.allocate(sizeof(T) * n, alignof(T)));
  }

  void deallocate(T* p, std::size_t /*n*/) const
  {
    return memory_.deallocate(p);
  }

private:
  template <typename> friend class context_allocator;

  // The underlying memory.
  context_memory& memory_;
};

// Class to manage the memory to be used for handler-based custom allocation.
// It contains a single block of memory which may be returned for allocation
// requests. If the memory is in use when an allocation request is made, the
// allocator delegates allocation to the global heap.
class handler_memory
{
public:
  handler_memory()
    : in_use_(false)
  {
  }

  handler_memory(const handler_memory&) = delete;
  handler_memory& operator=(const handler_memory&) = delete;

  void* allocate(std::size_t size)
  {
    if (!in_use_ && size < sizeof(storage_))
    {
      in_use_ = true;
      return &storage_;
    }
    else
    {
      return ::operator new(size);
    }
  }

  void deallocate(void* pointer)
  {
    if (pointer == &storage_)
    {
      in_use_ = false;
    }
    else
    {
      ::operator delete(pointer);
    }
  }

private:
  // Storage space used for handler-based custom memory allocation.
  typename std::aligned_storage<1024>::type storage_;

  // Whether the handler-based custom allocation storage has been used.
  bool in_use_;
};

// The allocator to be associated with the handler objects. This allocator only
// needs to satisfy the C++11 minimal allocator requirements.
template <typename T>
class handler_allocator
{
public:
  using value_type = T;

  explicit handler_allocator(handler_memory& mem)
    : memory_(mem)
  {
  }

  template <typename U>
  handler_allocator(const handler_allocator<U>& other) noexcept
    : memory_(other.memory_)
  {
  }

  bool operator==(const handler_allocator& other) const noexcept
  {
    return &memory_ == &other.memory_;
  }

  bool operator!=(const handler_allocator& other) const noexcept
  {
    return &memory_ != &other.memory_;
  }

  T* allocate(std::size_t n) const
  {
    return static_cast<T*>(memory_.allocate(sizeof(T) * n));
  }

  void deallocate(T* p, std::size_t /*n*/) const
  {
    return memory_.deallocate(p);
  }

private:
  template <typename> friend class handler_allocator;

  // The underlying memory.
  handler_memory& memory_;
};

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        boost::asio::bind_allocator(
          handler_allocator<int>(handler_memory_),
          [this, self](boost::system::error_code ec, std::size_t length)
          {
            if (!ec)
            {
              do_write(length);
            }
          }));
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
        boost::asio::bind_allocator(
          handler_allocator<int>(handler_memory_),
          [this, self](boost::system::error_code ec, std::size_t /*length*/)
          {
            if (!ec)
            {
              do_read();
            }
          }));
  }

  // The socket used to communicate with the client.
  tcp::socket socket_;

  // Buffer used to store data received from the client.
  std::array<char, 1024> data_;

  // The memory to use for handler-based custom memory allocation.
  handler_memory handler_memory_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: server <port>\n";
      return 1;
    }

    context_memory memory(4096);
    context_allocator<void> allocator(memory);
    boost::asio::io_context io_context(std::allocator_arg, allocator);
    server s(io_context, std::atoi(argv[1]));
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/allocation/server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_allocation_server

// libs/asio/example/cpp11/nonblocking/third_party_lib.cpp
namespace ex_cpp11_nonblocking_third_party_lib {
using boost::asio::ip::tcp;

namespace third_party_lib {

// Simulation of a third party library that wants to perform read and write
// operations directly on a socket. It needs to be polled to determine whether
// it requires a read or write operation, and notified when the socket is ready
// for reading or writing.
class session
{
public:
  session(tcp::socket& socket)
    : socket_(socket)
  {
  }

  // Returns true if the third party library wants to be notified when the
  // socket is ready for reading.
  bool want_read() const
  {
    return state_ == reading;
  }

  // Notify that third party library that it should perform its read operation.
  void do_read(boost::system::error_code& ec)
  {
    if (std::size_t len = socket_.read_some(boost::asio::buffer(data_), ec))
    {
      write_buffer_ = boost::asio::buffer(data_, len);
      state_ = writing;
    }
  }

  // Returns true if the third party library wants to be notified when the
  // socket is ready for writing.
  bool want_write() const
  {
    return state_ == writing;
  }

  // Notify that third party library that it should perform its write operation.
  void do_write(boost::system::error_code& ec)
  {
    if (std::size_t len = socket_.write_some(
          boost::asio::buffer(write_buffer_), ec))
    {
      write_buffer_ = write_buffer_ + len;
      state_ = boost::asio::buffer_size(write_buffer_) > 0 ? writing : reading;
    }
  }

private:
  tcp::socket& socket_;
  enum { reading, writing } state_ = reading;
  std::array<char, 128> data_;
  boost::asio::const_buffer write_buffer_;
};

} // namespace third_party_lib

// The glue between asio's sockets and the third party library.
class connection
  : public std::enable_shared_from_this<connection>
{
public:
  connection(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    // Put the socket into non-blocking mode.
    socket_.non_blocking(true);

    do_operations();
  }

private:
  void do_operations()
  {
    auto self(shared_from_this());

    // Start a read operation if the third party library wants one.
    if (session_impl_.want_read() && !read_in_progress_)
    {
      read_in_progress_ = true;
      socket_.async_wait(tcp::socket::wait_read,
          [this, self](boost::system::error_code ec)
          {
            read_in_progress_ = false;

            // Notify third party library that it can perform a read.
            if (!ec)
              session_impl_.do_read(ec);

            // The third party library successfully performed a read on the
            // socket. Start new read or write operations based on what it now
            // wants.
            if (!ec || ec == boost::asio::error::would_block)
              do_operations();

            // Otherwise, an error occurred. Closing the socket cancels any
            // outstanding asynchronous read or write operations. The
            // connection object will be destroyed automatically once those
            // outstanding operations complete.
            else
              socket_.close();
          });
    }

    // Start a write operation if the third party library wants one.
    if (session_impl_.want_write() && !write_in_progress_)
    {
      write_in_progress_ = true;
      socket_.async_wait(tcp::socket::wait_write,
          [this, self](boost::system::error_code ec)
          {
            write_in_progress_ = false;

            // Notify third party library that it can perform a write.
            if (!ec)
              session_impl_.do_write(ec);

            // The third party library successfully performed a write on the
            // socket. Start new read or write operations based on what it now
            // wants.
            if (!ec || ec == boost::asio::error::would_block)
              do_operations();

            // Otherwise, an error occurred. Closing the socket cancels any
            // outstanding asynchronous read or write operations. The
            // connection object will be destroyed automatically once those
            // outstanding operations complete.
            else
              socket_.close();
          });
    }
  }

private:
  tcp::socket socket_;
  third_party_lib::session session_impl_{socket_};
  bool read_in_progress_ = false;
  bool write_in_progress_ = false;
};

class server
{
public:
  server(boost::asio::io_context& io_context, unsigned short port)
    : acceptor_(io_context, {tcp::v4(), port})
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<connection>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: third_party_lib <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/nonblocking/third_party_lib") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp11_nonblocking_third_party_lib

// libs/asio/example/cpp11/buffers/reference_counted.cpp
namespace ex_cpp11_buffers_reference_counted {
using boost::asio::ip::tcp;

// A reference-counted non-modifiable buffer class.
class shared_const_buffer
{
public:
  // Construct from a std::string.
  explicit shared_const_buffer(const std::string& data)
    : data_(new std::vector<char>(data.begin(), data.end())),
      buffer_(boost::asio::buffer(*data_))
  {
  }

  // Implement the ConstBufferSequence requirements.
  typedef boost::asio::const_buffer value_type;
  typedef const boost::asio::const_buffer* const_iterator;
  const boost::asio::const_buffer* begin() const { return &buffer_; }
  const boost::asio::const_buffer* end() const { return &buffer_ + 1; }

private:
  std::shared_ptr<std::vector<char> > data_;
  boost::asio::const_buffer buffer_;
};

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    do_write();
  }

private:
  void do_write()
  {
    std::time_t now = std::time(0);
    shared_const_buffer buffer(std::ctime(&now));

    auto self(shared_from_this());
    boost::asio::async_write(socket_, buffer,
        [self](boost::system::error_code /*ec*/, std::size_t /*length*/)
        {
        });
  }

  // The socket used to communicate with the client.
  tcp::socket socket_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: reference_counted <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    server s(io_context, std::atoi(argv[1]));

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/buffers/reference_counted") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "", 0, false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
)ex"));
}
}  // namespace ex_cpp11_buffers_reference_counted

// libs/asio/example/cpp11/chat/chat_server.cpp
namespace ex_cpp11_chat_chat_server {
#undef CHAT_MESSAGE_HPP
//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP


class chat_message
{
public:
  static constexpr std::size_t header_length = 4;
  static constexpr std::size_t max_body_length = 512;

  chat_message()
    : body_length_(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  std::size_t length() const
  {
    return header_length + body_length_;
  }

  const char* body() const
  {
    return data_ + header_length;
  }

  char* body()
  {
    return data_ + header_length;
  }

  std::size_t body_length() const
  {
    return body_length_;
  }

  void body_length(std::size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  bool decode_header()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    body_length_ = std::atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    char header[header_length + 1] = "";
    std::snprintf(header, header_length + 1,
        "%4d", static_cast<int>(body_length_));
    std::memcpy(data_, header, header_length);
  }

private:
  char data_[header_length + max_body_length];
  std::size_t body_length_;
};

#endif // CHAT_MESSAGE_HPP
// ---- end chat_message.hpp ----

using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<chat_message> chat_message_queue;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const chat_message& msg) = 0;
};

typedef std::shared_ptr<chat_participant> chat_participant_ptr;

//----------------------------------------------------------------------

class chat_room
{
public:
  void join(chat_participant_ptr participant)
  {
    participants_.insert(participant);
    for (auto msg: recent_msgs_)
      participant->deliver(msg);
  }

  void leave(chat_participant_ptr participant)
  {
    participants_.erase(participant);
  }

  void deliver(const chat_message& msg)
  {
    recent_msgs_.push_back(msg);
    while (recent_msgs_.size() > max_recent_msgs)
      recent_msgs_.pop_front();

    for (auto participant: participants_)
      participant->deliver(msg);
  }

private:
  std::set<chat_participant_ptr> participants_;
  enum { max_recent_msgs = 100 };
  chat_message_queue recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      room_(room)
  {
  }

  void start()
  {
    room_.join(shared_from_this());
    do_read_header();
  }

  void deliver(const chat_message& msg)
  {
    bool write_in_progress = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!write_in_progress)
    {
      do_write();
    }
  }

private:
  void do_read_header()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_read_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            room_.deliver(read_msg_);
            do_read_header();
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  void do_write()
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            room_.leave(shared_from_this());
          }
        });
  }

  tcp::socket socket_;
  chat_room& room_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

//----------------------------------------------------------------------

class chat_server
{
public:
  chat_server(boost::asio::io_context& io_context,
      const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, tcp::socket socket)
        {
          if (!ec)
          {
            std::make_shared<chat_session>(std::move(socket), room_)->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  chat_room room_;
};

//----------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      ex_out << "Usage: chat_server <port> [<port> ...]\n";
      return 1;
    }

    boost::asio::io_context io_context;

    std::list<chat_server> servers;
    for (int i = 1; i < argc; ++i)
    {
      tcp::endpoint endpoint(tcp::v4(), std::atoi(argv[i]));
      servers.emplace_back(io_context, endpoint);
    }

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/chat/chat_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "   5hello", 9)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received:    5hello)ex"));
}
}  // namespace ex_cpp11_chat_chat_server

// libs/asio/example/cpp11/chat/chat_client.cpp
namespace ex_cpp11_chat_chat_client {
#undef CHAT_MESSAGE_HPP
//
// chat_message.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef CHAT_MESSAGE_HPP
#define CHAT_MESSAGE_HPP


class chat_message
{
public:
  static constexpr std::size_t header_length = 4;
  static constexpr std::size_t max_body_length = 512;

  chat_message()
    : body_length_(0)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  std::size_t length() const
  {
    return header_length + body_length_;
  }

  const char* body() const
  {
    return data_ + header_length;
  }

  char* body()
  {
    return data_ + header_length;
  }

  std::size_t body_length() const
  {
    return body_length_;
  }

  void body_length(std::size_t new_length)
  {
    body_length_ = new_length;
    if (body_length_ > max_body_length)
      body_length_ = max_body_length;
  }

  bool decode_header()
  {
    char header[header_length + 1] = "";
    std::strncat(header, data_, header_length);
    body_length_ = std::atoi(header);
    if (body_length_ > max_body_length)
    {
      body_length_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    char header[header_length + 1] = "";
    std::snprintf(header, header_length + 1,
        "%4d", static_cast<int>(body_length_));
    std::memcpy(data_, header, header_length);
  }

private:
  char data_[header_length + max_body_length];
  std::size_t body_length_;
};

#endif // CHAT_MESSAGE_HPP
// ---- end chat_message.hpp ----

using boost::asio::ip::tcp;

typedef std::deque<chat_message> chat_message_queue;

class chat_client
{
public:
  chat_client(boost::asio::io_context& io_context,
      const tcp::resolver::results_type& endpoints)
    : io_context_(io_context),
      socket_(io_context)
  {
    do_connect(endpoints);
  }

  void write(const chat_message& msg)
  {
    boost::asio::post(io_context_,
        [this, msg]()
        {
          bool write_in_progress = !write_msgs_.empty();
          write_msgs_.push_back(msg);
          if (!write_in_progress)
          {
            do_write();
          }
        });
  }

  void close()
  {
    boost::asio::post(io_context_, [this]() { socket_.close(); });
  }

private:
  void do_connect(const tcp::resolver::results_type& endpoints)
  {
    boost::asio::async_connect(socket_, endpoints,
        [this](boost::system::error_code ec, tcp::endpoint)
        {
          if (!ec)
          {
            do_read_header();
          }
        });
  }

  void do_read_header()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.data(), chat_message::header_length),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec && read_msg_.decode_header())
          {
            do_read_body();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_read_body()
  {
    boost::asio::async_read(socket_,
        boost::asio::buffer(read_msg_.body(), read_msg_.body_length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            ex_out.write(read_msg_.body(), read_msg_.body_length());
            ex_out << "\n";
            do_read_header();
          }
          else
          {
            socket_.close();
          }
        });
  }

  void do_write()
  {
    boost::asio::async_write(socket_,
        boost::asio::buffer(write_msgs_.front().data(),
          write_msgs_.front().length()),
        [this](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
          {
            write_msgs_.pop_front();
            if (!write_msgs_.empty())
            {
              do_write();
            }
          }
          else
          {
            socket_.close();
          }
        });
  }

private:
  boost::asio::io_context& io_context_;
  tcp::socket socket_;
  chat_message read_msg_;
  chat_message_queue write_msgs_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: chat_client <host> <port>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(argv[1], argv[2]);
    chat_client c(io_context, endpoints);

    std::thread t([&io_context](){ io_context.run(); });

    char line[chat_message::max_body_length + 1];
    while (ex_in.getline(line, chat_message::max_body_length + 1))
    {
      chat_message msg;
      msg.body_length(std::strlen(line));
      std::memcpy(msg.body(), line, msg.body_length());
      msg.encode_header();
      c.write(msg);
    }

    c.close();
    t.join();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/chat/chat_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello chat
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_chat_chat_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
#endif
}
}  // namespace ex_cpp11_chat_chat_client

// libs/asio/example/cpp11/tutorial/daytime2/server.cpp
namespace ex_cpp11_tutorial_daytime2_server {
using boost::asio::ip::tcp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), ex_port_number()));

    for (;;)
    {
      tcp::socket socket(io_context);
      acceptor.accept(socket);

      std::string message = make_daytime_string();

      boost::system::error_code ignored_error;
      boost::asio::write(socket, boost::asio::buffer(message), ignored_error);
    }
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime2/server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "", 0, false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
)ex"));
}
}  // namespace ex_cpp11_tutorial_daytime2_server

// libs/asio/example/cpp11/tutorial/daytime3/server.cpp
namespace ex_cpp11_tutorial_daytime3_server {
using boost::asio::ip::tcp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection
  : public std::enable_shared_from_this<tcp_connection>
{
public:
  typedef std::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_context& io_context)
  {
    return pointer(new tcp_connection(io_context));
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    message_ = make_daytime_string();

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        std::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

private:
  tcp_connection(boost::asio::io_context& io_context)
    : socket_(io_context)
  {
  }

  void handle_write(const boost::system::error_code& /*error*/,
      size_t /*bytes_transferred*/)
  {
  }

  tcp::socket socket_;
  std::string message_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_context& io_context)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), ex_port_number()))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    tcp_connection::pointer new_connection =
      tcp_connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        std::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};

int example_main()
{
  try
  {
    boost::asio::io_context io_context;
    tcp_server server(io_context);
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime3/server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "", 0, false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
)ex"));
}
}  // namespace ex_cpp11_tutorial_daytime3_server

// libs/asio/example/cpp11/tutorial/daytime5/server.cpp
namespace ex_cpp11_tutorial_daytime5_server {
using boost::asio::ip::udp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context;

    udp::socket socket(io_context, udp::endpoint(udp::v4(), ex_port_number()));

    for (;;)
    {
      std::array<char, 1> recv_buf;
      udp::endpoint remote_endpoint;
      socket.receive_from(boost::asio::buffer(recv_buf), remote_endpoint);

      std::string message = make_daytime_string();

      boost::system::error_code ignored_error;
      socket.send_to(boost::asio::buffer(message),
          remote_endpoint, 0, ignored_error);
    }
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime5/server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_udp_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_udp_exchange(ex_port, "x", false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
)ex"));
}
}  // namespace ex_cpp11_tutorial_daytime5_server

// libs/asio/example/cpp11/tutorial/daytime6/server.cpp
namespace ex_cpp11_tutorial_daytime6_server {
using boost::asio::ip::udp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class udp_server
{
public:
  udp_server(boost::asio::io_context& io_context)
    : socket_(io_context, udp::endpoint(udp::v4(), ex_port_number()))
  {
    start_receive();
  }

private:
  void start_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        std::bind(&udp_server::handle_receive, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void handle_receive(const boost::system::error_code& error,
      std::size_t /*bytes_transferred*/)
  {
    if (!error)
    {
      std::shared_ptr<std::string> message(
          new std::string(make_daytime_string()));

      socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
          std::bind(&udp_server::handle_send, this, message,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));

      start_receive();
    }
  }

  void handle_send(std::shared_ptr<std::string> /*message*/,
      const boost::system::error_code& /*error*/,
      std::size_t /*bytes_transferred*/)
  {
  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  std::array<char, 1> recv_buffer_;
};

int example_main()
{
  try
  {
    boost::asio::io_context io_context;
    udp_server server(io_context);
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime6/server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_udp_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_udp_exchange(ex_port, "x", false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
)ex"));
}
}  // namespace ex_cpp11_tutorial_daytime6_server

// libs/asio/example/cpp11/tutorial/daytime7/server.cpp
namespace ex_cpp11_tutorial_daytime7_server {
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection
  : public std::enable_shared_from_this<tcp_connection>
{
public:
  typedef std::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_context& io_context)
  {
    return pointer(new tcp_connection(io_context));
  }

  tcp::socket& socket()
  {
    return socket_;
  }

  void start()
  {
    message_ = make_daytime_string();

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        std::bind(&tcp_connection::handle_write, shared_from_this()));
  }

private:
  tcp_connection(boost::asio::io_context& io_context)
    : socket_(io_context)
  {
  }

  void handle_write()
  {
  }

  tcp::socket socket_;
  std::string message_;
};

class tcp_server
{
public:
  tcp_server(boost::asio::io_context& io_context)
    : io_context_(io_context),
      acceptor_(io_context, tcp::endpoint(tcp::v4(), ex_port_number()))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    tcp_connection::pointer new_connection =
      tcp_connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
        std::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_connection->start();
    }

    start_accept();
  }

  boost::asio::io_context& io_context_;
  tcp::acceptor acceptor_;
};

class udp_server
{
public:
  udp_server(boost::asio::io_context& io_context)
    : socket_(io_context, udp::endpoint(udp::v4(), ex_port_number()))
  {
    start_receive();
  }

private:
  void start_receive()
  {
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        std::bind(&udp_server::handle_receive, this,
          boost::asio::placeholders::error));
  }

  void handle_receive(const boost::system::error_code& error)
  {
    if (!error)
    {
      std::shared_ptr<std::string> message(
          new std::string(make_daytime_string()));

      socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
          std::bind(&udp_server::handle_send, this, message));

      start_receive();
    }
  }

  void handle_send(std::shared_ptr<std::string> /*message*/)
  {
  }

  udp::socket socket_;
  udp::endpoint remote_endpoint_;
  std::array<char, 1> recv_buffer_;
};

int example_main()
{
  try
  {
    boost::asio::io_context io_context;
    tcp_server server1(io_context);
    udp_server server2(io_context);
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime7/server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "", 0, false) + ex_udp_exchange(ex_port, "x", false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
received: a line
)ex"));
}
}  // namespace ex_cpp11_tutorial_daytime7_server

// libs/asio/example/cpp11/iostreams/daytime_server.cpp
namespace ex_cpp11_iostreams_daytime_server {
using boost::asio::ip::tcp;

std::string make_daytime_string()
{
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context;

    tcp::endpoint endpoint(tcp::v4(), ex_port_number());
    tcp::acceptor acceptor(io_context, endpoint);

    for (;;)
    {
      tcp::iostream stream;
      boost::system::error_code ec;
      acceptor.accept(stream.socket(), ec);
      if (!ec)
      {
        stream << make_daytime_string();
      }
    }
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/iostreams/daytime_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "", 0, false)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: a line
)ex"));
}
}  // namespace ex_cpp11_iostreams_daytime_server

// libs/asio/example/cpp11/tutorial/daytime1/client.cpp
namespace ex_cpp11_tutorial_daytime1_client {
using boost::asio::ip::tcp;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints =
      resolver.resolve(argv[1], ex_port);

    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);

    for (;;)
    {
      std::array<char, 128> buf;
      boost::system::error_code error;

      size_t len = socket.read_some(boost::asio::buffer(buf), error);

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      ex_out.write(buf.data(), len);
    }
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime1/client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_tutorial_daytime2_server::example_main, std::vector<std::string>{"server"}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Wed Nov 11 00:01:12 2026
)ex"));
#endif
}
}  // namespace ex_cpp11_tutorial_daytime1_client

// libs/asio/example/cpp11/services/daytime_client.cpp
namespace ex_cpp11_services_daytime_client {
#undef SERVICES_LOGGER_HPP
//
// logger.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERVICES_LOGGER_HPP
#define SERVICES_LOGGER_HPP

// ---- inlined basic_logger.hpp ----
#undef SERVICES_BASIC_LOGGER_HPP
//
// basic_logger.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERVICES_BASIC_LOGGER_HPP
#define SERVICES_BASIC_LOGGER_HPP


namespace services {

/// Class to provide simple logging functionality. Use the services::logger
/// typedef.
template <typename Service>
class basic_logger
{
public:
  /// The type of the service that will be used to provide timer operations.
  typedef Service service_type;

  /// The native implementation type of the timer.
  typedef typename service_type::impl_type impl_type;

  /// Constructor.
  /**
   * This constructor creates a logger.
   *
   * @param context The execution context used to locate the logger service.
   *
   * @param identifier An identifier for this logger.
   */
  explicit basic_logger(boost::asio::execution_context& context,
      const std::string& identifier)
    : service_(boost::asio::use_service<Service>(context)),
      impl_(service_.null())
  {
    service_.create(impl_, identifier);
  }

  basic_logger(const basic_logger&) = delete;
  basic_logger& operator=(basic_logger&) = delete;

  /// Destructor.
  ~basic_logger()
  {
    service_.destroy(impl_);
  }

  /// Set the output file for all logger instances.
  void use_file(const std::string& file)
  {
    service_.use_file(impl_, file);
  }

  /// Log a message.
  void log(const std::string& message)
  {
    service_.log(impl_, message);
  }

private:
  /// The backend service implementation.
  service_type& service_;

  /// The underlying native implementation.
  impl_type impl_;
};

} // namespace services

#endif // SERVICES_BASIC_LOGGER_HPP
// ---- end basic_logger.hpp ----
// ---- inlined logger_service.hpp ----
#undef SERVICES_LOGGER_SERVICE_HPP
//
// logger_service.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERVICES_LOGGER_SERVICE_HPP
#define SERVICES_LOGGER_SERVICE_HPP


namespace services {

/// Service implementation for the logger.
class logger_service
  : public boost::asio::execution_context::service
{
public:
  /// The type used to identify this service in the execution context.
  typedef logger_service key_type;

  /// The backend implementation of a logger.
  struct logger_impl
  {
    explicit logger_impl(const std::string& ident) : identifier(ident) {}
    std::string identifier;
  };

  /// The type for an implementation of the logger.
  typedef logger_impl* impl_type;

  /// Constructor creates a thread to run a private io_context.
  logger_service(boost::asio::execution_context& context)
    : boost::asio::execution_context::service(context),
      work_io_context_(),
      work_(boost::asio::make_work_guard(work_io_context_)),
      work_thread_([this]{ work_io_context_.run(); })
  {
  }

  logger_service(const logger_service&) = delete;
  logger_service& operator=(const logger_service&) = delete;;

  /// Destructor shuts down the private io_context.
  ~logger_service()
  {
    /// Indicate that we have finished with the private io_context. Its
    /// io_context::run() function will exit once all other work has completed.
    work_.reset();
    if (work_thread_.joinable())
      work_thread_.join();
  }

  /// Destroy all user-defined handler objects owned by the service.
  void shutdown()
  {
  }

  /// Return a null logger implementation.
  impl_type null() const
  {
    return 0;
  }

  /// Create a new logger implementation.
  void create(impl_type& impl, const std::string& identifier)
  {
    impl = new logger_impl(identifier);
  }

  /// Destroy a logger implementation.
  void destroy(impl_type& impl)
  {
    delete impl;
    impl = null();
  }

  /// Set the output file for the logger. The current implementation sets the
  /// output file for all logger instances, and so the impl parameter is not
  /// actually needed. It is retained here to illustrate how service functions
  /// are typically defined.
  void use_file(impl_type& /*impl*/, const std::string& file)
  {
    // Pass the work of opening the file to the background thread.
    boost::asio::post(work_io_context_,
        std::bind(&logger_service::use_file_impl, this, file));
  }

  /// Log a message.
  void log(impl_type& impl, const std::string& message)
  {
    // Format the text to be logged.
    std::ostringstream os;
    os << impl->identifier << ": " << message;

    // Pass the work of writing to the file to the background thread.
    boost::asio::post(work_io_context_,
        std::bind(&logger_service::log_impl, this, os.str()));
  }

private:
  /// Helper function used to open the output file from within the private
  /// io_context's thread.
  void use_file_impl(const std::string& file)
  {
    ofstream_.close();
    ofstream_.clear();
    ofstream_.open(file.c_str());
  }

  /// Helper function used to log a message from within the private io_context's
  /// thread.
  void log_impl(const std::string& text)
  {
    ofstream_ << text << std::endl;
  }

  /// Private io_context used for performing logging operations.
  boost::asio::io_context work_io_context_;

  /// Work for the private io_context to perform. If we do not give the
  /// io_context some work to do then the io_context::run() function will exit
  /// immediately.
  boost::asio::executor_work_guard<
      boost::asio::io_context::executor_type> work_;

  /// Thread used for running the work io_context's run loop.
  std::thread work_thread_;

  /// The file to which log messages will be written.
  std::ofstream ofstream_;
};

} // namespace services

#endif // SERVICES_LOGGER_SERVICE_HPP
// ---- end logger_service.hpp ----

namespace services {

/// Typedef for typical logger usage.
typedef basic_logger<logger_service> logger;

} // namespace services

#endif // SERVICES_LOGGER_HPP
// ---- end logger.hpp ----

using boost::asio::ip::tcp;

char read_buffer[1024];

void read_handler(const boost::system::error_code& e,
    std::size_t bytes_transferred, tcp::socket* s)
{
  if (!e)
  {
    ex_out.write(read_buffer, bytes_transferred);

    s->async_read_some(boost::asio::buffer(read_buffer),
        std::bind(read_handler, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred, s));
  }
  else
  {
    boost::asio::execution_context& context = boost::asio::query(
        s->get_executor(), boost::asio::execution::context);
    services::logger logger(context, "read_handler");

    std::string msg = "Read error: ";
    msg += e.message();
    logger.log(msg);
  }
}

void connect_handler(const boost::system::error_code& e, tcp::socket* s)
{
  boost::asio::execution_context& context = boost::asio::query(
      s->get_executor(), boost::asio::execution::context);
  services::logger logger(context, "connect_handler");

  if (!e)
  {
    logger.log("Connection established");

    s->async_read_some(boost::asio::buffer(read_buffer),
        std::bind(read_handler, boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred, s));
  }
  else
  {
    std::string msg = "Unable to establish connection: ";
    msg += e.message();
    logger.log(msg);
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_context io_context;

    // Set the name of the file that all logger instances will use.
    services::logger logger(io_context, "");
    logger.use_file(ex_temp_path("log.txt"));

    // Resolve the address corresponding to the given host.
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints =
      resolver.resolve(argv[1], ex_port);

    // Start an asynchronous connect.
    tcp::socket socket(io_context);
    boost::asio::async_connect(socket, endpoints,
        std::bind(connect_handler,
          boost::asio::placeholders::error, &socket));

    // Run the io_context until all operations have finished.
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}
// ---- merged logger_service.cpp (the same program) ----
//
// logger_service.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

TEST_CASE("example: cpp11/services/daytime_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_tutorial_daytime2_server::example_main, std::vector<std::string>{"server"}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Wed Nov 11 00:01:15 2026
)ex"));
#endif
}
}  // namespace ex_cpp11_services_daytime_client

// libs/asio/example/cpp11/tutorial/daytime4/client.cpp
namespace ex_cpp11_tutorial_daytime4_client {
using boost::asio::ip::udp;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: client <host>" << std::endl;
      return 1;
    }

    boost::asio::io_context io_context;

    udp::resolver resolver(io_context);
    udp::endpoint receiver_endpoint =
      *resolver.resolve(udp::v4(), argv[1], ex_port).begin();

    udp::socket socket(io_context);
    socket.open(udp::v4());

    std::array<char, 1> send_buf  = {{ 0 }};
    socket.send_to(boost::asio::buffer(send_buf), receiver_endpoint);

    std::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    size_t len = socket.receive_from(
        boost::asio::buffer(recv_buf), sender_endpoint);

    ex_out.write(recv_buf.data(), len);
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/tutorial/daytime4/client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_udp_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_tutorial_daytime5_server::example_main, std::vector<std::string>{"server"}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Wed Nov 11 00:01:17 2026
)ex"));
#endif
}
}  // namespace ex_cpp11_tutorial_daytime4_client

// libs/asio/example/cpp11/futures/daytime_client.cpp
namespace ex_cpp11_futures_daytime_client {
using boost::asio::ip::udp;

void get_daytime(boost::asio::io_context& io_context, const char* hostname)
{
  try
  {
    udp::resolver resolver(io_context);

    std::future<udp::resolver::results_type> endpoints =
      resolver.async_resolve(
          udp::v4(), hostname, ex_port,
          boost::asio::use_future);

    // The async_resolve operation above returns the endpoints as a future
    // value that is not retrieved ...

    udp::socket socket(io_context, udp::v4());

    std::array<char, 1> send_buf  = {{ 0 }};
    std::future<std::size_t> send_length =
      socket.async_send_to(boost::asio::buffer(send_buf),
          *endpoints.get().begin(), // ... until here. This call may block.
          boost::asio::use_future);

    // Do other things here while the send completes.

    send_length.get(); // Blocks until the send is complete. Throws any errors.

    std::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    std::future<std::size_t> recv_length =
      socket.async_receive_from(
          boost::asio::buffer(recv_buf),
          sender_endpoint,
          boost::asio::use_future);

    // Do other things here while the receive completes.

    ex_out.write(
        recv_buf.data(),
        recv_length.get()); // Blocks until receive is complete.
  }
  catch (boost::system::system_error& e)
  {
    ex_out << e.what() << std::endl;
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }

    // We run the io_context off in its own thread so that it operates
    // completely asynchronously with respect to the rest of the program.
    boost::asio::io_context io_context;
    auto work = boost::asio::make_work_guard(io_context);
    std::thread thread([&io_context](){ io_context.run(); });

    get_daytime(io_context, argv[1]);

    io_context.stop();
    thread.join();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/futures/daytime_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_udp_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_tutorial_daytime5_server::example_main, std::vector<std::string>{"server"}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Wed Nov 11 00:01:20 2026
)ex"));
#endif
}
}  // namespace ex_cpp11_futures_daytime_client

// libs/asio/example/cpp11/iostreams/daytime_client.cpp
namespace ex_cpp11_iostreams_daytime_client {
using boost::asio::ip::tcp;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: daytime_client <host>" << std::endl;
      return 1;
    }

    tcp::iostream s(argv[1], ex_port);
    if (!s)
    {
      ex_out << "Unable to connect: " << s.error().message() << std::endl;
      return 1;
    }

    std::string line;
    std::getline(s, line);
    ex_out << line << std::endl;
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/iostreams/daytime_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_iostreams_daytime_server::example_main, std::vector<std::string>{"server"}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Wed Nov 11 00:01:22 2026
)ex"));
#endif
}
}  // namespace ex_cpp11_iostreams_daytime_client

// libs/asio/example/cpp11/timeouts/blocking_tcp_client.cpp
namespace ex_cpp11_timeouts_blocking_tcp_client {
using boost::asio::ip::tcp;

//----------------------------------------------------------------------

//
// This class manages socket timeouts by running the io_context using the timed
// io_context::run_for() member function. Each asynchronous operation is given
// a timeout within which it must complete. The socket operations themselves
// use lambdas as completion handlers. For a given socket operation, the client
// object runs the io_context to block thread execution until the operation
// completes or the timeout is reached. If the io_context::run_for() function
// times out, the socket is closed and the outstanding asynchronous operation
// is cancelled.
//
class client
{
public:
  void connect(const std::string& host, const std::string& service,
      std::chrono::steady_clock::duration timeout)
  {
    // Resolve the host name and service to a list of endpoints.
    auto endpoints = tcp::resolver(io_context_).resolve(host, service);

    // Start the asynchronous operation itself. The lambda that is used as a
    // callback will update the error variable when the operation completes.
    // The blocking_udp_client.cpp example shows how you can use std::bind
    // rather than a lambda.
    boost::system::error_code error;
    boost::asio::async_connect(socket_, endpoints,
        [&](const boost::system::error_code& result_error,
            const tcp::endpoint& /*result_endpoint*/)
        {
          error = result_error;
        });

    // Run the operation until it completes, or until the timeout.
    run(timeout);

    // Determine whether a connection was successfully established.
    if (error)
      throw boost::system::system_error(error);
  }

  std::string read_line(std::chrono::steady_clock::duration timeout)
  {
    // Start the asynchronous operation. The lambda that is used as a callback
    // will update the error and n variables when the operation completes. The
    // blocking_udp_client.cpp example shows how you can use std::bind rather
    // than a lambda.
    boost::system::error_code error;
    std::size_t n = 0;
    boost::asio::async_read_until(socket_,
        boost::asio::dynamic_buffer(input_buffer_), '\n',
        [&](const boost::system::error_code& result_error,
            std::size_t result_n)
        {
          error = result_error;
          n = result_n;
        });

    // Run the operation until it completes, or until the timeout.
    run(timeout);

    // Determine whether the read completed successfully.
    if (error)
      throw boost::system::system_error(error);

    std::string line(input_buffer_.substr(0, n - 1));
    input_buffer_.erase(0, n);
    return line;
  }

  void write_line(const std::string& line,
      std::chrono::steady_clock::duration timeout)
  {
    std::string data = line + "\n";

    // Start the asynchronous operation itself. The lambda that is used as a
    // callback will update the error variable when the operation completes.
    // The blocking_udp_client.cpp example shows how you can use std::bind
    // rather than a lambda.
    boost::system::error_code error;
    boost::asio::async_write(socket_, boost::asio::buffer(data),
        [&](const boost::system::error_code& result_error,
            std::size_t /*result_n*/)
        {
          error = result_error;
        });

    // Run the operation until it completes, or until the timeout.
    run(timeout);

    // Determine whether the read completed successfully.
    if (error)
      throw boost::system::system_error(error);
  }

private:
  void run(std::chrono::steady_clock::duration timeout)
  {
    // Restart the io_context, as it may have been left in the "stopped" state
    // by a previous operation.
    io_context_.restart();

    // Block until the asynchronous operation has completed, or timed out. If
    // the pending asynchronous operation is a composed operation, the deadline
    // applies to the entire operation, rather than individual operations on
    // the socket.
    io_context_.run_for(timeout);

    // If the asynchronous operation completed successfully then the io_context
    // would have been stopped due to running out of work. If it was not
    // stopped, then the io_context::run_for call must have timed out.
    if (!io_context_.stopped())
    {
      // Close the socket to cancel the outstanding asynchronous operation.
      socket_.close();

      // Run the io_context again until the operation completes.
      io_context_.run();
    }
  }

  boost::asio::io_context io_context_;
  tcp::socket socket_{io_context_};
  std::string input_buffer_;
};

//----------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 4)
    {
      ex_out << "Usage: blocking_tcp_client <host> <port> <message>\n";
      return 1;
    }

    client c;
    c.connect(argv[1], argv[2], std::chrono::seconds(10));

    auto time_sent = std::chrono::steady_clock::now();

    c.write_line(argv[3], std::chrono::seconds(10));

    for (;;)
    {
      std::string line = c.read_line(std::chrono::seconds(10));

      // Keep going until we get back the line that was sent.
      if (line == argv[3])
        break;
    }

    auto time_received = std::chrono::steady_clock::now();

    ex_out << "Round trip time: ";
    ex_out << std::chrono::duration_cast<
      std::chrono::microseconds>(
        time_received - time_sent).count();
    ex_out << " microseconds\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/timeouts/blocking_tcp_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_echo_blocking_tcp_echo_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("hello");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Round trip time: 75 microseconds
)ex"));
#endif
}
}  // namespace ex_cpp11_timeouts_blocking_tcp_client

// libs/asio/example/cpp11/timeouts/blocking_token_tcp_client.cpp
namespace ex_cpp11_timeouts_blocking_token_tcp_client {
using boost::asio::ip::tcp;

// NOTE: This example uses the new form of the boost::asio::async_result trait.
// For an example that works with the Networking TS style of completion tokens,
// please see an older version of asio.

// We will use our sockets only with an io_context.
using tcp_socket = boost::asio::basic_stream_socket<
    tcp, boost::asio::io_context::executor_type>;

//----------------------------------------------------------------------

// A custom completion token that makes asynchronous operations behave as
// though they are blocking calls with a timeout.
struct close_after
{
  close_after(std::chrono::steady_clock::duration t, tcp_socket& s)
    : timeout_(t), socket_(s)
  {
  }

  // The maximum time to wait for an asynchronous operation to complete.
  std::chrono::steady_clock::duration timeout_;

  // The socket to be closed if the operation does not complete in time.
  tcp_socket& socket_;
};

}  // namespace ex_cpp11_timeouts_blocking_token_tcp_client (reopened after the specialization)
 namespace boost {
namespace asio {

// The async_result template is specialised to allow the close_after token to
// be used with asynchronous operations that have a completion signature of
// void(error_code, T). Generalising this for all completion signature forms is
// left as an exercise for the reader.
template <typename T>
class async_result<::ex_cpp11_timeouts_blocking_token_tcp_client::close_after, void(boost::system::error_code, T)>
{
public:
  // The initiate() function is used to launch the asynchronous operation by
  // calling the operation's initiation function object. For the close_after
  // completion token, we use this function to run the io_context until the
  // operation is complete.
  template <typename Init, typename... Args>
  static T initiate(Init init, ::ex_cpp11_timeouts_blocking_token_tcp_client::close_after token, Args&&... args)
  {
    boost::asio::io_context& io_context = boost::asio::query(
        token.socket_.get_executor(), boost::asio::execution::context);

    // Call the operation's initiation function object to start the operation.
    // A lambda is supplied as the completion handler, to be called when the
    // operation completes.
    boost::system::error_code error;
    T result;
    init([&](boost::system::error_code e, T t)
        {
          error = e;
          result = t;
        }, std::forward<Args>(args)...);

    // Restart the io_context, as it may have been left in the "stopped" state
    // by a previous operation.
    io_context.restart();

    // Block until the asynchronous operation has completed, or timed out. If
    // the pending asynchronous operation is a composed operation, the deadline
    // applies to the entire operation, rather than individual operations on
    // the socket.
    io_context.run_for(token.timeout_);

    // If the asynchronous operation completed successfully then the io_context
    // would have been stopped due to running out of work. If it was not
    // stopped, then the io_context::run_for call must have timed out and the
    // operation is still incomplete.
    if (!io_context.stopped())
    {
      // Close the socket to cancel the outstanding asynchronous operation.
      token.socket_.close();

      // Run the io_context again until the operation completes.
      io_context.run();
    }

    // If the operation failed, throw an exception. Otherwise return the result.
    return error ? throw boost::system::system_error(error) : result;
  }
};

} // namespace asio
} // namespace boost
namespace ex_cpp11_timeouts_blocking_token_tcp_client {

//----------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 4)
    {
      ex_out << "Usage: blocking_tcp_client <host> <port> <message>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    // Resolve the host name and service to a list of endpoints.
    auto endpoints = tcp::resolver(io_context).resolve(argv[1], argv[2]);

    tcp_socket socket(io_context);

    // Run an asynchronous connect operation with a timeout.
    boost::asio::async_connect(socket, endpoints,
        close_after(std::chrono::seconds(10), socket));

    auto time_sent = std::chrono::steady_clock::now();

    // Run an asynchronous write operation with a timeout.
    std::string msg = argv[3] + std::string("\n");
    boost::asio::async_write(socket, boost::asio::buffer(msg),
        close_after(std::chrono::seconds(10), socket));

    for (std::string input_buffer;;)
    {
      // Run an asynchronous read operation with a timeout.
      std::size_t n = boost::asio::async_read_until(socket,
          boost::asio::dynamic_buffer(input_buffer), '\n',
          close_after(std::chrono::seconds(10), socket));

      std::string line(input_buffer.substr(0, n - 1));
      input_buffer.erase(0, n);

      // Keep going until we get back the line that was sent.
      if (line == argv[3])
        break;
    }

    auto time_received = std::chrono::steady_clock::now();

    ex_out << "Round trip time: ";
    ex_out << std::chrono::duration_cast<
      std::chrono::microseconds>(
        time_received - time_sent).count();
    ex_out << " microseconds\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/timeouts/blocking_token_tcp_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_echo_blocking_tcp_echo_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("hello");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Round trip time: 79 microseconds
)ex"));
#endif
}
}  // namespace ex_cpp11_timeouts_blocking_token_tcp_client

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/local/stream_server.cpp
namespace ex_cpp11_local_stream_server {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(stream_protocol::socket sock)
    : socket_(std::move(sock))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (!ec)
            do_write(length);
        });
  }

  void do_write(std::size_t length)
  {
    auto self(shared_from_this());
    boost::asio::async_write(socket_,
        boost::asio::buffer(data_, length),
        [this, self](boost::system::error_code ec, std::size_t /*length*/)
        {
          if (!ec)
            do_read();
        });
  }

  // The socket used to communicate with the client.
  stream_protocol::socket socket_;

  // Buffer used to store data received from the client.
  std::array<char, 1024> data_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, const std::string& file)
    : acceptor_(io_context, stream_protocol::endpoint(file))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, stream_protocol::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  stream_protocol::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: stream_server <file>\n";
      ex_out << "*** WARNING: existing file is removed ***\n";
      return 1;
    }

    boost::asio::io_context io_context;

    std::remove(argv[1]);
    server s(io_context, argv[1]);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

TEST_CASE("example: cpp11/local/stream_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_local_path("stream.sock");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_local_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
#endif
}
}  // namespace ex_cpp11_local_stream_server
#endif

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/local/stream_client.cpp
namespace ex_cpp11_local_stream_client {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;

constexpr std::size_t max_length = 1024;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: stream_client <file>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    stream_protocol::socket s(io_context);
    s.connect(stream_protocol::endpoint(argv[1]));

    ex_out << "Enter message: ";
    char request[max_length];
    ex_in.getline(request, max_length);
    size_t request_length = std::strlen(request);
    boost::asio::write(s, boost::asio::buffer(request, request_length));

    char reply[max_length];
    size_t reply_length = boost::asio::read(s,
        boost::asio::buffer(reply, request_length));
    ex_out << "Reply is: ";
    ex_out.write(reply, reply_length);
    ex_out << "\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

TEST_CASE("example: cpp11/local/stream_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello local
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_local_path("stream.sock"); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_local_stream_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Enter message: Reply is: hello local
)ex"));
#endif
}
}  // namespace ex_cpp11_local_stream_client
#endif

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/local/iostream_client.cpp
namespace ex_cpp11_local_iostream_client {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;

constexpr std::size_t max_length = 1024;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: iostream_client <file>\n";
      return 1;
    }

    stream_protocol::endpoint ep(argv[1]);
    stream_protocol::iostream s(ep);
    if (!s)
    {
      ex_out << "Unable to connect: " << s.error().message() << std::endl;
      return 1;
    }

    ex_out << "Enter message: ";
    char request[max_length];
    ex_in.getline(request, max_length);
    size_t length = std::strlen(request);
    s << request;

    char reply[max_length];
    s.read(reply, length);
    ex_out << "Reply is: ";
    ex_out.write(reply, length);
    ex_out << "\n";
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

TEST_CASE("example: cpp11/local/iostream_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello local
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_local_path("stream.sock"); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_local_stream_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Enter message: Reply is: hello local
)ex"));
#endif
}
}  // namespace ex_cpp11_local_iostream_client
#endif

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/local/fd_passing_stream_server.cpp
namespace ex_cpp11_local_fd_passing_stream_server {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

using boost::asio::local::stream_protocol;

class session
  : public std::enable_shared_from_this<session>
{
public:
  session(stream_protocol::socket sock)
    : socket_(std::move(sock))
  {
  }

  void start()
  {
    do_read();
  }

private:
  void do_read()
  {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        [this, self](boost::system::error_code ec, std::size_t length)
        {
          if (ec)
            return;

          assert(length < data_.size());
          data_[length] = 0;
          do_write(data_.data());
        });
  }

  void do_write(const char* filename)
  {
    auto self(shared_from_this());
    socket_.async_wait(stream_protocol::socket::wait_write,
        [this, self, filename](boost::system::error_code ec)
        {
          if (ec)
            return;

          FILE* f(::fopen(filename, "w+"));
          if (!f)
            return;

          struct msghdr msg = {};
          char buf[] = { 0 };
          struct iovec iov = { buf, sizeof(buf) };
          msg.msg_iov = &iov;
          msg.msg_iovlen = 1;

          union
          {
            struct cmsghdr align;
            char buf[CMSG_SPACE(sizeof(int))];
          } cmsgu;
          msg.msg_control = cmsgu.buf;
          msg.msg_controllen = sizeof(cmsgu.buf);

          struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
          cmsg->cmsg_len = CMSG_LEN(sizeof(int));
          cmsg->cmsg_level = SOL_SOCKET;
          cmsg->cmsg_type = SCM_RIGHTS;
          int fd = ::fileno(f);
          std::memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));

          ssize_t s(::sendmsg(socket_.native_handle(), &msg, 0));
          ::fclose(f);
          if (s != -1)
            do_read();
        });
  }

  // The socket used to communicate with the client.
  stream_protocol::socket socket_;

  // Buffer used to store data received from the client.
  std::array<char, 1024> data_;
};

class server
{
public:
  server(boost::asio::io_context& io_context, const std::string& file)
    : acceptor_(io_context, stream_protocol::endpoint(file))
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(
        [this](boost::system::error_code ec, stream_protocol::socket socket)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket))->start();
          }

          do_accept();
        });
  }

  stream_protocol::acceptor acceptor_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: fd_passing_stream_server <socketfile>\n";
      ex_out << "*** WARNING: existing file is removed ***\n";
      return 1;
    }

    boost::asio::io_context io_context;

    std::remove(argv[1]);
    server s(io_context, argv[1]);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

TEST_CASE("example: cpp11/local/fd_passing_stream_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_local_path("stream.sock");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_local_exchange(ex_port, ex_temp_path("fd-target.txt"), 1, false)));
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(received: text
)ex"));
#endif
}
}  // namespace ex_cpp11_local_fd_passing_stream_server
#endif

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/asio/example/cpp11/local/fd_passing_stream_client.cpp
namespace ex_cpp11_local_fd_passing_stream_client {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)


using boost::asio::local::stream_protocol;

constexpr std::size_t max_length = 1024;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      ex_out << "Usage: fd_passing_stream_client <serversocket>\n";
      return 1;
    }

    boost::asio::io_context io_context;

    stream_protocol::socket s(io_context);
    s.connect(stream_protocol::endpoint(argv[1]));

    ex_out << "Enter path to write to: ";
    char request[max_length];
    std::strcpy(request, ex_temp_path("fd-passing.txt").c_str());
    size_t request_length = std::strlen(request);
    boost::asio::write(s, boost::asio::buffer(request, request_length));

    char reply[max_length];
    struct msghdr msg = {};
    struct iovec iov = { reply, sizeof(reply) };
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    union
    {
      struct cmsghdr align;
      char buf[CMSG_SPACE(sizeof(int))];
    } cmsgu;
    msg.msg_control = cmsgu.buf;
    msg.msg_controllen = sizeof(cmsgu.buf);

    ::recvmsg(s.native_handle(), &msg, 0);

    int fd = -1;
    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    while (cmsg != NULL)
    {
      if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS)
      {
        std::memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
        break;
      }
      cmsg = CMSG_NXTHDR(&msg, cmsg);
    }

    if (fd != -1)
    {
      ex_out << "File descriptor received is: " << fd << "\n";
      FILE* f(::fdopen(fd, "w+"));
      if (f)
      {
        ::fprintf(f, "stream_client writing to received fd #%d\n", fd);
        ::fclose(f);
      }
      else
        ::close(fd);
    }
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

TEST_CASE("example: cpp11/local/fd_passing_stream_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_local_path("stream.sock"); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_local_fd_passing_stream_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Enter path to write to: File descriptor received is: 17
)ex"));
#endif
}
}  // namespace ex_cpp11_local_fd_passing_stream_client
#endif
