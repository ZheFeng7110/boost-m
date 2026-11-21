// mcpplibs.mwrap.boost.dynamic_bitset — libs/dynamic_bitset/example/
// example1..3.cpp as test cases (example3 reads std::cin; it reads a string
// stream here).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.dynamic_bitset;

namespace {
template <class B>
std::string str(const B& b) {
    std::ostringstream os;
    os << b;
    return os.str();
}
}  // namespace

TEST_CASE("example: example1") {
    boost::dynamic_bitset<> x(5);
    x[0] = 1;
    x[1] = 1;
    x[4] = 1;
    std::string by_index;
    for (boost::dynamic_bitset<>::size_type i = 0; i < x.size(); ++i) by_index += x[i] ? '1' : '0';
    CHECK(by_index == "11001");
    CHECK(str(x) == "10011");  // operator<< prints the most significant bit first
}

TEST_CASE("example: example2") {
    CHECK(str(boost::dynamic_bitset<>(2, 0ul)) == "00");
    CHECK(str(boost::dynamic_bitset<>(2, 1ul)) == "01");
    CHECK(str(boost::dynamic_bitset<>(2, 2ul)) == "10");
    CHECK(str(boost::dynamic_bitset<>(2, 3ul)) == "11");
}

TEST_CASE("example: example3") {
    boost::dynamic_bitset<> mask(12, 2730ul);
    CHECK(str(mask) == "101010101010");
    boost::dynamic_bitset<> x;
    CHECK(x.size() == 0);
    std::istringstream in("100110");
    REQUIRE(static_cast<bool>(in >> x));
    const std::size_t sz = x.size();
    CHECK(sz == 6);
    CHECK(x.to_ulong() == 38ul);
    mask.resize(sz);
    CHECK(str(mask) == "101010");
    CHECK(str(x & mask) == "100010");
    CHECK(str(x | mask) == "101110");
    CHECK(str(x << 1) == "001100");
    CHECK(str(x >> 1) == "010011");
    boost::dynamic_bitset<> huge(200);
    huge.set(150);
    CHECK_THROWS_AS(huge.to_ulong(), std::overflow_error);
}
