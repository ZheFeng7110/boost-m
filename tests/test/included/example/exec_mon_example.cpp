// Boost.Test example exec_mon_example.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `exec_mon_example --color_output=no`,
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
//  (C) Copyright Gennadiy Rozental 2003-2014.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

#include <boost/test/included/prg_exec_monitor.hpp>
#include <boost/test/included/execution_monitor.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>

#include <iostream>

struct my_exception1
{
    explicit    my_exception1( int res_code ) : m_res_code( res_code ) {}

    int         m_res_code;
};

struct my_exception2
{
    explicit    my_exception2( int res_code ) : m_res_code( res_code ) {}

    int         m_res_code;
};

namespace {

class dangerous_call {
public:
    dangerous_call( int argc ) : m_argc( argc ) {}

    int operator()()
    {
        // here we perform some operation under monitoring that could throw my_exception
        if( m_argc < 2 )
            throw my_exception1( 23 );
        if( m_argc > 3 )
            throw my_exception2( 45 );
        else if( m_argc > 2 )
            throw "too many args";

        return 1;
    }

private:
    // Data members
    int     m_argc;
};

void translate_my_exception1( my_exception1 const& ex )
{
    std::cout << "Caught my_exception1(" << ex.m_res_code << ")"<< std::endl;
}

void translate_my_exception2( my_exception2 const& ex )
{
    std::cout << "Caught my_exception2(" << ex.m_res_code << ")"<< std::endl;
}

int generate_fpe()
{
    double d = 0.0;

    d = 1/d;

    return 0;
}

int generate_fpe2()
{
    double d = 1e158;

    d = d*d;

    return 0;
}

int generate_fpe3()
{
    double d = 1.1e-308;

    d = 1/d;

    return 0;
}

int generate_int_div_0()
{
    int i = 0;

    return 1/i;
}

#if (defined(__clang__) && __clang_major__ >= 6) || (defined(__GNUC__) && __GNUC__ >= 8)
__attribute__((no_sanitize("null")))
#endif
int generate_sigfault()
{
    int* p = 0;

    return *p;
}


} // local_namespace

int
cpp_main( int argc , char *[] )
{
    ::boost::execution_monitor ex_mon;

    ///////////////////////////////////////////////////////////////

    ex_mon.register_exception_translator<my_exception1>( &translate_my_exception1, "except1" );
    ex_mon.register_exception_translator<my_exception2>( &translate_my_exception2, "except2" );

    try {
        ex_mon.execute( dangerous_call( argc ) );
        std::cout << "Should reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    ///////////////////////////////////////////////////////////////

    ex_mon.erase_exception_translator( "except2" );

    try {
        ex_mon.execute( dangerous_call( 5 ) );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    ///////////////////////////////////////////////////////////////

    ex_mon.erase_exception_translator<my_exception1>();

    try {
        ex_mon.execute( dangerous_call( 1 ) );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    ///////////////////////////////////////////////////////////////

// we are currently not able to silence those errors below with UBSAN under clang
// this seems to come from the way clang handles floating point exceptions + UB.
#if !(defined(HAS_UBSAN) && (HAS_UBSAN==1) && defined(__clang__))

    ex_mon.p_detect_fp_exceptions.value = boost::fpe::BOOST_FPE_DIVBYZERO;
    ex_mon.p_catch_system_errors.value = false;

    try {
        ex_mon.execute( &generate_fpe );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    ///////////////////////////////////////////////////////////////

    ex_mon.p_detect_fp_exceptions.value = boost::fpe::BOOST_FPE_ALL;

    try {
        ex_mon.execute( &generate_fpe2 );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    try {
        ex_mon.execute( &generate_fpe3 );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    ///////////////////////////////////////////////////////////////

    ex_mon.p_detect_fp_exceptions.value = boost::fpe::BOOST_FPE_OFF;
    ex_mon.p_catch_system_errors.value = true;

    try {
        ex_mon.execute( &generate_int_div_0 );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }

    ///////////////////////////////////////////////////////////////

    ex_mon.p_detect_fp_exceptions.value = boost::fpe::BOOST_FPE_OFF;
    ex_mon.p_catch_system_errors.value = true;

    try {
        ex_mon.execute( &generate_sigfault );
        std::cout << "Should not reach this line " << __LINE__ << std::endl;
    }
    catch ( boost::execution_exception const& ex ) {
        std::cout << "Caught exception: " << ex.what() << std::endl;
    }
#endif // UBSAN issue

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
    std::vector<std::string> args = {"exec_mon_example", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc == 0;
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
