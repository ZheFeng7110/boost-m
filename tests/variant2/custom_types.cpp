// mcpplibs.mwrap.boost.variant2 — user alternatives: types whose construction
// throws (variant2's never-valueless guarantee, with double storage when an
// alternative is not nothrow move constructible), non-movable types built in
// place, user visitors (overload sets, visitors with state, multi-variant
// visits), user ordering/streaming/hashing used by the variant's own
// operators, and recursive data through an indirection.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.variant2;
import mcpplibs.mwrap.boost.container_hash;  // the definition of boost::hash

namespace v2 = boost::variant2;

namespace user {
struct thrower {
    static inline bool fail = false;
    int v = 0;
    thrower() = default;
    explicit thrower(int x) : v(x) {
        if (fail) throw std::runtime_error("ctor");
    }
    thrower(const thrower& o) : v(o.v) {
        if (fail) throw std::runtime_error("copy");
    }
    thrower(thrower&& o) : v(o.v) {  // not noexcept
        if (fail) throw std::runtime_error("move");
    }
    thrower& operator=(const thrower&) = default;
};

struct pinned {
    explicit pinned(int x) : v(x) {}
    pinned(const pinned&) = delete;
    pinned& operator=(const pinned&) = delete;
    int v;
};

struct money {
    long cents;
    auto operator<=>(const money&) const = default;
    friend std::ostream& operator<<(std::ostream& os, const money& m) {
        return os << m.cents / 100 << '.' << std::setw(2) << std::setfill('0') << m.cents % 100;
    }
    friend std::size_t hash_value(const money& m) { return std::hash<long>()(m.cents); }
};

struct counter {  // a stateful visitor, passed by std::ref
    int calls = 0;
    void operator()(const auto&) { ++calls; }
};

template <class... Fs>
struct overloaded : Fs... {
    using Fs::operator()...;
};
template <class... Fs>
overloaded(Fs...) -> overloaded<Fs...>;

// A JSON-like recursive value through std::vector / std::map indirection.
struct node;
using array = std::vector<node>;
struct node {
    v2::variant<std::nullptr_t, bool, double, std::string, array> value;
};
std::size_t count_leaves(const node& n) {
    return v2::visit(overloaded{[](const array& a) {
                                    std::size_t s = 0;
                                    for (auto& x : a) s += count_leaves(x);
                                    return s;
                                },
                                [](const auto&) -> std::size_t { return 1; }},
                     n.value);
}
}  // namespace user

template <>
struct std::hash<user::money> {
    std::size_t operator()(const user::money& m) const noexcept { return std::hash<long>()(m.cents); }
};

TEST_CASE("never valueless: a throwing alternative keeps the old value") {
    using V = v2::variant<int, user::thrower>;
    V v = 5;
    user::thrower::fail = true;
    CHECK_THROWS_AS(v.emplace<user::thrower>(1), std::runtime_error);
    user::thrower::fail = false;
    // variant2 never becomes valueless: v still holds a valid alternative.
    CHECK(!v.valueless_by_exception());
    CHECK((v2::holds_alternative<int>(v) || v2::holds_alternative<user::thrower>(v)));
    v = user::thrower(3);
    CHECK(v2::get<user::thrower>(v).v == 3);
    V copy = v;
    CHECK(v2::get<1>(copy).v == 3);
}

TEST_CASE("non-movable alternatives are built in place") {
    v2::variant<v2::monostate, user::pinned> v;
    v.emplace<user::pinned>(42);
    CHECK(v2::get<user::pinned>(v).v == 42);
    v2::variant<user::pinned, int> w(v2::in_place_index<0>, 7);
    CHECK(v2::get<0>(w).v == 7);
}

TEST_CASE("user visitors") {
    using V = v2::variant<int, std::string, user::money>;
    std::vector<V> items{1, std::string("two"), user::money{350}};
    std::string log;
    for (auto& x : items)
        v2::visit(user::overloaded{[&](int i) { log += "i" + std::to_string(i); },
                                   [&](const std::string& s) { log += "s" + s; },
                                   [&](const user::money& m) { log += "m" + std::to_string(m.cents); }},
                  x);
    CHECK(log == "i1stwom350");
    user::counter c;
    for (auto& x : items) v2::visit(std::ref(c), x);
    CHECK(c.calls == 3);
    auto same_kind = [](const auto& a, const auto& b) {
        return std::is_same_v<std::decay_t<decltype(a)>, std::decay_t<decltype(b)>>;
    };
    CHECK(v2::visit(same_kind, items[2], V(user::money{1})));
    CHECK_FALSE(v2::visit(same_kind, items[0], items[1]));
}

TEST_CASE("user ordering, streaming and hashing through the variant's operators") {
    using V = v2::variant<user::money, std::string>;
    V a = user::money{199}, b = user::money{250};
    CHECK(a < b);
    CHECK(a == V(user::money{199}));
    std::ostringstream os;
    os << a;
    CHECK(os.str() == "1.99");
    CHECK(std::hash<V>()(a) == std::hash<V>()(V(user::money{199})));
    CHECK(boost::hash<V>()(a) == boost::hash<V>()(V(user::money{199})));
    std::set<V> sorted{V(std::string("x")), b, a};
    CHECK(v2::get<0>(*sorted.begin()).cents == 199);
}

TEST_CASE("recursive user data") {
    using user::node;
    node doc{user::array{node{1.0}, node{std::string("s")},
                         node{user::array{node{true}, node{nullptr}}}}};
    CHECK(user::count_leaves(doc) == 4);
}
