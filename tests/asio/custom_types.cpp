// mcpplibs.mwrap.boost.asio — user types at Asio's customization points, the
// "Requirements" pages (libs/asio/doc/requirements/*.qbk): a user completion
// token (async_result specialization) and user handlers for each token kind
// (wait, read, write, accept, move-accept, connect, range/iterator connect,
// resolve, signal, shutdown, nullary), user ConstBufferSequence /
// MutableBufferSequence types, a user Executor, a user Service and
// IoObjectService, a user CancellationHandler, user SyncRandomAccessRead/
// WriteDevice types for read_at / write_at, and a user asynchronous
// operation built with async_initiate (asynchronous_operations.qbk).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <csignal>
import std;
import mcpplibs.mwrap.boost.asio;

namespace asio = boost::asio;
using asio::ip::tcp;
using boost::system::error_code;
using namespace std::chrono_literals;

namespace user {
// A completion token that records results into a user log.
struct log_token {
    std::vector<std::string>* log;
};
}  // namespace user

template <class... Args>
struct boost::asio::async_result<user::log_token, void(Args...)> {
    template <class Initiation, class... InitArgs>
    static void initiate(Initiation&& init, user::log_token tok, InitArgs&&... args) {
        std::move(init)(
            [log = tok.log](Args... results) {
                std::ostringstream os;
                ((os << "[" << results << "]"), ...);
                log->push_back(os.str());
            },
            std::forward<InitArgs>(args)...);
    }
};

namespace user {
// ConstBufferSequence / MutableBufferSequence over user storage.
class two_part_buffer {
public:
    two_part_buffer(std::string& a, std::string& b) : parts_{asio::buffer(a), asio::buffer(b)} {}
    using value_type = asio::mutable_buffer;
    using const_iterator = const asio::mutable_buffer*;
    const_iterator begin() const { return parts_.data(); }
    const_iterator end() const { return parts_.data() + parts_.size(); }
private:
    std::array<asio::mutable_buffer, 2> parts_;
};

// A user Executor wrapping another and counting submissions.
template <class Inner>
class counting_executor {
public:
    counting_executor(Inner inner, std::atomic<int>& n) : inner_(inner), n_(&n) {}
    template <class F>
    void execute(F&& f) const {
        ++*n_;
        inner_.execute(std::forward<F>(f));
    }
    auto query(asio::execution::context_t q) const noexcept -> decltype(asio::query(std::declval<Inner>(), q)) {
        return asio::query(inner_, q);
    }
    static constexpr auto query(asio::execution::blocking_t) noexcept { return asio::execution::blocking.never; }
    bool operator==(const counting_executor& o) const noexcept { return inner_ == o.inner_; }
private:
    Inner inner_;
    std::atomic<int>* n_;
};

// A user Service registered in an execution_context.
class stats_service : public asio::execution_context::service {
public:
    using key_type = stats_service;
    static inline asio::execution_context::id id;
    explicit stats_service(asio::execution_context& ctx) : asio::execution_context::service(ctx) {}
    int created = 0;
private:
    void shutdown() override { shut = true; }
public:
    bool shut = false;
};

// SyncRandomAccessReadDevice / WriteDevice over a user byte array.
class memory_device {
public:
    explicit memory_device(std::size_t n) : bytes_(n) {}
    template <class MutableBufferSequence>
    std::size_t read_some_at(std::uint64_t off, const MutableBufferSequence& b, error_code& ec) {
        ec = {};
        if (off >= bytes_.size()) { ec = asio::error::eof; return 0; }
        return asio::buffer_copy(b, asio::buffer(bytes_.data() + off, bytes_.size() - off));
    }
    template <class MutableBufferSequence>
    std::size_t read_some_at(std::uint64_t off, const MutableBufferSequence& b) {
        error_code ec;
        auto n = read_some_at(off, b, ec);
        if (ec) throw boost::system::system_error(ec);
        return n;
    }
    template <class ConstBufferSequence>
    std::size_t write_some_at(std::uint64_t off, const ConstBufferSequence& b, error_code& ec) {
        ec = {};
        if (off >= bytes_.size()) { ec = asio::error::no_buffer_space; return 0; }
        return asio::buffer_copy(asio::buffer(bytes_.data() + off, bytes_.size() - off), b);
    }
    template <class ConstBufferSequence>
    std::size_t write_some_at(std::uint64_t off, const ConstBufferSequence& b) {
        error_code ec;
        auto n = write_some_at(off, b, ec);
        if (ec) throw boost::system::system_error(ec);
        return n;
    }
private:
    std::vector<char> bytes_;
};

// A user Clock for basic_waitable_timer.
struct tick_clock {
    using rep = std::int64_t;
    using period = std::milli;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<tick_clock>;
    static constexpr bool is_steady = true;
    static time_point now() { return time_point(std::chrono::duration_cast<duration>(std::chrono::steady_clock::now().time_since_epoch())); }
};

// A user asynchronous operation: completes with the sum after a timer.
template <class Token>
auto async_sum(asio::steady_timer& t, int a, int b, Token&& token) {
    return asio::async_initiate<Token, void(error_code, int)>(
        [&t](auto handler, int x, int y) {
            t.expires_after(1ms);
            t.async_wait([h = std::move(handler), x, y](error_code ec) mutable { std::move(h)(ec, x + y); });
        },
        token, a, b);
}
}  // namespace user

TEST_CASE("doc: asynchronous_operations#asynchronous_operations") {
    asio::io_context ctx;
    asio::steady_timer t(ctx);
    int r = 0;
    user::async_sum(t, 2, 3, [&](error_code, int v) { r = v; });
    ctx.run();
    CHECK(r == 5);
    ctx.restart();
    auto f = user::async_sum(t, 4, 4, asio::use_future);
    ctx.run();
    CHECK(f.get() == 8);
}

TEST_CASE("a user completion token") {
    asio::io_context ctx;
    std::vector<std::string> log;
    asio::steady_timer t(ctx);
    user::async_sum(t, 1, 1, user::log_token{&log});
    ctx.run();
    REQUIRE(log.size() == 1);
    CHECK(log[0].find("[2]") != std::string::npos);
}

TEST_CASE("doc: WaitToken#WaitToken") {
    asio::io_context ctx;
    asio::steady_timer t(ctx, 1ms);
    error_code got = asio::error::fault;
    t.async_wait([&](error_code ec) { got = ec; });
    ctx.run();
    CHECK(!got);
}

TEST_CASE("token kinds over a loopback connection") {
    asio::io_context ctx;
    tcp::acceptor acc(ctx, {asio::ip::make_address("127.0.0.1"), 0});
    tcp::socket client(ctx), server(ctx);
    std::vector<std::string> steps;
    acc.async_accept(server, [&](error_code ec) { steps.push_back(ec ? "accept-fail" : "accept"); });  // AcceptToken
    tcp::resolver res(ctx);
    res.async_resolve("127.0.0.1", std::to_string(acc.local_endpoint().port()),
                      [&](error_code ec, tcp::resolver::results_type r) {  // ResolveToken
                          steps.push_back("resolve");
                          if (ec) return;
                          asio::async_connect(client, r, [&](error_code ec2, const tcp::endpoint&) {  // RangeConnectToken
                              steps.push_back(ec2 ? "connect-fail" : "connect");
                          });
                      });
    ctx.run();
    CHECK(std::count(steps.begin(), steps.end(), "accept") == 1);
    CHECK(std::count(steps.begin(), steps.end(), "connect") == 1);
    ctx.restart();
    std::string out = "hello";
    std::string a(2, '\0'), b(3, '\0');
    user::two_part_buffer in(a, b);
    asio::async_write(client, asio::buffer(out), [&](error_code, std::size_t n) { steps.push_back("write" + std::to_string(n)); });  // WriteToken
    asio::async_read(server, in, [&](error_code, std::size_t n) { steps.push_back("read" + std::to_string(n)); });  // ReadToken, user MutableBufferSequence
    ctx.run();
    CHECK(a + b == "hello");
    CHECK(std::find(steps.begin(), steps.end(), "read5") != steps.end());
    ctx.restart();
    // MoveAcceptToken + IteratorConnectToken
    tcp::socket moved_peer(ctx);
    acc.async_accept([&](error_code ec, tcp::socket s) { if (!ec) moved_peer = std::move(s); });
    std::array<tcp::endpoint, 1> eps{acc.local_endpoint()};
    tcp::socket c2(ctx);
    asio::async_connect(c2, eps.begin(), eps.end(), [&](error_code ec, decltype(eps.begin())) { CHECK(!ec); });
    ctx.run();
    CHECK(moved_peer.is_open());
    // ShutdownToken (via post, the operation is synchronous) / NullaryToken
    error_code sec;
    c2.shutdown(tcp::socket::shutdown_both, sec);
    bool nullary = false;
    asio::post(ctx, [&] { nullary = true; });
    ctx.restart();
    ctx.run();
    CHECK(nullary);
}

TEST_CASE("doc: SignalToken#SignalToken") {
    asio::io_context ctx;
    asio::signal_set s(ctx, SIGTERM);
    int sig = 0;
    s.async_wait([&](error_code, int n) { sig = n; });
    std::raise(SIGTERM);
    ctx.run();
    CHECK(sig == SIGTERM);
}

TEST_CASE("doc: ConstBufferSequence#ConstBufferSequence") {
    const std::string x = "ab", y = "cd";
    std::vector<asio::const_buffer> seq{asio::buffer(x), asio::buffer(y)};
    static_assert(asio::is_const_buffer_sequence<std::vector<asio::const_buffer>>::value);
    CHECK(asio::buffer_size(seq) == 4);
    std::string out(4, '\0');
    asio::buffer_copy(asio::buffer(out), seq);
    CHECK(out == "abcd");
}

TEST_CASE("doc: MutableBufferSequence#MutableBufferSequence") {
    std::string a(1, '\0'), b(1, '\0');
    user::two_part_buffer seq(a, b);
    static_assert(asio::is_mutable_buffer_sequence<user::two_part_buffer>::value);
    CHECK(asio::buffer_copy(seq, asio::buffer("xy", 2)) == 2);
    CHECK(a + b == "xy");
}

TEST_CASE("doc: Executor#Executor1") {
    asio::io_context ctx;
    std::atomic<int> n{0};
    user::counting_executor<asio::io_context::executor_type> ex(ctx.get_executor(), n);
    static_assert(asio::execution::is_executor_v<decltype(ex)>);
    int ran = 0;
    asio::post(ex, [&] { ++ran; });
    asio::steady_timer t(ex, 1ms);  // I/O objects on a user executor
    t.async_wait([&](error_code) { ++ran; });
    ctx.run();
    CHECK(ran == 2);
    CHECK(n >= 1);
}

TEST_CASE("doc: Service#Service") {
    {
        asio::io_context ctx;
        auto& s = asio::use_service<user::stats_service>(ctx);
        s.created++;
        CHECK(asio::has_service<user::stats_service>(ctx));
        CHECK(asio::use_service<user::stats_service>(ctx).created == 1);
    }
}

TEST_CASE("doc: IoObjectService#IoObjectService") {
    // basic_waitable_timer over a user clock uses the library's I/O object
    // service with user-supplied traits.
    using user::tick_clock;
    asio::io_context ctx;
    asio::basic_waitable_timer<tick_clock> t(ctx, 1ms);
    bool done = false;
    t.async_wait([&](error_code ec) { done = !ec; });
    ctx.run();
    CHECK(done);
}

TEST_CASE("doc: CancellationHandler#CancellationHandler") {
    asio::cancellation_signal sig;
    int called = 0;
    struct handler {
        int* n;
        void operator()(asio::cancellation_type t) const { *n += t == asio::cancellation_type::partial ? 1 : 100; }
    };
    sig.slot().assign(handler{&called});
    sig.emit(asio::cancellation_type::partial);
    CHECK(called == 1);
    sig.slot().clear();
    sig.emit(asio::cancellation_type::partial);
    CHECK(called == 1);
}

TEST_CASE("doc: SyncRandomAccessWriteDevice#SyncRandomAccessWriteDevice") {
    user::memory_device dev(16);
    CHECK(asio::write_at(dev, 4, asio::buffer(std::string("data"))) == 4);
    error_code ec;
    asio::write_at(dev, 100, asio::buffer(std::string("x")), ec);
    CHECK(ec == asio::error::no_buffer_space);
}

TEST_CASE("doc: SyncRandomAccessReadDevice#SyncRandomAccessReadDevice") {
    user::memory_device dev(8);
    asio::write_at(dev, 0, asio::buffer(std::string("abcdefgh")));
    char buf[3];
    CHECK(asio::read_at(dev, 2, asio::buffer(buf)) == 3);
    CHECK(std::string(buf, 3) == "cde");
}
