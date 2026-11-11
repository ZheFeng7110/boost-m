// Boost.Test example filtering_example.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `filtering_example --color_output=no`,
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
//  (C) Copyright Gennadiy Rozental 2015.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

#define BOOST_TEST_MODULE filtering test
#include <boost/test/included/unit_test.hpp>
namespace bt=boost::unit_test;

const std::string test1v("test1");
const std::string test2v("test2");
const std::string test3v("test3");

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE( s1,
* bt::disabled()
* bt::description( "initially disabled suite 1")
* bt::label( "label1" )
* bt::label( "label2" ))

BOOST_AUTO_TEST_CASE( test1,
* bt::enabled()
* bt::description("initially enabled case s1/t1"))
{
    BOOST_TEST( "s1" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::description( "initially defaulted case s1/t2")
* bt::expected_failures( 1 ))
{
    BOOST_TEST( "s1" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test3,
* bt::description( "initially defaulted case s1/t3"))
{
    BOOST_TEST( "s1" == test3v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s2,
* bt::disabled()
* bt::label( "label1" )
* bt::expected_failures( 3 ))

BOOST_AUTO_TEST_CASE( test1,
* bt::description( "initially defaulted case s2/t1"))
{
    BOOST_TEST( "s2" == test1v );
}

//____________________________________________________________________________//

boost::test_tools::assertion_result
do_it( bt::test_unit_id )
{
   return false;
}

BOOST_AUTO_TEST_CASE( test2,
* bt::enabled()
* bt::description( "initially enabled case s2/t2")
* bt::precondition(do_it))
{
    BOOST_TEST( "s2" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test3,
* bt::description( "initially defaulted case s2/t3"))
{
    BOOST_TEST( "s2" == test3v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s3,
* bt::disabled())

BOOST_AUTO_TEST_CASE( test1 )
{
    BOOST_TEST( "s3" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::timeout( 10 ))
{
    BOOST_TEST( "s3" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test3,
* bt::enabled())
{
    BOOST_TEST( "s3" == test3v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE( s14,
* bt::depends_on( "s3/s15" )
* bt::description( "test suite which depends on another test suite"))

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s2" ))
{
    BOOST_TEST( "s14" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s15 )

BOOST_AUTO_TEST_CASE( test1 )
{
    BOOST_TEST( "s15" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s4 )

BOOST_AUTO_TEST_CASE( test1,
* bt::disabled()
* bt::label( "label2" ))
{
    BOOST_TEST( "s4" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s4/test1" )
* bt::description( "test case which depends on disabled s4/t1"))
{
    BOOST_TEST( "s4" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test3,
* bt::depends_on( "s4/test2" )
* bt::description( "test case which depends on enabled s4/t2, but indirectly on disabled s4/t1"))
{
    BOOST_TEST( "s4" == test3v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

#if 0
BOOST_AUTO_TEST_SUITE( s5 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s5/test3" ))
{
    BOOST_TEST( "s5" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s5/test1" ))
{
    BOOST_TEST( "s5" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test3,
* bt::depends_on( "s5/test2" ))
{
    BOOST_TEST( "s5" == test3v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()
#endif

BOOST_AUTO_TEST_SUITE( s6 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s6/test3" )
* bt::description( "test case which depends on enabled s6/t3"))
{
    BOOST_TEST( "s6" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s6/test1" )
* bt::description( "test case which depends on enabled s6/t1"))
{
    BOOST_TEST( "s6" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test3 )
{
    BOOST_TEST( "s6" == test3v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE( s9,
* bt::description( "test suite with all test cases disabled"))

BOOST_AUTO_TEST_CASE( test1,
* bt::disabled()
* bt::label( "label1" ))
{
    BOOST_TEST( "s9" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::disabled())
{
    BOOST_TEST( "s9" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()

#if 0
BOOST_AUTO_TEST_SUITE( s7 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s8/test1" ))
{
    BOOST_TEST( "s7" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s7/test1" ))
{
    BOOST_TEST( "s7" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s8 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s8/test2" ))
{
    BOOST_TEST( "s8" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s7/test2" ))
{
    BOOST_TEST( "s8" == test2v );
}

//____________________________________________________________________________//
`
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s10 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s11" ))
{
    BOOST_TEST( "s10" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s10/test1" ))
{
    BOOST_TEST( "s10" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s11 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s11/test2" ))
{
    BOOST_TEST( "s11" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::depends_on( "s10" ))
{
    BOOST_TEST( "s11" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

#endif

BOOST_AUTO_TEST_SUITE( s12 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s13" )
* bt::description( "test test case which depends on test suite with all test cases skipped"))
{
    BOOST_TEST( "s12" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( s13 )

BOOST_AUTO_TEST_CASE( test1,
* bt::depends_on( "s13/test2" ))
{
    BOOST_TEST( "s13" == test1v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_CASE( test2,
* bt::disabled())
{
    BOOST_TEST( "s13" == test2v );
}

//____________________________________________________________________________//

BOOST_AUTO_TEST_SUITE_END()

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
    std::vector<std::string> args = {"filtering_example", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc == 200;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 10 test cases...
<src>: error: in "s1/test1": check "s1" == test1v has failed [s1 != test1]
<src>: error: in "s6/test3": check "s6" == test3v has failed [s6 != test3]
<src>: error: in "s3/test3": check "s3" == test3v has failed [s3 != test3]

*** 3 failures are detected (4 failures are expected) in the test module "filtering test"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
