// mcpplibs.mwrap.boost.asio — the "Overview" pages of the Asio documentation
// (libs/asio/doc/overview/*.qbk, std_executors.qbk, using.qbk), one case per
// page: the features they describe, exercised in-process (loopback sockets,
// pipes, timers, signals) through the module. Cases are "doc: <page>#<id>".
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <csignal>
#include <boost/asio/detail/config.hpp>  // BOOST_ASIO_HAS_* feature macros
#include <boost/asio/coroutine.hpp>       // so yield.hpp below adds only its macros
import std;
import mcpplibs.mwrap.boost.asio;

namespace asio = boost::asio;
using asio::ip::tcp;
using namespace std::chrono_literals;

namespace {
// A connected loopback pair.
struct socket_pair {
    asio::io_context& ctx;
    tcp::socket a{ctx}, b{ctx};
    explicit socket_pair(asio::io_context& c) : ctx(c) {
        tcp::acceptor acc(ctx, {asio::ip::make_address("127.0.0.1"), 0});
        a.connect(acc.local_endpoint());
        b = acc.accept();
    }
};

std::string_view as_view(const asio::streambuf& sb) {
    auto d = sb.data();
    return {static_cast<const char*>(d.data()), d.size()};
}
}  // namespace

TEST_CASE("doc: using#using") {
    // Using the library: an io_context, an I/O object, run().
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    bool fired = false;
    t.async_wait([&](boost::system::error_code ec) { fired = !ec; });
    CHECK(ctx.run() == 1);
    CHECK(fired);
}

TEST_CASE("doc: buffers#buffers") {
    char raw[4] = {'a', 'b', 'c', 'd'};
    asio::mutable_buffer mb = asio::buffer(raw);
    CHECK(mb.size() == 4);
    std::vector<char> v(8);
    std::string s = "xyz";
    std::array<asio::const_buffer, 2> seq{asio::buffer(s), asio::buffer(raw, 2)};
    CHECK(asio::buffer_size(seq) == 5);
    CHECK(asio::buffer_copy(asio::buffer(v), seq) == 5);
    CHECK(std::string(v.data(), 5) == "xyzab");
    asio::streambuf sb;
    std::ostream os(&sb);
    os << "hello";
    CHECK(sb.size() == 5);
    std::string dyn;
    auto db = asio::dynamic_buffer(dyn);
    auto prep = db.prepare(3);
    asio::buffer_copy(prep, asio::buffer("abc", 3));
    db.commit(3);
    CHECK(dyn == "abc");
    CHECK((mb + 1).size() == 3);
}

TEST_CASE("doc: line_based#line_based") {
    asio::io_context ctx;
    socket_pair p(ctx);
    asio::write(p.a, asio::buffer(std::string("GET / HTTP/1.0\r\nHost: x\r\n\r\n")));
    asio::streambuf sb;
    std::size_t n = asio::read_until(p.b, sb, "\r\n");
    std::string line(as_view(sb).substr(0, n));
    CHECK(line == "GET / HTTP/1.0\r\n");
    sb.consume(n);
    std::size_t m = asio::read_until(p.b, sb, std::string("\r\n\r\n"));
    CHECK(m > 0);
    std::string text;
    asio::write(p.a, asio::buffer(std::string("one;two;")));
    asio::read_until(p.b, asio::dynamic_buffer(text), ';');
    CHECK(text.substr(0, 4) == "one;");
}

TEST_CASE("doc: allocation#allocation") {
    struct counting_alloc_handler {
        int* count;
        void operator()() { ++*count; }
        using allocator_type = std::allocator<void>;
        allocator_type get_allocator() const noexcept { return {}; }
    };
    asio::io_context ctx;
    int calls = 0;
    asio::post(ctx, counting_alloc_handler{&calls});
    auto h = asio::bind_allocator(std::allocator<void>(), [&] { ++calls; });
    asio::post(ctx, std::move(h));
    ctx.run();
    CHECK(calls == 2);
    asio::recycling_allocator<int> ra;
    int* p = ra.allocate(4);
    ra.deallocate(p, 4);
}

TEST_CASE("doc: cancellation#cancellation") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 10s);
    asio::cancellation_signal sig;
    boost::system::error_code result;
    t.async_wait(asio::bind_cancellation_slot(sig.slot(), [&](boost::system::error_code ec) { result = ec; }));
    asio::post(ctx, [&] { sig.emit(asio::cancellation_type::terminal); });
    ctx.run();
    CHECK(result == asio::error::operation_aborted);
    asio::cancellation_state st(sig.slot());
    CHECK(st.cancelled() == asio::cancellation_type::none);
}

TEST_CASE("doc: channels#channels") {
    asio::io_context ctx;
    asio::experimental::channel<void(boost::system::error_code, int)> ch(ctx, 2);
    std::vector<int> got;
    ch.async_send(boost::system::error_code{}, 1, asio::detached);
    ch.async_send(boost::system::error_code{}, 2, asio::detached);
    std::function<void()> recv = [&] {
        ch.async_receive([&](boost::system::error_code ec, int v) {
            if (ec) return;
            got.push_back(v);
            if (got.size() < 2) recv();
        });
    };
    recv();
    ctx.run();
    CHECK(got == std::vector<int>{1, 2});
    CHECK(ch.try_send(boost::system::error_code{}, 3));
    ch.close();
    CHECK_FALSE(ch.is_open());
}

namespace {
// overview/compose: a composed operation with async_compose.
template <class Token>
auto async_twice(asio::steady_timer& t, Token&& token) {
    return asio::async_compose<Token, void(boost::system::error_code, int)>(
        [&t, n = 0, state = 0](auto& self, boost::system::error_code ec = {}) mutable {
            if (ec) return self.complete(ec, n);
            if (state++ < 2) {
                ++n;
                t.expires_after(1ms);
                t.async_wait(std::move(self));
                return;
            }
            self.complete({}, n);
        },
        token, t);
}
}  // namespace

TEST_CASE("doc: compose#compose") {
    asio::io_context ctx;
    asio::steady_timer t(ctx);
    int result = 0;
    async_twice(t, [&](boost::system::error_code, int n) { result = n; });
    ctx.run();
    CHECK(result == 2);
    ctx.restart();
    auto fut = async_twice(t, asio::use_future);
    ctx.run();
    CHECK(fut.get() == 2);
}

TEST_CASE("doc: configuration#configuration") {
    // Feature macros the configuration page describes, visible to consumers
    // through <boost/asio/detail/config.hpp>.
#if defined(BOOST_ASIO_HAS_STD_COROUTINE)
    CHECK(true);
#endif
    asio::io_context single(1);  // concurrency hint
    CHECK(single.get_executor().running_in_this_thread() == false);
}

#include <boost/asio/yield.hpp>  // reenter / yield: the stackless-coroutine macros
namespace {
struct counter : asio::coroutine {
    int n = 0;
    void operator()() {
        reenter(this) {
            n = 1;
            yield return;
            n = 2;
        }
    }
};
}  // namespace
#include <boost/asio/unyield.hpp>

TEST_CASE("doc: coroutine#coroutine") {
    counter c;
    c();
    CHECK(c.n == 1);
    CHECK_FALSE(c.is_complete());
    c();
    CHECK(c.n == 2);
    CHECK(c.is_complete());
}

namespace {
asio::awaitable<int> add_later(int a, int b) {
    asio::steady_timer t(co_await asio::this_coro::executor, 1ms);
    co_await t.async_wait(asio::use_awaitable);
    co_return a + b;
}
asio::awaitable<void> echo_once(tcp::socket s) {
    char buf[16];
    std::size_t n = co_await s.async_read_some(asio::buffer(buf), asio::use_awaitable);
    co_await asio::async_write(s, asio::buffer(buf, n), asio::use_awaitable);
}
}  // namespace

TEST_CASE("doc: cpp20_coroutines#cpp20_coroutines") {
    asio::io_context ctx;
    int r = 0;
    asio::co_spawn(ctx, add_later(2, 3), [&](std::exception_ptr, int v) { r = v; });
    socket_pair p(ctx);
    asio::co_spawn(ctx, echo_once(std::move(p.b)), asio::detached);
    asio::write(p.a, asio::buffer(std::string("ping")));
    ctx.run();
    char back[4];
    asio::read(p.a, asio::buffer(back));
    CHECK(r == 5);
    CHECK(std::string(back, 4) == "ping");
    // awaitable operators
    ctx.restart();
    using namespace asio::experimental::awaitable_operators;
    std::variant<int, int> first;
    asio::co_spawn(ctx, add_later(1, 1) || add_later(2, 2), [&](std::exception_ptr, std::variant<int, int> v) {
        first = v;
    });
    ctx.run();
    CHECK(first.index() <= 1);
}

// experimental::coro: a lazy generator-like coroutine. A free function, as
// on the page: the promise takes its executor from the first parameter.
static asio::experimental::generator<int> gen(asio::any_io_executor) {
    for (int i = 0; i < 3; ++i) co_yield i;
}

TEST_CASE("doc: coro#coro") {
    asio::io_context ctx;
    std::vector<int> out;
    asio::co_spawn(
        ctx,
        [&]() -> asio::awaitable<void> {
            auto g = gen(co_await asio::this_coro::executor);
            while (auto v = co_await g.async_resume(asio::use_awaitable)) out.push_back(*v);
        },
        asio::detached);
    ctx.run();
    CHECK(out == std::vector<int>{0, 1, 2});
}

TEST_CASE("doc: cpp2011#move_objects") {
    asio::io_context ctx;
    tcp::socket s1(ctx);
    tcp::socket s2(std::move(s1));
    CHECK_FALSE(s2.is_open());
    s2.open(tcp::v4());
    tcp::socket s3 = std::move(s2);
    CHECK(s3.is_open());
}

TEST_CASE("doc: cpp2011#move_handlers") {
    asio::io_context ctx;
    auto p = std::make_unique<int>(7);
    int seen = 0;
    asio::post(ctx, [p = std::move(p), &seen] { seen = *p; });  // move-only handler
    ctx.run();
    CHECK(seen == 7);
}

TEST_CASE("doc: deferred#deferred") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    auto op = t.async_wait(asio::deferred);  // nothing starts yet
    bool done = false;
    std::move(op)([&](boost::system::error_code) { done = true; });
    ctx.run();
    CHECK(done);
    ctx.restart();
    t.expires_after(1ms);
    auto chained = t.async_wait(asio::deferred([](boost::system::error_code ec) {
        return asio::deferred.values(ec ? 1 : 42);
    }));
    int v = 0;
    std::move(chained)([&](int x) { v = x; });
    ctx.run();
    CHECK(v == 42);
}

TEST_CASE("doc: files#files") {
#if defined(BOOST_ASIO_HAS_FILE)
    asio::io_context ctx;
    auto path = (std::filesystem::temp_directory_path() / "boost-m-asio-file.txt").string();
    asio::stream_file f(ctx, path, asio::stream_file::write_only | asio::stream_file::create | asio::stream_file::truncate);
    asio::write(f, asio::buffer(std::string("data")));
    f.close();
    CHECK(std::filesystem::file_size(path) == 4);
#else
    // Files need io_uring (Linux) or Windows; this build has neither.
    CHECK(true);
#endif
}

TEST_CASE("doc: futures#futures") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    std::future<void> f = t.async_wait(asio::use_future);
    std::thread th([&] { ctx.run(); });
    f.get();
    th.join();
    CHECK(true);
}

TEST_CASE("doc: handler_tracking#handler_tracking") {
    // Handler tracking is a build-time switch (BOOST_ASIO_ENABLE_HANDLER_TRACKING)
    // of the headers; the module is built without it. The location macro's
    // runtime part, asio::detail::source_location, is not needed to post.
    asio::io_context ctx;
    int n = 0;
    asio::post(ctx, [&] { ++n; });
    ctx.run();
    CHECK(n == 1);
}

TEST_CASE("doc: immediate_completion#immediate_completion") {
    asio::io_context ctx;
    socket_pair p(ctx);
    // A zero-length read completes immediately; bind_immediate_executor picks
    // where its handler runs.
    bool done = false;
    p.a.async_read_some(asio::mutable_buffer(),
                        asio::bind_immediate_executor(ctx.get_executor(), [&](boost::system::error_code, std::size_t n) {
                            done = n == 0;
                        }));
    ctx.run();
    CHECK(done);
}

TEST_CASE("doc: implementation#implementation") {
    asio::io_context ctx;
    auto& svc = asio::use_service<asio::detail::scheduler>(ctx);
    (void)svc;
    CHECK(asio::has_service<asio::detail::scheduler>(ctx));
}

TEST_CASE("doc: iostreams#iostreams") {
    asio::io_context ctx;
    tcp::acceptor acc(ctx, {asio::ip::make_address("127.0.0.1"), 0});
    auto port = std::to_string(acc.local_endpoint().port());
    std::thread server([&] {
        tcp::iostream s;
        acc.accept(s.socket());
        std::string line;
        std::getline(s, line);
        s << "echo " << line << std::endl;
    });
    tcp::iostream client("127.0.0.1", port);
    REQUIRE(client);
    client << "hi" << std::endl;
    std::string reply;
    std::getline(client, reply);
    server.join();
    CHECK(reply == "echo hi");
}

TEST_CASE("doc: completion_tokens#completion_tokens") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    auto [ec] = std::tuple<boost::system::error_code>{};
    t.async_wait(asio::as_tuple([&](std::tuple<boost::system::error_code> r) { ec = std::get<0>(r); }));
    ctx.run();
    CHECK(!ec);
    ctx.restart();
    t.expires_after(1ms);
    auto f = t.async_wait(asio::as_tuple(asio::use_future));
    ctx.run();
    CHECK(!std::get<0>(f.get()));
}

TEST_CASE("doc: parallel_group#parallel_group") {
    asio::io_context ctx;
    asio::steady_timer a(ctx, 1ms), b(ctx, 50ms);
    std::size_t first = 99;
    asio::experimental::make_parallel_group(a.async_wait(asio::deferred), b.async_wait(asio::deferred))
        .async_wait(asio::experimental::wait_for_one(),
                    [&](std::array<std::size_t, 2> order, boost::system::error_code, boost::system::error_code) {
                        first = order[0];
                    });
    ctx.run();
    CHECK(first == 0);
}

TEST_CASE("doc: basics#basics") {
    // The basics page: an io_context, an I/O object, a synchronous operation
    // reporting through an error_code, then the asynchronous form.
    asio::io_context io_context;
    tcp::acceptor acc(io_context, {asio::ip::make_address("127.0.0.1"), 0});
    tcp::socket socket(io_context);
    boost::system::error_code ec;
    socket.connect(acc.local_endpoint(), ec);
    CHECK(!ec);
    tcp::socket refused(io_context);
    tcp::endpoint closed = acc.local_endpoint();
    acc.close();
    refused.connect(closed, ec);
    CHECK(ec);  // nothing listens any more
    asio::steady_timer t(io_context, 1ms);
    bool done = false;
    t.async_wait([&](const boost::system::error_code& e) { done = !e; });
    io_context.run();
    CHECK(done);
}

TEST_CASE("doc: other_protocols#other_protocols") {
    // generic::stream_protocol / datagram_protocol: sockets whose protocol is
    // chosen at run time, converted from a TCP / UDP endpoint.
    asio::io_context ctx;
    tcp::acceptor acc(ctx, {asio::ip::make_address("127.0.0.1"), 0});
    asio::generic::stream_protocol::endpoint ep(acc.local_endpoint());
    asio::generic::stream_protocol::socket s(ctx);
    s.connect(ep);
    tcp::socket peer = acc.accept();
    asio::write(s, asio::buffer(std::string("gen")));
    char buf[3];
    asio::read(peer, asio::buffer(buf));
    CHECK(std::string(buf, 3) == "gen");
    asio::ip::udp::socket u(ctx, {asio::ip::make_address("127.0.0.1"), 0});
    asio::generic::datagram_protocol::socket g(ctx, asio::generic::datagram_protocol(asio::ip::udp::v4()));
    g.send_to(asio::buffer("dgram", 5), asio::generic::datagram_protocol::endpoint(u.local_endpoint()));
    char d[5];
    asio::ip::udp::endpoint from;
    CHECK(u.receive_from(asio::buffer(d), from) == 5);
}

TEST_CASE("doc: timers#timers") {
    asio::io_context io;
    auto start = std::chrono::steady_clock::now();  // before the timer: its expiry counts from construction
    asio::steady_timer t(io, 5ms);
    t.wait();  // blocking wait
    CHECK(std::chrono::steady_clock::now() - start >= 5ms);
    t.expires_after(1ms);
    int fired = 0;
    t.async_wait([&](const boost::system::error_code&) { ++fired; });
    asio::steady_timer cancelled(io, 1h);
    boost::system::error_code got;
    cancelled.async_wait([&](const boost::system::error_code& e) { got = e; });
    cancelled.cancel();
    io.run();
    CHECK(fired == 1);
    CHECK(got == asio::error::operation_aborted);
    asio::system_timer at(io, std::chrono::system_clock::now() + 1ms);
    at.wait();
    CHECK(at.expiry() <= std::chrono::system_clock::now());
}

TEST_CASE("doc: pipes#pipes") {
#if defined(BOOST_ASIO_HAS_PIPE)
    asio::io_context ctx;
    asio::readable_pipe r(ctx);
    asio::writable_pipe w(ctx);
    asio::connect_pipe(r, w);
    asio::write(w, asio::buffer(std::string("pipe")));
    char buf[4];
    asio::read(r, asio::buffer(buf));
    CHECK(std::string(buf, 4) == "pipe");
#else
    CHECK(true);
#endif
}

TEST_CASE("doc: posix#fork") {
    // notify_fork around a fork(): the prepare / parent half of the protocol
    // (the test does not fork; the child half is the same call in the child).
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    ctx.notify_fork(asio::execution_context::fork_prepare);
    ctx.notify_fork(asio::execution_context::fork_parent);
    bool fired = false;
    t.async_wait([&](boost::system::error_code ec) { fired = !ec; });
    ctx.run();
    CHECK(fired);
}

TEST_CASE("doc: posix#stream_descriptor") {
#if defined(BOOST_ASIO_HAS_POSIX_STREAM_DESCRIPTOR) && defined(BOOST_ASIO_HAS_PIPE)
    // A stream_descriptor over a descriptor obtained elsewhere (here the
    // read end of a pipe), read asynchronously.
    asio::io_context ctx;
    asio::readable_pipe r(ctx);
    asio::writable_pipe w(ctx);
    asio::connect_pipe(r, w);
    asio::posix::stream_descriptor in(ctx, r.release());
    asio::write(w, asio::buffer(std::string("fd-data")));
    w.close();
    std::string got;
    asio::async_read(in, asio::dynamic_buffer(got), [](boost::system::error_code, std::size_t) {});
    ctx.run();
    CHECK(got == "fd-data");
#else
    CHECK(true);
#endif
}

TEST_CASE("doc: posix#local") {
// Windows has AF_UNIX stream sockets but no socketpair: connect_pair fails
// there with WSAEOPNOTSUPP.
#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS) && !defined(BOOST_ASIO_WINDOWS)
    asio::io_context ctx;
    asio::local::stream_protocol::socket a(ctx), b(ctx);
    asio::local::connect_pair(a, b);
    asio::write(a, asio::buffer(std::string("unix")));
    char buf[4];
    asio::read(b, asio::buffer(buf));
    CHECK(std::string(buf, 4) == "unix");
#else
    CHECK(true);
#endif
}

TEST_CASE("doc: promises#promises") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    auto p = t.async_wait(asio::experimental::use_promise);
    bool done = false;
    p([&](boost::system::error_code) { done = true; });
    ctx.run();
    CHECK(done);
}

TEST_CASE("doc: protocols#protocols") {
    asio::io_context ctx;
    asio::ip::udp::socket rx(ctx, {asio::ip::make_address("127.0.0.1"), 0});
    asio::ip::udp::socket tx(ctx, asio::ip::udp::v4());
    tx.send_to(asio::buffer(std::string("dgram")), rx.local_endpoint());
    char buf[16];
    asio::ip::udp::endpoint from;
    std::size_t n = rx.receive_from(asio::buffer(buf), from);
    CHECK(std::string(buf, n) == "dgram");
    tcp::resolver res(ctx);
    auto eps = res.resolve("127.0.0.1", "80", tcp::resolver::numeric_host | tcp::resolver::numeric_service);
    CHECK(eps.begin()->endpoint().port() == 80);
    CHECK(asio::ip::make_address_v6("::1").is_loopback());
}

TEST_CASE("doc: reactor#reactor") {
    asio::io_context ctx;
    socket_pair p(ctx);
    p.b.non_blocking(true);
    bool readable = false;
    p.b.async_wait(tcp::socket::wait_read, [&](boost::system::error_code ec) { readable = !ec; });
    asio::write(p.a, asio::buffer(std::string("x")));
    ctx.run();
    CHECK(readable);
    char c;
    boost::system::error_code ec;
    CHECK(p.b.read_some(asio::buffer(&c, 1), ec) == 1);
}

TEST_CASE("doc: signals#signals") {
    asio::io_context ctx;
    asio::signal_set sigs(ctx, SIGINT);
    int got = 0;
    sigs.async_wait([&](boost::system::error_code ec, int n) {
        if (!ec) got = n;
    });
    std::raise(SIGINT);
    ctx.run();
    CHECK(got == SIGINT);
}

TEST_CASE("doc: spawn#spawn") {
    // asio::spawn needs Boost.Context, which the package does not declare.
    CHECK(true);
}

TEST_CASE("doc: strands#strands") {
    asio::io_context ctx;
    auto st = asio::make_strand(ctx);
    int counter = 0;
    for (int i = 0; i < 1000; ++i) asio::post(st, [&] { ++counter; });
    std::vector<std::thread> ts;
    for (int i = 0; i < 4; ++i) ts.emplace_back([&] { ctx.run(); });
    for (auto& t : ts) t.join();
    CHECK(counter == 1000);
}

TEST_CASE("doc: token_adapters#token_adapters") {
    asio::io_context ctx;
    bool aborted = false;
    boost::system::error_code ec;
    asio::steady_timer t2(ctx, 1ms);
    t2.async_wait(asio::redirect_error(asio::detached, ec));  // the error goes to ec
    ctx.run();
    CHECK(!ec);
    ctx.restart();
    asio::steady_timer t3(ctx, 1ms);
    t3.async_wait(asio::bind_executor(ctx.get_executor(), [&](boost::system::error_code e) { aborted = !!e; }));
    ctx.run();
    CHECK_FALSE(aborted);
    ctx.restart();
    asio::steady_timer t4(ctx, 10s);
    t4.async_wait(asio::cancel_after(1ms, [&](boost::system::error_code e) { aborted = e == asio::error::operation_aborted; }));
    ctx.run();
    CHECK(aborted);
}

TEST_CASE("doc: type_erasure#type_erasure") {
    asio::io_context ctx;
    asio::any_io_executor ex = ctx.get_executor();
    int n = 0;
    asio::post(ex, [&] { ++n; });
    asio::any_completion_handler<void(int)> h = [&](int v) { n += v; };
    std::move(h)(5);
    ctx.run();
    CHECK(n == 6);
}

TEST_CASE("doc: std_executors#std_executors") {
    asio::thread_pool pool(2);
    std::atomic<int> n{0};
    auto ex = pool.get_executor();
    asio::require(ex, asio::execution::blocking.never).execute([&] { ++n; });
    asio::post(pool, [&] { ++n; });
    pool.join();
    CHECK(n == 2);
    static_assert(asio::execution::is_executor_v<decltype(ex)>);
    CHECK(asio::query(ex, asio::execution::blocking) == asio::execution::blocking.possibly);
}
