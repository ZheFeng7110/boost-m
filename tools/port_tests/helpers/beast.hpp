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
