// Boost.Test documentation example exception_api.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `exception_api --log_level=all --color_output=no` (the command line of exception_api.output),
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
//  (C) Copyright 2015 Boost.Test team.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

//[example_code
#define BOOST_TEST_MODULE example
#include <boost/test/included/unit_test.hpp>
#include <stdexcept>
#include <fstream>

//! Computes the histogram of the words in a text file
class FileWordHistogram
{
public:
  //!@throw std::exception if the file does not exist
  FileWordHistogram(std::string filename) : is_processed(false), fileStream_(filename) {
    if(!fileStream_.is_open()) throw std::runtime_error("Cannot open the file");
  }

  //! @returns true on success, false otherwise
  bool process() {
    if(is_processed) return true;

    // ...
    is_processed = true;
    return true;
  }

  //!@pre process has been called with status success
  //!@throw std::logic_error if preconditions not met
  std::map<std::string, std::size_t>
  result() const {
    if(!is_processed)
      throw std::runtime_error("\"process\" has not been called or was not successful");
    return histogram;
  }

private:
  bool is_processed;
  std::ifstream fileStream_;
  std::map<std::string, std::size_t> histogram;
};

BOOST_AUTO_TEST_CASE( test_throw_behaviour )
{
  // __FILE__ is accessible, no exception expected
  BOOST_REQUIRE_NO_THROW( FileWordHistogram(__FILE__) );

  // ".. __FILE__" does not exist, API says std::exception, and implementation
  // raises std::runtime_error child of std::exception
  BOOST_CHECK_THROW( FileWordHistogram(".." __FILE__), std::exception );

  {
    FileWordHistogram instance(__FILE__);

    // api says "std::logic_error", implementation is wrong.
    // std::runtime_error not a child of std::logic_error, not intercepted
    // here.
    BOOST_CHECK_THROW(instance.result(), std::logic_error);
  }
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
    std::vector<std::string> args = {"exception_api", "--log_level=all", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Running 1 test case...
Entering test module "example"
<src>: Entering test case "test_throw_behaviour"
<src>: fatal error: in "test_throw_behaviour": unexpected exception thrown by FileWordHistogram("test.cpp")
<src>: Leaving test case "test_throw_behaviour"; testing time: <t>
Leaving test module "example"; testing time: <t>

*** 1 failure is detected in the test module "example"
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
