// Boost.Test documentation example boost_runtime_list_content.run.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `boost_runtime_list_content --list_content --color_output=no` (the command line of boost_runtime_list_content.output),
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
//  (C) Copyright 2015 Boost.Test team.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_MODULE list_content
#include <boost/test/included/unit_test.hpp>
namespace utf=boost::unit_test;

//// --------------------------------------------------------------------------
// Test suite 1, disabled by default, s1/test2 is explicitely enabled.
BOOST_AUTO_TEST_SUITE( s1,
* utf::disabled()                         // suite is not disabled because of the
* utf::description( "disabled suite 1")   // extra declaration at the end of the file
* utf::label( "label1" )
* utf::label( "label2" ))

BOOST_AUTO_TEST_CASE( test1, // s1/test1
* utf::enabled() * utf::description("enabled"))
{
    BOOST_TEST(true);
}

BOOST_AUTO_TEST_CASE( test2, // s1/test2
* utf::description( "defaulted") * utf::expected_failures( 1 ))
{
    BOOST_TEST(false);
}

BOOST_AUTO_TEST_CASE( test3, // s1/test3
* utf::description( "defaulted"))
{
    BOOST_TEST(false);
}
BOOST_AUTO_TEST_SUITE_END()


//// --------------------------------------------------------------------------
// Test suite 2, disabled by default, s2/test2 is explicitely enabled.
BOOST_AUTO_TEST_SUITE( s2,
* utf::disabled()
* utf::label( "label1" )
* utf::expected_failures( 3 ))

BOOST_AUTO_TEST_CASE( test1, // s2/test1
* utf::description( "defaulted"))
{
    BOOST_TEST(false);
}

boost::test_tools::assertion_result do_it( utf::test_unit_id )
{
   return false;
}

BOOST_AUTO_TEST_CASE( test2, // s2/test2
* utf::enabled()
* utf::description( "enabled w. precondition")
* utf::precondition(do_it))
{
  BOOST_TEST(false);
}

//// --------------------------------------------------------------------------
// Test suite s2/s23, disabled
BOOST_AUTO_TEST_SUITE( s23, * utf::disabled())

BOOST_AUTO_TEST_CASE( test1 ) // s2/s23/test1
{
  BOOST_TEST(false);
}

BOOST_AUTO_TEST_CASE( test2, // s2/s23/test2
* utf::timeout( 10 ))
{
  BOOST_TEST( true );
}

BOOST_AUTO_TEST_CASE( test3, // s2/s23/test3
* utf::enabled()
* utf::depends_on( "s2/test2" ))
{
  BOOST_TEST( true );
}
BOOST_AUTO_TEST_SUITE_END() // s2/s23
BOOST_AUTO_TEST_SUITE_END() // s2



//// --------------------------------------------------------------------------
// Test suite s1 continued
BOOST_AUTO_TEST_SUITE( s1 )
BOOST_AUTO_TEST_SUITE( s14,
* utf::depends_on( "s2/s23/test3" )
* utf::description( "test suite which depends on another test suite"))

BOOST_AUTO_TEST_CASE( test1, // s1/s14/test1
* utf::depends_on( "s2" ))
{
    BOOST_TEST( "s14" == "test" );
}

BOOST_AUTO_TEST_SUITE_END() // s1/s14
BOOST_AUTO_TEST_SUITE_END() // s1
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
    std::vector<std::string> args = {"boost_runtime_list_content", "--list_content", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(s1*: disabled suite 1
    test1*: enabled
    test2 : defaulted
    test3 : defaulted
    s14 : test suite which depends on another test suite
        test1
s2*
    test1 : defaulted
    test2*: enabled w. precondition
    s23*
        test1
        test2
        test3*
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
