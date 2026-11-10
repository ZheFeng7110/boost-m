// mcpplibs.mwrap.boost.asio — the official cpp11/serialization (libs/asio/example/cpp11/serialization/) examples that need a network
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
import mcpplibs.mwrap.boost.serialization;

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

// libs/asio/example/cpp11/serialization/server.cpp
namespace ex_cpp11_serialization_server {
#undef SERIALIZATION_CONNECTION_HPP
//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERIALIZATION_CONNECTION_HPP
#define SERIALIZATION_CONNECTION_HPP


namespace s11n_example {

/// The connection class provides serialization primitives on top of a socket.
/**
 * Each message sent using this class consists of:
 * @li An 8-byte header containing the length of the serialized data in
 * hexadecimal.
 * @li The serialized data.
 */
class connection
{
public:
  /// Constructor.
  connection(const boost::asio::any_io_executor& ex)
    : socket_(ex)
  {
  }

  /// Get the underlying socket. Used for making a connection or for accepting
  /// an incoming connection.
  boost::asio::ip::tcp::socket& socket()
  {
    return socket_;
  }

  /// Asynchronously write a data structure to the socket.
  template <typename T, typename Handler>
  void async_write(const T& t, Handler handler)
  {
    // Serialize the data first so we know how large it is.
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << t;
    outbound_data_ = archive_stream.str();

    // Format the header.
    std::ostringstream header_stream;
    header_stream << std::setw(header_length)
      << std::hex << outbound_data_.size();
    if (!header_stream || header_stream.str().size() != header_length)
    {
      // Something went wrong, inform the caller.
      boost::system::error_code error(boost::asio::error::invalid_argument);
      boost::asio::post(socket_.get_executor(), std::bind(handler, error));
      return;
    }
    outbound_header_ = header_stream.str();

    // Write the serialized data to the socket. We use "gather-write" to send
    // both the header and the data in a single write operation.
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(outbound_header_));
    buffers.push_back(boost::asio::buffer(outbound_data_));
    boost::asio::async_write(socket_, buffers, handler);
  }

  /// Asynchronously read a data structure from the socket.
  template <typename T, typename Handler>
  void async_read(T& t, Handler handler)
  {
    // Issue a read operation to read exactly the number of bytes in a header.
    void (connection::*f)(const boost::system::error_code&, T&, std::tuple<Handler>)
      = &connection::handle_read_header<T, Handler>;
    boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
        std::bind(f,
          this, boost::asio::placeholders::error, std::ref(t),
          std::make_tuple(handler)));
  }

  /// Handle a completed read of a message header.
  template <typename T, typename Handler>
  void handle_read_header(const boost::system::error_code& e,
      T& t, std::tuple<Handler> handler)
  {
    if (e)
    {
      std::get<0>(handler)(e);
    }
    else
    {
      // Determine the length of the serialized data.
      std::istringstream is(std::string(inbound_header_, header_length));
      std::size_t inbound_data_size = 0;
      if (!(is >> std::hex >> inbound_data_size))
      {
        // Header doesn't seem to be valid. Inform the caller.
        boost::system::error_code error(boost::asio::error::invalid_argument);
        std::get<0>(handler)(error);
        return;
      }

      // Start an asynchronous call to receive the data.
      inbound_data_.resize(inbound_data_size);
      void (connection::*f)(
          const boost::system::error_code&,
          T&, std::tuple<Handler>)
        = &connection::handle_read_data<T, Handler>;
      boost::asio::async_read(socket_, boost::asio::buffer(inbound_data_),
        std::bind(f, this,
          boost::asio::placeholders::error, std::ref(t), handler));
    }
  }

  /// Handle a completed read of message data.
  template <typename T, typename Handler>
  void handle_read_data(const boost::system::error_code& e,
      T& t, std::tuple<Handler> handler)
  {
    if (e)
    {
      std::get<0>(handler)(e);
    }
    else
    {
      // Extract the data structure from the data just received.
      try
      {
        std::string archive_data(&inbound_data_[0], inbound_data_.size());
        std::istringstream archive_stream(archive_data);
        boost::archive::text_iarchive archive(archive_stream);
        archive >> t;
      }
      catch (std::exception& e)
      {
        // Unable to decode data.
        boost::system::error_code error(boost::asio::error::invalid_argument);
        std::get<0>(handler)(error);
        return;
      }

      // Inform caller that data has been received ok.
      std::get<0>(handler)(e);
    }
  }

private:
  /// The underlying socket.
  boost::asio::ip::tcp::socket socket_;

  /// The size of a fixed length header.
  enum { header_length = 8 };

  /// Holds an outbound header.
  std::string outbound_header_;

  /// Holds the outbound data.
  std::string outbound_data_;

  /// Holds an inbound header.
  char inbound_header_[header_length];

  /// Holds the inbound data.
  std::vector<char> inbound_data_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace s11n_example

#endif // SERIALIZATION_CONNECTION_HPP
// ---- end connection.hpp ----
// ---- inlined stock.hpp ----
#undef SERIALIZATION_STOCK_HPP
//
// stock.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERIALIZATION_STOCK_HPP
#define SERIALIZATION_STOCK_HPP


namespace s11n_example {

/// Structure to hold information about a single stock.
struct stock
{
  std::string code;
  std::string name;
  double open_price;
  double high_price;
  double low_price;
  double last_price;
  double buy_price;
  int buy_quantity;
  double sell_price;
  int sell_quantity;

  template <typename Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    ar & code;
    ar & name;
    ar & open_price;
    ar & high_price;
    ar & low_price;
    ar & last_price;
    ar & buy_price;
    ar & buy_quantity;
    ar & sell_price;
    ar & sell_quantity;
  }
};

} // namespace s11n_example

#endif // SERIALIZATION_STOCK_HPP
// ---- end stock.hpp ----

namespace s11n_example {

/// Serves stock quote information to any client that connects to it.
class server
{
public:
  /// Constructor opens the acceptor and starts waiting for the first incoming
  /// connection.
  server(boost::asio::io_context& io_context, unsigned short port)
    : acceptor_(io_context,
        boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
  {
    // Create the data to be sent to each client.
    stock s;
    s.code = "ABC";
    s.name = "A Big Company";
    s.open_price = 4.56;
    s.high_price = 5.12;
    s.low_price = 4.33;
    s.last_price = 4.98;
    s.buy_price = 4.96;
    s.buy_quantity = 1000;
    s.sell_price = 4.99;
    s.sell_quantity = 2000;
    stocks_.push_back(s);
    s.code = "DEF";
    s.name = "Developer Entertainment Firm";
    s.open_price = 20.24;
    s.high_price = 22.88;
    s.low_price = 19.50;
    s.last_price = 19.76;
    s.buy_price = 19.72;
    s.buy_quantity = 34000;
    s.sell_price = 19.85;
    s.sell_quantity = 45000;
    stocks_.push_back(s);

    // Start an accept operation for a new connection.
    connection_ptr new_conn(new connection(acceptor_.get_executor()));
    acceptor_.async_accept(new_conn->socket(),
        std::bind(&server::handle_accept, this,
          boost::asio::placeholders::error, new_conn));
  }

  /// Handle completion of a accept operation.
  void handle_accept(const boost::system::error_code& e, connection_ptr conn)
  {
    if (!e)
    {
      // Successfully accepted a new connection. Send the list of stocks to the
      // client. The connection::async_write() function will automatically
      // serialize the data structure for us.
      conn->async_write(stocks_,
          std::bind(&server::handle_write, this,
            boost::asio::placeholders::error, conn));
    }

    // Start an accept operation for a new connection.
    connection_ptr new_conn(new connection(acceptor_.get_executor()));
    acceptor_.async_accept(new_conn->socket(),
        std::bind(&server::handle_accept, this,
          boost::asio::placeholders::error, new_conn));
  }

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e, connection_ptr conn)
  {
    // Nothing to do. The socket will be closed automatically when the last
    // reference to the connection object goes away.
  }

private:
  /// The acceptor object used to accept incoming socket connections.
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The data to be sent to each client.
  std::vector<stock> stocks_;
};

} // namespace s11n_example

int example_main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 2)
    {
      ex_out << "Usage: server <port>" << std::endl;
      return 1;
    }
    unsigned short port = std::stoi(argv[1]);

    boost::asio::io_context io_context;
    s11n_example::server server(io_context, port);
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/serialization/server") {
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
    CHECK(ex_out.str() == std::string_view(R"ex(received: text
)ex"));
}
}  // namespace ex_cpp11_serialization_server

// libs/asio/example/cpp11/serialization/client.cpp
namespace ex_cpp11_serialization_client {
#undef SERIALIZATION_CONNECTION_HPP
//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERIALIZATION_CONNECTION_HPP
#define SERIALIZATION_CONNECTION_HPP


namespace s11n_example {

/// The connection class provides serialization primitives on top of a socket.
/**
 * Each message sent using this class consists of:
 * @li An 8-byte header containing the length of the serialized data in
 * hexadecimal.
 * @li The serialized data.
 */
class connection
{
public:
  /// Constructor.
  connection(const boost::asio::any_io_executor& ex)
    : socket_(ex)
  {
  }

  /// Get the underlying socket. Used for making a connection or for accepting
  /// an incoming connection.
  boost::asio::ip::tcp::socket& socket()
  {
    return socket_;
  }

  /// Asynchronously write a data structure to the socket.
  template <typename T, typename Handler>
  void async_write(const T& t, Handler handler)
  {
    // Serialize the data first so we know how large it is.
    std::ostringstream archive_stream;
    boost::archive::text_oarchive archive(archive_stream);
    archive << t;
    outbound_data_ = archive_stream.str();

    // Format the header.
    std::ostringstream header_stream;
    header_stream << std::setw(header_length)
      << std::hex << outbound_data_.size();
    if (!header_stream || header_stream.str().size() != header_length)
    {
      // Something went wrong, inform the caller.
      boost::system::error_code error(boost::asio::error::invalid_argument);
      boost::asio::post(socket_.get_executor(), std::bind(handler, error));
      return;
    }
    outbound_header_ = header_stream.str();

    // Write the serialized data to the socket. We use "gather-write" to send
    // both the header and the data in a single write operation.
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(outbound_header_));
    buffers.push_back(boost::asio::buffer(outbound_data_));
    boost::asio::async_write(socket_, buffers, handler);
  }

  /// Asynchronously read a data structure from the socket.
  template <typename T, typename Handler>
  void async_read(T& t, Handler handler)
  {
    // Issue a read operation to read exactly the number of bytes in a header.
    void (connection::*f)(const boost::system::error_code&, T&, std::tuple<Handler>)
      = &connection::handle_read_header<T, Handler>;
    boost::asio::async_read(socket_, boost::asio::buffer(inbound_header_),
        std::bind(f,
          this, boost::asio::placeholders::error, std::ref(t),
          std::make_tuple(handler)));
  }

  /// Handle a completed read of a message header.
  template <typename T, typename Handler>
  void handle_read_header(const boost::system::error_code& e,
      T& t, std::tuple<Handler> handler)
  {
    if (e)
    {
      std::get<0>(handler)(e);
    }
    else
    {
      // Determine the length of the serialized data.
      std::istringstream is(std::string(inbound_header_, header_length));
      std::size_t inbound_data_size = 0;
      if (!(is >> std::hex >> inbound_data_size))
      {
        // Header doesn't seem to be valid. Inform the caller.
        boost::system::error_code error(boost::asio::error::invalid_argument);
        std::get<0>(handler)(error);
        return;
      }

      // Start an asynchronous call to receive the data.
      inbound_data_.resize(inbound_data_size);
      void (connection::*f)(
          const boost::system::error_code&,
          T&, std::tuple<Handler>)
        = &connection::handle_read_data<T, Handler>;
      boost::asio::async_read(socket_, boost::asio::buffer(inbound_data_),
        std::bind(f, this,
          boost::asio::placeholders::error, std::ref(t), handler));
    }
  }

  /// Handle a completed read of message data.
  template <typename T, typename Handler>
  void handle_read_data(const boost::system::error_code& e,
      T& t, std::tuple<Handler> handler)
  {
    if (e)
    {
      std::get<0>(handler)(e);
    }
    else
    {
      // Extract the data structure from the data just received.
      try
      {
        std::string archive_data(&inbound_data_[0], inbound_data_.size());
        std::istringstream archive_stream(archive_data);
        boost::archive::text_iarchive archive(archive_stream);
        archive >> t;
      }
      catch (std::exception& e)
      {
        // Unable to decode data.
        boost::system::error_code error(boost::asio::error::invalid_argument);
        std::get<0>(handler)(error);
        return;
      }

      // Inform caller that data has been received ok.
      std::get<0>(handler)(e);
    }
  }

private:
  /// The underlying socket.
  boost::asio::ip::tcp::socket socket_;

  /// The size of a fixed length header.
  enum { header_length = 8 };

  /// Holds an outbound header.
  std::string outbound_header_;

  /// Holds the outbound data.
  std::string outbound_data_;

  /// Holds an inbound header.
  char inbound_header_[header_length];

  /// Holds the inbound data.
  std::vector<char> inbound_data_;
};

typedef std::shared_ptr<connection> connection_ptr;

} // namespace s11n_example

#endif // SERIALIZATION_CONNECTION_HPP
// ---- end connection.hpp ----
// ---- inlined stock.hpp ----
#undef SERIALIZATION_STOCK_HPP
//
// stock.hpp
// ~~~~~~~~~
//
// Copyright (c) 2003-2026 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef SERIALIZATION_STOCK_HPP
#define SERIALIZATION_STOCK_HPP


namespace s11n_example {

/// Structure to hold information about a single stock.
struct stock
{
  std::string code;
  std::string name;
  double open_price;
  double high_price;
  double low_price;
  double last_price;
  double buy_price;
  int buy_quantity;
  double sell_price;
  int sell_quantity;

  template <typename Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    ar & code;
    ar & name;
    ar & open_price;
    ar & high_price;
    ar & low_price;
    ar & last_price;
    ar & buy_price;
    ar & buy_quantity;
    ar & sell_price;
    ar & sell_quantity;
  }
};

} // namespace s11n_example

#endif // SERIALIZATION_STOCK_HPP
// ---- end stock.hpp ----

namespace s11n_example {

/// Downloads stock quote information from a server.
class client
{
public:
  /// Constructor starts the asynchronous connect operation.
  client(boost::asio::io_context& io_context,
      const std::string& host, const std::string& service)
    : connection_(io_context.get_executor())
  {
    // Resolve the host name into a sequence of endpoints.
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve(host, service);

    // Start an asynchronous connect operation.
    boost::asio::async_connect(connection_.socket(), endpoints,
        std::bind(&client::handle_connect, this,
          boost::asio::placeholders::error));
  }

  /// Handle completion of a connect operation.
  void handle_connect(const boost::system::error_code& e)
  {
    if (!e)
    {
      // Successfully established connection. Start operation to read the list
      // of stocks. The connection::async_read() function will automatically
      // decode the data that is read from the underlying socket.
      connection_.async_read(stocks_,
          std::bind(&client::handle_read, this,
            boost::asio::placeholders::error));
    }
    else
    {
      // An error occurred. Log it and return. Since we are not starting a new
      // operation the io_context will run out of work to do and the client will
      // exit.
      ex_out << e.message() << std::endl;
    }
  }

  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& e)
  {
    if (!e)
    {
      // Print out the data that was received.
      for (std::size_t i = 0; i < stocks_.size(); ++i)
      {
        ex_out << "Stock number " << i << "\n";
        ex_out << "  code: " << stocks_[i].code << "\n";
        ex_out << "  name: " << stocks_[i].name << "\n";
        ex_out << "  open_price: " << stocks_[i].open_price << "\n";
        ex_out << "  high_price: " << stocks_[i].high_price << "\n";
        ex_out << "  low_price: " << stocks_[i].low_price << "\n";
        ex_out << "  last_price: " << stocks_[i].last_price << "\n";
        ex_out << "  buy_price: " << stocks_[i].buy_price << "\n";
        ex_out << "  buy_quantity: " << stocks_[i].buy_quantity << "\n";
        ex_out << "  sell_price: " << stocks_[i].sell_price << "\n";
        ex_out << "  sell_quantity: " << stocks_[i].sell_quantity << "\n";
      }
    }
    else
    {
      // An error occurred.
      ex_out << e.message() << std::endl;
    }

    // Since we are not starting a new operation the io_context will run out of
    // work to do and the client will exit.
  }

private:
  /// The connection to the server.
  connection connection_;

  /// The data received from the server.
  std::vector<stock> stocks_;
};

} // namespace s11n_example

int example_main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 3)
    {
      ex_out << "Usage: client <host> <port>" << std::endl;
      return 1;
    }

    boost::asio::io_context io_context;
    s11n_example::client client(io_context, argv[1], argv[2]);
    io_context.run();
  }
  catch (std::exception& e)
  {
    ex_out << e.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: cpp11/serialization/client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_cpp11_serialization_server::example_main, std::vector<std::string>{"server", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Stock number 0
  code: ABC
  name: A Big Company
  open_price: 4.56
  high_price: 5.12
  low_price: 4.33
  last_price: 4.98
  buy_price: 4.96
  buy_quantity: 1000
  sell_price: 4.99
  sell_quantity: 2000
Stock number 1
  code: DEF
  name: Developer Entertainment Firm
  open_price: 20.24
  high_price: 22.88
  low_price: 19.5
  last_price: 19.76
  buy_price: 19.72
  buy_quantity: 34000
  sell_price: 19.85
  sell_quantity: 45000
)ex"));
}
}  // namespace ex_cpp11_serialization_client
