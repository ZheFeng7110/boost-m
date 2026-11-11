// Boost.Test documentation example decorator_07.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `decorator_07 --report_level=detailed --log_level=all --color_output=no` (the command line of decorator_07.output),
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
//  (C) Copyright Andrzej Krzemienski 2015.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_MODULE decorator_07
#include <boost/test/included/unit_test.hpp>

namespace utf = boost::unit_test;

// test1 and test2 defined at the bottom

BOOST_AUTO_TEST_CASE(test3, * utf::depends_on("s1/test1"))
{
  BOOST_TEST(false);
}

BOOST_AUTO_TEST_CASE(test4, * utf::depends_on("test3"))
{
  BOOST_TEST(false);
}

BOOST_AUTO_TEST_CASE(test5, * utf::depends_on("s1/test2"))
{
  BOOST_TEST(false);
}

BOOST_AUTO_TEST_SUITE(s1)

  BOOST_AUTO_TEST_CASE(test1)
  {
    BOOST_TEST(true);
  }

  BOOST_AUTO_TEST_CASE(test2, * utf::disabled())
  {
    BOOST_TEST(false);
  }

BOOST_AUTO_TEST_SUITE_END()
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
    static const std::regex loc(R"([A-Za-z0-9_./\-]+\.(cpp|hpp|ipp)(\(([0-9]+)\)|:([0-9]+)):)");
    s = std::regex_replace(s, loc, "<src>:");
    // trailing blanks and blank lines are not part of what is compared
    static const std::regex trailing(R"([ \t]+\n)");
    s = std::regex_replace(s, trailing, "\n");
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    while (!s.empty() && s.front() == '\n') s.erase(0, 1);
    return s;
}
}
int main() {
    std::vector<std::string> args = {"decorator_07", "--report_level=detailed", "--log_level=all", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = true ? rc != 0 : rc == 0;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 4 test cases...
Entering test module "decorator_07"
<src>: Entering test suite "s1"
<src>: Entering test case "test1"
<src>: info: check true has passed
<src>: Leaving test case "test1"; testing time: <t>
<src>: Test case "s1/test2" is skipped because disabled
<src>: Leaving test suite "s1"; testing time: <t>
<src>: Entering test case "test3"
<src>: error: in "test3": check false has failed
<src>: Leaving test case "test3"; testing time: <t>
<src>: Test case "test5" is skipped because dependency test case "s1/test2" is disabled
<src>: Test case "test4" is skipped because dependency test case "test3" has failed
Leaving test module "decorator_07"; testing time: <t>

Test module "decorator_07" has failed with:
  1 test case out of 5 passed
  1 test case out of 5 failed
  3 test cases out of 5 skipped
  1 assertion out of 2 passed
  1 assertion out of 2 failed

  Test case "test3" has failed with:
    1 assertion out of 1 failed

  Test case "test4" was skipped
  Test case "test5" was skipped
  Test suite "s1" has passed with:
    1 test case out of 2 passed
    1 test case out of 2 skipped
    1 assertion out of 1 passed

    Test case "s1/test1" has passed with:
      1 assertion out of 1 passed
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
