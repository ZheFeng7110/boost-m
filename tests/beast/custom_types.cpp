// mcpplibs.mwrap.boost.beast — user types at Beast's customization points: a
// user Body with its own reader and writer (parsed and serialized), Fields
// with a user allocator, a user DynamicBuffer (v1 interface) for http::read
// and websocket::stream::read, a user stream as the next layer of a
// websocket::stream with its own teardown / async_teardown /
// beast_close_socket overloads, and the
// compile-time traits rejecting types that do not model the concepts.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.beast;

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

namespace user {
// A body holding lines; serialized one line per buffer, parsed by splitting
// at '\n'.
struct lines_body {
    using value_type = std::vector<std::string>;

    static std::uint64_t size(value_type const& v) {
        std::uint64_t n = 0;
        for (auto const& l : v)
            n += l.size() + 1;
        return n;
    }

    class writer {
        value_type const& body_;
        std::size_t i_ = 0;
        std::string cur_;

    public:
        using const_buffers_type = net::const_buffer;
        template <bool isRequest, class Fields>
        writer(http::header<isRequest, Fields> const&, value_type const& b) : body_(b) {}
        void init(beast::error_code& ec) { ec = {}; }
        boost::optional<std::pair<const_buffers_type, bool>> get(beast::error_code& ec) {
            ec = {};
            if (i_ == body_.size())
                return boost::none;
            cur_ = body_[i_++] + "\n";
            return {{net::buffer(cur_), i_ < body_.size()}};
        }
    };

    class reader {
        value_type& body_;
        std::string partial_;

    public:
        template <bool isRequest, class Fields>
        reader(http::header<isRequest, Fields>&, value_type& b) : body_(b) {}
        void init(boost::optional<std::uint64_t> const&, beast::error_code& ec) { ec = {}; }
        template <class ConstBufferSequence>
        std::size_t put(ConstBufferSequence const& buffers, beast::error_code& ec) {
            ec = {};
            std::size_t n = 0;
            for (auto b : beast::buffers_range_ref(buffers)) {
                partial_.append(static_cast<char const*>(b.data()), b.size());
                n += b.size();
            }
            std::size_t pos;
            while ((pos = partial_.find('\n')) != std::string::npos) {
                body_.push_back(partial_.substr(0, pos));
                partial_.erase(0, pos + 1);
            }
            return n;
        }
        void finish(beast::error_code& ec) {
            ec = {};
            if (!partial_.empty())
                body_.push_back(std::move(partial_));
        }
    };
};

// An allocator counting what the fields container allocates.
inline std::size_t allocated = 0;
template <class T>
struct counting_allocator {
    using value_type = T;
    counting_allocator() = default;
    template <class U>
    counting_allocator(counting_allocator<U> const&) noexcept {}
    T* allocate(std::size_t n) {
        allocated += n * sizeof(T);
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>{}.deallocate(p, n); }
    template <class U>
    bool operator==(counting_allocator<U> const&) const noexcept { return true; }
};

// A DynamicBuffer_v1 over a std::string.
class string_buffer {
    std::string s_;
    std::size_t in_ = 0;  // committed size

public:
    using const_buffers_type = net::const_buffer;
    using mutable_buffers_type = net::mutable_buffer;
    std::size_t size() const { return in_; }
    std::size_t max_size() const { return 1 << 20; }
    std::size_t capacity() const { return s_.size(); }
    const_buffers_type data() const { return net::buffer(s_.data(), in_); }
    mutable_buffers_type prepare(std::size_t n) {
        if (in_ + n > max_size())
            throw std::length_error("string_buffer");
        s_.resize(in_ + n);
        return net::buffer(s_.data() + in_, n);
    }
    void commit(std::size_t n) { in_ += (std::min)(n, s_.size() - in_); }
    void consume(std::size_t n) {
        n = (std::min)(n, in_);
        s_.erase(0, n);
        in_ -= n;
    }
    std::string str() const { return s_.substr(0, in_); }
};

// A next layer of its own: a test stream underneath, and teardown overloads
// found by ADL that record being called.
struct logged_stream {
    beast::test::stream inner;
    std::vector<std::string>* log;
    using executor_type = beast::test::stream::executor_type;
    executor_type get_executor() { return inner.get_executor(); }
    template <class B>
    std::size_t read_some(B const& b) { return inner.read_some(b); }
    template <class B>
    std::size_t read_some(B const& b, beast::error_code& ec) { return inner.read_some(b, ec); }
    template <class B>
    std::size_t write_some(B const& b) { return inner.write_some(b); }
    template <class B>
    std::size_t write_some(B const& b, beast::error_code& ec) { return inner.write_some(b, ec); }
    template <class B, class H>
    auto async_read_some(B const& b, H&& h) {
        log->push_back("read");
        return inner.async_read_some(b, std::forward<H>(h));
    }
    template <class B, class H>
    auto async_write_some(B const& b, H&& h) {
        log->push_back("write");
        return inner.async_write_some(b, std::forward<H>(h));
    }
};
inline void teardown(beast::role_type role, logged_stream& s, beast::error_code& ec) {
    s.log->push_back("teardown");
    beast::test::teardown(role, s.inner, ec);
}
// beast::close_socket (used on failures and timeouts) reaches it by ADL too.
inline void beast_close_socket(logged_stream& s) {
    s.log->push_back("close");
    s.inner.close();
}
template <class TeardownHandler>
void async_teardown(beast::role_type role, logged_stream& s, TeardownHandler&& h) {
    s.log->push_back("async_teardown");
    beast::test::async_teardown(role, s.inner, std::forward<TeardownHandler>(h));
}
}  // namespace user

TEST_CASE("custom: Body with reader and writer") {
    static_assert(http::is_body<user::lines_body>::value);
    static_assert(http::is_body_reader<user::lines_body>::value);
    static_assert(http::is_body_writer<user::lines_body>::value);
    static_assert(!http::is_body<int>::value);
    static_assert(!http::is_body_writer<http::buffer_body::value_type>::value);

    http::response<user::lines_body> res{http::status::ok, 11};
    res.body() = {"alpha", "beta", "gamma"};
    res.prepare_payload();
    CHECK(res[http::field::content_length] == "17");

    net::io_context ioc;
    beast::test::stream out(ioc), in(ioc);
    out.connect(in);
    http::write(out, res);
    CHECK(in.str() == "HTTP/1.1 200 OK\r\nContent-Length: 17\r\n\r\nalpha\nbeta\ngamma\n");

    // parsed back, a few octets at a time
    beast::test::stream src(ioc, std::string(in.str()));
    src.read_size(3);
    beast::flat_buffer buf;
    http::response<user::lines_body> back;
    http::read(src, buf, back);
    CHECK(back.body() == std::vector<std::string>{"alpha", "beta", "gamma"});

    // chunked without size(): the serializer emits one chunk per buffer
    res.chunked(true);
    in.clear();
    http::write(out, res);
    CHECK(in.str().ends_with("6\r\nalpha\n\r\n5\r\nbeta\n\r\n6\r\ngamma\n\r\n0\r\n\r\n"));
}

TEST_CASE("custom: Fields with a user allocator") {
    using fields = http::basic_fields<user::counting_allocator<char>>;
    static_assert(http::is_fields<fields>::value);
    static_assert(!http::is_fields<std::map<std::string, std::string>>::value);
    user::allocated = 0;
    http::request<http::string_body, fields> req{http::verb::post, "/u", 11};
    req.set(http::field::host, "example.com");
    req.set("X-User", "value");
    req.body() = "x";
    req.prepare_payload();
    CHECK(user::allocated > 0);
    CHECK(req["X-User"] == "value");
    std::ostringstream os;
    os << req;
    CHECK(os.str() == "POST /u HTTP/1.1\r\nHost: example.com\r\nX-User: value\r\nContent-Length: 1\r\n\r\nx");

    // parsed into a message with those fields
    net::io_context ioc;
    beast::test::stream in(ioc, os.str());
    beast::flat_buffer buf;
    http::request<http::string_body, fields> back;
    http::read(in, buf, back);
    CHECK(back.method() == http::verb::post);
    CHECK(back["X-User"] == "value");
}

TEST_CASE("custom: DynamicBuffer") {
    static_assert(net::is_dynamic_buffer_v1<user::string_buffer>::value);
    static_assert(!net::is_dynamic_buffer_v1<std::string>::value);
    user::string_buffer b;
    net::buffer_copy(b.prepare(5), net::buffer("hello", 5));
    b.commit(5);
    CHECK(beast::buffers_to_string(b.data()) == "hello");
    b.consume(2);
    CHECK(b.str() == "llo");
    CHECK_THROWS_AS(b.prepare(std::size_t(1) << 21), std::length_error);

    // http::read with the user buffer holding the octets read ahead
    net::io_context ioc;
    beast::test::stream in(ioc, "GET / HTTP/1.1\r\n\r\nNEXT");
    user::string_buffer rb;
    http::request<http::empty_body> req;
    http::read(in, rb, req);
    CHECK(req.method() == http::verb::get);
    CHECK(rb.str() == "NEXT");

    // a websocket message read into it
    websocket::stream<beast::test::stream> a(ioc), c(ioc);
    a.next_layer().connect(c.next_layer());
    beast::error_code ea = net::error::would_block, ec2 = net::error::would_block;
    a.async_accept([&](beast::error_code e) { ea = e; });
    c.async_handshake("h", "/", [&](beast::error_code e) { ec2 = e; });
    ioc.run();
    REQUIRE_FALSE(ea);
    REQUIRE_FALSE(ec2);
    c.write(net::buffer(std::string("user buffer")));
    user::string_buffer wb;
    a.read(wb);
    CHECK(wb.str() == "user buffer");
}

TEST_CASE("custom: next layer with teardown overloads") {
    std::vector<std::string> log;
    net::io_context ioc;
    websocket::stream<user::logged_stream> server(user::logged_stream{beast::test::stream(ioc), &log});
    websocket::stream<beast::test::stream> client(ioc);
    server.next_layer().inner.connect(client.next_layer());
    beast::error_code se = net::error::would_block, ce = net::error::would_block;
    server.async_accept([&](beast::error_code e) { se = e; });
    client.async_handshake("h", "/", [&](beast::error_code e) { ce = e; });
    ioc.run();
    REQUIRE_FALSE(se);
    REQUIRE_FALSE(ce);
    CHECK(std::find(log.begin(), log.end(), "read") != log.end());

    // the client closes; the server reads the close frame, answers it and
    // tears its next layer down through the user's async_teardown
    ioc.restart();
    beast::flat_buffer b;
    beast::error_code re, cle;
    server.async_read(b, [&](beast::error_code e, std::size_t) { re = e; });
    client.async_close(websocket::close_code::normal, [&](beast::error_code e) { cle = e; });
    ioc.run();
    CHECK(re == websocket::error::closed);
    CHECK_FALSE(cle);
    CHECK(std::find(log.begin(), log.end(), "async_teardown") != log.end());

    // the synchronous teardown overload
    beast::error_code te;
    user::logged_stream ls{beast::test::stream(ioc), &log};
    teardown(beast::role_type::server, ls, te);
    CHECK(log.back() == "teardown");
    beast::close_socket(ls);
    CHECK(log.back() == "close");
}
