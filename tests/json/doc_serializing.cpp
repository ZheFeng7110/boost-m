// mcpplibs.mwrap.boost.json — libs/json/test/doc_serializing.cpp, the code the
// documentation pages include, unchanged except that its includes are imports
// and its test macros come from doc_test_suite.hpp (doctest).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "doc_test_suite.hpp"
#include <cassert>
#include <cmath>
#include <boost/config.hpp>
#include <boost/core/detail/static_assert.hpp>
import std;
import mcpplibs.mwrap.boost.json;
import mcpplibs.mwrap.boost.container;

namespace boost {
namespace json {

//----------------------------------------------------------

static void set1() {

//----------------------------------------------------------
{
// tag::doc_serializing_1[]
value jv = { 1, 2, 3, 4, 5 };

std::cout << jv << "\n";
// end::doc_serializing_1[]
}
//----------------------------------------------------------
{
// tag::doc_serializing_2[]
value jv = { 1, 2, 3, 4, 5 };

std::string s = serialize( jv );
// end::doc_serializing_2[]
}
//----------------------------------------------------------
{
//[doc_serializing_5
//]
}

} // set1

class doc_serializing_test
{
public:
    void
    run()
    {
        (void)&set1;
    }
};

TEST_SUITE(doc_serializing_test, "boost.json.doc_serializing");

} // namespace json
} // namespace boost
