// mcpplibs.mwrap.boost.dynamic_bitset — the documented members and free
// functions: construction from integers/strings/blocks, element access, the
// bitwise and shift operators, set algebra (subset, intersects), searches
// (find_first/find_next), resizing, block conversion, comparison, hashing and
// string conversion.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.dynamic_bitset;

using bits = boost::dynamic_bitset<>;

TEST_CASE("construction") {
    CHECK(bits().empty());
    CHECK(bits(8, 0xF0ul).count() == 4);
    CHECK(bits(std::string("1011")).to_ulong() == 11ul);
    CHECK(bits(std::string("xx1011"), 2).to_ulong() == 11ul);
    CHECK(bits(std::string("101100"), 0, 3).to_ulong() == 5ul);
    std::vector<unsigned long> blocks{0x1ul, 0x2ul};
    bits fromBlocks(blocks.begin(), blocks.end());
    CHECK(fromBlocks.size() == 2 * bits::bits_per_block);
    CHECK(fromBlocks.test(0));
    CHECK(fromBlocks.test(bits::bits_per_block + 1));
    bits copy(fromBlocks), moved(std::move(copy));
    CHECK(moved == fromBlocks);
}

TEST_CASE("element access and modifiers") {
    bits b(10);
    b.set(2).set(4, true).set(6, 2, true);  // bits 6 and 7
    CHECK(b.count() == 4);
    CHECK(b.test(7));
    CHECK_FALSE(b.test_set(8));  // returns the previous value
    CHECK(b.test_set(8));
    b.reset(2);
    b.flip(0);
    CHECK(b[0]);
    b[0].flip();
    CHECK_FALSE(b[0]);
    b.reset(6, 2);
    CHECK_FALSE(b.test(6));
    b.flip(0, 3);
    CHECK(b.test(0));
    CHECK(b.all() == false);
    CHECK(bits(3, 7ul).all());
    CHECK_THROWS_AS(b.at(100), std::out_of_range);
    CHECK(b.at(4));
    b.set();
    CHECK(b.all());
    b.reset();
    CHECK(b.none());
    b.flip();
    CHECK(b.count() == 10);
}

TEST_CASE("bitwise operators and shifts") {
    bits a(std::string("1100")), b(std::string("1010"));
    CHECK((a & b) == bits(std::string("1000")));
    CHECK((a | b) == bits(std::string("1110")));
    CHECK((a ^ b) == bits(std::string("0110")));
    CHECK((a - b) == bits(std::string("0100")));
    CHECK(~a == bits(std::string("0011")));
    bits c = a;
    c <<= 1;
    CHECK(c == bits(std::string("1000")));
    c >>= 2;
    CHECK(c == bits(std::string("0010")));
    c |= b;
    c &= a;
    c ^= b;
    c -= a;
    CHECK(c == bits(std::string("0010")));
}

TEST_CASE("set algebra and searches") {
    bits a(std::string("0110")), b(std::string("1110"));
    CHECK(a.is_subset_of(b));
    CHECK(a.is_proper_subset_of(b));
    CHECK(a.intersects(b));
    CHECK_FALSE(bits(std::string("0001")).intersects(a));
    bits s(16);
    s.set(3).set(9).set(12);
    CHECK(s.find_first() == 3);
    CHECK(s.find_next(3) == 9);
    CHECK(s.find_next(12) == bits::npos);
    CHECK(s.find_first_off() == 0);
    CHECK(s.find_next_off(2) == 4);
    CHECK(s.find_first(10) == 12);
}

TEST_CASE("size, blocks, conversions") {
    bits b(5, 0x1Ful);
    b.resize(8, true);
    CHECK(b.count() == 8);
    b.push_back(false);
    b.push_front(true);
    CHECK(b.size() == 10);
    CHECK(b.test(0));
    b.pop_back();
    b.pop_front();
    CHECK(b.size() == 8);
    b.append(0x3ul);  // a whole block
    CHECK(b.size() == 8 + bits::bits_per_block);
    CHECK(b.num_blocks() == 2);
    std::vector<unsigned long> out;
    boost::to_block_range(b, std::back_inserter(out));
    CHECK(out.size() == 2);
    CHECK(out[1] == 0ul);
    bits roundtrip(b.size());
    boost::from_block_range(out.begin(), out.end(), roundtrip);
    CHECK(roundtrip == b);
    b.clear();
    CHECK(b.empty());
    b.shrink_to_fit();
    CHECK(bits(4, 9ul).to_ulong() == 9ul);
    std::string s;
    boost::to_string(bits(4, 9ul), s);
    CHECK(s == "1001");
    CHECK(bits(4).max_size() >= 4);
}

TEST_CASE("iterators (1.92) and capacity") {
    bits b(std::string("1011"));
    std::vector<bool> seen(b.begin(), b.end());  // least significant bit first
    CHECK(seen == std::vector<bool>{true, true, false, true});
    CHECK(std::count(b.cbegin(), b.cend(), true) == 3);
    CHECK(*b.rbegin() == true);
    for (auto ref : b) ref = !ref;
    CHECK(b == bits(std::string("0100")));
    b.reserve(1000);
    CHECK(b.capacity() >= 1000);
    CHECK(b.size() == 4);
}

TEST_CASE("comparison, swap, hash") {
    bits a(std::string("0101")), b(std::string("0110"));
    CHECK(a < b);
    CHECK(a <= b);
    CHECK(b > a);
    CHECK(a != b);
    swap(a, b);
    CHECK(a == bits(std::string("0110")));
    // hash_value is a hidden friend (found by ADL); std::hash is specialized.
    CHECK(hash_value(a) == hash_value(bits(std::string("0110"))));
    CHECK(std::hash<bits>()(a) == std::hash<bits>()(bits(std::string("0110"))));
    std::unordered_set<bits> set{a, b};
    CHECK(set.size() == 2);
}
