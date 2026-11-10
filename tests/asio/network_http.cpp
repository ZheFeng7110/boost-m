// mcpplibs.mwrap.boost.asio — the official HTTP (libs/asio/example/cpp11/http/, iostreams/http_client) examples that need a network
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
#if defined(_WIN32)
// Windows: each case's name goes to stderr (unbuffered) before it runs, so a
// case that ends the process (a fast-fail such as heap corruption leaves no
// doctest report) can be told from the log.
struct ex_case_names : doctest::IReporter {
    explicit ex_case_names(const doctest::ContextOptions&) {}
    void report_query(const doctest::QueryData&) override {}
    void test_run_start() override {}
    void test_run_end(const doctest::TestRunStats&) override {}
    void test_case_start(const doctest::TestCaseData& d) override { std::fprintf(stderr, "case: %s\n", d.m_name); }
    void test_case_reenter(const doctest::TestCaseData&) override {}
    void test_case_end(const doctest::CurrentTestCaseStats&) override {}
    void test_case_exception(const doctest::TestCaseException&) override {}
    void subcase_start(const doctest::SubcaseSignature&) override {}
    void subcase_end() override {}
    void log_assert(const doctest::AssertData&) override {}
    void log_message(const doctest::MessageData&) override {}
    void test_case_skipped(const doctest::TestCaseData&) override {}
};
DOCTEST_REGISTER_LISTENER("ex_case_names", 1, ex_case_names);
#endif

// libs/asio/example/cpp11/http/server/main.cpp
namespace ex_cpp11_http_server_main {
#undef HTTP_SERVER_HPP
//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

// ---- inlined connection.hpp ----
#undef HTTP_CONNECTION_HPP
//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_HPP
#define HTTP_CONNECTION_HPP

// ---- inlined reply.hpp ----
#undef HTTP_REPLY_HPP
//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REPLY_HPP
#define HTTP_REPLY_HPP

// ---- inlined header.hpp ----
#undef HTTP_HEADER_HPP
//
// header.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_HEADER_HPP
#define HTTP_HEADER_HPP


namespace http {
namespace server {

struct header
{
  std::string name;
  std::string value;
};

} // namespace server
} // namespace http

#endif // HTTP_HEADER_HPP
// ---- end header.hpp ----

namespace http {
namespace server {

/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers();

  /// Get a stock reply.
  static reply stock_reply(status_type status);
};

} // namespace server
} // namespace http

#endif // HTTP_REPLY_HPP
// ---- end reply.hpp ----
// ---- inlined request.hpp ----
#undef HTTP_REQUEST_HPP
//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP


namespace http {
namespace server {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
// ---- end request.hpp ----
// ---- inlined request_handler.hpp ----
#undef HTTP_REQUEST_HANDLER_HPP
//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP


namespace http {
namespace server {

struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler
{
public:
  request_handler(const request_handler&) = delete;
  request_handler& operator=(const request_handler&) = delete;

  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP
// ---- end request_handler.hpp ----
// ---- inlined request_parser.hpp ----
#undef HTTP_REQUEST_PARSER_HPP
//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_PARSER_HPP
#define HTTP_REQUEST_PARSER_HPP


namespace http {
namespace server {

struct request;

/// Parser for incoming requests.
class request_parser
{
public:
  /// Construct ready to parse the request method.
  request_parser();

  /// Reset to initial parser state.
  void reset();

  /// Result of parse.
  enum result_type { good, bad, indeterminate };

  /// Parse some data. The enum return value is good when a complete request has
  /// been parsed, bad if the data is invalid, indeterminate when more data is
  /// required. The InputIterator return value indicates how much of the input
  /// has been consumed.
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(request& req,
      InputIterator begin, InputIterator end)
  {
    while (begin != end)
    {
      result_type result = consume(req, *begin++);
      if (result == good || result == bad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(indeterminate, begin);
  }

private:
  /// Handle the next character of input.
  result_type consume(request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool is_char(int c);

  /// Check if a byte is an HTTP control character.
  static bool is_ctl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool is_tspecial(int c);

  /// Check if a byte is a digit.
  static bool is_digit(int c);

  /// The current state of the parser.
  enum state
  {
    method_start,
    method,
    uri,
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3
  } state_;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_PARSER_HPP
// ---- end request_parser.hpp ----

namespace http {
namespace server {

class connection_manager;

/// Represents a single connection from a client.
class connection
  : public std::enable_shared_from_this<connection>
{
public:
  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  /// Construct a connection with the given socket.
  explicit connection(boost::asio::ip::tcp::socket socket,
      connection_manager& manager, request_handler& handler);

  /// Start the first asynchronous operation for the connection.
  void start();

  /// Stop all asynchronous operations associated with the connection.
  void stop();

private:
  /// Perform an asynchronous read operation.
  void do_read();

  /// Perform an asynchronous write operation.
  void do_write();

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The manager for this connection.
  connection_manager& connection_manager_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// Buffer for incoming data.
  std::array<char, 8192> buffer_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_HPP
// ---- end connection.hpp ----
// ---- inlined connection_manager.hpp ----
#undef HTTP_CONNECTION_MANAGER_HPP
//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_MANAGER_HPP
#define HTTP_CONNECTION_MANAGER_HPP


namespace http {
namespace server {

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class connection_manager
{
public:
  connection_manager(const connection_manager&) = delete;
  connection_manager& operator=(const connection_manager&) = delete;

  /// Construct a connection manager.
  connection_manager();

  /// Add the specified connection to the manager and start it.
  void start(connection_ptr c);

  /// Stop the specified connection.
  void stop(connection_ptr c);

  /// Stop all connections.
  void stop_all();

private:
  /// The managed connections.
  std::set<connection_ptr> connections_;
};

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_MANAGER_HPP
// ---- end connection_manager.hpp ----

namespace http {
namespace server {

/// The top-level class of the HTTP server.
class server
{
public:
  server(const server&) = delete;
  server& operator=(const server&) = delete;

  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const std::string& port,
      const std::string& doc_root);

  /// Run the server's io_context loop.
  void run();

private:
  /// Perform an asynchronous accept operation.
  void do_accept();

  /// Wait for a request to stop the server.
  void do_await_stop();

  /// The io_context used to perform asynchronous operations.
  boost::asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  boost::asio::signal_set signals_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The connection manager which owns all live connections.
  connection_manager connection_manager_;

  /// The handler for all incoming requests.
  request_handler request_handler_;
};

} // namespace server
} // namespace http

#endif // HTTP_SERVER_HPP
// ---- end server.hpp ----

int example_main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 4)
    {
      ex_out << "Usage: http_server <address> <port> <doc_root>\n";
      ex_out << "  For IPv4, try:\n";
      ex_out << "    receiver 0.0.0.0 80 .\n";
      ex_out << "  For IPv6, try:\n";
      ex_out << "    receiver 0::0 80 .\n";
      return 1;
    }

    // Initialise the server.
    http::server::server s(argv[1], argv[2], argv[3]);

    // Run the server until stopped.
    s.run();
  }
  catch (std::exception& e)
  {
    ex_out << "exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}
// ---- merged connection.cpp (the same program) ----
//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server {

connection::connection(boost::asio::ip::tcp::socket socket,
    connection_manager& manager, request_handler& handler)
  : socket_(std::move(socket)),
    connection_manager_(manager),
    request_handler_(handler)
{
}

void connection::start()
{
  do_read();
}

void connection::stop()
{
  socket_.close();
}

void connection::do_read()
{
  auto self(shared_from_this());
  socket_.async_read_some(boost::asio::buffer(buffer_),
      [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
      {
        if (!ec)
        {
          request_parser::result_type result;
          std::tie(result, std::ignore) = request_parser_.parse(
              request_, buffer_.data(), buffer_.data() + bytes_transferred);

          if (result == request_parser::good)
          {
            request_handler_.handle_request(request_, reply_);
            do_write();
          }
          else if (result == request_parser::bad)
          {
            reply_ = reply::stock_reply(reply::bad_request);
            do_write();
          }
          else
          {
            do_read();
          }
        }
        else if (ec != boost::asio::error::operation_aborted)
        {
          connection_manager_.stop(shared_from_this());
        }
      });
}

void connection::do_write()
{
  auto self(shared_from_this());
  boost::asio::async_write(socket_, reply_.to_buffers(),
      [this, self](boost::system::error_code ec, std::size_t)
      {
        if (!ec)
        {
          // Initiate graceful connection closure.
          boost::system::error_code ignored_ec;
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
            ignored_ec);
        }

        if (ec != boost::asio::error::operation_aborted)
        {
          connection_manager_.stop(shared_from_this());
        }
      });
}

} // namespace server
} // namespace http
// ---- merged connection_manager.cpp (the same program) ----
//
// connection_manager.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server {

connection_manager::connection_manager()
{
}

void connection_manager::start(connection_ptr c)
{
  connections_.insert(c);
  c->start();
}

void connection_manager::stop(connection_ptr c)
{
  connections_.erase(c);
  c->stop();
}

void connection_manager::stop_all()
{
  for (auto c: connections_)
    c->stop();
  connections_.clear();
}

} // namespace server
} // namespace http
// ---- merged mime_types.cpp (the same program) ----
//
// mime_types.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// ---- inlined mime_types.hpp ----
#undef HTTP_MIME_TYPES_HPP
//
// mime_types.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_MIME_TYPES_HPP
#define HTTP_MIME_TYPES_HPP


namespace http {
namespace server {
namespace mime_types {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

} // namespace mime_types
} // namespace server
} // namespace http

#endif // HTTP_MIME_TYPES_HPP
// ---- end mime_types.hpp ----

namespace http {
namespace server {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
{
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" }
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping m: mappings)
  {
    if (m.extension == extension)
    {
      return m.mime_type;
    }
  }

  return "text/plain";
}

} // namespace mime_types
} // namespace server
} // namespace http
// ---- merged reply.cpp (the same program) ----
//
// reply.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server {

namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return boost::asio::buffer(ok);
  case reply::created:
    return boost::asio::buffer(created);
  case reply::accepted:
    return boost::asio::buffer(accepted);
  case reply::no_content:
    return boost::asio::buffer(no_content);
  case reply::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case reply::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case reply::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case reply::not_modified:
    return boost::asio::buffer(not_modified);
  case reply::bad_request:
    return boost::asio::buffer(bad_request);
  case reply::unauthorized:
    return boost::asio::buffer(unauthorized);
  case reply::forbidden:
    return boost::asio::buffer(forbidden);
  case reply::not_found:
    return boost::asio::buffer(not_found);
  case reply::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case reply::not_implemented:
    return boost::asio::buffer(not_implemented);
  case reply::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case reply::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(status_strings::to_buffer(status));
  for (std::size_t i = 0; i < headers.size(); ++i)
  {
    header& h = headers[i];
    buffers.push_back(boost::asio::buffer(h.name));
    buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
    buffers.push_back(boost::asio::buffer(h.value));
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  }
  buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  buffers.push_back(boost::asio::buffer(content));
  return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return ok;
  case reply::created:
    return created;
  case reply::accepted:
    return accepted;
  case reply::no_content:
    return no_content;
  case reply::multiple_choices:
    return multiple_choices;
  case reply::moved_permanently:
    return moved_permanently;
  case reply::moved_temporarily:
    return moved_temporarily;
  case reply::not_modified:
    return not_modified;
  case reply::bad_request:
    return bad_request;
  case reply::unauthorized:
    return unauthorized;
  case reply::forbidden:
    return forbidden;
  case reply::not_found:
    return not_found;
  case reply::internal_server_error:
    return internal_server_error;
  case reply::not_implemented:
    return not_implemented;
  case reply::bad_gateway:
    return bad_gateway;
  case reply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status)
{
  reply rep;
  rep.status = status;
  rep.content = stock_replies::to_string(status);
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "text/html";
  return rep;
}

} // namespace server
} // namespace http
// ---- merged request_handler.cpp (the same program) ----
//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  if (!is)
  {
    rep = reply::stock_reply(reply::not_found);
    return;
  }

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
  char buf[512];
  while (is.read(buf, sizeof(buf)).gcount() > 0)
    rep.content.append(buf, is.gcount());
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server
} // namespace http
// ---- merged request_parser.cpp (the same program) ----
//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

request_parser::result_type request_parser::consume(request& req, char input)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.method.push_back(input);
      return indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.uri.push_back(input);
      return indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_3:
    return (input == '\n') ? good : bad;
  default:
    return bad;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

} // namespace server
} // namespace http
// ---- merged server.cpp (the same program) ----
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server {

server::server(const std::string& address, const std::string& port,
    const std::string& doc_root)
  : io_context_(1),
    signals_(io_context_),
    acceptor_(io_context_),
    connection_manager_(),
    request_handler_(doc_root)
{
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

  do_await_stop();

  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(io_context_);
  boost::asio::ip::tcp::endpoint endpoint =
    *resolver.resolve(address, port).begin();
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  do_accept();
}

void server::run()
{
  // The io_context::run() call will block until all asynchronous operations
  // have finished. While the server is running, there is always at least one
  // asynchronous operation outstanding: the asynchronous accept call waiting
  // for new incoming connections.
  io_context_.run();
}

void server::do_accept()
{
  acceptor_.async_accept(
      [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
      {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
          return;
        }

        if (!ec)
        {
          connection_manager_.start(std::make_shared<connection>(
              std::move(socket), connection_manager_, request_handler_));
        }

        do_accept();
      });
}

void server::do_await_stop()
{
  signals_.async_wait(
      [this](boost::system::error_code /*ec*/, int /*signo*/)
      {
        // The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_context::run()
        // call will exit.
        acceptor_.close();
        connection_manager_.stop_all();
      });
}

} // namespace server
} // namespace http

TEST_CASE("example: cpp11/http/server/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back(ex_root);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "GET /index.html HTTP/1.0\r\n\r\n", 0)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("received: HTTP/1.0 200 OK\r\n"
        "Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n", 108)));
}
}  // namespace ex_cpp11_http_server_main

// libs/asio/example/cpp11/http/server2/main.cpp
namespace ex_cpp11_http_server2_main {
#undef HTTP_SERVER2_SERVER_HPP
//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_SERVER_HPP
#define HTTP_SERVER2_SERVER_HPP

// ---- inlined io_context_pool.hpp ----
#undef HTTP_SERVER2_IO_SERVICE_POOL_HPP
//
// io_context_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_IO_SERVICE_POOL_HPP
#define HTTP_SERVER2_IO_SERVICE_POOL_HPP


namespace http {
namespace server2 {

/// A pool of io_context objects.
class io_context_pool
{
public:
  /// Construct the io_context pool.
  explicit io_context_pool(std::size_t pool_size);

  /// Run all io_context objects in the pool.
  void run();

  /// Stop all io_context objects in the pool.
  void stop();

  /// Get an io_context to use.
  boost::asio::io_context& get_io_context();

private:
  io_context_pool(const io_context_pool&) = delete;
  io_context_pool& operator=(const io_context_pool&) = delete;

  typedef std::shared_ptr<boost::asio::io_context> io_context_ptr;
  typedef boost::asio::executor_work_guard<
    boost::asio::io_context::executor_type> io_context_work;

  /// The pool of io_contexts.
  std::vector<io_context_ptr> io_contexts_;

  /// The work that keeps the io_contexts running.
  std::list<io_context_work> work_;

  /// The next io_context to use for a connection.
  std::size_t next_io_context_;
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_IO_SERVICE_POOL_HPP
// ---- end io_context_pool.hpp ----
// ---- inlined request_handler.hpp ----
#undef HTTP_SERVER2_REQUEST_HANDLER_HPP
//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_HANDLER_HPP
#define HTTP_SERVER2_REQUEST_HANDLER_HPP


namespace http {
namespace server2 {

struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler
{
public:
  request_handler(const request_handler&) = delete;
  request_handler& operator=(const request_handler&) = delete;

  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_HANDLER_HPP
// ---- end request_handler.hpp ----

namespace http {
namespace server2 {

/// The top-level class of the HTTP server.
class server
{
public:
  server(const server&) = delete;
  server& operator=(const server&) = delete;

  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const std::string& port,
      const std::string& doc_root, std::size_t io_context_pool_size);

  /// Run the server's io_context loop.
  void run();

private:
  /// Perform an asynchronous accept operation.
  void do_accept();

  /// Wait for a request to stop the server.
  void do_await_stop();

  /// The pool of io_context objects used to perform asynchronous operations.
  io_context_pool io_context_pool_;

  /// The signal_set is used to register for process termination notifications.
  boost::asio::signal_set signals_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The handler for all incoming requests.
  request_handler request_handler_;
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_SERVER_HPP
// ---- end server.hpp ----

int example_main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 5)
    {
      ex_out << "Usage: http_server <address> <port> <threads> <doc_root>\n";
      ex_out << "  For IPv4, try:\n";
      ex_out << "    receiver 0.0.0.0 80 1 .\n";
      ex_out << "  For IPv6, try:\n";
      ex_out << "    receiver 0::0 80 1 .\n";
      return 1;
    }

    // Initialise the server.
    std::size_t num_threads = std::stoi(argv[3]);
    http::server2::server s(argv[1], argv[2], argv[4], num_threads);

    // Run the server until stopped.
    s.run();
  }
  catch (std::exception& e)
  {
    ex_out << "exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}
// ---- merged connection.cpp (the same program) ----
//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// ---- inlined connection.hpp ----
#undef HTTP_SERVER2_CONNECTION_HPP
//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_CONNECTION_HPP
#define HTTP_SERVER2_CONNECTION_HPP

// ---- inlined reply.hpp ----
#undef HTTP_SERVER2_REPLY_HPP
//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REPLY_HPP
#define HTTP_SERVER2_REPLY_HPP

// ---- inlined header.hpp ----
#undef HTTP_SERVER2_HEADER_HPP
//
// header.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_HEADER_HPP
#define HTTP_SERVER2_HEADER_HPP


namespace http {
namespace server2 {

struct header
{
  std::string name;
  std::string value;
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_HEADER_HPP
// ---- end header.hpp ----

namespace http {
namespace server2 {

/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers();

  /// Get a stock reply.
  static reply stock_reply(status_type status);
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REPLY_HPP
// ---- end reply.hpp ----
// ---- inlined request.hpp ----
#undef HTTP_SERVER2_REQUEST_HPP
//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_HPP
#define HTTP_SERVER2_REQUEST_HPP


namespace http {
namespace server2 {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_HPP
// ---- end request.hpp ----
// ---- inlined request_parser.hpp ----
#undef HTTP_SERVER2_REQUEST_PARSER_HPP
//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_PARSER_HPP
#define HTTP_SERVER2_REQUEST_PARSER_HPP


namespace http {
namespace server2 {

struct request;

/// Parser for incoming requests.
class request_parser
{
public:
  /// Construct ready to parse the request method.
  request_parser();

  /// Reset to initial parser state.
  void reset();

  /// Result of parse.
  enum result_type { good, bad, indeterminate };

  /// Parse some data. The enum return value is good when a complete request has
  /// been parsed, bad if the data is invalid, indeterminate when more data is
  /// required. The InputIterator return value indicates how much of the input
  /// has been consumed.
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(request& req,
      InputIterator begin, InputIterator end)
  {
    while (begin != end)
    {
      result_type result = consume(req, *begin++);
      if (result == good || result == bad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(indeterminate, begin);
  }

private:
  /// Handle the next character of input.
  result_type consume(request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool is_char(int c);

  /// Check if a byte is an HTTP control character.
  static bool is_ctl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool is_tspecial(int c);

  /// Check if a byte is a digit.
  static bool is_digit(int c);

  /// The current state of the parser.
  enum state
  {
    method_start,
    method,
    uri,
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3
  } state_;
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_PARSER_HPP
// ---- end request_parser.hpp ----

namespace http {
namespace server2 {

/// Represents a single connection from a client.
class connection
  : public std::enable_shared_from_this<connection>
{
public:
  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  /// Construct a connection with the given socket.
  explicit connection(boost::asio::ip::tcp::socket socket,
      request_handler& handler);

  /// Start the first asynchronous operation for the connection.
  void start();

private:
  /// Perform an asynchronous read operation.
  void do_read();

  /// Perform an asynchronous write operation.
  void do_write();

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// Buffer for incoming data.
  std::array<char, 8192> buffer_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_CONNECTION_HPP
// ---- end connection.hpp ----

namespace http {
namespace server2 {

connection::connection(boost::asio::ip::tcp::socket socket,
    request_handler& handler)
  : socket_(std::move(socket)),
    request_handler_(handler)
{
}

void connection::start()
{
  do_read();
}

void connection::do_read()
{
  auto self(shared_from_this());
  socket_.async_read_some(boost::asio::buffer(buffer_),
      [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
      {
        if (!ec)
        {
          request_parser::result_type result;
          std::tie(result, std::ignore) = request_parser_.parse(
              request_, buffer_.data(), buffer_.data() + bytes_transferred);

          if (result == request_parser::good)
          {
            request_handler_.handle_request(request_, reply_);
            do_write();
          }
          else if (result == request_parser::bad)
          {
            reply_ = reply::stock_reply(reply::bad_request);
            do_write();
          }
          else
          {
            do_read();
          }
        }

        // If an error occurs then no new asynchronous operations are
        // started. This means that all shared_ptr references to the
        // connection object will disappear and the object will be
        // destroyed automatically after this handler returns. The
        // connection class's destructor closes the socket.
      });
}

void connection::do_write()
{
  auto self(shared_from_this());
  boost::asio::async_write(socket_, reply_.to_buffers(),
      [this, self](boost::system::error_code ec, std::size_t)
      {
        if (!ec)
        {
          // Initiate graceful connection closure.
          boost::system::error_code ignored_ec;
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
            ignored_ec);
        }

        // No new asynchronous operations are started. This means that
        // all shared_ptr references to the connection object will
        // disappear and the object will be destroyed automatically after
        // this handler returns. The connection class's destructor closes
        // the socket.
      });
}

} // namespace server2
} // namespace http
// ---- merged mime_types.cpp (the same program) ----
//
// mime_types.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// ---- inlined mime_types.hpp ----
#undef HTTP_SERVER2_MIME_TYPES_HPP
//
// mime_types.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_MIME_TYPES_HPP
#define HTTP_SERVER2_MIME_TYPES_HPP


namespace http {
namespace server2 {
namespace mime_types {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

} // namespace mime_types
} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_MIME_TYPES_HPP
// ---- end mime_types.hpp ----

namespace http {
namespace server2 {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
{
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" },
  { 0, 0 } // Marks end of list.
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping* m = mappings; m->extension; ++m)
  {
    if (m->extension == extension)
    {
      return m->mime_type;
    }
  }

  return "text/plain";
}

} // namespace mime_types
} // namespace server2
} // namespace http
// ---- merged reply.cpp (the same program) ----
//
// reply.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server2 {

namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return boost::asio::buffer(ok);
  case reply::created:
    return boost::asio::buffer(created);
  case reply::accepted:
    return boost::asio::buffer(accepted);
  case reply::no_content:
    return boost::asio::buffer(no_content);
  case reply::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case reply::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case reply::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case reply::not_modified:
    return boost::asio::buffer(not_modified);
  case reply::bad_request:
    return boost::asio::buffer(bad_request);
  case reply::unauthorized:
    return boost::asio::buffer(unauthorized);
  case reply::forbidden:
    return boost::asio::buffer(forbidden);
  case reply::not_found:
    return boost::asio::buffer(not_found);
  case reply::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case reply::not_implemented:
    return boost::asio::buffer(not_implemented);
  case reply::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case reply::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(status_strings::to_buffer(status));
  for (std::size_t i = 0; i < headers.size(); ++i)
  {
    header& h = headers[i];
    buffers.push_back(boost::asio::buffer(h.name));
    buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
    buffers.push_back(boost::asio::buffer(h.value));
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  }
  buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  buffers.push_back(boost::asio::buffer(content));
  return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return ok;
  case reply::created:
    return created;
  case reply::accepted:
    return accepted;
  case reply::no_content:
    return no_content;
  case reply::multiple_choices:
    return multiple_choices;
  case reply::moved_permanently:
    return moved_permanently;
  case reply::moved_temporarily:
    return moved_temporarily;
  case reply::not_modified:
    return not_modified;
  case reply::bad_request:
    return bad_request;
  case reply::unauthorized:
    return unauthorized;
  case reply::forbidden:
    return forbidden;
  case reply::not_found:
    return not_found;
  case reply::internal_server_error:
    return internal_server_error;
  case reply::not_implemented:
    return not_implemented;
  case reply::bad_gateway:
    return bad_gateway;
  case reply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status)
{
  reply rep;
  rep.status = status;
  rep.content = stock_replies::to_string(status);
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "text/html";
  return rep;
}

} // namespace server2
} // namespace http
// ---- merged request_handler.cpp (the same program) ----
//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server2 {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  if (!is)
  {
    rep = reply::stock_reply(reply::not_found);
    return;
  }

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
  char buf[512];
  while (is.read(buf, sizeof(buf)).gcount() > 0)
    rep.content.append(buf, is.gcount());
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server2
} // namespace http
// ---- merged request_parser.cpp (the same program) ----
//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server2 {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

request_parser::result_type request_parser::consume(request& req, char input)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.method.push_back(input);
      return indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.uri.push_back(input);
      return indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_3:
    return (input == '\n') ? good : bad;
  default:
    return bad;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

} // namespace server2
} // namespace http
// ---- merged server.cpp (the same program) ----
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server2 {

server::server(const std::string& address, const std::string& port,
    const std::string& doc_root, std::size_t io_context_pool_size)
  : io_context_pool_(io_context_pool_size),
    signals_(io_context_pool_.get_io_context()),
    acceptor_(io_context_pool_.get_io_context()),
    request_handler_(doc_root)
{
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

  do_await_stop();

  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(acceptor_.get_executor());
  boost::asio::ip::tcp::endpoint endpoint =
    *resolver.resolve(address, port).begin();
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  do_accept();
}

void server::run()
{
  io_context_pool_.run();
}

void server::do_accept()
{
  acceptor_.async_accept(io_context_pool_.get_io_context(),
      [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
      {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
          return;
        }

        if (!ec)
        {
          std::make_shared<connection>(
              std::move(socket), request_handler_)->start();
        }

        do_accept();
      });
}

void server::do_await_stop()
{
  signals_.async_wait(
      [this](boost::system::error_code /*ec*/, int /*signo*/)
      {
        io_context_pool_.stop();
      });
}

} // namespace server2
} // namespace http
// ---- merged io_context_pool.cpp (the same program) ----
//
// io_context_pool.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server2 {

io_context_pool::io_context_pool(std::size_t pool_size)
  : next_io_context_(0)
{
  if (pool_size == 0)
    throw std::runtime_error("io_context_pool size is 0");

  // Give all the io_contexts work to do so that their run() functions will not
  // exit until they are explicitly stopped.
  for (std::size_t i = 0; i < pool_size; ++i)
  {
    io_context_ptr io_context(new boost::asio::io_context);
    io_contexts_.push_back(io_context);
    work_.push_back(boost::asio::make_work_guard(*io_context));
  }
}

void io_context_pool::run()
{
  // Create a pool of threads to run all of the io_contexts.
  std::vector<std::thread> threads;
  for (std::size_t i = 0; i < io_contexts_.size(); ++i)
    threads.emplace_back([this, i]{ io_contexts_[i]->run(); });

  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i].join();
}

void io_context_pool::stop()
{
  // Explicitly stop all io_contexts.
  for (std::size_t i = 0; i < io_contexts_.size(); ++i)
    io_contexts_[i]->stop();
}

boost::asio::io_context& io_context_pool::get_io_context()
{
  // Use a round-robin scheme to choose the next io_context to use.
  boost::asio::io_context& io_context = *io_contexts_[next_io_context_];
  ++next_io_context_;
  if (next_io_context_ == io_contexts_.size())
    next_io_context_ = 0;
  return io_context;
}

} // namespace server2
} // namespace http

TEST_CASE("example: cpp11/http/server2/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("2");
    ex_args.push_back(ex_root);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "GET /index.html HTTP/1.0\r\n\r\n", 0)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("received: HTTP/1.0 200 OK\r\n"
        "Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n", 108)));
}
}  // namespace ex_cpp11_http_server2_main

// libs/asio/example/cpp11/http/server3/main.cpp
namespace ex_cpp11_http_server3_main {
#undef HTTP_SERVER3_SERVER_HPP
//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_SERVER_HPP
#define HTTP_SERVER3_SERVER_HPP

// ---- inlined request_handler.hpp ----
#undef HTTP_SERVER3_REQUEST_HANDLER_HPP
//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_HANDLER_HPP
#define HTTP_SERVER3_REQUEST_HANDLER_HPP


namespace http {
namespace server3 {

struct reply;
struct request;

/// The common handler for all incoming requests.
class request_handler
{
public:
  request_handler(const request_handler&) = delete;
  request_handler& operator=(const request_handler&) = delete;

  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void handle_request(const request& req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_REQUEST_HANDLER_HPP
// ---- end request_handler.hpp ----

namespace http {
namespace server3 {

/// The top-level class of the HTTP server.
class server
{
public:
  server(const server&) = delete;
  server& operator=(const server&) = delete;

  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const std::string& port,
      const std::string& doc_root, std::size_t thread_pool_size);

  /// Run the server's io_context loop.
  void run();

private:
  /// Perform an asynchronous accept operation.
  void do_accept();

  /// Wait for a request to stop the server.
  void do_await_stop();

  /// The number of threads that will call io_context::run().
  std::size_t thread_pool_size_;

  /// The io_context used to perform asynchronous operations.
  boost::asio::io_context io_context_;

  /// The signal_set is used to register for process termination notifications.
  boost::asio::signal_set signals_;

  /// Acceptor used to listen for incoming connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The handler for all incoming requests.
  request_handler request_handler_;
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_SERVER_HPP
// ---- end server.hpp ----

int example_main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 5)
    {
      ex_out << "Usage: http_server <address> <port> <threads> <doc_root>\n";
      ex_out << "  For IPv4, try:\n";
      ex_out << "    receiver 0.0.0.0 80 1 .\n";
      ex_out << "  For IPv6, try:\n";
      ex_out << "    receiver 0::0 80 1 .\n";
      return 1;
    }

    // Initialise the server.
    std::size_t num_threads = std::stoi(argv[3]);
    http::server3::server s(argv[1], argv[2], argv[4], num_threads);

    // Run the server until stopped.
    s.run();
  }
  catch (std::exception& e)
  {
    ex_out << "exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}
// ---- merged connection.cpp (the same program) ----
//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// ---- inlined connection.hpp ----
#undef HTTP_SERVER3_CONNECTION_HPP
//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_CONNECTION_HPP
#define HTTP_SERVER3_CONNECTION_HPP

// ---- inlined reply.hpp ----
#undef HTTP_SERVER3_REPLY_HPP
//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REPLY_HPP
#define HTTP_SERVER3_REPLY_HPP

// ---- inlined header.hpp ----
#undef HTTP_SERVER3_HEADER_HPP
//
// header.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_HEADER_HPP
#define HTTP_SERVER3_HEADER_HPP


namespace http {
namespace server3 {

struct header
{
  std::string name;
  std::string value;
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_HEADER_HPP
// ---- end header.hpp ----

namespace http {
namespace server3 {

/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers();

  /// Get a stock reply.
  static reply stock_reply(status_type status);
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_REPLY_HPP
// ---- end reply.hpp ----
// ---- inlined request.hpp ----
#undef HTTP_SERVER3_REQUEST_HPP
//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_HPP
#define HTTP_SERVER3_REQUEST_HPP


namespace http {
namespace server3 {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_REQUEST_HPP
// ---- end request.hpp ----
// ---- inlined request_parser.hpp ----
#undef HTTP_SERVER3_REQUEST_PARSER_HPP
//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_PARSER_HPP
#define HTTP_SERVER3_REQUEST_PARSER_HPP


namespace http {
namespace server3 {

struct request;

/// Parser for incoming requests.
class request_parser
{
public:
  /// Construct ready to parse the request method.
  request_parser();

  /// Reset to initial parser state.
  void reset();

  /// Result of parse.
  enum result_type { good, bad, indeterminate };

  /// Parse some data. The enum return value is good when a complete request has
  /// been parsed, bad if the data is invalid, indeterminate when more data is
  /// required. The InputIterator return value indicates how much of the input
  /// has been consumed.
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(request& req,
      InputIterator begin, InputIterator end)
  {
    while (begin != end)
    {
      result_type result = consume(req, *begin++);
      if (result == good || result == bad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(indeterminate, begin);
  }

private:
  /// Handle the next character of input.
  result_type consume(request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool is_char(int c);

  /// Check if a byte is an HTTP control character.
  static bool is_ctl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool is_tspecial(int c);

  /// Check if a byte is a digit.
  static bool is_digit(int c);

  /// The current state of the parser.
  enum state
  {
    method_start,
    method,
    uri,
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3
  } state_;
};

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_REQUEST_PARSER_HPP
// ---- end request_parser.hpp ----

namespace http {
namespace server3 {

/// Represents a single connection from a client.
class connection
  : public std::enable_shared_from_this<connection>
{
public:
  connection(const connection&) = delete;
  connection& operator=(const connection&) = delete;

  /// Construct a connection with the given socket.
  explicit connection(boost::asio::ip::tcp::socket socket,
      request_handler& handler);

  /// Start the first asynchronous operation for the connection.
  void start();

private:
  /// Perform an asynchronous read operation.
  void do_read();

  /// Perform an asynchronous write operation.
  void do_write();

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// Buffer for incoming data.
  std::array<char, 8192> buffer_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_CONNECTION_HPP
// ---- end connection.hpp ----

namespace http {
namespace server3 {

connection::connection(boost::asio::ip::tcp::socket socket,
    request_handler& handler)
  : socket_(std::move(socket)),
    request_handler_(handler)
{
}

void connection::start()
{
  do_read();
}

void connection::do_read()
{
  auto self(shared_from_this());
  socket_.async_read_some(boost::asio::buffer(buffer_),
      [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
      {
        if (!ec)
        {
          request_parser::result_type result;
          std::tie(result, std::ignore) = request_parser_.parse(
              request_, buffer_.data(), buffer_.data() + bytes_transferred);

          if (result == request_parser::good)
          {
            request_handler_.handle_request(request_, reply_);
            do_write();
          }
          else if (result == request_parser::bad)
          {
            reply_ = reply::stock_reply(reply::bad_request);
            do_write();
          }
          else
          {
            do_read();
          }
        }

        // If an error occurs then no new asynchronous operations are
        // started. This means that all shared_ptr references to the
        // connection object will disappear and the object will be
        // destroyed automatically after this handler returns. The
        // connection class's destructor closes the socket.
      });
}

void connection::do_write()
{
  auto self(shared_from_this());
  boost::asio::async_write(socket_, reply_.to_buffers(),
      [this, self](boost::system::error_code ec, std::size_t)
      {
        if (!ec)
        {
          // Initiate graceful connection closure.
          boost::system::error_code ignored_ec;
          socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
            ignored_ec);
        }

        // No new asynchronous operations are started. This means that
        // all shared_ptr references to the connection object will
        // disappear and the object will be destroyed automatically after
        // this handler returns. The connection class's destructor closes
        // the socket.
      });
}

} // namespace server3
} // namespace http
// ---- merged mime_types.cpp (the same program) ----
//
// mime_types.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// ---- inlined mime_types.hpp ----
#undef HTTP_SERVER3_MIME_TYPES_HPP
//
// mime_types.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_MIME_TYPES_HPP
#define HTTP_SERVER3_MIME_TYPES_HPP


namespace http {
namespace server3 {
namespace mime_types {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

} // namespace mime_types
} // namespace server3
} // namespace http

#endif // HTTP_SERVER3_MIME_TYPES_HPP
// ---- end mime_types.hpp ----

namespace http {
namespace server3 {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
{
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" },
  { 0, 0 } // Marks end of list.
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping* m = mappings; m->extension; ++m)
  {
    if (m->extension == extension)
    {
      return m->mime_type;
    }
  }

  return "text/plain";
}

} // namespace mime_types
} // namespace server3
} // namespace http
// ---- merged reply.cpp (the same program) ----
//
// reply.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server3 {

namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return boost::asio::buffer(ok);
  case reply::created:
    return boost::asio::buffer(created);
  case reply::accepted:
    return boost::asio::buffer(accepted);
  case reply::no_content:
    return boost::asio::buffer(no_content);
  case reply::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case reply::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case reply::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case reply::not_modified:
    return boost::asio::buffer(not_modified);
  case reply::bad_request:
    return boost::asio::buffer(bad_request);
  case reply::unauthorized:
    return boost::asio::buffer(unauthorized);
  case reply::forbidden:
    return boost::asio::buffer(forbidden);
  case reply::not_found:
    return boost::asio::buffer(not_found);
  case reply::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case reply::not_implemented:
    return boost::asio::buffer(not_implemented);
  case reply::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case reply::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(status_strings::to_buffer(status));
  for (std::size_t i = 0; i < headers.size(); ++i)
  {
    header& h = headers[i];
    buffers.push_back(boost::asio::buffer(h.name));
    buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
    buffers.push_back(boost::asio::buffer(h.value));
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  }
  buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  buffers.push_back(boost::asio::buffer(content));
  return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return ok;
  case reply::created:
    return created;
  case reply::accepted:
    return accepted;
  case reply::no_content:
    return no_content;
  case reply::multiple_choices:
    return multiple_choices;
  case reply::moved_permanently:
    return moved_permanently;
  case reply::moved_temporarily:
    return moved_temporarily;
  case reply::not_modified:
    return not_modified;
  case reply::bad_request:
    return bad_request;
  case reply::unauthorized:
    return unauthorized;
  case reply::forbidden:
    return forbidden;
  case reply::not_found:
    return not_found;
  case reply::internal_server_error:
    return internal_server_error;
  case reply::not_implemented:
    return not_implemented;
  case reply::bad_gateway:
    return bad_gateway;
  case reply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status)
{
  reply rep;
  rep.status = status;
  rep.content = stock_replies::to_string(status);
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "text/html";
  return rep;
}

} // namespace server3
} // namespace http
// ---- merged request_handler.cpp (the same program) ----
//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server3 {

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

void request_handler::handle_request(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  if (!is)
  {
    rep = reply::stock_reply(reply::not_found);
    return;
  }

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
  char buf[512];
  while (is.read(buf, sizeof(buf)).gcount() > 0)
    rep.content.append(buf, is.gcount());
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server3
} // namespace http
// ---- merged request_parser.cpp (the same program) ----
//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server3 {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

request_parser::result_type request_parser::consume(request& req, char input)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.method.push_back(input);
      return indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.uri.push_back(input);
      return indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_3:
    return (input == '\n') ? good : bad;
  default:
    return bad;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

} // namespace server3
} // namespace http
// ---- merged server.cpp (the same program) ----
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server3 {

server::server(const std::string& address, const std::string& port,
    const std::string& doc_root, std::size_t thread_pool_size)
  : thread_pool_size_(thread_pool_size),
    signals_(io_context_),
    acceptor_(io_context_),
    request_handler_(doc_root)
{
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

  do_await_stop();

  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(io_context_);
  boost::asio::ip::tcp::endpoint endpoint =
    *resolver.resolve(address, port).begin();
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  do_accept();
}

void server::run()
{
  // Create a pool of threads to run the io_context.
  std::vector<std::thread> threads;
  for (std::size_t i = 0; i < thread_pool_size_; ++i)
    threads.emplace_back([this]{ io_context_.run(); });

  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i].join();
}

void server::do_accept()
{
  // The newly accepted socket is put into its own strand to ensure that all
  // completion handlers associated with the connection do not run concurrently.
  acceptor_.async_accept(boost::asio::make_strand(io_context_),
      [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket)
      {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
          return;
        }

        if (!ec)
        {
          std::make_shared<connection>(
              std::move(socket), request_handler_)->start();
        }

        do_accept();
      });
}

void server::do_await_stop()
{
  signals_.async_wait(
      [this](boost::system::error_code /*ec*/, int /*signo*/)
      {
        io_context_.stop();
      });
}

} // namespace server3
} // namespace http

TEST_CASE("example: cpp11/http/server3/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("2");
    ex_args.push_back(ex_root);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "GET /index.html HTTP/1.0\r\n\r\n", 0)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("received: HTTP/1.0 200 OK\r\n"
        "Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n", 108)));
}
}  // namespace ex_cpp11_http_server3_main

// libs/asio/example/cpp11/http/server4/main.cpp
namespace ex_cpp11_http_server4_main {
#undef HTTP_SERVER4_SERVER_HPP
//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_SERVER_HPP
#define HTTP_SERVER4_SERVER_HPP

// ---- inlined request_parser.hpp ----
#undef HTTP_SERVER4_REQUEST_PARSER_HPP
//
// request_parser.hpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_REQUEST_PARSER_HPP
#define HTTP_SERVER4_REQUEST_PARSER_HPP


namespace http {
namespace server4 {

struct request;

/// Parser for incoming requests.
class request_parser
{
public:
  /// Construct ready to parse the request method.
  request_parser();

  /// Reset to initial parser state.
  void reset();

  /// Result of parse.
  enum result_type { good, bad, indeterminate };

  /// Parse some data. The enum return value is good when a complete request has
  /// been parsed, bad if the data is invalid, indeterminate when more data is
  /// required. The InputIterator return value indicates how much of the input
  /// has been consumed.
  template <typename InputIterator>
  std::tuple<result_type, InputIterator> parse(request& req,
      InputIterator begin, InputIterator end)
  {
    while (begin != end)
    {
      result_type result = consume(req, *begin++);
      if (result == good || result == bad)
        return std::make_tuple(result, begin);
    }
    return std::make_tuple(indeterminate, begin);
  }

private:
  /// Handle the next character of input.
  result_type consume(request& req, char input);

  /// Check if a byte is an HTTP character.
  static bool is_char(int c);

  /// Check if a byte is an HTTP control character.
  static bool is_ctl(int c);

  /// Check if a byte is defined as an HTTP tspecial character.
  static bool is_tspecial(int c);

  /// Check if a byte is a digit.
  static bool is_digit(int c);

  /// The current state of the parser.
  enum state
  {
    method_start,
    method,
    uri,
    http_version_h,
    http_version_t_1,
    http_version_t_2,
    http_version_p,
    http_version_slash,
    http_version_major_start,
    http_version_major,
    http_version_minor_start,
    http_version_minor,
    expecting_newline_1,
    header_line_start,
    header_lws,
    header_name,
    space_before_header_value,
    header_value,
    expecting_newline_2,
    expecting_newline_3
  } state_;
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_REQUEST_PARSER_HPP
// ---- end request_parser.hpp ----

namespace http {
namespace server4 {

struct request;
struct reply;

/// The top-level coroutine of the HTTP server.
class server : boost::asio::coroutine
{
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(boost::asio::io_context& io_context,
      const std::string& address, const std::string& port,
      std::function<void(const request&, reply&)> request_handler);

  /// Perform work associated with the server.
  void operator()(
      boost::system::error_code ec = boost::system::error_code(),
      std::size_t length = 0);

private:
  typedef boost::asio::ip::tcp tcp;

  /// The user-supplied handler for all incoming requests.
  std::function<void(const request&, reply&)> request_handler_;

  /// Acceptor used to listen for incoming connections.
  std::shared_ptr<tcp::acceptor> acceptor_;

  /// The current connection from a client.
  std::shared_ptr<tcp::socket> socket_;

  /// Buffer for incoming data.
  std::shared_ptr<std::array<char, 8192>> buffer_;

  /// The incoming request.
  std::shared_ptr<request> request_;

  /// Whether the request is valid or not.
  request_parser::result_type parse_result_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  std::shared_ptr<reply> reply_;
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_SERVER_HPP
// ---- end server.hpp ----
// ---- inlined file_handler.hpp ----
#undef HTTP_SERVER4_FILE_HANDLER_HPP
//
// file_handler.hpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_FILE_HANDLER_HPP
#define HTTP_SERVER4_FILE_HANDLER_HPP


namespace http {
namespace server4 {

struct reply;
struct request;

/// The common handler for all incoming requests.
class file_handler
{
public:
  /// Construct with a directory containing files to be served.
  explicit file_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void operator()(const request& req, reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_FILE_HANDLER_HPP
// ---- end file_handler.hpp ----

int example_main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 4)
    {
      ex_out << "Usage: http_server <address> <port> <doc_root>\n";
      ex_out << "  For IPv4, try:\n";
      ex_out << "    receiver 0.0.0.0 80 .\n";
      ex_out << "  For IPv6, try:\n";
      ex_out << "    receiver 0::0 80 .\n";
      return 1;
    }

    boost::asio::io_context io_context;

    // Launch the initial server coroutine.
    http::server4::server(io_context, argv[1], argv[2],
        http::server4::file_handler(argv[3]))();

    // Wait for signals indicating time to shut down.
    boost::asio::signal_set signals(io_context);
    signals.add(SIGINT);
    signals.add(SIGTERM);
#if defined(SIGQUIT)
    signals.add(SIGQUIT);
#endif // defined(SIGQUIT)
    signals.async_wait(
        [&io_context](boost::system::error_code, int)
        {
          io_context.stop();
        });

    // Run the server.
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}
// ---- merged file_handler.cpp (the same program) ----
//
// file_handler.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

// ---- inlined mime_types.hpp ----
#undef HTTP_SERVER4_MIME_TYPES_HPP
//
// mime_types.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_MIME_TYPES_HPP
#define HTTP_SERVER4_MIME_TYPES_HPP


namespace http {
namespace server4 {
namespace mime_types {

/// Convert a file extension into a MIME type.
std::string extension_to_type(const std::string& extension);

} // namespace mime_types
} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_MIME_TYPES_HPP
// ---- end mime_types.hpp ----
// ---- inlined reply.hpp ----
#undef HTTP_SERVER4_REPLY_HPP
//
// reply.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_REPLY_HPP
#define HTTP_SERVER4_REPLY_HPP

// ---- inlined header.hpp ----
#undef HTTP_SERVER4_HEADER_HPP
//
// header.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_HEADER_HPP
#define HTTP_SERVER4_HEADER_HPP


namespace http {
namespace server4 {

struct header
{
  std::string name;
  std::string value;
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_HEADER_HPP
// ---- end header.hpp ----

namespace http {
namespace server4 {

/// A reply to be sent to a client.
struct reply
{
  /// The status of the reply.
  enum status_type
  {
    ok = 200,
    created = 201,
    accepted = 202,
    no_content = 204,
    multiple_choices = 300,
    moved_permanently = 301,
    moved_temporarily = 302,
    not_modified = 304,
    bad_request = 400,
    unauthorized = 401,
    forbidden = 403,
    not_found = 404,
    internal_server_error = 500,
    not_implemented = 501,
    bad_gateway = 502,
    service_unavailable = 503
  } status;

  /// The headers to be included in the reply.
  std::vector<header> headers;

  /// The content to be sent in the reply.
  std::string content;

  /// Convert the reply into a vector of buffers. The buffers do not own the
  /// underlying memory blocks, therefore the reply object must remain valid and
  /// not be changed until the write operation has completed.
  std::vector<boost::asio::const_buffer> to_buffers();

  /// Get a stock reply.
  static reply stock_reply(status_type status);
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_REPLY_HPP
// ---- end reply.hpp ----
// ---- inlined request.hpp ----
#undef HTTP_SERVER4_REQUEST_HPP
//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER4_REQUEST_HPP
#define HTTP_SERVER4_REQUEST_HPP


namespace http {
namespace server4 {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
};

} // namespace server4
} // namespace http

#endif // HTTP_SERVER4_REQUEST_HPP
// ---- end request.hpp ----

namespace http {
namespace server4 {

file_handler::file_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

void file_handler::operator()(const request& req, reply& rep)
{
  // Decode url to path.
  std::string request_path;
  if (!url_decode(req.uri, request_path))
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = reply::stock_reply(reply::bad_request);
    return;
  }

  // If path ends in slash (i.e. is a directory) then add "index.html".
  if (request_path[request_path.size() - 1] == '/')
  {
    request_path += "index.html";
  }

  // Determine the file extension.
  std::size_t last_slash_pos = request_path.find_last_of("/");
  std::size_t last_dot_pos = request_path.find_last_of(".");
  std::string extension;
  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
  {
    extension = request_path.substr(last_dot_pos + 1);
  }

  // Open the file to send back.
  std::string full_path = doc_root_ + request_path;
  std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
  if (!is)
  {
    rep = reply::stock_reply(reply::not_found);
    return;
  }

  // Fill out the reply to be sent to the client.
  rep.status = reply::ok;
  char buf[512];
  while (is.read(buf, sizeof(buf)).gcount() > 0)
    rep.content.append(buf, is.gcount());
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = mime_types::extension_to_type(extension);
}

bool file_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

} // namespace server4
} // namespace http
// ---- merged mime_types.cpp (the same program) ----
//
// mime_types.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server4 {
namespace mime_types {

struct mapping
{
  const char* extension;
  const char* mime_type;
} mappings[] =
{
  { "gif", "image/gif" },
  { "htm", "text/html" },
  { "html", "text/html" },
  { "jpg", "image/jpeg" },
  { "png", "image/png" },
  { 0, 0 } // Marks end of list.
};

std::string extension_to_type(const std::string& extension)
{
  for (mapping* m = mappings; m->extension; ++m)
  {
    if (m->extension == extension)
    {
      return m->mime_type;
    }
  }

  return "text/plain";
}

} // namespace mime_types
} // namespace server4
} // namespace http
// ---- merged reply.cpp (the same program) ----
//
// reply.cpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server4 {

namespace status_strings {

const std::string ok =
  "HTTP/1.0 200 OK\r\n";
const std::string created =
  "HTTP/1.0 201 Created\r\n";
const std::string accepted =
  "HTTP/1.0 202 Accepted\r\n";
const std::string no_content =
  "HTTP/1.0 204 No Content\r\n";
const std::string multiple_choices =
  "HTTP/1.0 300 Multiple Choices\r\n";
const std::string moved_permanently =
  "HTTP/1.0 301 Moved Permanently\r\n";
const std::string moved_temporarily =
  "HTTP/1.0 302 Moved Temporarily\r\n";
const std::string not_modified =
  "HTTP/1.0 304 Not Modified\r\n";
const std::string bad_request =
  "HTTP/1.0 400 Bad Request\r\n";
const std::string unauthorized =
  "HTTP/1.0 401 Unauthorized\r\n";
const std::string forbidden =
  "HTTP/1.0 403 Forbidden\r\n";
const std::string not_found =
  "HTTP/1.0 404 Not Found\r\n";
const std::string internal_server_error =
  "HTTP/1.0 500 Internal Server Error\r\n";
const std::string not_implemented =
  "HTTP/1.0 501 Not Implemented\r\n";
const std::string bad_gateway =
  "HTTP/1.0 502 Bad Gateway\r\n";
const std::string service_unavailable =
  "HTTP/1.0 503 Service Unavailable\r\n";

boost::asio::const_buffer to_buffer(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return boost::asio::buffer(ok);
  case reply::created:
    return boost::asio::buffer(created);
  case reply::accepted:
    return boost::asio::buffer(accepted);
  case reply::no_content:
    return boost::asio::buffer(no_content);
  case reply::multiple_choices:
    return boost::asio::buffer(multiple_choices);
  case reply::moved_permanently:
    return boost::asio::buffer(moved_permanently);
  case reply::moved_temporarily:
    return boost::asio::buffer(moved_temporarily);
  case reply::not_modified:
    return boost::asio::buffer(not_modified);
  case reply::bad_request:
    return boost::asio::buffer(bad_request);
  case reply::unauthorized:
    return boost::asio::buffer(unauthorized);
  case reply::forbidden:
    return boost::asio::buffer(forbidden);
  case reply::not_found:
    return boost::asio::buffer(not_found);
  case reply::internal_server_error:
    return boost::asio::buffer(internal_server_error);
  case reply::not_implemented:
    return boost::asio::buffer(not_implemented);
  case reply::bad_gateway:
    return boost::asio::buffer(bad_gateway);
  case reply::service_unavailable:
    return boost::asio::buffer(service_unavailable);
  default:
    return boost::asio::buffer(internal_server_error);
  }
}

} // namespace status_strings

namespace misc_strings {

const char name_value_separator[] = { ':', ' ' };
const char crlf[] = { '\r', '\n' };

} // namespace misc_strings

std::vector<boost::asio::const_buffer> reply::to_buffers()
{
  std::vector<boost::asio::const_buffer> buffers;
  buffers.push_back(status_strings::to_buffer(status));
  for (std::size_t i = 0; i < headers.size(); ++i)
  {
    header& h = headers[i];
    buffers.push_back(boost::asio::buffer(h.name));
    buffers.push_back(boost::asio::buffer(misc_strings::name_value_separator));
    buffers.push_back(boost::asio::buffer(h.value));
    buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  }
  buffers.push_back(boost::asio::buffer(misc_strings::crlf));
  buffers.push_back(boost::asio::buffer(content));
  return buffers;
}

namespace stock_replies {

const char ok[] = "";
const char created[] =
  "<html>"
  "<head><title>Created</title></head>"
  "<body><h1>201 Created</h1></body>"
  "</html>";
const char accepted[] =
  "<html>"
  "<head><title>Accepted</title></head>"
  "<body><h1>202 Accepted</h1></body>"
  "</html>";
const char no_content[] =
  "<html>"
  "<head><title>No Content</title></head>"
  "<body><h1>204 Content</h1></body>"
  "</html>";
const char multiple_choices[] =
  "<html>"
  "<head><title>Multiple Choices</title></head>"
  "<body><h1>300 Multiple Choices</h1></body>"
  "</html>";
const char moved_permanently[] =
  "<html>"
  "<head><title>Moved Permanently</title></head>"
  "<body><h1>301 Moved Permanently</h1></body>"
  "</html>";
const char moved_temporarily[] =
  "<html>"
  "<head><title>Moved Temporarily</title></head>"
  "<body><h1>302 Moved Temporarily</h1></body>"
  "</html>";
const char not_modified[] =
  "<html>"
  "<head><title>Not Modified</title></head>"
  "<body><h1>304 Not Modified</h1></body>"
  "</html>";
const char bad_request[] =
  "<html>"
  "<head><title>Bad Request</title></head>"
  "<body><h1>400 Bad Request</h1></body>"
  "</html>";
const char unauthorized[] =
  "<html>"
  "<head><title>Unauthorized</title></head>"
  "<body><h1>401 Unauthorized</h1></body>"
  "</html>";
const char forbidden[] =
  "<html>"
  "<head><title>Forbidden</title></head>"
  "<body><h1>403 Forbidden</h1></body>"
  "</html>";
const char not_found[] =
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>404 Not Found</h1></body>"
  "</html>";
const char internal_server_error[] =
  "<html>"
  "<head><title>Internal Server Error</title></head>"
  "<body><h1>500 Internal Server Error</h1></body>"
  "</html>";
const char not_implemented[] =
  "<html>"
  "<head><title>Not Implemented</title></head>"
  "<body><h1>501 Not Implemented</h1></body>"
  "</html>";
const char bad_gateway[] =
  "<html>"
  "<head><title>Bad Gateway</title></head>"
  "<body><h1>502 Bad Gateway</h1></body>"
  "</html>";
const char service_unavailable[] =
  "<html>"
  "<head><title>Service Unavailable</title></head>"
  "<body><h1>503 Service Unavailable</h1></body>"
  "</html>";

std::string to_string(reply::status_type status)
{
  switch (status)
  {
  case reply::ok:
    return ok;
  case reply::created:
    return created;
  case reply::accepted:
    return accepted;
  case reply::no_content:
    return no_content;
  case reply::multiple_choices:
    return multiple_choices;
  case reply::moved_permanently:
    return moved_permanently;
  case reply::moved_temporarily:
    return moved_temporarily;
  case reply::not_modified:
    return not_modified;
  case reply::bad_request:
    return bad_request;
  case reply::unauthorized:
    return unauthorized;
  case reply::forbidden:
    return forbidden;
  case reply::not_found:
    return not_found;
  case reply::internal_server_error:
    return internal_server_error;
  case reply::not_implemented:
    return not_implemented;
  case reply::bad_gateway:
    return bad_gateway;
  case reply::service_unavailable:
    return service_unavailable;
  default:
    return internal_server_error;
  }
}

} // namespace stock_replies

reply reply::stock_reply(reply::status_type status)
{
  reply rep;
  rep.status = status;
  rep.content = stock_replies::to_string(status);
  rep.headers.resize(2);
  rep.headers[0].name = "Content-Length";
  rep.headers[0].value = std::to_string(rep.content.size());
  rep.headers[1].name = "Content-Type";
  rep.headers[1].value = "text/html";
  return rep;
}

} // namespace server4
} // namespace http
// ---- merged request_parser.cpp (the same program) ----
//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server4 {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

request_parser::result_type request_parser::consume(request& req, char input)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.method.push_back(input);
      return indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.uri.push_back(input);
      return indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return indeterminate;
    }
    else if (is_ctl(input))
    {
      return bad;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return indeterminate;
    }
    else
    {
      return bad;
    }
  case expecting_newline_3:
    return (input == '\n') ? good : bad;
  default:
    return bad;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

} // namespace server4
} // namespace http
// ---- merged server.cpp (the same program) ----
//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//


namespace http {
namespace server4 {

server::server(boost::asio::io_context& io_context,
    const std::string& address, const std::string& port,
    std::function<void(const request&, reply&)> request_handler)
  : request_handler_(request_handler)
{
  tcp::resolver resolver(io_context);
  boost::asio::ip::tcp::endpoint endpoint =
    *resolver.resolve(address, port).begin();
  acceptor_.reset(new tcp::acceptor(io_context, endpoint));
}

// Enable the pseudo-keywords reenter, yield and fork.
#include <boost/asio/yield.hpp>

void server::operator()(boost::system::error_code ec, std::size_t length)
{
  // In this example we keep the error handling code in one place by
  // hoisting it outside the coroutine. An alternative approach would be to
  // check the value of ec after each yield for an asynchronous operation.
  if (!ec)
  {
    // On reentering a coroutine, control jumps to the location of the last
    // yield or fork. The argument to the "reenter" pseudo-keyword can be a
    // pointer or reference to an object of type coroutine.
    reenter (this)
    {
      // Loop to accept incoming connections.
      do
      {
        // Create a new socket for the next incoming connection.
        socket_.reset(new tcp::socket(acceptor_->get_executor()));

        // Accept a new connection. The "yield" pseudo-keyword saves the current
        // line number and exits the coroutine's "reenter" block. We use the
        // server coroutine as the completion handler for the async_accept
        // operation. When the asynchronous operation completes, the io_context
        // invokes the function call operator, we "reenter" the coroutine, and
        // then control resumes at the following line.
        yield acceptor_->async_accept(*socket_, *this);

        // We "fork" by cloning a new server coroutine to handle the connection.
        // After forking we have a parent coroutine and a child coroutine. Both
        // parent and child continue execution at the following line. They can
        // be distinguished using the functions coroutine::is_parent() and
        // coroutine::is_child().
        fork server(*this)();

        // The parent continues looping to accept the next incoming connection.
        // The child exits the loop and processes the connection.
      } while (is_parent());

      // Create the objects needed to receive a request on the connection.
      buffer_.reset(new std::array<char, 8192>);
      request_.reset(new request);

      // Loop until a complete request (or an invalid one) has been received.
      do
      {
        // Receive some more data. When control resumes at the following line,
        // the ec and length parameters reflect the result of the asynchronous
        // operation.
        yield socket_->async_read_some(boost::asio::buffer(*buffer_), *this);

        // Parse the data we just received.
        std::tie(parse_result_, std::ignore)
          = request_parser_.parse(*request_,
              buffer_->data(), buffer_->data() + length);

        // An indeterminate result means we need more data, so keep looping.
      } while (parse_result_ == request_parser::indeterminate);

      // Create the reply object that will be sent back to the client.
      reply_.reset(new reply);

      if (parse_result_ == request_parser::good)
      {
        // A valid request was received. Call the user-supplied function object
        // to process the request and compose a reply.
        request_handler_(*request_, *reply_);
      }
      else
      {
        // The request was invalid.
        *reply_ = reply::stock_reply(reply::bad_request);
      }

      // Send the reply back to the client.
      yield boost::asio::async_write(*socket_, reply_->to_buffers(), *this);

      // Initiate graceful connection closure.
      socket_->shutdown(tcp::socket::shutdown_both, ec);
    }
  }

  // If an error occurs then the coroutine is not reentered. Consequently, no
  // new asynchronous operations are started. This means that all shared_ptr
  // references will disappear and the resources associated with the coroutine
  // will be destroyed automatically after this function call returns.
}

// Disable the pseudo-keywords reenter, yield and fork.
#include <boost/asio/unyield.hpp>

} // namespace server4
} // namespace http

TEST_CASE("example: cpp11/http/server4/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back(ex_root);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "GET /index.html HTTP/1.0\r\n\r\n", 0)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("received: HTTP/1.0 200 OK\r\n"
        "Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n", 108)));
}
}  // namespace ex_cpp11_http_server4_main

// libs/asio/example/cpp11/http/client/sync_client.cpp
namespace ex_cpp11_http_client_sync_client {
using boost::asio::ip::tcp;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: sync_client <server> <path>\n";
      ex_out << "Example:\n";
      ex_out << "  sync_client www.boost.org /LICENSE_1_0.txt\n";
      return 1;
    }

    boost::asio::io_context io_context;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints = resolver.resolve(argv[1], ex_port);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << argv[2] << " HTTP/1.0\r\n";
    request_stream << "Host: " << argv[1] << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
      ex_out << "Invalid response\n";
      return 1;
    }
    if (status_code != 200)
    {
      ex_out << "Response returned with status code " << status_code << "\n";
      return 1;
    }

    // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r")
      ex_out << header << "\n";
    ex_out << "\n";

    // Write whatever content we already have to output.
    if (response.size() > 0)
      ex_out << &response;

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;
    while (boost::asio::read(socket, response,
          boost::asio::transfer_at_least(1), error))
      ex_out << &response;
    if (error != boost::asio::error::eof)
      throw boost::system::system_error(error);
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/http/client/sync_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_http_server_main::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back("/index.html");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\n"
        "<html><body>boost-m</body></html>\n", 80)));
}
}  // namespace ex_cpp11_http_client_sync_client

// libs/asio/example/cpp11/http/client/async_client.cpp
namespace ex_cpp11_http_client_async_client {
using boost::asio::ip::tcp;

class client
{
public:
  client(boost::asio::io_context& io_context,
      const std::string& server, const std::string& path)
    : resolver_(io_context),
      socket_(io_context)
  {
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_);
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    resolver_.async_resolve(server, ex_port,
        std::bind(&client::handle_resolve, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::results));
  }

private:
  void handle_resolve(const boost::system::error_code& err,
      const tcp::resolver::results_type& endpoints)
  {
    if (!err)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
      boost::asio::async_connect(socket_, endpoints,
          std::bind(&client::handle_connect, this,
            boost::asio::placeholders::error));
    }
    else
    {
      ex_out << "Error: " << err.message() << "\n";
    }
  }

  void handle_connect(const boost::system::error_code& err)
  {
    if (!err)
    {
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, request_,
          std::bind(&client::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
      ex_out << "Error: " << err.message() << "\n";
    }
  }

  void handle_write_request(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Read the response status line. The response_ streambuf will
      // automatically grow to accommodate the entire line. The growth may be
      // limited by passing a maximum size to the streambuf constructor.
      boost::asio::async_read_until(socket_, response_, "\r\n",
          std::bind(&client::handle_read_status_line, this,
            boost::asio::placeholders::error));
    }
    else
    {
      ex_out << "Error: " << err.message() << "\n";
    }
  }

  void handle_read_status_line(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
        ex_out << "Invalid response\n";
        return;
      }
      if (status_code != 200)
      {
        ex_out << "Response returned with status code ";
        ex_out << status_code << "\n";
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          std::bind(&client::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      ex_out << "Error: " << err << "\n";
    }
  }

  void handle_read_headers(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        ex_out << header << "\n";
      ex_out << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0)
        ex_out << &response_;

      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          std::bind(&client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      ex_out << "Error: " << err << "\n";
    }
  }

  void handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Write all of the data that has been read so far.
      ex_out << &response_;

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          std::bind(&client::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      ex_out << "Error: " << err << "\n";
    }
  }

  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
};

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: async_client <server> <path>\n";
      ex_out << "Example:\n";
      ex_out << "  async_client www.boost.org /LICENSE_1_0.txt\n";
      return 1;
    }

    boost::asio::io_context io_context;
    client c(io_context, argv[1], argv[2]);
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/http/client/async_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_http_server_main::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back("/index.html");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\n"
        "<html><body>boost-m</body></html>\n", 80)));
}
}  // namespace ex_cpp11_http_client_async_client

// libs/asio/example/cpp11/iostreams/http_client.cpp
namespace ex_cpp11_iostreams_http_client {
using boost::asio::ip::tcp;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: http_client <server> <path>\n";
      ex_out << "Example:\n";
      ex_out << "  http_client www.boost.org /LICENSE_1_0.txt\n";
      return 1;
    }

    boost::asio::ip::tcp::iostream s;

    // The entire sequence of I/O operations must complete within 60 seconds.
    // If an expiry occurs, the socket is automatically closed and the stream
    // becomes bad.
    s.expires_after(std::chrono::seconds(60));

    // Establish a connection to the server.
    s.connect(argv[1], ex_port);
    if (!s)
    {
      ex_out << "Unable to connect: " << s.error().message() << "\n";
      return 1;
    }

    // Send the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    s << "GET " << argv[2] << " HTTP/1.0\r\n";
    s << "Host: " << argv[1] << "\r\n";
    s << "Accept: */*\r\n";
    s << "Connection: close\r\n\r\n";

    // By default, the stream is tied with itself. This means that the stream
    // automatically flush the buffered output before attempting a read. It is
    // not necessary not explicitly flush the stream at this point.

    // Check that response is OK.
    std::string http_version;
    s >> http_version;
    unsigned int status_code;
    s >> status_code;
    std::string status_message;
    std::getline(s, status_message);
    if (!s || http_version.substr(0, 5) != "HTTP/")
    {
      ex_out << "Invalid response\n";
      return 1;
    }
    if (status_code != 200)
    {
      ex_out << "Response returned with status code " << status_code << "\n";
      return 1;
    }

    // Process the response headers, which are terminated by a blank line.
    std::string header;
    while (std::getline(s, header) && header != "\r")
      ex_out << header << "\n";
    ex_out << "\n";

    // Write the remaining data to output.
    ex_out << s.rdbuf();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/iostreams/http_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_http_server_main::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back("/index.html");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\n"
        "<html><body>boost-m</body></html>\n", 80)));
}
}  // namespace ex_cpp11_iostreams_http_client

// libs/asio/example/cpp14/iostreams/http_client.cpp
namespace ex_cpp14_iostreams_http_client {
using boost::asio::ip::tcp;

int example_main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      ex_out << "Usage: http_client <server> <path>\n";
      ex_out << "Example:\n";
      ex_out << "  http_client www.boost.org /LICENSE_1_0.txt\n";
      return 1;
    }

    boost::asio::ip::tcp::iostream s;

    // The entire sequence of I/O operations must complete within 60 seconds.
    // If an expiry occurs, the socket is automatically closed and the stream
    // becomes bad.
    s.expires_after(std::chrono::seconds(60));

    // Establish a connection to the server.
    s.connect(argv[1], ex_port);
    if (!s)
    {
      ex_out << "Unable to connect: " << s.error().message() << "\n";
      return 1;
    }

    // Send the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    s << "GET " << argv[2] << " HTTP/1.0\r\n";
    s << "Host: " << argv[1] << "\r\n";
    s << "Accept: */*\r\n";
    s << "Connection: close\r\n\r\n";

    // By default, the stream is tied with itself. This means that the stream
    // automatically flush the buffered output before attempting a read. It is
    // not necessary not explicitly flush the stream at this point.

    // Check that response is OK.
    std::string http_version;
    s >> http_version;
    unsigned int status_code;
    s >> status_code;
    std::string status_message;
    std::getline(s, status_message);
    if (!s || http_version.substr(0, 5) != "HTTP/")
    {
      ex_out << "Invalid response\n";
      return 1;
    }
    if (status_code != 200)
    {
      ex_out << "Response returned with status code " << status_code << "\n";
      return 1;
    }

    // Process the response headers, which are terminated by a blank line.
    std::string header;
    while (std::getline(s, header) && header != "\r")
      ex_out << header << "\n";
    ex_out << "\n";

    // Write the remaining data to output.
    ex_out << s.rdbuf();
  }
  catch (std::exception& e)
  {
    ex_out << "Exception: " << e.what() << "\n";
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp14/iostreams/http_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_http_server_main::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back("/index.html");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(std::string_view("Content-Length: 34\r\n"
        "Content-Type: text/html\r\n"
        "\n"
        "<html><body>boost-m</body></html>\n", 80)));
}
}  // namespace ex_cpp14_iostreams_http_client
