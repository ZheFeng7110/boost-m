// boost.hash2 with described classes — libs/hash2/doc/hash2/
// hashing_objects.adoc#described-classes: the BOOST_DESCRIBE_* macros come from
// #include (no module carries them), hash_append from the hash2 module, which
// hashes a described class member by member and base by base (design §4.4:
// includes precede imports).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/describe/class.hpp>
import std;
import mcpplibs.mwrap.boost.hash2;

namespace user {
struct X {
    int a;
};
BOOST_DESCRIBE_STRUCT(X, (), (a))
struct Y : public X {
    int b;
};
BOOST_DESCRIBE_STRUCT(Y, (X), (b))
}  // namespace user

TEST_CASE("doc: hashing_objects#described-classes") {
    boost::hash2::fnv1a_32 h1;
    user::Y v1 = {{1}, 2};
    boost::hash2::hash_append(h1, {}, v1);
    boost::hash2::fnv1a_32 h2;
    boost::hash2::hash_append(h2, {}, v1.a);
    boost::hash2::hash_append(h2, {}, v1.b);
    CHECK(h1.result() == h2.result());
}
