// mcpplibs.mwrap.boost.geometry — the documentation's core, geometry and registration examples
// (libs/geometry/doc/src/examples/{core,geometries}/) ported mechanically: each program runs as a case
// and its output must equal what the same code printed when built from
// upstream/boost headers alone. The registration macros
// (BOOST_GEOMETRY_REGISTER_*) and the adaptation headers that define them
// (geometries/register) stay #included before the imports; of the adapted
// headers (geometries/adapted), whose specializations the module has, only the
// macro definitions are copied (gcc takes the module's and a textual copy of
// the same partial specializations as ambiguous);
// and the case namespace closes around each registration (the macros open
// namespace boost::geometry::traits) and each specialization the examples
// write in namespace boost (in place: they use the examples' own types).
// Examples whose registrations of std:: containers or coordinate systems
// disagree are in different files (_2, _3, ...). Not compared: output of
// typeid(...).name() (the C++ library's mangling) and of the rtree examples
// whose polygons come from std::cos/std::sin values on rounding boundaries
// (and mapped_file, whose file outlives a run).
// boost_range/uniqued.cpp does not compile upstream (coverage.toml). The examples' SVG helpers are inlined;
// they write SVG only under HAVE_SVG, which the cases do not define.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/assert.hpp>
#include <cassert>
// the macros of <boost/geometry/geometries/adapted/boost_tuple.hpp> (its declarations are in the module)
#ifndef BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS
#define BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(CoordinateSystem) \
    namespace boost { namespace geometry { namespace traits { \
    template <typename T1, typename T2, typename T3, typename T4, typename T5, \
              typename T6, typename T7, typename T8, typename T9, typename T10> \
    struct coordinate_system<boost::tuple<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10> > \
    { \
        typedef CoordinateSystem type; \
    }; \
    }}}
#endif

#include <boost/config.hpp>
// the macros of <boost/geometry/geometries/adapted/boost_array.hpp> (its declarations are in the module)
#ifndef BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS
#define BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(CoordinateSystem) \
    namespace boost { namespace geometry { namespace traits { \
    template <class T, std::size_t N> \
    struct coordinate_system<boost::array<T, N> > \
    { \
        typedef CoordinateSystem type; \
    }; \
    }}}
#endif

#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
// the macros of <boost/geometry/geometries/adapted/boost_polygon.hpp> (its declarations are in the module)

// the macros of <boost/geometry/geometries/adapted/c_array.hpp> (its declarations are in the module)
#ifndef BOOST_GEOMETRY_REGISTER_C_ARRAY_CS
#define BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(CoordinateSystem) \
    namespace boost { namespace geometry { namespace traits { \
    template <typename T, std::size_t N> \
    struct coordinate_system<T[N]> \
    { \
        typedef CoordinateSystem type; \
    }; \
    }}}
#endif

// the macros of <boost/geometry/geometries/adapted/std_array.hpp> (its declarations are in the module)
#ifndef BOOST_GEOMETRY_REGISTER_STD_ARRAY_CS
#define BOOST_GEOMETRY_REGISTER_STD_ARRAY_CS(CoordinateSystem) \
    namespace boost { namespace geometry { namespace traits { \
    template <class T, std::size_t N> \
    struct coordinate_system<std::array<T, N> > \
    { \
        typedef CoordinateSystem type; \
    }; \
    }}}
#endif

// the macros of <boost/geometry/geometries/adapted/boost_range/filtered.hpp> (its declarations are in the module)

// the macros of <boost/geometry/geometries/adapted/boost_range/reversed.hpp> (its declarations are in the module)

// the macros of <boost/geometry/geometries/adapted/boost_range/sliced.hpp> (its declarations are in the module)

// the macros of <boost/geometry/geometries/adapted/boost_range/strided.hpp> (its declarations are in the module)

import std;
import mcpplibs.mwrap.boost.geometry;
import mcpplibs.mwrap.boost.algorithm;
import mcpplibs.mwrap.boost.core;
import mcpplibs.mwrap.boost.polygon;
import mcpplibs.mwrap.boost.tuple;
import mcpplibs.mwrap.boost.range;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// The examples write their SVG and data files to the current directory: the
// cases run in the temporary directory.
inline const bool ex_cwd_is_tmp = (std::filesystem::current_path(std::filesystem::temp_directory_path()), true);

// libs/geometry/doc/src/examples/core/closure.cpp
namespace ex_core_closure {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    boost::geometry::closure_selector clos = boost::geometry::closure<polygon_type>::value;

    ex_out << "closure: " << clos << std::endl
        << "(open = " << boost::geometry::open
        << ", closed = " << boost::geometry::closed
        << ") "<< std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[closure_output
/*`
Output:
[pre
closure: 1
(open = 0, closed = 1)
]
*/
//]

TEST_CASE("example: core/closure") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(closure: 1
(open = 0, closed = 1) 
)ex"));
}
}  // namespace ex_core_closure

// libs/geometry/doc/src/examples/core/coordinate_dimension.cpp
namespace ex_core_coordinate_dimension {
}  // namespace ex_core_coordinate_dimension (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_core_coordinate_dimension {;

int example_main()
{
    int dim = boost::geometry::dimension
        <
            boost::geometry::model::linestring
                <
                    boost::tuple<float, float, float>
                >
        >::value;

    ex_out << "dimensions: " << dim << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[dimension_output
/*`
Output:
[pre
dimensions: 3
]
*/
//]

TEST_CASE("example: core/coordinate_dimension") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(dimensions: 3
)ex"));
}
}  // namespace ex_core_coordinate_dimension

// libs/geometry/doc/src/examples/core/coordinate_system.cpp
namespace ex_core_coordinate_system {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    using system = boost::geometry::coordinate_system<polygon_type>::type;

    ex_out << "system: " << typeid(system).name() << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[coordinate_system_output
/*`
Output (using MSVC):
[pre
system: struct boost::geometry::cs::cartesian
]
*/
//]

TEST_CASE("example: core/coordinate_system") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(system: N5boost8geometry2cs9cartesianE
)ex"));
#endif
}
}  // namespace ex_core_coordinate_system

// libs/geometry/doc/src/examples/core/coordinate_type.cpp
namespace ex_core_coordinate_type {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    using ctype = boost::geometry::coordinate_type<polygon_type>::type;

    ex_out << "type: " << typeid(ctype).name() << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[coordinate_type_output
/*`
Output (using MSVC):
[pre
type: double
]
*/
//]

TEST_CASE("example: core/coordinate_type") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(type: d
)ex"));
#endif
}
}  // namespace ex_core_coordinate_type

// libs/geometry/doc/src/examples/core/degree_radian.cpp
namespace ex_core_degree_radian {
using namespace boost::geometry;

int example_main()
{
    using degree_point = model::point<double, 2, cs::spherical_equatorial<degree>>;
    using radian_point = model::point<double, 2, cs::spherical_equatorial<radian>>;

    degree_point d(4.893, 52.373);
    radian_point r(0.041, 0.8527);

    double dist = distance(d, r);
    ex_out
        << "distance:" << std::endl
        << dist << " over unit sphere" << std::endl
        << dist * 3959  << " over a spherical earth, in miles" << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[degree_radian_output
/*`
Output:
[pre
distance:
0.0675272 over unit sphere
267.34 over a spherical earth, in miles
]
*/
//]

TEST_CASE("example: core/degree_radian") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(distance:
0.0675272 over unit sphere
267.34 over a spherical earth, in miles
)ex"));
}
}  // namespace ex_core_degree_radian

// libs/geometry/doc/src/examples/core/get_box.cpp
namespace ex_core_get_box {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::box<bg::model::d2::point_xy<double>> box;

    bg::assign_values(box, 1, 3, 5, 6);

    ex_out << "Box:"
        << " " << bg::get<bg::min_corner, 0>(box)
        << " " << bg::get<bg::min_corner, 1>(box)
        << " " << bg::get<bg::max_corner, 0>(box)
        << " " << bg::get<bg::max_corner, 1>(box)
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[get_box_output
/*`
Output:
[pre
Box: 1 3 5 6
]
*/
//]

TEST_CASE("example: core/get_box") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Box: 1 3 5 6
)ex"));
}
}  // namespace ex_core_get_box

// libs/geometry/doc/src/examples/core/get_point.cpp
namespace ex_core_get_point {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::d2::point_xy<double> point(1, 2);

    double x = bg::get<0>(point);
    double y = bg::get<1>(point);

    ex_out << "x=" << x << " y=" << y << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[get_point_output
/*`
Output:
[pre
x=1 y=2
]
*/
//]

TEST_CASE("example: core/get_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(x=1 y=2
)ex"));
}
}  // namespace ex_core_get_point

// libs/geometry/doc/src/examples/core/interior_type.cpp
namespace ex_core_interior_type {
}  // namespace ex_core_interior_type (reopened after the registration)
#ifndef EX_REGISTERED_5e756957de5c
#define EX_REGISTERED_5e756957de5c
BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(cs::cartesian)
#endif
namespace ex_core_interior_type {

int example_main()
{
    // Define a polygon storing points in a deque and storing interior rings
    // in a list (note that std::list is not supported by most algorithms
    // because not supporting a random access iterator)
    using polygon = boost::geometry::model::polygon
        <
            boost::array<short, 3>,
            true, true,
            std::deque, std::list
        >;

    ex_out << typeid(boost::geometry::interior_type<polygon>::type).name() << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[interior_type_output
/*`
Output (using MSVC) is a long story (part manually replaced with ellipsis):
[pre
class std::list<class boost::geometry::model::ring<class boost::array<short,3>,1,1,class std::deque,class std::allocator>,class std::allocator<...>>>
]
*/
//]

TEST_CASE("example: core/interior_type") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(NSt3__14listIN5boost8geometry5model4ringINS1_5arrayIsLm3EEELb1ELb1ENS_5dequeENS_9allocatorEEENS8_IS9_EEEE
)ex"));
#endif
}
}  // namespace ex_core_interior_type

// libs/geometry/doc/src/examples/core/point_order.cpp
namespace ex_core_point_order {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type, false>;

    boost::geometry::order_selector order = boost::geometry::point_order<polygon_type>::value;

    ex_out << "order: " << order << std::endl
        << "(clockwise = " << boost::geometry::clockwise
        << ", counterclockwise = " << boost::geometry::counterclockwise
        << ") "<< std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[point_order_output
/*`
Output:
[pre
order: 2
(clockwise = 1, counterclockwise = 2)
]
*/
//]

TEST_CASE("example: core/point_order") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(order: 2
(clockwise = 1, counterclockwise = 2) 
)ex"));
}
}  // namespace ex_core_point_order

// libs/geometry/doc/src/examples/core/point_type.cpp
namespace ex_core_point_type {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;
    using mp_type = boost::geometry::model::multi_polygon<polygon_type>;
    using ptype = boost::geometry::point_type<mp_type>::type;

    ex_out << "point type: " << typeid(ptype).name() << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[point_type_output
/*`
Output (in MSVC):
[pre
point type: class boost::geometry::model::d2::point_xy<double,struct boost::geometry::cs::cartesian>
]
*/
//]

TEST_CASE("example: core/point_type") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(point type: N5boost8geometry5model2d28point_xyIdNS0_2cs9cartesianEEE
)ex"));
#endif
}
}  // namespace ex_core_point_type

// libs/geometry/doc/src/examples/core/ring_type.cpp
namespace ex_core_ring_type {
int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon = boost::geometry::model::polygon<point>;

    using ring_type = boost::geometry::ring_type<polygon>::type;
    using int_type = boost::geometry::interior_type<polygon>::type;

    ex_out << typeid(ring_type).name() << std::endl;
    ex_out << typeid(int_type).name() << std::endl;

    // So int_type defines a collection of rings,
    // which is a Boost.Range compatible range
    // The type of an element of the collection is the very same ring type again.
    // We show that.
    using int_ring_type = boost::range_value<int_type>::type;

    ex_out
        << std::boolalpha
        << std::is_same<ring_type, int_ring_type>::value
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[ring_type_output
/*`
Output (using gcc):
[pre
          N5boost8geometry5model4ringINS1_2d28point_xyIdNS0_2cs9cartesianEEELb1ELb1ESt6vectorSaEE
St6vectorIN5boost8geometry5model4ringINS2_2d28point_xyIdNS1_2cs9cartesianEEELb1ELb1ES_SaEESaIS9_EE
true
]
*/
//]

TEST_CASE("example: core/ring_type") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(N5boost8geometry5model4ringINS1_2d28point_xyIdNS0_2cs9cartesianEEELb1ELb1ENSt3__16vectorENS8_9allocatorEEE
NSt3__16vectorIN5boost8geometry5model4ringINS3_2d28point_xyIdNS2_2cs9cartesianEEELb1ELb1ES0_NS_9allocatorEEENSA_ISB_EEEE
true
)ex"));
#endif
}
}  // namespace ex_core_ring_type

// libs/geometry/doc/src/examples/core/rings.cpp
namespace ex_core_rings {
int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point>;

    polygon_type poly;

    using ring_type = boost::geometry::ring_type<polygon_type>::type;
    ring_type& ring = boost::geometry::exterior_ring(poly);

    // For a ring of model::polygon, you can call "push_back".
    // (internally, it is done using a traits::push_back class)
    ring.push_back(point(0, 0));
    ring.push_back(point(0, 5));
    ring.push_back(point(5, 4));
    ring.push_back(point(0, 0));

    ring_type inner;
    inner.push_back(point(1, 1));
    inner.push_back(point(2, 1));
    inner.push_back(point(2, 2));
    inner.push_back(point(1, 1));

    auto& interiors = boost::geometry::interior_rings(poly);
    interiors.push_back(inner);

    ex_out << boost::geometry::dsv(poly) << std::endl;

    // Let int_type define a collection of rings,
    // which is a Boost.Range compatible range
    using int_type = boost::geometry::interior_type<polygon_type>::type;

    // The type of an element of the collection is the very same ring type again.
    // We show that.
    using int_ring_type = boost::range_value<int_type>::type;

    ex_out
        << std::boolalpha
        << std::is_same<ring_type, int_ring_type>::value
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[rings_output
/*`
Output:
[pre
(((0, 0), (0, 5), (5, 4), (0, 0)), ((1, 1), (2, 1), (2, 2), (1, 1)))
true
]
*/
//]

TEST_CASE("example: core/rings") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((((0, 0), (0, 5), (5, 4), (0, 0)), ((1, 1), (2, 1), (2, 2), (1, 1)))
true
)ex"));
}
}  // namespace ex_core_rings

// libs/geometry/doc/src/examples/core/set_box.cpp
namespace ex_core_set_box {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::box<bg::model::d2::point_xy<double>> box;

    bg::set<bg::min_corner, 0>(box, 0);
    bg::set<bg::min_corner, 1>(box, 2);
    bg::set<bg::max_corner, 0>(box, 4);
    bg::set<bg::max_corner, 1>(box, 5);

    ex_out << "Extent: " << bg::dsv(box) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[set_box_output
/*`
Output:
[pre
Extent: ((0, 2), (4, 5))
]
*/
//]

TEST_CASE("example: core/set_box") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Extent: ((0, 2), (4, 5))
)ex"));
}
}  // namespace ex_core_set_box

// libs/geometry/doc/src/examples/core/set_point.cpp
namespace ex_core_set_point {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::d2::point_xy<double> point;

    bg::set<0>(point, 1);
    bg::set<1>(point, 2);

    ex_out << "Location: " << bg::dsv(point) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[set_point_output
/*`
Output:
[pre
Location: (1, 2)
]
*/
//]

TEST_CASE("example: core/set_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Location: (1, 2)
)ex"));
}
}  // namespace ex_core_set_point

// libs/geometry/doc/src/examples/core/tag.cpp
namespace ex_core_tag {
}  // namespace ex_core_tag (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_core_tag {

template <typename Tag> struct dispatch {};

// Specialization for points
template <> struct dispatch<boost::geometry::point_tag>
{
    template <typename Point>
    static inline void apply(Point const& p)
    {
        // Use the Boost.Geometry free function "get"
        // working on all supported point types
        ex_out << "Hello POINT, you are located at: "
            << boost::geometry::get<0>(p) << ", "
            << boost::geometry::get<1>(p)
            << std::endl;
    }
};

// Specialization for polygons
template <> struct dispatch<boost::geometry::polygon_tag>
{
    template <typename Polygon>
    static inline void apply(Polygon const& p)
    {
        // Use the Boost.Geometry manipulator "dsv"
        // working on all supported geometries
        ex_out << "Hello POLYGON, you look like: "
            << boost::geometry::dsv(p)
            << std::endl;
    }
};

// Specialization for multipolygons
template <> struct dispatch<boost::geometry::multi_polygon_tag>
{
    template <typename MultiPolygon>
    static inline void apply(MultiPolygon const& m)
    {
        // Use the Boost.Range free function "size" because all
        // multigeometries comply to Boost.Range
        ex_out << "Hello MULTIPOLYGON, you contain: "
            << boost::size(m) << " polygon(s)"
            << std::endl;
    }
};

template <typename Geometry>
inline void hello(Geometry const& geometry)
{
    // Call the metafunction "tag" to dispatch, and call method (here "apply")
    dispatch
        <
            typename boost::geometry::tag<Geometry>::type
        >::apply(geometry);
}

int example_main()
{
    // Define polygon type (here: based on a Boost.Tuple)
    using polygon_type = boost::geometry::model::polygon<boost::tuple<int, int>>;

    // Declare and fill a polygon and a multipolygon
    polygon_type poly;
    boost::geometry::exterior_ring(poly) = {{0, 0}, {0, 10}, {10, 5}, {0, 0}};

    boost::geometry::model::multi_polygon<polygon_type> multi;
    multi.push_back(poly);

    // Call "hello" for point, polygon, multipolygon
    hello(boost::make_tuple(2, 3));
    hello(poly);
    hello(multi);

    return 0;
    return 0;  // main's implicit return
}

//]

//[tag_output
/*`
Output:
[pre
Hello POINT, you are located at: 2, 3
Hello POLYGON, you look like: (((0, 0), (0, 10), (10, 5), (0, 0)))
Hello MULTIPOLYGON, you contain: 1 polygon(s)
]
*/
//]

TEST_CASE("example: core/tag") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Hello POINT, you are located at: 2, 3
Hello POLYGON, you look like: (((0, 0), (0, 10), (10, 5), (0, 0)))
Hello MULTIPOLYGON, you contain: 1 polygon(s)
)ex"));
}
}  // namespace ex_core_tag

// libs/geometry/doc/src/examples/core/tag_cast.cpp
namespace ex_core_tag_cast {
namespace geo = boost::geometry;
int example_main()
{
    using point_type = geo::model::d2::point_xy<double>;
    using polygon_type = geo::model::polygon<point_type>;

    using tag = geo::tag<polygon_type>::type;
    using base_tag = geo::tag_cast<tag, geo::linear_tag, geo::areal_tag>::type;

    ex_out << "tag: " << typeid(tag).name() << std::endl
        << "base tag: " << typeid(base_tag).name() << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[tag_cast_output
/*`
Output (in MSVC):
[pre
tag: struct boost::geometry::polygon_tag
base tag: struct boost::geometry::areal_tag
]
*/
//]

TEST_CASE("example: core/tag_cast") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(tag: N5boost8geometry11polygon_tagE
base tag: N5boost8geometry9areal_tagE
)ex"));
#endif
}
}  // namespace ex_core_tag_cast

// libs/geometry/doc/src/examples/geometries/box.cpp
namespace ex_geometries_box {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using box_t = bg::model::box<point_t>;

    box_t box1; /*< Default-construct a box. >*/
    box_t box2(point_t(0.0, 0.0), point_t(5.0, 5.0)); /*< Construct, assigning min and max corner point. >*/
    box_t box3{{0.0, 0.0}, {5.0, 5.0}}; /*< Construct, using C++11 unified initialization syntax. >*/

    bg::set<bg::min_corner, 0>(box1, 1.0); /*< Set a coordinate, generic. >*/
    bg::set<bg::min_corner, 1>(box1, 2.0);
    box1.max_corner().set<0>(3.0); /*< Set a coordinate, class-specific ([*Note]: prefer `bg::set()`). >*/
    box1.max_corner().set<1>(4.0);

    double min_x = bg::get<bg::min_corner, 0>(box1); /*< Get a coordinate, generic. >*/
    double min_y = bg::get<bg::min_corner, 1>(box1);
    double max_x = box1.max_corner().get<0>(); /*< Get a coordinate, class-specific ([*Note]: prefer `bg::get()`). >*/
    double max_y = box1.max_corner().get<1>();

    ex_out << min_x << ", " << min_y << ", " << max_x << ", " << max_y << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[box_output
/*`
Output:
[pre
1, 2, 3, 4
]
*/
//]

TEST_CASE("example: geometries/box") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1, 2, 3, 4
)ex"));
}
}  // namespace ex_geometries_box

// libs/geometry/doc/src/examples/geometries/linestring.cpp
namespace ex_geometries_linestring {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using linestring_t = bg::model::linestring<point_t>;

    linestring_t ls1; /*< Default-construct a linestring. >*/
    linestring_t ls2{{0.0, 0.0}, {1.0, 0.0}, {1.0, 2.0}}; /*< Construct a linestring containing three points, using C++11 unified initialization syntax. >*/

    bg::append(ls1, point_t(0.0, 0.0)); /*< Append point. >*/
    bg::append(ls1, point_t(1.0, 0.0));
    bg::append(ls1, point_t(1.0, 2.0));

    double l = bg::length(ls1);

    ex_out << l << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[linestring_output
/*`
Output:
[pre
3
]
*/
//]

TEST_CASE("example: geometries/linestring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(3
)ex"));
}
}  // namespace ex_geometries_linestring

// libs/geometry/doc/src/examples/geometries/multi_linestring.cpp
namespace ex_geometries_multi_linestring {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using linestring_t = bg::model::linestring<point_t>;
    using mlinestring_t = bg::model::multi_linestring<linestring_t>;

    mlinestring_t mls1; /*< Default-construct a multi_linestring. >*/
    mlinestring_t mls2{{{0.0, 0.0}, {0.0, 1.0}, {2.0, 1.0}},
                       {{1.0, 0.0}, {2.0, 0.0}}}; /*< Construct a multi_linestring containing two linestrings, using C++11 unified initialization syntax. >*/

    mls1.resize(2); /*< Resize a multi_linestring, store two linestrings. >*/

    bg::append(mls1[0], point_t(0.0, 0.0)); /*< Append point to the first linestring. >*/
    bg::append(mls1[0], point_t(0.0, 1.0));
    bg::append(mls1[0], point_t(2.0, 1.0));

    bg::append(mls1[1], point_t(1.0, 0.0)); /*< Append point to the second linestring. >*/
    bg::append(mls1[1], point_t(2.0, 0.0));

    double l = bg::length(mls1);

    ex_out << l << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[multi_linestring_output
/*`
Output:
[pre
4
]
*/
//]

TEST_CASE("example: geometries/multi_linestring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(4
)ex"));
}
}  // namespace ex_geometries_multi_linestring

// libs/geometry/doc/src/examples/geometries/multi_point.cpp
namespace ex_geometries_multi_point {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using mpoint_t = bg::model::multi_point<point_t>;

    mpoint_t mpt1; /*< Default-construct a multi_point. >*/
    mpoint_t mpt2{{{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}}}; /*< Construct a multi_point containing three points, using C++11 unified initialization syntax. >*/

    bg::append(mpt1, point_t(0.0, 0.0)); /*< Append point to the multi_point. >*/
    bg::append(mpt1, point_t(1.0, 1.0));
    bg::append(mpt1, point_t(2.0, 2.0));

    std::size_t count = bg::num_points(mpt1);

    ex_out << count << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[multi_point_output
/*`
Output:
[pre
3
]
*/
//]

TEST_CASE("example: geometries/multi_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(3
)ex"));
}
}  // namespace ex_geometries_multi_point

// libs/geometry/doc/src/examples/geometries/multi_polygon.cpp
namespace ex_geometries_multi_polygon {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using polygon_t = bg::model::polygon<point_t>; /*< Default parameters, clockwise, closed polygon. >*/
    using mpolygon_t = bg::model::multi_polygon<polygon_t>; /*< Clockwise, closed multi_polygon. >*/

    mpolygon_t mpoly1; /*< Default-construct a multi_polygon. >*/
    mpolygon_t mpoly2{{{{0.0, 0.0}, {0.0, 5.0}, {5.0, 5.0}, {5.0, 0.0}, {0.0, 0.0}},
                       {{1.0, 1.0}, {4.0, 1.0}, {4.0, 4.0}, {1.0, 4.0}, {1.0, 1.0}}},
                      {{{5.0, 5.0}, {5.0, 6.0}, {6.0, 6.0}, {6.0, 5.0}, {5.0, 5.0}}}}; /*< Construct a multi_polygon containing two polygons, using C++11 unified initialization syntax. >*/

    mpoly1.resize(2); /*< Resize a multi_polygon, store two polygons. >*/

    bg::append(mpoly1[0].outer(), point_t(0.0, 0.0)); /*< Append point to the exterior ring of the first polygon. >*/
    bg::append(mpoly1[0].outer(), point_t(0.0, 5.0));
    bg::append(mpoly1[0].outer(), point_t(5.0, 5.0));
    bg::append(mpoly1[0].outer(), point_t(5.0, 0.0));
    bg::append(mpoly1[0].outer(), point_t(0.0, 0.0));

    mpoly1[0].inners().resize(1); /*< Resize a container of interior rings of the first polygon. >*/
    bg::append(mpoly1[0].inners()[0], point_t(1.0, 1.0)); /*< Append point to the interior ring of the first polygon. >*/
    bg::append(mpoly1[0].inners()[0], point_t(4.0, 1.0));
    bg::append(mpoly1[0].inners()[0], point_t(4.0, 4.0));
    bg::append(mpoly1[0].inners()[0], point_t(1.0, 4.0));
    bg::append(mpoly1[0].inners()[0], point_t(1.0, 1.0));

    bg::append(mpoly1[1].outer(), point_t(5.0, 5.0)); /*< Append point to the exterior ring of the second polygon. >*/
    bg::append(mpoly1[1].outer(), point_t(5.0, 6.0));
    bg::append(mpoly1[1].outer(), point_t(6.0, 6.0));
    bg::append(mpoly1[1].outer(), point_t(6.0, 5.0));
    bg::append(mpoly1[1].outer(), point_t(5.0, 5.0));

    double a = bg::area(mpoly1);

    ex_out << a << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[multi_polygon_output
/*`
Output:
[pre
17
]
*/
//]

TEST_CASE("example: geometries/multi_polygon") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(17
)ex"));
}
}  // namespace ex_geometries_multi_polygon

// libs/geometry/doc/src/examples/geometries/point.cpp
namespace ex_geometries_point {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::point<double, 2, bg::cs::cartesian> point1;
    bg::model::point<double, 3, bg::cs::cartesian> point2(1.0, 2.0, 3.0); /*< Construct, assigning three coordinates >*/

    bg::set<0>(point1, 1.0); /*< Set a coordinate, generic. >*/
    point1.set<1>(2.0); /*< Set a coordinate, class-specific ([*Note]: prefer `bg::set()`). >*/

    double x = bg::get<0>(point1); /*< Get a coordinate, generic. >*/
    double y = point1.get<1>(); /*< Get a coordinate, class-specific ([*Note]: prefer `bg::get()`). >*/

    ex_out << x << ", " << y << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[point_output
/*`
Output:
[pre
1, 2
]
*/
//]

TEST_CASE("example: geometries/point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1, 2
)ex"));
}
}  // namespace ex_geometries_point

// libs/geometry/doc/src/examples/geometries/point_xy.cpp
namespace ex_geometries_point_xy {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::d2::point_xy<double> point1;
    bg::model::d2::point_xy<double> point2(1.0, 2.0); /*< Construct, assigning coordinates. >*/

    bg::set<0>(point1, 1.0); /*< Set a coordinate, generic. >*/
    point1.y(2.0); /*< Set a coordinate, class-specific ([*Note]: prefer `bg::set()`). >*/

    double x = bg::get<0>(point1); /*< Get a coordinate, generic. >*/
    double y = point1.y(); /*< Get a coordinate, class-specific ([*Note]: prefer `bg::get()`). >*/

    ex_out << x << ", " << y << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[point_xy_output
/*`
Output:
[pre
1, 2
]
*/
//]

TEST_CASE("example: geometries/point_xy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1, 2
)ex"));
}
}  // namespace ex_geometries_point_xy

// libs/geometry/doc/src/examples/geometries/point_xyz.cpp
namespace ex_geometries_point_xyz {
namespace bg = boost::geometry;

int example_main()
{
    bg::model::d3::point_xyz<double> point1;
    bg::model::d3::point_xyz<double> point2(3, 4, 5); /*< Construct, assigning coordinates. >*/

    bg::set<0>(point1, 1.0); /*< Set a coordinate, generic. >*/
    point1.y(2.0); /*< Set a coordinate, class-specific ([*Note]: prefer `bg::set()`). >*/
    point1.z(4.0);

    double x = bg::get<0>(point1); /*< Get a coordinate, generic. >*/
    double y = point1.y(); /*< Get a coordinate, class-specific ([*Note]: prefer `bg::get()`). >*/
    double z = point1.z();

    ex_out << x << ", " << y << ", " << z << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[point_xyz_output
/*`
Output:
[pre
1, 2, 4
]
*/
//]

TEST_CASE("example: geometries/point_xyz") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1, 2, 4
)ex"));
}
}  // namespace ex_geometries_point_xyz

// libs/geometry/doc/src/examples/geometries/polygon.cpp
namespace ex_geometries_polygon {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using polygon_t = bg::model::polygon<point_t>; /*< Default parameters, clockwise, closed polygon. >*/

    polygon_t poly1; /*< Default-construct a polygon. >*/

    polygon_t polygon2{{{0.0, 0.0}, {0.0, 5.0}, {5.0, 5.0}, {5.0, 0.0}, {0.0, 0.0}},
                       {{1.0, 1.0}, {4.0, 1.0}, {4.0, 4.0}, {1.0, 4.0}, {1.0, 1.0}}}; /*< Construct a polygon containing an exterior and interior ring, using C++11 unified initialization syntax. >*/

    bg::append(poly1.outer(), point_t(0.0, 0.0)); /*< Append point to the exterior ring. >*/
    bg::append(poly1.outer(), point_t(0.0, 5.0));
    bg::append(poly1.outer(), point_t(5.0, 5.0));
    bg::append(poly1.outer(), point_t(5.0, 0.0));
    bg::append(poly1.outer(), point_t(0.0, 0.0));

    poly1.inners().resize(1); /*< Resize a container of interior rings. >*/
    bg::append(poly1.inners()[0], point_t(1.0, 1.0)); /*< Append point to the interior ring. >*/
    bg::append(poly1.inners()[0], point_t(4.0, 1.0));
    bg::append(poly1.inners()[0], point_t(4.0, 4.0));
    bg::append(poly1.inners()[0], point_t(1.0, 4.0));
    bg::append(poly1.inners()[0], point_t(1.0, 1.0));

    double a = bg::area(poly1);

    ex_out << a << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[polygon_output
/*`
Output:
[pre
16
]
*/
//]

TEST_CASE("example: geometries/polygon") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(16
)ex"));
}
}  // namespace ex_geometries_polygon

// libs/geometry/doc/src/examples/geometries/polyhedral_surface.cpp
namespace ex_geometries_polyhedral_surface {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 3, bg::cs::cartesian>;
    using polygon_t = bg::model::polygon<point_t>;
    using polyhedral_t = bg::model::polyhedral_surface<polygon_t>;

    polyhedral_t polyhedral0; /*< Initializing an empty polyhedral surface (default constructor) >*/

    polyhedral_t polyhedral1 = {{{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0}}},
                                {{{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}, {0, 0, 0}}},
                                {{{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}, {0, 0, 0}}},
                                {{{1, 1, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}}},
                                {{{1, 1, 1}, {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1}}},
                                {{{1, 1, 1}, {1, 1, 0}, {0, 1, 0}, {0, 1, 1}, {1, 1, 1}}}}; /*< Creating a polyhedral surface (cube) using standard initialized list >*/


    polyhedral0.resize(4);
    bg::append(polyhedral0[0].outer(), point_t(0.0, 0.0, 0.0));
    bg::append(polyhedral0[0].outer(), point_t(5.0, 0.0, 0.0));
    bg::append(polyhedral0[0].outer(), point_t(0.0, 5.0, 0.0));
    bg::append(polyhedral0[1].outer(), point_t(0.0, 0.0, 0.0));
    bg::append(polyhedral0[1].outer(), point_t(0.0, 5.0, 0.0));
    bg::append(polyhedral0[1].outer(), point_t(0.0, 0.0, 5.0));
    bg::append(polyhedral0[2].outer(), point_t(0.0, 0.0, 0.0));
    bg::append(polyhedral0[2].outer(), point_t(5.0, 0.0, 0.0));
    bg::append(polyhedral0[2].outer(), point_t(0.0, 0.0, 5.0));
    bg::append(polyhedral0[3].outer(), point_t(5.0, 0.0, 0.0));
    bg::append(polyhedral0[3].outer(), point_t(0.0, 5.0, 0.0));
    bg::append(polyhedral0[3].outer(), point_t(0.0, 0.0, 5.0)); /*< Creating a polyhedral surface (triangular pyramid) using append and resize. See figure below.>*/

    polyhedral_t polyhedral2;
    bg::read_wkt("POLYHEDRALSURFACE(((0 0 0, 0 1 0, 1 1 0, 1 0 0, 0 0 0)),\
                                    ((0 0 0, 0 1 0, 0 1 1, 0 0 1, 0 0 0)),\
                                    ((0 0 0, 1 0 0, 1 0 1, 0 0 1, 0 0 0)),\
                                    ((1 1 1, 1 0 1, 0 0 1, 0 1 1, 1 1 1)),\
                                    ((1 1 1, 1 0 1, 1 0 0, 1 1 0, 1 1 1)),\
                                    ((1 1 1, 1 1 0, 0 1 0, 0 1 1, 1 1 1)))", polyhedral2); /*< Initialize polyhedral surface (cube) with wkt >*/

    ex_out << bg::wkt(polyhedral0) << std::endl; /*< Write polyhedral surface wkt >*/
    ex_out << bg::wkt(polyhedral1) << std::endl;
    ex_out << bg::wkt(polyhedral2) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[polyhedral_surface_output
/*`
Output:
[pre
POLYHEDRALSURFACE(((0 0 0,5 0 0,0 5 0,0 0 0)),((0 0 0,0 5 0,0 0 5,0 0 0)),((0 0 0,5 0 0,0 0 5,0 0 0)),((5 0 0,0 5 0,0 0 5,5 0 0)))
POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 0,0 1 0,0 1 1,0 0 1,0 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((1 1 1,1 0 1,0 0 1,0 1 1,1 1 1)),((1 1 1,1 0 1,1 0 0,1 1 0,1 1 1)),((1 1 1,1 1 0,0 1 0,0 1 1,1 1 1)))
POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 0,0 1 0,0 1 1,0 0 1,0 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((1 1 1,1 0 1,0 0 1,0 1 1,1 1 1)),((1 1 1,1 0 1,1 0 0,1 1 0,1 1 1)),((1 1 1,1 1 0,0 1 0,0 1 1,1 1 1)))
]

[$img/geometries/triangular_pyramid.png]

*/
//]

TEST_CASE("example: geometries/polyhedral_surface") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(POLYHEDRALSURFACE(((0 0 0,5 0 0,0 5 0,0 0 0)),((0 0 0,0 5 0,0 0 5,0 0 0)),((0 0 0,5 0 0,0 0 5,0 0 0)),((5 0 0,0 5 0,0 0 5,5 0 0)))
POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 0,0 1 0,0 1 1,0 0 1,0 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((1 1 1,1 0 1,0 0 1,0 1 1,1 1 1)),((1 1 1,1 0 1,1 0 0,1 1 0,1 1 1)),((1 1 1,1 1 0,0 1 0,0 1 1,1 1 1)))
POLYHEDRALSURFACE(((0 0 0,0 1 0,1 1 0,1 0 0,0 0 0)),((0 0 0,0 1 0,0 1 1,0 0 1,0 0 0)),((0 0 0,1 0 0,1 0 1,0 0 1,0 0 0)),((1 1 1,1 0 1,0 0 1,0 1 1,1 1 1)),((1 1 1,1 0 1,1 0 0,1 1 0,1 1 1)),((1 1 1,1 1 0,0 1 0,0 1 1,1 1 1)))
)ex"));
}
}  // namespace ex_geometries_polyhedral_surface

// libs/geometry/doc/src/examples/geometries/ring.cpp
namespace ex_geometries_ring {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using ring_t = bg::model::ring<point_t>; /*< Default parameters, clockwise, closed ring. >*/

    ring_t ring1; /*< Default-construct a ring. >*/
    ring_t ring2{{0.0, 0.0}, {0.0, 5.0}, {5.0, 5.0}, {5.0, 0.0}, {0.0, 0.0}}; /*< Construct a ring containing four points plus one closing point, using C++11 unified initialization syntax. >*/

    bg::append(ring1, point_t(0.0, 0.0)); /*< Append point. >*/
    bg::append(ring1, point_t(0.0, 5.0));
    bg::append(ring1, point_t(5.0, 5.0));
    bg::append(ring1, point_t(5.0, 0.0));
    bg::append(ring1, point_t(0.0, 0.0));

    double a = bg::area(ring1);

    ex_out << a << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[ring_output
/*`
Output:
[pre
25
]
*/
//]

TEST_CASE("example: geometries/ring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(25
)ex"));
}
}  // namespace ex_geometries_ring

// libs/geometry/doc/src/examples/geometries/segment.cpp
namespace ex_geometries_segment {
namespace bg = boost::geometry;

int example_main()
{
    using point_t = bg::model::point<double, 2, bg::cs::cartesian>;
    using segment_t = bg::model::segment<point_t>;

    segment_t seg1; /*< Default-construct a segment. >*/
    segment_t seg2(point_t(0.0, 0.0), point_t(5.0, 5.0)); /*< Construct, assigning the first and the second point. >*/
    segment_t seg3{{0.0, 0.0}, {5.0, 5.0}}; /*< Construct, using C++11 unified initialization syntax. >*/

    bg::set<0, 0>(seg1, 1.0); /*< Set a coordinate. >*/
    bg::set<0, 1>(seg1, 2.0);
    bg::set<1, 0>(seg1, 3.0);
    bg::set<1, 1>(seg1, 4.0);

    double x0 = bg::get<0, 0>(seg1); /*< Get a coordinate. >*/
    double y0 = bg::get<0, 1>(seg1);
    double x1 = bg::get<1, 0>(seg1);
    double y1 = bg::get<1, 1>(seg1);

    ex_out << x0 << ", " << y0 << ", " << x1 << ", " << y1 << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[segment_output
/*`
Output:
[pre
1, 2, 3, 4
]
*/
//]

TEST_CASE("example: geometries/segment") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1, 2, 3, 4
)ex"));
}
}  // namespace ex_geometries_segment

// libs/geometry/doc/src/examples/geometries/register/box.cpp
namespace ex_geometries_register_box {
struct my_point
{
    double x, y;
};

struct my_box
{
    my_point ll, ur;
};

// Register the point type
}  // namespace ex_geometries_register_box (reopened after the registration)
#ifndef EX_REGISTERED_e65242d1ce91
#define EX_REGISTERED_e65242d1ce91
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_geometries_register_box::my_point, double, cs::cartesian, x, y)
#endif
namespace ex_geometries_register_box {

// Register the box type, also notifying that it is based on "my_point"
}  // namespace ex_geometries_register_box (reopened after the registration)
#ifndef EX_REGISTERED_551b8563a22c
#define EX_REGISTERED_551b8563a22c
BOOST_GEOMETRY_REGISTER_BOX(::ex_geometries_register_box::my_box, ::ex_geometries_register_box::my_point, ll, ur)
#endif
namespace ex_geometries_register_box {

int example_main()
{
    my_box b = boost::geometry::make<my_box>(0, 0, 2, 2);
    ex_out << "Area: "  << boost::geometry::area(b) << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[register_box_output
/*`
Output:
[pre
Area: 4
]
*/
//]

TEST_CASE("example: geometries/register/box") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 4
)ex"));
}
}  // namespace ex_geometries_register_box

// libs/geometry/doc/src/examples/geometries/register/box_2d_4values.cpp
namespace ex_geometries_register_box_2d_4values {
struct my_point
{
    int x, y;
};

struct my_box
{
    int left, top, right, bottom;
};

}  // namespace ex_geometries_register_box_2d_4values (reopened after the registration)
#ifndef EX_REGISTERED_54bc7fdb8b73
#define EX_REGISTERED_54bc7fdb8b73
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_geometries_register_box_2d_4values::my_point, int, cs::cartesian, x, y)
#endif
namespace ex_geometries_register_box_2d_4values {

// Register the box type, also notifying that it is based on "my_point"
// (even if it does not contain it)
}  // namespace ex_geometries_register_box_2d_4values (reopened after the registration)
#ifndef EX_REGISTERED_129b38be0c8c
#define EX_REGISTERED_129b38be0c8c
BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(::ex_geometries_register_box_2d_4values::my_box, ::ex_geometries_register_box_2d_4values::my_point, left, top, right, bottom)
#endif
namespace ex_geometries_register_box_2d_4values {

int example_main()
{
    my_box b = boost::geometry::make<my_box>(0, 0, 2, 2);
    ex_out << "Area: "  << boost::geometry::area(b) << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[register_box_2d_4values_output
/*`
Output:
[pre
Area: 4
]
*/
//]

TEST_CASE("example: geometries/register/box_2d_4values") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 4
)ex"));
}
}  // namespace ex_geometries_register_box_2d_4values

// libs/geometry/doc/src/examples/geometries/register/box_templated.cpp
namespace ex_geometries_register_box_templated {
template <typename P>
struct my_box
{
    P ll, ur;
};

// Register the box type
}  // namespace ex_geometries_register_box_templated (reopened after the registration)
#ifndef EX_REGISTERED_2ca0505f4cd9
#define EX_REGISTERED_2ca0505f4cd9
BOOST_GEOMETRY_REGISTER_BOX_TEMPLATED(::ex_geometries_register_box_templated::my_box, ll, ur)
#endif
namespace ex_geometries_register_box_templated {

int example_main()
{
    using box = my_box<boost::geometry::model::d2::point_xy<double>>;
    box b = boost::geometry::make<box>(0, 0, 2, 2);
    ex_out << "Area: "  << boost::geometry::area(b) << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[register_box_templated_output
/*`
Output:
[pre
Area: 4
]
*/
//]

TEST_CASE("example: geometries/register/box_templated") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 4
)ex"));
}
}  // namespace ex_geometries_register_box_templated

// libs/geometry/doc/src/examples/geometries/register/linestring.cpp
namespace ex_geometries_register_linestring {
using point_2d = boost::geometry::model::d2::point_xy<double>;

}  // namespace ex_geometries_register_linestring (reopened after the registration)
#ifndef EX_REGISTERED_25f7717c40bd
#define EX_REGISTERED_25f7717c40bd
BOOST_GEOMETRY_REGISTER_LINESTRING(std::vector<::ex_geometries_register_linestring::point_2d>)
#endif
namespace ex_geometries_register_linestring {

int example_main()
{
    // Normal usage of std::
    std::vector<point_2d> line;
    line.push_back(point_2d(1, 1));
    line.push_back(point_2d(2, 2));
    line.push_back(point_2d(3, 1));

    // Usage of Boost.Geometry's length and wkt functions
    ex_out << "Length: "
        << boost::geometry::length(line)
        << std::endl;

    ex_out << "WKT: "
        << boost::geometry::wkt(line)
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[register_linestring_output
/*`
Output:
[pre
Length: 2.82843
WKT: LINESTRING(1 1,2 2,3 1)
]
*/
//]

TEST_CASE("example: geometries/register/linestring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Length: 2.82843
WKT: LINESTRING(1 1,2 2,3 1)
)ex"));
}
}  // namespace ex_geometries_register_linestring

// libs/geometry/doc/src/examples/geometries/register/linestring_templated.cpp
namespace ex_geometries_register_linestring_templated {
}  // namespace ex_geometries_register_linestring_templated (reopened after the registration)
#ifndef EX_REGISTERED_bd72d1c8d0e5
#define EX_REGISTERED_bd72d1c8d0e5
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)
#endif
namespace ex_geometries_register_linestring_templated {

int example_main()
{
    std::deque<boost::geometry::model::d2::point_xy<double>> line(2);
    boost::geometry::assign_values(line[0], 1, 1);
    boost::geometry::assign_values(line[1], 2, 2);

    // Boost.Geometry algorithms work on any deque now
    ex_out << "Length: "  << boost::geometry::length(line) << std::endl;
    ex_out << "Line: "  << boost::geometry::dsv(line) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[register_linestring_templated_output
/*`
Output:
[pre
Length: 1.41421
Line: ((1, 1), (2, 2))
]
*/
//]

TEST_CASE("example: geometries/register/linestring_templated") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Length: 1.41421
Line: ((1, 1), (2, 2))
)ex"));
}
}  // namespace ex_geometries_register_linestring_templated

// libs/geometry/doc/src/examples/geometries/register/point.cpp
namespace ex_geometries_register_point {
struct legacy_point
{
    double x, y;
};

}  // namespace ex_geometries_register_point (reopened after the registration)
#ifndef EX_REGISTERED_b96d498f7139
#define EX_REGISTERED_b96d498f7139
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_geometries_register_point::legacy_point, double, cs::cartesian, x, y)
#endif
namespace ex_geometries_register_point { /*< The magic: adapt it to Boost.Geometry Point Concept >*/

int example_main()
{
    legacy_point p1, p2;

    namespace bg = boost::geometry;

    /*< Any Boost.Geometry function can be used for legacy point now. Here: assign_values and distance >*/
    bg::assign_values(p1, 1, 1);
    bg::assign_values(p2, 2, 2);

    double d = bg::distance(p1, p2);

    ex_out << "Distance: " << d << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[register_point_2d_output
/*`
Output:
[pre
Distance: 1.41421
]
*/
//]

TEST_CASE("example: geometries/register/point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Distance: 1.41421
)ex"));
}
}  // namespace ex_geometries_register_point

// libs/geometry/doc/src/examples/geometries/adapted/boost_array.cpp
namespace ex_geometries_adapted_boost_array {
}  // namespace ex_geometries_adapted_boost_array (reopened after the registration)
#ifndef EX_REGISTERED_5e756957de5c
#define EX_REGISTERED_5e756957de5c
BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(cs::cartesian)
#endif
namespace ex_geometries_adapted_boost_array {

int example_main()
{
    boost::array<float, 2> a = { {1, 2} };
    boost::array<double, 2> b = { {2, 3} };
    ex_out << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::set<0>(a, 1.1f);
    boost::geometry::set<1>(a, 2.2f);
    ex_out << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::assign_values(b, 2.2, 3.3);
    ex_out << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::model::linestring<boost::array<double, 2>> line;
    line.push_back(b);

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_array_output
/*`
Output:
[pre
1.41421
1.20416
1.55563
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_array") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1.41421
1.20416
1.55563
)ex"));
}
}  // namespace ex_geometries_adapted_boost_array

// libs/geometry/doc/src/examples/geometries/adapted/boost_polygon_box.cpp
namespace ex_geometries_adapted_boost_polygon_box {
int example_main()
{
    using rect = boost::polygon::rectangle_data<int>;

    rect b = boost::polygon::construct<rect>(1, 2, 3, 4);

    ex_out << "Area (using Boost.Geometry): "
        << boost::geometry::area(b) << std::endl;
    ex_out << "Area (using Boost.Polygon): "
        << boost::polygon::area(b) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_polygon_box_output
/*`
Output:
[pre
Area (using Boost.Geometry): 4
Area (using Boost.Polygon): 4
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_polygon_box") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area (using Boost.Geometry): 4
Area (using Boost.Polygon): 4
)ex"));
}
}  // namespace ex_geometries_adapted_boost_polygon_box

// libs/geometry/doc/src/examples/geometries/adapted/boost_polygon_point.cpp
namespace ex_geometries_adapted_boost_polygon_point {
int example_main()
{
    boost::polygon::point_data<int> a(1, 2), b(3, 4);
    ex_out << "Distance (using Boost.Geometry): "
        << boost::geometry::distance(a, b) << std::endl;
    ex_out << "Distance (using Boost.Polygon): "
        << boost::polygon::euclidean_distance(a, b) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_polygon_point_output
/*`
Output:
[pre
Distance (using Boost.Geometry): 2.82843
Distance (using Boost.Polygon): 2.82843
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_polygon_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Distance (using Boost.Geometry): 2.82843
Distance (using Boost.Polygon): 2.82843
)ex"));
}
}  // namespace ex_geometries_adapted_boost_polygon_point

// libs/geometry/doc/src/examples/geometries/adapted/boost_polygon_polygon.cpp
namespace ex_geometries_adapted_boost_polygon_polygon {
int example_main()
{
    using polygon = boost::polygon::polygon_with_holes_data<int>;
    using point = boost::polygon::polygon_traits<polygon>::point_type;
    using hole = boost::polygon::polygon_with_holes_traits<polygon>::hole_type;

    point pts[5] = {
        boost::polygon::construct<point>(0, 0),
        boost::polygon::construct<point>(0, 10),
        boost::polygon::construct<point>(10, 10),
        boost::polygon::construct<point>(10, 0),
        boost::polygon::construct<point>(0, 0)
    };
    point hole_pts[5] = {
        boost::polygon::construct<point>(1, 1),
        boost::polygon::construct<point>(9, 1),
        boost::polygon::construct<point>(9, 9),
        boost::polygon::construct<point>(1, 9),
        boost::polygon::construct<point>(1, 1)
    };

    hole hls[1];
    boost::polygon::set_points(hls[0], hole_pts, hole_pts+5);

    polygon poly;
    boost::polygon::set_points(poly, pts, pts+5);
    boost::polygon::set_holes(poly, hls, hls+1);
    
    ex_out << "Area (using Boost.Geometry): "
        << boost::geometry::area(poly) << std::endl;
    ex_out << "Area (using Boost.Polygon): "
        << boost::polygon::area(poly) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_polygon_polygon_output
/*`
Output:
[pre
Area (using Boost.Geometry): 36
Area (using Boost.Polygon): 36
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_polygon_polygon") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area (using Boost.Geometry): 36
Area (using Boost.Polygon): 36
)ex"));
}
}  // namespace ex_geometries_adapted_boost_polygon_polygon

// libs/geometry/doc/src/examples/geometries/adapted/boost_polygon_ring.cpp
namespace ex_geometries_adapted_boost_polygon_ring {
int example_main()
{
    using polygon = boost::polygon::polygon_data<int>;
    using point = boost::polygon::polygon_traits<polygon>::point_type;

    point pts[5] = {
        boost::polygon::construct<point>(0, 0),
        boost::polygon::construct<point>(0, 10),
        boost::polygon::construct<point>(10, 10),
        boost::polygon::construct<point>(10, 0),
        boost::polygon::construct<point>(0, 0)
    };

    polygon poly;
    boost::polygon::set_points(poly, pts, pts+5);
    
    ex_out << "Area (using Boost.Geometry): "
        << boost::geometry::area(poly) << std::endl;
    ex_out << "Area (using Boost.Polygon): "
        << boost::polygon::area(poly) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_polygon_ring_output
/*`
Output:
[pre
Area (using Boost.Geometry): 100
Area (using Boost.Polygon): 100
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_polygon_ring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area (using Boost.Geometry): 100
Area (using Boost.Polygon): 100
)ex"));
}
}  // namespace ex_geometries_adapted_boost_polygon_ring

// libs/geometry/doc/src/examples/geometries/adapted/boost_tuple.cpp
namespace ex_geometries_adapted_boost_tuple {
}  // namespace ex_geometries_adapted_boost_tuple (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_geometries_adapted_boost_tuple {

int example_main()
{
    boost::geometry::model::polygon<boost::tuple<double, double>> poly;
    poly.outer().push_back(boost::make_tuple(1.0, 2.0));
    poly.outer().push_back(boost::make_tuple(6.0, 4.0));
    poly.outer().push_back(boost::make_tuple(5.0, 1.0));
    poly.outer().push_back(boost::make_tuple(1.0, 2.0));

    ex_out << "Area: " << boost::geometry::area(poly) << std::endl;
    ex_out << "Contains (1.5, 2.5): "
        << std::boolalpha
        << boost::geometry::within(boost::make_tuple(1.5, 2.5), poly)
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_tuple_output
/*`
Output:
[pre
Area: 6.5
Contains (1.5, 2.5): false
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_tuple") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 6.5
Contains (1.5, 2.5): false
)ex"));
}
}  // namespace ex_geometries_adapted_boost_tuple

// libs/geometry/doc/src/examples/geometries/adapted/c_array.cpp
namespace ex_geometries_adapted_c_array {
}  // namespace ex_geometries_adapted_c_array (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_geometries_adapted_c_array {

int example_main()
{
    int a[3] = {1, 2, 3};
    int b[3] = {2, 3, 4};

    ex_out << boost::geometry::distance(a, b) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[c_array_output
/*`
Output:
[pre
1.73205
]
*/
//]

TEST_CASE("example: geometries/adapted/c_array") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1.73205
)ex"));
}
}  // namespace ex_geometries_adapted_c_array

// libs/geometry/doc/src/examples/geometries/adapted/std_array.cpp
namespace ex_geometries_adapted_std_array {
}  // namespace ex_geometries_adapted_std_array (reopened after the registration)
#ifndef EX_REGISTERED_4a408ec89456
#define EX_REGISTERED_4a408ec89456
BOOST_GEOMETRY_REGISTER_STD_ARRAY_CS(cs::cartesian)
#endif
namespace ex_geometries_adapted_std_array {

int example_main()
{
    std::array<float, 2> a = { {1, 2} };
    std::array<double, 2> b = { {2, 3} };
    ex_out << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::set<0>(a, 1.1f);
    boost::geometry::set<1>(a, 2.2f);
    ex_out << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::assign_values(b, 2.2, 3.3);
    ex_out << boost::geometry::distance(a, b) << std::endl;

    boost::geometry::model::linestring<std::array<double, 2>> line;
    line.push_back(b);

    return 0;
    return 0;  // main's implicit return
}

//]

//[std_array_output
/*`
Output:
[pre
1.41421
1.20416
1.55563
]
*/
//]

TEST_CASE("example: geometries/adapted/std_array") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1.41421
1.20416
1.55563
)ex"));
}
}  // namespace ex_geometries_adapted_std_array

// libs/geometry/doc/src/examples/geometries/adapted/boost_range/filtered.cpp
namespace ex_geometries_adapted_boost_range_filtered {
struct not_two
{
    template <typename P>
    bool operator()(P const& p) const
    {
        return boost::geometry::get<1>(p) != 2;
    }
};


int example_main()
{
    using xy = boost::geometry::model::d2::point_xy<int>;
    boost::geometry::model::linestring<xy> line;
    line.push_back(xy(0, 0));
    line.push_back(xy(1, 1));
    line.push_back(xy(2, 2));
    line.push_back(xy(3, 1));
    line.push_back(xy(4, 0));
    line.push_back(xy(5, 1));
    line.push_back(xy(6, 2));
    line.push_back(xy(7, 1));
    line.push_back(xy(8, 0));

    using boost::adaptors::filtered;
    ex_out
        << boost::geometry::length(line) << std::endl
        << boost::geometry::length(line | filtered(not_two())) << std::endl
        << boost::geometry::dsv(line | filtered(not_two())) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_range_filtered_output
/*`
Output:
[pre
11.3137
9.65685
((0, 0), (1, 1), (3, 1), (4, 0), (5, 1), (7, 1), (8, 0))
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_range/filtered") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(11.3137
9.65685
((0, 0), (1, 1), (3, 1), (4, 0), (5, 1), (7, 1), (8, 0))
)ex"));
}
}  // namespace ex_geometries_adapted_boost_range_filtered

// libs/geometry/doc/src/examples/geometries/adapted/boost_range/reversed.cpp
namespace ex_geometries_adapted_boost_range_reversed {
int example_main()
{
    using xy = boost::geometry::model::d2::point_xy<int>;
    boost::geometry::model::linestring<xy> line;
    line.push_back(xy(0, 0));
    line.push_back(xy(1, 1));

    ex_out
        << boost::geometry::dsv(line | boost::adaptors::reversed)
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_range_reversed_output
/*`
Output:
[pre
((1, 1), (0, 0))
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_range/reversed") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((1, 1), (0, 0))
)ex"));
}
}  // namespace ex_geometries_adapted_boost_range_reversed

// libs/geometry/doc/src/examples/geometries/adapted/boost_range/sliced.cpp
namespace ex_geometries_adapted_boost_range_sliced {
int example_main()
{
    using xy = boost::geometry::model::d2::point_xy<int>;
    boost::geometry::model::linestring<xy> line = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}};

    ex_out
        << boost::geometry::dsv(line | boost::adaptors::sliced(1, 3)) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_range_sliced_output
/*`
Output:
[pre
((1, 1), (2, 2))
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_range/sliced") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((1, 1), (2, 2))
)ex"));
}
}  // namespace ex_geometries_adapted_boost_range_sliced

// libs/geometry/doc/src/examples/geometries/adapted/boost_range/strided.cpp
namespace ex_geometries_adapted_boost_range_strided {
int example_main()
{
    using boost::adaptors::strided;

    using xy = boost::geometry::model::d2::point_xy<int>;
    boost::geometry::model::ring<xy> ring {{0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 0}};

    boost::geometry::correct(ring);

    ex_out
        << "Normal : " << boost::geometry::dsv(ring) << std::endl
        << "Strided: " << boost::geometry::dsv(ring | strided(2)) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[boost_range_strided_output
/*`
Output:
[pre
Normal : ((0, 0), (0, 1), (0, 2), (1, 2), (2, 2), (2, 0), (0, 0))
Strided: ((0, 0), (0, 2), (2, 2), (0, 0))
]
*/
//]

TEST_CASE("example: geometries/adapted/boost_range/strided") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Normal : ((0, 0), (0, 1), (0, 2), (1, 2), (2, 2), (2, 0), (0, 0))
Strided: ((0, 0), (0, 2), (2, 2), (0, 0))
)ex"));
}
}  // namespace ex_geometries_adapted_boost_range_strided
