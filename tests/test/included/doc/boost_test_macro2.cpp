// Boost.Test documentation example boost_test_macro2.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `boost_test_macro2 --log_level=all --color_output=no` (the command line of boost_test_macro2.output),
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
#define BOOST_TEST_MODULE boost_test_macro2
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE( test_op_precedence )
{
  int a = 13, b = 2, c = 12;
  // left term of == is expanded in the logs
  BOOST_TEST(a % b == c);
  // right term of == is not expanded in the logs
  BOOST_TEST(a == c % b);
}

BOOST_AUTO_TEST_CASE( test_op_right_associative )
{
  int a = 1;
  BOOST_TEST(a);
  BOOST_TEST(!a);
  BOOST_TEST(--a);
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
    std::vector<std::string> args = {"boost_test_macro2", "--log_level=all", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Running 2 test cases...
Entering test module "boost_test_macro2"
<src>: Entering test case "test_op_precedence"
<src>: error: in "test_op_precedence": check a % b == c has failed [13 % 2 != 12]
<src>: error: in "test_op_precedence": check a == c % b has failed [13 != 0]
<src>: Leaving test case "test_op_precedence"; testing time: <t>
<src>: Entering test case "test_op_right_associative"
<src>: info: check a has passed
<src>: error: in "test_op_right_associative": check !a has failed
<src>: error: in "test_op_right_associative": check --a has failed ['0' evaluates to false]
<src>: Leaving test case "test_op_right_associative"; testing time: <t>
Leaving test module "boost_test_macro2"; testing time: <t>

*** 4 failures are detected in the test module "boost_test_macro2"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
