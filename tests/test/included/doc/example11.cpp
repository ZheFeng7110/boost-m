// Boost.Test documentation example example11.run.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `example11 --log_level=test_suite --color_output=no` (the command line of example11.output),
// compares its output with the oracle's and checks a zero exit status.
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
//  (C) Copyright Gennadiy Rozental 2011-2015.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;

void test_case1() { /* ... */ }
void test_case2() { /* ... */ }
void test_case3() { /* ... */ }
void test_case4() { /* ... */ }

test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/[] )
{
  test_suite* ts1 = BOOST_TEST_SUITE( "test_suite1" );
  ts1->add( BOOST_TEST_CASE( &test_case1 ) );
  ts1->add( BOOST_TEST_CASE( &test_case2 ) );

  test_suite* ts2 = BOOST_TEST_SUITE( "test_suite2" );
  ts2->add( BOOST_TEST_CASE( &test_case3 ) );
  ts2->add( BOOST_TEST_CASE( &test_case4 ) );

  framework::master_test_suite().add( ts1 );
  framework::master_test_suite().add( ts2 );

  return 0;
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
    std::vector<std::string> args = {"example11", "--log_level=test_suite", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc == 0;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 4 test cases...
Entering test module "Master Test Suite"
<src>: Entering test suite "test_suite1"
<src>: Entering test case "test_case1"
Test case test_suite1/test_case1 did not check any assertions
<src>: Leaving test case "test_case1"; testing time: <t>
<src>: Entering test case "test_case2"
Test case test_suite1/test_case2 did not check any assertions
<src>: Leaving test case "test_case2"; testing time: <t>
<src>: Leaving test suite "test_suite1"; testing time: <t>
<src>: Entering test suite "test_suite2"
<src>: Entering test case "test_case3"
Test case test_suite2/test_case3 did not check any assertions
<src>: Leaving test case "test_case3"; testing time: <t>
<src>: Entering test case "test_case4"
Test case test_suite2/test_case4 did not check any assertions
<src>: Leaving test case "test_case4"; testing time: <t>
<src>: Leaving test suite "test_suite2"; testing time: <t>
Leaving test module "Master Test Suite"; testing time: <t>

*** No errors detected
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
