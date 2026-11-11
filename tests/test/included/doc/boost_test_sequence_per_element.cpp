// Boost.Test documentation example boost_test_sequence_per_element.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `boost_test_sequence_per_element --color_output=no` (the command line of boost_test_sequence_per_element.output),
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
#define BOOST_TEST_MODULE boost_test_sequence_per_element
#include <boost/test/included/unit_test.hpp>
#include <vector>
#include <list>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE( test_sequence_per_element )
{
  std::vector<int> a{1,2,3};
  std::vector<long> b{1,5,3};
  std::list<short> c{1,5,3,4};

  BOOST_TEST(a == b, tt::per_element()); // nok: a[1] != b[1]

  BOOST_TEST(a != b, tt::per_element()); // nok: a[0] == b[0] ...
  BOOST_TEST(a <= b, tt::per_element()); // ok
  BOOST_TEST(b  < c, tt::per_element()); // nok: size mismatch
  BOOST_TEST(b >= c, tt::per_element()); // nok: size mismatch
  BOOST_TEST(b != c, tt::per_element()); // nok: size mismatch
}

BOOST_AUTO_TEST_CASE( test_compare_c_arrays_element_wise )
{
  int a[] = {1, 2, 3};
  int b[] = {1, 5, 3};
  std::vector<long> c{1, 5, 3};
  BOOST_TEST(a == b, boost::test_tools::per_element());
  BOOST_TEST(a != b, boost::test_tools::per_element());
  BOOST_TEST(a < b, boost::test_tools::per_element());
  BOOST_TEST(b < c, boost::test_tools::per_element());
  BOOST_TEST(c < a, boost::test_tools::per_element());
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
    std::vector<std::string> args = {"boost_test_sequence_per_element", "--color_output=no"};
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
#if 1
    const std::string want = ex_harness::normalize(R"ex(Running 2 test cases...
<src>: error: in "test_sequence_per_element": check a == b has failed
  - mismatch at position 1: [2 == 5] is false
<src>: error: in "test_sequence_per_element": check a != b has failed
  - mismatch at position 0: [1 != 1] is false
  - mismatch at position 2: [3 != 3] is false
<src>: error: in "test_sequence_per_element": check b < c has failed
Collections size mismatch: 3 != 4
<src>: error: in "test_sequence_per_element": check b >= c has failed
Collections size mismatch: 3 != 4
<src>: error: in "test_sequence_per_element": check b != c has failed
Collections size mismatch: 3 != 4
<src>: error: in "test_compare_c_arrays_element_wise": check a == b has failed
  - mismatch at position 1: [2 == 5] is false
<src>: error: in "test_compare_c_arrays_element_wise": check a != b has failed
  - mismatch at position 0: [1 != 1] is false
  - mismatch at position 2: [3 != 3] is false
<src>: error: in "test_compare_c_arrays_element_wise": check a < b has failed
  - mismatch at position 0: [1 < 1] is false
  - mismatch at position 2: [3 < 3] is false
<src>: error: in "test_compare_c_arrays_element_wise": check b < c has failed
  - mismatch at position 0: [1 < 1] is false
  - mismatch at position 1: [5 < 5] is false
  - mismatch at position 2: [3 < 3] is false
<src>: error: in "test_compare_c_arrays_element_wise": check c < a has failed
  - mismatch at position 0: [1 < 1] is false
  - mismatch at position 1: [5 < 2] is false
  - mismatch at position 2: [3 < 3] is false

*** 10 failures are detected in the test module "boost_test_sequence_per_element"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
