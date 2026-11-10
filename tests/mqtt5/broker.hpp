// boost-m tests: the in-process MQTT 5 broker of tests/mqtt5/examples.cpp
// (written by the generator from the same helper code), for doc.cpp and
// custom_types.cpp. Include after the imports.
#pragma once
inline std::string ex_free_port() {
    boost::asio::io_context c;
    boost::asio::ip::tcp::acceptor a(c, {boost::asio::ip::make_address("127.0.0.1"), 0});
    return std::to_string(a.local_endpoint().port());
}
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
