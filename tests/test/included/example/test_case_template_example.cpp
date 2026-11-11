// Boost.Test example test_case_template_example.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `test_case_template_example --color_output=no`,
// compares its output and exit status with the oracle's.
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
//  (C) Copyright Gennadiy Rozental 2001-2006.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#ifdef BOOST_MSVC
# pragma warning(disable: C4345)
#endif

#include <boost/test/included/unit_test.hpp>
using boost::unit_test::test_suite;

// Boost.MPL
#include <boost/mpl/range_c.hpp>

BOOST_TEST_CASE_TEMPLATE_FUNCTION( free_test_function, Number )
{
  BOOST_CHECK_EQUAL( 2, static_cast<int>(Number::value) );
}

test_suite* init_unit_test_suite( int, char* [] ) 
{
  test_suite* test= BOOST_TEST_SUITE( "Test case template example" );

  typedef boost::mpl::range_c<int,0,10> numbers;

  test->add( BOOST_TEST_CASE_TEMPLATE( free_test_function, numbers ) );

  return test;
}

// EOF

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
    std::vector<std::string> args = {"test_case_template_example", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc == 201;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 10 test cases...
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 0>>": check 2 == static_cast<int>(Number::value) has failed [2 != 0]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 1>>": check 2 == static_cast<int>(Number::value) has failed [2 != 1]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 3>>": check 2 == static_cast<int>(Number::value) has failed [2 != 3]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 4>>": check 2 == static_cast<int>(Number::value) has failed [2 != 4]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 5>>": check 2 == static_cast<int>(Number::value) has failed [2 != 5]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 6>>": check 2 == static_cast<int>(Number::value) has failed [2 != 6]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 7>>": check 2 == static_cast<int>(Number::value) has failed [2 != 7]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 8>>": check 2 == static_cast<int>(Number::value) has failed [2 != 8]
<src>: error: in "Test case template example/free_test_function<mpl___integral_c<int_ 9>>": check 2 == static_cast<int>(Number::value) has failed [2 != 9]

*** 9 failures are detected in the test module "Master Test Suite"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
