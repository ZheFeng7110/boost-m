// Boost.Test example unit_test_example_04.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `unit_test_example_04 --color_output=no`,
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
//  (C) Copyright Gennadiy Rozental 2005-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#define BOOST_TEST_MODULE Unit_test_example_04 
#include <boost/test/included/unit_test.hpp>
namespace bt=boost::unit_test;

//____________________________________________________________________________//

struct suite_fixture {
    suite_fixture()     { BOOST_TEST_MESSAGE( "Running some test suite setup" ); }
    ~suite_fixture()    { BOOST_TEST_MESSAGE( "Running some test suite teardown" ); }
};
struct suite_fixture2 {
    suite_fixture2()    { BOOST_TEST_MESSAGE( "Running some more test suite setup" ); }
    ~suite_fixture2()   { BOOST_TEST_MESSAGE( "Running some more test suite teardown" ); }
};

// automatically registered test cases could be organized in test suites
BOOST_AUTO_TEST_SUITE( my_suite1, 
* bt::fixture<suite_fixture>()
* bt::fixture<suite_fixture2>() )

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(my_test1,1)

void some_setup()
{
    BOOST_TEST_MESSAGE( "Running some extra setup" );
}

auto& d =
* bt::label( "L1" )
* bt::label( "L2" )
* bt::description( "test case 1 description" )
* bt::fixture( &some_setup );
BOOST_AUTO_TEST_CASE( my_test1 )
{
    BOOST_TEST( 2 == 1 );
}

//____________________________________________________________________________//

// this test case belongs to suite1 test suite
BOOST_AUTO_TEST_CASE( my_test2, * bt::description( "test case 2 description" ) )
{
    int i = 0;

    BOOST_TEST( i == 2 );

    BOOST_TEST( i == 0 );
}

BOOST_AUTO_TEST_SUITE_END()

//____________________________________________________________________________//

// this test case belongs to master test suite
BOOST_AUTO_TEST_CASE( my_test3 )
{
    int i = 0;

    BOOST_TEST( i == 0 );
}

//____________________________________________________________________________//

BOOST_TEST_DECORATOR(
* bt::label( "L3" )
* bt::description( "suite description" )
)
BOOST_AUTO_TEST_SUITE( my_suite2 )

// this test case belongs to suite2 test suite
BOOST_AUTO_TEST_CASE( my_test4, * bt::depends_on( "my_suite2/internal_suite/my_test5" ) )
{
    int i = 0;

    BOOST_CHECK_EQUAL( i, 1 );
}

BOOST_AUTO_TEST_SUITE( internal_suite )

// this test case belongs to my_suite2:internal_suite test suite

BOOST_TEST_DECORATOR(
* bt::timeout( 100 )
)
BOOST_AUTO_TEST_CASE( my_test5, * bt::expected_failures( 1 ) )
{
    int i = 0;

    BOOST_CHECK_EQUAL( i, 1 );
}

BOOST_AUTO_TEST_CASE( my_test6, *bt::disabled() )
{
}

BOOST_AUTO_TEST_CASE( this_should_also_be_disabled, 
* bt::depends_on( "my_suite2/internal_suite/disabled_suite/my_test7" ) )
{
}

BOOST_AUTO_TEST_SUITE( disabled_suite, * bt::disabled() )

BOOST_AUTO_TEST_CASE( my_test7 ) {}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

//____________________________________________________________________________//

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
    std::vector<std::string> args = {"unit_test_example_04", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Running 6 test cases...
<src>: error: in "my_suite1/my_test1": check 2 == 1 has failed [2 != 1]
<src>: error: in "my_suite1/my_test2": check i == 2 has failed [0 != 2]
<src>: error: in "my_suite2/internal_suite/my_test5": check i == 1 has failed [0 != 1]
<src>: error: in "my_suite2/my_test4": check i == 1 has failed [0 != 1]

*** 4 failures are detected (2 failures are expected) in the test module "Unit_test_example_04"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
