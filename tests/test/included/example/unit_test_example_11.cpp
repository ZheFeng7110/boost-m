// Boost.Test example unit_test_example_11.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `unit_test_example_11 --color_output=no`,
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
//  (C) Copyright Gennadiy Rozental 2002-2014.
//  (C) Copyright Gennadiy Rozental & Ullrich Koethe 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
using namespace boost::unit_test;

// STL
#include <vector>
#include <string>

#define LOG_FATAL_ERROR( M )                                  \
BOOST_TEST_LOG_ENTRY( ::boost::unit_test::log_fatal_errors )  \
    << (::boost::unit_test::lazy_ostream::instance() << M)
//____________________________________________________________________________//

// this free function is invoked with all parameters specified in a list
void check_string( std::string const& s )
{
    // reports 'error in "check_string": test s.substr( 0, 3 ) == "hdr" failed [actual_value != hdr]'
    BOOST_CHECK_EQUAL( s.substr( 0, 3 ), "hdr" );
}

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/[] ) {
    framework::master_test_suite().p_name.value = "Unit test example 11";

    LOG_FATAL_ERROR( "something happened" );

    // parameters have no requirements to stay alive beyond the next statement
    std::string const params[] = { "hdr1 ", "hdr2", "3  " };

    framework::master_test_suite().add( 
        BOOST_PARAM_TEST_CASE( &check_string, (std::string const*)params, params+3 ) );

    return 0; 
}

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
    std::vector<std::string> args = {"unit_test_example_11", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(<src>: fatal error: in "Test setup": something happened
Running 3 test cases...
<src>: error: in "check_string_2": check s.substr( 0, 3 ) == "hdr" has failed [3   != hdr]

*** 1 failure is detected in the test module "Unit test example 11"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
