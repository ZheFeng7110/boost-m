// mcpplibs.mwrap.boost.geometry — the documentation's algorithm examples
// (libs/geometry/doc/src/examples/algorithms/) ported mechanically: each program runs as a case
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

#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
// the macros of <boost/geometry/geometries/adapted/boost_range/filtered.hpp> (its declarations are in the module)

#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/geometry/geometries/register/point.hpp>
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

import std;
import mcpplibs.mwrap.boost.geometry;
import mcpplibs.mwrap.boost.algorithm;
import mcpplibs.mwrap.boost.core;

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

// libs/geometry/doc/src/examples/algorithms/append.cpp
namespace ex_algorithms_append {
}  // namespace ex_algorithms_append (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_append {

int example_main()
{
    using boost::geometry::append;

    using vector = std::vector<boost::tuple<int, int>>;
    using polygon = boost::geometry::model::polygon<boost::tuple<int, int>>;

    polygon poly;

    // Append a range
    append(poly, vector{{0, 0}, {0, 10}, {11, 11}, {10, 0}}); /*< vector models a range and can therefore be used in boost::geometry::append >*/
    // Append a point (in this case the closing point)
    append(poly, boost::make_tuple(0, 0));

    // Create an interior ring (append does not do this automatically)
    boost::geometry::interior_rings(poly).resize(1);

    // Append a range to the interior ring
    append(poly, vector{{2, 2}, {2, 5}, {6, 6}, {5, 2}}, 0); /*< The last parameter ring_index 0 denotes the first interior ring >*/

    // Append a point to the first interior ring
    append(poly, boost::make_tuple(2, 2), 0);

    ex_out << boost::geometry::dsv(poly) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[append_output
/*`
Output:
[pre
(((0, 0), (0, 10), (11, 11), (10, 0), (0, 0)), ((2, 2), (2, 5), (6, 6), (5, 2), (2, 2)))
]
*/
//]

TEST_CASE("example: algorithms/append") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((((0, 0), (0, 10), (11, 11), (10, 0), (0, 0)), ((2, 2), (2, 5), (6, 6), (5, 2), (2, 2)))
)ex"));
}
}  // namespace ex_algorithms_append

// libs/geometry/doc/src/examples/algorithms/area.cpp
namespace ex_algorithms_area {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Calculate the area of a cartesian polygon
    bg::model::polygon<bg::model::d2::point_xy<double>> poly;
    bg::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly);
    double area = bg::area(poly);
    ex_out << "Area: " << area << std::endl;

    // Calculate the area of a spherical equatorial polygon
    bg::model::polygon<bg::model::point<float, 2, bg::cs::spherical_equatorial<bg::degree>>> sph_poly;
    bg::read_wkt("POLYGON((0 0,0 45,45 0,0 0))", sph_poly);
    area = bg::area(sph_poly);
    ex_out << "Area: " << area << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[area_output
/*`
Output:
[pre
Area: 16
Area: 0.339837
]
*/
//]

TEST_CASE("example: algorithms/area") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 16
Area: 0.339837
)ex"));
}
}  // namespace ex_algorithms_area

// libs/geometry/doc/src/examples/algorithms/area_with_strategy.cpp
namespace ex_algorithms_area_with_strategy {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Create spherical polygon
    bg::model::polygon<bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>> sph_poly;
    bg::read_wkt("POLYGON((0 0,0 1,1 0,0 0))", sph_poly);

    // Create spherical strategy with mean Earth radius in meters
    bg::strategy::area::spherical<> sph_strategy(6371008.8);

    // Calculate the area of a spherical polygon
    double area = bg::area(sph_poly, sph_strategy);
    ex_out << "Area: " << area << std::endl;

    // Create geographic polygon
    bg::model::polygon<bg::model::point<double, 2, bg::cs::geographic<bg::degree>>> geo_poly;
    bg::read_wkt("POLYGON((0 0,0 1,1 0,0 0))", geo_poly);

    // Create geographic strategy with WGS84 spheroid
    bg::srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);
    bg::strategy::area::geographic<> geo_strategy(spheroid);

    // Calculate the area of a geographic polygon
    area = bg::area(geo_poly, geo_strategy);
    ex_out << "Area: " << area << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[area_with_strategy_output
/*`
Output:
[pre
Area: 6.18249e+09
Area: 6.15479e+09
]
*/
//]

TEST_CASE("example: algorithms/area_with_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 6.18249e+09
Area: 6.15486e+09
)ex"));
}
}  // namespace ex_algorithms_area_with_strategy

// libs/geometry/doc/src/examples/algorithms/assign.cpp
namespace ex_algorithms_assign {
}  // namespace ex_algorithms_assign (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_assign {

int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using box = boost::geometry::model::box<point>;
    using polygon = boost::geometry::model::polygon<point>;

    point p1;
    box b;
    boost::geometry::assign_values(p1, 1, 1);
    boost::geometry::assign_values(b, 1, 1, 2, 2);

    // Assign a box to a polygon (target = source)
    polygon p;
    boost::geometry::assign(p, b);

    // Assign a point to another point type (conversion of point-type)
    boost::tuple<double, double> p2;
    boost::geometry::assign(p2, p1);

    using boost::geometry::dsv;
    ex_out
        << "box: " << dsv(b) << std::endl
        << "polygon: " << dsv(p) << std::endl
        << "point: " << dsv(p1) << std::endl
        << "point tuples: " << dsv(p2) << std::endl
        ;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_output
/*`
Output:
[pre
box: ((1, 1), (2, 2))
polygon: (((1, 1), (1, 2), (2, 2), (2, 1), (1, 1)))
point: (1, 1)
point tuples: (1, 1)
]
*/
//]

TEST_CASE("example: algorithms/assign") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(box: ((1, 1), (2, 2))
polygon: (((1, 1), (1, 2), (2, 2), (2, 1), (1, 1)))
point: (1, 1)
point tuples: (1, 1)
)ex"));
}
}  // namespace ex_algorithms_assign

// libs/geometry/doc/src/examples/algorithms/assign_2d_point.cpp
namespace ex_algorithms_assign_2d_point {
int example_main()
{
    using boost::geometry::assign_values;


    boost::geometry::model::d2::point_xy<double> p1;
    assign_values(p1, 1.2345, 2.3456);

    ex_out
        << std::setprecision(20)
        << boost::geometry::dsv(p1) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_2d_point_output
/*`
Output:
[pre
(1.2344999999999999, 2.3456000000000001)
(1.2345, 2.3456)
]
*/
//]

TEST_CASE("example: algorithms/assign_2d_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((1.2344999999999999307, 2.3456000000000001293)
)ex"));
}
}  // namespace ex_algorithms_assign_2d_point

// libs/geometry/doc/src/examples/algorithms/assign_3d_point.cpp
namespace ex_algorithms_assign_3d_point {
int example_main()
{
    boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian> p;
    boost::geometry::assign_values(p, 1.2345, 2.3456, 3.4567);

    ex_out << boost::geometry::dsv(p) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_3d_point_output
/*`
Output:
[pre
(1.2345, 2.3456, 3.4567)
]
*/
//]

TEST_CASE("example: algorithms/assign_3d_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((1.2345, 2.3456, 3.4567)
)ex"));
}
}  // namespace ex_algorithms_assign_3d_point

// libs/geometry/doc/src/examples/algorithms/assign_box_corners.cpp
namespace ex_algorithms_assign_box_corners {
using namespace boost::geometry;

int example_main()
{
    using point = model::d2::point_xy<double>;
    using box = model::box<point>;

    box b;
    assign_values(b, 2, 2, 5, 5);

    point ll, lr, ul, ur;
    detail::assign_box_corners(b, ll, lr, ul, ur);

    ex_out << "box: " << dsv(b) << std::endl << std::endl;

    ex_out << dsv(ul) << " --- " << dsv(ur) << std::endl;
    for (int i = 0; i < 3; i++)
    {
        ex_out << "  |          |" << std::endl;
    }
    ex_out << dsv(ll) << " --- " << dsv(lr) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_box_corners_output
/*`
Output:
[pre
box: ((2, 2), (5, 5))

(2, 5) --- (5, 5)
  |          |
  |          |
  |          |
(2, 2) --- (5, 2)
]
*/
//]

TEST_CASE("example: algorithms/assign_box_corners") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(box: ((2, 2), (5, 5))

(2, 5) --- (5, 5)
  |          |
  |          |
  |          |
(2, 2) --- (5, 2)
)ex"));
}
}  // namespace ex_algorithms_assign_box_corners

// libs/geometry/doc/src/examples/algorithms/assign_inverse.cpp
namespace ex_algorithms_assign_inverse {
using namespace boost::geometry;

int example_main()
{
    using point = model::point<float, 3, cs::cartesian>;
    using box = model::box<point>;

    box all;
    assign_inverse(all);
    ex_out << dsv(all) << std::endl;
    expand(all, point(0, 0, 0));
    expand(all, point(1, 2, 3));
    ex_out << dsv(all) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_inverse_output
/*`
Output:
[pre
((3.40282e+038, 3.40282e+038, 3.40282e+038), (-3.40282e+038, -3.40282e+038, -3.40282e+038))
((0, 0, 0), (1, 2, 3))]
*/
//]

TEST_CASE("example: algorithms/assign_inverse") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((3.40282e+38, 3.40282e+38, 3.40282e+38), (-3.40282e+38, -3.40282e+38, -3.40282e+38))
((0, 0, 0), (1, 2, 3))
)ex"));
}
}  // namespace ex_algorithms_assign_inverse

// libs/geometry/doc/src/examples/algorithms/assign_point_from_index.cpp
namespace ex_algorithms_assign_point_from_index {
using namespace boost::geometry;

int example_main()
{
    using point = model::d2::point_xy<double>;
    using segment = model::segment<point>;

    segment s;
    assign_values(s, 1, 1, 2, 2);

    point first, second;
    detail::assign_point_from_index<0>(s, first);
    detail::assign_point_from_index<1>(s, second);
    ex_out
        << "segment: " << dsv(s) << std::endl
        << "first: " << dsv(first) << std::endl
        << "second: " << dsv(second) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_point_from_index_output
/*`
Output:
[pre
segment: ((1, 1), (2, 2))
first: (1, 1)
second: (2, 2)
]
*/
//]

TEST_CASE("example: algorithms/assign_point_from_index") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(segment: ((1, 1), (2, 2))
first: (1, 1)
second: (2, 2)
)ex"));
}
}  // namespace ex_algorithms_assign_point_from_index

// libs/geometry/doc/src/examples/algorithms/assign_point_to_index.cpp
namespace ex_algorithms_assign_point_to_index {
using namespace boost::geometry;

int example_main()
{
    using point = model::d2::point_xy<int>;
    using box = model::box<point>;

    point lower_left(0, 0), upper_right(2, 2);

    box b;
    detail::assign_point_to_index<0>(lower_left, b);
    detail::assign_point_to_index<1>(upper_right, b);
    ex_out << "box: " << dsv(b) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_point_to_index_output
/*`
Output:
[pre
box: ((0, 0), (2, 2))
]
*/
//]

TEST_CASE("example: algorithms/assign_point_to_index") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(box: ((0, 0), (2, 2))
)ex"));
}
}  // namespace ex_algorithms_assign_point_to_index

// libs/geometry/doc/src/examples/algorithms/assign_points.cpp
namespace ex_algorithms_assign_points {
}  // namespace ex_algorithms_assign_points (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_assign_points {

template <typename T>
struct x_between
{
    x_between(T a, T b)
        : fa(a), fb(b)
    {}

    template <typename P>
    bool operator()(P const& p) const
    {
        return boost::geometry::get<0>(p) >= fa
            && boost::geometry::get<0>(p) <= fb;
    }
private :
    T fa, fb;
};


int example_main()
{
    using ls = boost::geometry::model::linestring<boost::tuple<int, int>>;

    ls line1, line2, line3;

    line1 = {{0, 0}, {2, 3}, {4, 0}, {6, 3}, {8, 0}, {10, 3}, {12, 0}};
    boost::geometry::assign_points(line2, ls({{0, 0}, {2, 2}, {4, 0}, {6, 2}, {8, 0}}));
    boost::geometry::assign_points(line3, line1 | boost::adaptors::filtered(x_between<int>(4, 8))); /*< Boost.Range adaptors can also be used in boost::geometry::assign >*/

    ex_out << "line 1: " << boost::geometry::dsv(line1) << std::endl;
    ex_out << "line 2: " << boost::geometry::dsv(line2) << std::endl;
    ex_out << "line 3: " << boost::geometry::dsv(line3) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[assign_points_output
/*`
Output:
[pre
line 1: ((0, 0), (2, 3), (4, 0), (6, 3), (8, 0), (10, 3), (12, 0))
line 2: ((0, 0), (2, 2), (4, 0), (6, 2), (8, 0))
line 3: ((4, 0), (6, 3), (8, 0))
]
*/
//]

TEST_CASE("example: algorithms/assign_points") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(line 1: ((0, 0), (2, 3), (4, 0), (6, 3), (8, 0), (10, 3), (12, 0))
line 2: ((0, 0), (2, 2), (4, 0), (6, 2), (8, 0))
line 3: ((4, 0), (6, 3), (8, 0))
)ex"));
}
}  // namespace ex_algorithms_assign_points

// libs/geometry/doc/src/examples/algorithms/azimuth.cpp
namespace ex_algorithms_azimuth {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;

    point_type p1(0, 0);
    point_type p2(1, 1);

    auto azimuth = boost::geometry::azimuth(p1, p2);

    ex_out << "azimuth: " << azimuth << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[azimuth_output
/*`
Output:
[pre
azimuth: 0.785398
]
*/
//]

TEST_CASE("example: algorithms/azimuth") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(azimuth: 0.785398
)ex"));
}
}  // namespace ex_algorithms_azimuth

// libs/geometry/doc/src/examples/algorithms/azimuth_strategy.cpp
namespace ex_algorithms_azimuth_strategy {
int example_main()
{
    namespace bg = boost::geometry;
    using point_type = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;

    point_type p1(0, 0);
    point_type p2(1, 1);

    bg::srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);
    bg::strategies::azimuth::geographic<> strategy(spheroid);

    auto azimuth = boost::geometry::azimuth(p1, p2, strategy);

    ex_out << "azimuth: " << azimuth << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[azimuth_strategy_output
/*`
Output:
[pre
azimuth: 0.788674
]
*/
//]

TEST_CASE("example: algorithms/azimuth_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(azimuth: 0.788674
)ex"));
}
}  // namespace ex_algorithms_azimuth_strategy

// libs/geometry/doc/src/examples/algorithms/buffer_with_strategies.cpp
namespace ex_algorithms_buffer_with_strategies {
#undef CREATE_SVG_BUFFER_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2014 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for buffer examples

#ifndef CREATE_SVG_BUFFER_HPP
#define CREATE_SVG_BUFFER_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg_buffer(std::string const& filename, Geometry1 const& original, Geometry2 const& buffer)
{
#if defined(HAVE_SVG)
    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(original);
    mapper.add(buffer);

    // Draw buffer at bottom
    mapper.map(buffer, "fill-opacity:0.6;fill:rgb(255,255,64);stroke:rgb(255,128,0);stroke-width:3");

    // Draw original on top
    mapper.map(original, "fill-opacity:0.6;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");

#else
    boost::ignore_unused(filename, original, buffer);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/...


#endif // CREATE_SVG_BUFFER_HPP

// ---- end ../examples_utils/create_svg_buffer.hpp ----

int example_main()
{
    using coordinate_type = double;
    using point = boost::geometry::model::d2::point_xy<coordinate_type>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare strategies
    const double buffer_distance = 1.0;
    const int points_per_circle = 36;
    boost::geometry::strategy::buffer::distance_symmetric<coordinate_type> distance_strategy(buffer_distance);
    boost::geometry::strategy::buffer::join_round join_strategy(points_per_circle);
    boost::geometry::strategy::buffer::end_round end_strategy(points_per_circle);
    boost::geometry::strategy::buffer::point_circle circle_strategy(points_per_circle);
    boost::geometry::strategy::buffer::side_straight side_strategy;

    // Declare output
    boost::geometry::model::multi_polygon<polygon> result;

    // Declare/fill a linestring
    boost::geometry::model::linestring<point> ls;
    boost::geometry::read_wkt("LINESTRING(0 0,4 5,7 4,10 6)", ls);

    // Create the buffer of a linestring
    boost::geometry::buffer(ls, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, circle_strategy);
     create_svg_buffer("buffer_linestring.svg", ls, result); 

    // Declare/fill a multi point
    boost::geometry::model::multi_point<point> mp;
    boost::geometry::read_wkt("MULTIPOINT((3 3),(4 4),(6 2))", mp);

    // Create the buffer of a multi point
    boost::geometry::buffer(mp, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, circle_strategy);
     create_svg_buffer("buffer_multi_point.svg", mp, result); 

    // Declare/fill a multi_polygon
    boost::geometry::model::multi_polygon<polygon> mpol;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 1,2 5,5 3,0 1)),((1 1,5 2,5 0,1 1)))", mpol);

    // Create the buffer of a multi polygon
    boost::geometry::buffer(mpol, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, circle_strategy);
     create_svg_buffer("buffer_multi_polygon.svg", mpol, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: algorithms/buffer_with_strategies") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_algorithms_buffer_with_strategies

// libs/geometry/doc/src/examples/algorithms/centroid.cpp
namespace ex_algorithms_centroid {
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);

    point_type p;
    boost::geometry::centroid(poly, p);

    ex_out << "centroid: " << boost::geometry::dsv(p) << std::endl;
     create_svg("centroid.svg", poly, p); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[centroid_output
/*`
Output:
[pre
centroid: (4.04663, 1.6349)
[$img/algorithms/centroid.png]
]
Note that the centroid might be located in a hole or outside a polygon, easily.
*/
//]

TEST_CASE("example: algorithms/centroid") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(centroid: (4.04663, 1.6349)
)ex"));
}
}  // namespace ex_algorithms_centroid

// libs/geometry/doc/src/examples/algorithms/clear.cpp
namespace ex_algorithms_clear {
}  // namespace ex_algorithms_clear (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_clear {

int example_main()
{
    using point = boost::tuple<float, float>;
    using polygon = boost::geometry::model::polygon<point>;
    using ring = boost::geometry::model::ring<point>;

    // Create a square as exterior ring, with a triangle as interior ring
    polygon poly{{{0, 0}, {0, 10}, {10, 10}, {10, 0}, {0, 0}}, {{{1, 2}, {8, 2}, {4, 6}, {1, 2}}}};

    ex_out << boost::geometry::dsv(poly) << std::endl;
    boost::geometry::clear(poly);
    ex_out << boost::geometry::dsv(poly) << std::endl;

    // Create a triangle
    ring r{{0, 0}, {0, 9}, {8, 8}, {0, 0}};

    ex_out << boost::geometry::dsv(r) << std::endl;
    boost::geometry::clear(r);
    ex_out << boost::geometry::dsv(r) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[clear_output
/*`
Output:
[pre
(((0, 0), (0, 10), (10, 10), (10, 0), (0, 0)), ((1, 2), (8, 2), (4, 6), (1, 2)))
(())
((0, 0), (0, 9), (8, 8), (0, 0))
()
]
*/
//]

TEST_CASE("example: algorithms/clear") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((((0, 0), (0, 10), (10, 10), (10, 0), (0, 0)), ((1, 2), (8, 2), (4, 6), (1, 2)))
(())
((0, 0), (0, 9), (8, 8), (0, 0))
()
)ex"));
}
}  // namespace ex_algorithms_clear

// libs/geometry/doc/src/examples/algorithms/closest_points.cpp
namespace ex_algorithms_closest_points {
#undef CREATE_SVG_CLOSEST_POINTS_HPP
// Boost.Geometry
// QuickBook Example

// Copyright (c) 2021, Oracle and/or its affiliates
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for closest_points examples

#ifndef CREATE_SVG_CLOSEST_POINTS_HPP
#define CREATE_SVG_CLOSEST_POINTS_HPP


template
<
    typename Point,
    typename Line,
    typename Polygon,
    typename Multipoint,
    typename Segment
>
void create_svg_closest_points(Point const& p0,
                               Point const& p,
                               Line const& line,
                               Polygon const& poly,
                               Multipoint const& mp,
                               Segment const& sout1,
                               Segment const& sout2,
                               Segment const& sout3)
{
    // Declare a stream and an SVG mapper
    std::ofstream svg("closest_points_example.svg");
    boost::geometry::svg_mapper<Point> mapper(svg, 400, 400);

    // Add geometries such that all these geometries fit on the map

    mapper.add(p0);
    mapper.add(p);
    mapper.add(poly);
    mapper.add(line);
    mapper.add(mp);
    mapper.add(sout1);
    mapper.add(sout2);
    mapper.add(sout3);

    // Draw the geometries on the SVG map, using a specific SVG style
    mapper.map(sout1, "opacity:1.0;fill:rgb(255,0,0);stroke:rgb(0,0,0);stroke-width:1;stroke-dasharray:1,2");
    mapper.map(sout2, "opacity:1.0;fill:rgb(255,0,0);stroke:rgb(0,0,0);stroke-width:1;stroke-dasharray:1,2");
    mapper.map(sout3, "opacity:1.0;fill:rgb(255,0,0);stroke:rgb(0,0,0);stroke-width:1;stroke-dasharray:1,2");
    mapper.map(p, "opacity:1.0;fill:rgb(255,0,0);stroke:rgb(255,0,0);stroke-width:1");
    mapper.map(poly, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    mapper.map(line, "opacity:1.0;fill:none;stroke:rgb(0,0,255);stroke-width:2");
    mapper.map(mp, "opacity:2.0;fill:rgb(0,255,255);stroke:rgb(0,255,255);stroke-width:.5");
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/...

#endif // CREATE_SVG_CLOSEST_POINTS_HPP
// ---- end ../examples_utils/create_svg_closest_points.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;
    using linestring_type = boost::geometry::model::linestring<point_type>;
    using multi_point_type = boost::geometry::model::multi_point<point_type>;

    point_type p(4.3,1.9);
    point_type p0(0,0);
    polygon_type poly;
    linestring_type line;
    multi_point_type mp;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
        "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);
    line.push_back(point_type(5,0));
    line.push_back(point_type(2,0));
    mp.push_back(point_type(2,0.5));
    mp.push_back(point_type(2,3));

    boost::geometry::model::segment<point_type> sout1,sout2,sout3;

    boost::geometry::closest_points(p, poly, sout1);
    ex_out << "Point-Poly: " << boost::geometry::wkt(sout1) << std::endl;

    boost::geometry::closest_points(poly, line, sout2);
    ex_out << "Poly-Line: " << boost::geometry::wkt(sout2) << std::endl;

    boost::geometry::closest_points(poly, mp, sout3);
    ex_out << "Poly-MultiPoint: " << boost::geometry::wkt(sout3) << std::endl;

     create_svg_closest_points(p0, p, line, poly, mp, sout1, sout2, sout3); 

    return 0;
    return 0;  // main's implicit return
}

//]


//[closest_points_output
/*`
Output:
[pre
Point-Poly: LINESTRING(4.3 1.9,4.2 2.1)
Poly-Line: LINESTRING(2.9 0.7,2.9 0)
Poly-MultiPoint: LINESTRING(2.36923 1.05385,2 0.5)
[$img/algorithms/closest_points.png]
]
*/
//]

TEST_CASE("example: algorithms/closest_points") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Point-Poly: LINESTRING(4.3 1.9,4.2 2.1)
Poly-Line: LINESTRING(2.9 0.7,2.9 0)
Poly-MultiPoint: LINESTRING(2.36923 1.05385,2 0.5)
)ex"));
}
}  // namespace ex_algorithms_closest_points

// libs/geometry/doc/src/examples/algorithms/closest_points_strategy.cpp
namespace ex_algorithms_closest_points_strategy {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy
        <
            double,
            boost::geometry::cs::geographic<boost::geometry::degree>
        >;
    using polygon_type = boost::geometry::model::polygon<point_type>;
    using linestring_type = boost::geometry::model::linestring<point_type>;
    using multi_point_type = boost::geometry::model::multi_point<point_type>;

    point_type p(4.3,1.9);
    polygon_type poly;
    linestring_type line;
    multi_point_type mp;

    boost::geometry::strategies::closest_points::geographic<> strategies
        (boost::geometry::srs::spheroid<double>(6378137.0, 6356752.3142451793));

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);
    
    line.push_back(point_type(5,0));
    line.push_back(point_type(2,0));
    mp.push_back(point_type(2,0.5));
    mp.push_back(point_type(2,3));

    boost::geometry::model::segment<point_type> sout1,sout2,sout3;

    boost::geometry::closest_points(p, poly, sout1, strategies);
    ex_out << "Point-Poly: " << boost::geometry::wkt(sout1) << std::endl;

    boost::geometry::closest_points(poly, line, sout2, strategies);
    ex_out << "Poly-Line: " << boost::geometry::wkt(sout2) << std::endl;

    boost::geometry::closest_points(p, mp, sout3, strategies);
    ex_out << "Poly-MultiPoint: " << boost::geometry::wkt(sout3) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[closest_points_strategy_output
/*`
Output:
[pre
Point-Poly: LINESTRING(4.3 1.9,4.20096 2.1005)
Poly-Line: LINESTRING(2.9 0.7,2.9 0)
Poly-MultiPoint: LINESTRING(4.3 1.9,2 3)
]
*/
//]

TEST_CASE("example: algorithms/closest_points_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Point-Poly: LINESTRING(4.3 1.9,4.20096 2.1005)
Poly-Line: LINESTRING(2.9 0.7,2.9 0)
Poly-MultiPoint: LINESTRING(4.3 1.9,2 3)
)ex"));
}
}  // namespace ex_algorithms_closest_points_strategy

// libs/geometry/doc/src/examples/algorithms/comparable_distance.cpp
namespace ex_algorithms_comparable_distance {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;

    point_type p(1.4, 2.6);

    std::vector<point_type> v;
    for (double x = 0.0; x <= 4.0; x++)
    {
        for (double y = 0.0; y <= 4.0; y++)
        {
            v.push_back(point_type(x, y));
        }
    }

    point_type min_p;
    double min_d = boost::numeric::bounds<double>::highest();
    for (point_type const& pv : v)
    {
        double d = boost::geometry::comparable_distance(p, pv);
        if (d < min_d)
        {
            min_d = d;
            min_p = pv;
        }
    }

    ex_out
        << "Closest: " << boost::geometry::dsv(min_p) << std::endl
        << "At: " << boost::geometry::distance(p, min_p) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[comparable_distance_output
/*`
Output:
[pre
Closest: (1, 3)
At: 0.565685
]
*/
//]

TEST_CASE("example: algorithms/comparable_distance") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Closest: (1, 3)
At: 0.565685
)ex"));
}
}  // namespace ex_algorithms_comparable_distance

// libs/geometry/doc/src/examples/algorithms/convert.cpp
namespace ex_algorithms_convert {
}  // namespace ex_algorithms_convert (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_convert {

int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using box = boost::geometry::model::box<point>;
    using polygon = boost::geometry::model::polygon<point>;

    point p1(1, 1);
    box bx = boost::geometry::make<box>(1, 1, 2, 2);

    // Assign a box to a polygon (conversion box->poly)
    polygon poly;
    boost::geometry::convert(bx, poly);

    // Convert a point to another point type (conversion of point-type)
    boost::tuple<double, double> p2;
    boost::geometry::convert(p1, p2); // source -> target

    using boost::geometry::dsv;
    ex_out
        << "box: " << dsv(bx) << std::endl
        << "polygon: " << dsv(poly) << std::endl
        << "point: " << dsv(p1) << std::endl
        << "point tuples: " << dsv(p2) << std::endl
        ;

    return 0;
    return 0;  // main's implicit return
}

//]


//[convert_output
/*`
Output:
[pre
box: ((1, 1), (2, 2))
polygon: (((1, 1), (1, 2), (2, 2), (2, 1), (1, 1)))
point: (1, 1)
point tuples: (1, 1)
]
*/
//]

TEST_CASE("example: algorithms/convert") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(box: ((1, 1), (2, 2))
polygon: (((1, 1), (1, 2), (2, 2), (2, 1), (1, 1)))
point: (1, 1)
point tuples: (1, 1)
)ex"));
}
}  // namespace ex_algorithms_convert

// libs/geometry/doc/src/examples/algorithms/convex_hull.cpp
namespace ex_algorithms_convex_hull {
}  // namespace ex_algorithms_convex_hull (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_convex_hull {

// ---- inlined create_svg_two.hpp ----
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----
int example_main()
{
    using point = boost::tuple<double, double>;
    using polygon = boost::geometry::model::polygon<point>;

    polygon poly;
    boost::geometry::read_wkt("polygon((2.0 1.3, 2.4 1.7, 2.8 1.8, 3.4 1.2, 3.7 1.6,3.4 2.0, 4.1 3.0"
        ", 5.3 2.6, 5.4 1.2, 4.9 0.8, 2.9 0.7,2.0 1.3))", poly);

    polygon hull;
    boost::geometry::convex_hull(poly, hull);

    using boost::geometry::dsv;
    ex_out
        << "polygon: " << dsv(poly) << std::endl
        << "hull: " << dsv(hull) << std::endl
        ;

     create_svg("hull.svg", poly, hull); 
    return 0;
    return 0;  // main's implicit return
}

//]


//[convex_hull_output
/*`
Output:
[pre
polygon: (((2, 1.3), (2.4, 1.7), (2.8, 1.8), (3.4, 1.2), (3.7, 1.6), (3.4, 2), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))
hull: (((2, 1.3), (2.4, 1.7), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))

[$img/algorithms/convex_hull.png]
]
*/
//]

TEST_CASE("example: algorithms/convex_hull") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(polygon: (((2, 1.3), (2.4, 1.7), (2.8, 1.8), (3.4, 1.2), (3.7, 1.6), (3.4, 2), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))
hull: (((2, 1.3), (2.4, 1.7), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))
)ex"));
}
}  // namespace ex_algorithms_convex_hull

// libs/geometry/doc/src/examples/algorithms/correct.cpp
namespace ex_algorithms_correct {
}  // namespace ex_algorithms_correct (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_correct {

int example_main()
{
    using clockwise_closed_polygon = boost::geometry::model::polygon
        <
            boost::tuple<int, int>
        >;

    clockwise_closed_polygon cwcp;

    // Fill it counterclockwise (so wrongly), forgetting the closing point
    boost::geometry::exterior_ring(cwcp) = {{0, 0}, {10, 10}, {0, 9}};

    // Add a counterclockwise closed inner ring (this is correct)
    boost::geometry::interior_rings(cwcp).push_back({{1, 2}, {4, 6}, {2, 8}, {1, 2}});

    // Its area should be negative (because of wrong orientation)
    //     and wrong (because of omitted closing point)
    const auto area_before = boost::geometry::area(cwcp);
    const auto count_before = boost::geometry::num_points(cwcp);

    // Correct it!
    boost::geometry::correct(cwcp);

    // Check its new area
    const auto area_after = boost::geometry::area(cwcp);
    const auto count_after = boost::geometry::num_points(cwcp);

    // And output it
    ex_out << boost::geometry::dsv(cwcp) << std::endl;
    ex_out << area_before << " -> " << area_after << std::endl;
    ex_out << count_before << " -> " << count_after << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[correct_output
/*`
Output:
[pre
(((0, 0), (0, 9), (10, 10), (0, 0)), ((1, 2), (4, 6), (2, 8), (1, 2)))
-7 -> 38
7 -> 8
]
*/
//]

TEST_CASE("example: algorithms/correct") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((((0, 0), (0, 9), (10, 10), (0, 0)), ((1, 2), (4, 6), (2, 8), (1, 2)))
-7 -> 38
7 -> 8
)ex"));
}
}  // namespace ex_algorithms_correct

// libs/geometry/doc/src/examples/algorithms/covered_by.cpp
namespace ex_algorithms_covered_by {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Checks if the first geometry is inside or on border the second geometry.
    bg::model::polygon<bg::model::d2::point_xy<double>> poly1;
    bg::read_wkt("POLYGON((0 2,0 3,2 4,1 2,0 2))", poly1);
    bg::model::polygon<bg::model::d2::point_xy<double>> poly2;
    bg::read_wkt("POLYGON((0 4,3 4,2 2,0 1,0 4))", poly2);
    bool check_covered = bg::covered_by(poly1, poly2);
    if (check_covered) {
        ex_out << "Covered: Yes" << std::endl;
    } else {
        ex_out << "Covered: No" << std::endl;
    }

    bg::model::polygon<bg::model::d2::point_xy<double>> poly3;
    bg::read_wkt("POLYGON((-1 -1,-3 -4,-7 -7,-4 -3,-1 -1))", poly3);
    check_covered = bg::covered_by(poly1, poly3);
    if (check_covered) {
        ex_out << "Covered: Yes" << std::endl;
    } else {
        ex_out << "Covered: No" << std::endl;
    }

    // This should return true since both polygons are same, so they are lying on each other.
    check_covered = bg::covered_by(poly1, poly1);
    if (check_covered) {
         ex_out << "Covered: Yes" << std::endl;
    } else {
        ex_out << "Covered: No" << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]


//[covered_by_output
/*`
Output:
[pre
Covered: Yes

[$img/algorithms/covered_by.png]

Covered: No
Covered: Yes
]
*/
//]

TEST_CASE("example: algorithms/covered_by") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Covered: Yes
Covered: No
Covered: Yes
)ex"));
}
}  // namespace ex_algorithms_covered_by

// libs/geometry/doc/src/examples/algorithms/crosses.cpp
namespace ex_algorithms_crosses {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Checks if the two geometries (here, a polygon and a linestring) crosses or not. 
    bg::model::polygon<bg::model::d2::point_xy<double>> poly;
    bg::read_wkt("POLYGON((0 0,0 3,3 3,3 0,0 0))", poly);
    bg::model::linestring<bg::model::d2::point_xy<double>> line1;
    bg::read_wkt("LINESTRING(1 1,2 2,4 4)", line1);
    bool check_crosses = bg::crosses(poly, line1);
    if (check_crosses) {
        ex_out << "Crosses: Yes" << std::endl;
    } else {
        ex_out << "Crosses: No" << std::endl;
    }

    // Edge case: linestring just touches the polygon but doesn't crosses it.
    bg::model::linestring<bg::model::d2::point_xy<double>> line2;
    bg::read_wkt("LINESTRING(1 1,1 2,1 3)", line2);
    check_crosses = bg::crosses(poly, line2);
    if (check_crosses) {
        ex_out << "Crosses: Yes" << std::endl;
    } else {
        ex_out << "Crosses: No" << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]


//[crosses_output
/*`
Output:
[pre
Crosses: Yes
Crosses: No

[$img/algorithms/crosses.png]

]
*/
//]

TEST_CASE("example: algorithms/crosses") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Crosses: Yes
Crosses: No
)ex"));
}
}  // namespace ex_algorithms_crosses

// libs/geometry/doc/src/examples/algorithms/densify.cpp
namespace ex_algorithms_densify {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((0 0,0 10,10 10,10 0,0 0),(1 1,4 1,4 4,1 4,1 1))", poly);

    polygon_type res;

    boost::geometry::densify(poly, res, 6.0);

    ex_out << "densified: " << boost::geometry::wkt(res) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[densify_output
/*`
Output:
[pre
densified: POLYGON((0 0,0 5,0 10,5 10,10 10,10 5,10 0,5 0,0 0),(1 1,4 1,4 4,1 4,1 1))
]
*/
//]

TEST_CASE("example: algorithms/densify") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(densified: POLYGON((0 0,0 5,0 10,5 10,10 10,10 5,10 0,5 0,0 0),(1 1,4 1,4 4,1 4,1 1))
)ex"));
}
}  // namespace ex_algorithms_densify

// libs/geometry/doc/src/examples/algorithms/densify_strategy.cpp
namespace ex_algorithms_densify_strategy {
int example_main()
{
    namespace bg = boost::geometry;
    using point_type = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;
    using linestring_type = bg::model::linestring<point_type>;

    linestring_type ls;
    bg::read_wkt("LINESTRING(0 0,1 1)", ls);

    linestring_type res;

    bg::srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);
    bg::strategy::densify::geographic<> strategy(spheroid);

    boost::geometry::densify(ls, res, 50000.0, strategy);

    ex_out << "densified: " << boost::geometry::wkt(res) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[densify_strategy_output
/*`
Output:
[pre
densified: LINESTRING(0 0,0.249972 0.250011,0.499954 0.500017,0.749954 0.750013,1 1)
]
*/
//]

TEST_CASE("example: algorithms/densify_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(densified: LINESTRING(0 0,0.249972 0.250011,0.499954 0.500017,0.749954 0.750013,1 1)
)ex"));
}
}  // namespace ex_algorithms_densify_strategy

// libs/geometry/doc/src/examples/algorithms/difference.cpp
namespace ex_algorithms_difference {
#undef CREATE_SVG_OVERLAY_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_OVERLAY_HPP
#define CREATE_SVG_OVERLAY_HPP



#if defined(HAVE_SVG)
#endif

template <typename Geometry, typename Range>
void create_svg(std::string const& filename, Geometry const& a, Geometry const& b, Range const& range)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    int i = 0;
    for (typename boost::range_value<Range>::type const& g : range)
    {
        mapper.map(g, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
        std::ostringstream out;
        out << i++;
        mapper.text(boost::geometry::return_centroid<point_type>(g), out.str(),
                    "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
    }
#else
    boost::ignore_unused(filename, a, b, range);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_OVERLAY_HPP

// ---- end create_svg_overlay.hpp ----

int example_main()
{
    using polygon = boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double>>;

    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    std::list<polygon> output;
    boost::geometry::difference(green, blue, output);

    int i = 0;
    ex_out << "green - blue:" << std::endl;
    for (polygon const& p : output)
    {
        ex_out << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

     create_svg("difference_a.svg", green, blue, output); 
    output.clear();
    boost::geometry::difference(blue, green, output);

    i = 0;
    ex_out << "blue - green:" << std::endl;
    for (polygon const& p : output)
    {
        ex_out << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

     create_svg("difference_b.svg", green, blue, output); 
    return 0;
    return 0;  // main's implicit return
}

//]


//[difference_output
/*`
Output:
[pre
green - blue:
0: 0.02375
1: 0.542951
2: 0.0149697
3: 0.226855
4: 0.839424

[$img/algorithms/difference_a.png]

blue - green:
0: 0.525154
1: 0.015
2: 0.181136
3: 0.128798
4: 0.340083
5: 0.307778

[$img/algorithms/difference_b.png]
]
*/
//]

TEST_CASE("example: algorithms/difference") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(green - blue:
0: 0.02375
1: 0.542951
2: 0.0149697
3: 0.226855
4: 0.839424
blue - green:
0: 0.525154
1: 0.015
2: 0.181136
3: 0.340083
4: 0.128798
5: 0.307778
)ex"));
}
}  // namespace ex_algorithms_difference

// libs/geometry/doc/src/examples/algorithms/discrete_frechet_distance.cpp
namespace ex_algorithms_discrete_frechet_distance {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using linestring_type = boost::geometry::model::linestring<point_type>;

    linestring_type ls1, ls2;
    boost::geometry::read_wkt("LINESTRING(0 0,1 1,1 2,2 1,2 2)", ls1);
    boost::geometry::read_wkt("LINESTRING(1 0,0 1,1 1,2 1,3 1)", ls2);

    double res = boost::geometry::discrete_frechet_distance(ls1, ls2);

    ex_out << "Discrete Frechet Distance: " << res << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[discrete_frechet_distance_output
/*`
Output:
[pre
Discrete Frechet Distance:  1.41421
]
*/
//]

TEST_CASE("example: algorithms/discrete_frechet_distance") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Discrete Frechet Distance: 1.41421
)ex"));
}
}  // namespace ex_algorithms_discrete_frechet_distance

// libs/geometry/doc/src/examples/algorithms/discrete_frechet_distance_strategy.cpp
namespace ex_algorithms_discrete_frechet_distance_strategy {
int example_main()
{
    namespace bg = boost::geometry;
    using point_type = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;
    using linestring_type = bg::model::linestring<point_type>;

    linestring_type ls1, ls2;
    bg::read_wkt("LINESTRING(0 0,1 1,1 2,2 1,2 2)", ls1);
    bg::read_wkt("LINESTRING(1 0,0 1,1 1,2 1,3 1)", ls2);

    bg::srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);
    bg::strategy::distance::geographic<> strategy(spheroid);

    double res = bg::discrete_frechet_distance(ls1, ls2, strategy);

    ex_out << "Discrete Frechet Distance: " << res << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[discrete_frechet_distance_strategy_output
/*`
Output:
[pre
Discrete Frechet Distance: 156874
]
*/
//]

TEST_CASE("example: algorithms/discrete_frechet_distance_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Discrete Frechet Distance: 156874
)ex"));
}
}  // namespace ex_algorithms_discrete_frechet_distance_strategy

// libs/geometry/doc/src/examples/algorithms/discrete_hausdorff_distance.cpp
namespace ex_algorithms_discrete_hausdorff_distance {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using linestring_type = boost::geometry::model::linestring<point_type>;

    linestring_type ls1, ls2;
    boost::geometry::read_wkt("LINESTRING(0 0,1 1,1 2,2 1,2 2)", ls1);
    boost::geometry::read_wkt("LINESTRING(1 0,0 1,1 1,2 1,3 1)", ls2);

    double res = boost::geometry::discrete_hausdorff_distance(ls1, ls2);

    ex_out << "Discrete Hausdorff Distance: " << res << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[discrete_hausdorff_distance_output
/*`
Output:
[pre
Discrete Hausdorff Distance: 1.0
]
*/
//]

TEST_CASE("example: algorithms/discrete_hausdorff_distance") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Discrete Hausdorff Distance: 1
)ex"));
}
}  // namespace ex_algorithms_discrete_hausdorff_distance

// libs/geometry/doc/src/examples/algorithms/discrete_hausdorff_distance_strategy.cpp
namespace ex_algorithms_discrete_hausdorff_distance_strategy {
int example_main()
{
    namespace bg = boost::geometry;
    using point_type = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;
    using linestring_type = bg::model::linestring<point_type>;

    linestring_type ls1, ls2;
    bg::read_wkt("LINESTRING(0 0,1 1,1 2,2 1,2 2)", ls1);
    bg::read_wkt("LINESTRING(1 0,0 1,1 1,2 1,3 1)", ls2);

    bg::srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);
    bg::strategy::distance::geographic<> strategy(spheroid);

    double res = bg::discrete_hausdorff_distance(ls1, ls2, strategy);

    ex_out << "Discrete Hausdorff Distance: " << res << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[discrete_hausdorff_distance_strategy_output
/*`
Output:
[pre
Discrete Hausdorff Distance: 110574
]
*/
//]

TEST_CASE("example: algorithms/discrete_hausdorff_distance_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Discrete Hausdorff Distance: 110574
)ex"));
}
}  // namespace ex_algorithms_discrete_hausdorff_distance_strategy

// libs/geometry/doc/src/examples/algorithms/disjoint.cpp
namespace ex_algorithms_disjoint {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Checks if two geometries are disjoint, which means that two geometries have zero intersection.
    bg::model::polygon<bg::model::d2::point_xy<double>> poly1;
    bg::read_wkt("POLYGON((0 2,-2 0,-4 2,-2 4,0 2))", poly1);
    bg::model::polygon<bg::model::d2::point_xy<double>> poly2;
    bg::read_wkt("POLYGON((2 2,4 4,6 2,4 0,2 2))", poly2);
    bool check_disjoint = bg::disjoint(poly1, poly2);
    if (check_disjoint) {
        ex_out << "Disjoint: Yes" << std::endl;
    } else {
        ex_out << "Disjoint: No" << std::endl;
    }

    bg::model::polygon<bg::model::d2::point_xy<double>> poly3;
    bg::read_wkt("POLYGON((0 2,2 4,4 2,2 0,0 2))", poly3);
    check_disjoint = bg::disjoint(poly1, poly3);
    if (check_disjoint) {
        ex_out << "Disjoint: Yes" << std::endl;
    } else {
        ex_out << "Disjoint: No" << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]


//[disjoint_output
/*`
Output:
[pre
Disjoint: Yes
Disjoint: No

[$img/algorithms/disjoint.png]

]
*/
//]

TEST_CASE("example: algorithms/disjoint") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Disjoint: Yes
Disjoint: No
)ex"));
}
}  // namespace ex_algorithms_disjoint

// libs/geometry/doc/src/examples/algorithms/distance.cpp
namespace ex_algorithms_distance {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;
    using linestring_type = boost::geometry::model::linestring<point_type>;
    using multi_point_type = boost::geometry::model::multi_point<point_type>;

    point_type p(1,2);
    polygon_type poly;
    linestring_type line;
    multi_point_type mp;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);
    line.push_back(point_type(0,0));
    line.push_back(point_type(0,3));
    mp.push_back(point_type(0,0));
    mp.push_back(point_type(3,3));

    ex_out
        << "Point-Poly: " << boost::geometry::distance(p, poly) << std::endl
        << "Point-Line: " << boost::geometry::distance(p, line) << std::endl
        << "Point-MultiPoint: " << boost::geometry::distance(p, mp) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[distance_output
/*`
Output:
[pre
Point-Poly: 1.22066
Point-Line: 1
Point-MultiPoint: 2.23607
]
*/
//]

TEST_CASE("example: algorithms/distance") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Point-Poly: 1.22066
Point-Line: 1
Point-MultiPoint: 2.23607
)ex"));
}
}  // namespace ex_algorithms_distance

// libs/geometry/doc/src/examples/algorithms/envelope.cpp
namespace ex_algorithms_envelope {
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----

int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;

    boost::geometry::model::polygon<point> polygon;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", polygon);

    boost::geometry::model::box<point> box;
    boost::geometry::envelope(polygon, box);

    ex_out << "envelope:" << boost::geometry::dsv(box) << std::endl;

     create_svg("envelope.svg", polygon, box); 
    return 0;
    return 0;  // main's implicit return
}

//]


//[envelope_output
/*`
Output:
[pre
envelope:((2, 0.7), (5.4, 3))

[$img/algorithms/envelope.png]
]
*/
//]

TEST_CASE("example: algorithms/envelope") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(envelope:((2, 0.7), (5.4, 3))
)ex"));
}
}  // namespace ex_algorithms_envelope

// libs/geometry/doc/src/examples/algorithms/equals.cpp
namespace ex_algorithms_equals {
}  // namespace ex_algorithms_equals (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_equals {

int example_main()
{
    using point = boost::tuple<int, int>;

    boost::geometry::model::polygon<point> poly1, poly2;
    boost::geometry::exterior_ring(poly1) = {{0, 0}, {0, 5}, {5, 5}, {5, 0}, {0, 0}};
    boost::geometry::exterior_ring(poly2) = {{5, 0}, {0, 0}, {0, 5}, {5, 5}, {5, 0}};

    ex_out
        << "polygons are spatially "
        << (boost::geometry::equals(poly1, poly2) ? "equal" : "not equal")
        << std::endl;

    boost::geometry::model::box<point> box;
    boost::geometry::assign_values(box, 0, 0, 5, 5);

    ex_out
        << "polygon and box are spatially "
        << (boost::geometry::equals(box, poly2) ? "equal" : "not equal")
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[equals_output
/*`
Output:
[pre
polygons are spatially equal
polygon and box are spatially equal
]
*/
//]

TEST_CASE("example: algorithms/equals") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(polygons are spatially equal
polygon and box are spatially equal
)ex"));
}
}  // namespace ex_algorithms_equals

// libs/geometry/doc/src/examples/algorithms/expand.cpp
namespace ex_algorithms_expand {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<short int>;
    using box_type = boost::geometry::model::box<point_type>;

    using boost::geometry::expand;

    box_type box = boost::geometry::make_inverse<box_type>(); /*< expand is usually preceded by a call to assign_inverse or make_inverse  >*/

    expand(box, point_type(0, 0));
    expand(box, point_type(1, 2));
    expand(box, point_type(5, 4));
    expand(box, boost::geometry::make<box_type>(3, 3, 5, 5));

    ex_out << boost::geometry::dsv(box) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[expand_output
/*`
Output:
[pre
((0, 0), (5, 5))
]
*/
//]

TEST_CASE("example: algorithms/expand") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((0, 0), (5, 5))
)ex"));
}
}  // namespace ex_algorithms_expand

// libs/geometry/doc/src/examples/algorithms/for_each_point.cpp
namespace ex_algorithms_for_each_point {
int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;

    constexpr auto factor = 0.1;
    auto round = [&](auto value)
    {
        return floor(0.5 + (value / factor)) * factor;
    };

    boost::geometry::model::polygon<point> poly;
    boost::geometry::read_wkt("POLYGON((0 0,1.123 9.987,8.876 2.234,0 0),(3.345 4.456,7.654 8.765,9.123 5.432,3.345 4.456))", poly);
    boost::geometry::for_each_point(poly,
            [&round](auto& p)
            {
                using boost::geometry::get;
                using boost::geometry::set;
                set<0>(p, round(get<0>(p)));
                set<1>(p, round(get<1>(p)));
            }
        );
    ex_out << "Rounded: " << boost::geometry::wkt(poly) << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[for_each_point_output
/*`
Output:
[pre
 Rounded: POLYGON((0 0,1.1 10,8.9 2.2,0 0),(3.3 4.5,7.7 8.8,9.1 5.4,3.3 4.5))
]
*/
//]

TEST_CASE("example: algorithms/for_each_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Rounded: POLYGON((0 0,1.1 10,8.9 2.2,0 0),(3.3 4.5,7.7 8.8,9.1 5.4,3.3 4.5))
)ex"));
}
}  // namespace ex_algorithms_for_each_point

// libs/geometry/doc/src/examples/algorithms/for_each_point_const.cpp
namespace ex_algorithms_for_each_point_const {
template <typename Point>
void list_coordinates(Point const& p)
{
    using boost::geometry::get;
    ex_out << "x = " << get<0>(p) << " y = " << get<1>(p) << std::endl;
}

int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    boost::geometry::model::polygon<point> poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 4,4 0,0 0))", poly);
    boost::geometry::for_each_point(poly, list_coordinates<point>);
    return 0;
    return 0;  // main's implicit return
}

//]


//[for_each_point_const_output
/*`
Output:
[pre
x = 0 y = 0
x = 0 y = 4
x = 4 y = 0
x = 0 y = 0
]
*/
//]

TEST_CASE("example: algorithms/for_each_point_const") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(x = 0 y = 0
x = 0 y = 4
x = 4 y = 0
x = 0 y = 0
)ex"));
}
}  // namespace ex_algorithms_for_each_point_const

// libs/geometry/doc/src/examples/algorithms/for_each_segment_const.cpp
namespace ex_algorithms_for_each_segment_const {
int example_main()
{
    // Define a type
    using point = boost::geometry::model::d2::point_xy<double>;

    // Create a linestring
    const boost::geometry::model::linestring<point> polyline =
        {{0, 0}, {3, 3}, {5, 1}, {6, 2}, {8, 0}, {4, -4}, {1, -1}, {3, 2}};

    // Iterate over its segments to find the minimum and maximum length
    double min_length = std::numeric_limits<double>::max();
    double max_length = -std::numeric_limits<double>::max();
    
    boost::geometry::for_each_segment(polyline, 
        [&](auto const& s)
        {
            const auto length = static_cast<double>(boost::geometry::length(s));
            min_length = std::min(min_length, length);
            max_length = std::max(max_length, length);
        });

    // Output the results
    ex_out
        << "Min segment length: " << min_length << std::endl
        << "Max segment length: " << max_length << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[for_each_segment_const_output
/*`
Output:
[pre
Min segment length: 1.41421
Max segment length: 5.65685
]
*/
//]

TEST_CASE("example: algorithms/for_each_segment_const") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Min segment length: 1.41421
Max segment length: 5.65685
)ex"));
}
}  // namespace ex_algorithms_for_each_segment_const

// libs/geometry/doc/src/examples/algorithms/intersection_ls_ls_point.cpp
namespace ex_algorithms_intersection_ls_ls_point {
}  // namespace ex_algorithms_intersection_ls_ls_point (reopened after the registration)
#ifndef EX_REGISTERED_21ad29ae05b8
#define EX_REGISTERED_21ad29ae05b8
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
#endif
namespace ex_algorithms_intersection_ls_ls_point {


int example_main()
{
    using P = boost::geometry::model::d2::point_xy<double>;
    std::vector<P> line1, line2;
    boost::geometry::read_wkt("linestring(1 1,2 2,3 1)", line1);
    boost::geometry::read_wkt("linestring(1 2,2 1,3 2)", line2);

    std::deque<P> intersection_points;
    boost::geometry::intersection(line1, line2, intersection_points);

    for (P const& p : intersection_points)
    {
        ex_out << " " << boost::geometry::wkt(p);
    }
    ex_out << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[intersection_ls_ls_point_output
/*`
Output:
[pre
 POINT(1.5 1.5) POINT(2.5 1.5)
]
*/
//]

TEST_CASE("example: algorithms/intersection_ls_ls_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( POINT(1.5 1.5) POINT(2.5 1.5)
)ex"));
}
}  // namespace ex_algorithms_intersection_ls_ls_point

// libs/geometry/doc/src/examples/algorithms/intersection_poly_poly.cpp
namespace ex_algorithms_intersection_poly_poly {
#undef CREATE_SVG_OVERLAY_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_OVERLAY_HPP
#define CREATE_SVG_OVERLAY_HPP



#if defined(HAVE_SVG)
#endif

template <typename Geometry, typename Range>
void create_svg(std::string const& filename, Geometry const& a, Geometry const& b, Range const& range)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    int i = 0;
    for (typename boost::range_value<Range>::type const& g : range)
    {
        mapper.map(g, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
        std::ostringstream out;
        out << i++;
        mapper.text(boost::geometry::return_centroid<point_type>(g), out.str(),
                    "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
    }
#else
    boost::ignore_unused(filename, a, b, range);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_OVERLAY_HPP

// ---- end create_svg_overlay.hpp ----

int example_main()
{
    using polygon = boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double>> ;

    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    std::deque<polygon> output;
    boost::geometry::intersection(green, blue, output);

    int i = 0;
    ex_out << "green && blue:" << std::endl;
    for (polygon const& p : output)
    {
        ex_out << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

     create_svg("intersection.svg", green, blue, output); 
    return 0;
    return 0;  // main's implicit return
}

//]


//[intersection_output
/*`
Output:
[pre
green && blue:
0: 2.50205

[$img/algorithms/intersection.png]

]
*/
//]

TEST_CASE("example: algorithms/intersection_poly_poly") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(green && blue:
0: 2.50205
)ex"));
}
}  // namespace ex_algorithms_intersection_poly_poly

// libs/geometry/doc/src/examples/algorithms/intersection_segment.cpp
namespace ex_algorithms_intersection_segment {
int example_main()
{
    using P = boost::geometry::model::d2::point_xy<double>;
    boost::geometry::model::segment<P> segment1, segment2;
    boost::geometry::read_wkt("linestring(1 1,2 2)", segment1);
    boost::geometry::read_wkt("linestring(2 1,1 2)", segment2);

    std::vector<P> intersections;
    boost::geometry::intersection(segment1, segment2, intersections);

    for (P const& p : intersections)
    {
        ex_out << " " << boost::geometry::wkt(p);
    }
    ex_out << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[intersection_segment_output
/*`
Output:
[pre
 POINT(1.5 1.5)
]
*/
//]

TEST_CASE("example: algorithms/intersection_segment") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( POINT(1.5 1.5)
)ex"));
}
}  // namespace ex_algorithms_intersection_segment

// libs/geometry/doc/src/examples/algorithms/intersects_linestring.cpp
namespace ex_algorithms_intersects_linestring {
int example_main()
{
    // Calculate the intersects of a cartesian polygon
    using P = boost::geometry::model::d2::point_xy<double>;
    boost::geometry::model::linestring<P> line1, line2;

    boost::geometry::read_wkt("linestring(1 1,2 2,3 3)", line1);
    boost::geometry::read_wkt("linestring(2 1,1 2,4 0)", line2);

    bool b = boost::geometry::intersects(line1, line2);

    ex_out << "Intersects: " << (b ? "YES" : "NO") << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[intersects_linestring_output
/*`
Output:
[pre
Intersects: YES
]
*/
//]

TEST_CASE("example: algorithms/intersects_linestring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Intersects: YES
)ex"));
}
}  // namespace ex_algorithms_intersects_linestring

// libs/geometry/doc/src/examples/algorithms/intersects_segment.cpp
namespace ex_algorithms_intersects_segment {
int example_main()
{
    // Calculate the intersects of a cartesian polygon
    using P = boost::geometry::model::d2::point_xy<double>;
    boost::geometry::model::linestring<P> line1, line2;

    boost::geometry::read_wkt("linestring(1 1,2 2)", line1);
    boost::geometry::read_wkt("linestring(2 1,1 2)", line2);

    bool intersects = boost::geometry::intersects(line1, line2);

    ex_out << "Intersects: " << (intersects ? "YES" : "NO") << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[intersects_output
/*`
Output:
[pre
Intersects: YES
]
*/
//]

TEST_CASE("example: algorithms/intersects_segment") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Intersects: YES
)ex"));
}
}  // namespace ex_algorithms_intersects_segment

// libs/geometry/doc/src/examples/algorithms/is_empty.cpp
namespace ex_algorithms_is_empty {
int example_main()
{
    boost::geometry::model::multi_linestring
        <
            boost::geometry::model::linestring
                <
                    boost::geometry::model::d2::point_xy<double>
                >
        > mls;
    boost::geometry::read_wkt("MULTILINESTRING((0 0,0 10,10 0),(1 1,8 1,1 8))", mls);
    ex_out << "Is empty? " << (boost::geometry::is_empty(mls) ? "yes" : "no") << std::endl;
    boost::geometry::clear(mls);
    ex_out << "Is empty (after clearing)? " << (boost::geometry::is_empty(mls) ? "yes" : "no") << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[is_empty_output
/*`
Output:
[pre
Is empty? no
Is empty (after clearing)? yes
]
*/
//]

TEST_CASE("example: algorithms/is_empty") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Is empty? no
Is empty (after clearing)? yes
)ex"));
}
}  // namespace ex_algorithms_is_empty

// libs/geometry/doc/src/examples/algorithms/is_simple.cpp
namespace ex_algorithms_is_simple {
#undef CREATE_SVG_ONE_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_ONE_HPP
#define CREATE_SVG_ONE_HPP


#if defined(HAVE_SVG)
#endif

template <typename Geometry>
void create_svg(std::string const& filename, Geometry const& g)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(g);

    mapper.map(g, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
#else
    boost::ignore_unused(filename, g);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_ONE_HPP

// ---- end create_svg_one.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using linestring_type = boost::geometry::model::linestring<point_type>;
    using multi_linestring_type = boost::geometry::model::multi_linestring<linestring_type>;

    multi_linestring_type multi_linestring;
    boost::geometry::read_wkt("MULTILINESTRING((0 0,0 10,10 10,10 0,0 0),(10 10,20 20))", multi_linestring);

    ex_out << "is simple? "
              << (boost::geometry::is_simple(multi_linestring) ? "yes" : "no")
              << std::endl;
     create_svg("is_simple_example.svg", multi_linestring); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[is_simple_output
/*`
Output:
[pre
is simple? no

[$img/algorithms/is_simple_example.png]

]

*/
//]

TEST_CASE("example: algorithms/is_simple") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(is simple? no
)ex"));
}
}  // namespace ex_algorithms_is_simple

// libs/geometry/doc/src/examples/algorithms/is_valid.cpp
namespace ex_algorithms_is_valid {
#undef CREATE_SVG_ONE_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_ONE_HPP
#define CREATE_SVG_ONE_HPP


#if defined(HAVE_SVG)
#endif

template <typename Geometry>
void create_svg(std::string const& filename, Geometry const& g)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(g);

    mapper.map(g, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
#else
    boost::ignore_unused(filename, g);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_ONE_HPP

// ---- end create_svg_one.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 2,0 0),(0 0,2 9,1 9,0 0),(2 9,9 2,9 9,2 9))", poly);

    ex_out << "is valid? " << (boost::geometry::is_valid(poly) ? "yes" : "no") << std::endl;
     create_svg("is_valid_example.svg", poly); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[is_valid_output
/*`
Output:
[pre
is valid? no

[$img/algorithms/is_valid_example.png]

]

*/
//]

TEST_CASE("example: algorithms/is_valid") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(is valid? no
)ex"));
}
}  // namespace ex_algorithms_is_valid

// libs/geometry/doc/src/examples/algorithms/is_valid_failure.cpp
namespace ex_algorithms_is_valid_failure {
#undef CREATE_SVG_ONE_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_ONE_HPP
#define CREATE_SVG_ONE_HPP


#if defined(HAVE_SVG)
#endif

template <typename Geometry>
void create_svg(std::string const& filename, Geometry const& g)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(g);

    mapper.map(g, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
#else
    boost::ignore_unused(filename, g);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_ONE_HPP

// ---- end create_svg_one.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 10,10 10,10 0),(0 0,9 2,9 1,0 0),(0 0,2 9,1 9,0 0))", poly);

    ex_out << "original geometry: " << boost::geometry::dsv(poly) << std::endl;
    boost::geometry::validity_failure_type failure;
    bool valid = boost::geometry::is_valid(poly, failure);

    // if the invalidity is only due to lack of closing points and/or wrongly oriented rings, then bg::correct can fix it
    bool could_be_fixed = (failure == boost::geometry::failure_not_closed
                           || failure == boost::geometry::failure_wrong_orientation);
    ex_out << "is valid? " << (valid ? "yes" : "no") << std::endl;
    if (! valid)
    {
        ex_out << "can boost::geometry::correct remedy invalidity? " << (could_be_fixed ? "possibly yes" : "no") << std::endl;
        if (could_be_fixed)
        {
            boost::geometry::correct(poly);
            ex_out << "after correction: " << (boost::geometry::is_valid(poly) ? "valid" : "still invalid") << std::endl;
            ex_out << "corrected geometry: " << boost::geometry::dsv(poly) << std::endl;
        }
    }
     create_svg("is_valid_failure_example.svg", poly); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[is_valid_failure_output
/*`
Output:
[pre
original geometry: (((0, 0), (0, 10), (10, 10), (10, 0)), ((0, 0), (9, 2), (9, 1), (0, 0)), ((0, 0), (2, 9), (1, 9), (0, 0)))
is valid? no
can boost::geometry::correct remedy invalidity? possibly yes
after correction: valid
corrected geometry: (((0, 0), (0, 10), (10, 10), (10, 0), (0, 0)), ((0, 0), (9, 1), (9, 2), (0, 0)), ((0, 0), (2, 9), (1, 9), (0, 0)))

[$img/algorithms/is_valid_failure_example.png]

]

*/
//]

TEST_CASE("example: algorithms/is_valid_failure") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(original geometry: (((0, 0), (0, 10), (10, 10), (10, 0)), ((0, 0), (9, 2), (9, 1), (0, 0)), ((0, 0), (2, 9), (1, 9), (0, 0)))
is valid? no
can boost::geometry::correct remedy invalidity? possibly yes
after correction: valid
corrected geometry: (((0, 0), (0, 10), (10, 10), (10, 0), (0, 0)), ((0, 0), (9, 1), (9, 2), (0, 0)), ((0, 0), (2, 9), (1, 9), (0, 0)))
)ex"));
}
}  // namespace ex_algorithms_is_valid_failure

// libs/geometry/doc/src/examples/algorithms/is_valid_message.cpp
namespace ex_algorithms_is_valid_message {
#undef CREATE_SVG_ONE_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2014.
// Modifications copyright (c) 2014, Oracle and/or its affiliates.

// Contributed and/or modified by Menelaos Karavelas, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_ONE_HPP
#define CREATE_SVG_ONE_HPP


#if defined(HAVE_SVG)
#endif

template <typename Geometry>
void create_svg(std::string const& filename, Geometry const& g)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(g);

    mapper.map(g, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
#else
    boost::ignore_unused(filename, g);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_ONE_HPP

// ---- end create_svg_one.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 10,10 10,10 0,0 0),(0 0,9 1,9 2,0 0),(0 0,2 9,1 9,0 0),(2 9,9 2,9 9,2 9))", poly);

    std::string message;
    bool valid = boost::geometry::is_valid(poly, message);
    ex_out << "is valid? " << (valid ? "yes" : "no") << std::endl;
    if (! valid)
    {
        ex_out << "why not valid? " << message << std::endl;
    }
     create_svg("is_valid_example.svg", poly); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[is_valid_message_output
/*`
Output:
[pre
is valid? no
why not valid? Geometry has disconnected interior

[$img/algorithms/is_valid_example.png]

]

*/
//]

TEST_CASE("example: algorithms/is_valid_message") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(is valid? no
why not valid? Geometry has disconnected interior
)ex"));
}
}  // namespace ex_algorithms_is_valid_message

// libs/geometry/doc/src/examples/algorithms/length.cpp
namespace ex_algorithms_length {
    //` The following simple example shows the calculation of the length of a linestring containing three points



int example_main()
{
    using namespace boost::geometry;
    model::linestring<model::d2::point_xy<double>> line;
    read_wkt("linestring(0 0,1 1,4 8,3 2)", line);
    ex_out << "linestring length is "
        << length(line)
        << " units" << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[length_output
/*`
Output:
[pre
linestring length is 15.1127 units
]
*/
//]

TEST_CASE("example: algorithms/length") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(linestring length is 15.1127 units
)ex"));
}
}  // namespace ex_algorithms_length

// libs/geometry/doc/src/examples/algorithms/length_with_strategy.cpp
namespace ex_algorithms_length_with_strategy {
int example_main()
{
    using namespace boost::geometry;
    using P = model::point<float, 2, cs::spherical_equatorial<degree>>;
    model::linestring<P> line;
    line.push_back(P(2, 41));
    line.push_back(P(2, 48));
    line.push_back(P(5, 52));
    double const mean_radius = 6371.0; /*< [@http://en.wikipedia.org/wiki/Earth_radius Wiki]  >*/
    ex_out << "length is "
        << length(line, strategy::distance::haversine<float>(mean_radius) )
        << " kilometers " << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[length_with_strategy_output
/*`
Output:
[pre
length is 1272.03 kilometers
]
*/
//]

TEST_CASE("example: algorithms/length_with_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(length is 1272.03 kilometers 
)ex"));
}
}  // namespace ex_algorithms_length_with_strategy

// libs/geometry/doc/src/examples/algorithms/line_interpolate.cpp
namespace ex_algorithms_line_interpolate {
using namespace boost::geometry;

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using segment_type = model::segment<point_type>;
    using linestring_type = model::linestring<point_type>;
    using multipoint_type = model::multi_point<point_type>;

    segment_type const s { {0, 0}, {1, 1} };
    linestring_type const l { {0, 0}, {1, 0}, {1, 1}, {0, 1}, {0, 2} };
    point_type p;
    multipoint_type mp;

    ex_out << "point interpolation" << std::endl;

    line_interpolate(s, std::sqrt(2)/4, p);
    ex_out << "on segment : " << wkt(p) << std::endl;

    line_interpolate(l, 1.4, p);
    ex_out << "on linestring : " << wkt(p) << std::endl << std::endl;

    ex_out << "multipoint interpolation" << std::endl;

    line_interpolate(s, std::sqrt(2)/4, mp);
    ex_out << "on segment : " << wkt(mp) << std::endl;

    mp=multipoint_type();
    line_interpolate(l, 1.4, mp);
    ex_out << "on linestring : " << wkt(mp) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[line_interpolate_output
/*`
Output:
[pre
point interpolation
on segment : POINT(0.25 0.25)
on linestring : POINT(1 0.4)

multipoint interpolation
on segment : MULTIPOINT((0.25 0.25),(0.5 0.5),(0.75 0.75),(1 1))
on linestring : MULTIPOINT((1 0.4),(0.2 1))
]
*/
//]

TEST_CASE("example: algorithms/line_interpolate") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(point interpolation
on segment : POINT(0.25 0.25)
on linestring : POINT(1 0.4)

multipoint interpolation
on segment : MULTIPOINT((0.25 0.25),(0.5 0.5),(0.75 0.75),(1 1))
on linestring : MULTIPOINT((1 0.4),(0.2 1))
)ex"));
}
}  // namespace ex_algorithms_line_interpolate

// libs/geometry/doc/src/examples/algorithms/line_interpolate_strategy.cpp
namespace ex_algorithms_line_interpolate_strategy {
using namespace boost::geometry;

int example_main()
{
    using point_type = model::d2::point_xy<double,cs::geographic<degree>>;
    using segment_type = model::segment<point_type>;
    using linestring_type = model::linestring<point_type>;
    using multipoint_type = model::multi_point<point_type>;

    segment_type const s { {0, 0}, {1, 1} };
    linestring_type const l { {0, 1}, {1, 1}, {1, 2}, {0, 2}, {0, 3} };
    point_type p;
    multipoint_type mp;
    double distance = 50000;

    srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);
    strategy::line_interpolate
            ::geographic<strategy::vincenty> str(spheroid);

    ex_out << "point interpolation" << std::endl;

    line_interpolate(s, distance, p, str);
    ex_out << "on segment : " << wkt(p) << std::endl;

    line_interpolate(l, distance, p, str);
    ex_out << "on linestring : " << wkt(p) << std::endl << std::endl;

    ex_out << "multipoint interpolation" << std::endl;

    line_interpolate(s, distance, mp, str);
    ex_out << "on segment : " << wkt(mp) << std::endl;

    mp=multipoint_type();
    line_interpolate(l,distance, mp, str);
    ex_out << "on linestring : " << wkt(mp) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[line_interpolate_strategy_output
/*`
Output:
[pre
point interpolation
on segment : POINT(0.318646 0.31869)
on linestring : POINT(0.449226 1.00004)

multipoint interpolation
on segment : MULTIPOINT((0.318646 0.31869),(0.637312 0.63737),(0.956017 0.95603))
on linestring : MULTIPOINT((0.449226 1.00004),(0.898451 1.00001),(1 1.34997),
(1 1.80215),(0.74722 2.00006),(0.297791 2.00006),(0 2.15257),(0 2.60474))
]
*/
//]

TEST_CASE("example: algorithms/line_interpolate_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(point interpolation
on segment : POINT(0.318646 0.31869)
on linestring : POINT(0.449226 1.00004)

multipoint interpolation
on segment : MULTIPOINT((0.318646 0.31869),(0.637312 0.63737),(0.956017 0.95603))
on linestring : MULTIPOINT((0.449226 1.00004),(0.898451 1.00001),(1 1.34997),(1 1.80215),(0.74722 2.00006),(0.297791 2.00006),(0 2.15257),(0 2.60474))
)ex"));
}
}  // namespace ex_algorithms_line_interpolate_strategy

// libs/geometry/doc/src/examples/algorithms/make_2d_point.cpp
namespace ex_algorithms_make_2d_point {
}  // namespace ex_algorithms_make_2d_point (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_make_2d_point {

struct mypoint { float _x, _y; };

}  // namespace ex_algorithms_make_2d_point (reopened after the registration)
#ifndef EX_REGISTERED_8adf51ef0a17
#define EX_REGISTERED_8adf51ef0a17
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_algorithms_make_2d_point::mypoint, float, cs::cartesian, _x, _y)
#endif
namespace ex_algorithms_make_2d_point {

template <typename Point>
void construct_and_display()
{
    using boost::geometry::make;
    using boost::geometry::get;

    Point p = make<Point>(1, 2);

    ex_out << "x=" << get<0>(p) << " y=" << get<1>(p)
        << " (" << typeid(Point).name() << ")"
        << std::endl;
}

int example_main()
{
    construct_and_display<boost::geometry::model::d2::point_xy<double>>();
    construct_and_display<boost::geometry::model::d2::point_xy<int>>();
    construct_and_display<boost::tuple<double, double>>();
    construct_and_display<mypoint>();
    return 0;
    return 0;  // main's implicit return
}

//]

//[make_2d_point_output
/*`
Output (compiled using gcc):
[pre
x=1 y=2 (N5boost8geometry5model2d28point_xyIdNS0_2cs9cartesianEEE)
x=1 y=2 (N5boost8geometry5model2d28point_xyIiNS0_2cs9cartesianEEE)
x=1 y=2 (N5boost6tuples5tupleIddNS0_9null_typeES2_S2_S2_S2_S2_S2_S2_EE)
x=1 y=2 (N5boost7polygon10point_dataIiEE)
x=1 y=2 (7mypoint)
]
*/
//]

TEST_CASE("example: algorithms/make_2d_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(x=1 y=2 (N5boost8geometry5model2d28point_xyIdNS0_2cs9cartesianEEE)
x=1 y=2 (N5boost8geometry5model2d28point_xyIiNS0_2cs9cartesianEEE)
x=1 y=2 (N5boost6tuples5tupleIddNS0_9null_typeES2_S2_S2_S2_S2_S2_S2_EE)
x=1 y=2 (N27ex_algorithms_make_2d_point7mypointE)
)ex"));
#endif
}
}  // namespace ex_algorithms_make_2d_point

// libs/geometry/doc/src/examples/algorithms/make_3d_point.cpp
namespace ex_algorithms_make_3d_point {
int example_main()
{
    using point_type = boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian>;
    const auto p = boost::geometry::make<point_type>(1, 2, 3);
    ex_out << boost::geometry::dsv(p) << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[make_3d_point_output
/*`
Output:
[pre
(1, 2, 3)
]
*/
//]

TEST_CASE("example: algorithms/make_3d_point") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((1, 2, 3)
)ex"));
}
}  // namespace ex_algorithms_make_3d_point

// libs/geometry/doc/src/examples/algorithms/make_inverse.cpp
namespace ex_algorithms_make_inverse {
using namespace boost::geometry;

int example_main()
{

    using point = model::d2::point_xy<double>;
    using box = model::box<point>;

    box all = make_inverse<box>();
    ex_out << dsv(all) << std::endl;
    expand(all, make<box>(0, 0, 3, 4));
    expand(all, make<box>(2, 2, 5, 6));
    ex_out << dsv(all) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[make_inverse_output
/*`
Output:
[pre
((1.79769e+308, 1.79769e+308), (-1.79769e+308, -1.79769e+308))
((0, 0), (5, 6))
]
*/
//]

TEST_CASE("example: algorithms/make_inverse") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((1.79769e+308, 1.79769e+308), (-1.79769e+308, -1.79769e+308))
((0, 0), (5, 6))
)ex"));
}
}  // namespace ex_algorithms_make_inverse

// libs/geometry/doc/src/examples/algorithms/make_with_range.cpp
namespace ex_algorithms_make_with_range {
}  // namespace ex_algorithms_make_with_range (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_algorithms_make_with_range { /*< Necessary to register a C array like {1,2} as a point >*/

int example_main()
{
    using boost::geometry::make;
    using boost::geometry::detail::make::make_points;

    using point = boost::geometry::model::d2::point_xy<double>;
    using linestring = boost::geometry::model::linestring<point>;

    double coordinates[][2] = {{1,2}, {3,4}, {5, 6}}; /*< Initialize with C array points >*/
    linestring ls = make_points<linestring>(coordinates);
    ex_out << boost::geometry::dsv(ls) << std::endl;

    point points[3] = { make<point>(9,8), make<point>(7,6), make<point>(5,4) }; /*< Construct array with points, using make which should work for any point type >*/
    ex_out << boost::geometry::dsv(make_points<linestring>(points)) << std::endl; /*< Construct linestring with point-array and output it as Delimiter Separated Values >*/

    return 0;
    return 0;  // main's implicit return
}

//]


//[make_with_range_output
/*`
Output:
[pre
((1, 2), (3, 4), (5, 6))
((9, 8), (7, 6), (5, 4))
]
*/
//]

TEST_CASE("example: algorithms/make_with_range") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((1, 2), (3, 4), (5, 6))
((9, 8), (7, 6), (5, 4))
)ex"));
}
}  // namespace ex_algorithms_make_with_range

// libs/geometry/doc/src/examples/algorithms/num_geometries.cpp
namespace ex_algorithms_num_geometries {
int example_main()
{
    boost::geometry::model::multi_polygon
        <
            boost::geometry::model::polygon
                <
                    boost::geometry::model::d2::point_xy<double>
                >
        > mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 0,0 10,10 0,0 0),(1 1,1 9,9 1,1 1)),((10 10,10 7,7 10,10 10)))", mp);
    ex_out << "Number of geometries: " << boost::geometry::num_geometries(mp) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[num_geometries_output
/*`
Output:
[pre
 Number of geometries: 2
]
*/
//]

TEST_CASE("example: algorithms/num_geometries") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Number of geometries: 2
)ex"));
}
}  // namespace ex_algorithms_num_geometries

// libs/geometry/doc/src/examples/algorithms/num_interior_rings.cpp
namespace ex_algorithms_num_interior_rings {
int example_main()
{
    boost::geometry::model::multi_polygon
        <
            boost::geometry::model::polygon
                <
                    boost::geometry::model::d2::point_xy<double>
                >
        > mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 0,0 10,10 0,0 0),(1 1,1 9,9 1,1 1)),((10 10,10 7,7 10,10 10)))", mp);
    ex_out << "Number of interior rings: " << boost::geometry::num_interior_rings(mp) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[num_interior_rings_output
/*`
Output:
[pre
 Number of interior rings: 1
]
*/
//]

TEST_CASE("example: algorithms/num_interior_rings") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Number of interior rings: 1
)ex"));
}
}  // namespace ex_algorithms_num_interior_rings

// libs/geometry/doc/src/examples/algorithms/num_points.cpp
namespace ex_algorithms_num_points {
int example_main()
{
    boost::geometry::model::multi_polygon
        <
            boost::geometry::model::polygon
                <
                    boost::geometry::model::d2::point_xy<double>
                >
        > mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 0,0 10,10 0,0 0),(1 1,1 9,9 1,1 1)),((10 10,10 7,7 10,10 10)))", mp);
    ex_out << "Number of points: " << boost::geometry::num_points(mp) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[num_points_output
/*`
Output:
[pre
 Number of points: 12
]
*/
//]

TEST_CASE("example: algorithms/num_points") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Number of points: 12
)ex"));
}
}  // namespace ex_algorithms_num_points

// libs/geometry/doc/src/examples/algorithms/num_segments.cpp
namespace ex_algorithms_num_segments {
int example_main()
{
    boost::geometry::model::multi_polygon
        <
            boost::geometry::model::polygon
                <
                    boost::geometry::model::d2::point_xy<double>, true, false // cw, open polygon
                >
        > mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((0 0,0 10,10 0),(1 1,8 1,1 8)),((10 10,10 20,20 10)))", mp);
    ex_out << "Number of segments: " << boost::geometry::num_segments(mp) << std::endl;
    return 0;
    return 0;  // main's implicit return
}

//]


//[num_segments_output
/*`
Output:
[pre
 Number of segments: 9
]
*/
//]

TEST_CASE("example: algorithms/num_segments") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Number of segments: 9
)ex"));
}
}  // namespace ex_algorithms_num_segments

// libs/geometry/doc/src/examples/algorithms/overlaps.cpp
namespace ex_algorithms_overlaps {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Checks if the two geometries overlaps or not. 
    bg::model::polygon<bg::model::d2::point_xy<double>> poly1;
    bg::read_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", poly1);
    bg::model::polygon<bg::model::d2::point_xy<double>> poly2;
    bg::read_wkt("POLYGON((2 2,2 6,6 7,6 1,2 2))", poly2);
    bool check_overlap = bg::overlaps(poly1, poly2);
    if (check_overlap) {
        ex_out << "Overlaps: Yes" << std::endl;
    } else {
        ex_out << "Overlaps: No" << std::endl;
    }

    bg::model::polygon<bg::model::d2::point_xy<double>> poly3;
    bg::read_wkt("POLYGON((-1 -1,-3 -4,-7 -7,-4 -3,-1 -1))", poly3);
    check_overlap = bg::overlaps(poly1, poly3);
    if (check_overlap) {
        ex_out << "Overlaps: Yes" << std::endl;
    } else {
        ex_out << "Overlaps: No" << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]


//[overlaps_output
/*`
Output:
[pre
Overlaps: Yes

[$img/algorithms/overlaps.png]

Overlaps: No
]
*/
//]

TEST_CASE("example: algorithms/overlaps") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Overlaps: Yes
Overlaps: No
)ex"));
}
}  // namespace ex_algorithms_overlaps

// libs/geometry/doc/src/examples/algorithms/perimeter.cpp
namespace ex_algorithms_perimeter {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Calculate the perimeter of a cartesian polygon
    bg::model::polygon<bg::model::d2::point_xy<double>> poly;
    bg::read_wkt("POLYGON((0 0,3 4,5 -5,-2 -4, 0 0))", poly);
    double perimeter = bg::perimeter(poly);
    ex_out << "Perimeter: " << perimeter << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[perimeter_output
/*`
Output:
[pre
Perimeter: 25.7627
]
*/
//]

TEST_CASE("example: algorithms/perimeter") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Perimeter: 25.7627
)ex"));
}
}  // namespace ex_algorithms_perimeter

// libs/geometry/doc/src/examples/algorithms/relate.cpp
namespace ex_algorithms_relate {
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);

    point_type p(4, 1);

    boost::geometry::de9im::mask mask("T*F**F***"); // within
    bool check = boost::geometry::relate(p, poly, mask);

    ex_out << "relate: " << (check ? "yes" : "no") << std::endl;
     create_svg("relate.svg", poly, p); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[relate_output
/*`
Output:
[pre
relate: yes

[$img/algorithms/within.png]
]

*/
//]

TEST_CASE("example: algorithms/relate") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(relate: yes
)ex"));
}
}  // namespace ex_algorithms_relate

// libs/geometry/doc/src/examples/algorithms/relation.cpp
namespace ex_algorithms_relation {
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);

    point_type p(4, 1);

    boost::geometry::de9im::matrix matrix = boost::geometry::relation(p, poly);
    std::string code = matrix.str();

    ex_out << "relation: " << code << std::endl;
     create_svg("relation.svg", poly, p); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[relation_output
/*`
Output:
[pre
relation: 0FFFFF212

[$img/algorithms/within.png]
]

*/
//]

TEST_CASE("example: algorithms/relation") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(relation: 0FFFFF212
)ex"));
}
}  // namespace ex_algorithms_relation

// libs/geometry/doc/src/examples/algorithms/return_envelope.cpp
namespace ex_algorithms_return_envelope {
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----

int example_main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using box = boost::geometry::model::box<point>;

    const boost::geometry::model::ring<point> ring {
        {4.0, -0.5}, {3.5, 1.0}, {2.0, 1.5}, {3.5, 2.0},
        {4.0, 3.5}, {4.5, 2.0}, {6.0, 1.5}, {4.5, 1.0},
        {4.0, -0.5}
    };

    ex_out
        << "return_envelope:"
        << boost::geometry::dsv(boost::geometry::return_envelope<box>(ring))
        << std::endl;

     create_svg("return_envelope.svg", ring, boost::geometry::return_envelope<box>(ring)); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[return_envelope_output
/*`
Output:
[pre
return_envelope:((2, -0.5), (6, 3.5))

[$img/algorithms/return_envelope.png]
]
*/
//]

TEST_CASE("example: algorithms/return_envelope") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(return_envelope:((2, -0.5), (6, 3.5))
)ex"));
}
}  // namespace ex_algorithms_return_envelope

// libs/geometry/doc/src/examples/algorithms/reverse.cpp
namespace ex_algorithms_reverse {
}  // namespace ex_algorithms_reverse (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_reverse {

int example_main()
{
    using point = boost::tuple<int, int>;
    using polygon = boost::geometry::model::polygon<point>;
    using ring = boost::geometry::model::ring<point>;

    polygon poly;
    boost::geometry::exterior_ring(poly) = {{0, 0}, {0, 9}, {10, 10}, {0, 0}};
    boost::geometry::interior_rings(poly).push_back({{1, 2}, {4, 6}, {2, 8}, {1, 2}});

    double area_before = boost::geometry::area(poly);
    boost::geometry::reverse(poly);
    double area_after = boost::geometry::area(poly);
    ex_out << boost::geometry::dsv(poly) << std::endl;
    ex_out << area_before << " -> " << area_after << std::endl;

    ring r = {{0, 0}, {0, 9}, {8, 8}, {0, 0}};

    area_before = boost::geometry::area(r);
    boost::geometry::reverse(r);
    area_after = boost::geometry::area(r);
    ex_out << boost::geometry::dsv(r) << std::endl;
    ex_out << area_before << " -> " << area_after << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[reverse_output
/*`
Output:
[pre
(((0, 0), (10, 10), (0, 9), (0, 0)), ((1, 2), (2, 8), (4, 6), (1, 2)))
38 -> -38
((0, 0), (8, 8), (0, 9), (0, 0))
36 -> -36
]
*/
//]

TEST_CASE("example: algorithms/reverse") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((((0, 0), (10, 10), (0, 9), (0, 0)), ((1, 2), (2, 8), (4, 6), (1, 2)))
38 -> -38
((0, 0), (8, 8), (0, 9), (0, 0))
36 -> -36
)ex"));
}
}  // namespace ex_algorithms_reverse

// libs/geometry/doc/src/examples/algorithms/simplify.cpp
namespace ex_algorithms_simplify {
int example_main()
{
    using xy = boost::geometry::model::d2::point_xy<double>;

    const boost::geometry::model::linestring<xy> line
        {{1.1, 1.1}, {2.5, 2.1}, {3.1, 3.1}, {4.9, 1.1}, {3.1, 1.9}};

    // Simplify it, using distance of 0.5 units
    boost::geometry::model::linestring<xy> simplified;
    boost::geometry::simplify(line, simplified, 0.5);
    ex_out
        << "  original: " << boost::geometry::dsv(line) << std::endl
        << "simplified: " << boost::geometry::dsv(simplified) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[simplify_output
/*`
Output:
[pre
  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
]
*/
//]

TEST_CASE("example: algorithms/simplify") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
)ex"));
}
}  // namespace ex_algorithms_simplify

// libs/geometry/doc/src/examples/algorithms/simplify_insert.cpp
namespace ex_algorithms_simplify_insert {
int example_main()
{
    using L = boost::geometry::model::linestring<boost::geometry::model::d2::point_xy<double>>;

    L const line{{1.1, 1.1}, {2.5, 2.1}, {3.1, 3.1}, {4.9, 1.1}, {3.1, 1.9}};
    L simplified;
    
    boost::geometry::detail::simplify::simplify_insert(line, std::back_inserter(simplified), 0.5);

    ex_out
        << "  original: " << boost::geometry::dsv(line) << std::endl
        << "simplified: " << boost::geometry::dsv(simplified) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[simplify_inserter_output
/*`
Output:
[pre
  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
]
*/
//]

TEST_CASE("example: algorithms/simplify_insert") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(  original: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
)ex"));
}
}  // namespace ex_algorithms_simplify_insert

// libs/geometry/doc/src/examples/algorithms/sym_difference.cpp
namespace ex_algorithms_sym_difference {
#undef CREATE_SVG_OVERLAY_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_OVERLAY_HPP
#define CREATE_SVG_OVERLAY_HPP



#if defined(HAVE_SVG)
#endif

template <typename Geometry, typename Range>
void create_svg(std::string const& filename, Geometry const& a, Geometry const& b, Range const& range)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    int i = 0;
    for (typename boost::range_value<Range>::type const& g : range)
    {
        mapper.map(g, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
        std::ostringstream out;
        out << i++;
        mapper.text(boost::geometry::return_centroid<point_type>(g), out.str(),
                    "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
    }
#else
    boost::ignore_unused(filename, a, b, range);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_OVERLAY_HPP

// ---- end create_svg_overlay.hpp ----

int example_main()
{
    using polygon = boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double>>;

    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    boost::geometry::model::multi_polygon<polygon> multi;

    boost::geometry::sym_difference(green, blue, multi);

    ex_out
        << "green XOR blue:" << std::endl
        << "total: " << boost::geometry::area(multi) << std::endl;
    int i = 0;
    for (polygon const& p : multi)
    {
        ex_out << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

     create_svg("sym_difference.svg", green, blue, multi); 
    return 0;
    return 0;  // main's implicit return
}

//]


//[sym_difference_output
/*`
Output:
[pre
green XOR blue:
total: 3.1459
0: 0.02375
1: 0.542951
2: 0.0149697
3: 0.226855
4: 0.839424
5: 0.525154
6: 0.015
7: 0.181136
8: 0.128798
9: 0.340083
10: 0.307778

[$img/algorithms/sym_difference.png]

]
*/
//]

TEST_CASE("example: algorithms/sym_difference") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(green XOR blue:
total: 3.1459
0: 0.02375
1: 0.340083
2: 0.015
3: 0.542951
4: 0.525154
5: 0.0149697
6: 0.181136
7: 0.226855
8: 0.128798
9: 0.839424
10: 0.307778
)ex"));
}
}  // namespace ex_algorithms_sym_difference

// libs/geometry/doc/src/examples/algorithms/touches_one_geometry.cpp
namespace ex_algorithms_touches_one_geometry {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Checks if the geometry has self-tangency.
    bg::model::polygon<bg::model::d2::point_xy<double> > poly1;
    bg::read_wkt("POLYGON((0 0,0 3,2 3,2 2,1 2,1 1,2 1,2 2,3 2,3 0,0 0))", poly1);
    bool check_touches = bg::touches(poly1);
    if (check_touches) {
         ex_out << "Touches: Yes" << std::endl;
    } else {
        ex_out << "Touches: No" << std::endl;
    }

    bg::model::polygon<bg::model::d2::point_xy<double> > poly2;
    bg::read_wkt("POLYGON((0 0,0 4,4 4,4 0,2 3,0 0))", poly2);
    check_touches = bg::touches(poly2);
    if (check_touches) {
         ex_out << "Touches: Yes" << std::endl;
    } else {
        ex_out << "Touches: No" << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]


//[touches_one_geometry_output
/*`
Output:
[pre
Touches: Yes

[$img/algorithms/touches_one_geometry.png]

Touches: No
]
*/
//]

TEST_CASE("example: algorithms/touches_one_geometry") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Touches: Yes
Touches: No
)ex"));
}
}  // namespace ex_algorithms_touches_one_geometry

// libs/geometry/doc/src/examples/algorithms/touches_two_geometries.cpp
namespace ex_algorithms_touches_two_geometries {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
    // Checks if the two geometries touch
    bg::model::polygon<bg::model::d2::point_xy<double>> poly1;
    bg::read_wkt("POLYGON((0 0,0 4,4 4,4 0,0 0))", poly1);
    bg::model::polygon<bg::model::d2::point_xy<double>> poly2;
    bg::read_wkt("POLYGON((0 0,0 -4,-4 -4,-4 0,0 0))", poly2);
    bool check_touches = bg::touches(poly1, poly2);
    if (check_touches) {
         ex_out << "Touches: Yes" << std::endl;
    } else {
        ex_out << "Touches: No" << std::endl;
    }

    bg::model::polygon<bg::model::d2::point_xy<double>> poly3;
    bg::read_wkt("POLYGON((1 1,0 -4,-4 -4,-4 0,1 1))", poly3);
    check_touches = bg::touches(poly1, poly3);
    if (check_touches) {
         ex_out << "Touches: Yes" << std::endl;
    } else {
        ex_out << "Touches: No" << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]


//[touches_two_geometries_output
/*`
Output:
[pre
Touches: Yes

[$img/algorithms/touches_two_geometries.png]

Touches: No
]
*/
//]

TEST_CASE("example: algorithms/touches_two_geometries") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Touches: Yes
Touches: No
)ex"));
}
}  // namespace ex_algorithms_touches_two_geometries

// libs/geometry/doc/src/examples/algorithms/transform.cpp
namespace ex_algorithms_transform {
int example_main()
{
    namespace bg = boost::geometry;

    // Select a point near the pole (theta=5.0, phi=15.0)
    bg::model::point<long double, 2, bg::cs::spherical<bg::degree>> p1(15.0, 5.0);

    // Transform from degree to radian. Default strategy is automatically selected,
    // it will convert from degree to radian
    bg::model::point<long double, 2, bg::cs::spherical<bg::radian>> p2;
    bg::transform(p1, p2);

    // Transform from degree (lon-lat) to 3D (x,y,z). Default strategy is automatically selected,
    // it will consider points on a unit sphere
    bg::model::point<long double, 3, bg::cs::cartesian> p3;
    bg::transform(p1, p3);

    ex_out
        << "p1: " << bg::dsv(p1) << std::endl
        << "p2: " << bg::dsv(p2) << std::endl
        << "p3: " << bg::dsv(p3) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[transform_output
/*`
Output:
[pre
p1: (15, 5)
p2: (0.261799, 0.0872665)
p3: (0.084186, 0.0225576, 0.996195)
]
*/
//]

TEST_CASE("example: algorithms/transform") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(p1: (15, 5)
p2: (0.261799, 0.0872665)
p3: (0.084186, 0.0225576, 0.996195)
)ex"));
}
}  // namespace ex_algorithms_transform

// libs/geometry/doc/src/examples/algorithms/transform_with_strategy.cpp
namespace ex_algorithms_transform_with_strategy {
int example_main()
{
    namespace trans = boost::geometry::strategy::transform;
    using boost::geometry::dsv;

    using point_type = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;

    point_type p1(1.0, 1.0);

    // Translate over (1.5, 1.5)
    point_type p2;
    trans::translate_transformer<double, 2, 2> translate(1.5, 1.5);
    boost::geometry::transform(p1, p2, translate);

    // Scale with factor 3.0
    point_type p3;
    trans::scale_transformer<double, 2, 2> scale(3.0);
    boost::geometry::transform(p1, p3, scale);

    // Rotate with respect to the origin (0,0) over 90 degrees (clockwise)
    point_type p4;
    trans::rotate_transformer<boost::geometry::degree, double, 2, 2> rotate(90.0);
    boost::geometry::transform(p1, p4, rotate);

    ex_out
        << "p1: " << dsv(p1) << std::endl
        << "p2: " << dsv(p2) << std::endl
        << "p3: " << dsv(p3) << std::endl
        << "p4: " << dsv(p4) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[transform_with_strategy_output
/*`
Output:
[pre
p1: (1, 1)
p2: (2.5, 2.5)
p3: (3, 3)
p4: (1, -1)
]
*/
//]

TEST_CASE("example: algorithms/transform_with_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(p1: (1, 1)
p2: (2.5, 2.5)
p3: (3, 3)
p4: (1, -1)
)ex"));
}
}  // namespace ex_algorithms_transform_with_strategy

// libs/geometry/doc/src/examples/algorithms/union.cpp
namespace ex_algorithms_union {
#undef CREATE_SVG_OVERLAY_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_OVERLAY_HPP
#define CREATE_SVG_OVERLAY_HPP



#if defined(HAVE_SVG)
#endif

template <typename Geometry, typename Range>
void create_svg(std::string const& filename, Geometry const& a, Geometry const& b, Range const& range)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    int i = 0;
    for (typename boost::range_value<Range>::type const& g : range)
    {
        mapper.map(g, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
        std::ostringstream out;
        out << i++;
        mapper.text(boost::geometry::return_centroid<point_type>(g), out.str(),
                    "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
    }
#else
    boost::ignore_unused(filename, a, b, range);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_OVERLAY_HPP

// ---- end create_svg_overlay.hpp ----

int example_main()
{
    using polygon = boost::geometry::model::polygon<boost::geometry::model::d2::point_xy<double>>;

    polygon green, blue;

    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", green);

    boost::geometry::read_wkt(
        "POLYGON((4.0 -0.5 , 3.5 1.0 , 2.0 1.5 , 3.5 2.0 , 4.0 3.5 , 4.5 2.0 , 6.0 1.5 , 4.5 1.0 , 4.0 -0.5))", blue);

    std::vector<polygon> output;
    boost::geometry::union_(green, blue, output);

    int i = 0;
    ex_out << "green || blue:" << std::endl;
    for(polygon const& p : output)
    {
        ex_out << i++ << ": " << boost::geometry::area(p) << std::endl;
    }

     create_svg("union.svg", green, blue, output); 
    return 0;
    return 0;  // main's implicit return
}

//]


//[union_output
/*`
Output:
[pre
green || blue:
0: 5.64795

[$img/algorithms/union.png]

]
*/
//]

TEST_CASE("example: algorithms/union") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(green || blue:
0: 5.64795
)ex"));
}
}  // namespace ex_algorithms_union

// libs/geometry/doc/src/examples/algorithms/unique.cpp
namespace ex_algorithms_unique {
}  // namespace ex_algorithms_unique (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_algorithms_unique {

int example_main()
{
    boost::geometry::model::polygon<boost::tuple<double, double>> poly;
    boost::geometry::read_wkt("POLYGON((0 0,0 0,0 5,5 5,5 5,5 5,5 0,5 0,0 0,0 0,0 0,0 0))", poly);
    boost::geometry::unique(poly);
    ex_out << boost::geometry::wkt(poly) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[unique_output
/*`
Output:
[pre
POLYGON((0 0,0 5,5 5,5 0,0 0))
]
*/
//]

TEST_CASE("example: algorithms/unique") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(POLYGON((0 0,0 5,5 5,5 0,0 0))
)ex"));
}
}  // namespace ex_algorithms_unique

// libs/geometry/doc/src/examples/algorithms/within.cpp
namespace ex_algorithms_within {
#undef CREATE_SVG_TWO_HPP
// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Code to create SVG for examples

#ifndef CREATE_SVG_TWO_HPP
#define CREATE_SVG_TWO_HPP

#if defined(HAVE_SVG)
#endif

template <typename Geometry1, typename Geometry2>
void create_svg(std::string const& filename, Geometry1 const& a, Geometry2 const& b)
{
#if defined(HAVE_SVG)
    ex_out  << std::endl << "[$img/algorithms/" << boost::replace_all_copy(filename, ".svg", ".png") << "]" << std::endl << std::endl;

    using point_type = typename boost::geometry::point_type<Geometry1>::type;
    std::ofstream svg(filename.c_str());

    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);
    mapper.add(a);
    mapper.add(b);

    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2");
    if (boost::geometry::geometry_id<Geometry2>::value == 1)
    {
        mapper.map(b, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
    }
    else
    {
        mapper.map(b, "opacity:0.8;fill:none;stroke:rgb(255,128,0);stroke-width:4;stroke-dasharray:1,7;stroke-linecap:round");
    }
#else
    boost::ignore_unused(filename, a, b);
#endif
}

// NOTE: convert manually from svg to png using Inkscape ctrl-shift-E
// and copy png to html/img/algorithms/


#endif // CREATE_SVG_TWO_HPP

// ---- end create_svg_two.hpp ----

int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);

    point_type p(4, 1);

    ex_out << "within: " << (boost::geometry::within(p, poly) ? "yes" : "no") << std::endl;
     create_svg("within.svg", poly, p); 
    return 0;
    return 0;  // main's implicit return
}

//]

//[within_output
/*`
Output:
[pre
within: yes

[$img/algorithms/within.png]
]

*/
//]

TEST_CASE("example: algorithms/within") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(within: yes
)ex"));
}
}  // namespace ex_algorithms_within
