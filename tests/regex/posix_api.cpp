// mcpplibs.mwrap.boost.regex — the POSIX-compatible C API (posix_api.qbk):
// regcomp / regexec / regerror / regfree over regex_t. The documented names
// are macros (regcomp → regcompA / regcompW by UNICODE, regex_t likewise), so
// a consumer includes <boost/cregex.hpp> for them, as here; the functions are
// the ones the package compiles (libs/regex/src/posix_api.cpp) and the
// module's C++ API is imported alongside.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/cregex.hpp>
import std;
import mcpplibs.mwrap.boost.regex;

using namespace boost;

TEST_CASE("doc: posix") {
    regex_t re;
    REQUIRE(regcomp(&re, "([a-z]+)@([a-z]+)\\.com", REG_EXTENDED) == 0);
    CHECK(re.re_nsub == 2);
    regmatch_t m[3];
    const char* text = "mail: user@example.com.";
    REQUIRE(regexec(&re, text, 3, m, 0) == 0);
    CHECK(std::string(text + m[0].rm_so, text + m[0].rm_eo) == "user@example.com");
    CHECK(std::string(text + m[1].rm_so, text + m[1].rm_eo) == "user");
    CHECK(std::string(text + m[2].rm_so, text + m[2].rm_eo) == "example");
    CHECK(regexec(&re, "no address", 3, m, 0) == static_cast<int>(REG_NOMATCH));
    regfree(&re);

    // REG_ICASE, REG_NOSPEC (a literal), REG_BASIC syntax
    REQUIRE(regcomp(&re, "ABC", REG_EXTENDED | REG_ICASE) == 0);
    CHECK(regexec(&re, "xxabcxx", 0, nullptr, 0) == 0);
    regfree(&re);
    REQUIRE(regcomp(&re, "a.c", REG_EXTENDED | REG_NOSPEC) == 0);
    CHECK(regexec(&re, "abc", 0, nullptr, 0) != 0);
    CHECK(regexec(&re, "a.c", 0, nullptr, 0) == 0);
    regfree(&re);
    REQUIRE(regcomp(&re, "\\(ab\\)\\{2\\}", REG_BASIC) == 0);
    CHECK(regexec(&re, "abab", 0, nullptr, 0) == 0);
    regfree(&re);
    // REG_NOTBOL
    REQUIRE(regcomp(&re, "^x", REG_EXTENDED) == 0);
    CHECK(regexec(&re, "x", 0, nullptr, REG_NOTBOL) != 0);
    regfree(&re);

    // regerror: a failed compile, the message, and the REG_ATOI / REG_ITOA
    // name <-> number conversions
    int rc = regcomp(&re, "(unclosed", REG_EXTENDED);
    CHECK(rc == static_cast<int>(REG_EPAREN));
    char buf[256];
    CHECK(regerror(rc, nullptr, buf, sizeof buf) > 1);
    CHECK(std::string(buf).size() > 0);
    CHECK(regerror(rc | REG_ITOA, nullptr, buf, sizeof buf) > 0);
    CHECK(std::string(buf) == "REG_EPAREN");
    regex_t named{};
    named.re_endp = "REG_EPAREN";
    CHECK(regerror(REG_ATOI, &named, buf, sizeof buf) > 0);
    CHECK(std::string(buf) == std::to_string(REG_EPAREN));
}
