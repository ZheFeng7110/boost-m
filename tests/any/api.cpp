// mcpplibs.mwrap.boost.any — boost::any, boost::anys::basic_any and
// boost::anys::unique_any: construction, assignment, swap, the any_cast
// family (value, reference, pointer, rvalue), unsafe_any_cast, type queries.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.any;

TEST_CASE("any: construct, assign, query") {
    boost::any a;
    CHECK(a.empty());
    CHECK(a.type() == typeid(void));
    a = 3;
    CHECK_FALSE(a.empty());
    CHECK(a.type() == typeid(int));
    a = std::string("x");
    CHECK(a.type() == typeid(std::string));
    a.clear();
    CHECK(a.empty());
    boost::any b(std::vector<int>{1, 2});
    boost::any c(std::move(b));
    CHECK(b.empty());
    CHECK(boost::any_cast<std::vector<int>&>(c).size() == 2);
}

TEST_CASE("any: any_cast forms") {
    boost::any a = std::string("abc");
    CHECK(boost::any_cast<std::string>(a) == "abc");
    CHECK(boost::any_cast<const std::string&>(a) == "abc");
    boost::any_cast<std::string&>(a) += "d";
    CHECK(*boost::any_cast<std::string>(&a) == "abcd");
    const boost::any& ca = a;
    CHECK(boost::any_cast<std::string>(&ca)->size() == 4);
    CHECK(boost::any_cast<int>(&a) == nullptr);
    CHECK_THROWS_AS(boost::any_cast<int>(a), boost::bad_any_cast);
    CHECK_THROWS_WITH_AS(boost::any_cast<double>(a), doctest::Contains("any_cast"),
                         boost::bad_any_cast);
    std::string moved = boost::any_cast<std::string>(std::move(a));
    CHECK(moved == "abcd");
    boost::any n = 5;
    CHECK(*boost::unsafe_any_cast<int>(&n) == 5);
}

TEST_CASE("any: swap") {
    boost::any a = 1, b = std::string("two");
    boost::swap(a, b);
    CHECK(boost::any_cast<std::string>(a) == "two");
    CHECK(boost::any_cast<int>(b) == 1);
    a.swap(b);
    CHECK(boost::any_cast<int>(a) == 1);
}

TEST_CASE("basic_any: small buffer and large values") {
    using small_any = boost::anys::basic_any<16, 8>;
    small_any s = 7;
    CHECK(boost::anys::any_cast<int>(s) == 7);
    small_any big = std::array<char, 256>{};
    CHECK(boost::anys::any_cast<std::array<char, 256>&>(big).size() == 256);
    small_any copy = big;
    CHECK(copy.type() == typeid(std::array<char, 256>));
    small_any moved = std::move(s);
    CHECK(boost::anys::any_cast<int>(moved) == 7);
    boost::anys::swap(moved, copy);
    CHECK(moved.type() == typeid(std::array<char, 256>));
    CHECK(boost::anys::any_cast<int>(&copy) != nullptr);
    CHECK(*boost::anys::unsafe_any_cast<int>(&copy) == 7);
    CHECK_THROWS_AS(boost::anys::any_cast<double>(copy), boost::bad_any_cast);
    copy.clear();
    CHECK(copy.empty());
}

TEST_CASE("unique_any: in_place, emplace, reset, from boost::any") {
    boost::anys::unique_any u(boost::anys::in_place_type<std::vector<int>>, 3, 9);
    CHECK(boost::anys::any_cast<std::vector<int>&>(u) == std::vector<int>{9, 9, 9});
    auto& s = u.emplace<std::string>("hi");
    s += "!";
    CHECK(boost::anys::any_cast<const std::string&>(u) == "hi!");
    CHECK(u.type() == typeid(std::string));
    u.emplace<std::vector<int>>({1, 2, 3});
    CHECK(boost::anys::any_cast<std::vector<int>&>(u).size() == 3);
    u.reset();
    CHECK_FALSE(u.has_value());
    boost::anys::unique_any from_any(boost::any(5));
    CHECK(boost::anys::any_cast<int>(from_any) == 5);
    boost::anys::unique_any other = std::move(from_any);
    CHECK(boost::anys::any_cast<int>(other) == 5);
    boost::anys::unique_any w;
    w.swap(other);
    CHECK(w.has_value());
    CHECK_FALSE(other.has_value());
}
