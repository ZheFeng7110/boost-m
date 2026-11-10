// mcpplibs.mwrap.boost.cobalt — the documentation's code (libs/cobalt/doc/):
// the background pages (asio::awaitable, executors and their properties, lazy
// and eager coroutines, stackless resumption), the benchmarks' three loops,
// the compiler page's final awaitable, the design pages (associators,
// concepts, promise, race and interrupt_await, threading, strands), the
// primer (async programming, awaitables, co_await, co_yield), the tour
// (entry points, generator, join, promise, race, task) and the push-value
// generator of the tutorial. Declarations the pages leave undefined
// (do_the_thing, gen1, some_data_source, ...) are given small bodies; printf
// output goes to a string the cases compare.
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
namespace bsys = boost::system;  // (`system` would clash with ::system)
namespace variant2 = boost::variant2;
using namespace std::chrono_literals;

namespace {
std::string out;
template <class... A>
void print(const char* fmt, A... a) {
    char buf[256];
    std::snprintf(buf, sizeof buf, fmt, a...);
    out += buf;
}
}  // namespace

// ---- background ----

TEST_CASE("doc: asio-awaitable") {
    // asio::awaitable's await_suspend takes only asio's own frames, so a cobalt
    // coroutine cannot co_await one; it runs it with co_spawn instead.
    static_assert(!cobalt::awaitable_type<asio::awaitable<int>, cobalt::detail::task_promise<int>>);
    auto value = []() -> asio::awaitable<int> { co_return 7; };
    auto t = [&]() -> cobalt::task<int> {
        co_return co_await asio::co_spawn(co_await cobalt::this_coro::executor, value(), cobalt::use_op);
    };
    CHECK(cobalt::run(t()) == 7);
}

namespace {
// A minimal executor over an io_context, then a work-tracking one.
struct minimal_executor {
    asio::io_context* ctx = nullptr;
    asio::execution_context& query(asio::execution::context_t) const { return *ctx; }
    static constexpr asio::execution::blocking_t query(asio::execution::blocking_t) noexcept {
        return asio::execution::blocking.never;
    }
    template <class F>
    void execute(F&& f) const {
        asio::post(*ctx, std::forward<F>(f));
    }
    bool operator==(minimal_executor const& other) const noexcept { return ctx == other.ctx; }
    bool operator!=(minimal_executor const& other) const noexcept { return ctx != other.ctx; }
};
static_assert(asio::execution::executor<minimal_executor>);

struct trackable_executor : minimal_executor {
    bool tracked = false;
    constexpr trackable_executor require(asio::execution::outstanding_work_t::tracked_t) const {
        auto e = *this;
        e.tracked = true;
        return e;
    }
    constexpr trackable_executor require(asio::execution::outstanding_work_t::untracked_t) const {
        auto e = *this;
        e.tracked = false;
        return e;
    }
    constexpr asio::execution::outstanding_work_t query(asio::execution::outstanding_work_t) const {
        return tracked ? asio::execution::outstanding_work_t(asio::execution::outstanding_work.tracked)
                       : asio::execution::outstanding_work_t(asio::execution::outstanding_work.untracked);
    }
    constexpr trackable_executor prefer(asio::execution::outstanding_work_t::tracked_t t) const { return require(t); }
    constexpr trackable_executor prefer(asio::execution::outstanding_work_t::untracked_t u) const { return require(u); }
    asio::execution::relationship_t query(asio::execution::relationship_t) const {
        return asio::execution::relationship.fork;
    }
};
}  // namespace

TEST_CASE("doc: a-minimal-executor") {
    asio::io_context ctx;
    minimal_executor e{&ctx};
    int ran = 0;
    e.execute([&] { ++ran; });
    CHECK(ran == 0);  // blocking.never
    ctx.run();
    CHECK(ran == 1);
    CHECK(&asio::query(e, asio::execution::context) == &ctx);
    CHECK(asio::query(e, asio::execution::blocking) == asio::execution::blocking.never);
}

TEST_CASE("doc: adding-a-work-guard") {
    asio::io_context ctx;
    trackable_executor e{{&ctx}};
    auto t = asio::require(e, asio::execution::outstanding_work.tracked);
    CHECK(t.tracked);
    auto u = asio::require(t, asio::execution::outstanding_work.untracked);
    CHECK_FALSE(u.tracked);
    CHECK(asio::prefer(e, asio::execution::outstanding_work.tracked).tracked);
}

TEST_CASE("doc: execute") {
    asio::io_context ctx;
    std::vector<int> order;
    minimal_executor e{&ctx};
    e.execute([&] { order.push_back(1); });
    e.execute([&] { order.push_back(2); });
    ctx.run();
    CHECK(order == std::vector<int>{1, 2});
}

TEST_CASE("doc: properties") {
    asio::io_context ctx;
    trackable_executor e{{&ctx}};
    CHECK(asio::query(e, asio::execution::relationship) == asio::execution::relationship.fork);
    // a property the executor does not have can be preferred, not required
    auto p = asio::prefer(e, asio::execution::blocking.possibly);
    static_assert(std::is_same_v<decltype(p), trackable_executor>);
}

TEST_CASE("doc: properties-of-the-asio-any-io-executor") {
    asio::io_context ctx;
    // the properties any_io_executor needs: context, blocking.never, the
    // outstanding_work requirements and equality
    asio::any_io_executor any = ctx.get_executor();
    CHECK(&asio::query(any, asio::execution::context) == &ctx);
    CHECK(asio::query(asio::require(any, asio::execution::blocking.never), asio::execution::blocking) ==
          asio::execution::blocking.never);
    // a user executor with them: custom_types.cpp
    cobalt::executor exec = any;
    int ran = 0;
    asio::post(exec, [&] { ++ran; });
    ctx.run();
    CHECK(ran == 1);
}

namespace {
// The fictional coroutine types of the lazy & eager page.
template <bool Eager>
struct demo_coro {
    struct promise_type {
        demo_coro get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        auto initial_suspend() noexcept {
            if constexpr (Eager)
                return std::suspend_never{};
            else
                return std::suspend_always{};
        }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(int) noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() { throw; }
    };
    std::coroutine_handle<promise_type> h;
    void resume() { h.resume(); }
    void reenter() { h.resume(); }
    ~demo_coro() {
        if (h)
            h.destroy();
    }
};
using lazy_coro = demo_coro<false>;
using eager_coro = demo_coro<true>;
}  // namespace

TEST_CASE("doc: lazy-eager") {
    out.clear();
    {
        auto co_example = []() -> lazy_coro {
            print("Entered coro\n");
            co_yield 0;
            print("Coro done\n");
        };
        print("enter main\n");
        auto lazy = co_example();
        print("constructed coro\n");
        lazy.resume();
        print("resumed once\n");
        lazy.resume();
        print("resumed twice\n");
    }
    // (the page writes "Coro Done": the code prints "Coro done")
    CHECK(out == "enter main\nconstructed coro\nEntered coro\nresumed once\nCoro done\nresumed twice\n");
    out.clear();
    {
        auto co_example = []() -> eager_coro {
            print("Entered coro\n");
            co_yield 0;
            print("Coro done\n");
        };
        print("enter main\n");
        auto lazy = co_example();
        print("constructed coro\n");
        lazy.resume();
        print("resumed once\n");
    }
    // (the page's listing says "resume once" / "Coro Done")
    CHECK(out == "enter main\nEntered coro\nconstructed coro\nCoro done\nresumed once\n");
}

namespace {
int bar() { return 0; }  // the deepest point of the stack
int foo() { return bar(); }
}  // namespace

TEST_CASE("doc: stackless") {
    CHECK(foo() == 0);
    // a coroutine resumed from a nested function returns there, not to main
    out.clear();
    auto example = []() -> eager_coro {
        print("example 1\n");
        co_yield 0;
        print("example 2\n");
        co_yield 1;
        print("example 3\n");
    };
    auto nested_resume = [](eager_coro& f) {
        print("nested_resume\n");
        f.resume();
        print("nested_resume returns\n");
    };
    auto f = example();
    nested_resume(f);
    f.reenter();
    CHECK(out == "example 1\nnested_resume\nexample 2\nnested_resume returns\nexample 3\n");
}

// ---- benchmarks ----

namespace {
constexpr std::size_t n = 1000;
}

TEST_CASE("doc: immediate") {
    auto atest = []() -> cobalt::task<std::size_t> {
        asio::experimental::channel<void(bsys::error_code)> chan{co_await cobalt::this_coro::executor, 1u};
        std::size_t i = 0u;
        for (; i < n; i++) {
            co_await chan.async_send(bsys::error_code{}, cobalt::use_op);
            co_await chan.async_receive(cobalt::use_op);
        }
        co_return i;
    };
    CHECK(cobalt::run(atest()) == n);
}

TEST_CASE("doc: posting-to-an-executor") {
    auto atest = []() -> cobalt::task<std::size_t> {
        std::size_t i = 0u;
        for (; i < n; i++)
            co_await asio::post(cobalt::use_op);
        co_return i;
    };
    CHECK(cobalt::run(atest()) == n);
}

TEST_CASE("doc: running-noop-coroutine-in-parallel") {
    auto atest = []() -> cobalt::task<std::size_t> {
        asio::experimental::channel<void(bsys::error_code)> chan{co_await cobalt::this_coro::executor, 0u};
        std::size_t i = 0u;
        for (; i < n; i++)
            co_await cobalt::gather(chan.async_send(bsys::error_code{}, cobalt::use_task),
                                    chan.async_receive(cobalt::use_task));
        co_return i;
    };
    CHECK(cobalt::run(atest()) == n);

    auto awtest = []() -> asio::awaitable<std::size_t> {
        asio::experimental::channel<void(bsys::error_code)> chan{co_await asio::this_coro::executor, 0u};
        using boost::asio::experimental::awaitable_operators::operator&&;
        std::size_t i = 0u;
        for (; i < n; i++)
            co_await (chan.async_send(bsys::error_code{}, asio::use_awaitable) &&
                      chan.async_receive(asio::use_awaitable));
        co_return i;
    };
    asio::io_context ctx;
    std::size_t done = 0;
    asio::co_spawn(ctx, awtest(), [&](std::exception_ptr ep, std::size_t r) {
        CHECK_FALSE(ep);
        done = r;
    });
    ctx.run();
    CHECK(done == n);
}

// ---- compiler ----

namespace {
// The page's final awaitable: destroy the frame, then continue.
struct self_destroying {
    struct promise_type {
        std::coroutine_handle<void> my_continuation{std::noop_coroutine()};
        int* result = nullptr;
        self_destroying get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
        std::suspend_always initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept {
            struct final_awaitable {
                std::coroutine_handle<void> continuation{std::noop_coroutine()};
                bool await_ready() const noexcept { return false; }
                std::coroutine_handle<void> await_suspend(std::coroutine_handle<void> h) noexcept {
                    auto cc = continuation;
                    h.destroy();
                    return cc;
                }
                void await_resume() noexcept {}
            };
            return final_awaitable{my_continuation};
        }
        void return_value(int v) { *result = v; }
        void unhandled_exception() { throw; }
    };
    std::coroutine_handle<promise_type> h;
};
}  // namespace

TEST_CASE("doc: compiler") {
    int result = 0;
    auto c = []() -> self_destroying { co_return 3; }();
    c.h.promise().result = &result;
    c.h.resume();  // runs to the end; the frame destroys itself
    CHECK(result == 3);
}

// ---- design ----

namespace {
// An awaitable using the associators of the awaiting promise.
struct my_awaitable {
    bool saw_executor = false, saw_slot = false;
    bool await_ready() { return false; }
    template <typename Promise>
    bool await_suspend(std::coroutine_handle<Promise> h) {
        if constexpr (requires(Promise p) { p.get_executor(); })
            saw_executor = true;
        if constexpr (requires(Promise p) { p.get_cancellation_slot(); })
            saw_slot = h.promise().get_cancellation_slot().is_connected();
        return false;  // resume right away
    }
    std::pair<bool, bool> await_resume() { return {saw_executor, saw_slot}; }
};
}  // namespace

TEST_CASE("doc: associators") {
    auto t = []() -> cobalt::task<std::pair<bool, bool>> { co_return co_await my_awaitable{}; };
    auto [exec, slot] = cobalt::run(t());
    CHECK(exec);
    CHECK(slot);
}

TEST_CASE("doc: concepts") {
    static_assert(cobalt::awaitable_type<my_awaitable>);
    static_assert(cobalt::awaitable<cobalt::promise<void>>);
    static_assert(!cobalt::awaitable<int>);
    auto t = []() -> cobalt::task<bool> {
        asio::steady_timer tim{co_await cobalt::this_coro::executor, 5ms};
        co_await tim.async_wait(cobalt::use_op);
        auto exec = co_await cobalt::this_coro::executor;
        co_return exec == cobalt::this_thread::get_executor();
    };
    CHECK(cobalt::run(t()));
}

TEST_CASE("doc: promise") {
    auto noop = []() -> cobalt::promise<void> { co_return; };
    auto t = [&]() -> cobalt::task<bool> {
        auto p = noop();  // eager: already done
        bool ready = p.ready();
        co_await p;
        co_return ready;
    };
    CHECK(cobalt::run(t()));
}

namespace {
// A wait that a race may cancel (a generator in an operation cannot be
// interrupted, so race cancels it): it starts over, and neither it nor the
// generators awaiting it throw once cancelled, so they stay usable.
cobalt::promise<void> slow() {
    co_await cobalt::this_coro::throw_if_cancelled(false);
    asio::steady_timer t{co_await cobalt::this_coro::executor};
    for (;;) {
        t.expires_after(50ms);
        auto [ec] = co_await t.async_wait(asio::as_tuple(cobalt::use_op));
        if (!ec)
            co_return;
    }
}
cobalt::generator<int> gen1() {
    for (int i = 0; i < 3; i++)
        co_yield i;
    co_return 3;
}
cobalt::generator<double> gen2() {
    co_await cobalt::this_coro::throw_if_cancelled(false);
    for (int i = 0;; i++) {
        co_await slow();  // later than gen1
        co_yield i * 0.5;
    }
}
}  // namespace

TEST_CASE("doc: interrupt-await") {
    // A generator can take part in race without being cancelled: race
    // interrupts its await (interrupt_await) and the generator is kept.
    auto t = []() -> cobalt::task<int> {
        auto g = gen1();
        auto g2 = gen2();
        auto v = co_await cobalt::race(g, g2);
        int got = variant2::get<0>(v);
        got += co_await g;  // g still usable after the race
        co_return got;
    };
    CHECK(cobalt::run(t()) == 0 + 1);
}

TEST_CASE("doc: race") {
    out.clear();
    auto p = []() -> cobalt::promise<void> {
        auto g1 = gen1();
        auto g2 = gen2();
        while (g1) {
            switch (auto v = co_await cobalt::race(g1, g2); v.index()) {
            case 0:
                print("Got int %d\n", variant2::get<0>(v));
                break;
            case 1:
                print("Got double %f\n", variant2::get<1>(v));
                break;
            }
        }
    };
    cobalt::run([&]() -> cobalt::task<void> { co_await p(); }());
    CHECK(out == "Got int 0\nGot int 1\nGot int 2\nGot int 3\n");

    // race(timeout(), work())
    auto timeout = []() -> cobalt::promise<void> {
        asio::steady_timer t{co_await cobalt::this_coro::executor, 10ms};
        co_await t.async_wait(cobalt::use_op);
    };
    auto work = []() -> cobalt::promise<void> {
        asio::steady_timer t{co_await cobalt::this_coro::executor, 5s};
        co_await t.async_wait(cobalt::use_op);
    };
    auto winner = cobalt::run([&]() -> cobalt::task<std::size_t> {
        co_return co_await cobalt::race(timeout(), work());  // the winner's index (void results)
    }());
    CHECK(winner == 0);
}

TEST_CASE("doc: threading") {
    // cobalt coroutines are bound to their thread's executor; a cobalt::thread
    // runs its own.
    auto main_exec_thread = std::this_thread::get_id();
    std::thread::id inner;
    auto thr = [&]() -> cobalt::thread {
        inner = std::this_thread::get_id();
        co_return;
    };
    auto t = thr();
    t.join();
    CHECK(inner != main_exec_thread);
}

namespace {
cobalt::promise<int> example_with_executor(int some_arg, asio::executor_arg_t = asio::executor_arg,
                                           cobalt::executor = cobalt::this_thread::get_executor()) {
    co_return some_arg * 2;
}
}  // namespace

TEST_CASE("doc: using-strands") {
    asio::thread_pool tp{2};
    auto strand = asio::make_strand(tp.get_executor());
    std::promise<int> result;
    cobalt::spawn(strand,
                  [&]() -> cobalt::task<int> {
                      co_return co_await example_with_executor(21, asio::executor_arg,
                                                               co_await cobalt::this_coro::executor);
                  }(),
                  [&](std::exception_ptr ep, int v) {
                      if (ep)
                          result.set_exception(ep);
                      else
                          result.set_value(v);
                  });
    CHECK(result.get_future().get() == 42);
    tp.join();
    // on the thread's own executor, through the default arguments
    CHECK(cobalt::run([]() -> cobalt::task<int> { co_return co_await example_with_executor(5); }()) == 10);
}

// ---- primer ----

namespace {
// http_get stands in for a request: a timer.
cobalt::promise<std::string> http_cobalt_get(std::string_view url) {
    asio::steady_timer t{co_await cobalt::this_coro::executor, 10ms};
    co_await t.async_wait(cobalt::use_op);
    co_return "<" + std::string(url) + ">";
}
}  // namespace

TEST_CASE("doc: async-programming") {
    auto start = std::chrono::steady_clock::now();
    auto [res, other_res] = cobalt::run([]() -> cobalt::task<std::tuple<std::string, std::string>> {
        co_return co_await cobalt::join(http_cobalt_get("https://boost.org"),
                                        http_cobalt_get("https://cppalliance.org"));
    }());
    CHECK(res == "<https://boost.org>");
    CHECK(other_res == "<https://cppalliance.org>");
    CHECK(std::chrono::steady_clock::now() - start < 1s);  // concurrently, not one after the other
}

namespace {
struct awaitable_with_int_result {
    bool await_ready() { return true; }
    void await_suspend(std::coroutine_handle<>) {}
    int await_resume() { return 11; }
};
}  // namespace

TEST_CASE("doc: awaitables") {
    auto t = []() -> cobalt::task<int> {
        int i = co_await awaitable_with_int_result();
        co_return i;
    };
    CHECK(cobalt::run(t()) == 11);
}

namespace {
cobalt::promise<void> delay(std::chrono::milliseconds ms) {
    asio::steady_timer t{co_await cobalt::this_coro::executor, ms};
    co_await t.async_wait(cobalt::use_op);
}
cobalt::promise<std::string> read_some() { co_return "some"; }
}  // namespace

TEST_CASE("doc: co-await") {
    auto example = []() -> cobalt::task<std::string> {
        co_await delay(std::chrono::milliseconds(50));
        std::string res = co_await read_some();
        co_return res;
    };
    CHECK(cobalt::run(example()) == "some");
}

TEST_CASE("doc: co-yield") {
    auto iota = [](int max) -> cobalt::generator<int> {
        int i = 0;
        while (i < max)
            co_yield i++;
        co_return i;
    };
    auto values = cobalt::run([&]() -> cobalt::task<std::vector<int>> {
        std::vector<int> v;
        auto g = iota(3);
        while (g)
            v.push_back(co_await g);
        co_return v;
    }());
    CHECK(values == std::vector<int>{0, 1, 2, 3});

    // a pushed value: co_yield returns what the awaiter passes in
    auto iota2 = []() -> cobalt::generator<int, bool> {
        int i = 0;
        bool more = false;
        do {
            more = co_yield i++;
        } while (more);
        co_return -1;
    };
    auto pushed = cobalt::run([&]() -> cobalt::task<std::vector<int>> {
        std::vector<int> v;
        auto g = iota2();
        v.push_back(co_await g(true));
        v.push_back(co_await g(true));
        v.push_back(co_await g(false));
        v.push_back(co_await g(false));
        co_return v;
    }());
    CHECK(pushed == std::vector<int>{0, 1, 2, -1});
}

// ---- tour ----

TEST_CASE("doc: entry-into-a-cobalt-environment") {
    // cobalt::main is the first; this file cannot have it (see examples.cpp).
    // cobalt::thread
    bool in_thread = false;
    auto my_thread = [&]() -> cobalt::thread {
        in_thread = true;
        co_return;
    };
    auto t = my_thread();
    t.join();
    CHECK(in_thread);
    // run (sync) and spawn
    int runs = 0;
    auto my_task = [&]() -> cobalt::task<void> {
        ++runs;
        co_return;
    };
    cobalt::run(my_task());
    asio::io_context ctx;
    cobalt::spawn(ctx, my_task(), asio::detached);
    ctx.run();
    CHECK(runs == 2);
}

TEST_CASE("doc: tour-generator") {
    out.clear();
    auto my_generator = []() -> cobalt::generator<int> {
        for (int i = 0; i < 10; i++)
            co_yield i;
        co_return 10;
    };
    cobalt::run([&]() -> cobalt::task<void> {
        auto g = my_generator();
        while (g)
            print("Generator %d\n", co_await g);
    }());
    std::string expect;
    for (int i = 0; i <= 10; i++)
        expect += "Generator " + std::to_string(i) + "\n";
    CHECK(out == expect);

    // eager push generator (the page calls it my_generator in co_main)
    auto my_eager_push_generator = [](int value) -> cobalt::generator<std::string, int> {
        while (value != 0)
            value = co_yield std::to_string(value);
        co_return "";
    };
    cobalt::run([&]() -> cobalt::task<void> {
        auto g = my_eager_push_generator(5);
        CHECK("5" == co_await g(4));
        CHECK("4" == co_await g(3));
        CHECK("3" == co_await g(2));
        CHECK("2" == co_await g(1));
        CHECK("1" == co_await g(0));
    }());

    // lazy: waits for the first pushed value (this_coro::initial)
    auto my_lazy_push_generator = []() -> cobalt::generator<std::string, int> {
        auto value = co_await cobalt::this_coro::initial;
        while (value != 0)
            value = co_yield std::to_string(value);
        co_return "";
    };
    cobalt::run([&]() -> cobalt::task<void> {
        auto g = my_lazy_push_generator();
        CHECK("5" == co_await g(5));
        CHECK("4" == co_await g(4));
        CHECK("3" == co_await g(3));
        CHECK("2" == co_await g(2));
        CHECK("1" == co_await g(1));
    }());
}

namespace {
cobalt::promise<int> some_work() { co_return 1; }
cobalt::promise<double> more_work() { co_return 2.5; }
cobalt::promise<void> do_the_thing() { co_await delay(1ms); }
cobalt::promise<void> do_the_other_thing() { co_await delay(1ms); }
}  // namespace

TEST_CASE("doc: tour-join") {
    auto res = cobalt::run([]() -> cobalt::task<std::tuple<int, double>> {
        std::tuple<int, double> res = co_await cobalt::join(some_work(), more_work());
        co_return res;
    }());
    CHECK(res == std::tuple<int, double>{1, 2.5});
}

TEST_CASE("doc: promises") {
    auto my_promise = []() -> cobalt::promise<int> {
        co_await do_the_thing();
        co_return 0;
    };
    auto res = cobalt::run([&]() -> cobalt::task<int> {
        auto p = my_promise();  // starts here
        co_await do_the_other_thing();
        auto res = co_await p;
        co_return res;
    }());
    CHECK(res == 0);
}

namespace {
cobalt::generator<int> some_data_source() {
    for (int i = 1; i <= 2; i++)
        co_yield i;
    co_return 3;
}
cobalt::generator<double> another_data_source() {
    co_await cobalt::this_coro::throw_if_cancelled(false);
    for (;;) {
        co_await slow();  // slower than some_data_source
        co_yield 0.5;
    }
}
}  // namespace

TEST_CASE("doc: tour-race") {
    out.clear();
    cobalt::run([]() -> cobalt::task<void> {
        auto g1 = some_data_source();
        auto g2 = another_data_source();
        int res1 = co_await g1;
        double res2 = 2.0;  // (the page awaits g2 here too)
        print("Result: %f\n", res1 * res2);
        while (g1 && g2) {
            switch (variant2::variant<int, double> nx = co_await cobalt::race(g1, g2); nx.index()) {
            case 0:
                res1 = variant2::get<0>(nx);
                break;
            case 1:
                res2 = variant2::get<1>(nx);
                break;
            }
            print("New result: %f\n", res1 * res2);
        }
    }());
    CHECK(out == "Result: 2.000000\nNew result: 4.000000\nNew result: 6.000000\n");
}

TEST_CASE("doc: tasks") {
    bool started = false;
    auto my_task = [&]() -> cobalt::task<int> {
        started = true;
        co_await do_the_thing();
        co_return 0;
    };
    auto res = cobalt::run([&]() -> cobalt::task<int> {
        auto t = my_task();  // lazy: not started yet
        co_await do_the_other_thing();
        CHECK_FALSE(started);
        auto res = co_await t;
        co_return res;
    }());
    CHECK(started);
    CHECK(res == 0);
}

// ---- tutorial: generator with push value ----

namespace {
// json_writer with a string sink in place of the websocket.
struct sink {
    std::string written;
    bool open = true;
    cobalt::promise<void> cobalt_write(std::string_view s) {
        written += s;
        written += '\n';
        co_return;
    }
};
cobalt::generator<bsys::error_code, std::string> json_writer(sink& ws) {
    try {
        while (ws.open) {
            auto val = co_yield bsys::error_code{};
            co_await ws.cobalt_write(val);
        }
        co_return {};
    } catch (bsys::system_error& e) {
        co_return e.code();
    }
}
}  // namespace

TEST_CASE("doc: generator-with-push-value") {
    sink my_ws;
    auto ec = cobalt::run([&]() -> cobalt::task<bsys::error_code> {
        auto g = json_writer(my_ws);
        std::vector<std::string> to_write{R"({"a":1})", R"({"b":2})"};
        for (auto&& tw : std::move(to_write))
            if (auto ec = co_await g(std::move(tw)))
                co_return ec;  // yield error
        my_ws.open = false;
        co_return co_await g(std::string());
    }());
    CHECK_FALSE(ec);
    // each pushed value is written when the generator resumes for the next
    CHECK(my_ws.written == "{\"a\":1}\n{\"b\":2}\n");
}
