// boost.test smoke — compiled Unit Test Framework form (the official
// recommended configuration). boost.test is compiled include-only — the
// unit-test-framework feature ships the framework TUs but NO module; there is
// no `import boost.test`. The consumer TU owns main + the unit_test_main
// runner: BOOST_TEST_NO_MAIN pulls the runner without its main wrapper,
// BOOST_TEST_MODULE enables the auto-registration init glue
// (unit_test_suite.hpp) and the BOOST_TEST_* macros stay include-side.
//
// This project (tests/test/utf/mcpp.toml) always enables the feature, so the
// file no longer switches on MCPP_FEATURE_UNIT_TEST_FRAMEWORK; the
// header-only form lives in the root project (tests/test/included/), and the
// two never link together.
#define BOOST_TEST_MODULE boost_test_module_smoke
#define BOOST_TEST_NO_MAIN
#include <boost/test/unit_test.hpp>
#include <boost/test/impl/unit_test_main.ipp>

BOOST_AUTO_TEST_CASE(arithmetic) {
    BOOST_TEST(1 + 1 == 2);
    BOOST_CHECK_EQUAL(3 * 3, 9);
}

BOOST_AUTO_TEST_CASE(strings) {
    std::string s = "boost";
    BOOST_TEST(s == "boost");
}

int main(int argc, char* argv[]) {
    return boost::unit_test::unit_test_main(&init_unit_test_suite, argc, argv);
}
