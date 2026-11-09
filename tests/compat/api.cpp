// mcpplibs.mwrap.boost.compat — every documented component of Boost.Compat
// (libs/compat/doc/compat/*.adoc): bind_front/bind_back, function_ref,
// integer_sequence, invoke and its traits, latch, mem_fn, move_only_function,
// nontype, shared_lock, to_array, to_underlying. Cases are named
// "doc: <page>#<section>" after the documentation section they exercise.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.compat;

namespace compat = boost::compat;

namespace {
struct X {
    int f(int a, int b) const noexcept { return a * 10 + b; }
    int g(int a) { return a + base; }
    int base = 100;
};
int add(int x, int y) noexcept { return x * 10 + y; }
struct point {
    int x = 1, y = 2;
};
}  // namespace

// ---- bind_front / bind_back ---------------------------------------------

TEST_CASE("doc: bind_back#example") {
    auto fn = compat::bind_back(&X::f, 1, 2);
    X x;
    CHECK(fn(x) == 12);
}

TEST_CASE("doc: bind_back#bind-back") {
    auto minus = compat::bind_back([](int a, int b) { return a - b; }, 1);
    CHECK(minus(10) == 9);
    std::string s = "x";
    auto append = compat::bind_back([](std::string a, const std::string& b) { return a + b; }, s);
    s = "changed";  // bound arguments are copied
    CHECK(append(std::string("a")) == "ax");
}

TEST_CASE("doc: bind_front#example") {
    X x;
    auto fn = compat::bind_front(&X::f, &x);
    CHECK(fn(1, 2) == 12);
}

TEST_CASE("doc: bind_front#bind-front") {
    auto minus = compat::bind_front([](int a, int b) { return a - b; }, 10);
    CHECK(minus(1) == 9);
    auto ptr = std::make_unique<int>(5);
    auto owns = compat::bind_front([](const std::unique_ptr<int>& p, int k) { return *p * k; },
                                   std::move(ptr));
    CHECK(owns(3) == 15);
}

// ---- function_ref --------------------------------------------------------

TEST_CASE("doc: function_ref#example") {
    auto add2 = [](int x, int y) { return x * 100 + y; };
    std::vector<compat::function_ref<int(int, int)>> fns;
    fns.push_back({add});
    fns.push_back({add2});
    std::vector<int> out;
    for (auto fn : fns) out.push_back(fn(1, 2));
    CHECK(out == std::vector<int>{12, 102});
}

TEST_CASE("doc: function_ref#function-pointer-constructor") {
    compat::function_ref<int(int, int)> f(&add);
    CHECK(f(3, 4) == 34);
}

TEST_CASE("doc: function_ref#object-constructor") {
    int calls = 0;
    auto counter = [&](int) { return ++calls; };
    compat::function_ref<int(int)> f(counter);
    f(0);
    f(0);
    CHECK(calls == 2);  // refers to the object, does not copy it
}

TEST_CASE("doc: function_ref#pointer-to-member-function-constructor") {
    point p;
    compat::function_ref<int(point const&)> f(compat::nontype<&point::x>);
    CHECK(f(p) == 1);
}

TEST_CASE("doc: function_ref#bound-object-constructor") {
    point p;
    compat::function_ref<int()> f(compat::nontype<&point::x>, p);
    CHECK(f() == 1);
}

TEST_CASE("doc: function_ref#bound-pointer-constructor") {
    point p;
    p.x = 7;
    compat::function_ref<int()> f(compat::nontype<&point::x>, &p);
    CHECK(f() == 7);
}

TEST_CASE("doc: function_ref#copy-constructor") {
    compat::function_ref<int(int, int)> f(add);
    compat::function_ref<int(int, int)> g(f);
    CHECK(g(1, 1) == 11);
}

TEST_CASE("doc: function_ref#call-operator") {
    compat::function_ref<int(int, int) const noexcept> f(add);
    static_assert(noexcept(f(1, 2)));
    CHECK(f(2, 3) == 23);
}

TEST_CASE("doc: function_ref#copy-assignment") {
    auto mul = [](int a, int b) { return a * b; };
    compat::function_ref<int(int, int)> f(add), g(mul);
    f = g;
    CHECK(f(3, 4) == 12);
}

// ---- integer_sequence ----------------------------------------------------

namespace {
template <class T, T... I>
constexpr T sum(compat::integer_sequence<T, I...>) {
    return (T(0) + ... + I);
}
template <std::size_t... I>
constexpr std::size_t count(compat::index_sequence<I...>) {
    return sizeof...(I);
}
}  // namespace

TEST_CASE("doc: integer_sequence#integer-sequence") {
    static_assert(sum(compat::integer_sequence<int, 1, 2, 3>()) == 6);
}
TEST_CASE("doc: integer_sequence#make-integer-sequence") {
    static_assert(sum(compat::make_integer_sequence<long, 5>()) == 10);
}
TEST_CASE("doc: integer_sequence#index-sequence") {
    static_assert(std::is_same_v<compat::index_sequence<0, 1>, compat::integer_sequence<std::size_t, 0, 1>>);
}
TEST_CASE("doc: integer_sequence#make-index-sequence") {
    static_assert(std::is_same_v<compat::make_index_sequence<3>, compat::index_sequence<0, 1, 2>>);
}
TEST_CASE("doc: integer_sequence#index-sequence-for") {
    static_assert(count(compat::index_sequence_for<int, char, double>()) == 3);
}

// ---- invoke ---------------------------------------------------------------

TEST_CASE("doc: invoke#example") {
    X x;
    CHECK(compat::invoke(&X::f, x, 1, 2) == 12);
}
TEST_CASE("doc: invoke#invoke") {
    X x;
    CHECK(compat::invoke(&X::g, &x, 1) == 101);
    CHECK(compat::invoke(&X::base, x) == 100);  // pointer to data member
    CHECK(compat::invoke(add, 4, 5) == 45);
    CHECK(compat::invoke([](int v) { return -v; }, 3) == -3);
    static_assert(noexcept(compat::invoke(&X::f, x, 1, 2)));
}
TEST_CASE("doc: invoke#invoke-result-t") {
    static_assert(std::is_same_v<compat::invoke_result_t<decltype(&X::f), X, int, int>, int>);
    static_assert(std::is_same_v<compat::invoke_result_t<decltype(&X::base), X&>, int&>);
}
TEST_CASE("doc: invoke#is-invocable") {
    static_assert(compat::is_invocable<decltype(&X::f), X, int, int>::value);
    static_assert(!compat::is_invocable<decltype(&X::f), X, int>::value);
}
TEST_CASE("doc: invoke#is-nothrow-invocable") {
    static_assert(compat::is_nothrow_invocable<decltype(&X::f), X, int, int>::value);
    static_assert(!compat::is_nothrow_invocable<decltype(&X::g), X&, int>::value);
}
TEST_CASE("doc: invoke#invoke-r") {
    X x;
    long r = compat::invoke_r<long>(&X::f, x, 1, 2);
    CHECK(r == 12L);
    compat::invoke_r<void>(&X::f, x, 1, 2);  // result discarded
}
TEST_CASE("doc: invoke#is-invocable-r") {
    static_assert(compat::is_invocable_r<long, decltype(&X::f), X, int, int>::value);
    static_assert(compat::is_invocable_r<void, decltype(&X::f), X, int, int>::value);
    static_assert(!compat::is_invocable_r<std::string, decltype(&X::f), X, int, int>::value);
}
TEST_CASE("doc: invoke#is-nothrow-invocable-r") {
    static_assert(compat::is_nothrow_invocable_r<long, decltype(&X::f), X, int, int>::value);
}

// ---- latch -----------------------------------------------------------------

TEST_CASE("doc: latch#example") {
    std::ptrdiff_t const num_threads = 8;
    compat::latch l(num_threads);
    std::atomic<int> after{0};
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i)
        threads.emplace_back([&] {
            l.arrive_and_wait();
            ++after;
        });
    for (auto& t : threads) t.join();
    CHECK(after == num_threads);
    CHECK(l.try_wait());
}
TEST_CASE("doc: latch#counter-constructor") {
    compat::latch l(2);
    CHECK_FALSE(l.try_wait());
    compat::latch zero(0);
    CHECK(zero.try_wait());
}
TEST_CASE("doc: latch#copy-constructor") {
    static_assert(!std::is_copy_constructible_v<compat::latch>);
    static_assert(!std::is_copy_assignable_v<compat::latch>);
}
TEST_CASE("doc: latch#count-down") {
    compat::latch l(3);
    l.count_down();
    l.count_down(2);
    CHECK(l.try_wait());
}
TEST_CASE("doc: latch#try-wait") {
    compat::latch l(1);
    CHECK_FALSE(l.try_wait());
    l.count_down();
    CHECK(l.try_wait());
}
TEST_CASE("doc: latch#wait") {
    compat::latch l(1);
    std::thread t([&] { l.count_down(); });
    l.wait();
    t.join();
    CHECK(l.try_wait());
}
TEST_CASE("doc: latch#arrive-and-wait") {
    compat::latch l(2);
    std::thread t([&] { l.arrive_and_wait(); });
    l.arrive_and_wait();
    t.join();
    CHECK(l.try_wait());
}
TEST_CASE("doc: latch#max") {
    static_assert(compat::latch::max() > 0);
    CHECK(compat::latch::max() >= 1000);
}

// ---- mem_fn ----------------------------------------------------------------

TEST_CASE("doc: mem_fn#example") {
    auto fn = compat::mem_fn(&X::f);
    X x;
    CHECK(fn(x, 1, 2) == 12);
}
TEST_CASE("doc: mem_fn#mem-fn") {
    auto data = compat::mem_fn(&X::base);
    X x;
    CHECK(data(x) == 100);
    CHECK(data(&x) == 100);
    std::vector<X> xs(3);
    std::vector<int> bases;
    std::transform(xs.begin(), xs.end(), std::back_inserter(bases), compat::mem_fn(&X::base));
    CHECK(bases == std::vector<int>{100, 100, 100});
}

// ---- move_only_function ------------------------------------------------------

namespace {
struct moveonly {
    std::unique_ptr<int> p_ = std::make_unique<int>(1234);
    auto operator()(int x) noexcept { return *p_ + x; }
};
}  // namespace

TEST_CASE("doc: move_only_function#example") {
    compat::move_only_function<int(int) noexcept> f(moveonly{});
    CHECK(f(4321) == 5555);
}
TEST_CASE("doc: move_only_function#default-constructor") {
    compat::move_only_function<void()> f;
    CHECK_FALSE(f);
}
TEST_CASE("doc: move_only_function#null-constructor") {
    compat::move_only_function<void()> f(nullptr);
    CHECK(f == nullptr);
}
TEST_CASE("doc: move_only_function#object-constructor") {
    compat::move_only_function<int(int)> f([k = std::make_unique<int>(2)](int v) { return v * *k; });
    CHECK(f(21) == 42);
    compat::move_only_function<int(int, int)> g(&add);
    CHECK(g(1, 2) == 12);
    compat::move_only_function<int(int, int)> null_ptr(static_cast<int (*)(int, int)>(nullptr));
    CHECK_FALSE(null_ptr);  // a null function pointer gives an empty object
}
TEST_CASE("doc: move_only_function#in-place-constructor") {
    compat::move_only_function<int(int)> f(compat::in_place_type_t<moveonly>{});
    CHECK(f(1) == 1235);
}
TEST_CASE("doc: move_only_function#list-in-place-constructor") {
    struct summer {
        std::vector<int> v;
        summer(std::initializer_list<int> il, int extra) : v(il) { v.push_back(extra); }
        int operator()() const { return std::accumulate(v.begin(), v.end(), 0); }
    };
    compat::move_only_function<int() const> f(compat::in_place_type_t<summer>{}, {1, 2, 3}, 4);
    CHECK(f() == 10);
}
TEST_CASE("doc: move_only_function#move-constructor") {
    compat::move_only_function<int(int) noexcept> f(moveonly{});
    compat::move_only_function<int(int) noexcept> g(std::move(f));
    CHECK(g(0) == 1234);
}
TEST_CASE("doc: move_only_function#move-assignment") {
    compat::move_only_function<int(int) noexcept> f(moveonly{}), g;
    g = std::move(f);
    CHECK(g(1) == 1235);
}
TEST_CASE("doc: move_only_function#null-assigment") {
    compat::move_only_function<int(int) noexcept> f(moveonly{});
    f = nullptr;
    CHECK_FALSE(f);
}
TEST_CASE("doc: move_only_function#callable-assignment") {
    compat::move_only_function<int(int)> f;
    f = [](int v) { return v + 1; };
    CHECK(f(1) == 2);
}
TEST_CASE("doc: move_only_function#null-equality") {
    compat::move_only_function<void()> f, g([] {});
    CHECK(f == nullptr);
    CHECK(g != nullptr);
}
TEST_CASE("doc: move_only_function#swap") {
    compat::move_only_function<int()> a([] { return 1; }), b([] { return 2; });
    a.swap(b);
    CHECK(a() == 2);
    swap(a, b);
    CHECK(a() == 1);
}
TEST_CASE("doc: move_only_function#boolean-conversion") {
    compat::move_only_function<void()> f;
    CHECK_FALSE(static_cast<bool>(f));
    f = [] {};
    CHECK(static_cast<bool>(f));
}
TEST_CASE("doc: move_only_function#call-operator") {
    struct qualified {
        int operator()() & { return 1; }
        int operator()() && { return 2; }
        int operator()() const& { return 3; }
    };
    compat::move_only_function<int() &> l(qualified{});
    CHECK(l() == 1);
    compat::move_only_function<int() &&> r(qualified{});
    CHECK(std::move(r)() == 2);
    const compat::move_only_function<int() const&> c(qualified{});
    CHECK(c() == 3);
}

// ---- nontype ------------------------------------------------------------------

TEST_CASE("doc: nontype#example") {
    X x;
    auto fnref = compat::function_ref<int(int, int)>(compat::nontype<&X::f>, &x);
    CHECK(fnref(1, 2) == 12);
    static_assert(std::is_same_v<decltype(compat::nontype<&add>), const compat::nontype_t<&add>>);
}

// ---- shared_lock --------------------------------------------------------------

TEST_CASE("doc: shared_lock#example") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> guard(m);
    CHECK(guard.owns_lock());
    CHECK_FALSE(m.try_lock());  // a writer is excluded
}
TEST_CASE("doc: shared_lock#default-constructor") {
    compat::shared_lock<std::shared_mutex> l;
    CHECK(l.mutex() == nullptr);
    CHECK_FALSE(l.owns_lock());
}
TEST_CASE("doc: shared_lock#locking-constructor") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> a(m), b(m);  // shared: both hold it
    CHECK(a.owns_lock());
    CHECK(b.owns_lock());
}
TEST_CASE("doc: shared_lock#deferred-constructor") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m, std::defer_lock);
    CHECK_FALSE(l.owns_lock());
    CHECK(l.mutex() == &m);
}
TEST_CASE("doc: shared_lock#try-to-lock-constructor") {
    std::shared_mutex m;
    m.lock();
    compat::shared_lock<std::shared_mutex> l(m, std::try_to_lock);
    CHECK_FALSE(l.owns_lock());
    m.unlock();
    compat::shared_lock<std::shared_mutex> l2(m, std::try_to_lock);
    CHECK(l2.owns_lock());
}
TEST_CASE("doc: shared_lock#adopting-constructor") {
    std::shared_mutex m;
    m.lock_shared();
    compat::shared_lock<std::shared_mutex> l(m, std::adopt_lock);
    CHECK(l.owns_lock());
}
TEST_CASE("doc: shared_lock#copy-constructor") {
    static_assert(!std::is_copy_constructible_v<compat::shared_lock<std::shared_mutex>>);
}
TEST_CASE("doc: shared_lock#copy-assignment") {
    static_assert(!std::is_copy_assignable_v<compat::shared_lock<std::shared_mutex>>);
}
TEST_CASE("doc: shared_lock#move-constructor") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> a(m);
    compat::shared_lock<std::shared_mutex> b(std::move(a));
    CHECK(b.owns_lock());
    CHECK_FALSE(a.owns_lock());
    CHECK(a.mutex() == nullptr);
}
TEST_CASE("doc: shared_lock#move-assignment") {
    std::shared_mutex m1, m2;
    compat::shared_lock<std::shared_mutex> a(m1), b(m2);
    b = std::move(a);  // releases m2
    CHECK(b.mutex() == &m1);
    CHECK(m2.try_lock());
    m2.unlock();
}
TEST_CASE("doc: shared_lock#destructor") {
    std::shared_mutex m;
    { compat::shared_lock<std::shared_mutex> l(m); }
    CHECK(m.try_lock());
    m.unlock();
}
TEST_CASE("doc: shared_lock#lock") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m, std::defer_lock);
    l.lock();
    CHECK(l.owns_lock());
    CHECK_THROWS_AS(l.lock(), std::system_error);  // already owns
}
TEST_CASE("doc: shared_lock#try-lock") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m, std::defer_lock);
    CHECK(l.try_lock());
    compat::shared_lock<std::shared_mutex> none;
    CHECK_THROWS_AS(none.try_lock(), std::system_error);  // no mutex
}
TEST_CASE("doc: shared_lock#unlock") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m);
    l.unlock();
    CHECK_FALSE(l.owns_lock());
    CHECK_THROWS_AS(l.unlock(), std::system_error);
}
TEST_CASE("doc: shared_lock#swap") {
    std::shared_mutex m1, m2;
    compat::shared_lock<std::shared_mutex> a(m1), b(m2, std::defer_lock);
    a.swap(b);
    CHECK(a.mutex() == &m2);
    CHECK(b.owns_lock());
}
TEST_CASE("doc: shared_lock#free-function-swap") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> a(m), b;
    swap(a, b);
    CHECK(b.owns_lock());
}
TEST_CASE("doc: shared_lock#release") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m);
    std::shared_mutex* p = l.release();
    CHECK(p == &m);
    CHECK_FALSE(l.owns_lock());
    p->unlock_shared();
}
TEST_CASE("doc: shared_lock#mutex") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m);
    CHECK(l.mutex() == &m);
}
TEST_CASE("doc: shared_lock#owns-lock") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m, std::defer_lock);
    CHECK_FALSE(l.owns_lock());
}
TEST_CASE("doc: shared_lock#boolean-conversion") {
    std::shared_mutex m;
    compat::shared_lock<std::shared_mutex> l(m);
    CHECK(static_cast<bool>(l));
}

// ---- to_array / to_underlying -------------------------------------------------

TEST_CASE("doc: to_array#example") {
    int input[] = {1, 2, 3};
    std::array<int, 3> output = compat::to_array(input);
    CHECK(output == std::array<int, 3>{{1, 2, 3}});
}
TEST_CASE("doc: to_array#to-array") {
    constexpr auto a = compat::to_array({4, 5});
    static_assert(a.size() == 2 && a[1] == 5);
    const char str[] = "hi";
    auto c = compat::to_array(str);
    static_assert(std::is_same_v<decltype(c), std::array<char, 3>>);
    auto m = compat::to_array({std::make_unique<int>(1)});  // moves from the rvalue array
    CHECK(*m[0] == 1);
}

namespace {
enum e : std::uint16_t { e_value = 5 };
enum class color : char { red = 'r' };
}  // namespace

TEST_CASE("doc: to_underlying#example") {
    auto output = compat::to_underlying(e_value);
    static_assert(std::is_same_v<decltype(output), std::uint16_t>);
    CHECK(output == 5);
}
TEST_CASE("doc: to_underlying#to-underlying") {
    static_assert(compat::to_underlying(color::red) == 'r');
    static_assert(noexcept(compat::to_underlying(color::red)));
}
