// Boost.Test example named_param_example.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `named_param_example --color_output=no`,
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

// Library Code
#include <boost/test/utils/named_params.hpp>

using namespace boost::nfp;

////////////////////////////////////////////////////////////////
// Example:

#include <iostream>
#include <boost/shared_ptr.hpp>

namespace test {
  typed_keyword<char const*,struct name_t>  name;
  typed_keyword<int,struct test_index_t>    index;
  keyword<struct value_t,true>              value;
  keyword<struct instance_t,true>           instance;
  keyword<struct ref_t>                     ref;

  template<typename ValueType>
  void foo1( char const* n, ValueType v, int i )
  {
      std::cout << n << '[' << i << "]=" << v << std::endl;
  }

  template<class Params>
  void foo(Params const& params)
  {
      int i = params[index];
      foo1( params[name], params[value], i );
  }

  template<class Params>
  void boo(Params const& params)
  {
      foo1( params[name], params[value], params.has(index) ? params[index] : 0 );
  }

  template<class Params>
  void doo(Params const& params)
  {
      char const* nm;
      if( params.has(name) )
          nm = params[name];
      else
          nm = "abc";
      foo1( nm, params[value], params.has(index) ? params[index] : 0 );
  }

  template<typename T>
  void moo1( T* t )
  {
      std::cout << "non shared " << *t << std::endl;
  }

  template<typename T>
  void moo1( boost::shared_ptr<T> const& t )
  {
      std::cout << "shared " << *t << std::endl;
  }

  template<class Params>
  void moo(Params const& params)
  {
      moo1( params[instance] );
  }

  template<class Params>
  void goo(Params const& params)
  {
      params[ref] = 6;
  }
}

int main()
{
   using test::foo;
   using test::boo;
   using test::moo;
   using test::doo;
   using test::goo;
   using test::name;
   using test::value;
   using test::index;
   using test::instance;
   using test::ref;

   foo(( name = "foo", index = 0, value = 2.5 ));
   foo(( value = 'a', index = 1, name = "foo" ));
   foo(( name = "foo", value = "abc", index = 1 ));

   try {
       foo(( name = "foo", value = "abc" ));
   }
   catch( nfp_detail::access_to_invalid_parameter const& ) {
       std::cout << "Got access_to_invalid_parameter" << std::endl;
   }

   boo(( name = "boo", value = "abc" ));
   boo(( name = "boo", index = 1, value = "abc" ));
   doo(( value = "abc" ));
   doo(( value = 1.56, name = "ytr" ));

   int i = 5;

   moo( instance = &i );
   moo( instance = boost::shared_ptr<float>( new float(1.2) ) );

   goo( ref = i );

   return 0;
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
    std::vector<std::string> args = {"named_param_example", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main();
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
    const std::string want = ex_harness::normalize(R"ex(foo[0]=2.5
foo[1]=a
foo[1]=abc
Got access_to_invalid_parameter
boo[0]=abc
boo[1]=abc
abc[0]=abc
ytr[0]=1.56
non shared 5
shared 1.2
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
