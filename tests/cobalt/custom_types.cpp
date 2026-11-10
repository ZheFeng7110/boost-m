// mcpplibs.mwrap.boost.cobalt — user types at Cobalt's customization points: a
// user awaitable (await_ready / await_suspend / await_resume) that reads the
// awaiting promise's executor, an interruptible awaitable raced with
// interrupt_await, a user operation derived from cobalt::op, task / promise /
// generator / channel carrying a move-only user type, and a user executor
// (the properties any_io_executor needs) that cobalt::spawn runs a task on;
// the concepts rejecting non-awaitables.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <coroutine>
#include <new>
#include <typeinfo>
import std;
import mcpplibs.mwrap.boost.asio;
import mcpplibs.mwrap.boost.cobalt;

namespace cobalt = boost::cobalt;
namespace asio = boost::asio;
namespace bsys = boost::system;
using namespace std::chrono_literals;

namespace user {
// Completes on the awaiting coroutine's executor, a post later.
struct posted_value {
    int value;
    int result = 0;
    bool await_ready() const noexcept { return false; }
    template <class Promise>
    void await_suspend(std::coroutine_handle<Promise> h) {
        static_assert(requires(Promise& p) { p.get_executor(); });
        asio::post(h.promise().get_executor(), [this, h] {
            result = value * 2;
            h.resume();
        });
    }
    int await_resume() const noexcept { return result; }
};

// Never completes by itself; interrupt_await resumes it with "interrupted".
struct waits_forever {
    std::coroutine_handle<> awaiter;
    bool interrupted = false;
    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) { awaiter = h; }
    std::string await_resume() { return interrupted ? "interrupted" : "done"; }
    void interrupt_await() & {
        interrupted = true;
        if (auto h = std::exchange(awaiter, nullptr))
            h.resume();
    }
};

// A cobalt::op over a timer: ready() short-cuts an expired one.
struct wait_op final : cobalt::op<bsys::error_code> {
    asio::steady_timer& tim;
    bool* took_shortcut;
    wait_op(asio::steady_timer& t, bool* s) : tim(t), took_shortcut(s) {}
    void ready(cobalt::handler<bsys::error_code> h) override {
        if (tim.expiry() < std::chrono::steady_clock::now()) {
            *took_shortcut = true;
            h(bsys::error_code{});
        }
    }
    void initiate(cobalt::completion_handler<bsys::error_code> complete) override {
        tim.async_wait(std::move(complete));
    }
};

// Move-only payload.
struct ticket {
    std::unique_ptr<int> id;
    explicit ticket(int i) : id(std::make_unique<int>(i)) {}
};

// An executor of its own over an io_context: counts what it runs and
// provides the properties asio::any_io_executor requires.
struct counting_executor {
    asio::io_context* ctx;
    std::shared_ptr<std::atomic<int>> runs = std::make_shared<std::atomic<int>>(0);

    asio::execution_context& query(asio::execution::context_t) const noexcept { return *ctx; }
    static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept {
        return asio::execution::blocking.never;
    }
    counting_executor require(asio::execution::blocking_t::never_t) const { return *this; }
    counting_executor prefer(asio::execution::blocking_t::possibly_t) const { return *this; }
    counting_executor prefer(asio::execution::outstanding_work_t::tracked_t) const { return *this; }
    counting_executor prefer(asio::execution::outstanding_work_t::untracked_t) const { return *this; }
    counting_executor prefer(asio::execution::relationship_t::fork_t) const { return *this; }
    counting_executor prefer(asio::execution::relationship_t::continuation_t) const { return *this; }
    template <class F>
    void execute(F&& f) const {
        ++*runs;
        asio::post(ctx->get_executor(), std::forward<F>(f));
    }
    bool operator==(counting_executor const& o) const noexcept { return ctx == o.ctx; }
    bool operator!=(counting_executor const& o) const noexcept { return ctx != o.ctx; }
};
}  // namespace user

TEST_CASE("custom: awaitable") {
    static_assert(cobalt::awaitable_type<user::posted_value, cobalt::detail::task_promise<int>>);
    static_assert(!cobalt::awaitable<int>);
    static_assert(!cobalt::awaitable<std::string>);
    auto t = []() -> cobalt::task<int> {
        int a = co_await user::posted_value{20};
        co_return a + 2;
    };
    CHECK(cobalt::run(t()) == 42);
    // from a promise and a generator too
    auto p = []() -> cobalt::promise<int> { co_return co_await user::posted_value{1}; };
    auto g = []() -> cobalt::generator<int> {
        co_yield co_await user::posted_value{2};
        co_return co_await user::posted_value{3};
    };
    auto r = cobalt::run([&]() -> cobalt::task<std::vector<int>> {
        auto gen = g();
        std::vector<int> v{co_await p()};
        while (gen)
            v.push_back(co_await gen);
        co_return v;
    }());
    CHECK(r == std::vector<int>{2, 4, 6});
}

TEST_CASE("custom: interruptible awaitable in race") {
    static_assert(cobalt::interruptible<user::waits_forever&>);
    auto t = []() -> cobalt::task<std::string> {
        user::waits_forever w;
        auto v = co_await cobalt::race(w, user::posted_value{5});
        // the user awaitable was interrupted, not cancelled, and reports it
        co_return std::to_string(v.index()) + ":" + std::to_string(boost::variant2::get<1>(v)) + ":" +
            (w.interrupted ? "interrupted" : "pending");
    };
    CHECK(cobalt::run(t()) == "1:10:interrupted");
}

TEST_CASE("custom: op") {
    auto t = [](std::chrono::milliseconds d) -> cobalt::task<bool> {
        bool shortcut = false;
        asio::steady_timer tim{co_await cobalt::this_coro::executor, d};
        if (d == 0ms)
            std::this_thread::sleep_for(1ms);  // already expired
        co_await user::wait_op(tim, &shortcut);
        co_return shortcut;
    };
    CHECK_FALSE(cobalt::run(t(5ms)));
    CHECK(cobalt::run(t(0ms)));
}

TEST_CASE("custom: move-only results") {
    auto make = [](int i) -> cobalt::promise<user::ticket> { co_return user::ticket{i}; };
    auto task = [](int i) -> cobalt::task<user::ticket> { co_return user::ticket{i}; };
    auto gen = []() -> cobalt::generator<user::ticket> {
        co_yield user::ticket{1};
        co_return user::ticket{2};
    };
    auto ids = cobalt::run([&]() -> cobalt::task<std::vector<int>> {
        std::vector<int> v;
        v.push_back(*(co_await make(7)).id);
        v.push_back(*(co_await task(8)).id);
        auto g = gen();
        while (g)
            v.push_back(*(co_await g).id);
        // through a channel
        cobalt::channel<user::ticket> ch{1u};
        co_await ch.write(user::ticket{9});
        v.push_back(*(co_await ch.read()).id);
        // join of move-only results
        auto [a, b] = co_await cobalt::join(make(10), task(11));
        v.push_back(*a.id);
        v.push_back(*b.id);
        co_return v;
    }());
    CHECK(ids == std::vector<int>{7, 8, 1, 2, 9, 10, 11});
}

TEST_CASE("custom: executor") {
    static_assert(asio::execution::executor<user::counting_executor>);
    asio::io_context ctx;
    user::counting_executor exec{&ctx};
    cobalt::executor any = exec;  // asio::any_io_executor
    CHECK(any.target<user::counting_executor>() != nullptr);
    int result = 0;
    cobalt::spawn(any,
                  []() -> cobalt::task<int> {
                      auto e = co_await cobalt::this_coro::executor;
                      co_await asio::post(e, cobalt::use_op);
                      co_return e.target<user::counting_executor>() != nullptr ? 1 : 0;
                  }(),
                  [&](std::exception_ptr ep, int v) {
                      CHECK_FALSE(ep);
                      result = v;
                  });
    ctx.run();
    CHECK(result == 1);
    CHECK(*exec.runs > 0);
}
