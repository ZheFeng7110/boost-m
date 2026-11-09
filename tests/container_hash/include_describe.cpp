// boost.container_hash + boost.describe — pure include. describe has no
// module (its API is BOOST_DESCRIBE_*, docs/usage.md §2.3), so a described
// class is hashed in a TU that includes container_hash too. Covers
// libs/container_hash/examples/point2.cpp; mixed_describe.cpp checks the same
// with the container_hash module imported.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstddef>
#include <string>
#include <boost/container_hash/hash.hpp>
#include <boost/describe/class.hpp>
#include <boost/describe/operators.hpp>

namespace ex_point2 {
class point {
    int x;
    int y;
    BOOST_DESCRIBE_CLASS(point, (), (), (), (x, y))
public:
    point() : x(0), y(0) {}
    point(int x, int y) : x(x), y(y) {}
};
using boost::describe::operators::operator==;
using boost::describe::operators::operator!=;

struct base {
    int b = 1;
};
BOOST_DESCRIBE_STRUCT(base, (), (b))
struct derived : base {
    std::string s = "x";
};
BOOST_DESCRIBE_STRUCT(derived, (base), (s))
}  // namespace ex_point2

TEST_CASE("example: point2") {
    using ex_point2::point;
    boost::hash<point> point_hasher;
    point p1(0, 0), p2(1, 2), p3(4, 1), p4 = p1;
    CHECK(point_hasher(p1) == point_hasher(p4));
    CHECK(point_hasher(p1) != point_hasher(p2));
    CHECK(point_hasher(p1) != point_hasher(p3));
    CHECK(p1 == p4);
    CHECK(p1 != p2);
}

TEST_CASE("described bases are hashed too") {
    ex_point2::derived a, b;
    b.b = 2;
    CHECK(boost::hash<ex_point2::derived>()(a) != boost::hash<ex_point2::derived>()(b));
    static_assert(boost::container_hash::is_described_class<ex_point2::derived>::value);
}
