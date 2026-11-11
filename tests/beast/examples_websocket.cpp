// mcpplibs.mwrap.boost.beast — the official WebSocket examples
// (libs/beast/example/), run over the loopback interface in one process. A
// server example runs in a background thread on a free port (ex_serve_async)
// and a harness client below — itself written with Beast — sends one request
// (HTTP GET of a document root's index.html, or one WebSocket message) and the
// case output is what it received. A client example runs against the ported
// server example it belongs with, started the same way. The expected output
// is what the same code printed when built from upstream/boost headers alone.
// Response headers that carry the build (Server: Boost.Beast/<version>) or the
// time are left out by the harness.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cstdlib>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/coroutine.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/throw_exception.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <cstddef>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.beast;

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
// ---- beast harness clients ----
// HTTP/1.1 GET over TCP: prints the status line, the Content-Type and the
// body of the response (Server / Date headers vary between builds).
inline int ex_http_get(const std::string& port, const std::string& target) {
    namespace http = boost::beast::http;
    auto& c = *new boost::asio::io_context;  // kept open (see ex_tcp_exchange)
    auto& s = *new boost::asio::ip::tcp::socket(c);
    if (!ex_try_connect(s, port)) { ex_out << "connect failed\n"; return 1; }
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "127.0.0.1");
    http::write(s, req);
    boost::beast::flat_buffer b;
    http::response<http::string_body> res;
    http::read(s, b, res);
    ex_out << "received: " << res.result_int() << " " << res.reason() << "\n"
           << "Content-Type: " << res[http::field::content_type] << "\n" << res.body();
    return 0;
}
// WebSocket: handshake on "/", send `text`, print the first message back.
inline int ex_ws_exchange(const std::string& port, const std::string& text) {
    namespace websocket = boost::beast::websocket;
    auto& c = *new boost::asio::io_context;
    auto& ws = *new websocket::stream<boost::asio::ip::tcp::socket>(c);
    if (!ex_try_connect(ws.next_layer(), port)) { ex_out << "connect failed\n"; return 1; }
    ws.handshake("127.0.0.1:" + port, "/");
    ws.write(boost::asio::buffer(text));
    boost::beast::flat_buffer b;
    ws.read(b);
    ex_out << "received: " << boost::beast::buffers_to_string(b.data()) << "\n";
    return 0;
}
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
inline int ex_http_get_local(const std::string& path, const std::string& target) {
    namespace http = boost::beast::http;
    auto& c = *new boost::asio::io_context;
    auto& s = *new boost::asio::local::stream_protocol::socket(c);
    s.connect(boost::asio::local::stream_protocol::endpoint(path));
    http::request<http::string_body> req{http::verb::get, target, 11};
    req.set(http::field::host, "localhost");
    http::write(s, req);
    boost::beast::flat_buffer b;
    http::response<http::string_body> res;
    http::read(s, b, res);
    ex_out << "received: " << res.result_int() << " " << res.reason() << "\n"
           << "Content-Type: " << res[http::field::content_type] << "\n" << res.body();
    return 0;
}
inline int ex_ws_exchange_local(const std::string& path, const std::string& text) {
    namespace websocket = boost::beast::websocket;
    auto& c = *new boost::asio::io_context;
    auto& ws = *new websocket::stream<boost::asio::local::stream_protocol::socket>(c);
    ws.next_layer().connect(boost::asio::local::stream_protocol::endpoint(path));
    ws.handshake("localhost", "/");
    ws.write(boost::asio::buffer(text));
    boost::beast::flat_buffer b;
    ws.read(b);
    ex_out << "received: " << boost::beast::buffers_to_string(b.data()) << "\n";
    return 0;
}
#else
inline int ex_http_get_local(const std::string&, const std::string&) { return 0; }
inline int ex_ws_exchange_local(const std::string&, const std::string&) { return 0; }
#endif

// libs/beast/example/websocket/server/sync/websocket_server_sync.cpp
namespace ex_websocket_server_sync_websocket_server_sync {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Echoes back all received WebSocket messages
void
do_session(tcp::socket socket)
{
    try
    {
        // Construct the stream by moving in the socket
        websocket::stream<tcp::socket> ws{std::move(socket)};

        // Set a decorator to change the Server of the handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-sync");
            }));

        // Accept the websocket handshake
        ws.accept();

        for(;;)
        {
            // This buffer will hold the incoming message
            beast::flat_buffer buffer;

            // Read a message
            ws.read(buffer);

            // Echo the message back
            ws.text(ws.got_text());
            ws.write(buffer.data());
        }
    }
    catch(beast::system_error const& se)
    {
        // This indicates that the session was closed
        if(se.code() != websocket::error::closed)
            ex_out << "Error: " << se.code().message() << std::endl;
    }
    catch(std::exception const& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
    }
}

//------------------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 3)
        {
            ex_out <<
                "Usage: websocket-server-sync <address> <port>\n" <<
                "Example:\n" <<
                "    websocket-server-sync 0.0.0.0 8080\n";
            return EXIT_FAILURE;
        }
        auto const address = net::ip::make_address(argv[1]);
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

        // The io_context is required for all I/O
        net::io_context ioc{1};

        // The acceptor receives incoming connections
        tcp::acceptor acceptor{ioc, {address, port}};
        for(;;)
        {
            // This will receive the new connection
            tcp::socket socket{ioc};

            // Block until we get a connection
            acceptor.accept(socket);

            // Launch the session, transferring ownership of the socket
            std::thread(
                &do_session,
                std::move(socket)).detach();
        }
    }
    catch (const std::exception& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: websocket/server/sync/websocket_server_sync") {
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
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_ws_exchange(ex_port, "hello beast")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello beast
)ex"));
}
}  // namespace ex_websocket_server_sync_websocket_server_sync

// libs/beast/example/websocket/server/async/websocket_server_async.cpp
namespace ex_websocket_server_async_websocket_server_async {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    // Take ownership of the socket
    explicit
    session(tcp::socket&& socket)
        : ws_(std::move(socket))
    {
    }

    // Get on the correct executor
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(
                &session::on_run,
                shared_from_this()));
    }

    // Start the asynchronous operation
    void
    on_run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            return fail(ec, "read");

        // Echo the message
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(ioc)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        do_accept();
    }

private:
    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, tcp::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        ex_out <<
            "Usage: websocket-server-async <address> <port> <threads>\n" <<
            "Example:\n" <<
            "    websocket-server-async 0.0.0.0 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: websocket/server/async/websocket_server_async") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_ws_exchange(ex_port, "hello beast")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello beast
)ex"));
}
}  // namespace ex_websocket_server_async_websocket_server_async

// libs/beast/example/websocket/server/stackless/websocket_server_stackless.cpp
namespace ex_websocket_server_stackless_websocket_server_stackless {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class session
    : public boost::asio::coroutine
    , public std::enable_shared_from_this<session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    // Take ownership of the socket
    explicit
    session(tcp::socket socket)
        : ws_(std::move(socket))
    {
    }

    // Start the asynchronous operation
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
                      beast::bind_front_handler(&session::loop,
                                                shared_from_this(),
                                                beast::error_code{},
                                                0));
    }

#include <boost/asio/yield.hpp>

    void
    loop(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        reenter(*this)
        {
            // Set suggested timeout settings for the websocket
            ws_.set_option(
                websocket::stream_base::timeout::suggested(
                    beast::role_type::server));

            // Set a decorator to change the Server of the handshake
            ws_.set_option(websocket::stream_base::decorator(
                [](websocket::response_type& res)
                {
                    res.set(http::field::server,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                            " websocket-server-stackless");
                }));

            // Accept the websocket handshake
            yield ws_.async_accept(
                std::bind(
                    &session::loop,
                    shared_from_this(),
                    std::placeholders::_1,
                    0));
            if(ec)
                return fail(ec, "accept");

            for(;;)
            {
                // Read a message into our buffer
                yield ws_.async_read(
                    buffer_,
                    std::bind(
                        &session::loop,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
                if(ec == websocket::error::closed)
                {
                    // This indicates that the session was closed
                    return;
                }
                if(ec)
                    return fail(ec, "read");

                // Echo the message
                ws_.text(ws_.got_text());
                yield ws_.async_write(
                    buffer_.data(),
                    std::bind(
                        &session::loop,
                        shared_from_this(),
                        std::placeholders::_1,
                        std::placeholders::_2));
                if(ec)
                    return fail(ec, "write");

                // Clear the buffer
                buffer_.consume(buffer_.size());
            }
        }
    }

#include <boost/asio/unyield.hpp>
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener
    : public boost::asio::coroutine
    , public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , socket_(net::make_strand(ioc))
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        loop();
    }

private:

#include <boost/asio/yield.hpp>

    void
    loop(beast::error_code ec = {})
    {
        reenter(*this)
        {
            for(;;)
            {
                yield acceptor_.async_accept(
                    socket_,
                    std::bind(
                        &listener::loop,
                        shared_from_this(),
                        std::placeholders::_1));
                if(ec)
                {
                    fail(ec, "accept");
                }
                else
                {
                    // Create the session and run it
                    std::make_shared<session>(std::move(socket_))->run();
                }

                // Make sure each session gets its own strand
                socket_ = tcp::socket(net::make_strand(ioc_));
            }
        }
    }

#include <boost/asio/unyield.hpp>
};

//------------------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        ex_out <<
            "Usage: websocket-server-stackless <address> <port> <threads>\n" <<
            "Example:\n" <<
            "    websocket-server-stackless 0.0.0.0 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: websocket/server/stackless/websocket_server_stackless") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_ws_exchange(ex_port, "hello beast")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello beast
)ex"));
}
}  // namespace ex_websocket_server_stackless_websocket_server_stackless

// libs/beast/example/websocket/server/awaitable/websocket_server_awaitable.cpp
namespace ex_websocket_server_awaitable_websocket_server_awaitable {
#if defined(BOOST_ASIO_HAS_CO_AWAIT)

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;

// Echoes back all received WebSocket messages
net::awaitable<void>
do_session(websocket::stream<beast::tcp_stream> stream)
{
    // Set suggested timeout settings for the websocket
    stream.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    stream.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(
                http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-server-coro");
        }));

    // Accept the websocket handshake
    co_await stream.async_accept();

    for(;;)
    {
        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message
        auto [ec, _] = co_await stream.async_read(buffer, net::as_tuple);

        if(ec == websocket::error::closed)
            co_return;

        if(ec)
            throw boost::system::system_error{ ec };

        // Echo the message back
        stream.text(stream.got_text());
        co_await stream.async_write(buffer.data());
    }
}

// Accepts incoming connections and launches the sessions
net::awaitable<void>
do_listen(net::ip::tcp::endpoint endpoint)
{
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor{ executor, endpoint };

    for(;;)
    {
        net::co_spawn(
            executor,
            do_session(websocket::stream<beast::tcp_stream>{
                co_await acceptor.async_accept() }),
            [](std::exception_ptr e)
            {
                if(e)
                {
                    try
                    {
                        std::rethrow_exception(e);
                    }
                    catch(std::exception& e)
                    {
                        ex_out << "Error in session: " << e.what() << "\n";
                    }
                }
            });
    }
}

int example_main(int argc, char* argv[])
{
    // Check command line arguments.
    if(argc != 4)
    {
        ex_out
            << "Usage: websocket-server-awaitable <address> <port> <threads>\n"
            << "Example:\n"
            << "    websocket-server-awaitable 0.0.0.0 8080 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port    = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    net::io_context ioc(threads);

    // Spawn a listening port
    net::co_spawn(
        ioc,
        do_listen(net::ip::tcp::endpoint{ address, port }),
        [](std::exception_ptr e)
        {
            if(e)
            {
                try
                {
                    std::rethrow_exception(e);
                }
                catch(std::exception const& e)
                {
                    ex_out << "Error: " << e.what() << std::endl;
                }
            }
        });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc] { ioc.run(); });
    ioc.run();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

#else

int example_main(int, char*[])
{
    ex_printf("awaitables require C++20\n");
    return EXIT_FAILURE;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: websocket/server/awaitable/websocket_server_awaitable") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_ws_exchange(ex_port, "hello beast")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello beast
)ex"));
}
}  // namespace ex_websocket_server_awaitable_websocket_server_awaitable

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/beast/example/websocket/server/async-local/websocket_server_async_local.cpp
namespace ex_websocket_server_async_local_websocket_server_async_local {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace beast = boost::beast;                      // from <boost/beast.hpp>
namespace http = beast::http;                        // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;              // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                         // from <boost/asio.hpp>
using stream_protocol = net::local::stream_protocol; // from <boost/asio/local/stream_protocol.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<stream_protocol::socket> ws_;
    beast::flat_buffer buffer_;

public:
    // Take ownership of the socket
    explicit
    session(stream_protocol::socket&& socket)
        : ws_(std::move(socket))
    {
    }

    // Get on the correct executor
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(ws_.get_executor(),
            beast::bind_front_handler(
                &session::on_run,
                shared_from_this()));
    }

    // Start the asynchronous operation
    void
    on_run()
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-server-async-local");
            }));
        // Accept the websocket handshake
        ws_.async_accept(
            beast::bind_front_handler(
                &session::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            return fail(ec, "read");

        // Echo the message
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    stream_protocol::acceptor acceptor_;

public:
    listener(
        net::io_context& ioc,
        stream_protocol::endpoint endpoint)
        : ioc_(ioc)
        , acceptor_(ioc, std::move(endpoint))
    {
    }

    // Start accepting incoming connections
    void
    run()
    {
        do_accept();
    }

private:
    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, stream_protocol::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
            return;
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 3)
        {
            ex_out <<
                "Usage: websocket-server-async-local <path> <threads>\n" <<
                "Example:\n" <<
                "    websocket-server-async-local /tmp/ws.sock 1\n";
            return EXIT_FAILURE;
        }
        auto const path = argv[1];
        auto const threads = std::max<int>(1, std::atoi(argv[2]));

        // The io_context is required for all I/O
        net::io_context ioc{threads};

        // Remove previous binding
        std::remove(path);

        // Create and launch a listening port
        std::make_shared<listener>(ioc, stream_protocol::endpoint{path})->run();

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(threads - 1);
        for(auto i = threads - 1; i > 0; --i)
            v.emplace_back(
            [&ioc]
            {
                ioc.run();
            });
        ioc.run();

        return EXIT_SUCCESS;
    }
    catch(std::exception const& e)
    {
        ex_out << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;  // main's implicit return
}

#else

int example_main(int, char*[])
{
    ex_out <<
        "Local sockets not available on this platform" << std::endl;
    return EXIT_FAILURE;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: websocket/server/async-local/websocket_server_async_local") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_local_path("beast.sock");
    ex_args.push_back(ex_port);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_ws_exchange_local(ex_port, "hello beast")));
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello beast
)ex"));
#endif
}
}  // namespace ex_websocket_server_async_local_websocket_server_async_local
#endif

// libs/beast/example/websocket/client/sync/websocket_client_sync.cpp
namespace ex_websocket_client_sync_websocket_client_sync {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Sends a WebSocket message and prints the response
int example_main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if(argc != 4)
        {
            ex_out <<
                "Usage: websocket-client-sync <host> <port> <text>\n" <<
                "Example:\n" <<
                "    websocket-client-sync echo.websocket.org 80 \"Hello, world!\"\n";
            return EXIT_FAILURE;
        }
        std::string host = argv[1];
        auto const  port = argv[2];
        auto const  text = argv[3];

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver{ioc};
        websocket::stream<tcp::socket> ws{ioc};

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        auto ep = net::connect(ws.next_layer(), results);

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host += ':' + std::to_string(ep.port());

        // Set a decorator to change the User-Agent of the handshake
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
            }));

        // Perform the websocket handshake
        ws.handshake(host, "/");

        // Send the message
        ws.write(net::buffer(std::string(text)));

        // This buffer will hold the incoming message
        beast::flat_buffer buffer;

        // Read a message into our buffer
        ws.read(buffer);

        // Close the WebSocket connection
        ws.close(websocket::close_code::normal);

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        ex_out << beast::make_printable(buffer.data()) << std::endl;
    }
    catch(std::exception const& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: websocket/client/sync/websocket_client_sync") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_websocket_server_sync_websocket_server_sync::example_main, std::vector<std::string>{"server", "127.0.0.1", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("hello beast");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(hello beast
)ex"));
}
}  // namespace ex_websocket_client_sync_websocket_client_sync

// libs/beast/example/websocket/client/async/websocket_client_async.cpp
namespace ex_websocket_client_async_websocket_client_async {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;

public:
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc))
        , ws_(net::make_strand(ioc))
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* text)
    {
        // Save these for later
        host_ = host;
        text_ = text;

        // Look up the domain name
        resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                &session::on_resolve,
                shared_from_this()));
    }

    void
    on_resolve(
        beast::error_code ec,
        tcp::resolver::results_type results)
    {
        if(ec)
            return fail(ec, "resolve");

        // Set the timeout for the operation
        beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep)
    {
        if(ec)
            return fail(ec, "connect");

        // Turn off the timeout on the tcp_stream, because
        // the websocket stream has its own timeout system.
        beast::get_lowest_layer(ws_).expires_never();

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async");
            }));

        // Update the host_ string. This will provide the value of the
        // Host HTTP header during the WebSocket handshake.
        // See https://tools.ietf.org/html/rfc7230#section-5.4
        host_ += ':' + std::to_string(ep.port());

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void
    on_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");
        
        // Send the message
        ws_.async_write(
            net::buffer(text_),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");
        
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));
    }

    void
    on_close(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        ex_out << beast::make_printable(buffer_.data()) << std::endl;
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc != 4)
    {
        ex_out <<
            "Usage: websocket-client-async <host> <port> <text>\n" <<
            "Example:\n" <<
            "    websocket-client-async echo.websocket.org 80 \"Hello, world!\"\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const text = argv[3];

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run(host, port, text);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: websocket/client/async/websocket_client_async") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_websocket_server_sync_websocket_server_sync::example_main, std::vector<std::string>{"server", "127.0.0.1", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("hello beast");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(hello beast
)ex"));
}
}  // namespace ex_websocket_client_async_websocket_client_async

// libs/beast/example/websocket/client/awaitable/websocket_client_awaitable.cpp
namespace ex_websocket_client_awaitable_websocket_client_awaitable {
#if defined(BOOST_ASIO_HAS_CO_AWAIT)

namespace beast     = boost::beast;
namespace http      = beast::http;
namespace websocket = beast::websocket;
namespace net       = boost::asio;

// Sends a WebSocket message and prints the response
net::awaitable<void>
do_session(std::string host, std::string port, std::string text)
{
    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{ executor };
    auto stream   = websocket::stream<beast::tcp_stream>{ executor };

    // Look up the domain name
    auto const results = co_await resolver.async_resolve(host, port);

    // Set a timeout on the operation
    beast::get_lowest_layer(stream).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    auto ep = co_await beast::get_lowest_layer(stream).async_connect(results);

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host += ':' + std::to_string(ep.port());

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(stream).expires_never();

    // Set suggested timeout settings for the websocket
    stream.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    stream.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(
                http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    // Perform the websocket handshake
    co_await stream.async_handshake(host, "/");

    // Send the message
    co_await stream.async_write(net::buffer(text));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    // Read a message into our buffer
    co_await stream.async_read(buffer);

    // Close the WebSocket connection
    co_await stream.async_close(websocket::close_code::normal);

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
    ex_out << beast::make_printable(buffer.data()) << std::endl;
}

//------------------------------------------------------------------------------

int example_main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if(argc != 4)
        {
            ex_out
                << "Usage: websocket-client-awaitable <host> <port> <text>\n"
                << "Example:\n"
                << "    websocket-client-awaitable echo.websocket.org 80 \"Hello, world!\"\n";
            return EXIT_FAILURE;
        }
        auto const host = argv[1];
        auto const port = argv[2];
        auto const text = argv[3];

        // The io_context is required for all I/O
        net::io_context ioc;

        // Launch the asynchronous operation
        net::co_spawn(
            ioc,
            do_session(host, port, text),
            [](std::exception_ptr e)
            {
                if(e)
                    std::rethrow_exception(e);
            });

        // Run the I/O service. The call will return when
        // the socket is closed.
        ioc.run();
    }
    catch(std::exception const& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

#else

int example_main(int, char*[])
{
    ex_printf("awaitables require C++20\n");
    return EXIT_FAILURE;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: websocket/client/awaitable/websocket_client_awaitable") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_websocket_server_sync_websocket_server_sync::example_main, std::vector<std::string>{"server", "127.0.0.1", p}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("hello beast");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(hello beast
)ex"));
}
}  // namespace ex_websocket_client_awaitable_websocket_client_awaitable

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/beast/example/websocket/client/async-local/websocket_client_async_local.cpp
namespace ex_websocket_client_async_local_websocket_client_async_local {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace beast = boost::beast;                      // from <boost/beast.hpp>
namespace http = beast::http;                        // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;              // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                         // from <boost/asio.hpp>
using stream_protocol = net::local::stream_protocol; // from <boost/asio/local/stream_protocol.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Sends a WebSocket message and prints the response
class session : public std::enable_shared_from_this<session>
{
    websocket::stream<stream_protocol::socket> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
    std::string text_;

public:
    // Resolver and socket require an io_context
    explicit
    session(net::io_context& ioc)
        : ws_(net::make_strand(ioc))
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* path,
        char const* host,
        char const* port,
        char const* text)
    {
        // Save for later
        host_ = std::string{host} + ":" + port;
        text_ = text;

        // Make the connection
        beast::get_lowest_layer(ws_).async_connect(
            stream_protocol::endpoint{path},
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "connect");

        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::client));

        // Set a decorator to change the User-Agent of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-local");
            }));

        // Perform the websocket handshake
        ws_.async_handshake(host_, "/",
            beast::bind_front_handler(
                &session::on_handshake,
                shared_from_this()));
    }

    void
    on_handshake(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "handshake");
        
        // Send the message
        ws_.async_write(
            net::buffer(text_),
            beast::bind_front_handler(
                &session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");
        
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "read");

        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
            beast::bind_front_handler(
                &session::on_close,
                shared_from_this()));
    }

    void
    on_close(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "close");

        // If we get here then the connection is closed gracefully

        // The make_printable() function helps print a ConstBufferSequence
        ex_out << beast::make_printable(buffer_.data()) << std::endl;
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc != 5)
    {
        ex_out <<
            "Usage: websocket-client-async-local <path> <host> <port> <text>\n" <<
            "Example:\n" <<
            "    websocket-client-async-local /tmp/ws.sock localhost 80 \"Hello, world!\"\n";
        return EXIT_FAILURE;
    }
    auto const path = argv[1];
    auto const host = argv[2];
    auto const port = argv[3];
    auto const text = argv[4];

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run(path, host, port, text);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

#else

int example_main(int, char*[])
{
ex_out <<
    "Local sockets not available on this platform" << std::endl;
return EXIT_FAILURE;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: websocket/client/async-local/websocket_client_async_local") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_local_path("beast.sock"); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_websocket_server_async_local_websocket_server_async_local::example_main, std::vector<std::string>{"server", p, "1"}); }, ex_port);
    ex_args.push_back(ex_port);
    ex_args.push_back("localhost");
    ex_args.push_back("80");
    ex_args.push_back("hello beast");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(hello beast
)ex"));
#endif
}
}  // namespace ex_websocket_client_async_local_websocket_client_async_local
#endif

// libs/beast/example/websocket/server/chat-multi/main.cpp
namespace ex_websocket_server_chat_multi_main {
//------------------------------------------------------------------------------

// ---- inlined listener.hpp ----
#undef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_LISTENER_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_LISTENER_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_LISTENER_HPP

// ---- inlined beast.hpp ----
#undef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_BEAST_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_BEAST_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_BEAST_HPP


namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>

#endif
// ---- end beast.hpp ----
// ---- inlined net.hpp ----
#undef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_NET_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_NET_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_NET_HPP


namespace net = boost::asio;                    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

#endif
// ---- end net.hpp ----

// Forward declaration
class shared_state;

// Accepts incoming connections and launches the sessions
class listener : public boost::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    boost::shared_ptr<shared_state> state_;

    void fail(beast::error_code ec, char const* what);
    void on_accept(beast::error_code ec, tcp::socket socket);

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        boost::shared_ptr<shared_state> const& state);

    // Start accepting incoming connections
    void run();
};

#endif
// ---- end listener.hpp ----
// ---- inlined shared_state.hpp ----
#undef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_SHARED_STATE_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_SHARED_STATE_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_SHARED_STATE_HPP


// Forward declaration
class websocket_session;

// Represents the shared server state
class shared_state
{
    std::string const doc_root_;

    // This mutex synchronizes all access to sessions_
    std::mutex mutex_;

    // Keep a list of all the connected clients
    std::unordered_set<websocket_session*> sessions_;

public:
    explicit
    shared_state(std::string doc_root);

    std::string const&
    doc_root() const noexcept
    {
        return doc_root_;
    }

    void join  (websocket_session* session);
    void leave (websocket_session* session);
    void send  (std::string message);
};

#endif
// ---- end shared_state.hpp ----


int example_main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 5)
    {
        ex_out <<
            "Usage: websocket-chat-multi <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    websocket-chat-server 0.0.0.0 8080 . 5\n";
        return EXIT_FAILURE;
    }
    auto address = net::ip::make_address(argv[1]);
    auto port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto doc_root = argv[3];
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    net::io_context ioc;

    // Create and launch a listening port
    boost::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        boost::make_shared<shared_state>(doc_root))->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&ioc](boost::system::error_code const&, int)
        {
            // Stop the io_context. This will cause run()
            // to return immediately, eventually destroying the
            // io_context and any remaining handlers in it.
            ioc.stop();
        });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    // Block until all the threads exit
    for(auto& t : v)
        t.join();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}
// ---- merged http_session.cpp (the same program) ----
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

// ---- inlined http_session.hpp ----
#undef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_HTTP_SESSION_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_HTTP_SESSION_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_HTTP_SESSION_HPP


/** Represents an established HTTP connection
*/
class http_session : public boost::enable_shared_from_this<http_session>
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    boost::shared_ptr<shared_state> state_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

    struct send_lambda;

    void fail(beast::error_code ec, char const* what);
    void do_read();
    void on_read(beast::error_code ec, std::size_t);
    void on_write(beast::error_code ec, std::size_t, bool close);

public:
    http_session(
        tcp::socket&& socket,
        boost::shared_ptr<shared_state> const& state);

    void run();
};

#endif
// ---- end http_session.hpp ----
// ---- inlined websocket_session.hpp ----
#undef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_WEBSOCKET_SESSION_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//

#ifndef BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_WEBSOCKET_SESSION_HPP
#define BOOST_BEAST_EXAMPLE_WEBSOCKET_CHAT_MULTI_WEBSOCKET_SESSION_HPP



// Forward declaration
class shared_state;

/** Represents an active WebSocket connection to the server
*/
class websocket_session : public boost::enable_shared_from_this<websocket_session>
{
    beast::flat_buffer buffer_;
    websocket::stream<beast::tcp_stream> ws_;
    boost::shared_ptr<shared_state> state_;
    std::vector<boost::shared_ptr<std::string const>> queue_;

    void fail(beast::error_code ec, char const* what);
    void on_accept(beast::error_code ec);
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, std::size_t bytes_transferred);

public:
    websocket_session(
        tcp::socket&& socket,
        boost::shared_ptr<shared_state> const& state);

    ~websocket_session();

    template<class Body, class Allocator>
    void
    run(http::request<Body, http::basic_fields<Allocator>> req);

    // Send a message
    void
    send(boost::shared_ptr<std::string const> const& ss);

private:
    void
    on_send(boost::shared_ptr<std::string const> const& ss);
};

template<class Body, class Allocator>
void
websocket_session::
run(http::request<Body, http::basic_fields<Allocator>> req)
{
    // Set suggested timeout settings for the websocket
    ws_.set_option(
        websocket::stream_base::timeout::suggested(
            beast::role_type::server));

    // Set a decorator to change the Server of the handshake
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res)
        {
            res.set(http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-chat-multi");
        }));

    // Accept the websocket handshake
    ws_.async_accept(
        req,
        beast::bind_front_handler(
            &websocket_session::on_accept,
            shared_from_this()));
}

#endif
// ---- end websocket_session.hpp ----

//------------------------------------------------------------------------------

// Return a reasonable mime type based on the extension of a file.
beast::string_view
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(
    beast::string_view base,
    beast::string_view path)
{
    if(base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator
handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return bad_request("Illegal request-target");

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == boost::system::errc::no_such_file_or_directory)
        return not_found(req.target());

    // Handle an unknown error
    if(ec)
        return server_error(ec.message());

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

//------------------------------------------------------------------------------

http_session::
http_session(
    tcp::socket&& socket,
    boost::shared_ptr<shared_state> const& state)
    : stream_(std::move(socket))
    , state_(state)
{
}

void
http_session::
run()
{
    do_read();
}

// Report a failure
void
http_session::
fail(beast::error_code ec, char const* what)
{
    // Don't report on canceled operations
    if(ec == net::error::operation_aborted)
        return;

    ex_out << what << ": " << ec.message() << "\n";
}

void
http_session::
do_read()
{
    // Construct a new parser for each message
    parser_.emplace();

    // Apply a reasonable limit to the allowed size
    // of the body in bytes to prevent abuse.
    parser_->body_limit(10000);

    // Set the timeout.
    stream_.expires_after(std::chrono::seconds(30));

    // Read a request
    http::async_read(
        stream_,
        buffer_,
        parser_->get(),
        beast::bind_front_handler(
            &http_session::on_read,
            shared_from_this()));
}

void
http_session::
on_read(beast::error_code ec, std::size_t)
{
    // This means they closed the connection
    if(ec == http::error::end_of_stream)
    {
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Handle the error, if any
    if(ec)
        return fail(ec, "read");

    // See if it is a WebSocket Upgrade
    if(websocket::is_upgrade(parser_->get()))
    {
        // Create a websocket session, transferring ownership
        // of both the socket and the HTTP request.
        boost::make_shared<websocket_session>(
            stream_.release_socket(),
                state_)->run(parser_->release());
        return;
    }

    // Handle request
    http::message_generator msg =
        handle_request(state_->doc_root(), parser_->release());

    // Determine if we should close the connection
    bool keep_alive = msg.keep_alive();

    auto self = shared_from_this();

    // Send the response
    beast::async_write(
        stream_, std::move(msg),
        [self, keep_alive](beast::error_code ec, std::size_t bytes)
        {
            self->on_write(ec, bytes, keep_alive);
        });
}

void
http_session::
on_write(beast::error_code ec, std::size_t, bool keep_alive)
{
    // Handle the error, if any
    if(ec)
        return fail(ec, "write");

    if(! keep_alive)
    {
        // This means we should close the connection, usually because
        // the response indicated the "Connection: close" semantic.
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
        return;
    }

    // Read another request
    do_read();
}
// ---- merged listener.cpp (the same program) ----
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//


listener::
listener(
    net::io_context& ioc,
    tcp::endpoint endpoint,
    boost::shared_ptr<shared_state> const& state)
    : ioc_(ioc)
    , acceptor_(ioc)
    , state_(state)
{
    beast::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec)
    {
        fail(ec, "open");
        return;
    }

    // Allow address reuse
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if(ec)
    {
        fail(ec, "set_option");
        return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec)
    {
        fail(ec, "bind");
        return;
    }

    // Start listening for connections
    acceptor_.listen(
        net::socket_base::max_listen_connections, ec);
    if(ec)
    {
        fail(ec, "listen");
        return;
    }
}

void
listener::
run()
{
    // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &listener::on_accept,
            shared_from_this()));
}

// Report a failure
void
listener::
fail(beast::error_code ec, char const* what)
{
    // Don't report on canceled operations
    if(ec == net::error::operation_aborted)
        return;
    ex_out << what << ": " << ec.message() << "\n";
}

// Handle a connection
void
listener::
on_accept(beast::error_code ec, tcp::socket socket)
{
    if(ec)
        return fail(ec, "accept");
    else
        // Launch a new session for this connection
        boost::make_shared<http_session>(
            std::move(socket),
            state_)->run();

    // The new connection gets its own strand
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(
            &listener::on_accept,
            shared_from_this()));
}
// ---- merged shared_state.cpp (the same program) ----
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//


shared_state::
shared_state(std::string doc_root)
    : doc_root_(std::move(doc_root))
{
}

void
shared_state::
join(websocket_session* session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(session);
}

void
shared_state::
leave(websocket_session* session)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(session);
}

// Broadcast a message to all websocket client sessions
void
shared_state::
send(std::string message)
{
    // Put the message in a shared pointer so we can re-use it for each client
    auto const ss = boost::make_shared<std::string const>(std::move(message));

    // Make a local list of all the weak pointers representing
    // the sessions, so we can do the actual sending without
    // holding the mutex:
    std::vector<boost::weak_ptr<websocket_session>> v;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        v.reserve(sessions_.size());
        for(auto p : sessions_)
            v.emplace_back(p->weak_from_this());
    }

    // For each session in our local list, try to acquire a strong
    // pointer. If successful, then send the message on that session.
    for(auto const& wp : v)
        if(auto sp = wp.lock())
            sp->send(ss);
}
// ---- merged websocket_session.cpp (the same program) ----
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/vinniefalco/CppCon2018
//


websocket_session::
websocket_session(
    tcp::socket&& socket,
    boost::shared_ptr<shared_state> const& state)
    : ws_(std::move(socket))
    , state_(state)
{
}

websocket_session::
~websocket_session()
{
    // Remove this session from the list of active sessions
    state_->leave(this);
}

void
websocket_session::
fail(beast::error_code ec, char const* what)
{
    // Don't report these
    if( ec == net::error::operation_aborted ||
        ec == websocket::error::closed)
        return;

    ex_out << what << ": " << ec.message() << "\n";
}

void
websocket_session::
on_accept(beast::error_code ec)
{
    // Handle the error, if any
    if(ec)
        return fail(ec, "accept");

    // Add this session to the list of active sessions
    state_->join(this);

    // Read a message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void
websocket_session::
on_read(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if(ec)
        return fail(ec, "read");

    // Send to all connections
    state_->send(beast::buffers_to_string(buffer_.data()));

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Read another message
    ws_.async_read(
        buffer_,
        beast::bind_front_handler(
            &websocket_session::on_read,
            shared_from_this()));
}

void
websocket_session::
send(boost::shared_ptr<std::string const> const& ss)
{
    // Post our work to the strand, this ensures
    // that the members of `this` will not be
    // accessed concurrently.

    net::post(
        ws_.get_executor(),
        beast::bind_front_handler(
            &websocket_session::on_send,
            shared_from_this(),
            ss));
}

void
websocket_session::
on_send(boost::shared_ptr<std::string const> const& ss)
{
    // Always add to queue
    queue_.push_back(ss);

    // Are we already writing?
    if(queue_.size() > 1)
        return;

    // We are not currently writing, so send this immediately
    ws_.async_write(
        net::buffer(*queue_.front()),
        beast::bind_front_handler(
            &websocket_session::on_write,
            shared_from_this()));
}

void
websocket_session::
on_write(beast::error_code ec, std::size_t)
{
    // Handle the error, if any
    if(ec)
        return fail(ec, "write");

    // Remove the string from the queue
    queue_.erase(queue_.begin());

    // Send the next message if any
    if(! queue_.empty())
        ws_.async_write(
            net::buffer(*queue_.front()),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
}

TEST_CASE("example: websocket/server/chat-multi/main") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back(ex_root);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_ws_exchange(ex_port, "hello beast")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello beast
)ex"));
}
}  // namespace ex_websocket_server_chat_multi_main

// libs/beast/example/advanced/server/advanced_server.cpp
namespace ex_advanced_server_advanced_server {
namespace beast = boost::beast;                 // from <boost/beast.hpp>
namespace http = beast::http;                   // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;         // from <boost/beast/websocket.hpp>
namespace net = boost::asio;                    // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>

// Return a reasonable mime type based on the extension of a file.
beast::string_view
mime_type(beast::string_view path)
{
    using beast::iequals;
    auto const ext = [&path]
    {
        auto const pos = path.rfind(".");
        if(pos == beast::string_view::npos)
            return beast::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Append an HTTP rel-path to a local filesystem path.
// The returned path is normalized for the platform.
std::string
path_cat(
    beast::string_view base,
    beast::string_view path)
{
    if(base.empty())
        return std::string(path);
    std::string result(base);
#ifdef BOOST_MSVC
    char constexpr path_separator = '\\';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
    for(auto& c : result)
        if(c == '/')
            c = path_separator;
#else
    char constexpr path_separator = '/';
    if(result.back() == path_separator)
        result.resize(result.size() - 1);
    result.append(path.data(), path.size());
#endif
    return result;
}

// Return a response for the given request.
//
// The concrete type of the response message (which depends on the
// request), is type-erased in message_generator.
template <class Body, class Allocator>
http::message_generator
handle_request(
    beast::string_view doc_root,
    http::request<Body, http::basic_fields<Allocator>>&& req)
{
    // Returns a bad request response
    auto const bad_request =
    [&req](beast::string_view why)
    {
        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    // Returns a not found response
    auto const not_found =
    [&req](beast::string_view target)
    {
        http::response<http::string_body> res{http::status::not_found, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    // Returns a server error response
    auto const server_error =
    [&req](beast::string_view what)
    {
        http::response<http::string_body> res{http::status::internal_server_error, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    // Make sure we can handle the method
    if( req.method() != http::verb::get &&
        req.method() != http::verb::head)
        return bad_request("Unknown HTTP-method");

    // Request path must be absolute and not contain "..".
    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != beast::string_view::npos)
        return bad_request("Illegal request-target");

    // Build the path to the requested file
    std::string path = path_cat(doc_root, req.target());
    if(req.target().back() == '/')
        path.append("index.html");

    // Attempt to open the file
    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    // Handle the case where the file doesn't exist
    if(ec == beast::errc::no_such_file_or_directory)
        return not_found(req.target());

    // Handle an unknown error
    if(ec)
        return server_error(ec.message());

    // Cache the size since we need it after the move
    auto const size = body.size();

    // Respond to HEAD request
    if(req.method() == http::verb::head)
    {
        http::response<http::empty_body> res{http::status::ok, req.version()};
        res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    // Respond to GET request
    http::response<http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(http::status::ok, req.version())};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class websocket_session : public std::enable_shared_from_this<websocket_session>
{
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;

public:
    // Take ownership of the socket
    explicit
    websocket_session(tcp::socket&& socket)
        : ws_(std::move(socket))
    {
    }

    // Start the asynchronous accept operation
    template<class Body, class Allocator>
    void
    do_accept(http::request<Body, http::basic_fields<Allocator>> req)
    {
        // Set suggested timeout settings for the websocket
        ws_.set_option(
            websocket::stream_base::timeout::suggested(
                beast::role_type::server));

        // Set a decorator to change the Server of the handshake
        ws_.set_option(websocket::stream_base::decorator(
            [](websocket::response_type& res)
            {
                res.set(http::field::server,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " advanced-server");
            }));

        // Accept the websocket handshake
        ws_.async_accept(
            req,
            beast::bind_front_handler(
                &websocket_session::on_accept,
                shared_from_this()));
    }

private:
    void
    on_accept(beast::error_code ec)
    {
        if(ec)
            return fail(ec, "accept");

        // Read a message
        do_read();
    }

    void
    do_read()
    {
        // Read a message into our buffer
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &websocket_session::on_read,
                shared_from_this()));
    }

    void
    on_read(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This indicates that the websocket_session was closed
        if(ec == websocket::error::closed)
            return;

        if(ec)
            return fail(ec, "read");

        // Echo the message
        ws_.text(ws_.got_text());
        ws_.async_write(
            buffer_.data(),
            beast::bind_front_handler(
                &websocket_session::on_write,
                shared_from_this()));
    }

    void
    on_write(
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Do another read
        do_read();
    }
};

//------------------------------------------------------------------------------

// Handles an HTTP server connection
class http_session : public std::enable_shared_from_this<http_session>
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;

    static constexpr std::size_t queue_limit = 8; // max responses
    std::queue<http::message_generator> response_queue_;

    // The parser is stored in an optional container so we can
    // construct it from scratch it at the beginning of each new message.
    boost::optional<http::request_parser<http::string_body>> parser_;

public:
    // Take ownership of the socket
    http_session(
        tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root)
        : stream_(std::move(socket))
        , doc_root_(doc_root)
    {
        static_assert(queue_limit > 0,
                      "queue limit must be positive");
    }

    // Start the session
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(
            stream_.get_executor(),
            beast::bind_front_handler(
                &http_session::do_read,
                this->shared_from_this()));
    }

private:
    void
    do_read()
    {
        // Construct a new parser for each message
        parser_.emplace();

        // Apply a reasonable limit to the allowed size
        // of the body in bytes to prevent abuse.
        parser_->body_limit(10000);

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request using the parser-oriented interface
        http::async_read(
            stream_,
            buffer_,
            *parser_,
            beast::bind_front_handler(
                &http_session::on_read,
                shared_from_this()));
    }

    void
    on_read(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // See if it is a WebSocket Upgrade
        if(websocket::is_upgrade(parser_->get()))
        {
            // Create a websocket session, transferring ownership
            // of both the socket and the HTTP request.
            std::make_shared<websocket_session>(
                stream_.release_socket())->do_accept(parser_->release());
            return;
        }

        // Send the response
        queue_write(handle_request(*doc_root_, parser_->release()));

        // If we aren't at the queue limit, try to pipeline another request
        if (response_queue_.size() < queue_limit)
            do_read();
    }

    void
    queue_write(http::message_generator response)
    {
        // Allocate and store the work
        response_queue_.push(std::move(response));

        // If there was no previous work, start the write loop
        if (response_queue_.size() == 1)
            do_write();
    }

    // Called to start/continue the write-loop. Should not be called when
    // write_loop is already active.
    void
    do_write()
    {
        if(! response_queue_.empty())
        {
            bool keep_alive = response_queue_.front().keep_alive();

            beast::async_write(
                stream_,
                std::move(response_queue_.front()),
                beast::bind_front_handler(
                    &http_session::on_write,
                    shared_from_this(),
                    keep_alive));
        }
    }

    void
    on_write(
        bool keep_alive,
        beast::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return fail(ec, "write");

        if(! keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            return do_close();
        }

        // Resume the read if it has been paused
        if(response_queue_.size() == queue_limit)
            do_read();

        response_queue_.pop();

        do_write();
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , doc_root_(doc_root)
    {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if(ec)
        {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            net::socket_base::max_listen_connections, ec);
        if(ec)
        {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void
    run()
    {
        // We need to be executing within a strand to perform async operations
        // on the I/O objects in this session. Although not strictly necessary
        // for single-threaded contexts, this example code is written to be
        // thread-safe by default.
        net::dispatch(
            acceptor_.get_executor(),
            beast::bind_front_handler(
                &listener::do_accept,
                this->shared_from_this()));
    }

private:
    void
    do_accept()
    {
        // The new connection gets its own strand
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(
                &listener::on_accept,
                shared_from_this()));
    }

    void
    on_accept(beast::error_code ec, tcp::socket socket)
    {
        if(ec)
        {
            fail(ec, "accept");
        }
        else
        {
            // Create the http session and run it
            std::make_shared<http_session>(
                std::move(socket),
                doc_root_)->run();
        }

        // Accept another connection
        do_accept();
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 5)
    {
        ex_out <<
            "Usage: advanced-server <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    advanced-server 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    net::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(
        ioc,
        tcp::endpoint{address, port},
        doc_root)->run();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    net::signal_set signals(ioc, SIGINT, SIGTERM);
    signals.async_wait(
        [&](beast::error_code const&, int)
        {
            // Stop the `io_context`. This will cause `run()`
            // to return immediately, eventually destroying the
            // `io_context` and all of the sockets in it.
            ioc.stop();
        });

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    // (If we get here, it means we got a SIGINT or SIGTERM)

    // Block until all the threads exit
    for(auto& t : v)
        t.join();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: advanced/server/advanced_server") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back(ex_root);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/index.html") + ex_ws_exchange(ex_port, "hello beast")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
received: hello beast
)ex"));
}
}  // namespace ex_advanced_server_advanced_server

// libs/beast/example/echo-op/echo_op.cpp
namespace ex_echo_op_echo_op {
namespace net = boost::asio;
namespace beast = boost::beast;

//[example_core_echo_op_1

template<
    class AsyncStream,
    class DynamicBuffer,
    class CompletionToken>
auto
async_echo (AsyncStream& stream, DynamicBuffer& buffer, CompletionToken&& token)

//]
    ->
    typename net::async_result<
        typename std::decay<CompletionToken>::type,
        void(beast::error_code)>::return_type;

//------------------------------------------------------------------------------

//[example_core_echo_op_2

/** Asynchronously read a line and echo it back.

    This function is used to asynchronously read a line ending
    in a newline (`"\n"`) from the stream, and then write
    it back.

    This call always returns immediately. The asynchronous operation
    will continue until one of the following conditions is true:

    @li A line was read in and written back on the stream

    @li An error occurs.

    The algorithm, known as a <em>composed asynchronous operation</em>,
    is implemented in terms of calls to the stream's `async_read_some`
    and `async_write_some` function. The program must ensure that no
    other reads or writes are performed until this operation completes.

    Since the length of the line is not known ahead of time, the
    implementation may read additional characters that lie past the
    first line. These characters are stored in the dynamic buffer_.
    The same dynamic buffer must be presented again in each call,
    to provide the implementation with any leftover bytes.

    @param stream The stream to operate on. The type must meet the
    requirements of <em>AsyncReadStream</em> and @AsyncWriteStream

    @param buffer A dynamic buffer to hold implementation-defined
    temporary data. Ownership is not transferred; the caller is
    responsible for ensuring that the lifetime of this object is
    extended least until the completion handler is invoked.

    @param token The handler to be called when the operation completes.
    The implementation takes ownership of the handler by performing a decay-copy.
    The handler must be invocable with this signature:
    @code
    void handler(
        beast::error_code error      // Result of operation.
    );
    @endcode

    Regardless of whether the asynchronous operation completes immediately or
    not, the handler will not be invoked from within this function. Invocation
    of the handler will be performed in a manner equivalent to using
    `net::post`.
*/
template<
    class AsyncStream,
    class DynamicBuffer,
    class CompletionToken>
auto
async_echo (
    AsyncStream& stream,
    DynamicBuffer& buffer, /*< Unlike Asio, we pass by non-const reference instead of rvalue-ref >*/
    CompletionToken&& token) ->
        typename net::async_result< /*< `async_result` deduces the return type from the completion handler >*/
            typename std::decay<CompletionToken>::type,
            void(beast::error_code) /*< The completion handler signature goes here >*/
                >::return_type;
//]

//[example_core_echo_op_3

template<class AsyncStream, class Handler>
class echo_op;

// This example uses the Asio's stackless "fauxroutines", implemented
// using a macro-based solution. It makes the code easier to write and
// easier to read. This include file defines the necessary macros and types.
#include <boost/asio/yield.hpp>

// Read a line and echo it back
//
template<
    class AsyncStream,
    class DynamicBuffer,
    class CompletionToken>
auto
async_echo(
    AsyncStream& stream,
    DynamicBuffer& buffer,
    CompletionToken&& token) ->
        typename net::async_result<
            typename std::decay<CompletionToken>::type,
            void(beast::error_code)>::return_type /*< The completion handler signature goes here >*/
{
    // Perform some type checks using static assert, this helps
    // with more friendly error messages when passing the wrong types.
    static_assert(
        beast::is_async_stream<AsyncStream>::value,
        "AsyncStream type requirements not met");
    static_assert(
        net::is_dynamic_buffer<DynamicBuffer>::value,
        "DynamicBuffer type requirements not met");

    // This class template deduces the actual handler type from a
    // CompletionToken, captures a local reference to the handler,
    // and creates the `async_result` object which becomes the
    // return value of this initiating function.

    net::async_completion<CompletionToken, void(beast::error_code)> init(token);

    // The helper macro BOOST_ASIO_HANDLER_TYPE converts the completion
    // token type into a concrete handler type of the correct signature.

    using handler_type = BOOST_ASIO_HANDLER_TYPE(CompletionToken, void(beast::error_code));

    // The class template `async_base` holds the caller's completion
    // handler for us, and provides all of the boilerplate for forwarding
    // the associated allocator and associated executor from the caller's
    // handler to our operation. It also maintains a `net::executor_work_guard`
    // for the executor associated with the stream. This work guard is
    // inexpensive, and prevents the execution context from running out
    // of work. It is usually necessary although rarely it can be skipped
    // depending on the operation (this echo example needs it because it
    // performs more than one asynchronous operation in a row).
    // We declare this type alias to make the code easier to read.

    using base_type = beast::async_base<
        handler_type, /*< The type of the completion handler obtained from the token >*/
        beast::executor_type<AsyncStream> /*< The type of executor used by the stream to dispatch asynchronous operations >*/
    >;

    // This nested class implements the echo composed operation as a
    // stateful completion handler. We derive from `async_base` to
    // take care of boilerplate and we derived from asio::coroutine to
    // allow the reenter and yield keywords to work.

    struct echo_op : base_type, boost::asio::coroutine
    {
        AsyncStream& stream_;
        DynamicBuffer& buffer_;

        echo_op(
            AsyncStream& stream,
            DynamicBuffer& buffer,
            handler_type&& handler)
            : base_type(
                std::move(handler), /*< The `async_base` helper takes ownership of the handler, >*/
                stream.get_executor()) /*<  and also needs to know which executor to use. >*/
            , stream_(stream)
            , buffer_(buffer)
        {
            // Launch the operation directly from the constructor. We
            // pass `false` for `cont` to indicate that the calling
            // thread does not represent a continuation of our
            // asynchronous control flow.
            (*this)({}, 0, false);
        }

        // If a newline is present in the buffer sequence, this function returns
        // the number of characters from the beginning of the buffer up to the
        // newline, including the newline character. Otherwise it returns zero.

        std::size_t
        find_newline(typename DynamicBuffer::const_buffers_type const& buffers)
        {
            // The `buffers_iterator` class template provides random-access
            // iterators into a buffer sequence. Use the standard algorithm
            // to look for the new line if it exists.

            auto begin = net::buffers_iterator<
                typename DynamicBuffer::const_buffers_type>::begin(buffers);
            auto end =   net::buffers_iterator<
                typename DynamicBuffer::const_buffers_type>::end(buffers);
            auto result = std::find(begin, end, '\n');

            if(result == end)
                return 0; // not found

            return result + 1 - begin;
        }

        // This is the entry point of our completion handler. Every time an
        // asynchronous operation completes, this function will be invoked.

        void
        operator()(
            beast::error_code ec,
            std::size_t bytes_transferred = 0,
            bool cont = true) /*< Second and subsequent invocations will seee `cont=true`. */
        {
            // The `reenter` keyword transfers control to the last
            // yield point, or to the beginning of the scope if
            // this is the first time.

            reenter(*this)
            {
                for(;;)
                {
                    std::size_t pos;

                    // Search for a newline in the readable bytes of the buffer
                    pos = find_newline(buffer_.data());

                    // If we don't have the newline, then read more
                    if(pos == 0)
                    {
                        std::size_t bytes_to_read;

                        // Determine the number of bytes to read,
                        // using available capacity in the buffer first.

                        bytes_to_read = std::min<std::size_t>(
                              std::max<std::size_t>(512,                // under 512 is too little,
                                  buffer_.capacity() - buffer_.size()),
                              std::min<std::size_t>(65536,              // and over 65536 is too much.
                                  buffer_.max_size() - buffer_.size()));

                        // Read some data into our dynamic buffer_. We transfer
                        // ownership of the composed operation by using the
                        // `std::move(*this)` idiom. The `yield` keyword causes
                        // the function to return immediately after the initiating
                        // function returns.

                        yield stream_.async_read_some(
                            buffer_.prepare(bytes_to_read), std::move(*this));

                        // After the `async_read_some` completes, control is
                        // transferred to this line by the `reenter` keyword.

                        // Move the bytes read from the writable area to the
                        // readable area.

                        buffer_.commit(bytes_transferred);

                        // If an error occurs, deliver it to the caller's completion handler.
                        if(ec)
                            break;

                        // Keep looping until we get the newline
                        continue;
                    }

                    // We have our newline, so send the first `pos` bytes of the
                    // buffers. The function `buffers_prefix` returns the front part
                    // of the buffers we want.

                    yield net::async_write(stream_,
                        beast::buffers_prefix(pos, buffer_.data()), std::move(*this));

                    // After the `async_write` completes, our completion handler will
                    // be invoked with the error and the number of bytes transferred,
                    // and the `reenter` statement above will cause control to jump
                    // to the following line. The variable `pos` is no longer valid
                    // (remember that we returned from the function using `yield` above)
                    // but we can use `bytes_transferred` to know how much of the buffer
                    // to consume. With "real" coroutines this will be easier and more
                    // natural.

                    buffer_.consume(bytes_transferred);

                    // The loop terminates here, and we will either deliver a
                    // successful result or an error to the caller's completion handler.

                    break;
                }

                // When a composed operation completes immediately, it must not
                // directly invoke the completion handler otherwise it could
                // lead to unfairness, starvation, or stack overflow. Therefore,
                // if cont == false (meaning, that the call stack still includes
                // the frame of the initiating function) then we need to use
                // `net::post` to cause us to be called again after the initiating
                // function. The function `async_base::invoke` takes care of
                // calling the final completion handler, using post if the
                // first argument is false, otherwise invoking it directly.

                this->complete(cont, ec);
            }
        }
    };

    // Create the composed operation and launch it. This is a constructor
    // call followed by invocation of operator(). We use BOOST_ASIO_HANDLER_TYPE
    // to convert the completion token into the correct handler type,
    // allowing user-defined specializations of the async_result template
    // to be used.

    echo_op(stream, buffer, std::move(init.completion_handler));

    // This hook lets the caller see a return value when appropriate.
    // For example this might return std::future<error_code> if
    // CompletionToken is net::use_future, or this might
    // return an error code if CompletionToken specifies a coroutine.

    return init.result.get();
}

// Including this file undefines the macros used by the stackless fauxroutines.
#include <boost/asio/unyield.hpp>

//]

struct move_only_handler
{
    move_only_handler() = default;
    move_only_handler(move_only_handler&&) = default;
    move_only_handler(move_only_handler const&) = delete;

    void operator()(beast::error_code ec)
    {
        if(ec)
            ex_out << ": " << ec.message() << std::endl;
    }
};

int example_main(int argc, char** argv)
{
    if(argc != 3)
    {
        ex_out
        << "Usage: echo-op <address> <port>\n"
        << "Example:\n"
        << "    echo-op 0.0.0.0 8080\n";
        return EXIT_FAILURE;
    }

    namespace net = boost::asio;
    auto const address = net::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));

    using endpoint_type = net::ip::tcp::endpoint;

    // Create a listening socket, accept a connection, perform
    // the echo, and then shut everything down and exit.
    net::io_context ioc;
    net::ip::tcp::acceptor acceptor{ioc};
    endpoint_type ep(address, port);
    acceptor.open(ep.protocol());
    acceptor.set_option(net::socket_base::reuse_address(true));
    acceptor.bind(ep);
    acceptor.listen();
    auto sock = acceptor.accept();
    beast::flat_buffer buffer;
    async_echo(sock, buffer, move_only_handler{});
    ioc.run();
    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: echo-op/echo_op") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_no_probe = true;
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_tcp_exchange(ex_port, "hello op\n", 0)));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: hello op
)ex"));
}
}  // namespace ex_echo_op_echo_op
