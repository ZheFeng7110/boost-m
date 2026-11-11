// mcpplibs.mwrap.boost.mqtt5 — the official examples (libs/mqtt5/example/),
// ported mechanically against an in-process MQTT 5 broker (the harness below)
// instead of broker.hivemq.com: each case starts the broker on a free port and
// passes it as argv (host, port, client id). The output must equal what the
// same code printed when built from upstream/boost headers alone. Changes:
// ec.message() prints no error as "Success" (ex_message; the C library's text
// differs between platforms); the programs that run until SIGINT stop as on
// SIGINT (their signal_set is cancelled) after their first message
// (receiver, publisher) or their fifth completion (multiflight_client). The TLS examples need OpenSSL and are excluded.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/asio/detail/config.hpp>
#include <boost/asio/coroutine.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <cstdlib>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.beast;
import mcpplibs.mwrap.boost.mqtt5;

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
// ---- mqtt5: an in-process MQTT 5 broker over loopback (boost-m tests) ----
// The examples talk to broker.hivemq.com; here they get this broker's free
// port as argv. It speaks enough MQTT 5 for them: CONNECT/CONNACK, PUBLISH at
// QoS 0/1/2 (PUBACK, PUBREC/PUBREL/PUBCOMP), SUBSCRIBE/SUBACK with + and #
// filters and retained messages, UNSUBSCRIBE, PINGREQ and DISCONNECT, over TCP
// or (ex_mqtt_broker(port, true)) over WebSocket with the "mqtt" subprotocol.
// Messages reach subscribers at QoS 0. One thread per connection.
struct ex_mqtt_conn {
    std::function<std::size_t(char*, std::size_t)> read_some;  // 0: closed
    std::function<void(const std::string&)> write;
    std::string in;
    std::mutex wm;
    bool read_exact(std::string& out, std::size_t n) {
        char buf[4096];
        while (in.size() < n) {
            std::size_t k = read_some(buf, sizeof buf);
            if (!k)
                return false;
            in.append(buf, k);
        }
        out = in.substr(0, n);
        in.erase(0, n);
        return true;
    }
    void send(const std::string& s) {
        std::lock_guard<std::mutex> l(wm);
        write(s);
    }
};
struct ex_mqtt_state {
    std::mutex m;
    std::vector<std::pair<ex_mqtt_conn*, std::string>> subs;
    std::map<std::string, std::string> retained;
};
inline std::string ex_mqtt_varint(std::size_t n) {
    std::string s;
    do {
        unsigned char b = n % 128;
        n /= 128;
        if (n)
            b |= 128;
        s += char(b);
    } while (n);
    return s;
}
inline std::string ex_mqtt_packet(unsigned char head, const std::string& body) {
    return std::string(1, char(head)) + ex_mqtt_varint(body.size()) + body;
}
inline std::string ex_mqtt_str(const std::string& s) {
    return std::string{char(s.size() >> 8), char(s.size() & 0xff)} + s;
}
inline bool ex_mqtt_match(const std::string& filter, const std::string& topic) {
    std::size_t f = 0, t = 0;
    while (f < filter.size()) {
        if (filter[f] == '#')
            return true;
        if (filter[f] == '+') {
            while (t < topic.size() && topic[t] != '/') ++t;
            ++f;
            continue;
        }
        if (t >= topic.size() || filter[f] != topic[t])
            return false;
        ++f, ++t;
    }
    return t == topic.size();
}
inline std::string ex_mqtt_publish(const std::string& topic, const std::string& payload, bool retain) {
    return ex_mqtt_packet(0x30 | (retain ? 1 : 0), ex_mqtt_str(topic) + std::string(1, '\0') + payload);
}
inline void ex_mqtt_session(ex_mqtt_conn& c, ex_mqtt_state& st) {
    auto u16 = [](const std::string& s, std::size_t i) {
        return std::size_t((unsigned char)s[i]) << 8 | (unsigned char)s[i + 1];
    };
    auto varint = [](const std::string& s, std::size_t& i) {
        std::size_t v = 0, mul = 1;
        for (;;) {
            unsigned char b = s[i++];
            v += (b & 127) * mul;
            mul *= 128;
            if (!(b & 128))
                return v;
        }
    };
    for (;;) {
        std::string h;
        if (!c.read_exact(h, 1))
            break;
        std::size_t len = 0, mul = 1;
        for (;;) {
            std::string b;
            if (!c.read_exact(b, 1))
                goto done;
            len += ((unsigned char)b[0] & 127) * mul;
            mul *= 128;
            if (!((unsigned char)b[0] & 128))
                break;
        }
        std::string body;
        if (!c.read_exact(body, len))
            break;
        unsigned char head = (unsigned char)h[0];
        switch (head >> 4) {
        case 1: {  // CONNECT
            // CONNACK with Receive Maximum 20: over WebSocket the client reads
            // the 5 octets of a property-less CONNACK, then the rest with a
            // zero-size read, which a websocket::stream only completes on the
            // next frame (the public brokers always send properties). The
            // Authentication Method of the CONNECT (enhanced authentication) is
            // echoed, which completes it in one step.
            std::size_t i = 2 + u16(body, 0) + 1 + 1 + 2;  // name, level, flags, keep alive
            std::size_t end = varint(body, i);
            end += i;
            std::string props = std::string("\x21\0\x14", 3);
            while (i < end) {
                unsigned char id = body[i++];
                if (id == 0x15) {  // Authentication Method
                    std::string m = body.substr(i + 2, u16(body, i));
                    props += '\x15' + ex_mqtt_str(m);
                    i += 2 + m.size();
                } else if (id == 0x11 || id == 0x27) {  // four-byte integers
                    i += 4;
                } else if (id == 0x21 || id == 0x22) {  // two-byte integers
                    i += 2;
                } else if (id == 0x17 || id == 0x19) {  // bytes
                    i += 1;
                } else if (id == 0x16) {  // binary data
                    i += 2 + u16(body, i);
                } else if (id == 0x26) {  // user property
                    i += 2 + u16(body, i);
                    i += 2 + u16(body, i);
                } else {
                    break;
                }
            }
            c.send(ex_mqtt_packet(0x20, std::string("\0\0", 2) + ex_mqtt_varint(props.size()) + props));
            break;
        }
        case 3: {  // PUBLISH
            int qos = (head >> 1) & 3;
            bool retain = head & 1;
            std::size_t i = 0;
            std::string topic = body.substr(2, u16(body, 0));
            i = 2 + topic.size();
            std::string pid;
            if (qos) {
                pid = body.substr(i, 2);
                i += 2;
            }
            std::size_t plen = varint(body, i);
            i += plen;
            std::string payload = body.substr(i);
            std::vector<ex_mqtt_conn*> to;
            {
                std::lock_guard<std::mutex> l(st.m);
                if (retain) {
                    if (payload.empty())
                        st.retained.erase(topic);
                    else
                        st.retained[topic] = payload;
                }
                for (auto& [conn, filter] : st.subs)
                    if (ex_mqtt_match(filter, topic))
                        to.push_back(conn);
            }
            if (qos == 1)
                c.send(ex_mqtt_packet(0x40, pid));
            else if (qos == 2)
                c.send(ex_mqtt_packet(0x50, pid));
            for (auto* conn : to)
                conn->send(ex_mqtt_publish(topic, payload, false));
            break;
        }
        case 6:  // PUBREL
            c.send(ex_mqtt_packet(0x70, body.substr(0, 2)));
            break;
        case 8: {  // SUBSCRIBE
            std::string pid = body.substr(0, 2);
            std::size_t i = 2;
            std::size_t plen = varint(body, i);
            i += plen;
            std::string codes;
            std::vector<std::string> retained;
            while (i < body.size()) {
                std::string filter = body.substr(i + 2, u16(body, i));
                i += 2 + filter.size();
                codes += char(body[i++] & 3);  // granted the requested QoS
                std::lock_guard<std::mutex> l(st.m);
                st.subs.push_back({&c, filter});
                for (auto& [topic, payload] : st.retained)
                    if (ex_mqtt_match(filter, topic))
                        retained.push_back(ex_mqtt_publish(topic, payload, true));
            }
            c.send(ex_mqtt_packet(0x90, pid + std::string(1, '\0') + codes));
            for (auto& p : retained)
                c.send(p);
            break;
        }
        case 10: {  // UNSUBSCRIBE
            std::string pid = body.substr(0, 2);
            std::size_t i = 2;
            std::size_t plen = varint(body, i);
            i += plen;
            std::string codes;
            while (i < body.size()) {
                std::string filter = body.substr(i + 2, u16(body, i));
                i += 2 + filter.size();
                codes += '\0';
                std::lock_guard<std::mutex> l(st.m);
                std::erase_if(st.subs, [&](auto& s) { return s.first == &c && s.second == filter; });
            }
            c.send(ex_mqtt_packet(0xB0, pid + std::string(1, '\0') + codes));
            break;
        }
        case 12:  // PINGREQ
            c.send(ex_mqtt_packet(0xD0, ""));
            break;
        case 14:  // DISCONNECT
            goto done;
        default:
            break;
        }
    }
done:
    std::lock_guard<std::mutex> l(st.m);
    std::erase_if(st.subs, [&](auto& s) { return s.first == &c; });
}
// Starts the broker on `port` in the background; it serves until the process
// exits. With `ws`, the clients connect over WebSocket; `retained` are the
// retained messages it starts with.
inline void ex_mqtt_broker(const std::string& port, bool ws = false,
                           std::map<std::string, std::string> retained = {}) {
    auto* st = new ex_mqtt_state;
    st->retained = std::move(retained);
    auto* ctx = new boost::asio::io_context;
    auto* acc = new boost::asio::ip::tcp::acceptor(
        *ctx, {boost::asio::ip::make_address("127.0.0.1"), static_cast<unsigned short>(std::stoi(port))});
    std::thread([st, ctx, acc, ws] {
        for (;;) {
            auto* sock = new boost::asio::ip::tcp::socket(*ctx);
            boost::system::error_code ec;
            acc->accept(*sock, ec);
            if (ec)
                return;
            std::thread([st, sock, ws] {
                auto* c = new ex_mqtt_conn;
                if (!ws) {
                    c->read_some = [sock](char* p, std::size_t n) {
                        boost::system::error_code e;
                        return sock->read_some(boost::asio::buffer(p, n), e);
                    };
                    c->write = [sock](const std::string& s) {
                        boost::system::error_code e;
                        boost::asio::write(*sock, boost::asio::buffer(s), e);
                    };
                } else {
                    namespace websocket = boost::beast::websocket;
                    auto* w = new websocket::stream<boost::asio::ip::tcp::socket&>(*sock);
                    w->set_option(websocket::stream_base::decorator([](websocket::response_type& r) {
                        r.set(boost::beast::http::field::sec_websocket_protocol, "mqtt");
                    }));
                    boost::system::error_code e;
                    w->accept(e);
                    if (e)
                        return;
                    w->binary(true);
                    c->read_some = [w](char* p, std::size_t n) -> std::size_t {
                        for (;;) {  // (a frame boundary reads 0 octets without an error)
                            boost::system::error_code e2;
                            std::size_t k = w->read_some(boost::asio::buffer(p, n), e2);
                            if (k || e2)
                                return k;
                        }
                    };
                    c->write = [w](const std::string& s) {
                        boost::system::error_code e2;
                        w->write(boost::asio::buffer(s), e2);
                    };
                }
                ex_mqtt_session(*c, *st);
                boost::system::error_code e3;  // the client waits for the close
                sock->shutdown(boost::asio::ip::tcp::socket::shutdown_both, e3);
                sock->close(e3);
            }).detach();
        }
    }).detach();
}
// The examples that run until SIGINT/SIGTERM wait on a signal_set; it is
// recorded here and cancelled where the case ends them (its handler then does
// what the signal would).
inline boost::asio::signal_set* ex_signals = nullptr;

// libs/mqtt5/example/hello_world_over_tcp.cpp
namespace ex_hello_world_over_tcp {
struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883; // 1883 is the default TCP MQTT port.
    std::string client_id = "boost_mqtt5_tester";
};

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    boost::asio::io_context ioc;

    //[init_tcp_client_with_logger
    // Construct the Client with ``__TCP_SOCKET__`` as the underlying stream and enabled logging.
    // Since we are not establishing a secure connection, set the TlsContext template parameter to std::monostate.
    boost::mqtt5::mqtt_client<
        boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
    > client(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));
    //]

    // If you want to use the Client without logging, initialise it with the following line instead.
    //boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);

    //[configure_tcp_client
    client.brokers(cfg.brokers, cfg.port) // Set the Broker to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .connect_property(boost::mqtt5::prop::session_expiry_interval, 60) // optional
        .async_run(boost::asio::detached); // Start the Client.
    //]

    //[publish_hello_world
    client.async_publish<boost::mqtt5::qos_e::at_most_once>(
        "boost-mqtt5/test", "Hello world!",
        boost::mqtt5::retain_e::yes, boost::mqtt5::publish_props {},
        [&client](boost::mqtt5::error_code ec) {
            ex_out << ex_message(ec) << std::endl;

            // Disconnnect the Client.
            client.async_disconnect(boost::asio::detached);
        }
    );
    //]

    ioc.run();
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: hello_world_over_tcp") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Success
)ex"));
}
}  // namespace ex_hello_world_over_tcp

// libs/mqtt5/example/hello_world_in_multithreaded_env.cpp
namespace ex_hello_world_in_multithreaded_env {
struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    // Create a thread pool with 4 threads.
    boost::asio::thread_pool tp(4);
    
    // Create an explicit strand from thread_pool's executor.
    // The strand guarantees a serialised handler execution regardless of the 
    // number of threads running in the thread_pool.
    boost::asio::strand strand = boost::asio::make_strand(tp.get_executor());

    // Create the Client with the explicit strand as the default associated executor.
    boost::mqtt5::mqtt_client<
        boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
    > client(strand, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    // Configure the client.
    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id); // Set the Client Identifier. (optional)

    // Start the Client.
    // The async_run function call must be posted/dispatched to the strand.
    boost::asio::dispatch(
        boost::asio::bind_executor(
            strand,
            [&client, &strand, &cfg] { 
                // Considering that the default associated executor of all completion handlers is the strand,
                // it is not necessary to explicitly bind it to async_run or other async_xxx's handlers.
                client.async_run(boost::asio::detached); // Start the Client.
            }
        )
    );
    
    // The async_publish function call must be posted/dispatched to the strand.
    // The associated executor of async_publish's completion handler must be the same strand.
    boost::asio::dispatch(
        boost::asio::bind_executor(
            strand,
            [&client, &strand, &cfg] { 
                assert(strand.running_in_this_thread());

                client.async_publish<boost::mqtt5::qos_e::at_least_once>(
                    "boost-mqtt5/test", "Hello World!", boost::mqtt5::retain_e::no,
                    boost::mqtt5::publish_props {},
                    // You may bind the strand to this handler, but it is not necessary
                    // as the strand is already the default associated handler.
                    [&client, &strand](
                        boost::mqtt5::error_code ec, boost::mqtt5::reason_code rc,
                        boost::mqtt5::puback_props props
                    ) {
                        assert(strand.running_in_this_thread());

                        ex_out << ex_message(ec) << std::endl;
                        ex_out << rc.message() << std::endl;

                        // Stop the Client.
                        client.cancel();
                    }
                );
            }
        )
    );

    tp.join();

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: hello_world_in_multithreaded_env") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Success
The operation completed successfully
)ex"));
}
}  // namespace ex_hello_world_in_multithreaded_env

// libs/mqtt5/example/hello_world_in_coro_multithreaded_env.cpp
namespace ex_hello_world_in_coro_multithreaded_env {
#ifdef BOOST_ASIO_HAS_CO_AWAIT

//[hello_world_in_coro_multithreaded_env



struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

// client_type with logging enabled
using client_type = boost::mqtt5::mqtt_client<
    boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
>;

// client_type without logging
//using client_type = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>;

// Modified completion token that will prevent co_await from throwing exceptions.
constexpr auto use_nothrow_awaitable = boost::asio::as_tuple(boost::asio::deferred);

boost::asio::awaitable<void> publish_hello_world(
    const config& cfg, client_type& client,
    const boost::asio::strand<boost::asio::thread_pool::executor_type>& strand
) {
    // Confirmation that the coroutine running in the strand.
    assert(strand.running_in_this_thread());

    // All these function calls will be executed by the strand that is executing the coroutine.
    // All the completion handler's associated executors will be that same strand
    // because the Client was constructed with it as the default associated executor.
    client.brokers(cfg.brokers, cfg.port) // Set the Broker to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the Client.

    auto&& [ec, rc, puback_props] = co_await client.async_publish<boost::mqtt5::qos_e::at_least_once>(
        "boost-mqtt5/test" /* topic */, "Hello world!" /* payload*/, boost::mqtt5::retain_e::no,
        boost::mqtt5::publish_props {}, use_nothrow_awaitable);

    co_await client.async_disconnect(use_nothrow_awaitable);
    co_return;
}

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    // Create a thread pool with 4 threads.
    boost::asio::thread_pool tp(4);

    // Create an explicit strand from io_context's executor.
    // The strand guarantees a serialised handler execution regardless of the 
    // number of threads running in the io_context.
    boost::asio::strand strand = boost::asio::make_strand(tp.get_executor());

    // Create the Client with the explicit strand as the default associated executor.
    client_type client(strand, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    // Spawn the coroutine.
    // The executor that executes the coroutine must be the same executor
    // that is the Client's default associated executor.
    co_spawn(
        strand,
        publish_hello_world(cfg, client, strand),
        [](std::exception_ptr e) {
            if (e)
                std::rethrow_exception(e);
        }
    );

    tp.join();

    return 0;
    return 0;  // main's implicit return
}

//]

#else


int example_main() {
    ex_out << "This example requires C++20 standard to compile and run" << std::endl;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: hello_world_in_coro_multithreaded_env") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_hello_world_in_coro_multithreaded_env

// libs/mqtt5/example/timeout_with_awaitable_operators.cpp
namespace ex_timeout_with_awaitable_operators {
#ifdef BOOST_ASIO_HAS_CO_AWAIT

//[timeout_with_awaitable_operators




struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

// Modified completion token that will prevent co_await from throwing exceptions.
constexpr auto use_nothrow_awaitable = boost::asio::as_tuple(boost::asio::deferred);

// client_type with logging enabled
using client_type = boost::mqtt5::mqtt_client<
    boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
>;

// client_type without logging
//using client_type = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>;

boost::asio::awaitable<void> send_over_mqtt(
    const config& cfg, client_type& client
) {
    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the Client.

    auto&& [pec, prc, puback_props] = co_await client.async_publish<boost::mqtt5::qos_e::at_least_once>(
        "boost-mqtt5/test", "Hello World!",
        boost::mqtt5::retain_e::no, boost::mqtt5::publish_props {},
        use_nothrow_awaitable
    );

    co_await client.async_disconnect(use_nothrow_awaitable);
    co_return;
}

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    boost::asio::io_context ioc;

    co_spawn(
        ioc,
        [&ioc, &cfg]() -> boost::asio::awaitable<void> {
        // Initialise the Client to connect to the Broker over TCP.
        client_type client(ioc);

        // You can also initialise the Client and its logger with a specific log_level (default log_level::info).
        //client_type client(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::debug));

        // Construct the timer.
        boost::asio::steady_timer timer(ioc, std::chrono::seconds(5));

        using namespace boost::asio::experimental::awaitable_operators;
        auto res = co_await (
            send_over_mqtt(cfg, client) ||
            timer.async_wait(boost::asio::as_tuple(boost::asio::use_awaitable))
        );

        // The timer expired first. The client is cancelled.
        if (res.index() == 1)
            ex_out << "Send over MQTT timed out!" << std::endl;
        // send_over_mqtt completed first. The timer is cancelled.
        else
            ex_out << "Send over MQTT completed!" << std::endl;
        
        },
        [](std::exception_ptr e) {
            if (e)
                std::rethrow_exception(e);
        }
    );

    ioc.run();
    return 0;  // main's implicit return
}

//]

#else


int example_main() {
    ex_out << "This example requires C++20 standard to compile and run" << std::endl;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: timeout_with_awaitable_operators") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Send over MQTT completed!
)ex"));
}
}  // namespace ex_timeout_with_awaitable_operators

// libs/mqtt5/example/timeout_with_parallel_group.cpp
namespace ex_timeout_with_parallel_group {
struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    boost::asio::io_context ioc;

    // Construct the Client with ``__TCP_SOCKET__`` as the underlying stream and enabled logging.
    // Since we are not establishing a secure connection, set the TlsContext template parameter to std::monostate.
    boost::mqtt5::mqtt_client<
        boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
    > client(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    // If you want to use the Client without logging, initialise it with the following line instead.
    //boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket> client(ioc);

    // Construct the timer.
    boost::asio::steady_timer timer(ioc, std::chrono::seconds(5));

    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the Client.

    // Subscribe to a Topic.
    client.async_subscribe(
        { "test" /* Topic */}, boost::mqtt5::subscribe_props {},
        [](boost::mqtt5::error_code ec, std::vector<boost::mqtt5::reason_code> rcs, boost::mqtt5::suback_props) {
            ex_out << "[subscribe ec]: " << ex_message(ec) << std::endl;
            ex_out << "[subscribe rc]: " << rcs[0].message() << std::endl;
        }
    );

    // Create a parallel group to wait up to 5 seconds to receive a message
    // using client.async_receive(...).
    boost::asio::experimental::make_parallel_group(
        timer.async_wait(boost::asio::deferred),
        client.async_receive(boost::asio::deferred)
    ).async_wait(
        boost::asio::experimental::wait_for_one(),
        [&client](
            std::array<std::size_t, 2> ord, // Completion order
            boost::mqtt5::error_code /* timer_ec */, // timer.async_wait(...) handler signature
            // client.async_receive(...) handler signature
            boost::mqtt5::error_code receive_ec, 
            std::string topic, std::string payload, boost::mqtt5::publish_props /* props */
        ) {
            if (ord[0] == 1) {
                ex_out << "Received a message!" << std::endl;
                ex_out << "[receive ec]: " << receive_ec.message() << std::endl;
                ex_out << "[receive topic]: " << topic << std::endl;
                ex_out << "[receive payload]: " << payload << std::endl;
            }
            else
                ex_out << "Timed out! Did not receive a message within 5 seconds." << std::endl;

            client.cancel();
        }
    );

    ioc.run();
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: timeout_with_parallel_group") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex([subscribe ec]: Success
[subscribe rc]: The subscription is accepted with maximum QoS sent at 2
Timed out! Did not receive a message within 5 seconds.
)ex"));
}
}  // namespace ex_timeout_with_parallel_group

// libs/mqtt5/example/hello_world_over_websocket_tcp.cpp
namespace ex_hello_world_over_websocket_tcp {
struct config {
    std::string brokers = "broker.hivemq.com/mqtt"; // Path example: localhost/mqtt
    uint16_t port = 8000; // 8083 is the default Webscoket/TCP MQTT port. However, HiveMQ's public broker uses 8000 instead.
    std::string client_id = "boost_mqtt5_tester";
};

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    boost::asio::io_context ioc;

    // Construct the Client with WebSocket/TCP as the underlying stream and enabled logging.
    // Since we are not establishing a secure connection, set the TlsContext template parameter to std::monostate.
    boost::mqtt5::mqtt_client<
        boost::beast::websocket::stream<boost::asio::ip::tcp::socket>,
        std::monostate,
        boost::mqtt5::logger
    > client(ioc, {}, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    // If you want to use the Client without logging, initialise it with the following line instead.
    //boost::mqtt5::mqtt_client<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> client(ioc);

    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the Client.

    client.async_publish<boost::mqtt5::qos_e::at_most_once>(
        "boost-mqtt5/test", "Hello world!",
        boost::mqtt5::retain_e::no, boost::mqtt5::publish_props {},
        [&client](boost::mqtt5::error_code ec) {
            ex_out << ex_message(ec) << std::endl;
            client.async_disconnect(boost::asio::detached);
        }
    );

    ioc.run();
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: hello_world_over_websocket_tcp") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port, true);
    ex_args.push_back("127.0.0.1/mqtt");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Success
)ex"));
}
}  // namespace ex_hello_world_over_websocket_tcp

// libs/mqtt5/example/multiflight_client.cpp
namespace ex_multiflight_client {
struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    boost::asio::io_context ioc;

    // Construct the Client with ``__TCP_SOCKET__`` as the underlying stream and enabled logging.
    // Since we are not establishing a secure connection, set the TlsContext template parameter to std::monostate.
    boost::mqtt5::mqtt_client<
        boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
    > client(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the client.

    // Publish with QoS 2 five times in a row without waiting for the handler
    // of the previous async_publish call to be invoked.
    for (auto i = 1; i <= 5; ++i)
        client.async_publish<boost::mqtt5::qos_e::exactly_once>(
            "boost-mqtt5/test", "Hello world!",
            boost::mqtt5::retain_e::no, boost::mqtt5::publish_props {},
            [i](boost::mqtt5::error_code ec, boost::mqtt5::reason_code rc, boost::mqtt5::pubcomp_props) {
                ex_out << "Publish number " << i << " completed with: " << std::endl;
                ex_out << "\t ec: " << ex_message(ec) << std::endl;
                ex_out << "\t rc: " << rc.message() << std::endl;
                if (i == 5) ex_signals->cancel();  // boost-m: as on SIGINT
            }
        );

    // We can stop the Client by using signals.
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM); ex_signals = &signals;
    signals.async_wait([&client](boost::mqtt5::error_code, int) {
        client.async_disconnect(boost::asio::detached);
    });

    ioc.run();
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: multiflight_client") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Publish number 1 completed with: 
	 ec: Success
	 rc: The operation completed successfully
Publish number 2 completed with: 
	 ec: Success
	 rc: The operation completed successfully
Publish number 3 completed with: 
	 ec: Success
	 rc: The operation completed successfully
Publish number 4 completed with: 
	 ec: Success
	 rc: The operation completed successfully
Publish number 5 completed with: 
	 ec: Success
	 rc: The operation completed successfully
)ex"));
}
}  // namespace ex_multiflight_client

// libs/mqtt5/example/receiver.cpp
namespace ex_receiver {
#ifdef BOOST_ASIO_HAS_CO_AWAIT

//[receiver




struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

// Modified completion token that will prevent co_await from throwing exceptions.
constexpr auto use_nothrow_awaitable = boost::asio::as_tuple(boost::asio::deferred);

// client_type with logging enabled
using client_type = boost::mqtt5::mqtt_client<
    boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
>;

// client_type without logging
//using client_type = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>;

boost::asio::awaitable<bool> subscribe(client_type& client) {
    // Configure the request to subscribe to a Topic.
    boost::mqtt5::subscribe_topic sub_topic = boost::mqtt5::subscribe_topic{
        "test" /* topic */,
        boost::mqtt5::subscribe_options {
            boost::mqtt5::qos_e::exactly_once, // All messages will arrive at QoS 2.
            boost::mqtt5::no_local_e::no, // Forward message from Clients with same ID.
            boost::mqtt5::retain_as_published_e::retain, // Keep the original RETAIN flag.
            boost::mqtt5::retain_handling_e::send // Send retained messages when the subscription is established.
        }
    };

    // Subscribe to a single Topic.
    auto&& [ec, sub_codes, sub_props] = co_await client.async_subscribe(
        sub_topic, boost::mqtt5::subscribe_props {}, use_nothrow_awaitable
    );
    // Note: you can subscribe to multiple Topics in one mqtt_client::async_subscribe call.

    // An error can occur as a result of:
        //  a) wrong subscribe parameters
        //  b) mqtt_client::cancel is called while the Client is in the process of subscribing
    if (ec)
        ex_out << "Subscribe error occurred: " << ex_message(ec) << std::endl;
    else
        ex_out << "Result of subscribe request: " << sub_codes[0].message() << std::endl;

    co_return !ec && !sub_codes[0]; // True if the subscription was successfully established.
}

boost::asio::awaitable<void> subscribe_and_receive(
    const config& cfg, client_type& client
) {
    // Configure the Client.
    // It is mandatory to call brokers() and async_run() to configure the Brokers to connect to and start the Client.
    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the client.

    // Before attempting to receive an Application Message from the Topic we just subscribed to,
    // it is advisable to verify that the subscription succeeded.
    // It is not recommended to call mqtt_client::async_receive if you do not have any
    // subscription established as the corresponding handler will never be invoked.
    if (!(co_await subscribe(client)))
        co_return;

    for (;;) {
        // Receive an Appplication Message from the subscribed Topic(s).
        auto&& [ec, topic, payload, publish_props] = co_await client.async_receive(use_nothrow_awaitable);

        if (ec == boost::mqtt5::client::error::session_expired) {
            // The Client has reconnected, and the prior session has expired.
            // As a result, any previous subscriptions have been lost and must be reinstated.
            if (co_await subscribe(client))
                continue;
            else
                break;
        } else if (ec)
            break;

        ex_out << "Received message from the Broker" << std::endl;
        ex_out << "\t topic: " << topic << std::endl;
        ex_out << "\t payload: " << payload << std::endl;
        ex_signals->cancel();  // boost-m: as on SIGINT
    }

    co_return;
}

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    // Initialise execution context.
    boost::asio::io_context ioc;

    // Initialise the Client to connect to the Broker over TCP.
    client_type client(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    // Set up signals to stop the program on demand.
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM); ex_signals = &signals;
    signals.async_wait([&client](boost::mqtt5::error_code /* ec */, int /* signal */) {
        // After we are done with publishing all the messages, cancel the timer and the Client.
        // Alternatively, use mqtt_client::async_disconnect.
        client.cancel();
    });

    // Spawn the coroutine.
    co_spawn(
        ioc,
        subscribe_and_receive(cfg, client),
        [](std::exception_ptr e) {
            if (e)
                std::rethrow_exception(e);
        }
    );

    // Start the execution.
    ioc.run();
    return 0;  // main's implicit return
}

//]

#else


int example_main() {
    ex_out << "This example requires C++20 standard to compile and run" << std::endl;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: receiver") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port, false, {{"test", "a retained message"}});
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Result of subscribe request: The subscription is accepted with maximum QoS sent at 2
Received message from the Broker
	 topic: test
	 payload: a retained message
)ex"));
}
}  // namespace ex_receiver

// libs/mqtt5/example/publisher.cpp
namespace ex_publisher {
#ifdef BOOST_ASIO_HAS_CO_AWAIT

//[publisher



struct config {
    std::string brokers = "broker.hivemq.com";
    uint16_t port = 1883;
    std::string client_id = "boost_mqtt5_tester";
};

// Modified completion token that will prevent co_await from throwing exceptions.
constexpr auto use_nothrow_awaitable = boost::asio::as_tuple(boost::asio::deferred);

// client_type with logging enabled
using client_type = boost::mqtt5::mqtt_client<
    boost::asio::ip::tcp::socket, std::monostate /* TlsContext */, boost::mqtt5::logger
>;

// client_type without logging
//using client_type = boost::mqtt5::mqtt_client<boost::asio::ip::tcp::socket>;

int next_sensor_reading() {
    srand(static_cast<unsigned int>(std::time(0)));
    return rand() % 100;
}

boost::asio::awaitable<void> publish_sensor_readings(
    const config& cfg, client_type& client, boost::asio::steady_timer& timer
) {
    // Configure the Client.
    // It is mandatory to call brokers() and async_run() to configure the Brokers to connect to and start the Client.
    client.brokers(cfg.brokers, cfg.port) // Broker that we want to connect to.
        .credentials(cfg.client_id) // Set the Client Identifier. (optional)
        .async_run(boost::asio::detached); // Start the Client.

    for (;;) {
        // Get the next sensor reading.
        auto reading = std::to_string(next_sensor_reading());

        // Publish the sensor reading with QoS 1.
        auto&& [ec, rc, props] = co_await client.async_publish<boost::mqtt5::qos_e::at_least_once>(
            "boost-mqtt5/test" /* topic */, reading /* payload */,
            boost::mqtt5::retain_e::no, boost::mqtt5::publish_props {}, use_nothrow_awaitable
        );
        // An error can occur as a result of:
        //  a) wrong publish parameters
        //  b) mqtt_client::cancel is called while the Client is publishing the message
        //     resulting in cancellation.
        if (ec) {
            ex_out << "Publish error occurred: " << ex_message(ec) << std::endl;
            break;
        }

        // Reason code is the reply from the server presenting the result of the publish operation.
        ex_out << "Result of publish request: " << rc.message() << std::endl;
        if (!rc)
            ex_out << "Published sensor reading: " << reading << std::endl;

        // Wait 5 seconds before publishing the next reading.
        ex_signals->cancel();  // boost-m: as on SIGINT
        timer.expires_after(std::chrono::seconds(5));
        auto&& [tec] = co_await timer.async_wait(use_nothrow_awaitable);

        // An error occurred if we cancelled the timer.
        if (tec)
            break;
    }

    co_return;
}

int example_main(int argc, char** argv) {
    config cfg;

    if (argc == 4) {
        cfg.brokers = argv[1];
        cfg.port = uint16_t(std::stoi(argv[2]));
        cfg.client_id = argv[3];
    }

    // Initialise execution context.
    boost::asio::io_context ioc;

    // Initialise the Client to connect to the Broker over TCP.
    client_type client(ioc, {} /* tls_context */, boost::mqtt5::logger(boost::mqtt5::log_level::info));

    // Initialise the timer.
    boost::asio::steady_timer timer(ioc);

    // Set up signals to stop the program on demand.
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM); ex_signals = &signals;
    signals.async_wait([&client, &timer](boost::mqtt5::error_code /* ec */, int /* signal */) {
        // After we are done with publishing all the messages, cancel the timer and the Client.
        // Alternatively, use mqtt_client::async_disconnect.
        timer.cancel();
        client.cancel();
    });

    // Spawn the coroutine.
    co_spawn(
        ioc.get_executor(),
        publish_sensor_readings(cfg, client, timer),
        [](std::exception_ptr e) {
            if (e)
                std::rethrow_exception(e);
        }
    );

    // Start the execution.
    ioc.run();
    return 0;  // main's implicit return
}

//]

#else


int example_main() {
    ex_out << "This example requires C++20 standard to compile and run" << std::endl;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: publisher") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_port = ex_free_port(); ex_mqtt_broker(ex_port);
    ex_args.push_back("127.0.0.1");
    ex_args.push_back(ex_port);
    ex_args.push_back("boost_mqtt5_tester");
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Result of publish request: The operation completed successfully
Published sensor reading: 99
)ex"));
#endif
}
}  // namespace ex_publisher
