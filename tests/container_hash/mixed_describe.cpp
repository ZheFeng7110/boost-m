// boost.container_hash module + boost.describe headers in one TU: the
// BOOST_DESCRIBE_* macros come from #include (no module can carry them), the
// hashing from `import mcpplibs.mwrap.boost.container_hash;`, which exports
// boost::container_hash::is_described_class. This is the include+import mix
// of design §4.4; the includes precede every import.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/describe/class.hpp>
import std;
import mcpplibs.mwrap.boost.container_hash;

namespace mixed {
struct point {
    int x = 0, y = 0;
};
BOOST_DESCRIBE_STRUCT(point, (), (x, y))
}  // namespace mixed

TEST_CASE("described struct hashed through the module") {
    mixed::point a{1, 2}, b{1, 2}, c{2, 1};
    boost::hash<mixed::point> h;
    CHECK(h(a) == h(b));
    CHECK(h(a) != h(c));
    std::size_t seed = 0;
    boost::hash_combine(seed, 1);
    boost::hash_combine(seed, 2);
    CHECK(h(a) == seed);  // members combined in declaration order
}
