// mcpplibs.mwrap.boost.regex — the reference pages whose examples are inline
// (libs/regex/doc/*.qbk): basic_regex construction and flags, bad_expression,
// match_results and sub_match, the format string syntax, the perl syntax
// features, the Unicode iterators, and the "C++20 module" page (upstream's
// `import boost.regex;`, here this package's module).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.regex;

TEST_CASE("doc: intro") {
    // The introduction's credit-card style validation and the main algorithms.
    boost::regex e("(\\d{4}[- ]){3}\\d{4}");
    CHECK(boost::regex_match("1234-5678-1234-5678", e));
    CHECK_FALSE(boost::regex_match("1234-5678-1234", e));
    std::string s = "1234 5678 1234 5678";
    CHECK(boost::regex_replace(s, boost::regex("[ -]"), "") == "1234567812345678");
}

TEST_CASE("doc: modules") {
    boost::regex re("^b.+t$");
    CHECK(boost::regex_match(std::string("boost"), re));
}

TEST_CASE("doc: basic_regex") {
    boost::regex a("abc"), b("ABC", boost::regex::icase), c("a.c", boost::regex::perl | boost::regex::nosubs);
    CHECK(a.str() == "abc");
    CHECK(a.mark_count() == 0);
    CHECK(boost::regex_match("abc", b));
    CHECK(boost::regex_match("aXc", c));
    CHECK((b.flags() & boost::regex::icase) != 0);
    boost::regex d;
    CHECK(d.empty());
    d.assign("(x)(y)");
    CHECK(d.mark_count() == 2);
    boost::wregex w(L"\\w+");
    CHECK(boost::regex_match(std::wstring(L"word"), w));
    boost::regex basic("a\\{2\\}", boost::regex::basic);
    CHECK(boost::regex_match("aa", basic));
    a.swap(d);
    CHECK(a.mark_count() == 2);
}

TEST_CASE("doc: bad_expression") {
    CHECK_THROWS_AS(boost::regex("(unclosed"), boost::regex_error);
    try {
        boost::regex bad("[z-a]");
        FAIL("expected regex_error");
    } catch (const boost::regex_error& e) {
        CHECK(e.code() == boost::regex_constants::error_ctype);  // an invalid range is reported as error_ctype
        CHECK(std::string(e.what()).size() > 0);
    }
    static_assert(std::is_same_v<boost::bad_expression, boost::regex_error>);
    boost::regex nothrow("(", boost::regex::no_except);
    CHECK(nothrow.status() != 0);
}

TEST_CASE("doc: match_results") {
    boost::smatch what;
    std::string s = "key=value; other=thing";
    REQUIRE(boost::regex_search(s, what, boost::regex("(\\w+)=(\\w+)")));
    CHECK(what.size() == 3);
    CHECK(what[0] == "key=value");
    CHECK(what.str(2) == "value");
    CHECK(what.position(2) == 4);
    CHECK(what.length(1) == 3);
    CHECK(what.prefix().str().empty());
    CHECK(what.suffix().str() == "; other=thing");
    CHECK(what.format("$2:$1") == "value:key");
    boost::smatch named;
    REQUIRE(boost::regex_search(s, named, boost::regex("(?<k>\\w+)=(?<v>\\w+)")));
    CHECK(named["v"] == "value");
    CHECK(named.empty() == false);
}

TEST_CASE("doc: sub_match") {
    boost::cmatch m;
    REQUIRE(boost::regex_match("ab12", m, boost::regex("([a-z]+)(\\d+)?")));
    boost::csub_match sm = m[2];
    CHECK(sm.matched);
    CHECK(sm.length() == 2);
    CHECK(sm.str() == "12");
    CHECK(sm == "12");
    CHECK(sm.compare("13") < 0);
    boost::cmatch n;
    REQUIRE(boost::regex_match("ab", n, boost::regex("([a-z]+)(\\d+)?")));
    CHECK_FALSE(n[2].matched);
}

TEST_CASE("doc: regex_format") {
    std::string date = "2026-11-09";
    boost::regex re("(\\d+)-(\\d+)-(\\d+)");
    CHECK(boost::regex_replace(date, re, "$3/$2/$1") == "09/11/2026");                                  // perl
    CHECK(boost::regex_replace(date, re, "\\3.\\2.\\1", boost::format_sed) == "09.11.2026");             // sed
    CHECK(boost::regex_replace(date, re, "(?1year)", boost::format_all) == "year");                       // boost-extended
    CHECK(boost::regex_replace(std::string("a b"), boost::regex("\\w"), "[$&]") == "[a] [b]");
    CHECK(boost::regex_replace(std::string("a b"), boost::regex("\\w"), "x", boost::format_first_only) == "x b");
    CHECK(boost::regex_replace(std::string("a b"), boost::regex("\\w"), "x", boost::format_no_copy) == "xx");
    CHECK(boost::regex_replace(std::string("Ab"), boost::regex("(\\w)(\\w)"), "\\U$1\\L$2", boost::format_all) ==
          "Ab");
}

TEST_CASE("doc: perl_syntax") {
    auto m = [](const char* re, const char* s) { return boost::regex_search(s, boost::regex(re)); };
    CHECK(m("colou?r", "color"));
    CHECK(m("(?i)HELLO", "hello"));
    CHECK(m("\\bword\\b", "a word here"));
    CHECK(m("foo(?=bar)", "foobar"));
    CHECK_FALSE(m("foo(?!bar)", "foobar"));
    CHECK(m("(?<=\\$)\\d+", "cost $42"));
    CHECK(m("(a)\\1", "aa"));
    CHECK(m("[[:alpha:]]+[[:digit:]]", "abc1"));
    CHECK(m("a{2,3}", "baaab"));
    CHECK(m("(?>a+)b", "aab"));
    CHECK(m("\\d+(?#a comment)", "7"));
    CHECK(m("(?x) a b c ", "abc"));
    CHECK(m("\\Qa.b\\E", "a.b"));
    // Collating element names and multi-character collating elements
    // (def_coll_names / def_multi_coll in regex_traits_defaults.hpp).
    CHECK(m("a[[.space.]]b", "a b"));
    CHECK(m("[[.hyphen.]]", "-"));
    CHECK_FALSE(m("\\Qa.b\\E", "axb"));
}

TEST_CASE("doc: uni_iter") {
    const std::string utf8 = "h\xC3\xA9llo";  // "héllo"
    using u32it = boost::u8_to_u32_iterator<std::string::const_iterator>;
    std::u32string cps(u32it(utf8.begin(), utf8.begin(), utf8.end()), u32it(utf8.end(), utf8.begin(), utf8.end()));
    CHECK(cps.size() == 5);
    CHECK(cps[1] == 0xE9);
    std::string back;
    std::copy(cps.begin(), cps.end(), boost::utf8_output_iterator<std::back_insert_iterator<std::string>>(
                                          std::back_inserter(back)));
    CHECK(back == utf8);
    using u8it = boost::u32_to_u8_iterator<std::u32string::const_iterator>;
    CHECK(std::string(u8it(cps.begin()), u8it(cps.end())) == utf8);
}

TEST_CASE("regex_constants and match flags") {
    std::string s = "aaa";
    boost::smatch m;
    CHECK(boost::regex_search(s, m, boost::regex("a+?")));
    CHECK(m.length(0) == 1);
    CHECK_FALSE(boost::regex_search(s.cbegin() + 1, s.cend(), boost::regex("^a"), boost::match_not_bol));
    CHECK(boost::regex_match(s, boost::regex("a*"), boost::match_any));
}
