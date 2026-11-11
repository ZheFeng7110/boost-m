// Boost.Test example unit_test_example_10.cpp (libs/test/example/), verbatim below #line 1
// "test.cpp" except that it includes the header-only form of what it uses
// (boost/test/included/...). The harness at the end runs it as `unit_test_example_10 --color_output=no`,
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
//  (C) Copyright Gennadiy Rozental & Ullrich Koethe 2001.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/test for the library home page.

// Boost.Test
#include <boost/test/tools/floating_point_comparison.hpp>
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test;
using boost::math::fpc::close_at_tolerance;
using boost::math::fpc::percent_tolerance;

// BOOST
#include <boost/lexical_cast.hpp>

// STL
#include <functional>
#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>

//____________________________________________________________________________//

struct account {
    account()
    : m_amount(0.0)
    {}

    void deposit(double amount) { m_amount += amount; }
    void withdraw(double amount)
    {
        if(amount > m_amount)
        {
            throw std::logic_error("You don't have that much money!");
        }
        m_amount -= amount;
    }
    double balance() const { return m_amount; }

private:
    double m_amount;
};

//____________________________________________________________________________//

struct account_test {
    account_test( double init_value ) { m_account.deposit( init_value ); }

    account m_account;  // a very simple fixture

    void test_init()
    {
        // different kinds of non-critical tests
        // they report the error and continue

        // standard assertion
        // reports 'error in "account_test::test_init": test m_account.balance() >= 0.0 failed' on error
        BOOST_CHECK( m_account.balance() >= 0.0 );

        // customized assertion
        // reports 'error in "account_test::test_init": Initial balance should be more then 1, was actual_value' on error
        BOOST_CHECK_MESSAGE( m_account.balance() > 1.0,
                             "Initial balance should be more then 1, was " << m_account.balance() );

        // equality assertion (not very wise idea use equality check on floating point values)
        // reports 'error in "account_test::test_init": test m_account.balance() == 5.0 failed [actual_value != 5]' on error
        BOOST_CHECK_EQUAL( m_account.balance(), 5.0 );

        // closeness assertion for floating-point numbers (symbol (==) used to mark closeness, (!=) to mark non closeness )
        // reports 'error in "account_test::test_init": test m_account.balance() (==) 10.0 failed [actual_value (!=) 10 (1e-010)]' on error
        BOOST_CHECK_CLOSE( m_account.balance(), 10.0, /* tolerance */ 1e-10 );
    }

    void test_deposit()
    {
        // these 2 statements just to show that usage manipulators doesn't hurt Boost.Test output
        std::cout << "Current balance: " << std::hex << (int)m_account.balance() << std::endl;
        std::cerr << "Current balance: " << std::hex << (int)m_account.balance() << std::endl;

        float curr_ballance = (float)m_account.balance();
        float deposit_value;

        std::cout << "Enter deposit value:\n";
        std::cin  >> deposit_value;

        m_account.deposit( deposit_value );

        // correct result validation; could fail due to rounding errors; use BOOST_CHECK_CLOSE instead
        // reports "test m_account.balance() == curr_ballance + deposit_value failed" on error
        BOOST_CHECK( m_account.balance() == curr_ballance + deposit_value );

        // different kinds of critical tests

        // reports 'fatal error in "account_test::test_deposit": test m_account.balance() >= 100.0 failed' on error
        BOOST_REQUIRE( m_account.balance() >= 100.0 );

        // reports 'fatal error in "account_test::test_deposit": Balance should be more than 500.1, was actual_value' on error
        BOOST_REQUIRE_MESSAGE( m_account.balance() > 500.1,
                               "Balance should be more than 500.1, was " << m_account.balance());

        // reports 'fatal error in "account_test::test_deposit": test std::not_equal_to<double>()(m_account.balance(), 999.9) failed
        //          for (999.9, 999.9)' on error
        BOOST_REQUIRE_PREDICATE( std::not_equal_to<double>(), (m_account.balance())(999.9) );

        // reports 'fatal error in "account_test::test_deposit": test close_at_tolerance<double>( 1e-9 )( m_account.balance(), 605.5)
        //          failed for (actual_value, 605.5)
        BOOST_REQUIRE_PREDICATE( close_at_tolerance<double>( percent_tolerance( 1e-9 ) ),
                                 (m_account.balance())(605.5) );
    }

    void test_withdraw()
    {
        float curr_ballance = (float)m_account.balance();

        m_account.withdraw(2.5);

        // correct result validation; could fail due to rounding errors; use BOOST_CHECK_CLOSE instead
        // reports "test m_account.balance() == curr_ballance - 2.5 failed" on error
        BOOST_CHECK( m_account.balance() == curr_ballance - 2.5 );

        // reports 'error in "account_test::test_withdraw": exception std::runtime_error is expected' on error
        BOOST_CHECK_THROW( m_account.withdraw( m_account.balance() + 1 ), std::runtime_error );

    }
};

//____________________________________________________________________________//

struct account_test_suite : public test_suite {
    account_test_suite( double init_value ) : test_suite("account_test_suite") {
        // add member function test cases to a test suite
        boost::shared_ptr<account_test> instance( new account_test( init_value ) );

        test_case* init_test_case     = BOOST_CLASS_TEST_CASE( &account_test::test_init, instance );
        test_case* deposit_test_case  = BOOST_CLASS_TEST_CASE( &account_test::test_deposit, instance );
        test_case* withdraw_test_case = BOOST_CLASS_TEST_CASE( &account_test::test_withdraw, instance );

        deposit_test_case->depends_on( init_test_case );
        withdraw_test_case->depends_on( deposit_test_case );

        add( init_test_case, 1 );
        add( deposit_test_case, 1 );
        add( withdraw_test_case );
    }
};

//____________________________________________________________________________//

test_suite*
init_unit_test_suite( int argc, char * argv[] ) {
    framework::master_test_suite().p_name.value = "Unit test example 10";

    try {
        if( argc < 2 )
            throw std::logic_error( "Initial deposit expected" );

        framework::master_test_suite().add( new account_test_suite( boost::lexical_cast<double>( argv[1] ) ) );
    }
    catch( boost::bad_lexical_cast const& ) {
         throw std::logic_error( "Initial deposit value should match format of double" );
    }

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
    std::vector<std::string> args = {"unit_test_example_10", "--color_output=no"};
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
    const std::string want = ex_harness::normalize(R"ex(Test setup error: std::logic_error: Initial deposit expected
)ex"), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
#endif
