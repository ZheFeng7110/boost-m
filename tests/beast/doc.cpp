// mcpplibs.mwrap.boost.beast — the reference pages of libs/beast/doc/qbk that show
// code inline (the [snippet_*] blocks they also import come from test/doc/,
// which the release archive does not ship): io_context, the HTTP primer
// messages, the message containers and body types, serializer and parser
// stream operations, chunked encoding (the page's message and the output it
// lists for the chunk callbacks), the WebSocket handshake (the RFC 6455 key and
// accept pair), the Body and BuffersGenerator concepts, and the message
// container design. Everything runs over beast::test::stream, in memory.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.beast;

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

namespace {
template <class Message>
std::string wire(Message const& m) {
    std::ostringstream os;
    os << m;
    return os.str();
}
}  // namespace

TEST_CASE("doc: using_io") {
    net::io_context ioc;
    CHECK_FALSE(ioc.stopped());
    CHECK(ioc.run() == 0);  // no work
}

TEST_CASE("doc: protocol-primer") {
    http::request<http::empty_body> req;
    req.version(11);
    req.method(http::verb::get);
    req.target("/");
    req.set(http::field::user_agent, "Beast");
    CHECK(wire(req) == "GET / HTTP/1.1\r\nUser-Agent: Beast\r\n\r\n");

    http::response<http::string_body> res;
    res.version(11);
    res.result(http::status::ok);
    res.set(http::field::server, "Beast");
    res.body() = "Hello, world!";
    res.prepare_payload();
    CHECK(wire(res) == "HTTP/1.1 200 OK\r\nServer: Beast\r\nContent-Length: 13\r\n\r\nHello, world!");

    // the fields the table lists
    http::request<http::empty_body> up{http::verb::get, "/", 11};
    up.set(http::field::connection, "upgrade");
    up.set(http::field::upgrade, "websocket");
    CHECK(up.keep_alive());
    CHECK(http::token_list{up[http::field::connection]}.exists("upgrade"));
    res.chunked(true);
    CHECK(res.chunked());
    CHECK(res[http::field::transfer_encoding] == "chunked");
    CHECK(res.find(http::field::content_length) == res.end());
}

TEST_CASE("doc: message-containers") {
    static_assert(std::is_same_v<http::request<http::string_body>,
                                 http::message<true, http::string_body, http::fields>>);
    static_assert(std::is_same_v<http::response<http::string_body>,
                                 http::message<false, http::string_body, http::fields>>);
    static_assert(std::is_same_v<http::request_header<>, http::header<true, http::fields>>);
    static_assert(std::is_same_v<http::response_header<>, http::header<false, http::fields>>);
    static_assert(std::is_base_of_v<http::request_header<>, http::request<http::empty_body>>);

    // http_snippet_2: the request the page prints
    http::request<http::empty_body> req;
    req.version(11);
    req.method(http::verb::get);
    req.target("/index.htm");
    req.set(http::field::accept, "text/html");
    req.set(http::field::user_agent, "Beast");
    CHECK(wire(req) == "GET /index.htm HTTP/1.1\r\nAccept: text/html\r\nUser-Agent: Beast\r\n\r\n");

    // http_snippet_3: the response
    http::response<http::string_body> res;
    res.version(11);
    res.result(http::status::ok);
    res.set(http::field::server, "Beast");
    res.body() = "Hello, world!";
    res.prepare_payload();
    CHECK(wire(res) == "HTTP/1.1 200 OK\r\nServer: Beast\r\nContent-Length: 13\r\n\r\nHello, world!");

    // the body types of the table
    http::response<http::vector_body<char>> vr{http::status::ok, 11};
    vr.body() = {'a', 'b'};
    vr.prepare_payload();
    CHECK(vr[http::field::content_length] == "2");
    char text[] = "span";
    http::response<http::span_body<char>> sr{http::status::ok, 11};
    sr.body() = boost::span<char>(text, 4);
    sr.prepare_payload();
    CHECK(wire(sr).ends_with("\r\n\r\nspan"));
    http::response<http::dynamic_body> dr{http::status::ok, 11};
    beast::ostream(dr.body()) << "dynamic";
    dr.prepare_payload();
    CHECK(wire(dr).ends_with("dynamic"));
    http::response<http::buffer_body> br{http::status::ok, 11};
    br.body().data = text;
    br.body().size = 4;
    br.body().more = false;
    CHECK(br.body().size == 4);
    http::response<http::empty_body> er{http::status::ok, 11};
    er.prepare_payload();
    CHECK(wire(er) == "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");

    // file_body serializes an open file
    auto path = (std::filesystem::temp_directory_path() / "boost-m-beast-file-body.txt").string();
    std::ofstream(path, std::ios::binary) << "from a file";
    http::response<http::file_body> fr{http::status::ok, 11};
    beast::error_code ec;
    fr.body().open(path.c_str(), beast::file_mode::scan, ec);
    REQUIRE_FALSE(ec);
    fr.prepare_payload();
    CHECK(fr[http::field::content_length] == "11");
    net::io_context ioc;
    beast::test::stream out(ioc), in(ioc);
    out.connect(in);
    http::write(out, fr);
    CHECK(in.str().ends_with("\r\n\r\nfrom a file"));
    fr.body().close();
    std::filesystem::remove(path);
}

TEST_CASE("doc: serializer-stream-operations") {
    static_assert(std::is_same_v<http::request_serializer<http::string_body>,
                                 http::serializer<true, http::string_body, http::fields>>);
    static_assert(std::is_same_v<http::response_serializer<http::string_body>,
                                 http::serializer<false, http::string_body, http::fields>>);
    net::io_context ioc;
    beast::test::stream out(ioc), in(ioc);
    out.connect(in);

    http::response<http::string_body> res{http::status::ok, 11};
    res.body() = "body octets";
    res.prepare_payload();

    // write: everything
    http::response_serializer<http::string_body> sr{res};
    http::write(out, sr);
    CHECK(sr.is_done());
    CHECK(in.str() == wire(res));
    in.clear();

    // write_header, then write_some until done
    http::response_serializer<http::string_body> sr2{res};
    http::write_header(out, sr2);
    CHECK(sr2.is_header_done());
    CHECK(in.str().ends_with("\r\n\r\n"));
    out.write_size(3);  // a few octets per write_some
    while (!sr2.is_done())
        http::write_some(out, sr2);
    CHECK(in.str() == wire(res));
    in.clear();

    // async_write
    beast::error_code result = net::error::would_block;
    std::size_t n = 0;
    http::async_write(out, res, [&](beast::error_code ec, std::size_t bytes) {
        result = ec;
        n = bytes;
    });
    ioc.run();
    CHECK_FALSE(result);
    CHECK(n == wire(res).size());
    CHECK(in.str() == wire(res));
}

TEST_CASE("doc: parser-stream-operations") {
    static_assert(std::is_same_v<http::request_parser<http::string_body>,
                                 http::parser<true, http::string_body, std::allocator<char>>>);
    static_assert(std::is_base_of_v<http::basic_parser<false>, http::response_parser<http::string_body>>);
    net::io_context ioc;
    const std::string msg = "POST /submit HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\n\r\nhello";

    // read: everything into a parser
    {
        beast::test::stream in(ioc, msg);
        beast::flat_buffer buf;
        http::request_parser<http::string_body> p;
        http::read(in, buf, p);
        CHECK(p.is_done());
        CHECK(p.get().method() == http::verb::post);
        CHECK(p.get().body() == "hello");
    }
    // read_header, then read_some for the body
    {
        beast::test::stream in(ioc, msg);
        in.read_size(4);
        beast::flat_buffer buf;
        http::request_parser<http::string_body> p;
        http::read_header(in, buf, p);
        CHECK(p.is_header_done());
        CHECK(p.content_length() == std::uint64_t(5));
        CHECK(p.get()[http::field::host] == "x");
        while (!p.is_done())
            http::read_some(in, buf, p);
        CHECK(p.get().body() == "hello");
    }
    // async_read into a message
    {
        beast::test::stream in(ioc, msg);
        beast::flat_buffer buf;
        http::request<http::string_body> req;
        beast::error_code result = net::error::would_block;
        http::async_read(in, buf, req, [&](beast::error_code ec, std::size_t) { result = ec; });
        ioc.restart();
        ioc.run();
        CHECK_FALSE(result);
        CHECK(req.target() == "/submit");
        CHECK(req.body() == "hello");
    }
}

TEST_CASE("doc: chunked-encoding") {
    const std::string_view expected =
        "HTTP/1.1 200 OK\r\n"
        "Server: test\r\n"
        "Trailer: Expires, Content-MD5\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "5\r\n"
        "First\r\n"
        "d;quality=1.0\r\n"
        "Hello, world!\r\n"
        "e;file=abc.txt;quality=0.7\r\n"
        "The Next Chunk\r\n"
        "8;last\r\n"
        "Last one\r\n"
        "0\r\n"
        "Expires: never\r\n"
        "Content-MD5: f4a5c16584f03d90\r\n"
        "\r\n";
    net::io_context ioc;

    // Serializing: the header through an empty_body serializer, then the
    // chunks with make_chunk / chunk_extensions and the last chunk with trailers.
    {
        beast::test::stream out(ioc), in(ioc);
        out.connect(in);
        http::response<http::empty_body> res{http::status::ok, 11};
        res.set(http::field::server, "test");
        res.set(http::field::trailer, "Expires, Content-MD5");
        res.chunked(true);
        http::response_serializer<http::empty_body> sr{res};
        http::write_header(out, sr);
        net::write(out, http::make_chunk(net::buffer("First", 5)));
        http::chunk_extensions ext;
        ext.insert("quality", "1.0");
        net::write(out, http::make_chunk(net::buffer("Hello, world!", 13), ext));
        ext.clear();
        ext.insert("file", "abc.txt");
        ext.insert("quality", "0.7");
        net::write(out, http::make_chunk(net::buffer("The Next Chunk", 14), std::move(ext)));
        ext.clear();
        ext.insert("last");
        net::write(out, http::make_chunk(net::buffer("Last one", 8), std::move(ext)));
        http::fields trailers;
        trailers.set(http::field::expires, "never");
        trailers.set("Content-MD5", "f4a5c16584f03d90");
        net::write(out, http::make_chunk_last(trailers));
        CHECK(in.str() == expected);
    }

    // Parsing: on_chunk_header / on_chunk_body callbacks print what the page lists.
    {
        beast::test::stream in(ioc, std::string(expected));
        beast::flat_buffer buf;
        http::response_parser<http::empty_body> p;
        p.merge_all_trailers(true);  // Expires is not a well-known trailer field
        std::ostringstream os;
        std::string chunk;
        http::read_header(in, buf, p);
        auto header_cb = [&](std::uint64_t size, beast::string_view extensions, beast::error_code& ev) {
            http::chunk_extensions ce;
            ce.parse(extensions, ev);
            if (ev)
                return;
            if (size > (std::numeric_limits<std::size_t>::max)()) {
                ev = http::error::body_limit;
                return;
            }
            chunk.reserve(static_cast<std::size_t>(size));
            chunk.clear();
            for (auto const& e : ce) {
                os << "Extension: " << e.first;
                if (!e.second.empty())
                    os << " = " << e.second;
                os << "\n";
            }
        };
        p.on_chunk_header(header_cb);
        auto body_cb = [&](std::uint64_t remain, beast::string_view body, beast::error_code& ev) {
            if (remain == body.size())
                ev = http::error::end_of_chunk;
            chunk.append(body.data(), body.size());
            return body.size();
        };
        p.on_chunk_body(body_cb);
        while (!p.is_done()) {
            beast::error_code ec;
            http::read(in, buf, p, ec);
            if (ec == http::error::end_of_chunk) {
                os << "Chunk Body: " << chunk << "\n";
                continue;
            }
            REQUIRE_FALSE(ec);
        }
        for (beast::string_view name : {"Expires", "Content-MD5"})
            os << name << ": " << p.get()[name] << "\n";
        CHECK(os.str() ==
              "Chunk Body: First\n"
              "Extension: quality = 1.0\n"
              "Chunk Body: Hello, world!\n"
              "Extension: file = abc.txt\n"
              "Extension: quality = 0.7\n"
              "Chunk Body: The Next Chunk\n"
              "Extension: last\n"
              "Chunk Body: Last one\n"
              "Expires: never\n"
              "Content-MD5: f4a5c16584f03d90\n");
    }
}

TEST_CASE("doc: handshaking") {
    // The server side of the RFC 6455 example: key dGhlIHNhbXBsZSBub25jZQ==
    // is answered with Sec-WebSocket-Accept s3pPLMBiTxaQ9kYGzzhZRbK+xOo=.
    net::io_context ioc;
    websocket::stream<beast::test::stream> server(ioc);
    beast::test::stream client(ioc);
    server.next_layer().connect(client);
    server.next_layer().append(
        "GET / HTTP/1.1\r\n"
        "Host: www.example.com\r\n"
        "Upgrade: websocket\r\n"
        "Connection: upgrade\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n");
    server.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res) { res.set(http::field::server, "Boost.Beast"); }));
    server.accept();
    auto response = std::string(client.str());
    CHECK(response.starts_with("HTTP/1.1 101 Switching Protocols\r\n"));
    CHECK(response.find("Upgrade: websocket\r\n") != std::string::npos);
    CHECK(response.find("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\n") != std::string::npos);
    CHECK(response.find("Server: Boost.Beast\r\n") != std::string::npos);
    CHECK(server.is_open());

    // The client side: host and target become the Host field and the target.
    net::io_context ioc2;
    websocket::stream<beast::test::stream> c(ioc2);
    websocket::stream<beast::test::stream> s(ioc2);
    c.next_layer().connect(s.next_layer());
    websocket::response_type res;
    beast::error_code ce = net::error::would_block, se = net::error::would_block;
    std::string host;
    s.async_accept([&](beast::error_code ec) { se = ec; });
    c.set_option(websocket::stream_base::decorator(
        [&](websocket::request_type& req) { host = std::string(req[http::field::host]); }));
    c.async_handshake(res, "www.example.com", "/", [&](beast::error_code ec) { ce = ec; });
    ioc2.run();
    CHECK_FALSE(ce);
    CHECK_FALSE(se);
    CHECK(host == "www.example.com");
    CHECK(res.result() == http::status::switching_protocols);
}

namespace {
// A body without size(): payload_size() is unknown.
struct unsized_body {
    using value_type = std::string;
    struct writer {
        using const_buffers_type = net::const_buffer;
        value_type const& body;
        template <bool isRequest, class Fields>
        writer(http::header<isRequest, Fields> const&, value_type const& b) : body(b) {}
        void init(beast::error_code& ec) { ec = {}; }
        boost::optional<std::pair<const_buffers_type, bool>> get(beast::error_code& ec) {
            ec = {};
            return {{net::buffer(body), false}};
        }
    };
};
}  // namespace

TEST_CASE("doc: Body") {
    static_assert(http::is_body<http::string_body>::value);
    static_assert(http::is_body_writer<http::string_body>::value);
    static_assert(http::is_body_reader<http::string_body>::value);
    static_assert(http::is_body<unsized_body>::value);
    static_assert(http::is_body_writer<unsized_body>::value);
    static_assert(!http::is_body_reader<unsized_body>::value);

    // with B::size: payload_size is size(), prepare_payload sets Content-Length
    // and removes chunked
    http::response<http::string_body> sized{http::status::ok, 11};
    sized.body() = "12345";
    sized.chunked(true);
    CHECK(sized.payload_size() == std::uint64_t(5));
    sized.prepare_payload();
    CHECK_FALSE(sized.chunked());
    CHECK(sized[http::field::content_length] == "5");

    // without: payload_size is none, prepare_payload erases Content-Length and
    // adds chunked
    http::response<unsized_body> unsized{http::status::ok, 11};
    unsized.body() = "abc";
    unsized.set(http::field::content_length, "3");
    CHECK_FALSE(unsized.payload_size());
    unsized.prepare_payload();
    CHECK(unsized.chunked());
    CHECK(unsized.find(http::field::content_length) == unsized.end());
    CHECK(wire(unsized) == "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n3\r\nabc\r\n0\r\n\r\n");
}

namespace {
// The page's declaration, and a working generator of the same shape.
struct BuffersGenerator {
    using const_buffers_type = net::const_buffer;
    bool is_done() const;
    const_buffers_type prepare(beast::error_code& ec);
    void consume(std::size_t n);
};
static_assert(beast::is_buffers_generator<BuffersGenerator>::value, "");

struct counting_generator {
    using const_buffers_type = net::const_buffer;
    std::vector<std::string> parts;
    std::size_t index = 0, offset = 0;
    bool is_done() const { return index == parts.size(); }
    const_buffers_type prepare(beast::error_code& ec) {
        ec = {};
        return net::buffer(parts[index].data() + offset, parts[index].size() - offset);
    }
    void consume(std::size_t n) {
        offset += n;
        if (offset >= parts[index].size()) {
            ++index;
            offset = 0;
        }
    }
};
}  // namespace

TEST_CASE("doc: BuffersGenerator") {
    static_assert(beast::is_buffers_generator<counting_generator>::value);
    static_assert(beast::is_buffers_generator<http::message_generator>::value);
    static_assert(!beast::is_buffers_generator<std::string>::value);
    net::io_context ioc;
    beast::test::stream out(ioc), in(ioc);
    out.connect(in);
    out.write_size(2);
    counting_generator g{{"one ", "two ", "three"}};
    beast::error_code ec;
    CHECK(beast::write(out, g, ec) == 13);
    CHECK_FALSE(ec);
    CHECK(g.is_done());
    CHECK(in.str() == "one two three");
    in.clear();

    http::response<http::string_body> res{http::status::ok, 11};
    res.body() = "generated";
    res.prepare_payload();
    std::string expect = wire(res);
    http::message_generator mg{std::move(res)};
    beast::write(out, std::move(mg));
    CHECK(in.str() == expect);
}

namespace {
template <bool isRequest, class Body, class Fields>
std::string describe(http::message<isRequest, Body, Fields> const& msg) {
    if constexpr (isRequest)
        return std::string(msg.method_string()) + " " + std::string(msg.target());
    else
        return std::to_string(msg.result_int()) + " " + std::string(msg.reason());
}
}  // namespace

TEST_CASE("doc: http_message_container") {
    http::request<http::empty_body> req{http::verb::put, "/x", 11};
    http::response<http::empty_body> res{http::status::not_found, 10};
    CHECK(describe(req) == "PUT /x");
    CHECK(describe(res) == "404 Not Found");

    // message from a header
    http::response_header<> h;
    h.result(http::status::ok);
    h.version(11);
    h.reason("Fine");
    h.set(http::field::server, "test");
    http::response<http::string_body> m{std::move(h)};
    CHECK(m.reason() == "Fine");
    CHECK(m[http::field::server] == "test");

    // method as verb and as string, unknown methods kept verbatim
    req.method_string("PURGE");
    CHECK(req.method() == http::verb::purge);
    req.method_string("FROB");
    CHECK(req.method() == http::verb::unknown);
    CHECK(req.method_string() == "FROB");
    res.result(299);
    CHECK(res.result_int() == 299);
    CHECK(res.result() == http::status::unknown);
}
