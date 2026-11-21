// mcpplibs.mwrap.boost.json — libs/json/test/doc_uses_allocator.cpp, the code the
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
// tag::doc_uses_allocator_1[]
// We want to use this resource for all the containers
monotonic_resource mr;

// Declare a vector of JSON values
std::vector< value, boost::container::pmr::polymorphic_allocator< value > > v( &mr );

// The polymorphic allocator will use our resource
assert( v.get_allocator().resource() == &mr );

// Add a string to the vector
v.emplace_back( "boost" );

// The vector propagates the memory resource to the string
assert( v[0].storage().get() == &mr );
// end::doc_uses_allocator_1[]
}
//----------------------------------------------------------
{
// tag::doc_uses_allocator_2[]
// This vector will use the default memory resource
std::vector< value, boost::container::pmr::polymorphic_allocator < value > > v;

// This value will same memory resource as the vector
value jv( v.get_allocator() );

// However, ownership is not transferred,
assert( ! jv.storage().is_shared() );

// and deallocate is never null
assert( ! jv.storage().is_deallocate_trivial() );
// end::doc_uses_allocator_2[]
}
//----------------------------------------------------------

} // set1

//----------------------------------------------------------

class doc_uses_allocator_test
{
public:
    void
    run()
    {
        (void)&set1;
        BOOST_TEST_PASS();
    }
};

TEST_SUITE(doc_uses_allocator_test, "boost.json.doc_uses_allocator");

} // namespace json
} // namespace boost
