// mcpplibs.mwrap.boost.variant2 — the reference surface: constructors
// (in_place_type / in_place_index), emplace, swap, relational operators,
// get / get_if / unsafe_get / holds_alternative, visit, visit_by_index,
// variant_size / variant_alternative, subset<>(), operator<< and hashing.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.variant2;
import mcpplibs.mwrap.boost.container_hash;  // the definition of boost::hash

namespace v2 = boost::variant2;
using V = v2::variant<int, std::string, double>;

TEST_CASE("construction and emplace") {
    V a;
    CHECK(a.index() == 0);
    V b(v2::in_place_type<std::string>, 3, 'x');
    CHECK(v2::get<std::string>(b) == "xxx");
    V c(v2::in_place_index<2>, 1.5);
    CHECK(v2::get<2>(c) == 1.5);
    auto& s = c.emplace<std::string>("abc");
    CHECK(s == "abc");
    c.emplace<0>(5);
    CHECK(v2::get<int>(c) == 5);
    V d(v2::in_place_type<std::string>, {'a', 'b'});
    CHECK(v2::get<1>(d) == "ab");
    static_assert(v2::variant_size_v<V> == 3);
    static_assert(std::is_same_v<v2::variant_alternative_t<1, V>, std::string>);
    static_assert(v2::variant_size<const V>::value == 3);
}

TEST_CASE("access") {
    V v = std::string("hi");
    CHECK(v2::holds_alternative<std::string>(v));
    CHECK(v2::get_if<int>(&v) == nullptr);
    CHECK(*v2::get_if<1>(&v) == "hi");
    CHECK(v2::unsafe_get<1>(v) == "hi");
    v2::get<std::string>(v) += "!";
    CHECK(v2::get<1>(std::as_const(v)) == "hi!");
    std::string moved = v2::get<1>(std::move(v));
    CHECK(moved == "hi!");
    CHECK_THROWS_WITH_AS(v2::get<int>(v), doctest::Contains("bad"), v2::bad_variant_access);
}

TEST_CASE("relational operators and swap") {
    V a = 1, b = 2, s = std::string("a");
    CHECK(a < b);
    CHECK(b < s);  // ordered by index first
    CHECK(a != b);
    CHECK(a <= a);
    CHECK(s >= b);
    CHECK(s > a);
    swap(a, s);
    CHECK(v2::holds_alternative<std::string>(a));
    a.swap(s);
    CHECK(v2::get<int>(a) == 1);
    CHECK(v2::variant<v2::monostate>() == v2::variant<v2::monostate>());
}

TEST_CASE("visit and visit_by_index") {
    V v = 2.5;
    auto r = v2::visit([](auto const& x) -> std::string {
        if constexpr (std::is_same_v<std::decay_t<decltype(x)>, std::string>) return x;
        else return std::to_string(int(x * 2));
    }, v);
    CHECK(r == "5");
    // visit<R> converts each result to R.
    long size = v2::visit<long>([](auto const& x) { return sizeof(x); }, v);
    CHECK(size == long(sizeof(double)));
    std::string which;
    v2::visit_by_index(v,
                       [&](int) { which = "int"; },
                       [&](std::string const&) { which = "string"; },
                       [&](double) { which = "double"; });
    CHECK(which == "double");
    v2::variant<int, char> a = 1, b = 'x';
    CHECK(v2::visit([](auto x, auto y) { return int(x) + int(y); }, a, b) == 1 + 'x');
}

TEST_CASE("subset, streaming, hashing") {
    V v = 7;
    auto sub = v.subset<int, double>();
    CHECK(v2::get<int>(sub) == 7);
    V s = std::string("x");
    auto narrow = [&] { return s.subset<int, double>(); };
    CHECK_THROWS_AS(narrow(), v2::bad_variant_access);
    std::ostringstream os;
    os << v << ' ' << s;
    CHECK(os.str() == "7 x");
    CHECK(std::hash<V>()(v) == std::hash<V>()(V(7)));
    CHECK(boost::hash<V>()(v) == boost::hash<V>()(V(7)));
    CHECK(std::hash<V>()(v) != std::hash<V>()(V(8)));
    std::unordered_set<V> set{V(1), V(std::string("a")), V(1.0)};
    CHECK(set.size() == 3);
}
