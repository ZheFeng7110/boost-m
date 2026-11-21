// mcpplibs.mwrap.boost.compat — user callables and user types through the
// type-erasing and binding components: function_ref and move_only_function
// over user function objects (stateful, move-only, overloaded on value
// category), bind_front/bind_back/invoke over user member functions and data
// members, to_underlying on user enums, shared_lock over a user SharedMutex.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.compat;

namespace compat = boost::compat;

namespace user {
struct account {
    std::string owner;
    long balance = 0;
    long deposit(long amount) {
        balance += amount;
        return balance;
    }
    bool can_pay(long amount) const noexcept { return balance >= amount; }
};

struct audit_log {
    std::vector<std::string> lines;
    void operator()(std::string_view s) { lines.emplace_back(s); }
};

class unique_task {
public:
    explicit unique_task(std::string name) : name_(std::make_unique<std::string>(std::move(name))) {}
    std::string operator()() && { return "ran " + *name_; }
private:
    std::unique_ptr<std::string> name_;
};

// A user SharedMutex that records what the lock did.
class tracing_mutex {
public:
    void lock_shared() { log.push_back("S+"); }
    bool try_lock_shared() { log.push_back("S?"); return true; }
    void unlock_shared() { log.push_back("S-"); }
    std::vector<std::string> log;
};

enum class level : std::uint8_t { debug = 1, error = 9 };
}  // namespace user

TEST_CASE("function_ref over user function objects") {
    user::audit_log log;
    compat::function_ref<void(std::string_view)> sink(log);
    sink("a");
    sink("b");
    CHECK(log.lines == std::vector<std::string>{"a", "b"});  // refers, does not copy
    user::account acc{"ann", 10};
    compat::function_ref<long(long)> dep(compat::nontype<&user::account::deposit>, acc);
    CHECK(dep(5) == 15);
    compat::function_ref<bool(const user::account&, long) const noexcept> pay(
        compat::nontype<&user::account::can_pay>);
    CHECK(pay(acc, 15));
    CHECK_FALSE(pay(acc, 16));
}

TEST_CASE("move_only_function over user move-only callables") {
    compat::move_only_function<std::string() &&> task(user::unique_task("backup"));
    CHECK(std::move(task)() == "ran backup");
    std::vector<compat::move_only_function<int(int)>> pipeline;
    pipeline.emplace_back([p = std::make_unique<int>(3)](int x) { return x * *p; });
    pipeline.emplace_back([](int x) { return x + 1; });
    int v = 2;
    for (auto& stage : pipeline) v = stage(v);
    CHECK(v == 7);
    compat::move_only_function<void(std::string_view)> owning(user::audit_log{});
    owning("x");  // the log lives inside the wrapper
    CHECK(owning != nullptr);
}

TEST_CASE("bind and invoke on user members") {
    user::account acc{"bob", 0};
    auto add10 = compat::bind_back(&user::account::deposit, 10);
    add10(acc);
    CHECK(acc.balance == 10);
    auto into_acc = compat::bind_front(&user::account::deposit, std::ref(acc));
    into_acc(5);
    CHECK(acc.balance == 15);
    CHECK(compat::invoke(&user::account::owner, acc) == "bob");
    CHECK(compat::invoke(&user::account::can_pay, &acc, 15L));
    static_assert(compat::is_nothrow_invocable<decltype(&user::account::can_pay), user::account&, long>::value);
    static_assert(std::is_same_v<compat::invoke_result_t<decltype(&user::account::owner), user::account&>,
                                 std::string&>);
    std::vector<user::account> accounts{{"a", 1}, {"b", 2}};
    long total = 0;
    for (auto& a : accounts) total += compat::mem_fn(&user::account::balance)(a);
    CHECK(total == 3);
}

TEST_CASE("to_underlying on a user enum") {
    static_assert(std::is_same_v<decltype(compat::to_underlying(user::level::error)), std::uint8_t>);
    CHECK(compat::to_underlying(user::level::error) == 9);
}

TEST_CASE("shared_lock over a user SharedMutex") {
    user::tracing_mutex m;
    {
        compat::shared_lock<user::tracing_mutex> l(m);
        CHECK(l.owns_lock());
    }
    {
        compat::shared_lock<user::tracing_mutex> l(m, std::try_to_lock);
        l.unlock();
        l.lock();
    }
    CHECK(m.log == std::vector<std::string>{"S+", "S-", "S?", "S-", "S+", "S-"});
}
