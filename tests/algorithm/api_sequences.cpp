// mcpplibs.mwrap.boost.algorithm — the sequence algorithms (cxx11/14/17,
// searching, hex, gather, clamp, ordering, permutation) through the module.
// Cases named "doc: <section>" carry the Examples of libs/algorithm/doc/<x>.qbk.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.algorithm;

namespace ba = boost::algorithm;

namespace {
bool isOdd(int i) { return i % 2 == 1; }
bool lessThan10(int i) { return i < 10; }
const std::vector<int> c{0, 1, 2, 3, 14, 15};
}  // namespace

TEST_CASE("doc: all_of") {
    CHECK_FALSE(ba::all_of(c, isOdd));
    CHECK_FALSE(ba::all_of(c.begin(), c.end(), lessThan10));
    CHECK(ba::all_of(c.begin(), c.begin() + 3, lessThan10));
    CHECK(ba::all_of(c.end(), c.end(), isOdd));
    CHECK_FALSE(ba::all_of_equal(c, 3));
    CHECK(ba::all_of_equal(c.begin() + 3, c.begin() + 4, 3));
    CHECK(ba::all_of_equal(c.begin(), c.begin(), 99));
}

TEST_CASE("doc: any_of") {
    CHECK(ba::any_of(c, isOdd));
    CHECK(ba::any_of(c.begin(), c.end(), lessThan10));
    CHECK_FALSE(ba::any_of(c.begin() + 4, c.end(), lessThan10));
    CHECK_FALSE(ba::any_of(c.end(), c.end(), isOdd));
    CHECK(ba::any_of_equal(c, 3));
    CHECK_FALSE(ba::any_of_equal(c.begin(), c.begin() + 3, 3));
    CHECK_FALSE(ba::any_of_equal(c.begin(), c.begin(), 99));
}

TEST_CASE("doc: none_of") {
    CHECK_FALSE(ba::none_of(c, isOdd));
    CHECK_FALSE(ba::none_of(c.begin(), c.end(), lessThan10));
    CHECK(ba::none_of(c.begin() + 4, c.end(), lessThan10));
    CHECK(ba::none_of(c.end(), c.end(), isOdd));
    CHECK_FALSE(ba::none_of_equal(c, 3));
    CHECK(ba::none_of_equal(c.begin(), c.begin() + 3, 3));
    CHECK(ba::none_of_equal(c.begin(), c.begin(), 99));
}

TEST_CASE("doc: one_of") {
    CHECK_FALSE(ba::one_of(c, isOdd));
    CHECK_FALSE(ba::one_of(c.begin(), c.end(), lessThan10));
    CHECK(ba::one_of(c.begin() + 3, c.end(), lessThan10));
    CHECK_FALSE(ba::one_of(c.end(), c.end(), isOdd));
    CHECK(ba::one_of_equal(c, 3));
    CHECK_FALSE(ba::one_of_equal(c.begin(), c.begin() + 3, 3));
    CHECK_FALSE(ba::one_of_equal(c.begin(), c.begin(), 99));
}

TEST_CASE("doc: apply_permutation") {
    std::vector<char> v{'a', 'b', 'c', 'd'};
    std::vector<int> order{2, 3, 1, 0};
    ba::apply_permutation(v, order);
    CHECK(v == std::vector<char>{'c', 'd', 'b', 'a'});
    std::vector<char> w{'a', 'b', 'c', 'd'};
    std::vector<int> order2{2, 3, 1, 0};
    ba::apply_reverse_permutation(w.begin(), w.end(), order2.begin(), order2.end());
    CHECK(w == std::vector<char>{'d', 'c', 'a', 'b'});
}

TEST_CASE("doc: clamp") {
    CHECK(ba::clamp(1, 1, 10) == 1);
    CHECK(ba::clamp(0, 1, 10) == 1);
    CHECK(ba::clamp(11, 1, 10) == 10);
    CHECK(ba::clamp(3, 10, 1, std::greater<int>()) == 3);
    std::vector<int> in{-5, 0, 5, 50};
    std::vector<int> out;
    ba::clamp_range(in, std::back_inserter(out), 0, 10);
    CHECK(out == std::vector<int>{0, 0, 5, 10});
    CHECK(ba::is_clamped(5, 0, 10));
    CHECK_FALSE(ba::is_clamped(11, 0, 10));
}

TEST_CASE("doc: equal") {
    std::vector<int> a{0, 1, 2};
    std::vector<int> b{0, 1, 2, 3};
    CHECK_FALSE(ba::equal(a.begin(), a.end(), b.begin(), b.end()));
    CHECK(ba::equal(a.begin(), a.end(), b.begin(), b.begin() + 3));
    CHECK(ba::equal(a.begin(), a.begin(), b.begin(), b.begin()));
    CHECK(ba::equal(a.begin(), a.end(), b.begin(), b.begin() + 3, std::equal_to<int>()));
}

TEST_CASE("doc: mismatch") {
    std::vector<int> c1{0, 1, 2};
    std::vector<int> c2{0, 1, 2, 3};
    auto r = ba::mismatch(c1.begin(), c1.end(), c2.begin(), c2.end());
    CHECK(r.first == c1.end());
    CHECK(r.second == c2.begin() + 3);
    auto r2 = ba::mismatch(c1.begin(), c1.end(), c2.begin(), c2.begin());
    CHECK(r2.first == c1.begin());
    CHECK(r2.second == c2.begin());
}

TEST_CASE("doc: find_backward") {
    std::vector<int> v{1, 2, 3, 2, 1};
    CHECK(ba::find_backward(v.begin(), v.end(), 2) - v.begin() == 3);
    CHECK(ba::find_backward(v, 9) == v.end());
    CHECK(ba::find_not_backward(v, 1) - v.begin() == 3);
    CHECK(ba::find_if_backward(v, [](int i) { return i > 2; }) - v.begin() == 2);
    CHECK(ba::find_if_not_backward(v, [](int i) { return i < 3; }) - v.begin() == 2);
}

TEST_CASE("doc: find_not") {
    std::vector<int> vec{1, 1, 2};
    CHECK(ba::find_not(vec.begin(), vec.end(), 1) == vec.begin() + 2);
    std::vector<int> c1{1, 0, 0};
    CHECK(ba::find_not(c1.begin(), c1.end(), 1) == std::next(c1.begin()));
    CHECK(ba::find_not(c1.begin(), c1.end(), 0) == c1.begin());
    CHECK(ba::find_not(c1, 2) == c1.begin());
    CHECK(ba::find_if_not(c1, [](int i) { return i == 1; }) == std::next(c1.begin()));
}

TEST_CASE("doc: gather") {
    // gather moves the selected elements next to the pivot, stably.
    std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8, 9};
    auto pivot = v.begin() + 4;
    auto r = ba::gather(v.begin(), v.end(), pivot, [](int i) { return i % 2 == 0; });
    CHECK(r.second - r.first == 4);
    CHECK(std::vector<int>(r.first, r.second) == std::vector<int>{2, 4, 6, 8});
    CHECK(std::is_permutation(v.begin(), v.end(),
                              std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}.begin()));
}

TEST_CASE("doc: hex") {
    std::string out;
    ba::hex("abcdef", std::back_inserter(out));
    CHECK(out == "616263646566");
    CHECK(ba::hex(std::string("32")) == "3332");
    CHECK(ba::hex_lower(std::string("\x7f\xfe")) == "7ffe");
    CHECK(ba::unhex(std::string("616263646566")) == "abcdef");
    CHECK(ba::unhex(std::string("3332")) == "32");
    std::u16string wout;
    ba::unhex(std::string("3332"), std::back_inserter(wout));
    REQUIRE(wout.size() == 1);
    CHECK(wout[0] == 0x3332);
    CHECK_THROWS_AS(ba::unhex(std::string("3")), ba::not_enough_input);
    CHECK_THROWS_AS(ba::unhex(std::string("ACEG")), ba::non_hex_input);
    CHECK_THROWS_AS(ba::unhex(std::string("ACEG")), ba::hex_decode_error);
}

TEST_CASE("doc: is_palindrome") {
    CHECK(ba::is_palindrome(std::vector<int>{1, 2, 1}));
    CHECK_FALSE(ba::is_palindrome(std::vector<int>{1, 2, 3}));
    CHECK(ba::is_palindrome(std::vector<int>{}));
    CHECK(ba::is_palindrome("a"));
    CHECK(ba::is_palindrome(std::string("AbA"), [](char a, char b) {
        return std::tolower(a) == std::tolower(b);
    }));
}

TEST_CASE("doc: is_partitioned") {
    std::vector<int> v{0, 1, 2, 3, 14, 15};
    CHECK_FALSE(ba::is_partitioned(v, isOdd));
    CHECK(ba::is_partitioned(v, lessThan10));
    CHECK(ba::is_partitioned(v.begin(), v.end(), lessThan10));
    CHECK(ba::is_partitioned(v.begin(), v.begin() + 3, lessThan10));
    CHECK(ba::is_partitioned(v.end(), v.end(), isOdd));
}

TEST_CASE("doc: is_partitioned_until") {
    std::vector<int> v{0, 1, 2, 3, 14, 15};
    CHECK(ba::is_partitioned_until(v, isOdd) == v.begin() + 1);
    CHECK(ba::is_partitioned_until(v, lessThan10) == v.end());
    CHECK(ba::is_partitioned_until(v.end(), v.end(), isOdd) == v.end());
}

TEST_CASE("doc: is_permutation") {
    std::vector<int> c1{0, 1, 2, 3, 14, 15};
    std::vector<int> c2{15, 14, 3, 1, 2};
    CHECK_FALSE(ba::is_permutation(c1.begin(), c1.end(), c2.begin(), c2.end()));
    CHECK(ba::is_permutation(c1.begin() + 1, c1.end(), c2.begin(), c2.end()));
    CHECK(ba::is_permutation(c1.end(), c1.end(), c2.end(), c2.end()));
    CHECK(ba::is_permutation(c2, c1.begin() + 1));
}

TEST_CASE("doc: is_sorted") {
    std::vector<int> v{1, 2, 2, 3};
    CHECK(ba::is_sorted(v));
    CHECK(ba::is_sorted_until(v.begin(), v.end()) == v.end());
    CHECK(ba::is_increasing(v));
    CHECK_FALSE(ba::is_strictly_increasing(v));
    CHECK_FALSE(ba::is_decreasing(v));
    std::vector<int> d{3, 2, 1};
    CHECK(ba::is_strictly_decreasing(d));
    CHECK(ba::is_sorted(d, std::greater<int>()));
    std::vector<int> u{1, 3, 2};
    CHECK(ba::is_sorted_until(u) == u.begin() + 2);
}

TEST_CASE("doc: partition_point") {
    std::vector<int> v{1, 3, 5, 2, 4};
    CHECK(ba::partition_point(v, isOdd) == v.begin() + 3);
    CHECK(ba::partition_point(v.begin(), v.end(), isOdd) == v.begin() + 3);
}

TEST_CASE("doc: BoyerMoore") {
    const std::string corpus("abracadabra");
    const std::string pattern("cad");
    ba::boyer_moore<std::string::const_iterator> bm(pattern.begin(), pattern.end());
    auto r = bm(corpus.begin(), corpus.end());
    CHECK(r.first - corpus.begin() == 4);
    CHECK(r.second - corpus.begin() == 7);
    auto r2 = ba::boyer_moore_search(corpus, pattern);
    CHECK(r2.first - corpus.begin() == 4);
    auto searcher = ba::make_boyer_moore(pattern);
    CHECK(searcher(corpus).first - corpus.begin() == 4);
}

TEST_CASE("doc: BoyerMooreHorspool") {
    const std::string corpus("abracadabra");
    const std::string pattern("abra");
    ba::boyer_moore_horspool<std::string::const_iterator> bmh(pattern.begin(), pattern.end());
    CHECK(bmh(corpus.begin(), corpus.end()).first == corpus.begin());
    auto r = ba::boyer_moore_horspool_search(corpus.begin() + 1, corpus.end(),
                                             pattern.begin(), pattern.end());
    CHECK(r.first - corpus.begin() == 7);
    CHECK(ba::make_boyer_moore_horspool(pattern)(corpus).first == corpus.begin());
}

TEST_CASE("doc: KnuthMorrisPratt") {
    const std::string corpus("abracadabra");
    const std::string pattern("dab");
    ba::knuth_morris_pratt<std::string::const_iterator> kmp(pattern.begin(), pattern.end());
    CHECK(kmp(corpus.begin(), corpus.end()).first - corpus.begin() == 6);
    const std::string missing("xyz");
    auto r = ba::knuth_morris_pratt_search(corpus, missing);
    CHECK(r.first == corpus.end());
    CHECK(r.second == corpus.end());
    CHECK(ba::make_knuth_morris_pratt(pattern)(corpus).first - corpus.begin() == 6);
}

TEST_CASE("cxx11: copy_if / copy_while / copy_until / copy_n / iota / partition_copy") {
    std::vector<int> v{1, 2, 3, 4, 5};
    std::vector<int> out;
    ba::copy_if(v, std::back_inserter(out), isOdd);
    CHECK(out == std::vector<int>{1, 3, 5});
    out.clear();
    ba::copy_while(v, std::back_inserter(out), [](int i) { return i < 3; });
    CHECK(out == std::vector<int>{1, 2});
    out.clear();
    ba::copy_until(v, std::back_inserter(out), [](int i) { return i == 4; });
    CHECK(out == std::vector<int>{1, 2, 3});
    out.clear();
    ba::copy_n(v.begin(), 2, std::back_inserter(out));
    CHECK(out == std::vector<int>{1, 2});
    std::vector<int> seq(4);
    ba::iota(seq, 10);
    CHECK(seq == std::vector<int>{10, 11, 12, 13});
    std::vector<int> odd, even;
    ba::partition_copy(v, std::back_inserter(odd), std::back_inserter(even), isOdd);
    CHECK(odd == std::vector<int>{1, 3, 5});
    CHECK(even == std::vector<int>{2, 4});
}

TEST_CASE("cxx17: reduce / scans / for_each_n / transform_reduce") {
    std::vector<int> v{1, 2, 3, 4};
    CHECK(ba::reduce(v.begin(), v.end()) == 10);
    CHECK(ba::reduce(v, 5) == 15);
    CHECK(ba::reduce(v.begin(), v.end(), 1, std::multiplies<int>()) == 24);
    std::vector<int> out;
    ba::inclusive_scan(v.begin(), v.end(), std::back_inserter(out));
    CHECK(out == std::vector<int>{1, 3, 6, 10});
    out.clear();
    ba::exclusive_scan(v.begin(), v.end(), std::back_inserter(out), 0);
    CHECK(out == std::vector<int>{0, 1, 3, 6});
    out.clear();
    ba::transform_inclusive_scan(v.begin(), v.end(), std::back_inserter(out),
                                 std::plus<int>(), [](int i) { return i * i; });
    CHECK(out == std::vector<int>{1, 5, 14, 30});
    out.clear();
    ba::transform_exclusive_scan(v.begin(), v.end(), std::back_inserter(out), 0,
                                 std::plus<int>(), [](int i) { return i * 2; });
    CHECK(out == std::vector<int>{0, 2, 6, 12});
    CHECK(ba::transform_reduce(v.begin(), v.end(), v.begin(), 0) == 30);
    int sum = 0;
    ba::for_each_n(v.begin(), 3, [&](int i) { sum += i; });
    CHECK(sum == 6);
}

TEST_CASE("sort_subrange / partition_subrange") {
    std::vector<int> v{9, 1, 8, 2, 7, 3, 6, 4, 5};
    ba::sort_subrange(v.begin(), v.end(), v.begin() + 2, v.begin() + 5);
    CHECK(std::vector<int>(v.begin() + 2, v.begin() + 5) == std::vector<int>{3, 4, 5});
    std::vector<int> w{9, 1, 8, 2, 7, 3, 6, 4, 5};
    ba::partition_subrange(w.begin(), w.end(), w.begin() + 2, w.begin() + 5);
    std::vector<int> mid(w.begin() + 2, w.begin() + 5);
    std::sort(mid.begin(), mid.end());
    CHECK(mid == std::vector<int>{3, 4, 5});
}
