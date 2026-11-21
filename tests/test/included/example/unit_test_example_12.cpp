// Boost.Test example unit_test_example_12.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `unit_test_example_12 --color_output=no`,
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

// Boost.Test
#include <boost/test/included/unit_test.hpp>
#include <boost/test/utils/algorithm.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/parameterized_test.hpp>
using namespace boost::unit_test;

// BOOST
#include <boost/functional.hpp>
#include <boost/static_assert.hpp>
#include <boost/mem_fn.hpp>
#include <boost/bind/bind.hpp>

// STL
#include <string>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <list>

//____________________________________________________________________________//

template<int n>
struct power_of_10 {
    BOOST_STATIC_CONSTANT( unsigned long, value = 10*power_of_10<n-1>::value );
};

template<>
struct power_of_10<0> {
    BOOST_STATIC_CONSTANT( unsigned long, value = 1 );
};

//____________________________________________________________________________//

template<int AlphabetSize>
class hash_function {
public:
    BOOST_STATIC_ASSERT( AlphabetSize <= 5 );

    explicit        hash_function( std::string const& alphabet )
    : m_alphabet( alphabet )
    {
        if( m_alphabet.size() != AlphabetSize )
            throw std::runtime_error( "Wrong alphabet size" );

        std::sort( m_alphabet.begin(), m_alphabet.end() );

        if( std::adjacent_find( m_alphabet.begin(), m_alphabet.end() ) != m_alphabet.end() )
            throw std::logic_error( "Duplicate characters in alphabet" );
    }

    unsigned long   operator()( std::string const& arg )
    {
        m_result = 0;

        if( arg.length() > 8 )
            throw std::runtime_error( "Wrong argument size" );

        std::string::const_iterator it = std::find_if( arg.begin(), arg.end(),
            BOOST_TEST_BIND1ST( boost::mem_fun( &hash_function::helper_ ), this ) );

        if( it != arg.end() )
            throw std::out_of_range( std::string( "Invalid character " ) + *it );

        return m_result;
    }

private:
    bool            helper_( char c )
    {
        std::string::const_iterator it = std::find( m_alphabet.begin(), m_alphabet.end(), c );

        if( it == m_alphabet.end() )
            return true;

        m_result += power_of_10_( it - m_alphabet.begin() );

        return false;
    }

    unsigned long   power_of_10_( int i ) {
        switch( i ) {
        case 0: return power_of_10<0>::value;
        case 1: return power_of_10<1>::value;
        case 2: return power_of_10<2>::value;
        case 3: return power_of_10<3>::value;
        case 4: return power_of_10<4>::value;
        default: return 0;
        }
    }

    // Data members
    std::string     m_alphabet;
    unsigned long   m_result;
};

//____________________________________________________________________________//

struct hash_function_test_data {
    std::string     orig_string;
    unsigned long   exp_value;

    friend std::istream& operator>>( std::istream& istr, hash_function_test_data& test_data )
    {
        std::istream& tmp = istr >> test_data.orig_string;
        return  !tmp ? tmp : istr >> test_data.exp_value;
    }
};

//____________________________________________________________________________//

class hash_function_tester {
public:
    explicit        hash_function_tester( std::string const& alphabet )
    : m_function_under_test( alphabet ) {}

    void            test( hash_function_test_data const& test_data )
    {
        if( test_data.exp_value == (unsigned long)-1 )
            BOOST_CHECK_THROW( m_function_under_test( test_data.orig_string ), std::runtime_error );
        else if( test_data.exp_value == (unsigned long)-2 )
            BOOST_CHECK_THROW( m_function_under_test( test_data.orig_string ), std::out_of_range );
        else {
            BOOST_TEST_MESSAGE( "Testing: " << test_data.orig_string );
            BOOST_CHECK_EQUAL( m_function_under_test( test_data.orig_string ), test_data.exp_value );
        }
    }

private:
    hash_function<4> m_function_under_test;
};

//____________________________________________________________________________//

struct massive_hash_function_test : test_suite {
    massive_hash_function_test() : test_suite( "massive_hash_function_test" ) {
        std::string alphabet;
        std::cout << "Enter alphabet (4 characters without delimeters)\n";
        std::cin >> alphabet;

        boost::shared_ptr<hash_function_tester> instance( new hash_function_tester( alphabet ) );

        std::cout << "\nEnter test data in a format [string] [value] to check correct calculation\n";
        std::cout << "Enter test data in a format [string] -1 to check long string validation\n";
        std::cout << "Enter test data in a format [string] -2 to check invalid argument string validation\n";

        std::list<hash_function_test_data> test_data_store;

        while( !std::cin.eof() ) {
            hash_function_test_data test_data;

            if( !(std::cin >> test_data) )
                break;

            test_data_store.push_back( test_data );
        }

        add( make_test_case( &hash_function_tester::test,
                             "hash_function_tester",
                             __FILE__,
                             __LINE__,
                             instance,
                             test_data_store.begin(),
                             test_data_store.end() ) );
    }
};

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int, char* [] ) {
    framework::master_test_suite().p_name.value = "Unit test example 12";

    framework::master_test_suite().add( new massive_hash_function_test );

    return 0;
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
    std::vector<std::string> args = {"unit_test_example_12", "--color_output=no"};
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = ex_original_main(int(args.size()), argv.data());
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = rc == 200;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if 1
    const std::string want = ex_harness::normalize(R"ex(Enter alphabet (4 characters without delimeters)
Test setup error: std::runtime_error: Wrong alphabet size
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
