// Boost.Test documentation example runtime-configuration_4.run-fail.cpp (libs/test/doc/examples/), verbatim
// below #line 1 "test.cpp" (its documented output names that file). The
// harness at the end runs it as `runtime-configuration_4 test_file.txt --color_output=no` (the command line of runtime-configuration_4.output),
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
#define BOOST_TEST_MODULE runtime_configuration4

#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <iostream>
#include <functional>
#include <sstream>
#include <fstream>

// this dataset loads a file that contains a list of strings
// this list is used to create a dataset test case.
class file_dataset
{
private:
    std::string m_filename;
    std::size_t m_line_start;
    std::size_t m_line_end;

public:
    static const int arity = 2;

public:
    file_dataset(std::size_t line_start = 0, std::size_t line_end = std::size_t(-1))
    : m_line_start(line_start)
    , m_line_end(line_end)
    {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;

      if(argc != 3)
        throw std::logic_error("Incorrect number of arguments");
      if(std::string(argv[1]) != "--test-file")
        throw std::logic_error("First argument != '--test-file'");
      if(!(m_line_start < std::size_t(-1)))
        throw std::logic_error("Incorrect line start/end");

      m_filename = argv[2];

      std::ifstream file(m_filename);
      if(!file.is_open())
        throw std::logic_error("Cannot open the file '" + m_filename + "'");
      std::size_t nb_lines = std::count_if(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>(),
        [](char c){ return c == '\n';});

      m_line_end = (std::min)(nb_lines, m_line_end);
      if(!(m_line_start <= m_line_end))
        throw std::logic_error("Incorrect line start/end");
    }

    struct iterator {
        iterator(std::string const& filename, std::size_t line_start)
        : file(filename, std::ios::binary) {
          if(!file.is_open())
            throw std::runtime_error("Cannot open the file");
          for(std::size_t i = 0; i < line_start; i++) {
            getline(file, m_current_line);
          }
        }

        auto operator*() const -> std::tuple<float, float> {
          float a, b;
          std::istringstream istr(m_current_line);
          istr >> a >> b;
          return std::tuple<float, float>(a, b);
        }

        void operator++() {
          getline(file, m_current_line);
        }
    private:
        std::ifstream file;
        std::string m_current_line;
    };

    // size of the DS
    boost::unit_test::data::size_t size() const {
      return m_line_end - m_line_start;
    }

    // iterator over the lines of the file
    iterator begin() const   {
      return iterator(m_filename, m_line_start);
    }
};

namespace boost { namespace unit_test { namespace data {

namespace monomorphic {
  template <>
  struct is_dataset<file_dataset> : boost::mpl::true_ {};
}
}}}

BOOST_DATA_TEST_CASE(command_line_test_file,
    boost::unit_test::data::make_delayed<file_dataset>( 3, 10 ),
    input, expected) {
    BOOST_TEST(input <= expected);
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
    static const std::regex ansi("\x1b\\[[0-9;]*m");  // colour codes some programs turn on
    s = std::regex_replace(s, ansi, "");
    static const std::regex loc(R"(([A-Za-z0-9_./\-]+\.(cpp|hpp|ipp)|unknown location)(\(([0-9]+)\)|:([0-9]+)):)");
    s = std::regex_replace(s, loc, "<src>:");
    // the system's description of a memory fault (POSIX: "at address: A: no
    // mapping ..." / "invalid permissions"; Windows: "occurred at address A, while ...")
    static const std::regex fault(R"(memory access violation (?:occurred )?at address:? (0x[0-9a-fA-F]+)[,:][^\n]*)");
    s = std::regex_replace(s, fault, "memory access violation at address $1: <fault>");
    // MSVC's type names carry the class-key ("class std::runtime_error")
    static const std::regex key(R"((: |error: )(?:class|struct) )");
    s = std::regex_replace(s, key, "$1");
    // trailing blanks and blank lines are not part of what is compared
    static const std::regex trailing(R"([ \t]+\n)");
    s = std::regex_replace(s, trailing, "\n");
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    while (!s.empty() && s.front() == '\n') s.erase(0, 1);
    return s;
}
}
int main() {
    std::vector<std::string> args = {"runtime-configuration_4", "test_file.txt", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc != 0;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Boost.Test WARNING: token "test_file.txt" does not correspond to the Boost.Test argument
                    and should be placed after all Boost.Test arguments and the -- separator.
                    For example: runtime-configuration_4 --random -- test_file.txt
Test setup error: Incorrect number of arguments
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
