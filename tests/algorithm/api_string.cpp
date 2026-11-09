// mcpplibs.mwrap.boost.algorithm — the string algorithms (boost/algorithm/
// string.hpp, trim_all) through the module: case conversion, trimming,
// predicates, find/replace/erase families, split/join, finders and formatters.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.algorithm;

TEST_CASE("case conversion") {
    std::string s("MiXeD");
    CHECK(boost::to_upper_copy(s) == "MIXED");
    CHECK(boost::to_lower_copy(s) == "mixed");
    boost::to_upper(s);
    CHECK(s == "MIXED");
    CHECK(boost::to_lower_copy(std::string("ÄB"), std::locale::classic()).back() == 'b');
}

TEST_CASE("trim family") {
    CHECK(boost::trim_copy(std::string("  a b  ")) == "a b");
    CHECK(boost::trim_left_copy_if(std::string("xxab"), boost::is_any_of("x")) == "ab");
    CHECK(boost::trim_right_copy_if(std::string("ab12"), boost::is_digit()) == "ab");
    std::string s("  a   b  ");
    boost::trim_all(s);
    CHECK(s == "a b");
    std::string f("--a--b--");
    boost::trim_fill(f, "+");
    CHECK(f == "--a--b--");
    std::string g("  a   b  ");
    boost::trim_fill(g, "_");
    CHECK(g == "a_b");
}

// boost::find and boost::lexicographical_compare name both the range and the
// string algorithm (each library injects its own into boost::), as with plain
// #include; the string forms are spelled boost::algorithm:: here.
TEST_CASE("predicates and classification") {
    CHECK(boost::istarts_with("Hello", "he"));
    CHECK(boost::iends_with("Hello", "LO"));
    CHECK(boost::icontains("Hello", "ELL"));
    CHECK(boost::iequals("abc", "ABC"));
    CHECK(boost::algorithm::lexicographical_compare("abc", "abd"));
    CHECK(boost::ilexicographical_compare("ABC", "abd"));
    CHECK(boost::all("12345", boost::is_digit()));
    CHECK(boost::all("abc", boost::is_lower()));
    CHECK(boost::all("ABC", boost::is_upper()));
    CHECK(boost::all("a1", boost::is_alnum()));
    CHECK(boost::all(" \t", boost::is_space()));
    CHECK(boost::all("09af", boost::is_xdigit()));
    CHECK(boost::all("cdef", boost::is_from_range('c', 'f')));
    CHECK(boost::all("ab", boost::is_classified(std::ctype_base::alpha)));
    CHECK_FALSE(boost::all("a b", boost::is_alpha() || boost::is_digit()));
}

TEST_CASE("find family") {
    std::string s("one two one two");
    CHECK(boost::find_first(s, "two").begin() - s.begin() == 4);
    CHECK(boost::find_last(s, "two").begin() - s.begin() == 12);
    CHECK(boost::find_nth(s, "one", 1).begin() - s.begin() == 8);
    CHECK(boost::ifind_first(s, "TWO").begin() - s.begin() == 4);
    CHECK(boost::find_head(s, 3) == boost::as_literal("one"));
    CHECK(boost::find_tail(s, 3) == boost::as_literal("two"));
    auto tok = boost::find_token(s, boost::is_space(), boost::token_compress_on);
    CHECK(tok.begin() - s.begin() == 3);
    CHECK(boost::algorithm::find(s, boost::first_finder("one")).begin() == s.begin());
}

TEST_CASE("replace and erase family") {
    std::string s("aaa bbb aaa");
    CHECK(boost::replace_last_copy(s, "aaa", "c") == "aaa bbb c");
    CHECK(boost::ireplace_all_copy(s, "AAA", "x") == "x bbb x");
    CHECK(boost::replace_head_copy(s, 3, "z") == "z bbb aaa");
    CHECK(boost::replace_tail_copy(s, 3, "z") == "aaa bbb z");
    CHECK(boost::erase_first_copy(s, "aaa ") == "bbb aaa");
    CHECK(boost::erase_nth_copy(s, "a", 1) == "aa bbb aaa");
    CHECK(boost::erase_head_copy(s, 4) == "bbb aaa");
    CHECK(boost::erase_tail_copy(s, 4) == "aaa bbb");
    boost::ierase_all(s, "B");
    CHECK(s == "aaa  aaa");
}

TEST_CASE("split / join / iter_split / find_all") {
    std::vector<std::string> parts;
    boost::split(parts, "a;b;;c", boost::is_any_of(";"));
    CHECK(parts == std::vector<std::string>{"a", "b", "", "c"});
    CHECK(boost::algorithm::join(parts, "+") == "a+b++c");
    CHECK(boost::algorithm::join_if(parts, "+", [](const std::string& x) { return !x.empty(); }) ==
          "a+b+c");
    std::vector<std::string> pieces;
    boost::iter_split(pieces, std::string("a--b--c"), boost::first_finder("--"));
    CHECK(pieces == std::vector<std::string>{"a", "b", "c"});
    std::vector<boost::iterator_range<std::string::const_iterator>> hits;
    const std::string text("ab-ab-ab");
    boost::find_all(hits, text, "ab");
    CHECK(hits.size() == 3);
    using split_it = boost::split_iterator<std::string::const_iterator>;
    std::vector<std::string> viaIt;
    for (split_it it = boost::make_split_iterator(text, boost::first_finder("-"));
         it != split_it(); ++it)
        viaIt.push_back(boost::copy_range<std::string>(*it));
    CHECK(viaIt == std::vector<std::string>{"ab", "ab", "ab"});
}

TEST_CASE("finders and formatters") {
    std::string s("key=value");
    auto r = boost::algorithm::find(s, boost::token_finder(boost::is_any_of("=")));
    CHECK(r.begin() - s.begin() == 3);
    CHECK(boost::algorithm::find(s, boost::range_finder(boost::make_iterator_range(s.begin(), s.begin() + 3))) ==
          boost::as_literal("key"));
    CHECK(boost::find_format_copy(s, boost::first_finder("value"), boost::const_formatter("v")) ==
          "key=v");
    CHECK(boost::find_format_all_copy(std::string("a.b.c"), boost::first_finder("."),
                                      boost::empty_formatter(std::string())) == "abc");
    CHECK(boost::algorithm::find(s, boost::tail_finder(5)) == boost::as_literal("value"));
    CHECK(boost::algorithm::find(s, boost::last_finder("e")).begin() - s.begin() == 8);
    CHECK(boost::algorithm::find(s, boost::nth_finder("e", 1)).begin() - s.begin() == 8);
}
