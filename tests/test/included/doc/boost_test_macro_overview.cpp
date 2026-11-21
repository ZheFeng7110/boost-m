// Boost.Test documentation example boost_test_macro_overview.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `boost_test_macro_overview --log_level=all --color_output=no` (the command line of boost_test_macro_overview.output),
// compares its output with the oracle's and checks a failing exit status.
#include <cstdio>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
// With the unit-test-framework feature the framework TUs link into every
// test program and would collide with this header-only form (see
// tests/test/included/smoke.cpp): the program is a passing stub then.
#ifdef MCPP_FEATURE_UNIT_TEST_FRAMEWORK
int main() { return 0; }
#else
#define main ex_original_main
#line 1 "test.cpp"
//  (C) Copyright Raffi Enficiaud 2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_MODULE boost_test_macro_overview
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE( test_macro_overview )
{
  namespace tt = boost::test_tools;
  int a = 1;
  int b = 2;
  BOOST_TEST(a != b - 1);
  BOOST_TEST(a + 1 < b);
  BOOST_TEST(b -1 > a, a << " < " << b - 1 << " does not hold");
  BOOST_TEST(a == b, tt::bitwise());
  BOOST_TEST(a + 0.1 == b - 0.8, tt::tolerance(0.01));
}
//]

#line 1 "boost-m harness"
#undef main
// boost-m harness: run the example's main with the documented command line,
// compare what it printed with the documented output.
#include <cstdlib>
#include <cstring>
namespace ex_harness {
inline std::string normalize(std::string s) {
    static const std::regex time(R"(testing time: [0-9.]+ ?[mun]?s)");
    s = std::regex_replace(s, time, "testing time: <t>");
    // the source location: file(line) or file:line by platform, and the line
    // of a macro spanning lines differs between compilers
    static const std::regex ansi("\x1b\\[[0-9;]*m");  // colour codes some programs turn on
    s = std::regex_replace(s, ansi, "");
    static const std::regex loc(R"(([A-Za-z0-9_./\-]+\.(cpp|hpp|ipp)|unknown location)(\(([0-9]+)\)|:([0-9]+)):)");
    s = std::regex_replace(s, loc, "<src>:");
    // the system's description of a memory fault (POSIX: "at address: A: no
    // mapping ..." / "invalid permissions"; Windows: "occurred at address A, while ...")
    static const std::regex fault(R"(memory access violation (?:occurred )?at address:? (0x[0-9a-fA-F]+)[,:][^\n]*)");
    s = std::regex_replace(s, fault, "memory access violation at address $1: <fault>");
    // MSVC's type names carry the class-key ("class std::runtime_error")
    static const std::regex key(R"((: |error: )(?:class|struct) )");
    s = std::regex_replace(s, key, "$1");
    // trailing blanks and blank lines are not part of what is compared
    static const std::regex trailing(R"([ \t]+\n)");
    s = std::regex_replace(s, trailing, "\n");
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    while (!s.empty() && s.front() == '\n') s.erase(0, 1);
    return s;
}
}
int main() {
    std::vector<std::string> args = {"boost_test_macro_overview", "--log_level=all", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc != 0;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 1 test case...
Entering test module "boost_test_macro_overview"
<src>: Entering test case "test_macro_overview"
<src>: error: in "test_macro_overview": check a != b - 1 has failed [1 == 1]
<src>: error: in "test_macro_overview": check a + 1 < b has failed [1 + 1 >= 2]
<src>: error: in "test_macro_overview": 1 < 1 does not hold
<src>: error: in "test_macro_overview": check a == b has failed [1 != 2]. Bitwise comparison failed
Mismatch at position 0
Mismatch at position 1
<src>: error: in "test_macro_overview": check a + 0.1 == b - 0.8 has failed [1 + 0.10000000000000001 != 1.2]: relative difference exceeds tolerance [0.0909091 > 0.01]
<src>: Leaving test case "test_macro_overview"; testing time: <t>
Leaving test module "boost_test_macro_overview"; testing time: <t>

*** 5 failures are detected in the test module "boost_test_macro_overview"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
