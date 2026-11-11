// Boost.Test example unit_test_example_15.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `unit_test_example_15 --color_output=no`,
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
//
// ***************************************************************************

// Boost.Test
#define BOOST_TEST_MODULE data driven test example
#include <boost/test/included/unit_test.hpp>

#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
namespace data=boost::unit_test::data;
namespace bt=boost::unit_test;

//____________________________________________________________________________//

double x_samples[] = {1.1,2.1,3.1,4.1};
double y_samples[] = {10.1,9.1,8.1};

auto& D = * bt::tolerance(1e-1);
BOOST_DATA_TEST_CASE( data_driven_test, data::make(x_samples) * y_samples, x, y )
{
    BOOST_TEST( x*y < 32.4 );
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
    std::vector<std::string> args = {"unit_test_example_15", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Running 12 test cases...
<src>: error: in "data_driven_test/_6": check x*y < 32.4 has failed [3.1000000000000001 * 10.1 >= 32.399999999999999]: operation != on arguments yields 'false': relative difference is within tolerance [0.033642 < 0.1].
Failure occurred in a following context:
    x = 3.1000000000000001; y = 10.1;
<src>: error: in "data_driven_test/_9": check x*y < 32.4 has failed [4.0999999999999996 * 10.1 >= 32.399999999999999]: operation <  on arguments yields 'false'.
Failure occurred in a following context:
    x = 4.0999999999999996; y = 10.1;
<src>: error: in "data_driven_test/_10": check x*y < 32.4 has failed [4.0999999999999996 * 9.0999999999999996 >= 32.399999999999999]: operation <  on arguments yields 'false'.
Failure occurred in a following context:
    x = 4.0999999999999996; y = 9.0999999999999996;
<src>: error: in "data_driven_test/_11": check x*y < 32.4 has failed [4.0999999999999996 * 8.0999999999999996 >= 32.399999999999999]: operation <  on arguments yields 'false'.
Failure occurred in a following context:
    x = 4.0999999999999996; y = 8.0999999999999996;

*** 4 failures are detected in the test module "data driven test example"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
