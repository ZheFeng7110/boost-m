// The test macros of Boost.JSON's own test suite (libs/json/test/
// test_suite.hpp) mapped to doctest, so the documentation's code files
// (libs/json/test/{snippets,doc_*}.cpp) run unchanged: a
// TEST_SUITE(type, name) becomes a doctest case that calls type().run().
#ifndef BOOST_M_TESTS_JSON_DOC_TEST_SUITE_HPP
#define BOOST_M_TESTS_JSON_DOC_TEST_SUITE_HPP

#define BOOST_TEST(expr) CHECK(expr)
#define BOOST_TEST_NOT(expr) CHECK_FALSE(expr)
#define BOOST_TEST_EQ(a, b) CHECK_EQ(a, b)
#define BOOST_TEST_NE(a, b) CHECK_NE(a, b)
#define BOOST_TEST_PASS() ((void)0)
#define TEST_SUITE(type, name) \
    TEST_CASE("doc: " name) { type().run(); }

#endif
