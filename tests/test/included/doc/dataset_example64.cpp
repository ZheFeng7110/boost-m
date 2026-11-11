// Boost.Test documentation example dataset_example64.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `dataset_example64 --color_output=no` (the command line of dataset_example64.output),
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
#define BOOST_TEST_MODULE dataset_example64
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

namespace bdata = boost::unit_test::data;


BOOST_DATA_TEST_CASE( 
  test1, 
  bdata::xrange(2) * bdata::xrange(3), 
  xr1, xr2)
{
  std::cout << "test 1: " << xr1 << ", " << xr2 << std::endl;
  BOOST_TEST((xr1 <= 2 && xr2 <= 3));
}

BOOST_DATA_TEST_CASE( 
  test2, 
  bdata::xrange(3)
  *
  ( bdata::random( 
      bdata::distribution=std::uniform_real_distribution<float>(1, 2)) 
    ^ bdata::xrange(2) 
  ),  
  xr, random_sample, index)
{
  std::cout << "test 2: " 
    << xr << " / " 
    << random_sample << ", " << index
    << std::endl;
  BOOST_TEST(random_sample < 1.7); // 30% chance of failure
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
    std::vector<std::string> args = {"dataset_example64", "--color_output=no"};
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
#if 0
    const std::string want = ex_harness::normalize(R"ex(Running 12 test cases...
test 1: 0, 0
test 1: 0, 1
test 1: 0, 2
test 1: 1, 0
test 1: 1, 1
test 1: 1, 2
test 2: 0 / 1.00002, 0
test 2: 0 / 1.08503, 1
test 2: 1 / 1.60135, 0
test 2: 1 / 1.89161, 1
<src>: error: in "test2/_3": check random_sample < 1.7 has failed [1.89161134 >= 1.7]
Failure occurred in a following context:
    xr = 1; random_sample = 1.89161134; index = 1;
test 2: 2 / 1.96796, 0
<src>: error: in "test2/_4": check random_sample < 1.7 has failed [1.96795571 >= 1.7]
Failure occurred in a following context:
    xr = 2; random_sample = 1.96795571; index = 0;
test 2: 2 / 1.18969, 1

*** 2 failures are detected in the test module "dataset_example64"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
