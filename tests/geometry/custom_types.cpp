// mcpplibs.mwrap.boost.geometry — user geometry types: a point adapted with
// hand-written traits (tag, coordinate_type, coordinate_system, dimension,
// access — the Point Concept page), the legacy object model of
// example_adapting_a_legacy_geometry_object_model.qbk (QPoint, QLineString,
// QRing, QPolygon through the traits and Boost.Range customization), types
// registered with the BOOST_GEOMETRY_REGISTER_* macros (whose header stays
// #included before the import: macros are not exported), and a user
// coordinate type; algorithms on each, and the traits telling a non-geometry
// apart.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/concept/assert.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
import std;
import mcpplibs.mwrap.boost.geometry;

namespace bg = boost::geometry;

// ---- the Point Concept, spelled out ----
namespace user {
struct vec3 {
    float c[3];
};
}  // namespace user
namespace boost { namespace geometry { namespace traits {
template <> struct tag<user::vec3> { using type = point_tag; };
template <> struct dimension<user::vec3> : boost::mpl::int_<3> {};
template <> struct coordinate_type<user::vec3> { using type = float; };
template <> struct coordinate_system<user::vec3> { using type = boost::geometry::cs::cartesian; };
template <std::size_t Index>
struct access<user::vec3, Index> {
    static_assert(Index < 3, "Out of range");
    static inline float get(user::vec3 const& p) { return p.c[Index]; }
    static inline void set(user::vec3& p, float const& value) { p.c[Index] = value; }
};
}}}  // namespace boost::geometry::traits

TEST_CASE("doc: concept_point") {
    BOOST_CONCEPT_ASSERT((bg::concepts::Point<user::vec3>));
    static_assert(std::is_same_v<bg::tag_t<user::vec3>, bg::point_tag>);
    static_assert(bg::dimension<user::vec3>::value == 3);
    user::vec3 a{{0, 0, 0}}, b{{1, 2, 2}};
    CHECK(bg::distance(a, b) == 3.0f);
    bg::set<2>(a, 5.0f);
    CHECK(bg::get<2>(a) == 5.0f);
    bg::model::box<user::vec3> env;
    bg::envelope(bg::model::linestring<user::vec3>{a, b}, env);
    CHECK(bg::get<bg::max_corner, 2>(env) == 5.0f);
}

// ---- the legacy object model ----
namespace legacy {
struct QPoint {
    double x, y;
};
struct QLineString {
    bool cached;
    std::vector<QPoint*> points;
};
struct QRing {
    std::vector<QPoint*> points;
};
struct QPolygon {
    QRing* exterior;
    std::vector<QRing*> interiors;
};
// iterate the pointers as points
template <class P>
using deref_iterator = boost::indirect_iterator<typename std::vector<P*>::iterator>;
template <class P>
using deref_const_iterator = boost::indirect_iterator<typename std::vector<P*>::const_iterator>;
}  // namespace legacy

namespace boost {
template <> struct range_iterator<legacy::QLineString> { using type = legacy::deref_iterator<legacy::QPoint>; };
template <> struct range_const_iterator<legacy::QLineString> { using type = legacy::deref_const_iterator<legacy::QPoint>; };
template <> struct range_iterator<legacy::QRing> { using type = legacy::deref_iterator<legacy::QPoint>; };
template <> struct range_const_iterator<legacy::QRing> { using type = legacy::deref_const_iterator<legacy::QPoint>; };
}  // namespace boost
namespace legacy {
inline deref_iterator<QPoint> range_begin(QLineString& l) { return deref_iterator<QPoint>(l.points.begin()); }
inline deref_iterator<QPoint> range_end(QLineString& l) { return deref_iterator<QPoint>(l.points.end()); }
inline deref_const_iterator<QPoint> range_begin(QLineString const& l) { return deref_const_iterator<QPoint>(l.points.begin()); }
inline deref_const_iterator<QPoint> range_end(QLineString const& l) { return deref_const_iterator<QPoint>(l.points.end()); }
inline deref_iterator<QPoint> range_begin(QRing& r) { return deref_iterator<QPoint>(r.points.begin()); }
inline deref_iterator<QPoint> range_end(QRing& r) { return deref_iterator<QPoint>(r.points.end()); }
inline deref_const_iterator<QPoint> range_begin(QRing const& r) { return deref_const_iterator<QPoint>(r.points.begin()); }
inline deref_const_iterator<QPoint> range_end(QRing const& r) { return deref_const_iterator<QPoint>(r.points.end()); }
}  // namespace legacy

namespace boost { namespace geometry { namespace traits {
template <> struct tag<legacy::QPoint> { using type = point_tag; };
template <> struct coordinate_type<legacy::QPoint> { using type = double; };
template <> struct coordinate_system<legacy::QPoint> { using type = cs::cartesian; };
template <> struct dimension<legacy::QPoint> : boost::mpl::int_<2> {};
template <> struct access<legacy::QPoint, 0> {
    static double get(legacy::QPoint const& p) { return p.x; }
    static void set(legacy::QPoint& p, double const& v) { p.x = v; }
};
template <> struct access<legacy::QPoint, 1> {
    static double get(legacy::QPoint const& p) { return p.y; }
    static void set(legacy::QPoint& p, double const& v) { p.y = v; }
};
template <> struct tag<legacy::QLineString> { using type = linestring_tag; };
template <> struct tag<legacy::QRing> { using type = ring_tag; };
template <> struct tag<legacy::QPolygon> { using type = polygon_tag; };
template <> struct ring_const_type<legacy::QPolygon> { using type = legacy::QRing const&; };
template <> struct ring_mutable_type<legacy::QPolygon> { using type = legacy::QRing&; };
template <> struct interior_const_type<legacy::QPolygon> {
    using type = boost::indirected_range<std::vector<legacy::QRing*> const>;
};
template <> struct interior_mutable_type<legacy::QPolygon> {
    using type = boost::indirected_range<std::vector<legacy::QRing*>>;
};
template <> struct exterior_ring<legacy::QPolygon> {
    static legacy::QRing& get(legacy::QPolygon& p) { return *p.exterior; }
    static legacy::QRing const& get(legacy::QPolygon const& p) { return *p.exterior; }
};
template <> struct interior_rings<legacy::QPolygon> {
    static auto get(legacy::QPolygon& p) { return boost::adaptors::indirect(p.interiors); }
    static auto get(legacy::QPolygon const& p) { return boost::adaptors::indirect(p.interiors); }
};
}}}  // namespace boost::geometry::traits

TEST_CASE("doc: example-adapting-a-legacy-geometry-object-model") {
    legacy::QPoint p1{0, 0}, p2{3, 4}, p3{3, 0};
    legacy::QLineString ls{false, {&p1, &p2, &p3}};
    CHECK(bg::length(ls) == 5 + 4);
    CHECK(bg::num_points(ls) == 3);
    std::ostringstream os;
    os << bg::wkt(p2);
    CHECK(os.str() == "POINT(3 4)");
}

TEST_CASE("doc: example-source-code-adapting-a-legacy-geometry-object-model") {
    legacy::QPoint a{0, 0}, b{0, 4}, c{4, 4}, d{4, 0}, h1{1, 1}, h2{2, 1}, h3{2, 2}, h4{1, 2};
    legacy::QRing outer{{&a, &b, &c, &d, &a}};
    legacy::QRing hole{{&h1, &h2, &h3, &h4, &h1}};  // counterclockwise: an interior ring
    legacy::QPolygon poly{&outer, {&hole}};
    CHECK(bg::area(outer) == 16);
    CHECK(bg::area(poly) == 15);
    CHECK(bg::num_interior_rings(poly) == 1);
    legacy::QPoint inside{3, 3}, in_hole{1.5, 1.5};
    CHECK(bg::within(inside, poly));
    CHECK_FALSE(bg::within(in_hole, poly));
    bg::set<0>(a, -1.0);  // through access<>::set, into the shared point
    CHECK(outer.points.front()->x == -1);
}

// ---- registration macros ----
namespace user {
struct pt {
    double x, y;
};
struct rect {
    pt lo, hi;
};
struct track {
    std::vector<pt> v;
    using value_type = pt;
    auto begin() { return v.begin(); }
    auto end() { return v.end(); }
    auto begin() const { return v.begin(); }
    auto end() const { return v.end(); }
    std::size_t size() const { return v.size(); }
    void push_back(pt const& p) { v.push_back(p); }
    void clear() { v.clear(); }
    void resize(std::size_t n) { v.resize(n); }
    using iterator = std::vector<pt>::iterator;
    using const_iterator = std::vector<pt>::const_iterator;
};
struct fence : track {};
}  // namespace user
BOOST_GEOMETRY_REGISTER_POINT_2D(user::pt, double, cs::cartesian, x, y)
BOOST_GEOMETRY_REGISTER_BOX(user::rect, user::pt, lo, hi)
BOOST_GEOMETRY_REGISTER_LINESTRING(user::track)
BOOST_GEOMETRY_REGISTER_RING(user::fence)

TEST_CASE("custom: registered types") {
    static_assert(std::is_same_v<bg::tag_t<user::pt>, bg::point_tag>);
    static_assert(std::is_same_v<bg::tag_t<user::rect>, bg::box_tag>);
    static_assert(std::is_same_v<bg::tag_t<user::track>, bg::linestring_tag>);
    static_assert(std::is_same_v<bg::tag_t<user::fence>, bg::ring_tag>);
    user::rect r{{0, 0}, {2, 3}};
    CHECK(bg::area(r) == 6);
    user::track t;
    bg::append(t, user::pt{0, 0});
    bg::append(t, user::pt{0, 5});
    CHECK(bg::length(t) == 5);
    user::fence f;
    bg::read_wkt("POLYGON((0 0,0 1,1 1,1 0,0 0))", f);
    CHECK(bg::area(f) == 1);
    CHECK(bg::intersects(t, r));
    user::pt c;
    bg::centroid(r, c);
    CHECK(c.x == 1);
}

// ---- a user coordinate type ----
namespace user {
struct fixed {  // a decimal fixed-point number, 1/1000 units
    long long raw = 0;
    fixed() = default;
    fixed(double d) : raw(std::llround(d * 1000)) {}
    explicit operator double() const { return raw / 1000.0; }
    friend fixed operator+(fixed a, fixed b) { fixed r; r.raw = a.raw + b.raw; return r; }
    friend fixed operator-(fixed a, fixed b) { fixed r; r.raw = a.raw - b.raw; return r; }
    friend fixed operator*(fixed a, fixed b) { return fixed(double(a) * double(b)); }
    friend fixed operator/(fixed a, fixed b) { return fixed(double(a) / double(b)); }
    friend fixed operator-(fixed a) { fixed r; r.raw = -a.raw; return r; }
    friend bool operator<(fixed a, fixed b) { return a.raw < b.raw; }
    friend bool operator>(fixed a, fixed b) { return a.raw > b.raw; }
    friend bool operator<=(fixed a, fixed b) { return a.raw <= b.raw; }
    friend bool operator>=(fixed a, fixed b) { return a.raw >= b.raw; }
    friend bool operator==(fixed a, fixed b) { return a.raw == b.raw; }
    friend bool operator!=(fixed a, fixed b) { return a.raw != b.raw; }
    fixed& operator+=(fixed b) { raw += b.raw; return *this; }
    fixed& operator-=(fixed b) { raw -= b.raw; return *this; }
    fixed& operator*=(fixed b) { return *this = *this * b; }
    fixed& operator/=(fixed b) { return *this = *this / b; }
};
inline fixed abs(fixed a) { return a.raw < 0 ? -a : a; }
inline fixed sqrt(fixed a) { return fixed(std::sqrt(double(a))); }
}  // namespace user
namespace std {
template <> class numeric_limits<user::fixed> : public numeric_limits<long long> {
public:
    static constexpr bool is_specialized = true;
    static constexpr bool is_integer = false;
    static user::fixed epsilon() { user::fixed f; f.raw = 1; return f; }
    static user::fixed max() { user::fixed f; f.raw = numeric_limits<long long>::max(); return f; }
    static user::fixed lowest() { user::fixed f; f.raw = numeric_limits<long long>::lowest(); return f; }
    static user::fixed min() { return epsilon(); }
};
}  // namespace std

TEST_CASE("custom: coordinate type") {
    using P = bg::model::d2::point_xy<user::fixed>;
    static_assert(std::is_same_v<bg::coordinate_type_t<P>, user::fixed>);
    bg::model::linestring<P> ls{{user::fixed(0.0), user::fixed(0.0)}, {user::fixed(3.0), user::fixed(4.0)}};
    CHECK(double(bg::length(ls)) == 5.0);
    bg::model::box<P> b;
    bg::envelope(ls, b);
    CHECK(double(bg::get<bg::max_corner, 1>(b)) == 4.0);
}

TEST_CASE("custom: not a geometry") {
    // traits::tag is not specialized for an arbitrary type
    static_assert(std::is_same_v<bg::tag_t<int>, void>);
    static_assert(std::is_same_v<bg::tag_t<std::string>, void>);
    static_assert(!bg::util::is_point<int>::value);
    static_assert(bg::util::is_point<user::pt>::value);
}
