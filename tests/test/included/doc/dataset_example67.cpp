// Boost.Test documentation example dataset_example67.run.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `dataset_example67 --color_output=no` (the command line of dataset_example67.output),
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
//  (C) Copyright Raffi Enficiaud 2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_MODULE example67
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <sstream>

namespace bdata = boost::unit_test::data;

// Generates a Fibonacci sequence
std::vector<float> fibonacci() {
  std::vector<float> ret(8);
  ret[0] = 0;
  ret[1] = 1;
  
  for(std::size_t s(2); s < ret.size(); s++)
  {
    ret[s] = ret[s-1] + ret[s-2];
  }
  return ret;
}

BOOST_DATA_TEST_CASE( 
  test1, 
  bdata::make(fibonacci()),
  array_element)
{
  std::cout << "test 1: " 
    << array_element 
    << std::endl;
  BOOST_TEST(array_element <= 13);
}


// Generates a map from a vector
std::map<std::string, float> vect_2_str(std::vector<float> v) 
{
  std::map<std::string, float> out;
  for(std::size_t s(0); s < v.size(); s++)
  {
    std::ostringstream o;
    o << v[s];
    out[o.str()] = v[s];
  }
  return out;
}

typedef std::pair<const std::string, float> pair_map_t;
BOOST_TEST_DONT_PRINT_LOG_VALUE( pair_map_t )

BOOST_DATA_TEST_CASE( 
  test2, 
  bdata::make(vect_2_str(fibonacci())),
  array_element)
{
  std::cout << "test 2: \"" 
    << array_element.first << "\", "
    << array_element.second
    << std::endl;
  BOOST_TEST(array_element.second <= 13);
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
    std::vector<std::string> args = {"dataset_example67", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = false ? rc != 0 : rc == 0;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 15 test cases...
test 1: 0
test 1: 1
test 1: 1
test 1: 2
test 1: 3
test 1: 5
test 1: 8
test 1: 13
test 2: "0", 0
test 2: "1", 1
test 2: "13", 13
test 2: "2", 2
test 2: "3", 3
test 2: "5", 5
test 2: "8", 8

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
