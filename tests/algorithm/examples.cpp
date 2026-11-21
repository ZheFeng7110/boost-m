// mcpplibs.mwrap.boost.algorithm — the official examples of Boost.Algorithm
// (libs/algorithm/example, minmax/example, string/example) as test cases. Each
// case keeps the example's data and calls and turns what it prints into
// checks. string/example/regex_example.cpp is include_string_regex.cpp: the
// *_regex family is not in the module (tools/export_policy.toml [algorithm]).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.algorithm;

namespace ba = boost::algorithm;

TEST_CASE("example: apply_permutation_example") {
    {
        std::vector<int> vec{1, 2, 3, 4, 5}, order{4, 2, 3, 1, 0};
        ba::apply_permutation(vec.begin(), vec.end(), order.begin(), order.end());
        CHECK(vec == std::vector<int>{5, 3, 4, 2, 1});
    }
    {
        std::vector<int> vec{1, 2, 3, 4, 5}, order{4, 2, 3, 1, 0};
        ba::apply_reverse_permutation(vec.begin(), vec.end(), order.begin(), order.end());
        CHECK(vec == std::vector<int>{5, 4, 2, 3, 1});
    }
    {
        std::vector<int> vec{1, 2, 3, 4, 5}, order{4, 2, 3, 1, 0};
        ba::apply_reverse_permutation(vec, order);
        CHECK(vec == std::vector<int>{5, 4, 2, 3, 1});
    }
    {
        std::vector<int> vec{1, 2, 3, 4, 5}, order{4, 2, 3, 1, 0};
        ba::apply_permutation(vec, order);
        CHECK(vec == std::vector<int>{5, 3, 4, 2, 1});
    }
}

namespace {
bool compare_string_lengths(const std::string& one, const std::string& two) {
    return one.length() < two.length();
}
}  // namespace

TEST_CASE("example: clamp_example") {
    CHECK(ba::clamp(5, 1, 10) == 5);
    CHECK(ba::clamp(15, 1, 10) == 10);
    CHECK(ba::clamp(-15, 1, 10) == 1);
    CHECK(ba::clamp(5.1, 1.0, 10.0) == doctest::Approx(5.1));
    std::string one("Lower Bound"), two("upper bound!"), test1("test#"), test2("#test");
    CHECK(ba::clamp(test1, one, two) == "test#");
    CHECK(ba::clamp(test2, one, two) == "Lower Bound");
    CHECK(ba::clamp(test1, one, two, compare_string_lengths) == "Lower Bound");
    CHECK(ba::clamp(test2, one, two, compare_string_lengths) == "Lower Bound");
    // "Somewhat unexpected": T is deduced from the first argument (int).
    CHECK(ba::clamp(12, 14.7, 15.9) == 14);
    CHECK(ba::clamp(static_cast<double>(12), 14.7, 15.9) == doctest::Approx(14.7));
}

namespace {
template <typename T>
bool funcComparator(const T& v1, const T& v2) { return v1 == v2; }

struct functorComparator {
    template <typename T>
    bool operator()(const T& v1, const T& v2) const { return v1 == v2; }
};
}  // namespace

TEST_CASE("example: is_palindrome_example") {
    std::vector<int> vec{1, 2, 1};
    CHECK(ba::is_palindrome(vec.begin(), vec.end()));
    CHECK(ba::is_palindrome(vec.cbegin(), vec.cend()));
    std::list<int> list{1, 2, 1};
    CHECK(ba::is_palindrome(list.begin(), list.end()));
    auto lambdaComparator = [](int v1, int v2) { return v1 == v2; };
    auto objFunc = std::function<bool(int, int)>(lambdaComparator);
    CHECK(ba::is_palindrome(vec.begin(), vec.end(), lambdaComparator));
    CHECK(ba::is_palindrome(vec.begin(), vec.end(), funcComparator<int>));
    CHECK(ba::is_palindrome(vec.begin(), vec.end(), functorComparator()));
    CHECK(ba::is_palindrome(vec.begin(), vec.end(), objFunc));
    CHECK(ba::is_palindrome(vec));
    CHECK(ba::is_palindrome("aba"));
    CHECK_FALSE(ba::is_palindrome("abc"));
}

namespace {
bool isOdd(const int v1) { return v1 % 2 != 0; }
struct isOddComp {
    bool operator()(const int v1) const { return v1 % 2 != 0; }
};
}  // namespace

TEST_CASE("example: is_partitioned_until_example") {
    std::vector<int> good{1, 2, 4};
    std::vector<int> bad{1, 2, 3};
    CHECK(ba::is_partitioned_until(good.begin(), good.end(), isOdd) == good.end());
    CHECK(ba::is_partitioned_until(good.begin(), good.end(), isOddComp()) == good.end());
    auto it3 = ba::is_partitioned_until(bad, isOdd);
    REQUIRE(it3 != bad.end());
    CHECK(*it3 == 3);
}

TEST_CASE("example: search_example") {
    const std::string haystack("ABACAB is it everywhere!");
    const std::string needle1("ACAB");
    const std::string needle2("not ABA");
    const auto none = std::make_pair(haystack.end(), haystack.end());

    auto r1 = ba::boyer_moore_search(haystack.begin(), haystack.end(),
                                     needle1.begin(), needle1.end());
    CHECK(r1 != none);
    CHECK(r1.first - haystack.begin() == 2);

    ba::boyer_moore<std::string::const_iterator> search1(needle1.begin(), needle1.end());
    CHECK(search1(haystack.begin(), haystack.end()) != none);

    CHECK(ba::boyer_moore_horspool_search(haystack.begin(), haystack.end(),
                                          needle1.begin(), needle1.end()) != none);
    CHECK(ba::knuth_morris_pratt_search(haystack.begin(), haystack.end(),
                                        needle1.begin(), needle1.end()) != none);
    CHECK(ba::boyer_moore_search(haystack.begin(), haystack.end(),
                                 needle2.begin(), needle2.end()) == none);
}

TEST_CASE("example: minmax_ex") {
    boost::tuple<int const&, int const&> result1 = boost::minmax(1, 0);
    CHECK(result1.get<0>() == 0);
    CHECK(result1.get<1>() == 1);

    std::list<int> L;
    std::minstd_rand gen(42);
    std::generate_n(std::front_inserter(L), 1000, [&] { return static_cast<int>(gen()); });
    using iterator = std::list<int>::const_iterator;
    std::pair<iterator, iterator> result2 = boost::minmax_element(L.cbegin(), L.cend());
    CHECK(result2.first == std::min_element(L.cbegin(), L.cend()));
    CHECK(result2.second == std::max_element(L.cbegin(), L.cend()));
}

TEST_CASE("example: minmax_timer") {
    // The timing harness of minmax_timer.cpp, without the clock: every
    // first/last min/max variant it times must agree with the std algorithms.
    std::vector<int> v{5, 1, 9, 1, 9, 3};
    auto first = v.begin(), last = v.end();
    CHECK(*boost::first_min_element(first, last) == 1);
    CHECK(boost::first_min_element(first, last) - first == 1);
    CHECK(boost::last_min_element(first, last) - first == 3);
    CHECK(boost::first_max_element(first, last) - first == 2);
    CHECK(boost::last_max_element(first, last) - first == 4);
    auto fl = boost::first_min_last_max_element(first, last);
    CHECK(fl.first - first == 1);
    CHECK(fl.second - first == 4);
    auto lf = boost::last_min_first_max_element(first, last);
    CHECK(lf.first - first == 3);
    CHECK(lf.second - first == 2);
    auto ll = boost::last_min_last_max_element(first, last);
    CHECK(ll.first - first == 3);
    CHECK(ll.second - first == 4);
    auto mm = boost::minmax_element(first, last, std::greater<int>());
    CHECK(*mm.first == 9);
    CHECK(*mm.second == 1);
}

TEST_CASE("example: conv_example") {
    std::string str1("AbCdEfG");
    std::vector<char> vec1(str1.begin(), str1.end());
    std::string lowered;
    boost::to_lower_copy(std::back_inserter(lowered), vec1);
    CHECK(lowered == "abcdefg");
    CHECK(boost::to_upper_copy(str1) == "ABCDEFG");
    boost::to_lower(str1);
    CHECK(str1 == "abcdefg");
}

TEST_CASE("example: find_example") {
    std::string str1("abc___cde___efg");
    std::string str2("abc");
    boost::iterator_range<std::string::iterator> range =
        boost::find_first(str1, std::string("cde"));
    boost::to_upper(range);
    CHECK(str1 == "abc___CDE___efg");
    boost::iterator_range<std::string::iterator> head = boost::find_head(str1, 3);
    CHECK(std::string(head.begin(), head.end()) == "abc");
    head = boost::find_tail(str2, 5);
    CHECK(std::string(head.begin(), head.end()) == "abc");
    char text[] = "hello dolly!";
    boost::iterator_range<char*> crange = boost::find_last(text, "ll");
    std::transform(crange.begin(), crange.end(), crange.begin(),
                   [](char c) { return static_cast<char>(c + 1); });
    boost::to_upper(crange);
    CHECK(std::string(text) == "hello doMMy!");
}

TEST_CASE("example: predicate_example") {
    std::string str1("123xxx321");
    std::string str2("abc");
    CHECK(boost::starts_with(str1, std::string("123")));
    CHECK_FALSE(boost::ends_with(str1, std::string("123")));
    CHECK(boost::contains(str1, std::string("xxx")));
    CHECK(boost::equals(str2, std::string("abc")));
    CHECK(boost::all(";.,", boost::is_punct()));
    CHECK(boost::all("abcxxx", boost::is_any_of("xabc") && !boost::is_space()));
}

namespace {
std::string upcase_formatter(const boost::iterator_range<std::string::const_iterator>& Replace) {
    std::string Temp(Replace.begin(), Replace.end());
    boost::to_upper(Temp);
    return Temp;
}
}  // namespace

TEST_CASE("example: replace_example") {
    std::string str1("abc___cde___efg");
    CHECK(boost::erase_range_copy(
              str1, boost::make_iterator_range(str1.begin() + 6, str1.begin() + 9)) ==
          "abc______efg");
    CHECK(boost::replace_range_copy(
              str1, boost::make_iterator_range(str1.begin() + 6, str1.begin() + 9), "+++") ==
          "abc___+++___efg");
    std::string out;
    boost::replace_first_copy(std::back_inserter(out), str1, "cde", "XYZ");
    CHECK(out == "abc___XYZ___efg");
    CHECK(boost::replace_all_copy(str1, "___", "---") == "abc---cde---efg");
    CHECK(boost::erase_all_copy(str1, "___") == "abccdeefg");
    boost::replace_nth(str1, "_", 4, "+");
    boost::replace_nth(str1, "_", 2, "+");
    CHECK(str1 == "abc__+cde_+_efg");
    std::string str2("abC-xxxx-AbC-xxxx-abc");
    CHECK(boost::find_format_all_copy(str2, boost::first_finder("abc", boost::is_iequal()),
                                      upcase_formatter) == "ABC-xxxx-ABC-xxxx-ABC");
}

namespace rle {
template <typename T>
T repeat_mark() { return (std::numeric_limits<T>::max)(); }

struct find_compressF {
    template <typename ForwardIteratorT>
    boost::iterator_range<ForwardIteratorT> operator()(ForwardIteratorT Begin,
                                                      ForwardIteratorT End) const {
        using value_type = typename std::iterator_traits<ForwardIteratorT>::value_type;
        using result_type = boost::iterator_range<ForwardIteratorT>;
        ForwardIteratorT MStart = End;
        value_type Cnt = 0;
        for (ForwardIteratorT It = Begin; It != End;) {
            ForwardIteratorT It2 = It++;
            if (It == End || Cnt >= (std::numeric_limits<value_type>::max)())
                return result_type(MStart, It);
            if (*It == *It2) {
                if (MStart == End) MStart = It2;
                Cnt++;
            } else if (MStart != End) {
                if (Cnt > 2) return result_type(MStart, It);
                MStart = End;
                Cnt = 0;
            }
        }
        return result_type(End, End);
    }
};

template <typename SeqT>
struct format_compressF {
    using result_type = SeqT;
    using value_type = typename SeqT::value_type;
    template <typename ReplaceT>
    result_type operator()(const ReplaceT& Replace) const {
        SeqT r;
        if (!Replace.empty()) {
            r.push_back(repeat_mark<value_type>());
            r.push_back(*(Replace.begin()));
            r.push_back(value_type(Replace.size()));
        }
        return r;
    }
};

struct find_decompressF {
    template <typename ForwardIteratorT>
    boost::iterator_range<ForwardIteratorT> operator()(ForwardIteratorT Begin,
                                                      ForwardIteratorT End) const {
        using value_type = typename std::iterator_traits<ForwardIteratorT>::value_type;
        for (ForwardIteratorT It = Begin; It != End; It++) {
            if (*It == repeat_mark<value_type>()) {
                ForwardIteratorT It2 = It++;
                if (It == End) break;
                It++;
                if (It == End) break;
                It++;
                return boost::iterator_range<ForwardIteratorT>(It2, It);
            }
        }
        return boost::iterator_range<ForwardIteratorT>(End, End);
    }
};

template <typename SeqT>
struct format_decompressF {
    using result_type = SeqT;
    using value_type = typename SeqT::value_type;
    template <typename ReplaceT>
    result_type operator()(const ReplaceT& Replace) const {
        SeqT r;
        if (!Replace.empty()) {
            auto It = Replace.begin();
            value_type Value = *(++It);
            value_type Repeat = *(++It);
            for (value_type Index = 0; Index < Repeat; Index++) r.push_back(Value);
        }
        return r;
    }
};
}  // namespace rle

TEST_CASE("example: rle_example") {
    const std::string source("123_AA_*ZZZZZZZZZZZZZZ*34");
    std::string original = source;
    std::string compress = boost::find_format_all_copy(original, rle::find_compressF(),
                                                       rle::format_compressF<std::string>());
    CHECK(compress.size() < original.size());
    CHECK(compress.find('Z') != std::string::npos);
    std::string decompress = boost::find_format_all_copy(
        compress, rle::find_decompressF(), rle::format_decompressF<std::string>());
    CHECK(decompress == source);
    boost::find_format_all(original, rle::find_compressF(), rle::format_compressF<std::string>());
    CHECK(original == compress);
    boost::find_format_all(original, rle::find_decompressF(),
                           rle::format_decompressF<std::string>());
    CHECK(original == source);
}

TEST_CASE("example: split_example") {
    std::string str1("abc-*-ABC-*-aBc");
    using string_find_iterator = boost::find_iterator<std::string::iterator>;
    std::vector<std::string> found;
    for (string_find_iterator It = boost::make_find_iterator(
             str1, boost::first_finder("abc", boost::is_iequal()));
         It != string_find_iterator(); ++It) {
        found.push_back(boost::copy_range<std::string>(*It));
        std::transform(It->begin(), It->end(), It->begin(),
                       [](char c) { return static_cast<char>(c + 1); });
    }
    CHECK(found == std::vector<std::string>{"abc", "ABC", "aBc"});
    CHECK(str1 == "bcd-*-BCD-*-bCd");
    std::vector<std::string> ResultCopy;
    boost::split(ResultCopy, str1, boost::is_any_of("-*"), boost::token_compress_on);
    CHECK(ResultCopy == std::vector<std::string>{"bcd", "BCD", "bCd"});
}

TEST_CASE("example: trim_example") {
    std::string str1("     1x x x x1     ");
    std::string str2("<>trim<>");
    std::string str3("123abs343");
    CHECK(boost::trim_left_copy(str1) == "1x x x x1     ");
    boost::trim_right(str1);
    CHECK(str1 == "     1x x x x1");
    CHECK(boost::trim_copy_if(str2, boost::is_any_of("<>")) == "trim");
    CHECK(boost::trim_copy_if(str3, boost::is_digit()) == "abs");
}
