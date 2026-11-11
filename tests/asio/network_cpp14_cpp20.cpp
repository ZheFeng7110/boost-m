// mcpplibs.mwrap.boost.asio — the official C++14 echo, C++17 coroutines_ts and C++20 coroutines/channels examples that need a network
// peer, run over the loopback interface in one process. A server example
// runs in a background thread on a free port (ex_serve_async) and a harness
// client of the loopback harness below talks to it; the case output is what
// that client received. A client example runs against the ported server
// example it belongs with, started the same way. The expected output is what
// the same code printed when built from upstream/boost headers alone. Fixed
// ports (13, 55555, 54321) are replaced by the free one, "daytime"/"http"
// service names by its number, and output that carries the time of day is
// not compared.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cstdlib>
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

// libs/asio/example/cpp14/echo/async_tcp_echo_server.cpp
namespace ex_cpp14_echo_async_tcp_echo_server {
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
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
      socket_(io_context)
  {
    do_accept();
  }

private:
  void do_accept()
  {
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec)
        {
          if (!ec)
          {
            std::make_shared<session>(std::move(socket_))->start();
          }

          do_accept();
        });
  }

  tcp::acceptor acceptor_;
  tcp::socket socket_;
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

TEST_CASE("example: cpp14/echo/async_tcp_echo_server") {
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
}  // namespace ex_cpp14_echo_async_tcp_echo_server

// libs/asio/example/cpp14/echo/blocking_tcp_echo_server.cpp
namespace ex_cpp14_echo_blocking_tcp_echo_server {
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
      if (error == boost::asio::stream_errc::eof)
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
    tcp::socket sock(io_context);
    a.accept(sock);
    std::thread(session, std::move(sock)).detach();
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

TEST_CASE("example: cpp14/echo/blocking_tcp_echo_server") {
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
}  // namespace ex_cpp14_echo_blocking_tcp_echo_server

// libs/asio/example/cpp14/echo/async_udp_echo_server.cpp
namespace ex_cpp14_echo_async_udp_echo_server {
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

TEST_CASE("example: cpp14/echo/async_udp_echo_server") {
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
}  // namespace ex_cpp14_echo_async_udp_echo_server

// libs/asio/example/cpp14/echo/blocking_udp_echo_server.cpp
namespace ex_cpp14_echo_blocking_udp_echo_server {
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

TEST_CASE("example: cpp14/echo/blocking_udp_echo_server") {
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
}  // namespace ex_cpp14_echo_blocking_udp_echo_server

// libs/asio/example/cpp14/echo/blocking_tcp_echo_client.cpp
namespace ex_cpp14_echo_blocking_tcp_echo_client {
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

TEST_CASE("example: cpp14/echo/blocking_tcp_echo_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello world
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp14_echo_blocking_tcp_echo_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
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
}  // namespace ex_cpp14_echo_blocking_tcp_echo_client

// libs/asio/example/cpp14/echo/blocking_udp_echo_client.cpp
namespace ex_cpp14_echo_blocking_udp_echo_client {
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
    udp::endpoint endpoint =
      *resolver.resolve(udp::v4(), argv[1], argv[2]).begin();

    ex_out << "Enter message: ";
    char request[max_length];
    ex_in.getline(request, max_length);
    size_t request_length = std::strlen(request);
    s.send_to(boost::asio::buffer(request, request_length), endpoint);

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

TEST_CASE("example: cpp14/echo/blocking_udp_echo_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_in.clear();
    ex_in.str(R"ex(hello world
)ex");  // the program's stdin
    ex_args = {"example"};
    ex_port = ex_free_udp_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp14_echo_blocking_udp_echo_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
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
}  // namespace ex_cpp14_echo_blocking_udp_echo_client

// libs/asio/example/cpp17/coroutines_ts/echo_server.cpp
namespace ex_cpp17_coroutines_ts_echo_server {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

awaitable<void> echo(tcp::socket socket)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
      co_await async_write(socket, boost::asio::buffer(data, n), use_awaitable);
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp17/coroutines_ts/echo_server") {
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
}  // namespace ex_cpp17_coroutines_ts_echo_server

// libs/asio/example/cpp17/coroutines_ts/echo_server_with_as_single_default.cpp
namespace ex_cpp17_coroutines_ts_echo_server_with_as_single_default {
using boost::asio::experimental::as_single_t;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable_t;
using default_token = as_single_t<use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
  char data[1024];
  for (;;)
  {
    auto [e1, nread] = co_await socket.async_read_some(boost::asio::buffer(data));
    if (nread == 0) break;
    auto [e2, nwritten] = co_await async_write(socket, boost::asio::buffer(data, nread));
    if (nwritten != nread) break;
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp_acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    if (auto [e, socket] = co_await acceptor.async_accept(); socket.is_open())
      co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp17/coroutines_ts/echo_server_with_as_single_default") {
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
}  // namespace ex_cpp17_coroutines_ts_echo_server_with_as_single_default

// libs/asio/example/cpp17/coroutines_ts/echo_server_with_as_tuple_default.cpp
namespace ex_cpp17_coroutines_ts_echo_server_with_as_tuple_default {
using boost::asio::as_tuple_t;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable_t;
using default_token = as_tuple_t<use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
  char data[1024];
  for (;;)
  {
    auto [e1, nread] = co_await socket.async_read_some(boost::asio::buffer(data));
    if (nread == 0) break;
    auto [e2, nwritten] = co_await async_write(socket, boost::asio::buffer(data, nread));
    if (nwritten != nread) break;
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp_acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    if (auto [e, socket] = co_await acceptor.async_accept(); socket.is_open())
      co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp17/coroutines_ts/echo_server_with_as_tuple_default") {
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
}  // namespace ex_cpp17_coroutines_ts_echo_server_with_as_tuple_default

// libs/asio/example/cpp17/coroutines_ts/echo_server_with_default.cpp
namespace ex_cpp17_coroutines_ts_echo_server_with_default {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable_t;
using tcp_acceptor = use_awaitable_t<>::as_default_on_t<tcp::acceptor>;
using tcp_socket = use_awaitable_t<>::as_default_on_t<tcp::socket>;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data));
      co_await async_write(socket, boost::asio::buffer(data, n));
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp_acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    auto socket = co_await acceptor.async_accept();
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp17/coroutines_ts/echo_server_with_default") {
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
}  // namespace ex_cpp17_coroutines_ts_echo_server_with_default

// libs/asio/example/cpp17/coroutines_ts/refactored_echo_server.cpp
namespace ex_cpp17_coroutines_ts_refactored_echo_server {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo_once(tcp::socket& socket)
{
  char data[128];
  std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
  co_await async_write(socket, boost::asio::buffer(data, n), use_awaitable);
}

awaitable<void> echo(tcp::socket socket)
{
  try
  {
    for (;;)
    {
      // The asynchronous operations to echo a single chunk of data have been
      // refactored into a separate function. When this function is called, the
      // operations are still performed in the context of the current
      // coroutine, and the behaviour is functionally equivalent.
      co_await echo_once(socket);
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp17/coroutines_ts/refactored_echo_server") {
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
}  // namespace ex_cpp17_coroutines_ts_refactored_echo_server

// libs/asio/example/cpp17/coroutines_ts/chat_server.cpp
namespace ex_cpp17_coroutines_ts_chat_server {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const std::string& msg) = 0;
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

  void deliver(const std::string& msg)
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
  std::deque<std::string> recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      timer_(socket_.get_executor()),
      room_(room)
  {
    timer_.expires_at(std::chrono::steady_clock::time_point::max());
  }

  void start()
  {
    room_.join(shared_from_this());

    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->reader(); },
        detached);

    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->writer(); },
        detached);
  }

  void deliver(const std::string& msg)
  {
    write_msgs_.push_back(msg);
    timer_.cancel_one();
  }

private:
  awaitable<void> reader()
  {
    try
    {
      for (std::string read_msg;;)
      {
        std::size_t n = co_await boost::asio::async_read_until(socket_,
            boost::asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

        room_.deliver(read_msg.substr(0, n));
        read_msg.erase(0, n);
      }
    }
    catch (std::exception&)
    {
      stop();
    }
  }

  awaitable<void> writer()
  {
    try
    {
      while (socket_.is_open())
      {
        if (write_msgs_.empty())
        {
          boost::system::error_code ec;
          co_await timer_.async_wait(redirect_error(use_awaitable, ec));
        }
        else
        {
          co_await boost::asio::async_write(socket_,
              boost::asio::buffer(write_msgs_.front()), use_awaitable);
          write_msgs_.pop_front();
        }
      }
    }
    catch (std::exception&)
    {
      stop();
    }
  }

  void stop()
  {
    room_.leave(shared_from_this());
    socket_.close();
    timer_.cancel();
  }

  tcp::socket socket_;
  boost::asio::steady_timer timer_;
  chat_room& room_;
  std::deque<std::string> write_msgs_;
};

//----------------------------------------------------------------------

awaitable<void> listener(tcp::acceptor acceptor)
{
  chat_room room;

  for (;;)
  {
    std::make_shared<chat_session>(
        co_await acceptor.async_accept(use_awaitable),
        room
      )->start();
  }
}

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

    boost::asio::io_context io_context(1);

    for (int i = 1; i < argc; ++i)
    {
      unsigned short port = std::atoi(argv[i]);
      co_spawn(io_context,
          listener(tcp::acceptor(io_context, {tcp::v4(), port})),
          detached);
    }

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp17/coroutines_ts/chat_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello\n", 6)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello
)ex"));
}
}  // namespace ex_cpp17_coroutines_ts_chat_server

// libs/asio/example/cpp20/coroutines/echo_server.cpp
namespace ex_cpp20_coroutines_echo_server {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

#if defined(BOOST_ASIO_ENABLE_HANDLER_TRACKING)
# define use_awaitable \
  boost::asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
#endif

awaitable<void> echo(tcp::socket socket)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
      co_await async_write(socket, boost::asio::buffer(data, n), use_awaitable);
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/echo_server") {
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
}  // namespace ex_cpp20_coroutines_echo_server

// libs/asio/example/cpp20/coroutines/echo_server_with_as_single_default.cpp
namespace ex_cpp20_coroutines_echo_server_with_as_single_default {
using boost::asio::experimental::as_single_t;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable_t;
using default_token = as_single_t<use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
  char data[1024];
  for (;;)
  {
    auto [e1, nread] = co_await socket.async_read_some(boost::asio::buffer(data));
    if (nread == 0) break;
    auto [e2, nwritten] = co_await async_write(socket, boost::asio::buffer(data, nread));
    if (nwritten != nread) break;
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp_acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    if (auto [e, socket] = co_await acceptor.async_accept(); socket.is_open())
      co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/echo_server_with_as_single_default") {
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
}  // namespace ex_cpp20_coroutines_echo_server_with_as_single_default

// libs/asio/example/cpp20/coroutines/echo_server_with_as_tuple_default.cpp
namespace ex_cpp20_coroutines_echo_server_with_as_tuple_default {
using boost::asio::as_tuple_t;
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable_t;
using default_token = as_tuple_t<use_awaitable_t<>>;
using tcp_acceptor = default_token::as_default_on_t<tcp::acceptor>;
using tcp_socket = default_token::as_default_on_t<tcp::socket>;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
  char data[1024];
  for (;;)
  {
    auto [e1, nread] = co_await socket.async_read_some(boost::asio::buffer(data));
    if (nread == 0) break;
    auto [e2, nwritten] = co_await async_write(socket, boost::asio::buffer(data, nread));
    if (nwritten != nread) break;
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp_acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    if (auto [e, socket] = co_await acceptor.async_accept(); socket.is_open())
      co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/echo_server_with_as_tuple_default") {
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
}  // namespace ex_cpp20_coroutines_echo_server_with_as_tuple_default

// libs/asio/example/cpp20/coroutines/echo_server_with_default.cpp
namespace ex_cpp20_coroutines_echo_server_with_default {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable_t;
using tcp_acceptor = use_awaitable_t<>::as_default_on_t<tcp::acceptor>;
using tcp_socket = use_awaitable_t<>::as_default_on_t<tcp::socket>;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp_socket socket)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data));
      co_await async_write(socket, boost::asio::buffer(data, n));
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp_acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    auto socket = co_await acceptor.async_accept();
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/echo_server_with_default") {
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
}  // namespace ex_cpp20_coroutines_echo_server_with_default

// libs/asio/example/cpp20/coroutines/refactored_echo_server.cpp
namespace ex_cpp20_coroutines_refactored_echo_server {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::use_awaitable;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo_once(tcp::socket& socket)
{
  char data[128];
  std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), use_awaitable);
  co_await async_write(socket, boost::asio::buffer(data, n), use_awaitable);
}

awaitable<void> echo(tcp::socket socket)
{
  try
  {
    for (;;)
    {
      // The asynchronous operations to echo a single chunk of data have been
      // refactored into a separate function. When this function is called, the
      // operations are still performed in the context of the current
      // coroutine, and the behaviour is functionally equivalent.
      co_await echo_once(socket);
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/refactored_echo_server") {
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
}  // namespace ex_cpp20_coroutines_refactored_echo_server

// libs/asio/example/cpp20/coroutines/echo_server_with_deferred.cpp
namespace ex_cpp20_coroutines_echo_server_with_deferred {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::deferred;
using boost::asio::detached;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp::socket socket)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data), deferred);
      co_await async_write(socket, boost::asio::buffer(data, n), deferred);
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept(deferred);
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/echo_server_with_deferred") {
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
}  // namespace ex_cpp20_coroutines_echo_server_with_deferred

// libs/asio/example/cpp20/coroutines/echo_server_with_deferred_default.cpp
namespace ex_cpp20_coroutines_echo_server_with_deferred_default {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
namespace this_coro = boost::asio::this_coro;

awaitable<void> echo(tcp::socket socket)
{
  try
  {
    char data[1024];
    for (;;)
    {
      std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data));
      co_await async_write(socket, boost::asio::buffer(data, n));
    }
  }
  catch (std::exception& e)
  {
    ex_printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener()
{
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), ex_port_number()});
  for (;;)
  {
    tcp::socket socket = co_await acceptor.async_accept();
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int example_main()
{
  try
  {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_printf("Exception: %s\n", e.what());
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/echo_server_with_deferred_default") {
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
}  // namespace ex_cpp20_coroutines_echo_server_with_deferred_default

// libs/asio/example/cpp20/coroutines/chat_server.cpp
namespace ex_cpp20_coroutines_chat_server {
using boost::asio::ip::tcp;
using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::redirect_error;
using boost::asio::use_awaitable;

//----------------------------------------------------------------------

class chat_participant
{
public:
  virtual ~chat_participant() {}
  virtual void deliver(const std::string& msg) = 0;
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

  void deliver(const std::string& msg)
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
  std::deque<std::string> recent_msgs_;
};

//----------------------------------------------------------------------

class chat_session
  : public chat_participant,
    public std::enable_shared_from_this<chat_session>
{
public:
  chat_session(tcp::socket socket, chat_room& room)
    : socket_(std::move(socket)),
      timer_(socket_.get_executor()),
      room_(room)
  {
    timer_.expires_at(std::chrono::steady_clock::time_point::max());
  }

  void start()
  {
    room_.join(shared_from_this());

    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->reader(); },
        detached);

    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->writer(); },
        detached);
  }

  void deliver(const std::string& msg)
  {
    write_msgs_.push_back(msg);
    timer_.cancel_one();
  }

private:
  awaitable<void> reader()
  {
    try
    {
      for (std::string read_msg;;)
      {
        std::size_t n = co_await boost::asio::async_read_until(socket_,
            boost::asio::dynamic_buffer(read_msg, 1024), "\n", use_awaitable);

        room_.deliver(read_msg.substr(0, n));
        read_msg.erase(0, n);
      }
    }
    catch (std::exception&)
    {
      stop();
    }
  }

  awaitable<void> writer()
  {
    try
    {
      while (socket_.is_open())
      {
        if (write_msgs_.empty())
        {
          boost::system::error_code ec;
          co_await timer_.async_wait(redirect_error(use_awaitable, ec));
        }
        else
        {
          co_await boost::asio::async_write(socket_,
              boost::asio::buffer(write_msgs_.front()), use_awaitable);
          write_msgs_.pop_front();
        }
      }
    }
    catch (std::exception&)
    {
      stop();
    }
  }

  void stop()
  {
    room_.leave(shared_from_this());
    socket_.close();
    timer_.cancel();
  }

  tcp::socket socket_;
  boost::asio::steady_timer timer_;
  chat_room& room_;
  std::deque<std::string> write_msgs_;
};

//----------------------------------------------------------------------

awaitable<void> listener(tcp::acceptor acceptor)
{
  chat_room room;

  for (;;)
  {
    std::make_shared<chat_session>(
        co_await acceptor.async_accept(use_awaitable),
        room
      )->start();
  }
}

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

    boost::asio::io_context io_context(1);

    for (int i = 1; i < argc; ++i)
    {
      unsigned short port = std::atoi(argv[i]);
      co_spawn(io_context,
          listener(tcp::acceptor(io_context, {tcp::v4(), port})),
          detached);
    }

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto){ io_context.stop(); });

    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/chat_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello\n", 6)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello
)ex"));
}
}  // namespace ex_cpp20_coroutines_chat_server

// libs/asio/example/cpp20/coroutines/timeout_cancel_after.cpp
namespace ex_cpp20_coroutines_timeout_cancel_after {
using namespace boost::asio;
using ip::tcp;
using namespace std::literals::chrono_literals;

awaitable<void> echo(tcp::socket sock)
{
  char data[4196];
  for (;;)
  {
    auto n = co_await sock.async_read_some(buffer(data), cancel_after(10s));
    co_await async_write(sock, buffer(data, n), cancel_after(10s));
  }
}

awaitable<void> listen(tcp::acceptor& acceptor)
{
  for (;;)
  {
    co_spawn(
        acceptor.get_executor(),
        echo(co_await acceptor.async_accept()),
        detached);
  }
}

int example_main()
{
  io_context ctx;
  tcp::acceptor acceptor(ctx, {tcp::v4(), ex_port_number()});
  co_spawn(ctx, listen(acceptor), detached);
  ctx.run();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/timeout_cancel_after") {
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
}  // namespace ex_cpp20_coroutines_timeout_cancel_after

// libs/asio/example/cpp20/coroutines/timeout_watchdog.cpp
namespace ex_cpp20_coroutines_timeout_watchdog {
using namespace boost::asio;
using namespace boost::asio::experimental::awaitable_operators;
using time_point = std::chrono::steady_clock::time_point;
using ip::tcp;

awaitable<void> echo(tcp::socket& sock, time_point& deadline)
{
  char data[4196];
  for (;;)
  {
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    auto n = co_await sock.async_read_some(buffer(data), use_awaitable);
    co_await async_write(sock, buffer(data, n), use_awaitable);
  }
}

awaitable<void> watchdog(time_point& deadline)
{
  steady_timer timer(co_await this_coro::executor);
  auto now = std::chrono::steady_clock::now();
  while (deadline > now)
  {
    timer.expires_at(deadline);
    co_await timer.async_wait(use_awaitable);
    now = std::chrono::steady_clock::now();
  }
  throw boost::system::system_error(std::make_error_code(std::errc::timed_out));
}

awaitable<void> handle_connection(tcp::socket sock)
{
  time_point deadline{};
  co_await (echo(sock, deadline) && watchdog(deadline));
}

awaitable<void> listen(tcp::acceptor& acceptor)
{
  for (;;)
  {
    co_spawn(
        acceptor.get_executor(),
        handle_connection(co_await acceptor.async_accept(use_awaitable)),
        detached);
  }
}

int example_main()
{
  io_context ctx;
  tcp::acceptor acceptor(ctx, {tcp::v4(), ex_port_number()});
  co_spawn(ctx, listen(acceptor), detached);
  ctx.run();
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/coroutines/timeout_watchdog") {
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
}  // namespace ex_cpp20_coroutines_timeout_watchdog

// libs/asio/example/cpp20/channels/mutual_exclusion_1.cpp
namespace ex_cpp20_channels_mutual_exclusion_1 {
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::dynamic_buffer;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::experimental::channel;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::asio::steady_timer;
using namespace boost::asio::buffer_literals;
using namespace std::literals::chrono_literals;

// This class implements a simple line-based protocol:
//
// * For event line that is received from the client, the session sends a
//   message header followed by the content of the line as the message body.
//
// * The session generates heartbeat messages once a second.
//
// This protocol is implemented using two actors, handle_messages() and
// send_heartbeats(), each written as a coroutine.
class line_based_echo_session :
  public std::enable_shared_from_this<line_based_echo_session>
{
  // The socket used to read from and write to the client. This socket is a
  // data member as it is shared between the two actors.
  tcp::socket socket_;

  // As both of the actors will write to the socket, we need a lock to prevent
  // these writes from overlapping. To achieve this, we use a channel with a
  // buffer size of one. The lock is claimed by sending a message to the
  // channel, and then released by receiving this message back again. If the
  // lock is not held then the channel's buffer is empty, and the send will
  // complete without delay. Otherwise, if the lock is held by the other actor,
  // then the send operation will not complete until the lock is released.
  channel<void()> write_lock_{socket_.get_executor(), 1};

public:
  line_based_echo_session(tcp::socket socket)
    : socket_{std::move(socket)}
  {
    socket_.set_option(tcp::no_delay(true));
  }

  void start()
  {
    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->handle_messages(); },
        detached);

    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->send_heartbeats(); },
        detached);
  }

private:
  void stop()
  {
    socket_.close();
    write_lock_.cancel();
  }

  awaitable<void> handle_messages()
  {
    try
    {
      constexpr std::size_t max_line_length = 1024;
      std::string data;
      for (;;)
      {
        // Read an entire line from the client.
        std::size_t length = co_await async_read_until(socket_,
            dynamic_buffer(data, max_line_length), '\n');

        // Claim the write lock by sending a message to the channel. Since the
        // channel signature is void(), there are no arguments to send in the
        // message itself.
        co_await write_lock_.async_send();

        // Respond to the client with a message, echoing the line they sent.
        co_await async_write(socket_, "<line>"_buf);
        co_await async_write(socket_, dynamic_buffer(data, length));

        // Release the lock by receiving the message back again.
        write_lock_.try_receive([](auto...){});
      }
    }
    catch (const std::exception&)
    {
      stop();
    }
  }

  awaitable<void> send_heartbeats()
  {
    steady_timer timer{socket_.get_executor()};
    try
    {
      for (;;)
      {
        // Wait one second before trying to send the next heartbeat.
        timer.expires_after(1s);
        co_await timer.async_wait();

        // Claim the write lock by sending a message to the channel. Since the
        // channel signature is void(), there are no arguments to send in the
        // message itself.
        co_await write_lock_.async_send();

        // Send a heartbeat to the client. As the content of the heartbeat
        // message never varies, a buffer literal can be used to specify the
        // bytes of the message. The memory associated with a buffer literal is
        // valid for the lifetime of the program, which mean that the buffer
        // can be safely passed as-is to the asynchronous operation.
        co_await async_write(socket_, "<heartbeat>\n"_buf);

        // Release the lock by receiving the message back again.
        write_lock_.try_receive([](auto...){});
      }
    }
    catch (const std::exception&)
    {
      stop();
    }
  }

};

awaitable<void> listen(tcp::acceptor& acceptor)
{
  for (;;)
  {
    auto [e, socket] = co_await acceptor.async_accept(as_tuple);
    if (!e)
    {
      std::make_shared<line_based_echo_session>(std::move(socket))->start();
    }
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: mutual_exclusion_1";
      ex_out << " <listen_address> <listen_port>\n";
      return 1;
    }

    io_context ctx;

    auto listen_endpoint =
      *tcp::resolver(ctx).resolve(argv[1], argv[2],
          tcp::resolver::passive).begin();

    tcp::acceptor acceptor(ctx, listen_endpoint);
    co_spawn(ctx, listen(acceptor), detached);
    ctx.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/channels/mutual_exclusion_1") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hi\n", 9)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: <line>hi
)ex"));
}
}  // namespace ex_cpp20_channels_mutual_exclusion_1

// libs/asio/example/cpp20/channels/mutual_exclusion_2.cpp
namespace ex_cpp20_channels_mutual_exclusion_2 {
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::dynamic_buffer;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::experimental::channel;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::asio::steady_timer;
using namespace boost::asio::buffer_literals;
using namespace std::literals::chrono_literals;

// This class implements a simple line-based protocol:
//
// * For event line that is received from the client, the session sends a
//   message header followed by the content of the line as the message body.
//
// * The session generates heartbeat messages once a second.
//
// This protocol is implemented using two actors, handle_messages() and
// send_heartbeats(), each written as a coroutine.
class line_based_echo_session :
  public std::enable_shared_from_this<line_based_echo_session>
{
  // The socket used to read from and write to the client. This socket is a
  // data member as it is shared between the two actors.
  tcp::socket socket_;

  // As both of the actors will write to the socket, we need a lock to prevent
  // these writes from overlapping. To achieve this, we use a channel with a
  // buffer size of one. The lock is claimed by sending a message to the
  // channel, and then released by receiving this message back again. If the
  // lock is not held then the channel's buffer is empty, and the send will
  // complete without delay. Otherwise, if the lock is held by the other actor,
  // then the send operation will not complete until the lock is released.
  channel<void()> write_lock_{socket_.get_executor(), 1};

public:
  line_based_echo_session(tcp::socket socket)
    : socket_{std::move(socket)}
  {
    socket_.set_option(tcp::no_delay(true));
  }

  void start()
  {
    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->handle_messages(); },
        detached);

    co_spawn(socket_.get_executor(),
        [self = shared_from_this()]{ return self->send_heartbeats(); },
        detached);
  }

private:
  void stop()
  {
    socket_.close();
    write_lock_.cancel();
  }

  awaitable<void> handle_messages()
  {
    try
    {
      constexpr std::size_t max_line_length = 1024;
      std::string data;
      for (;;)
      {
        // Read an entire line from the client.
        std::size_t length = co_await async_read_until(socket_,
            dynamic_buffer(data, max_line_length), '\n');

        // Claim the write lock by sending a message to the channel. Since the
        // channel signature is void(), there are no arguments to send in the
        // message itself. In this example we optimise for the common case,
        // where the lock is not held by the other actor, by first trying a
        // non-blocking send.
        if (!write_lock_.try_send())
        {
          co_await write_lock_.async_send();
        }

        // Respond to the client with a message, echoing the line they sent.
        co_await async_write(socket_, "<line>"_buf);
        co_await async_write(socket_, dynamic_buffer(data, length));

        // Release the lock by receiving the message back again.
        write_lock_.try_receive([](auto...){});
      }
    }
    catch (const std::exception&)
    {
      stop();
    }
  }

  awaitable<void> send_heartbeats()
  {
    steady_timer timer{socket_.get_executor()};
    try
    {
      for (;;)
      {
        // Wait one second before trying to send the next heartbeat.
        timer.expires_after(1s);
        co_await timer.async_wait();

        // Claim the write lock by sending a message to the channel. Since the
        // channel signature is void(), there are no arguments to send in the
        // message itself. In this example we optimise for the common case,
        // where the lock is not held by the other actor, by first trying a
        // non-blocking send.
        if (!write_lock_.try_send())
        {
          co_await write_lock_.async_send();
        }

        // Send a heartbeat to the client. As the content of the heartbeat
        // message never varies, a buffer literal can be used to specify the
        // bytes of the message. The memory associated with a buffer literal is
        // valid for the lifetime of the program, which mean that the buffer
        // can be safely passed as-is to the asynchronous operation.
        co_await async_write(socket_, "<heartbeat>\n"_buf);

        // Release the lock by receiving the message back again.
        write_lock_.try_receive([](auto...){});
      }
    }
    catch (const std::exception&)
    {
      stop();
    }
  }

};

awaitable<void> listen(tcp::acceptor& acceptor)
{
  for (;;)
  {
    auto [e, socket] = co_await acceptor.async_accept(as_tuple);
    if (!e)
    {
      std::make_shared<line_based_echo_session>(std::move(socket))->start();
    }
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: mutual_exclusion_1";
      ex_out << " <listen_address> <listen_port>\n";
      return 1;
    }

    io_context ctx;

    auto listen_endpoint =
      *tcp::resolver(ctx).resolve(argv[1], argv[2],
          tcp::resolver::passive).begin();

    tcp::acceptor acceptor(ctx, listen_endpoint);
    co_spawn(ctx, listen(acceptor), detached);
    ctx.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/channels/mutual_exclusion_2") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hi\n", 9)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: <line>hi
)ex"));
}
}  // namespace ex_cpp20_channels_mutual_exclusion_2

// libs/asio/example/cpp20/channels/throttling_proxy.cpp
namespace ex_cpp20_channels_throttling_proxy {
using boost::asio::as_tuple;
using boost::asio::awaitable;
using boost::asio::buffer;
using boost::asio::co_spawn;
using boost::asio::detached;
using boost::asio::experimental::channel;
using boost::asio::io_context;
using boost::asio::ip::tcp;
using boost::asio::steady_timer;
namespace this_coro = boost::asio::this_coro;
using namespace boost::asio::experimental::awaitable_operators;
using namespace std::literals::chrono_literals;

using token_channel = channel<void(boost::system::error_code, std::size_t)>;

awaitable<void> produce_tokens(std::size_t bytes_per_token,
    steady_timer::duration token_interval, token_channel& tokens)
{
  steady_timer timer(co_await this_coro::executor);
  for (;;)
  {
    co_await tokens.async_send(boost::system::error_code{}, bytes_per_token);

    timer.expires_after(token_interval);
    co_await timer.async_wait();
  }
}

awaitable<void> transfer(tcp::socket& from,
    tcp::socket& to, token_channel& tokens)
{
  std::array<unsigned char, 4096> data;
  for (;;)
  {
    std::size_t bytes_available = co_await tokens.async_receive();
    while (bytes_available > 0)
    {
      std::size_t n = co_await from.async_read_some(
          buffer(data, bytes_available));

      co_await async_write(to, buffer(data, n));

      bytes_available -= n;
    }
  }
}

awaitable<void> proxy(tcp::socket client, tcp::endpoint target)
{
  constexpr std::size_t number_of_tokens = 100;
  constexpr size_t bytes_per_token = 20 * 1024;
  constexpr steady_timer::duration token_interval = 100ms;

  auto ex = client.get_executor();
  tcp::socket server(ex);
  token_channel client_tokens(ex, number_of_tokens);
  token_channel server_tokens(ex, number_of_tokens);

  co_await server.async_connect(target);
  co_await (
      produce_tokens(bytes_per_token, token_interval, client_tokens) &&
      transfer(client, server, client_tokens) &&
      produce_tokens(bytes_per_token, token_interval, server_tokens) &&
      transfer(server, client, server_tokens)
    );
}

awaitable<void> listen(tcp::acceptor& acceptor, tcp::endpoint target)
{
  for (;;)
  {
    auto [e, client] = co_await acceptor.async_accept(as_tuple);
    if (!e)
    {
      auto ex = client.get_executor();
      co_spawn(ex, proxy(std::move(client), target), detached);
    }
    else
    {
      ex_out << "Accept failed: " << e.message() << "\n";
      steady_timer timer(co_await this_coro::executor);
      timer.expires_after(100ms);
      co_await timer.async_wait();
    }
  }
}

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 5)
    {
      ex_out << "Usage: throttling_proxy";
      ex_out << " <listen_address> <listen_port>";
      ex_out << " <target_address> <target_port>\n";
      return 1;
    }

    io_context ctx;

    auto listen_endpoint =
      *tcp::resolver(ctx).resolve(argv[1], argv[2],
          tcp::resolver::passive).begin();

    auto target_endpoint =
      *tcp::resolver(ctx).resolve(argv[3], argv[4]).begin();

    tcp::acceptor acceptor(ctx, listen_endpoint);
    co_spawn(ctx, listen(acceptor, target_endpoint), detached);
    ctx.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp20/channels/throttling_proxy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_up = ex_harness_echo();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_up);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello", 5)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello)ex"));
}
}  // namespace ex_cpp20_channels_throttling_proxy
