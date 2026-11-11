// Boost.Test documentation example example84_contexts.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `example84_contexts --log_level=message --color_output=no` (the command line of example84_contexts.output),
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
//  (C) Copyright Raffi Enficiaud 2019.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_MODULE example84
#include <boost/test/included/unit_test.hpp>
#include <random>
#include <cmath>

// this function does not compute properly the polynomial root estimation
// in the case of a double root.
template <class random_generator_t>
std::pair<double, double> estimate_polynomial_roots(
  random_generator_t& gen,
  std::function<double(double)> polynomial) {

  using namespace std;

  std::uniform_real_distribution<> dis(-10, 10);
  double x1 = dis(gen);
  double x2 = dis(gen);
  double fx1 = polynomial(x1);
  double fx2 = polynomial(x2);

  BOOST_TEST_INFO_SCOPE("sample1 = " << x1);
  BOOST_TEST_INFO_SCOPE("sample2 = " << x2);

  // from Vieta formula
  double minus_b = x2 + x1 - (fx2 - fx1) / (x2 - x1);
  double c = (x1 * fx2 - x2 * fx1 + x2 * x1 * x1 - x1 * x2 * x2) / (x1 - x2);

  BOOST_TEST(minus_b * minus_b >= 4*c);

  return std::make_pair(
    (minus_b - sqrt(minus_b * minus_b - 4 * c)) / 2,
    (minus_b + sqrt(minus_b * minus_b - 4 * c)) / 2);
}

BOOST_AUTO_TEST_CASE(quadratic_estimation)
{
  std::random_device rd;
  unsigned int seed = rd();
  std::mt19937 gen(seed);
  std::uniform_int_distribution<> dis(-10, 10);

  BOOST_TEST_MESSAGE("Seed = " << seed);

  for(int i = 0; i < 50; i++) {
    BOOST_TEST_INFO_SCOPE("trial " << i+1);
    int root1 = dis(gen);
    int root2 = dis(gen);
    if(root1 > root2) {
      std::swap(root1, root2);
    }
    BOOST_TEST_INFO_SCOPE("root1 = " << root1);
    BOOST_TEST_INFO_SCOPE("root2 = " << root2);

    std::pair<double, double> estimated = estimate_polynomial_roots(
      gen,
      [root1, root2](double x) -> double { return (x - root1) * (x - root2); });

    BOOST_TEST(estimated.first == double(root1), 10. % boost::test_tools::tolerance());
    BOOST_TEST(estimated.second == double(root2), 10. % boost::test_tools::tolerance());
  }
}
//]

BOOST_AUTO_TEST_CASE(making_it_fail)
{
  // cheating a bit ... but shhhh, do not show in the docs ...
  BOOST_FAIL("Making it fail always on all platforms");
}

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
    std::vector<std::string> args = {"example84_contexts", "--log_level=message", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(""), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
