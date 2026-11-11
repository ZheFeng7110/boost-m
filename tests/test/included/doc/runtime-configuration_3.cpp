// Boost.Test documentation example runtime-configuration_3.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `runtime-configuration_3 --log_level=all -- --create-parametrized 3 --color_output=no` (the command line of runtime-configuration_3.output),
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
//  Copyright (c) 2019 Raffi Enficiaud
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_ALTERNATIVE_INIT_API
#include <boost/test/included/unit_test.hpp>
#include <functional>
#include <sstream>

using namespace boost::unit_test;

void test_function(int i) {
  BOOST_TEST(i >= 1);
}

// helper
int read_integer(const std::string &str) {
  std::istringstream buff( str );
  int number = 0;
  buff >> number;
  if(buff.fail()) {
    // it is also possible to raise a boost.test specific exception.
    throw framework::setup_error("Argument '" + str + "' not integer");
  }
  return number;
}

bool init_unit_test()
{
  int argc = boost::unit_test::framework::master_test_suite().argc;
  char** argv = boost::unit_test::framework::master_test_suite().argv;

  if( argc <= 1) {
      return false; // returning false to indicate an error
  }

  if( std::string(argv[1]) == "--create-parametrized" ) {
    if(argc < 3) {
      // the logging availability depends on the logger type
      BOOST_TEST_MESSAGE("Not enough parameters");
      return false;
    }

    int number_tests = read_integer(argv[2]);
    int test_start = 0;
    if(argc > 3) {
        test_start = read_integer(argv[3]);
    }

    for(int i = test_start; i < number_tests; i++) {
        std::ostringstream ostr;
        ostr << "name " << i;
        // create test-cases, avoiding duplicate names
        framework::master_test_suite().
            add( BOOST_TEST_CASE_NAME( std::bind(&test_function, i), ostr.str().c_str() ) );
    }
  }
  return true;
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
    std::vector<std::string> args = {"runtime-configuration_3", "--log_level=all", "--", "--create-parametrized", "3", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Test setup error: boost::unit_test::framework::setup_error: Argument '--color_output=no' not integer
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
