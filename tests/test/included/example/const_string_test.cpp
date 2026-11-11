// Boost.Test example const_string_test.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `const_string_test --color_output=no`,
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
//  (C) Copyright Gennadiy Rozental 2001-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.
//
//  File        : $RCSfile: const_string_test.cpp,v $
//
//  Version     : $Revision$
//
//  Description : simple string class test
// ***************************************************************************

#define BOOST_TEST_MODULE const_string test
#include <boost/test/included/unit_test.hpp>

#include "const_string.hpp"
using common_layer::const_string;

BOOST_AUTO_TEST_CASE( constructors_test )
{
    const_string cs0( "" );
    BOOST_CHECK_EQUAL( cs0.length(), (size_t)0 );
    BOOST_CHECK_EQUAL( cs0.begin(), "" );
    BOOST_CHECK_EQUAL( cs0.end(), "" );
    BOOST_CHECK( cs0.is_empty() );

    const_string cs01( NULL );
    BOOST_CHECK_EQUAL( cs01.length(), (size_t)0 );
    BOOST_CHECK_EQUAL( cs01.begin(), "" );
    BOOST_CHECK_EQUAL( cs01.end(), "" );
    BOOST_CHECK( cs01.is_empty() );

    const_string cs1( "test_string" );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );
    BOOST_CHECK_EQUAL( cs1.length(), std::strlen("test_string") );

    std::string  s( "test_string" );
    const_string cs2( s );
    BOOST_CHECK_EQUAL( std::strcmp( cs2.data(), "test_string" ), 0 );

    const_string cs3( cs1 );
    BOOST_CHECK_EQUAL( std::strcmp( cs3.data(), "test_string" ), 0 );

    const_string cs4( "test_string", 4 );
    BOOST_CHECK_EQUAL( std::strncmp( cs4.data(), "test", cs4.length() ), 0 );

    const_string cs5( s.data(), s.data() + s.length() );
    BOOST_CHECK_EQUAL( std::strncmp( cs5.data(), "test_string", cs5.length() ), 0 );

    const_string cs_array[] = { "str1", "str2" };

    BOOST_CHECK_EQUAL( cs_array[0], "str1" );
    BOOST_CHECK_EQUAL( cs_array[1], "str2" );
}

BOOST_AUTO_TEST_CASE( data_access_test )
{
    const_string cs1( "test_string" );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );

    BOOST_CHECK_EQUAL( cs1[(size_t)0], 't' );
    BOOST_CHECK_EQUAL( cs1[(size_t)4], '_' );
    BOOST_CHECK_EQUAL( cs1[cs1.length()-1], 'g' );

    BOOST_CHECK_EQUAL( cs1[(size_t)0], cs1.at( 0 ) );
    BOOST_CHECK_EQUAL( cs1[(size_t)2], cs1.at( 5 ) );
    BOOST_CHECK_EQUAL( cs1.at( cs1.length() - 1 ), 'g' );

    BOOST_CHECK_EQUAL( common_layer::first_char()( cs1  ), 't' );
    BOOST_CHECK_EQUAL( common_layer::last_char()( cs1  ) , 'g' );
}


BOOST_AUTO_TEST_CASE( length_test )
{
    const_string cs1;

    BOOST_CHECK_EQUAL( cs1.length(), (size_t)0 );
    BOOST_CHECK( cs1.is_empty() );

    cs1 = "";
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)0 );
    BOOST_CHECK( cs1.is_empty() );

    cs1 = "test_string";
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)11 );

    cs1.erase();
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)0 );
    BOOST_CHECK_EQUAL( cs1.data(), "" );

    cs1 = const_string( "test_string", 4 );
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)4 );

    cs1.resize( 5 );
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)4 );

    cs1.resize( 3 );
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)3 );

    cs1.rshorten();
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)2 );
    BOOST_CHECK_EQUAL( cs1[(size_t)0], 't' );

    cs1.lshorten();
    BOOST_CHECK_EQUAL( cs1.length(), (size_t)1 );
    BOOST_CHECK_EQUAL( cs1[(size_t)0], 'e' );

    cs1.lshorten();
    BOOST_CHECK( cs1.is_empty() );
    BOOST_CHECK_EQUAL( cs1.data(), "" );

    cs1 = "test_string";
    cs1.lshorten( 11 );
    BOOST_CHECK( cs1.is_empty() );
    BOOST_CHECK_EQUAL( cs1.data(), "" );
}

BOOST_AUTO_TEST_CASE( asignment_test )
{
    const_string cs1;
    std::string  s( "test_string" );

    cs1 = "test";
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test" ), 0 );

    cs1 = s;
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );

    cs1.assign( "test" );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test" ), 0 );

    const_string cs2( "test_string" );

    cs1.swap( cs2 );
    BOOST_CHECK_EQUAL( std::strcmp( cs1.data(), "test_string" ), 0 );
    BOOST_CHECK_EQUAL( std::strcmp( cs2.data(), "test" ), 0 );
}

BOOST_AUTO_TEST_CASE( comparison_test )
{
    const_string cs1( "test_string" );
    const_string cs2( "test_string" );
    std::string  s( "test_string" );

    BOOST_CHECK_EQUAL( cs1, "test_string" );
    BOOST_CHECK_EQUAL( "test_string", cs1 );
    BOOST_CHECK_EQUAL( cs1, cs2 );
    BOOST_CHECK_EQUAL( cs1, s );
    BOOST_CHECK_EQUAL( s  , cs1 );

    cs1.resize( 4 );

    BOOST_CHECK( cs1 != "test_string" );
    BOOST_CHECK( "test_string" != cs1 );
    BOOST_CHECK( cs1 != cs2 );
    BOOST_CHECK( cs1 != s );
    BOOST_CHECK( s   != cs1 );

    BOOST_CHECK_EQUAL( cs1, "test" );
}

BOOST_AUTO_TEST_CASE( iterators_test )
{
    const_string cs1( "test_string" );
    std::string  s;

    std::copy( cs1.begin(), cs1.end(), std::back_inserter( s ) );
    BOOST_CHECK_EQUAL( cs1, s );

    s.erase();

    std::copy( cs1.rbegin(), cs1.rend(), std::back_inserter( s ) );
    BOOST_CHECK_EQUAL( const_string( s ), "gnirts_tset" );
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
    std::vector<std::string> args = {"const_string_test", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Running 6 test cases...

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
