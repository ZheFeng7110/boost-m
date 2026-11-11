// mcpplibs.mwrap.boost.beast — the official HTTP examples
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

// libs/beast/example/http/server/sync/http_server_sync.cpp
namespace ex_http_server_sync_http_server_sync {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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
	if (base.empty())
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

// Handles an HTTP server connection
void
do_session(
    tcp::socket& socket,
    std::shared_ptr<std::string const> const& doc_root)
{
    beast::error_code ec;

    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    for(;;)
    {
        // Read a request
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if(ec == http::error::end_of_stream)
            break;
        if(ec)
            return fail(ec, "read");

        // Handle request
        http::message_generator msg =
            handle_request(*doc_root, std::move(req));

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        beast::write(socket, std::move(msg), ec);

        if(ec)
            return fail(ec, "write");
        if(! keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    socket.shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
}

//------------------------------------------------------------------------------

int example_main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 4)
        {
            ex_out <<
                "Usage: http-server-sync <address> <port> <doc_root>\n" <<
                "Example:\n" <<
                "    http-server-sync 0.0.0.0 8080 .\n";
            return EXIT_FAILURE;
        }
        auto const address = net::ip::make_address(argv[1]);
        auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
        auto const doc_root = std::make_shared<std::string>(argv[3]);

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
            std::thread{std::bind(
                &do_session,
                std::move(socket),
                doc_root)}.detach();
        }
    }
    catch (const std::exception& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: http/server/sync/http_server_sync") {
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
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/index.html")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
)ex"));
}
}  // namespace ex_http_server_sync_http_server_sync

// libs/beast/example/http/server/async/http_server_async.cpp
namespace ex_http_server_async_http_server_async {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;

public:
    // Take ownership of the stream
    session(
        tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root)
        : stream_(std::move(socket))
        , doc_root_(doc_root)
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
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(
                          &session::do_read,
                          shared_from_this()));
    }

    void
    do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request
        http::async_read(stream_, buffer_, req_,
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

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // Send the response
        send_response(
            handle_request(*doc_root_, std::move(req_)));
    }

    void
    send_response(http::message_generator&& msg)
    {
        bool keep_alive = msg.keep_alive();

        // Write the response
        beast::async_write(
            stream_,
            std::move(msg),
            beast::bind_front_handler(
                &session::on_write, shared_from_this(), keep_alive));
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

        // Read another request
        do_read();
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
            return; // To avoid infinite loop
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(
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
            "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-async 0.0.0.0 8080 . 1\n";
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

TEST_CASE("example: http/server/async/http_server_async") {
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
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/index.html")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
)ex"));
}
}  // namespace ex_http_server_async_http_server_async

// libs/beast/example/http/server/stackless/http_server_stackless.cpp
namespace ex_http_server_stackless_http_server_stackless {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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

// Handles an HTTP server connection
class session
    : public boost::asio::coroutine
    , public std::enable_shared_from_this<session>
{
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;
    bool keep_alive_ = true;

public:
    // Take ownership of the socket
    explicit
    session(
        tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root)
        : stream_(std::move(socket))
        , doc_root_(doc_root)
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
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(&session::loop,
                                                shared_from_this(),
                                                beast::error_code{},
                                                0));
    }

#include <boost/asio/yield.hpp>

    void
    loop(beast::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        reenter(*this)
        {
            for(;;)
            {
                // Make the request empty before reading,
                // otherwise the operation behavior is undefined.
                req_ = {};

                // Set the timeout.
                stream_.expires_after(std::chrono::seconds(30));

                // Read a request
                yield http::async_read(stream_, buffer_, req_,
                    beast::bind_front_handler(
                        &session::loop,
                        shared_from_this()));

                if(ec == http::error::end_of_stream)
                {
                    // The remote host closed the connection
                    break;
                }
                if(ec)
                    return fail(ec, "read");

                yield {
                    // Handle request
                    http::message_generator msg =
                        handle_request(*doc_root_, std::move(req_));

                    // Determine if we should close the connection
                    keep_alive_ = msg.keep_alive();

                    // Send the response
                    beast::async_write(
                        stream_,
                        std::move(msg),
                        beast::bind_front_handler(
                            &session::loop, shared_from_this()));
                }

                if(ec)
                    return fail(ec, "write");
                if(! keep_alive_)
                {
                    // This means we should close the connection, usually because
                    // the response indicated the "Connection: close" semantic.
                    break;
                }
            }

            // Send a TCP shutdown
            stream_.socket().shutdown(tcp::socket::shutdown_send, ec);

            // At this point the connection is closed gracefully
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
    std::shared_ptr<std::string const> doc_root_;

public:
    listener(
        net::io_context& ioc,
        tcp::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , socket_(net::make_strand(ioc))
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
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
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
                    beast::bind_front_handler(
                        &listener::loop,
                        shared_from_this()));
                if(ec)
                {
                    fail(ec, "accept");
                }
                else
                {
                    // Create the session and run it
                    std::make_shared<session>(
                        std::move(socket_),
                        doc_root_)->run();
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
    if (argc != 5)
    {
        ex_out <<
            "Usage: http-server-stackless <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-stackless 0.0.0.0 8080 . 1\n";
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

TEST_CASE("example: http/server/stackless/http_server_stackless") {
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
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/index.html")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
)ex"));
}
}  // namespace ex_http_server_stackless_http_server_stackless

// libs/beast/example/http/server/awaitable/http_server_awaitable.cpp
namespace ex_http_server_awaitable_http_server_awaitable {
#if defined(BOOST_ASIO_HAS_CO_AWAIT)

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

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

// Handles an HTTP server connection
net::awaitable<void>
do_session(
    beast::tcp_stream stream,
    std::shared_ptr<std::string const> doc_root)
{
    // This buffer is required to persist across reads
    beast::flat_buffer buffer;

    for(;;)
    {
        // Set the timeout.
        stream.expires_after(std::chrono::seconds(30));

        // Read a request
        http::request<http::string_body> req;
        co_await http::async_read(stream, buffer, req);

        // Handle the request
        http::message_generator msg = handle_request(*doc_root, std::move(req));

        // Determine if we should close the connection
        bool keep_alive = msg.keep_alive();

        // Send the response
        co_await beast::async_write(stream, std::move(msg));

        if(!keep_alive)
        {
            // This means we should close the connection, usually because
            // the response indicated the "Connection: close" semantic.
            break;
        }
    }

    // Send a TCP shutdown
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_send);

    // At this point the connection is closed gracefully
    // we ignore the error because the client might have
    // dropped the connection already.
}

// Accepts incoming connections and launches the sessions
net::awaitable<void>
do_listen(net::ip::tcp::endpoint endpoint, std::shared_ptr<std::string const> doc_root)
{
    auto executor = co_await net::this_coro::executor;
    auto acceptor = net::ip::tcp::acceptor{ executor, endpoint };

    for(;;)
    {
        net::co_spawn(
            executor,
            do_session(
                beast::tcp_stream{ co_await acceptor.async_accept() },
                doc_root),
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
                        ex_out << "Error in session: " << e.what() << "\n";
                    }
                }
            });
    }
}

int example_main(int argc, char* argv[])
{
    // Check command line arguments.
    if(argc != 5)
    {
        ex_out << "Usage: http-server-awaitable <address> <port> <doc_root> <threads>\n"
                    << "Example:\n"
                    << "    http-server-awaitable 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address  = net::ip::make_address(argv[1]);
    auto const port     = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const doc_root = std::make_shared<std::string>(argv[3]);
    auto const threads  = std::max<int>(1, std::atoi(argv[4]));

    // The io_context is required for all I/O
    net::io_context ioc{ threads };

    // Spawn a listening port
    net::co_spawn(
        ioc,
        do_listen(net::ip::tcp::endpoint{ address, port }, doc_root),
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

TEST_CASE("example: http/server/awaitable/http_server_awaitable") {
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
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/index.html")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
)ex"));
}
}  // namespace ex_http_server_awaitable_http_server_awaitable

// libs/beast/example/http/server/fast/http_server_fast.cpp
namespace ex_http_server_fast_http_server_fast {
#undef BOOST_BEAST_EXAMPLE_FIELDS_ALLOC_HPP
//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#ifndef BOOST_BEAST_EXAMPLE_FIELDS_ALLOC_HPP
#define BOOST_BEAST_EXAMPLE_FIELDS_ALLOC_HPP


namespace detail {

struct static_pool
{
    std::size_t size_;
    std::size_t refs_ = 1;
    std::size_t count_ = 0;
    char* p_;

    char*
    end()
    {
        return reinterpret_cast<char*>(this + 1) + size_;
    }

    explicit
    static_pool(std::size_t size)
        : size_(size)
        , p_(reinterpret_cast<char*>(this + 1))
    {
    }

public:
    static
    static_pool&
    construct(std::size_t size)
    {
        auto p = new char[sizeof(static_pool) + size];
        return *(::new(p) static_pool{size});
    }

    static_pool&
    share()
    {
        ++refs_;
        return *this;
    }

    void
    destroy()
    {
        if(--refs_)
            return;
        this->~static_pool();
        delete[] reinterpret_cast<char*>(this);
    }

    void*
    alloc(std::size_t n)
    {
        auto last = p_ + n;
        if(last >= end())
            BOOST_THROW_EXCEPTION(std::bad_alloc{});
        ++count_;
        auto p = p_;
        p_ = last;
        return p;
    }

    void
    dealloc()
    {
        if(--count_)
            return;
        p_ = reinterpret_cast<char*>(this + 1);
    }
};

} // detail

/** A non-thread-safe allocator optimized for @ref basic_fields.

    This allocator obtains memory from a pre-allocated memory block
    of a given size. It does nothing in deallocate until all
    previously allocated blocks are deallocated, upon which it
    resets the internal memory block for re-use.

    To use this allocator declare an instance persistent to the
    connection or session, and construct with the block size.
    A good rule of thumb is 20% more than the maximum allowed
    header size. For example if the application only allows up
    to an 8,000 byte header, the block size could be 9,600.

    Then, for every instance of `message` construct the header
    with a copy of the previously declared allocator instance.
*/
template<class T>
struct fields_alloc
{
    detail::static_pool* pool_;

public:
    using value_type = T;
    using is_always_equal = std::false_type;
    using pointer = T*;
    using reference = T&;
    using const_pointer = T const*;
    using const_reference = T const&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<class U>
    struct rebind
    {
        using other = fields_alloc<U>;
    };

#if defined(_GLIBCXX_USE_CXX11_ABI) && (_GLIBCXX_USE_CXX11_ABI == 0)
    // Workaround for g++
    // basic_string assumes that allocators are default-constructible
    // See: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56437
    fields_alloc() = default;
#endif

    explicit
    fields_alloc(std::size_t size)
        : pool_(&detail::static_pool::construct(size))
    {
    }

    fields_alloc(fields_alloc const& other)
        : pool_(&other.pool_->share())
    {
    }

    template<class U>
    fields_alloc(fields_alloc<U> const& other)
        : pool_(&other.pool_->share())
    {
    }

    ~fields_alloc()
    {
        pool_->destroy();
    }

    value_type*
    allocate(size_type n)
    {
        return static_cast<value_type*>(
            pool_->alloc(n * sizeof(T)));
    }

    void
    deallocate(value_type*, size_type)
    {
        pool_->dealloc();
    }

#if defined(BOOST_LIBSTDCXX_VERSION) && BOOST_LIBSTDCXX_VERSION < 60000
    template<class U, class... Args>
    void
    construct(U* ptr, Args&&... args)
    {
        ::new(static_cast<void*>(ptr)) U(
            std::forward<Args>(args)...);
    }

    template<class U>
    void
    destroy(U* ptr)
    {
        ptr->~U();
    }
#endif

    template<class U>
    friend
    bool
    operator==(
        fields_alloc const& lhs,
        fields_alloc<U> const& rhs)
    {
        return &lhs.pool_ == &rhs.pool_;
    }

    template<class U>
    friend
    bool
    operator!=(
        fields_alloc const& lhs,
        fields_alloc<U> const& rhs)
    {
        return ! (lhs == rhs);
    }
};

#endif
// ---- end fields_alloc.hpp ----


namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

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

class http_worker
{
public:
    http_worker(http_worker const&) = delete;
    http_worker& operator=(http_worker const&) = delete;

    http_worker(tcp::acceptor& acceptor, const std::string& doc_root) :
        acceptor_(acceptor),
        doc_root_(doc_root)
    {
    }

    void start()
    {
        accept();
        check_deadline();
    }

private:
    using alloc_t = fields_alloc<char>;
    //using request_body_t = http::basic_dynamic_body<beast::flat_static_buffer<1024 * 1024>>;
    using request_body_t = http::string_body;

    // The acceptor used to listen for incoming connections.
    tcp::acceptor& acceptor_;

    // The path to the root of the document directory.
    std::string doc_root_;

    // The socket for the currently connected client.
    tcp::socket socket_{acceptor_.get_executor()};

    // The buffer for performing reads
    beast::flat_static_buffer<8192> buffer_;

    // The allocator used for the fields in the request and reply.
    alloc_t alloc_{8192};

    // The parser for reading the requests
    boost::optional<http::request_parser<request_body_t, alloc_t>> parser_;

    // The timer putting a time limit on requests.
    net::steady_timer request_deadline_{
        acceptor_.get_executor(), (std::chrono::steady_clock::time_point::max)()};

    // The string-based response message.
    boost::optional<http::response<http::string_body, http::basic_fields<alloc_t>>> string_response_;

    // The string-based response serializer.
    boost::optional<http::response_serializer<http::string_body, http::basic_fields<alloc_t>>> string_serializer_;

    // The file-based response message.
    boost::optional<http::response<http::file_body, http::basic_fields<alloc_t>>> file_response_;

    // The file-based response serializer.
    boost::optional<http::response_serializer<http::file_body, http::basic_fields<alloc_t>>> file_serializer_;

    void accept()
    {
        // Clean up any previous connection.
        beast::error_code ec;
        socket_.close(ec);
        buffer_.consume(buffer_.size());

        acceptor_.async_accept(
            socket_,
            [this](beast::error_code ec)
            {
                if (ec)
                {
                    accept();
                }
                else
                {
                    // Request must be fully processed within 60 seconds.
                    request_deadline_.expires_after(
                        std::chrono::seconds(60));

                    read_request();
                }
            });
    }

    void read_request()
    {
        // On each read the parser needs to be destroyed and
        // recreated. We store it in a boost::optional to
        // achieve that.
        //
        // Arguments passed to the parser constructor are
        // forwarded to the message object. A single argument
        // is forwarded to the body constructor.
        //
        // We construct the dynamic body with a 1MB limit
        // to prevent vulnerability to buffer attacks.
        //
        parser_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        http::async_read(
            socket_,
            buffer_,
            *parser_,
            [this](beast::error_code ec, std::size_t)
            {
                if (ec)
                    accept();
                else
                    process_request(parser_->get());
            });
    }

    void process_request(http::request<request_body_t, http::basic_fields<alloc_t>> const& req)
    {
        switch (req.method())
        {
        case http::verb::get:
            send_file(req.target());
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            send_bad_response(
                http::status::bad_request,
                "Invalid request-method '" + std::string(req.method_string()) + "'\r\n");
            break;
        }
    }

    void send_bad_response(
        http::status status,
        std::string const& error)
    {
        string_response_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        string_response_->result(status);
        string_response_->keep_alive(false);
        string_response_->set(http::field::server, "Beast");
        string_response_->set(http::field::content_type, "text/plain");
        string_response_->body() = error;
        string_response_->prepare_payload();

        string_serializer_.emplace(*string_response_);

        http::async_write(
            socket_,
            *string_serializer_,
            [this](beast::error_code ec, std::size_t)
            {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                string_serializer_.reset();
                string_response_.reset();
                accept();
            });
    }

    void send_file(beast::string_view target)
    {
        // Request path must be absolute and not contain "..".
        if (target.empty() || target[0] != '/' || target.find("..") != std::string::npos)
        {
            send_bad_response(
                http::status::not_found,
                "File not found\r\n");
            return;
        }

        std::string full_path = doc_root_;
        full_path.append(
            target.data(),
            target.size());

        http::file_body::value_type file;
        beast::error_code ec;
        file.open(
            full_path.c_str(),
            beast::file_mode::read,
            ec);
        if(ec)
        {
            send_bad_response(
                http::status::not_found,
                "File not found\r\n");
            return;
        }

        file_response_.emplace(
            std::piecewise_construct,
            std::make_tuple(),
            std::make_tuple(alloc_));

        file_response_->result(http::status::ok);
        file_response_->keep_alive(false);
        file_response_->set(http::field::server, "Beast");
        file_response_->set(http::field::content_type, mime_type(std::string(target)));
        file_response_->body() = std::move(file);
        file_response_->prepare_payload();

        file_serializer_.emplace(*file_response_);

        http::async_write(
            socket_,
            *file_serializer_,
            [this](beast::error_code ec, std::size_t)
            {
                socket_.shutdown(tcp::socket::shutdown_send, ec);
                file_serializer_.reset();
                file_response_.reset();
                accept();
            });
    }

    void check_deadline()
    {
        // The deadline may have moved, so check it has really passed.
        if (request_deadline_.expiry() <= std::chrono::steady_clock::now())
        {
            // Close socket to cancel any outstanding operation.
            socket_.close();

            // Sleep indefinitely until we're given a new deadline.
            request_deadline_.expires_at(
                (std::chrono::steady_clock::time_point::max)());
        }

        request_deadline_.async_wait(
            [this](beast::error_code)
            {
                check_deadline();
            });
    }
};

int example_main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if (argc != 6)
        {
            ex_out << "Usage: http_server_fast <address> <port> <doc_root> <num_workers> {spin|block}\n";
            ex_out << "  For IPv4, try:\n";
            ex_out << "    http_server_fast 0.0.0.0 80 . 100 block\n";
            ex_out << "  For IPv6, try:\n";
            ex_out << "    http_server_fast 0::0 80 . 100 block\n";
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));
        std::string doc_root = argv[3];
        int num_workers = std::atoi(argv[4]);
        bool spin = (std::strcmp(argv[5], "spin") == 0);

        net::io_context ioc{1};
        tcp::acceptor acceptor{ioc, {address, port}};

        std::list<http_worker> workers;
        for (int i = 0; i < num_workers; ++i)
        {
            workers.emplace_back(acceptor, doc_root);
            workers.back().start();
        }

        if (spin)
          for (;;) ioc.poll();
        else
          ioc.run();
    }
    catch (const std::exception& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: http/server/fast/http_server_fast") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root();
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back(ex_root);
    ex_args.push_back("2");
    ex_args.push_back("block");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/index.html")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
)ex"));
}
}  // namespace ex_http_server_fast_http_server_fast

// libs/beast/example/http/server/small/http_server_small.cpp
namespace ex_http_server_small_http_server_small {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace my_program_state
{
    std::size_t
    request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
    now()
    {
        return std::time(0);
    }
}

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    // Initiate the asynchronous operations associated with the connection.
    void
    start()
    {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket socket_;

    // The buffer for performing reads.
    beast::flat_buffer buffer_{8192};

    // The request message.
    http::request<http::dynamic_body> request_;

    // The response message.
    http::response<http::dynamic_body> response_;

    // The timer for putting a deadline on connection processing.
    net::steady_timer deadline_{
        socket_.get_executor(), std::chrono::seconds(60)};

    // Asynchronously receive a complete request message.
    void
    read_request()
    {
        auto self = shared_from_this();

        http::async_read(
            socket_,
            buffer_,
            request_,
            [self](beast::error_code ec,
                std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if(!ec)
                    self->process_request();
            });
    }

    // Determine what needs to be done with the request message.
    void
    process_request()
    {
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
        case http::verb::get:
            response_.result(http::status::ok);
            response_.set(http::field::server, "Beast");
            create_response();
            break;

        default:
            // We return responses indicating an error if
            // we do not recognize the request method.
            response_.result(http::status::bad_request);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body())
                << "Invalid request-method '"
                << std::string(request_.method_string())
                << "'";
            break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void
    create_response()
    {
        if(request_.target() == "/count")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                << "<html>\n"
                <<  "<head><title>Request count</title></head>\n"
                <<  "<body>\n"
                <<  "<h1>Request count</h1>\n"
                <<  "<p>There have been "
                <<  my_program_state::request_count()
                <<  " requests so far.</p>\n"
                <<  "</body>\n"
                <<  "</html>\n";
        }
        else if(request_.target() == "/time")
        {
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                <<  "<html>\n"
                <<  "<head><title>Current time</title></head>\n"
                <<  "<body>\n"
                <<  "<h1>Current time</h1>\n"
                <<  "<p>The current time is "
                <<  my_program_state::now()
                <<  " seconds since the epoch.</p>\n"
                <<  "</body>\n"
                <<  "</html>\n";
        }
        else
        {
            response_.result(http::status::not_found);
            response_.set(http::field::content_type, "text/plain");
            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }

    // Asynchronously transmit the response message.
    void
    write_response()
    {
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
            socket_,
            response_,
            [self](beast::error_code ec, std::size_t)
            {
                self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                self->deadline_.cancel();
            });
    }

    // Check whether we have spent enough time on this connection.
    void
    check_deadline()
    {
        auto self = shared_from_this();

        deadline_.async_wait(
            [self](beast::error_code ec)
            {
                if(!ec)
                {
                    // Close socket to cancel any outstanding operation.
                    self->socket_.close(ec);
                }
            });
    }
};

// "Loop" forever accepting new connections.
void
http_server(tcp::acceptor& acceptor, tcp::socket& socket)
{
  acceptor.async_accept(socket,
      [&](beast::error_code ec)
      {
          if(!ec)
              std::make_shared<http_connection>(std::move(socket))->start();
          http_server(acceptor, socket);
      });
}

int example_main(int argc, char* argv[])
{
    try
    {
        // Check command line arguments.
        if(argc != 3)
        {
            ex_out << "Usage: " << argv[0] << " <address> <port>\n";
            ex_out << "  For IPv4, try:\n";
            ex_out << "    receiver 0.0.0.0 80\n";
            ex_out << "  For IPv6, try:\n";
            ex_out << "    receiver 0::0 80\n";
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        ioc.run();
    }
    catch(std::exception const& e)
    {
        ex_out << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: http/server/small/http_server_small") {
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
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get(ex_port, "/count")));
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html>
<head><title>Request count</title></head>
<body>
<h1>Request count</h1>
<p>There have been 1 requests so far.</p>
</body>
</html>
)ex"));
}
}  // namespace ex_http_server_small_http_server_small

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/beast/example/http/server/async-local/http_server_async_local.cpp
namespace ex_http_server_async_local_http_server_async_local {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace beast = boost::beast;                      // from <boost/beast.hpp>
namespace http = beast::http;                        // from <boost/beast/http.hpp>
namespace net = boost::asio;                         // from <boost/asio.hpp>
using stream_protocol = net::local::stream_protocol; // from <boost/asio/local/stream_protocol.hpp>

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

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session>
{
    beast::basic_stream<
        stream_protocol,
        net::any_io_executor,
        beast::unlimited_rate_policy> stream_;
    beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    http::request<http::string_body> req_;

public:
    // Take ownership of the stream
    session(
        stream_protocol::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root)
        : stream_(std::move(socket))
        , doc_root_(doc_root)
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
        net::dispatch(stream_.get_executor(),
                      beast::bind_front_handler(
                          &session::do_read,
                          shared_from_this()));
    }

    void
    do_read()
    {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        req_ = {};

        // Set the timeout.
        stream_.expires_after(std::chrono::seconds(30));

        // Read a request
        http::async_read(stream_, buffer_, req_,
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

        // This means they closed the connection
        if(ec == http::error::end_of_stream)
            return do_close();

        if(ec)
            return fail(ec, "read");

        // Send the response
        send_response(
            handle_request(*doc_root_, std::move(req_)));
    }

    void
    send_response(http::message_generator&& msg)
    {
        bool keep_alive = msg.keep_alive();

        // Write the response
        beast::async_write(
            stream_,
            std::move(msg),
            beast::bind_front_handler(
                &session::on_write, shared_from_this(), keep_alive));
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

        // Read another request
        do_read();
    }

    void
    do_close()
    {
        // Send a TCP shutdown
        beast::error_code ec;
        stream_.socket().shutdown(stream_protocol::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
    net::io_context& ioc_;
    stream_protocol::acceptor acceptor_;
    std::shared_ptr<std::string const> doc_root_;

public:
    listener(
        net::io_context& ioc,
        stream_protocol::endpoint endpoint,
        std::shared_ptr<std::string const> const& doc_root)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc), std::move(endpoint))
        , doc_root_(doc_root)
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
            return; // To avoid infinite loop
        }
        else
        {
            // Create the session and run it
            std::make_shared<session>(
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
    try
    {
        // Check command line arguments.
        if (argc != 4)
        {
            ex_out <<
                "Usage: http-server-async-local <path> <doc_root> <threads>\n" <<
                "Example:\n" <<
                "    http-server-async-local /tmp/http.sock . 1\n";
            return EXIT_FAILURE;
        }
        auto const path = argv[1];
        auto const doc_root = std::make_shared<std::string>(argv[2]);
        auto const threads = std::max<int>(1, std::atoi(argv[3]));

        // The io_context is required for all I/O
        net::io_context ioc{threads};

        // Remove previous binding
        std::remove(path);

        // Create and launch a listening port
        std::make_shared<listener>(
            ioc,
            stream_protocol::endpoint{path},
            doc_root)->run();

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

TEST_CASE("example: http/server/async-local/http_server_async_local") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_local_path("beast.sock"); static const std::string ex_root = ex_doc_root();
    ex_args.push_back(ex_port);
    ex_args.push_back(ex_root);
    ex_args.push_back("1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = (ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (ex_http_get_local(ex_port, "/index.html")));
    CHECK(rc == 0);
#if !(!defined(BOOST_ASIO_HAS_LOCAL_SOCKETS))  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(received: 200 OK
Content-Type: text/html
<html><body>boost-m</body></html>
)ex"));
#endif
}
}  // namespace ex_http_server_async_local_http_server_async_local
#endif

// libs/beast/example/http/client/sync/http_client_sync.cpp
namespace ex_http_client_sync_http_client_sync {
namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = beast::http;       // from <boost/beast/http.hpp>
namespace net = boost::asio;        // from <boost/asio.hpp>
using tcp = net::ip::tcp;           // from <boost/asio/ip/tcp.hpp>

// Performs an HTTP GET and prints the response
int example_main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if(argc != 4 && argc != 5)
        {
            ex_out <<
                "Usage: http-client-sync <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
                "Example:\n" <<
                "    http-client-sync www.example.com 80 /\n" <<
                "    http-client-sync www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host = argv[1];
        auto const port = argv[2];
        auto const target = argv[3];
        int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // These objects perform our I/O
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        // Look up the domain name
        auto const results = resolver.resolve(host, port);

        // Make the connection on the IP address we get from a lookup
        stream.connect(results);

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, target, version};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Send the HTTP request to the remote host
        http::write(stream, req);

        // This buffer is used for reading and must be persisted
        beast::flat_buffer buffer;

        // Declare a container to hold the response
        http::response<http::dynamic_body> res;

        // Receive the HTTP response
        http::read(stream, buffer, res);

        // Write the message to standard out
        ex_out << res << std::endl;

        // Gracefully close the socket
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes
        // so don't bother reporting it.
        //
        if(ec && ec != beast::errc::not_connected)
            throw beast::system_error{ec};

        // If we get here then the connection is closed gracefully
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

TEST_CASE("example: http/client/sync/http_client_sync") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_http_server_sync_http_server_sync::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("/index.html");
    ex_args.push_back("1.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("HTTP/1.1 200 OK\r\n"
        "Server: Boost.Beast/361\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 34\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n"
        "\n", 124)));
#endif
}
}  // namespace ex_http_client_sync_http_client_sync

// libs/beast/example/http/client/async/http_client_async.cpp
namespace ex_http_client_async_http_client_async {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

public:
    // Objects are constructed with a strand to
    // ensure that handlers do not execute concurrently.
    explicit
    session(net::io_context& ioc)
        : resolver_(net::make_strand(ioc))
        , stream_(net::make_strand(ioc))
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* host,
        char const* port,
        char const* target,
        int version)
    {
        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, host);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

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

        // Set a timeout on the operation
        stream_.expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        stream_.async_connect(
            results,
            beast::bind_front_handler(
                &session::on_connect,
                shared_from_this()));
    }

    void
    on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
    {
        if(ec)
            return fail(ec, "connect");

        // Set a timeout on the operation
        stream_.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        http::async_write(stream_, req_,
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
        
        // Receive the HTTP response
        http::async_read(stream_, buffer_, res_,
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

        // Write the message to standard out
        ex_out << res_ << std::endl;

        // Gracefully close the socket
        stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc != 4 && argc != 5)
    {
        ex_out <<
            "Usage: http-client-async <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
            "Example:\n" <<
            "    http-client-async www.example.com 80 /\n" <<
            "    http-client-async www.example.com 80 / 1.0\n";
        return EXIT_FAILURE;
    }
    auto const host = argv[1];
    auto const port = argv[2];
    auto const target = argv[3];
    int version = argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run(host, port, target, version);

    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    return EXIT_SUCCESS;
    return 0;  // main's implicit return
}

TEST_CASE("example: http/client/async/http_client_async") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_http_server_sync_http_server_sync::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("/index.html");
    ex_args.push_back("1.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("HTTP/1.1 200 OK\r\n"
        "Server: Boost.Beast/361\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 34\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n"
        "\n", 124)));
#endif
}
}  // namespace ex_http_client_async_http_client_async

// libs/beast/example/http/client/awaitable/http_client_awaitable.cpp
namespace ex_http_client_awaitable_http_client_awaitable {
#if defined(BOOST_ASIO_HAS_CO_AWAIT)

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;

// Performs an HTTP GET and prints the response
net::awaitable<void>
do_session(std::string host, std::string port, std::string target, int version)
{
    auto executor = co_await net::this_coro::executor;
    auto resolver = net::ip::tcp::resolver{ executor };
    auto stream   = beast::tcp_stream{ executor };

    // Look up the domain name
    auto const results = co_await resolver.async_resolve(host, port);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    co_await stream.async_connect(results);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{ http::verb::get, target, version };
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Send the HTTP request to the remote host
    co_await http::async_write(stream, req);

    // This buffer is used for reading and must be persisted
    beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    co_await http::async_read(stream, buffer, res);

    // Write the message to standard out
    ex_out << res << std::endl;

    // Gracefully close the socket
    beast::error_code ec;
    stream.socket().shutdown(net::ip::tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if(ec && ec != beast::errc::not_connected)
        throw boost::system::system_error(ec, "shutdown");

    // If we get here then the connection is closed gracefully
}

//------------------------------------------------------------------------------

int example_main(int argc, char** argv)
{
    try
    {
        // Check command line arguments.
        if(argc != 4 && argc != 5)
        {
            ex_out << "Usage: http-client-awaitable <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n"
                      << "Example:\n"
                      << "    http-client-awaitable www.example.com 80 /\n"
                      << "    http-client-awaitable www.example.com 80 / 1.0\n";
            return EXIT_FAILURE;
        }
        auto const host   = argv[1];
        auto const port   = argv[2];
        auto const target = argv[3];
        auto const version =
            argc == 5 && !std::strcmp("1.0", argv[4]) ? 10 : 11;

        // The io_context is required for all I/O
        net::io_context ioc;

        // Launch the asynchronous operation
        net::co_spawn(
            ioc,
            do_session(host, port, target, version),
            // If the awaitable exists with an exception, it gets delivered here
            // as `e`. This can happen for regular errors, such as connection
            // drops.
            [](std::exception_ptr e)
            {
                if(e)
                    std::rethrow_exception(e);
            });

        // Run the I/O service. The call will return when
        // the get operation is complete.
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

TEST_CASE("example: http/client/awaitable/http_client_awaitable") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_http_server_sync_http_server_sync::example_main, std::vector<std::string>{"server", "127.0.0.1", p, ex_root}); }, ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("/index.html");
    ex_args.push_back("1.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("HTTP/1.1 200 OK\r\n"
        "Server: Boost.Beast/361\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 34\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n"
        "\n", 124)));
#endif
}
}  // namespace ex_http_client_awaitable_http_client_awaitable

#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)
// libs/beast/example/http/client/async-local/http_client_async_local.cpp
namespace ex_http_client_async_local_http_client_async_local {
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using stream_protocol = net::local::stream_protocol; // from <boost/asio/local/stream_protocol.hpp>

//------------------------------------------------------------------------------

// Report a failure
void
fail(beast::error_code ec, char const* what)
{
    ex_out << what << ": " << ec.message() << "\n";
}

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session>
{
    beast::basic_stream<
        stream_protocol,
        net::any_io_executor,
        beast::unlimited_rate_policy> stream_;
    beast::flat_buffer buffer_; // (Must persist between reads)
    http::request<http::empty_body> req_;
    http::response<http::string_body> res_;

public:
    // Objects are constructed with a strand to
    // ensure that handlers do not execute concurrently.
    explicit
    session(net::io_context& ioc)
        : stream_(ioc)
    {
    }

    // Start the asynchronous operation
    void
    run(
        char const* path,
        char const* host,
        char const* port,
        char const* target,
        int version)
    {
        // Set up an HTTP GET request message
        req_.version(version);
        req_.method(http::verb::get);
        req_.target(target);
        req_.set(http::field::host, std::string{host} + ":" + port);
        req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        // Make the connection on the IP address we get from a lookup
        stream_.async_connect(
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

        // Set a timeout on the operation
        stream_.expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        http::async_write(stream_, req_,
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
        
        // Receive the HTTP response
        http::async_read(stream_, buffer_, res_,
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

        // Write the message to standard out
        ex_out << res_ << std::endl;

        // Gracefully close the socket
        stream_.socket().shutdown(stream_protocol::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != beast::errc::not_connected)
            return fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully
    }
};

//------------------------------------------------------------------------------

int example_main(int argc, char** argv)
{
    // Check command line arguments.
    if(argc != 5 && argc != 6)
    {
        ex_out <<
            "Usage: http-client-async-local <path> <host> <port> <target> [<HTTP version: 1.0 or 1.1(default)>]\n" <<
            "Example:\n" <<
            "    http-client-async-local /tmp/http.sock localhost 80 /\n" <<
            "    http-client-async-local /tmp/http.sock localhost 80 / 1.0\n";
        return EXIT_FAILURE;
    }
    auto const path = argv[1];
    auto const host = argv[2];
    auto const port = argv[3];
    auto const target = argv[4];
    int version = argc == 6 && !std::strcmp("1.0", argv[5]) ? 10 : 11;

    // The io_context is required for all I/O
    net::io_context ioc;

    // Launch the asynchronous operation
    std::make_shared<session>(ioc)->run(path, host, port, target, version);

    // Run the I/O service. The call will return when
    // the get operation is complete.
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

TEST_CASE("example: http/client/async-local/http_client_async_local") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_local_path("beast.sock"); static const std::string ex_root = ex_doc_root(); ex_serve_async([p = ex_port] { (void)ex_call_main(&ex_http_server_async_local_http_server_async_local::example_main, std::vector<std::string>{"server", p, ex_root, "1"}); }, ex_port);
    ex_args.push_back(ex_port);
    ex_args.push_back("localhost");
    ex_args.push_back("80");
    ex_args.push_back("/index.html");
    ex_args.push_back("1.1");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("HTTP/1.1 200 OK\r\n"
        "Server: Boost.Beast/361\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 34\r\n"
        "\r\n"
        "<html><body>boost-m</body></html>\n"
        "\n", 124)));
#endif
}
}  // namespace ex_http_client_async_local_http_client_async_local
#endif
