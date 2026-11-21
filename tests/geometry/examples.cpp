// mcpplibs.mwrap.boost.geometry — the official examples
// (libs/geometry/example/) ported mechanically: each program runs as a case
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

// the macros of <boost/geometry/geometries/adapted/boost_polygon/point.hpp> (its declarations are in the module)

#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <boost/geometry/geometries/register/linestring.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
import std;
import mcpplibs.mwrap.boost.geometry;
import mcpplibs.mwrap.boost.algorithm;
import mcpplibs.mwrap.boost.core;
import mcpplibs.mwrap.boost.tuple;
import mcpplibs.mwrap.boost.range;
import mcpplibs.mwrap.boost.graph;
import mcpplibs.mwrap.boost.polygon;
import mcpplibs.mwrap.boost.assign;

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

// libs/geometry/example/01_point_example.cpp
namespace ex_01_point_example {
}  // namespace ex_01_point_example (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_01_point_example {
}  // namespace ex_01_point_example (reopened after the registration)
#ifndef EX_REGISTERED_5e756957de5c
#define EX_REGISTERED_5e756957de5c
BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(cs::cartesian)
#endif
namespace ex_01_point_example {
}  // namespace ex_01_point_example (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_01_point_example {


int example_main()
{
    using namespace boost::geometry;

    // Boost.Geometry contains several point types:
    // 1: its own generic type
    model::point<double, 2, cs::cartesian> pt1;

    // 2: its own type targetted to Cartesian (x,y) coordinates
    model::d2::point_xy<double> pt2;

    // 3: it supports Boost tuple's
    boost::tuple<double, double> pt3;

    // 4: it supports normal arrays
    double pt4[2];

    // 5: it supports arrays-as-points from Boost.Array
    boost::array<double, 2> pt5;

    // 6: it supports points from Boost.Polygon
    boost::polygon::point_data<double> pt6;

    // 7: in the past there was a typedef point_2d
    //    But users are now supposted to do that themselves:
    typedef model::d2::point_xy<double> point_2d;
    point_2d pt7;


    // 7: there are more variants, and you can create your own.
    //    (see therefore the custom_point example)

    // All these types are handled the same way. We show here
    // assigning them and calculating distances.
    assign_values(pt1, 1, 1);
    assign_values(pt2, 2, 2);
    assign_values(pt3, 3, 3);
    assign_values(pt4, 4, 4);
    assign_values(pt5, 5, 5);
    assign_values(pt6, 6, 6);
    assign_values(pt7, 7, 7);


    double d1 = distance(pt1, pt2);
    double d2 = distance(pt3, pt4);
    double d3 = distance(pt5, pt6);
    ex_out << "Distances: "
        << d1 << " and " << d2 << " and " << d3 << std::endl;

    // (in case you didn't note, distances can be calculated
    //  from points with different point-types)


    // Several ways of construction and setting point values
    // 1: default, empty constructor, causing no initialization at all
    model::d2::point_xy<double> p1;

    // 2: as shown above, assign_values
    model::d2::point_xy<double> p2;
    assign_values(p2, 1, 1);

    // 3: using "set" function
    //    set uses the concepts behind, such that it can be applied for
    //    every point-type (like assign_values)
    model::d2::point_xy<double> p3;
    set<0>(p3, 1);
    set<1>(p3, 1);
    // set<2>(p3, 1); //will result in compile-error


    // 3: for any point type, and other geometry objects:
    //    there is the "make" object generator
    //    (this one requires to specify the point-type).
    model::d2::point_xy<double> p4 = make<model::d2::point_xy<double> >(1,1);


    // 5: for the d2::point_xy<...> type only: constructor with two values
    model::d2::point_xy<double> p5(1,1);

    // 6: for boost tuples you can of course use make_tuple


    // Some ways of getting point values

    // 1: using the "get" function following the concepts behind
    ex_out << get<0>(p2) << "," << get<1>(p2) << std::endl;

    // 2: for point_xy only
    ex_out << p2.x() << "," << p2.y() << std::endl;

    // 3: using boost-tuples you of course can boost-tuple-methods
    ex_out << pt3.get<0>() << "," << pt3.get<1>() << std::endl;

    // 4: Boost.Geometry supports various output formats, e.g. DSV
    //    (delimiter separated values)
    ex_out << dsv(pt3) << std::endl;

    // 5. or wkt
    ex_out << wkt(p4) << (equals(p4, p5) ? " equals " : " don't equals ") << wkt(p5) << std::endl;

    // There are 3-dimensional points too
    model::point<double, 3, cs::cartesian> d3a, d3b;
    assign_values(d3a, 1, 2, 3);
    assign_values(d3b, 4, 5, 6);
    d3 = distance(d3a, d3b);



    // Other examples show other types of points, geometries and more algorithms

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 01_point_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Distances: 1.41421 and 1.41421 and 1.41421
1,1
1,1
3,3
(3, 3)
POINT(1 1) equals POINT(1 1)
)ex"));
}
}  // namespace ex_01_point_example

// libs/geometry/example/02_linestring_example.cpp
namespace ex_02_linestring_example {
}  // namespace ex_02_linestring_example (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_02_linestring_example {

}  // namespace ex_02_linestring_example (reopened after the registration)
#ifndef EX_REGISTERED_21ad29ae05b8
#define EX_REGISTERED_21ad29ae05b8
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
#endif
namespace ex_02_linestring_example {
}  // namespace ex_02_linestring_example (reopened after the registration)
#ifndef EX_REGISTERED_bd72d1c8d0e5
#define EX_REGISTERED_bd72d1c8d0e5
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)
#endif
namespace ex_02_linestring_example {


template<typename P>
inline void translate_function(P& p)
{
        p.x(p.x() + 100.0);
}

template<typename P>
struct scale_functor
{
    inline void operator()(P& p)
    {
        p.x(p.x() * 1000.0);
        p.y(p.y() * 1000.0);
    }
};


template<typename Point>
struct round_coordinates
{
    typedef typename boost::geometry::coordinate_type<Point>::type coordinate_type;
    coordinate_type m_factor;

    inline round_coordinates(coordinate_type const& factor)
        : m_factor(factor)
    {}

    template <int Dimension>
    inline void round(Point& p)
    {
        coordinate_type c = boost::geometry::get<Dimension>(p) / m_factor;
        int rounded = c;
        boost::geometry::set<Dimension>(p, coordinate_type(rounded) * m_factor);
    }

    inline void operator()(Point& p)
    {
        round<0>(p);
        round<1>(p);
    }
};


int example_main(void)
{
    using namespace boost::geometry;

    // Define a linestring, which is a vector of points, and add some points
    // (we add them deliberately in different ways)
    typedef model::d2::point_xy<double> point_2d;
    typedef model::linestring<point_2d> linestring_2d;
    linestring_2d ls;

    // points can be created using "make" and added to a linestring using the std:: "push_back"
    ls.push_back(make<point_2d>(1.1, 1.1));

    // points can also be assigned using "assign_values" and added to a linestring using "append"
    point_2d lp;
    assign_values(lp, 2.5, 2.1);
    append(ls, lp);

    // Lines can be streamed using DSV (delimiter separated values)
    ex_out << dsv(ls) << std::endl;

    // The bounding box of linestrings can be calculated
    typedef model::box<point_2d> box_2d;
    box_2d b;
    envelope(ls, b);
    ex_out << dsv(b) << std::endl;

    // The length of the line can be calulated
    ex_out << "length: " << length(ls) << std::endl;

    // All things from std::vector can be called, because a linestring is a vector
    ex_out << "number of points 1: " << ls.size() << std::endl;

    // All things from boost ranges can be called because a linestring is considered as a range
    ex_out << "number of points 2: " << boost::size(ls) << std::endl;

    // Generic function from geometry/OGC delivers the same value
    ex_out << "number of points 3: " << num_points(ls) << std::endl;

    // The distance from a point to a linestring can be calculated
    point_2d p(1.9, 1.2);
    ex_out << "distance of " << dsv(p)
        << " to line: " << distance(p, ls) << std::endl;

    // A linestring is a vector. However, some algorithms consider "segments",
    // which are the line pieces between two points of a linestring.
    double d = distance(p, model::segment<point_2d >(ls.front(), ls.back()));
    ex_out << "distance: " << d << std::endl;

    // Add some three points more, let's do it using a classic array.
    // (See documentation for picture of this linestring)
    const double c[][2] = { {3.1, 3.1}, {4.9, 1.1}, {3.1, 1.9} };
    append(ls, c);
    ex_out << "appended: " << dsv(ls) << std::endl;

    // Output as iterator-pair on a vector
    {
        std::vector<point_2d> v;
        std::copy(ls.begin(), ls.end(), std::back_inserter(v));

        ex_out
            << "as vector: "
            << dsv(v)
            << std::endl;
    }

    // All algorithms from std can be used: a linestring is a vector
    std::reverse(ls.begin(), ls.end());
    ex_out << "reversed: " << dsv(ls) << std::endl;
    std::reverse(boost::begin(ls), boost::end(ls));

    // The other way, using a vector instead of a linestring, is also possible
    std::vector<point_2d> pv(ls.begin(), ls.end());
    ex_out << "length: " << length(pv) << std::endl;

    // If there are double points in the line, you can use unique to remove them
    // So we add the last point, print, make a unique copy and print
    {
        // (sidenote, we have to make copies, because
        // ls.push_back(ls.back()) often succeeds but
        // IS dangerous and erroneous!
        point_2d last = ls.back(), first = ls.front();
        ls.push_back(last);
        ls.insert(ls.begin(), first);
    }
    ex_out << "extra duplicate points: " << dsv(ls) << std::endl;

    {
        linestring_2d ls_copy;
        std::unique_copy(ls.begin(), ls.end(), std::back_inserter(ls_copy),
            boost::geometry::equal_to<point_2d>());
        ls = ls_copy;
        ex_out << "uniquecopy: " << dsv(ls) << std::endl;
    }

    // Lines can be simplified. This removes points, but preserves the shape
    linestring_2d ls_simplified;
    simplify(ls, ls_simplified, 0.5);
    ex_out << "simplified: " << dsv(ls_simplified) << std::endl;


    // for_each:
    // 1) Lines can be visited with std::for_each
    // 2) for_each_point is also defined for all geometries
    // 3) for_each_segment is defined for all geometries to all segments
    // 4) loop is defined for geometries to visit segments
    //    with state apart, and to be able to break out (not shown here)
    {
        linestring_2d lscopy = ls;
        std::for_each(lscopy.begin(), lscopy.end(), translate_function<point_2d>);
        for_each_point(lscopy, scale_functor<point_2d>());
        for_each_point(lscopy, translate_function<point_2d>);
        ex_out << "modified line: " << dsv(lscopy) << std::endl;
    }

    // Lines can be clipped using a clipping box. Clipped lines are added to the output iterator
    box_2d cb(point_2d(1.5, 1.5), point_2d(4.5, 2.5));

    std::vector<linestring_2d> clipped;
    intersection(cb, ls, clipped);

    // Also possible: clip-output to a vector of vectors
    std::vector<std::vector<point_2d> > vector_out;
    intersection(cb, ls, vector_out);

    ex_out << "clipped output as vector:" << std::endl;
    for (std::vector<std::vector<point_2d> >::const_iterator it
            = vector_out.begin(); it != vector_out.end(); ++it)
    {
        ex_out << dsv(*it) << std::endl;
    }

    // Calculate the convex hull of the linestring
    model::polygon<point_2d> hull;
    convex_hull(ls, hull);
    ex_out << "Convex hull:" << dsv(hull) << std::endl;

    // All the above assumed 2D Cartesian linestrings. 3D is possible as well
    // Let's define a 3D point ourselves, this time using 'float'
    typedef model::point<float, 3, cs::cartesian> point_3d;
    model::linestring<point_3d> line3;
    line3.push_back(make<point_3d>(1,2,3));
    line3.push_back(make<point_3d>(4,5,6));
    line3.push_back(make<point_3d>(7,8,9));

    // Not all algorithms work on 3d lines. For example convex hull does NOT.
    // But, for example, length, distance, simplify, envelope and stream do.
    ex_out << "3D: length: " << length(line3) << " line: " << dsv(line3) << std::endl;

    // With DSV you can also use other delimiters, e.g. JSON style
    ex_out << "JSON: "
        << dsv(ls, ", ", "[", "]", ", ", "[ ", " ]")
        << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 02_linestring_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((1.1, 1.1), (2.5, 2.1))
((1.1, 1.1), (2.5, 2.1))
length: 1.72047
number of points 1: 2
number of points 2: 2
number of points 3: 2
distance of (1.9, 1.2) to line: 0.383617
distance: 0.383617
appended: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
as vector: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
reversed: ((3.1, 1.9), (4.9, 1.1), (3.1, 3.1), (2.5, 2.1), (1.1, 1.1))
length: 7.54715
extra duplicate points: ((1.1, 1.1), (1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9), (3.1, 1.9))
uniquecopy: ((1.1, 1.1), (2.5, 2.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
simplified: ((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (3.1, 1.9))
modified line: ((101200, 1100), (102600, 2100), (103200, 3100), (105000, 1100), (103200, 1900))
clipped output as vector:
((1.66, 1.5), (2.5, 2.1), (2.74, 2.5))
((3.64, 2.5), (4.5, 1.54444))
((4, 1.5), (3.1, 1.9))
Convex hull:(((1.1, 1.1), (3.1, 3.1), (4.9, 1.1), (1.1, 1.1)))
3D: length: 10.3923 line: ((1, 2, 3), (4, 5, 6), (7, 8, 9))
JSON: [ [1.1, 1.1], [2.5, 2.1], [3.1, 3.1], [4.9, 1.1], [3.1, 1.9] ]
)ex"));
}
}  // namespace ex_02_linestring_example

// libs/geometry/example/03_polygon_example.cpp
namespace ex_03_polygon_example {
}  // namespace ex_03_polygon_example (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_03_polygon_example {


std::string boolstr(bool v)
{
    return v ? "true" : "false";
}

int example_main(void)
{
    using namespace boost::geometry;

    typedef model::d2::point_xy<double> point_2d;
    typedef model::polygon<point_2d> polygon_2d;
    typedef model::box<point_2d> box_2d;

    // Define a polygon and fill the outer ring.
    // In most cases you will read it from a file or database
    polygon_2d poly;
    {
        const double coor[][2] = {
            {2.0, 1.3}, {2.4, 1.7}, {2.8, 1.8}, {3.4, 1.2}, {3.7, 1.6},
            {3.4, 2.0}, {4.1, 3.0}, {5.3, 2.6}, {5.4, 1.2}, {4.9, 0.8}, {2.9, 0.7},
            {2.0, 1.3} // closing point is opening point
            };
        assign_points(poly, coor);
    }

    // Polygons should be closed, and directed clockwise. If you're not sure if that is the case,
    // call the correct algorithm
    correct(poly);

    // Polygons can be streamed as text
    // (or more precisely: as DSV (delimiter separated values))
    ex_out << dsv(poly) << std::endl;

    // As with lines, bounding box of polygons can be calculated
    box_2d b;
    envelope(poly, b);
    ex_out << dsv(b) << std::endl;

    // The area of the polygon can be calulated
    ex_out << "area: " << area(poly) << std::endl;

    // And the centroid, which is the center of gravity
    point_2d cent;
    centroid(poly, cent);
    ex_out << "centroid: " << dsv(cent) << std::endl;


    // The number of points can be requested per ring (using .size())
    // or per polygon (using num_points)
    ex_out << "number of points in outer ring: " << poly.outer().size() << std::endl;

    // Polygons can have one or more inner rings, also called holes, islands, interior rings.
    // Let's add one
    {
        poly.inners().resize(1);
        model::ring<point_2d>& inner = poly.inners().back();

        const double coor[][2] = { {4.0, 2.0}, {4.2, 1.4}, {4.8, 1.9}, {4.4, 2.2}, {4.0, 2.0} };
        assign_points(inner, coor);
    }

    correct(poly);

    ex_out << "with inner ring:" << dsv(poly) << std::endl;
    // The area of the polygon is changed of course
    ex_out << "new area of polygon: " << area(poly) << std::endl;
    centroid(poly, cent);
    ex_out << "new centroid: " << dsv(cent) << std::endl;

    // You can test whether points are within a polygon
    ex_out << "point in polygon:"
        << " p1: "  << boolstr(within(make<point_2d>(3.0, 2.0), poly))
        << " p2: "  << boolstr(within(make<point_2d>(3.7, 2.0), poly))
        << " p3: "  << boolstr(within(make<point_2d>(4.4, 2.0), poly))
        << std::endl;

    // As with linestrings and points, you can derive from polygon to add, for example,
    // fill color and stroke color. Or SRID (spatial reference ID). Or Z-value. Or a property map.
    // We don't show this here.

    // Clip the polygon using a box
    box_2d cb(make<point_2d>(1.5, 1.5), make<point_2d>(4.5, 2.5));
    typedef std::vector<polygon_2d> polygon_list;
    polygon_list v;

    intersection(cb, poly, v);
    ex_out << "Clipped output polygons" << std::endl;
    for (polygon_list::const_iterator it = v.begin(); it != v.end(); ++it)
    {
        ex_out << dsv(*it) << std::endl;
    }

    typedef model::multi_polygon<polygon_2d> polygon_set;
    polygon_set ps;
    union_(cb, poly, ps);

    polygon_2d hull;
    convex_hull(poly, hull);
    ex_out << "Convex hull:" << dsv(hull) << std::endl;

    // If you really want:
    //   You don't have to use a vector, you can define a polygon with a deque
    //   You can specify the container for the points and for the inner rings independantly

    typedef model::polygon<point_2d, true, true, std::deque, std::deque> deque_polygon;
    deque_polygon poly2;
    ring_type<deque_polygon>::type& ring = exterior_ring(poly2);
    append(ring, make<point_2d>(2.8, 1.9));
    append(ring, make<point_2d>(2.9, 2.4));
    append(ring, make<point_2d>(3.3, 2.2));
    append(ring, make<point_2d>(3.2, 1.8));
    append(ring, make<point_2d>(2.8, 1.9));
    ex_out << dsv(poly2) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 03_polygon_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((((2, 1.3), (2.4, 1.7), (2.8, 1.8), (3.4, 1.2), (3.7, 1.6), (3.4, 2), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))
((2, 0.7), (5.4, 3))
area: 4.48
centroid: (4.06923, 1.65056)
number of points in outer ring: 12
with inner ring:(((2, 1.3), (2.4, 1.7), (2.8, 1.8), (3.4, 1.2), (3.7, 1.6), (3.4, 2), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)), ((4, 2), (4.2, 1.4), (4.8, 1.9), (4.4, 2.2), (4, 2)))
new area of polygon: 4.15
new centroid: (4.04663, 1.6349)
point in polygon: p1: false p2: true p3: false
Clipped output polygons
(((2.2, 1.5), (2.4, 1.7), (2.8, 1.8), (3.1, 1.5), (2.2, 1.5)))
(((3.625, 1.5), (3.7, 1.6), (3.4, 2), (3.75, 2.5), (4.5, 2.5), (4.5, 2.125), (4.4, 2.2), (4, 2), (4.16667, 1.5), (3.625, 1.5)))
(((4.5, 1.65), (4.5, 1.5), (4.32, 1.5), (4.5, 1.65)))
Convex hull:(((2, 1.3), (2.4, 1.7), (4.1, 3), (5.3, 2.6), (5.4, 1.2), (4.9, 0.8), (2.9, 0.7), (2, 1.3)))
(((2.8, 1.9), (2.9, 2.4), (3.3, 2.2), (3.2, 1.8), (2.8, 1.9)))
)ex"));
}
}  // namespace ex_03_polygon_example

// libs/geometry/example/04_boost_example.cpp
namespace ex_04_boost_example {
}  // namespace ex_04_boost_example (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_04_boost_example {;


int example_main(void)
{
    using namespace boost::geometry;
    using namespace boost::assign;

    {
        typedef model::d2::point_xy<double> point;
        typedef model::polygon<point> polygon;
        typedef model::linestring<point> linestring;

        // Boost.Assign automatically works for linestring, rings, multi-geometries
        // It works because these are std:: containers

        // Using Boost.Assign operator +=
        linestring ls1;
        ls1 += point(1,2);
        ls1 += point(3,4), point(5,6), point(7,8);
        ex_out << dsv(ls1) << std::endl;

        // Using Boost.Assign operator()
        linestring ls2;
        push_back(ls2)(point(1, 2))(point(3, 4));
        ex_out << dsv(ls2) << std::endl;

        // Using Boost.Assign list_of
        linestring ls3 = list_of(point(1,2))(point(3,4));
        ex_out << dsv(ls3) << std::endl;

        // Using Boost.Assign + Boost.Range
        linestring ls4;
        push_back(ls4)(point(0, 0)).range(ls2).range(ls3);
        ex_out << dsv(ls4) << std::endl;

        // For a ring, it is similar to a linestring.
        // For a multi-point or multi-linestring, it is also similar
        // For a polygon, take the exterior ring or one of the interiors
        polygon p;
        push_back(exterior_ring(p))
            (point(0, 0))
            (point(0, 2))
            (point(2, 2))
            (point(2, 0))
            (point(0, 0))
            ;

        ex_out << dsv(p) << std::endl;
    }

    {
        // It is convenient to combine Boost.Assign on a geometry (e.g. polygon) with tuples.
        typedef model::polygon<boost::tuple<double,double> > polygon;

        polygon p;
        exterior_ring(p) = tuple_list_of(0, 0)(0, 5)(5, 5)(5, 0)(0, 0);

        ex_out << dsv(p) << std::endl;

        // And let it work on the interior_rings as well
        push_back(interior_rings(p))
            (tuple_list_of(1, 1)(2, 1)(2, 2)(1, 2)(1, 1))
            (tuple_list_of(3, 3)(4, 3)(4, 4)(3, 4)(3, 3))
            ;

        ex_out << "Area of " << dsv(p) << " is " << area(p) << std::endl;
    }


    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 04_boost_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(((1, 2), (3, 4), (5, 6), (7, 8))
((1, 2), (3, 4))
((1, 2), (3, 4))
((0, 0), (1, 2), (3, 4), (1, 2), (3, 4))
(((0, 0), (0, 2), (2, 2), (2, 0), (0, 0)))
(((0, 0), (0, 5), (5, 5), (5, 0), (0, 0)))
Area of (((0, 0), (0, 5), (5, 5), (5, 0), (0, 0)), ((1, 1), (2, 1), (2, 2), (1, 2), (1, 1)), ((3, 3), (4, 3), (4, 4), (3, 4), (3, 3))) is 23
)ex"));
}
}  // namespace ex_04_boost_example

// libs/geometry/example/05_a_overlay_polygon_example.cpp
namespace ex_05_a_overlay_polygon_example {
#if defined(HAVE_SVG)
#endif

}  // namespace ex_05_a_overlay_polygon_example (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_05_a_overlay_polygon_example {


int example_main(void)
{
    namespace bg = boost::geometry;

    typedef bg::model::d2::point_xy<double> point_2d;
    typedef bg::model::polygon<point_2d> polygon_2d;


#if defined(HAVE_SVG)
    std::ofstream stream("05_a_intersection_polygon_example.svg");
    bg::svg_mapper<point_2d> svg(stream, 500, 500);
#endif

    // Define a polygons and fill the outer rings.
    polygon_2d a;
    {
        const double c[][2] = {
            {160, 330}, {60, 260}, {20, 150}, {60, 40}, {190, 20}, {270, 130}, {260, 250}, {160, 330}
        };
        bg::assign_points(a, c);
    }
    bg::correct(a);
    ex_out << "A: " << bg::dsv(a) << std::endl;

    polygon_2d b;
    {
        const double c[][2] = {
            {300, 330}, {190, 270}, {150, 170}, {150, 110}, {250, 30}, {380, 50}, {380, 250}, {300, 330}
        };
        bg::assign_points(b, c);
    }
    bg::correct(b);
    ex_out << "B: " << bg::dsv(b) << std::endl;
#if defined(HAVE_SVG)
    svg.add(a);
    svg.add(b);

    svg.map(a, "opacity:0.6;fill:rgb(0,255,0);");
    svg.map(b, "opacity:0.6;fill:rgb(0,0,255);");
#endif


    // Calculate interesection(s)
    std::vector<polygon_2d> intersection;
    bg::intersection(a, b, intersection);

    ex_out << "Intersection of polygons A and B" << std::endl;
    BOOST_FOREACH(polygon_2d const& polygon, intersection)
    {
        ex_out << bg::dsv(polygon) << std::endl;
#if defined(HAVE_SVG)
        svg.map(polygon, "opacity:0.5;fill:none;stroke:rgb(255,0,0);stroke-width:6");
#endif
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 05_a_overlay_polygon_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(A: (((160, 330), (260, 250), (270, 130), (190, 20), (60, 40), (20, 150), (60, 260), (160, 330)))
B: (((300, 330), (380, 250), (380, 50), (250, 30), (150, 110), (150, 170), (190, 270), (300, 330)))
Intersection of polygons A and B
(((216.757, 284.595), (260, 250), (270, 130), (216.667, 56.6667), (150, 110), (150, 170), (190, 270), (216.757, 284.595)))
)ex"));
}
}  // namespace ex_05_a_overlay_polygon_example

// libs/geometry/example/05_b_overlay_linestring_polygon_example.cpp
namespace ex_05_b_overlay_linestring_polygon_example {
#if defined(HAVE_SVG)
#endif

}  // namespace ex_05_b_overlay_linestring_polygon_example (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_05_b_overlay_linestring_polygon_example {;


int example_main(void)
{
    namespace bg = boost::geometry;

    using point_2d = bg::model::d2::point_xy<double>;

    bg::model::linestring<point_2d> ls;
    {
        const double c[][2] = { {0, 1}, {2, 5}, {5, 3} };
        bg::assign_points(ls, c);
    }

    bg::model::polygon<point_2d> p;
    {
        const double c[][2] = { {3, 0}, {0, 3}, {4, 5}, {3, 0} };
        bg::assign_points(p, c);
    }
    bg::correct(p);

#if defined(HAVE_SVG)
    // Create SVG-mapper
    std::ofstream stream("05_b_overlay_linestring_polygon_example.svg");
    bg::svg_mapper<point_2d> svg(stream, 500, 500);
    // Determine extend by adding geometries
    svg.add(p);
    svg.add(ls);
    // Map geometries
    svg.map(ls, "opacity:0.6;stroke:rgb(255,0,0);stroke-width:2;");
    svg.map(p, "opacity:0.6;fill:rgb(0,0,255);");
#endif

    // Calculate intersection points (turn points)
    using segment_ratio = bg::segment_ratio_type<point_2d>::type;
    using turn_info = bg::detail::overlay::turn_info<point_2d, segment_ratio>;
    std::vector<turn_info> turns;
    bg::detail::get_turns::no_interrupt_policy policy;
    bg::strategies::relate::services::default_strategy<bg::model::linestring<point_2d>, bg::model::polygon<point_2d>>::type strategy;
    bg::get_turns<false, false, bg::detail::overlay::assign_null_policy>(ls, p, strategy, turns, policy);

    ex_out << "Intersection of linestring/polygon" << std::endl;
    for (turn_info const& turn : turns)
    {
        std::string action = "intersecting";
        if (turn.operations[0].operation
                == bg::detail::overlay::operation_intersection)
        {
            action = "entering";
        }
        else if (turn.operations[0].operation
                == bg::detail::overlay::operation_union)
        {
            action = "leaving";

        }
        ex_out << action << " polygon at " << bg::dsv(turn.point) << std::endl;
#if defined(HAVE_SVG)
        svg.map(turn.point, "fill:rgb(255,128,0);stroke:rgb(0,0,100);stroke-width:1");
        svg.text(turn.point, action, "fill:rgb(0,0,0);font-family:Arial;font-size:10px");
#endif
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 05_b_overlay_linestring_polygon_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Intersection of linestring/polygon
entering polygon at (0.666667, 2.33333)
leaving polygon at (1.33333, 3.66667)
entering polygon at (2.85714, 4.42857)
leaving polygon at (3.76471, 3.82353)
)ex"));
}
}  // namespace ex_05_b_overlay_linestring_polygon_example

// libs/geometry/example/06_a_transformation_example.cpp
namespace ex_06_a_transformation_example {
}  // namespace ex_06_a_transformation_example (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_06_a_transformation_example {


int example_main()
{
    using namespace boost::geometry;

    typedef model::d2::point_xy<double> point_2d;
    point_2d p(1, 1);
    point_2d p2;

    // Example: translate a point over (5,5)
    strategy::transform::translate_transformer<double, 2, 2> translate(5, 5);

    transform(p, p2, translate);
    ex_out << "transformed point " << boost::geometry::dsv(p2) << std::endl;

    // Transform a polygon
    model::polygon<point_2d> poly, poly2;
    const double coor[][2] = { {0, 0}, {0, 7}, {2, 2}, {2, 0}, {0, 0} };
    // note that for this syntax you have to include the two
    // include files above (c_array.hpp)
    assign_points(poly, coor);
    //read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", poly);
    transform(poly, poly2, translate);

    ex_out << "source      polygon " << boost::geometry::dsv(poly) << std::endl;
    ex_out << "transformed polygon " << boost::geometry::dsv(poly2) << std::endl;

    // Many more transformations are possible:
    // - from Cartesian to Spherical coordinate systems and back
    // - from Cartesian to Cartesian (mapping, affine transformations) and back (inverse)
    // - Map Projections
    // - from Degree to Radian and back in spherical_equatorial or geographic coordinate systems

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 06_a_transformation_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(transformed point (6, 6)
source      polygon (((0, 0), (0, 7), (2, 2), (2, 0), (0, 0)))
transformed polygon (((5, 5), (5, 12), (7, 7), (7, 5), (5, 5)))
)ex"));
}
}  // namespace ex_06_a_transformation_example

// libs/geometry/example/06_b_transformation_example.cpp
namespace ex_06_b_transformation_example {
#if defined(HAVE_SVG)
#endif


using namespace boost::geometry;

struct random_style
{
    random_style()
        : rng(static_cast<int>(std::time(0))), dist(0, 255), colour(rng, dist)
    {}

    std::string fill(double opacity = 1)
    {
        std::ostringstream oss;
        oss << "fill:rgba(" << colour() << "," << colour() << "," << colour() << "," << opacity << ");";
        return oss.str();
    }

    std::string stroke(int width, double opacity = 1)
    {
        std::ostringstream oss;
        oss << "stroke:rgba(" << colour() << "," << colour() << "," << colour() << "," << opacity << ");";
        oss << "stroke-width:" << width  << ";";
        return oss.str();
    }

    template <typename T>
    std::string text(T x, T y, std::string const& text)
    {
        std::ostringstream oss;
        oss << "<text x=\"" << static_cast<int>(x) - 90 << "\" y=\"" << static_cast<int>(y) << "\" font-family=\"Verdana\">" << text << "</text>";
        return oss.str();
    }

    boost::mt19937 rng;
    boost::uniform_int<> dist;
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > colour;
};

template <typename OutputStream>
struct svg_output
{
    svg_output(OutputStream& os, double opacity = 1) : os(os), opacity(opacity)
    {
        os << "<?xml version=\"1.0\" standalone=\"no\"?>\n"
            << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"\n"
            << "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
            << "<svg width=\"100%\" height=\"100%\" version=\"1.1\"\n"
            << "xmlns=\"http://www.w3.org/2000/svg\">" << std::endl;
    }

    ~svg_output()
    {
        os << "</svg>" << std::endl;
    }

    template <typename G>
    void put(G const& g, std::string const& label)
    {
        std::string style_str(style.fill(opacity) + style.stroke(5, opacity));
#if defined(HAVE_SVG)
        os << boost::geometry::svg(g, style_str) << std::endl;
#endif
        if (!label.empty())
        {
            typename point_type<G>::type c;
            centroid(g, c);
            os << style.text(static_cast<int>(get<0>(c)), static_cast<int>(get<1>(c)), label);
        }
    }

private:

    OutputStream& os;
    double opacity;
    random_style style;
};


int example_main()
{
    using namespace boost::geometry::strategy::transform;

    typedef boost::geometry::model::d2::point_xy<double> point_2d;

    try
    {
        std::string file("06_b_transformation_example.svg");
        std::ofstream ofs(file.c_str());
        svg_output<std::ofstream> svg(ofs, 0.5);

        // G1 - create subject for affine transformations
        model::polygon<point_2d> g1;
        read_wkt("POLYGON((50 250, 400 250, 150 50, 50 250))", g1);
        ex_out << "source box:\t" << boost::geometry::dsv(g1) << std::endl;
        svg.put(g1, "g1");

        // G1 - Translate -> G2
        translate_transformer<double, 2, 2> translate(0, 250);
        model::polygon<point_2d> g2;
        transform(g1, g2, translate);
        ex_out << "translated:\t" << boost::geometry::dsv(g2) << std::endl;
        svg.put(g2, "g2=g1.translate(0,250)");

        // G2 - Scale -> G3
        scale_transformer<double, 2, 2> scale(0.5, 0.5);
        model::polygon<point_2d> g3;
        transform(g2, g3, scale);
        ex_out << "scaled:\t" << boost::geometry::dsv(g3) << std::endl;
        svg.put(g3, "g3=g2.scale(0.5,0.5)");

        // G3 - Combine rotate and translate -> G4
        rotate_transformer<degree, double, 2, 2> rotate(45);

        // Compose matrix for the two transformation
        // Create transformer attached to the transformation matrix
        matrix_transformer<double, 2, 2> combined(rotate.matrix() * translate.matrix());

        // Apply transformation to subject geometry point-by-point
        model::polygon<point_2d> g4;
        transform(g3, g4, combined);

        ex_out << "rotated & translated:\t" << boost::geometry::dsv(g4) << std::endl;
        svg.put(g4, "g4 = g3.(rotate(45) * translate(0,250))");

        ex_out << "Saved SVG file:\t" << file << std::endl;
    }
    catch (std::exception const& e)
    {
        ex_out << e.what() << std::endl;
    }
    catch (...)
    {
        ex_out << "unknown error" << std::endl;
    }
    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 06_b_transformation_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(source box:	(((50, 250), (400, 250), (150, 50), (50, 250)))
translated:	(((50, 500), (400, 500), (150, 300), (50, 500)))
scaled:	(((25, 250), (200, 250), (75, 150), (25, 250)))
rotated & translated:	(((371.231, 335.876), (494.975, 212.132), (335.876, 229.81), (371.231, 335.876)))
Saved SVG file:	06_b_transformation_example.svg
)ex"));
}
}  // namespace ex_06_b_transformation_example

// libs/geometry/example/07_a_graph_route_example.cpp
namespace ex_07_a_graph_route_example {
template <typename Geometry, typename Tuple, typename Box>
void read_wkt(std::string const& filename, std::vector<Tuple>& tuples, Box& box)
{
    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            boost::trim(line);
            if (! line.empty() && ! boost::starts_with(line, "#"))
            {
                std::string name;

                // Split at ';', if any
                std::string::size_type pos = line.find(";");
                if (pos != std::string::npos)
                {
                    name = line.substr(pos + 1);
                    line.erase(pos);

                    boost::trim(line);
                    boost::trim(name);
                }

                Geometry geometry;
                boost::geometry::read_wkt(line, geometry);

                Tuple tuple(geometry, name);

                tuples.push_back(tuple);
                boost::geometry::expand(box, boost::geometry::return_envelope<Box>(geometry));
            }
        }
    }
}



// Code to define properties for Boost Graph's
enum vertex_bg_property_t { vertex_bg_property };
enum edge_bg_property_t { edge_bg_property };
}  // namespace ex_07_a_graph_route_example (reopened after the block)
 namespace boost
{
    template <> struct property_kind< ::ex_07_a_graph_route_example::vertex_bg_property_t > { typedef vertex_property_tag type; };  // BOOST_INSTALL_PROPERTY(vertex, bg_property)
    template <> struct property_kind< ::ex_07_a_graph_route_example::edge_bg_property_t > { typedef edge_property_tag type; };  // BOOST_INSTALL_PROPERTY(edge, bg_property)
}
namespace ex_07_a_graph_route_example {

// To calculate distance, declare and construct a strategy with average earth radius
boost::geometry::strategy::distance::haversine<double> const haversine(6372795.0);

// Define properties for vertex
template <typename Point>
struct bg_vertex_property
{
    bg_vertex_property()
    {
        boost::geometry::assign_zero(location);
    }
    bg_vertex_property(Point const& loc)
        : location(loc)
    {
    }

    Point location;
};

// Define properties for edge
template <typename Linestring>
struct bg_edge_property
{
    bg_edge_property(Linestring const& line)
        : m_length(boost::geometry::length(line, haversine))
        , m_line(line)
    {
    }

    inline operator double() const
    {
        return m_length;
    }

    inline Linestring const& line() const
    {
        return m_line;
    }

private :
    double m_length;
    Linestring m_line;
};

// Utility function to add a vertex to a graph. It might exist already. Then do not insert,
// but return vertex descriptor back. It might not exist. Then add it (and return).
// To efficiently handle this, a std::map is used.
template <typename M, typename K, typename G>
inline typename boost::graph_traits<G>::vertex_descriptor find_or_insert(M& map, K const& key, G& graph)
{
    typename M::const_iterator it = map.find(key);
    if (it == map.end())
    {
        // Add a vertex to the graph
        typename boost::graph_traits<G>::vertex_descriptor new_vertex
            = boost::add_vertex(graph);

        // Set the property (= location)
        boost::put(boost::get(vertex_bg_property, graph), new_vertex,
            bg_vertex_property<typename M::key_type>(key));

        // Add to the map, using POINT as key
        map[key] = new_vertex;
        return new_vertex;
    }
    return it->second;
}

template
<
    typename Graph,
    typename RoadTupleVector,
    typename CityTupleVector
>
void add_roads_and_connect_cities(Graph& graph,
            RoadTupleVector const& roads,
            CityTupleVector& cities)
{
    typedef typename boost::range_value<RoadTupleVector>::type road_type;
    typedef typename boost::tuples::element<0, road_type>::type line_type;
    typedef typename boost::geometry::point_type<line_type>::type point_type;

    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_type;

    // Define a map to be used during graph filling
    // Maps from point to vertex-id's
    typedef std::map<point_type, vertex_type, boost::geometry::less<point_type> > map_type;
    map_type map;


    // Fill the graph
    BOOST_FOREACH(road_type const& road, roads)
    {
        line_type const& line = road.template get<0>();
        // Find or add begin/end point of these line
        vertex_type from = find_or_insert(map, line.front(), graph);
        vertex_type to = find_or_insert(map, line.back(), graph);
        boost::add_edge(from, to, bg_edge_property<line_type>(line), graph);
    }

    // Find nearest graph vertex for each city, using the map
    typedef typename boost::range_value<CityTupleVector>::type city_type;
    BOOST_FOREACH(city_type& city, cities)
    {
        double min_distance = 1e300;
        for(typename map_type::const_iterator it = map.begin(); it != map.end(); ++it)
        {
            double dist = boost::geometry::distance(it->first, city.template get<0>());
            if (dist < min_distance)
            {
                min_distance = dist;
                // Set the vertex
                city.template get<2>() = it->second;
            }
        }
    }
}

template <typename Graph, typename Route>
inline void add_edge_to_route(Graph const& graph,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex1,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex2,
            Route& route)
{
    std::pair
        <
            typename boost::graph_traits<Graph>::edge_descriptor,
            bool
        > opt_edge = boost::edge(vertex1, vertex2, graph);
    if (opt_edge.second)
    {
        // Get properties of edge and of vertex
        bg_edge_property<Route> const& edge_prop =
            boost::get(boost::get(edge_bg_property, graph), opt_edge.first);

        bg_vertex_property<typename boost::geometry::point_type<Route>::type> const& vertex_prop =
            boost::get(boost::get(vertex_bg_property, graph), vertex2);

        // Depending on how edge connects to vertex, copy it forward or backward
        if (boost::geometry::equals(edge_prop.line().front(), vertex_prop.location))
        {
            std::copy(edge_prop.line().begin(), edge_prop.line().end(),
                std::back_inserter(route));
        }
        else
        {
            std::reverse_copy(edge_prop.line().begin(), edge_prop.line().end(),
                std::back_inserter(route));
        }
    }
}


template <typename Graph, typename Route>
inline void build_route(Graph const& graph,
            std::vector<typename boost::graph_traits<Graph>::vertex_descriptor> const& predecessors,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex1,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex2,
            Route& route)
{
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_type;
    vertex_type pred = predecessors[vertex2];

    add_edge_to_route(graph, vertex2, pred, route);
    while (pred != vertex1)
    {
        add_edge_to_route(graph, predecessors[pred], pred, route);
        pred = predecessors[pred];
    }
}


int example_main()
{
    // Define a point in the Geographic coordinate system (currently Spherical)
    // (geographic calculations are in an extension; for sample it makes no difference)
    typedef boost::geometry::model::point
        <
            double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>
        > point_type;

    typedef boost::geometry::model::linestring<point_type> line_type;

    // Define the graph, lateron containing the road network
    typedef boost::adjacency_list
        <
            boost::vecS, boost::vecS, boost::undirectedS
            , boost::property<vertex_bg_property_t, bg_vertex_property<point_type> >
            , boost::property<edge_bg_property_t, bg_edge_property<line_type> >
        > graph_type;

    typedef boost::graph_traits<graph_type>::vertex_descriptor vertex_type;


    // Init a bounding box, lateron used to define SVG map
    boost::geometry::model::box<point_type> box;
    boost::geometry::assign_inverse(box);

    // Read the cities
    typedef boost::tuple<point_type, std::string, vertex_type> city_type;
    std::vector<city_type> cities;
    read_wkt<point_type>("data/cities.wkt", cities, box);

    // Read the road network
    typedef boost::tuple<line_type, std::string> road_type;
    std::vector<road_type> roads;
    read_wkt<line_type>("data/roads.wkt", roads, box);


    graph_type graph;

    // Add roads and connect cities
    add_roads_and_connect_cities(graph, roads, cities);

    double const km = 1000.0;
    ex_out << "distances, all in KM" << std::endl
        << std::fixed << std::setprecision(0);

    // Main functionality: calculate shortest routes from/to all cities


    // For the first one, the complete route is stored as a linestring
    bool first = true;
    line_type route;

    int const n = boost::num_vertices(graph);
    BOOST_FOREACH(city_type const& city1, cities)
    {
        std::vector<vertex_type> predecessors(n);
        std::vector<double> costs(n);

        // Call Dijkstra (without named-parameter to be compatible with all VC)
        boost::dijkstra_shortest_paths(graph, city1.get<2>(),
                &predecessors[0], &costs[0],
                boost::get(edge_bg_property, graph),
                boost::get(boost::vertex_index, graph),
                std::less<double>(), std::plus<double>(),
                (std::numeric_limits<double>::max)(), double(),
                boost::dijkstra_visitor<boost::null_visitor>());

        BOOST_FOREACH(city_type const& city2, cities)
        {
            if (! boost::equals(city1.get<1>(), city2.get<1>()))
            {
                double distance = costs[city2.get<2>()] / km;
                double acof = boost::geometry::distance(city1.get<0>(), city2.get<0>(), haversine) / km;

                ex_out
                    << std::setiosflags (std::ios_base::left) << std::setw(15)
                        << city1.get<1>() << " - "
                    << std::setiosflags (std::ios_base::left) << std::setw(15)
                        << city2.get<1>()
                    << " -> through the air: " << std::setw(4) << acof
                    << " , over the road: " << std::setw(4) << distance
                    << std::endl;

                if (first)
                {
                    build_route(graph, predecessors,
                            city1.get<2>(), city2.get<2>(),
                            route);
                    first = false;
                }
            }
        }
    }

#if defined(HAVE_SVG)
    // Create the SVG
    std::ofstream stream("routes.svg");
    boost::geometry::svg_mapper<point_type> mapper(stream, 600, 600);

    // Map roads
    BOOST_FOREACH(road_type const& road, roads)
    {
        mapper.add(road.get<0>());
    }

    BOOST_FOREACH(road_type const& road, roads)
    {
        mapper.map(road.get<0>(),
                "stroke:rgb(128,128,128);stroke-width:1");
    }

    mapper.map(route,
            "stroke:rgb(0, 255, 0);stroke-width:6;opacity:0.5");

    // Map cities
    BOOST_FOREACH(city_type const& city, cities)
    {
        mapper.map(city.get<0>(),
                "fill:rgb(255,255,0);stroke:rgb(0,0,0);stroke-width:1");
        mapper.text(city.get<0>(), city.get<1>(),
                "fill:rgb(0,0,0);font-family:Arial;font-size:10px", 5, 5);
    }
#endif

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 07_a_graph_route_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(distances, all in KM
)ex"));
}
}  // namespace ex_07_a_graph_route_example

// libs/geometry/example/07_b_graph_route_example.cpp
namespace ex_07_b_graph_route_example {
template <typename Geometry, typename Tuple, typename Box>
void read_wkt(std::string const& filename, std::vector<Tuple>& tuples, Box& box)
{
    std::ifstream cpp_file(filename.c_str());
    if (cpp_file.is_open())
    {
        while (! cpp_file.eof() )
        {
            std::string line;
            std::getline(cpp_file, line);
            boost::trim(line);
            if (! line.empty() && ! boost::starts_with(line, "#"))
            {
                std::string name;

                // Split at ';', if any
                std::string::size_type pos = line.find(";");
                if (pos != std::string::npos)
                {
                    name = line.substr(pos + 1);
                    line.erase(pos);

                    boost::trim(line);
                    boost::trim(name);
                }

                Geometry geometry;
                boost::geometry::read_wkt(line, geometry);

                Tuple tuple(geometry, name);

                tuples.push_back(tuple);
                boost::geometry::expand(box, boost::geometry::return_envelope<Box>(geometry));
            }
        }
    }
}

// To calculate distance, declare and construct a strategy with average earth radius
boost::geometry::strategy::distance::haversine<double> const haversine(6372795.0);

// Define properties for vertex
template <typename Point>
struct bg_vertex_property
{
    bg_vertex_property()
    {
        boost::geometry::assign_zero(location);
    }
    bg_vertex_property(Point const& loc)
        : location(loc)
    {
    }

    Point location;
};

// Define properties for edge
template <typename Linestring>
struct bg_edge_property
{
    bg_edge_property(Linestring const& line)
        : length(boost::geometry::length(line, haversine))
        , m_line(line)
    {
    }

    inline Linestring const& line() const
    {
        return m_line;
    }

    double length;
private :
    Linestring m_line;
};

// Utility function to add a vertex to a graph. It might exist already. Then do not insert,
// but return vertex descriptor back. It might not exist. Then add it (and return).
// To efficiently handle this, a std::map is used.
template <typename M, typename K, typename G>
inline typename boost::graph_traits<G>::vertex_descriptor find_or_insert(M& map, K const& key, G& graph)
{
    typename M::const_iterator it = map.find(key);
    if (it == map.end())
    {
        // Add a vertex to the graph
        typename boost::graph_traits<G>::vertex_descriptor new_vertex
            = boost::add_vertex(bg_vertex_property<typename M::key_type>(key), graph);

        // Add to the map, using POINT as key
        map[key] = new_vertex;
        return new_vertex;
    }
    return it->second;
}

template
<
    typename Graph,
    typename RoadTupleVector,
    typename CityTupleVector
>
void add_roads_and_connect_cities(Graph& graph,
            RoadTupleVector const& roads,
            CityTupleVector& cities)
{
    typedef typename boost::range_value<RoadTupleVector>::type road_type;
    typedef typename boost::tuples::element<0, road_type>::type line_type;
    typedef typename boost::geometry::point_type<line_type>::type point_type;

    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_type;

    // Define a map to be used during graph filling
    // Maps from point to vertex-id's
    typedef std::map<point_type, vertex_type, boost::geometry::less<point_type> > map_type;
    map_type map;


    // Fill the graph
    BOOST_FOREACH(road_type const& road, roads)
    {
        line_type const& line = road.template get<0>();
        // Find or add begin/end point of these line
        vertex_type from = find_or_insert(map, line.front(), graph);
        vertex_type to = find_or_insert(map, line.back(), graph);
        boost::add_edge(from, to, bg_edge_property<line_type>(line), graph);
    }

    // Find nearest graph vertex for each city, using the map
    typedef typename boost::range_value<CityTupleVector>::type city_type;
    BOOST_FOREACH(city_type& city, cities)
    {
        double min_distance = 1e300;
        for(typename map_type::const_iterator it = map.begin(); it != map.end(); ++it)
        {
            double dist = boost::geometry::distance(it->first, city.template get<0>());
            if (dist < min_distance)
            {
                min_distance = dist;
                // Set the vertex
                city.template get<2>() = it->second;
            }
        }
    }
}

template <typename Graph, typename Route>
inline void add_edge_to_route(Graph const& graph,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex1,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex2,
            Route& route)
{
    std::pair
        <
            typename boost::graph_traits<Graph>::edge_descriptor,
            bool
        > opt_edge = boost::edge(vertex1, vertex2, graph);
    if (opt_edge.second)
    {
        // Get properties of edge and of vertex
        bg_edge_property<Route> const& edge_prop = graph[opt_edge.first];
        bg_vertex_property
            <
                typename boost::geometry::point_type<Route>::type
            > const& vertex_prop = graph[vertex2];

        // Depending on how edge connects to vertex, copy it forward or backward
        if (boost::geometry::equals(edge_prop.line().front(), vertex_prop.location))
        {
            std::copy(edge_prop.line().begin(), edge_prop.line().end(),
                std::back_inserter(route));
        }
        else
        {
            std::reverse_copy(edge_prop.line().begin(), edge_prop.line().end(),
                std::back_inserter(route));
        }
    }
}


template <typename Graph, typename Route>
inline void build_route(Graph const& graph,
            std::vector<typename boost::graph_traits<Graph>::vertex_descriptor> const& predecessors,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex1,
            typename boost::graph_traits<Graph>::vertex_descriptor vertex2,
            Route& route)
{
    typedef typename boost::graph_traits<Graph>::vertex_descriptor vertex_type;
    vertex_type pred = predecessors[vertex2];

    add_edge_to_route(graph, vertex2, pred, route);
    while (pred != vertex1)
    {
        add_edge_to_route(graph, predecessors[pred], pred, route);
        pred = predecessors[pred];
    }
}


int example_main()
{
    // Define a point in the Geographic coordinate system (currently spherical-equatorial)
    // (geographic calculations are in an extension; for sample it makes no difference)
    typedef boost::geometry::model::point
        <
            double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>
        > point_type;

    typedef boost::geometry::model::linestring<point_type> line_type;

    // Define the graph, lateron containing the road network
    // With this, specify bundled properties for vertex and edge,
    // as in http://www.boost.org/doc/libs/1_43_0/libs/graph/doc/bundles.html
    typedef boost::adjacency_list
        <
            boost::vecS, boost::vecS, boost::undirectedS
            , bg_vertex_property<point_type> // bundled
            , bg_edge_property<line_type>
        > graph_type;

    typedef boost::graph_traits<graph_type>::vertex_descriptor vertex_type;


    // Init a bounding box, lateron used to define SVG map
    boost::geometry::model::box<point_type> box;
    boost::geometry::assign_inverse(box);

    graph_type graph;

    // Read the cities
    typedef boost::tuple<point_type, std::string, vertex_type> city_type;
    std::vector<city_type> cities;
    read_wkt<point_type>("data/cities.wkt", cities, box);

    // Read the road network
    typedef boost::tuple<line_type, std::string> road_type;
    std::vector<road_type> roads;
    read_wkt<line_type>("data/roads.wkt", roads, box);


    // Add roads and connect cities
    add_roads_and_connect_cities(graph, roads, cities);

    double const km = 1000.0;
    ex_out << "distances, all in KM" << std::endl
        << std::fixed << std::setprecision(0);

    // Main functionality: calculate shortest routes from/to all cities

    // For the first one, the complete route is stored as a linestring
    bool first = true;
    line_type route;

    int const n = boost::num_vertices(graph);
    BOOST_FOREACH(city_type const& city1, cities)
    {
        std::vector<vertex_type> predecessors(n);
        std::vector<double> costs(n);

        // Call Dijkstra (without named-parameter to be compatible with all VC)
        boost::dijkstra_shortest_paths(graph, city1.get<2>(),
                &predecessors[0], &costs[0],
                boost::get(&bg_edge_property<line_type>::length, graph),
                boost::get(boost::vertex_index, graph),
                std::less<double>(), std::plus<double>(),
                (std::numeric_limits<double>::max)(), double(),
                boost::dijkstra_visitor<boost::null_visitor>());

        BOOST_FOREACH(city_type const& city2, cities)
        {
            if (! boost::equals(city1.get<1>(), city2.get<1>()))
            {
                double distance = costs[city2.get<2>()] / km;
                double acof = boost::geometry::distance(city1.get<0>(), city2.get<0>(), haversine) / km;

                ex_out
                    << std::setiosflags (std::ios_base::left) << std::setw(15)
                        << city1.get<1>() << " - "
                    << std::setiosflags (std::ios_base::left) << std::setw(15)
                        << city2.get<1>()
                    << " -> through the air: " << std::setw(4) << acof
                    << " , over the road: " << std::setw(4) << distance
                    << std::endl;

                if (first)
                {
                    build_route(graph, predecessors,
                            city1.get<2>(), city2.get<2>(),
                            route);
                    first = false;
                }
            }
        }
    }

#if defined(HAVE_SVG)
    // Create the SVG
    std::ofstream stream("routes.svg");
    boost::geometry::svg_mapper<point_type> mapper(stream, 600, 600);

    // Map roads
    BOOST_FOREACH(road_type const& road, roads)
    {
        mapper.add(road.get<0>());
    }

    BOOST_FOREACH(road_type const& road, roads)
    {
        mapper.map(road.get<0>(),
                "stroke:rgb(128,128,128);stroke-width:1");
    }

    mapper.map(route,
            "stroke:rgb(0, 255, 0);stroke-width:6;opacity:0.5");

    // Map cities
    BOOST_FOREACH(city_type const& city, cities)
    {
        mapper.map(city.get<0>(),
                "fill:rgb(255,255,0);stroke:rgb(0,0,0);stroke-width:1");
        mapper.text(city.get<0>(), city.get<1>(),
                "fill:rgb(0,0,0);font-family:Arial;font-size:10px", 5, 5);
    }
#endif

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: 07_b_graph_route_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(distances, all in KM
)ex"));
}
}  // namespace ex_07_b_graph_route_example

// libs/geometry/example/c01_custom_point_example.cpp
namespace ex_c01_custom_point_example {
struct my_color_point
{
    double red, green, blue;
};

// Sample point, having an int array defined
struct my_array_point
{
    int c[3];
};

// Sample point, having x/y
struct my_2d
{
    float x,y;
};

// Sample class, protected and construction-time-only x/y,
// Can (of course) only used in algorithms which take const& points
class my_class_ro
{
public:
    my_class_ro(double x, double y) : m_x(x), m_y(y) {}
    double x() const { return m_x; }
    double y() const { return m_y; }
private:
    double m_x, m_y;
};

// Sample class using references for read/write
class my_class_rw
{
public:
    const double& x() const { return m_x; }
    const double& y() const { return m_y; }
    double& x() { return m_x; }
    double& y() { return m_y; }
private:
    double m_x, m_y;
};

// Sample class using getters / setters
class my_class_gs
{
public:
    const double get_x() const { return m_x; }
    const double get_y() const { return m_y; }
    void set_x(double v) { m_x = v; }
    void set_y(double v) { m_y = v; }
private:
    double m_x, m_y;
};

// Sample point within a namespace
namespace my
{
    struct my_namespaced_point
    {
        double x, y;
    };
}



}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_09503c3d7043
#define EX_REGISTERED_09503c3d7043
BOOST_GEOMETRY_REGISTER_POINT_3D(::ex_c01_custom_point_example::my_color_point, double, cs::cartesian, red, green, blue)
#endif
namespace ex_c01_custom_point_example {
}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_2ace65025c31
#define EX_REGISTERED_2ace65025c31
BOOST_GEOMETRY_REGISTER_POINT_3D(::ex_c01_custom_point_example::my_array_point, int, cs::cartesian, c[0], c[1], c[2])
#endif
namespace ex_c01_custom_point_example {
}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_ef9ed5547181
#define EX_REGISTERED_ef9ed5547181
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c01_custom_point_example::my_2d, float, cs::cartesian, x, y)
#endif
namespace ex_c01_custom_point_example {
}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_59e32d41bf1e
#define EX_REGISTERED_59e32d41bf1e
BOOST_GEOMETRY_REGISTER_POINT_2D_CONST(::ex_c01_custom_point_example::my_class_ro, double, cs::cartesian, x(), y())
#endif
namespace ex_c01_custom_point_example {
}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_5dd5b85105a3
#define EX_REGISTERED_5dd5b85105a3
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c01_custom_point_example::my_class_rw, double, cs::cartesian, x(), y())
#endif
namespace ex_c01_custom_point_example {
}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_25e58b3c52d4
#define EX_REGISTERED_25e58b3c52d4
BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(::ex_c01_custom_point_example::my_class_gs, double, cs::cartesian, get_x, get_y, set_x, set_y)
#endif
namespace ex_c01_custom_point_example {

}  // namespace ex_c01_custom_point_example (reopened after the registration)
#ifndef EX_REGISTERED_280ba8dfafc2
#define EX_REGISTERED_280ba8dfafc2
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c01_custom_point_example::my::my_namespaced_point, double, cs::cartesian, x, y)
#endif
namespace ex_c01_custom_point_example {


int example_main()
{
    // Create 2 instances of our custom color point
    my_color_point c1 = boost::geometry::make<my_color_point>(255, 3, 233);
    my_color_point c2 = boost::geometry::make<my_color_point>(0, 50, 200);

    // The distance between them can be calculated using the cartesian method (=pythagoras)
    // provided with the library, configured by the coordinate_system type of the point
    ex_out << "color distance "
        << boost::geometry::dsv(c1) << " to "
        << boost::geometry::dsv(c2) << " is "
        << boost::geometry::distance(c1,c2) << std::endl;

    my_array_point a1 = {{0}};
    my_array_point a2 = {{0}};
    boost::geometry::assign_values(a1, 1, 2, 3);
    boost::geometry::assign_values(a2, 3, 2, 1);

    ex_out << "color distance "
        << boost::geometry::dsv(a1) << " to "
        << boost::geometry::dsv(a2) << " is "
        << boost::geometry::distance(a1,a2) << std::endl;

    my_2d p1 = {1, 5};
    my_2d p2 = {3, 4};
    ex_out << "float distance "
        << boost::geometry::dsv(p1) << " to "
        << boost::geometry::dsv(p2) << " is "
        << boost::geometry::distance(p1,p2) << std::endl;

    my_class_ro cro1(1, 2);
    my_class_ro cro2(3, 4);
    ex_out << "class ro distance "
        << boost::geometry::dsv(cro1) << " to "
        << boost::geometry::dsv(cro2) << " is "
        << boost::geometry::distance(cro1,cro2) << std::endl;

    my_class_rw crw1;
    my_class_rw crw2;
    boost::geometry::assign_values(crw1, 1, 2);
    boost::geometry::assign_values(crw2, 3, 4);
    ex_out << "class r/w distance "
        << boost::geometry::dsv(crw1) << " to "
        << boost::geometry::dsv(crw2) << " is "
        << boost::geometry::distance(crw1,crw2) << std::endl;

    my_class_gs cgs1;
    my_class_gs cgs2;
    boost::geometry::assign_values(cgs1, 1, 2);
    boost::geometry::assign_values(cgs2, 3, 4);
    ex_out << "class g/s distance "
        << boost::geometry::dsv(crw1) << " to "
        << boost::geometry::dsv(crw2) << " is "
        << boost::geometry::distance(cgs1,cgs2) << std::endl;

    my::my_namespaced_point nsp1 = boost::geometry::make<my::my_namespaced_point>(1, 2);
    my::my_namespaced_point nsp2 = boost::geometry::make<my::my_namespaced_point>(3, 4);
    ex_out << "namespaced distance "
        << boost::geometry::dsv(nsp1) << " to "
        << boost::geometry::dsv(nsp2) << " is "
        << boost::geometry::distance(nsp1,nsp2) << std::endl;


    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c01_custom_point_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(color distance (255, 3, 233) to (0, 50, 200) is 261.387
color distance (1, 2, 3) to (3, 2, 1) is 2.82843
float distance (1, 5) to (3, 4) is 2.23607
class ro distance (1, 2) to (3, 4) is 2.82843
class r/w distance (1, 2) to (3, 4) is 2.82843
class g/s distance (1, 2) to (3, 4) is 2.82843
namespaced distance (1, 2) to (3, 4) is 2.82843
)ex"));
}
}  // namespace ex_c01_custom_point_example

// libs/geometry/example/c02_custom_box_example.cpp
namespace ex_c02_custom_box_example {
struct my_point
{
    double x, y;
};

struct my_int_point
{
    int x, y;
};

struct my_box
{
    my_point ll, ur;
};

struct my_box_ltrb
{
    int left, top, right, bottom;
};

struct my_box_4
{
    double coors[4];
};

template <typename P>
struct my_box_t
{
    P ll, ur;
};

}  // namespace ex_c02_custom_box_example (reopened after the registration)
#ifndef EX_REGISTERED_4cdefb746c46
#define EX_REGISTERED_4cdefb746c46
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c02_custom_box_example::my_point, double, cs::cartesian, x, y)
#endif
namespace ex_c02_custom_box_example {
}  // namespace ex_c02_custom_box_example (reopened after the registration)
#ifndef EX_REGISTERED_60b389379561
#define EX_REGISTERED_60b389379561
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c02_custom_box_example::my_int_point, int, cs::cartesian, x, y)
#endif
namespace ex_c02_custom_box_example {
}  // namespace ex_c02_custom_box_example (reopened after the registration)
#ifndef EX_REGISTERED_dd48162d2f81
#define EX_REGISTERED_dd48162d2f81
BOOST_GEOMETRY_REGISTER_BOX(::ex_c02_custom_box_example::my_box, ::ex_c02_custom_box_example::my_point, ll, ur)
#endif
namespace ex_c02_custom_box_example {
}  // namespace ex_c02_custom_box_example (reopened after the registration)
#ifndef EX_REGISTERED_8b64624b152d
#define EX_REGISTERED_8b64624b152d
BOOST_GEOMETRY_REGISTER_BOX_TEMPLATED(::ex_c02_custom_box_example::my_box_t, ll, ur)
#endif
namespace ex_c02_custom_box_example {
}  // namespace ex_c02_custom_box_example (reopened after the registration)
#ifndef EX_REGISTERED_e9b8677b5347
#define EX_REGISTERED_e9b8677b5347
BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(::ex_c02_custom_box_example::my_box_ltrb, ::ex_c02_custom_box_example::my_int_point, left, top, right, bottom)
#endif
namespace ex_c02_custom_box_example {
}  // namespace ex_c02_custom_box_example (reopened after the registration)
#ifndef EX_REGISTERED_91c551981ef4
#define EX_REGISTERED_91c551981ef4
BOOST_GEOMETRY_REGISTER_BOX_2D_4VALUES(::ex_c02_custom_box_example::my_box_4, ::ex_c02_custom_box_example::my_point, coors[0], coors[1], coors[2], coors[3])
#endif
namespace ex_c02_custom_box_example {

int example_main()
{
    my_point p = boost::geometry::make<my_point>(3.5, 3.5);
    my_box b = boost::geometry::make<my_box>(0, 0, 2, 2);
    my_box_ltrb b1 = boost::geometry::make<my_box_ltrb>(0, 0, 3, 3);
    my_box_4 b4 = boost::geometry::make<my_box_4>(0, 0, 4, 4);
    my_box_t<my_point> bt = boost::geometry::make<my_box_t<my_point> >(0, 0, 5, 5);

    ex_out << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(b)
            << " : " << int(boost::geometry::within(p, b)) << std::endl;
    ex_out << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(b1)
            << " : " << int(boost::geometry::within(p, b1)) << std::endl;
    ex_out << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(b4)
            << " : " << int(boost::geometry::within(p, b4)) << std::endl;
    ex_out << boost::geometry::dsv(p) << " IN " << boost::geometry::dsv(bt)
            << " : " << int(boost::geometry::within(p, bt)) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c02_custom_box_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((3.5, 3.5) IN ((0, 0), (2, 2)) : 0
(3.5, 3.5) IN ((0, 0), (3, 3)) : 0
(3.5, 3.5) IN ((0, 0), (4, 4)) : 1
(3.5, 3.5) IN ((0, 0), (5, 5)) : 1
)ex"));
}
}  // namespace ex_c02_custom_box_example

// libs/geometry/example/c04_a_custom_triangle_example.cpp
namespace ex_c04_a_custom_triangle_example {
struct triangle : public std::array<boost::geometry::model::d2::point_xy<double>, 4>
{
    inline void close()
    {
        (*this)[3] = (*this)[0];
    }
};


// Register triangle as a ring
}  // namespace ex_c04_a_custom_triangle_example (reopened after the registration)
#ifndef EX_REGISTERED_96a72a45ac0b
#define EX_REGISTERED_96a72a45ac0b
BOOST_GEOMETRY_REGISTER_RING(::ex_c04_a_custom_triangle_example::triangle)
#endif
namespace ex_c04_a_custom_triangle_example {


// Specializations of algorithms, where useful. If not specialized the default ones
// (for linear rings) will be used for triangle. Which is OK as long as the triangle
// is closed, that means, has 4 points (the last one being the first).
}  // namespace ex_c04_a_custom_triangle_example (reopened after the block)
 namespace boost { namespace geometry {

template<>
inline auto area<::ex_c04_a_custom_triangle_example::triangle>(const ::ex_c04_a_custom_triangle_example::triangle& t)
{
    /*         C
              / \
             /   \
            A-----B

           ((Bx - Ax) * (Cy - Ay)) - ((Cx - Ax) * (By - Ay))
           -------------------------------------------------
                                   2
    */

    return 0.5 * ((t[1].x() - t[0].x()) * (t[2].y() - t[0].y())
                - (t[2].x() - t[0].x()) * (t[1].y() - t[0].y()));
}

}}
namespace ex_c04_a_custom_triangle_example { // namespace boost::geometry

int example_main()
{
    triangle t;

    t[0].x(0);
    t[0].y(0);
    t[1].x(5);
    t[1].y(0);
    t[2].x(2.5);
    t[2].y(2.5);

    t.close();

    ex_out << "Triangle: " << boost::geometry::dsv(t) << std::endl;
    ex_out << "Area: " << boost::geometry::area(t) << std::endl;

    boost::geometry::model::d2::point_xy<double> c;
    boost::geometry::centroid(t, c);
    ex_out << "Centroid: " << boost::geometry::dsv(c) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c04_a_custom_triangle_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Triangle: ((0, 0), (5, 0), (2.5, 2.5), (0, 0))
Area: 6.25
Centroid: (2.5, 0.833333)
)ex"));
}
}  // namespace ex_c04_a_custom_triangle_example

// libs/geometry/example/c04_b_custom_triangle_example.cpp
namespace ex_c04_b_custom_triangle_example {
}  // namespace ex_c04_b_custom_triangle_example (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_c04_b_custom_triangle_example {


template <typename P>
struct triangle : public std::array<P, 3>
{
};


// Register triangle<P> as a ring
}  // namespace ex_c04_b_custom_triangle_example (reopened after the registration)
#ifndef EX_REGISTERED_d9078428c2a5
#define EX_REGISTERED_d9078428c2a5
BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(::ex_c04_b_custom_triangle_example::triangle)
#endif
namespace ex_c04_b_custom_triangle_example {


}  // namespace ex_c04_b_custom_triangle_example (reopened after the block)
 namespace boost { namespace geometry { namespace dispatch {

// Specializations of area dispatch structure, implement algorithm
template<typename Point>
struct area<::ex_c04_b_custom_triangle_example::triangle<Point>, ring_tag>
{
    template <typename Strategy>
    static inline double apply(::ex_c04_b_custom_triangle_example::triangle<Point> const& t, Strategy const&)
    {
        return 0.5  * ((get<0>(t[1]) - get<0>(t[0])) * (get<1>(t[2]) - get<1>(t[0]))
                     - (get<0>(t[2]) - get<0>(t[0])) * (get<1>(t[1]) - get<1>(t[0])));
    }
};

}}}
namespace ex_c04_b_custom_triangle_example { // namespace boost::geometry::dispatch


int example_main()
{
    //triangle<boost::geometry::point_xy<double> > t;
    triangle<boost::tuple<double, double> > t;
    t[0] = boost::make_tuple(0, 0);
    t[1] = boost::make_tuple(5, 0);
    t[2] = boost::make_tuple(2.5, 2.5);

    ex_out << "Triangle: " << boost::geometry::dsv(t) << std::endl;
    ex_out << "Area: " << boost::geometry::area(t) << std::endl;

    //boost::geometry::point_xy<double> c;
    boost::tuple<double, double> c;
    boost::geometry::centroid(t, c);
    ex_out << "Centroid: " << boost::geometry::dsv(c) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c04_b_custom_triangle_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Triangle: ((0, 0), (5, 0), (2.5, 2.5))
Area: 6.25
Centroid: (2.5, 0.833333)
)ex"));
}
}  // namespace ex_c04_b_custom_triangle_example

// libs/geometry/example/c05_custom_point_pointer_example.cpp
namespace ex_c05_custom_point_pointer_example {
}  // namespace ex_c05_custom_point_pointer_example (reopened after the registration)
#ifndef EX_REGISTERED_21ad29ae05b8
#define EX_REGISTERED_21ad29ae05b8
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::vector)
#endif
namespace ex_c05_custom_point_pointer_example {
}  // namespace ex_c05_custom_point_pointer_example (reopened after the registration)
#ifndef EX_REGISTERED_bd72d1c8d0e5
#define EX_REGISTERED_bd72d1c8d0e5
BOOST_GEOMETRY_REGISTER_LINESTRING_TEMPLATED(std::deque)
#endif
namespace ex_c05_custom_point_pointer_example {

// Sample point, having x/y
struct my_point
{
    double x,y;
};


}  // namespace ex_c05_custom_point_pointer_example (reopened after the block)
 namespace boost { namespace geometry { namespace traits {

template<> struct tag<::ex_c05_custom_point_pointer_example::my_point>
{ typedef point_tag type; };

template<> struct coordinate_type<::ex_c05_custom_point_pointer_example::my_point>
{ typedef double type; };

template<> struct coordinate_system<::ex_c05_custom_point_pointer_example::my_point>
{ typedef cs::cartesian type; };

template<> struct dimension<::ex_c05_custom_point_pointer_example::my_point> : boost::mpl::int_<2> {};

template<>
struct access<::ex_c05_custom_point_pointer_example::my_point, 0>
{
    static double get(::ex_c05_custom_point_pointer_example::my_point const& p)
    {
        return p.x;
    }

    static void set(::ex_c05_custom_point_pointer_example::my_point& p, double const& value)
    {
        p.x = value;
    }
};

template<>
struct access<::ex_c05_custom_point_pointer_example::my_point, 1>
{
    static double get(::ex_c05_custom_point_pointer_example::my_point const& p)
    {
        return p.y;
    }

    static void set(::ex_c05_custom_point_pointer_example::my_point& p, double const& value)
    {
        p.y = value;
    }
};

}}}
namespace ex_c05_custom_point_pointer_example { // namespace boost::geometry::traits



int example_main()
{
    typedef std::vector<my_point*> ln;
    ln myline;
    for (float i = 0.0; i < 10.0; i++)
    {
        my_point* p = new my_point;
        p->x = i;
        p->y = i + 1;
        myline.push_back(p);
    }

    ex_out << boost::geometry::length(myline) << std::endl;

    typedef boost::geometry::model::d2::point_xy<double> point_2d;
    typedef boost::geometry::model::box<point_2d> box_2d;
    box_2d cb(point_2d(1.5, 1.5), point_2d(4.5, 4.5));

    // This will NOT work because would need dynamicly allocating memory for point* in algorithms:
    // std::vector<ln> clipped;
    //boost::geometry::intersection(cb, myline, clipped);

    // This works because outputs to a normal struct point, no point*
    typedef boost::geometry::model::linestring<point_2d> linestring_2d;
    std::vector<linestring_2d> clipped;
    boost::geometry::strategy::intersection::liang_barsky<box_2d, point_2d> strategy;
    boost::geometry::detail::intersection::clip_range_with_box<linestring_2d>(cb,
                    myline, std::back_inserter(clipped), strategy);


    ex_out << boost::geometry::length(clipped.front()) << std::endl;

    // free
    BOOST_FOREACH(my_point* p, myline)
    {
        delete p;
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c05_custom_point_pointer_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(12.7279
2.82843
)ex"));
}
}  // namespace ex_c05_custom_point_pointer_example

// libs/geometry/example/c06_custom_polygon_example.cpp
namespace ex_c06_custom_polygon_example {
struct my_point
{
    my_point(double an_x = 0, double an_y = 0)
        : x(an_x)
        , y(an_y)
    {}

    double x, y;
};

struct my_ring : std::deque<my_point>
{};

// Define a struct of a polygon, having always two holes
// (of course this can be implemented differently, usually
// with a vector or deque, but it is just an exampe)
struct my_polygon
{
    // required for a polygon: an outer ring...
    my_ring boundary;
    // ... and a Boost.Range compatible inner ring collection
    std::array<my_ring, 2> holes;

    // just for the sample
    std::string name;

    my_polygon(std::string const& n = "") : name(n) {}
};


// We can conveniently use macro's to register point and ring
}  // namespace ex_c06_custom_polygon_example (reopened after the registration)
#ifndef EX_REGISTERED_1ad755a15e0a
#define EX_REGISTERED_1ad755a15e0a
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c06_custom_polygon_example::my_point, double, cs::cartesian, x, y)
#endif
namespace ex_c06_custom_polygon_example {
}  // namespace ex_c06_custom_polygon_example (reopened after the registration)
#ifndef EX_REGISTERED_28a40e3fd149
#define EX_REGISTERED_28a40e3fd149
BOOST_GEOMETRY_REGISTER_RING(::ex_c06_custom_polygon_example::my_ring)
#endif
namespace ex_c06_custom_polygon_example {



// There is currently no registration macro for polygons
// and besides that a std::array<T,N> in a macro would
// be very specific, so we show it "by hand":
}  // namespace ex_c06_custom_polygon_example (reopened after the block)
 namespace boost { namespace geometry { namespace traits
{

template<> struct tag<::ex_c06_custom_polygon_example::my_polygon> { typedef polygon_tag type; };
template<> struct ring_const_type<::ex_c06_custom_polygon_example::my_polygon> { typedef ::ex_c06_custom_polygon_example::my_ring const& type; };
template<> struct ring_mutable_type<::ex_c06_custom_polygon_example::my_polygon> { typedef ::ex_c06_custom_polygon_example::my_ring& type; };

template<> struct interior_const_type<::ex_c06_custom_polygon_example::my_polygon>
{
    typedef std::array<::ex_c06_custom_polygon_example::my_ring, 2> const& type;
};

template<> struct interior_mutable_type<::ex_c06_custom_polygon_example::my_polygon>
{
    typedef std::array<::ex_c06_custom_polygon_example::my_ring, 2>& type;
};

template<> struct exterior_ring<::ex_c06_custom_polygon_example::my_polygon>
{
    static ::ex_c06_custom_polygon_example::my_ring& get(::ex_c06_custom_polygon_example::my_polygon& p)
    {
        return p.boundary;
    }

    static ::ex_c06_custom_polygon_example::my_ring const& get(::ex_c06_custom_polygon_example::my_polygon const& p)
    {
        return p.boundary;
    }
};

template<> struct interior_rings<::ex_c06_custom_polygon_example::my_polygon>
{
    typedef std::array<::ex_c06_custom_polygon_example::my_ring, 2> holes_type;

    static holes_type& get(::ex_c06_custom_polygon_example::my_polygon& p)
    {
        return p.holes;
    }

    static holes_type const& get(::ex_c06_custom_polygon_example::my_polygon const& p)
    {
        return p.holes;
    }
};

}}}
namespace ex_c06_custom_polygon_example { // namespace boost::geometry::traits



int example_main()
{
    my_polygon p1("my polygon");

    // Fill it the my-way, triangle
    p1.boundary.push_back(my_point(2, 0));
    p1.boundary.push_back(my_point(1, 5));
    p1.boundary.push_back(my_point(7, 6));
    p1.boundary.push_back(my_point(2, 0));

    // Triangle
    p1.holes[0].push_back(my_point(2, 1));
    p1.holes[0].push_back(my_point(2.4, 2));
    p1.holes[0].push_back(my_point(1.9, 2));
    p1.holes[0].push_back(my_point(2, 1));

    // Box
    p1.holes[1].push_back(my_point(3, 3));
    p1.holes[1].push_back(my_point(4, 3));
    p1.holes[1].push_back(my_point(4, 4));
    p1.holes[1].push_back(my_point(3, 4));
    p1.holes[1].push_back(my_point(3, 3));

    ex_out << "Representation of " << p1.name << ": "
        << boost::geometry::dsv(p1) << std::endl;
    ex_out << "Area of " << p1.name << ": "
        << boost::geometry::area(p1) << std::endl;
    ex_out << "Perimeter of " << p1.name << ": "
        << boost::geometry::perimeter(p1) << std::endl;
    ex_out << "Centroid of " << p1.name << ": "
        << boost::geometry::dsv(boost::geometry::return_centroid<my_point>(p1)) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c06_custom_polygon_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Representation of my polygon: (((2, 0), (1, 5), (7, 6), (2, 0)), ((2, 1), (2.4, 2), (1.9, 2), (2, 1)), ((3, 3), (4, 3), (4, 4), (3, 4), (3, 3)))
Area of my polygon: 14.25
Perimeter of my polygon: 25.5741
Centroid of my polygon: (3.34327, 3.71345)
)ex"));
}
}  // namespace ex_c06_custom_polygon_example

// libs/geometry/example/c08_custom_non_std_example.cpp
namespace ex_c08_custom_non_std_example {
struct my_point
{
    my_point(double a = 0, double b = 0)
        : x(a), y(b)
    {}
    double x,y;
};

// Sample polygon, having legacy methods
// (similar to e.g. COM objects)
class my_polygon
{
    std::vector<my_point> points;
    public :
        void add_point(my_point const& p) { points.push_back(p); }

        // Const access
        my_point const& get_point(std::size_t i) const
        {
            BOOST_ASSERT(i < points.size());
            return points[i];
        }

        // Mutable access
        my_point & get_point(std::size_t i)
        {
            BOOST_ASSERT(i < points.size());
            return points[i];
        }


        int point_count() const { return points.size(); }
        void erase_all() { points.clear(); }

        inline void set_size(int n) { points.resize(n); }
};

// ----------------------------------------------------------------------------
// Adaption: implement iterator and range-extension, and register with Boost.Geometry

// 1) implement iterator (const and non-const versions)
template<typename MyPolygon>
struct custom_iterator : public boost::iterator_facade
                            <
                                custom_iterator<MyPolygon>,
                                my_point,
                                boost::random_access_traversal_tag,
                                typename boost::mpl::if_
                                    <
                                        std::is_const<MyPolygon>,
                                        my_point const,
                                        my_point
                                    >::type&
                            >
{
    // Constructor for begin()
    explicit custom_iterator(MyPolygon& polygon)
        : m_polygon(&polygon)
        , m_index(0)
    {}

    // Constructor for end()
    explicit custom_iterator(bool, MyPolygon& polygon)
        : m_polygon(&polygon)
        , m_index(polygon.point_count())
    {}


    // Default constructor
    explicit custom_iterator()
        : m_polygon(NULL)
        , m_index(-1)
    {}

    typedef typename boost::mpl::if_
        <
            std::is_const<MyPolygon>,
            my_point const,
            my_point
        >::type my_point_type;

private:
    friend class boost::iterator_core_access;


    typedef boost::iterator_facade
        <
            custom_iterator<MyPolygon>,
            my_point,
            boost::random_access_traversal_tag,
            my_point_type&
        > facade;

    MyPolygon* m_polygon;
    int m_index;

    bool equal(custom_iterator const& other) const
    {
        return this->m_index == other.m_index;
    }
    typename facade::difference_type distance_to(custom_iterator const& other) const
    {
        return other.m_index - this->m_index;
    }

    void advance(typename facade::difference_type n)
    {
        m_index += n;
        if(m_polygon != NULL
            && (m_index >= m_polygon->point_count()
            || m_index < 0)
            )
        {
            m_index = m_polygon->point_count();
        }
    }

    void increment()
    {
        advance(1);
    }

    void decrement()
    {
        advance(-1);
    }

    // const and non-const dereference of this iterator
    my_point_type& dereference() const
    {
        return m_polygon->get_point(m_index);
    }
};




// 2) Implement Boost.Range const functionality
//    using method 2, "provide free-standing functions and specialize metafunctions"
// 2a) meta-functions
}  // namespace ex_c08_custom_non_std_example (reopened after the block)
 namespace boost
{
    template<> struct range_mutable_iterator<::ex_c08_custom_non_std_example::my_polygon>
    {
        typedef ::ex_c08_custom_non_std_example::custom_iterator<::ex_c08_custom_non_std_example::my_polygon> type;
    };

    template<> struct range_const_iterator<::ex_c08_custom_non_std_example::my_polygon>
    {
        typedef ::ex_c08_custom_non_std_example::custom_iterator<::ex_c08_custom_non_std_example::my_polygon const> type;
    };

    // RangeEx
    template<> struct range_size<::ex_c08_custom_non_std_example::my_polygon>
    {
        typedef std::size_t type;
    };

}
namespace ex_c08_custom_non_std_example { // namespace 'boost'


// 2b) free-standing function for Boost.Range ADP
inline custom_iterator<my_polygon> range_begin(my_polygon& polygon)
{
    return custom_iterator<my_polygon>(polygon);
}

inline custom_iterator<my_polygon const> range_begin(my_polygon const& polygon)
{
    return custom_iterator<my_polygon const>(polygon);
}

inline custom_iterator<my_polygon> range_end(my_polygon& polygon)
{
    return custom_iterator<my_polygon>(true, polygon);
}

inline custom_iterator<my_polygon const> range_end(my_polygon const& polygon)
{
    return custom_iterator<my_polygon const>(true, polygon);
}



// 3) optional, for writable geometries only, implement push_back/resize/clear
}  // namespace ex_c08_custom_non_std_example (reopened after the block)
 namespace boost { namespace geometry { namespace traits
{

template<> struct push_back<::ex_c08_custom_non_std_example::my_polygon>
{
    static inline void apply(::ex_c08_custom_non_std_example::my_polygon& polygon, ::ex_c08_custom_non_std_example::my_point const& point)
    {
        polygon.add_point(point);
    }
};

template<> struct resize<::ex_c08_custom_non_std_example::my_polygon>
{
    static inline void apply(::ex_c08_custom_non_std_example::my_polygon& polygon, std::size_t new_size)
    {
        polygon.set_size(new_size);
    }
};

template<> struct clear<::ex_c08_custom_non_std_example::my_polygon>
{
    static inline void apply(::ex_c08_custom_non_std_example::my_polygon& polygon)
    {
        polygon.erase_all();
    }
};

}}}
namespace ex_c08_custom_non_std_example {


// 4) register with Boost.Geometry
}  // namespace ex_c08_custom_non_std_example (reopened after the registration)
#ifndef EX_REGISTERED_9602cc9b8b63
#define EX_REGISTERED_9602cc9b8b63
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_c08_custom_non_std_example::my_point, double, cs::cartesian, x, y)
#endif
namespace ex_c08_custom_non_std_example {

}  // namespace ex_c08_custom_non_std_example (reopened after the registration)
#ifndef EX_REGISTERED_fedac471ecab
#define EX_REGISTERED_fedac471ecab
BOOST_GEOMETRY_REGISTER_RING(::ex_c08_custom_non_std_example::my_polygon)
#endif
namespace ex_c08_custom_non_std_example {


// end adaption
// ----------------------------------------------------------------------------


void walk_using_iterator(my_polygon const& polygon)
{
    for (custom_iterator<my_polygon const> it = custom_iterator<my_polygon const>(polygon);
        it != custom_iterator<my_polygon const>(true, polygon);
        ++it)
    {
        ex_out << boost::geometry::dsv(*it) << std::endl;
    }
    ex_out << std::endl;
}


void walk_using_range(my_polygon const& polygon)
{
    for (boost::range_iterator<my_polygon const>::type it
            = boost::begin(polygon);
        it != boost::end(polygon);
        ++it)
    {
        ex_out << boost::geometry::dsv(*it) << std::endl;
    }
    ex_out << std::endl;
}


int example_main()
{
    my_polygon container1;

    // Create (as an example) a regular polygon
    const int n = 5;
    const double d = (360 / n) * boost::geometry::math::d2r<double>();
    double a = 0;
    for (int i = 0; i < n + 1; i++, a += d)
    {
        container1.add_point(my_point(sin(a), cos(a)));
    }

    ex_out << "Walk using Boost.Iterator derivative" << std::endl;
    walk_using_iterator(container1);

    ex_out << "Walk using Boost.Range extension" << std::endl << std::endl;
    walk_using_range(container1);

    ex_out << "Use it by Boost.Geometry" << std::endl;
    ex_out << "Area: " << boost::geometry::area(container1) << std::endl;

    // Container 2 will be modified by Boost.Geometry. Add all points but the last one.
    my_polygon container2;
    for (int i = 0; i < n; i++)
    {
        // Use here the Boost.Geometry internal way of inserting (but the my_polygon way of getting)
        boost::geometry::traits::push_back<my_polygon>::apply(container2, container1.get_point(i));
    }

    ex_out << "Second container is not closed:" << std::endl;
    walk_using_range(container2);

    // Correct (= close it)
    boost::geometry::correct(container2);

    ex_out << "Now it is closed:" << std::endl;
    walk_using_range(container2);
    ex_out << "Area: " << boost::geometry::area(container2) << std::endl;

    // Use things from std:: using Boost.Range
    std::reverse(boost::begin(container2), boost::end(container2));
    ex_out << "Area reversed: " << boost::geometry::area(container2) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c08_custom_non_std_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Walk using Boost.Iterator derivative
(0, 1)
(0.951057, 0.309017)
(0.587785, -0.809017)
(-0.587785, -0.809017)
(-0.951057, 0.309017)
(-2.44929e-16, 1)

Walk using Boost.Range extension

(0, 1)
(0.951057, 0.309017)
(0.587785, -0.809017)
(-0.587785, -0.809017)
(-0.951057, 0.309017)
(-2.44929e-16, 1)

Use it by Boost.Geometry
Area: 2.37764
Second container is not closed:
(0, 1)
(0.951057, 0.309017)
(0.587785, -0.809017)
(-0.587785, -0.809017)
(-0.951057, 0.309017)

Now it is closed:
(0, 1)
(0.951057, 0.309017)
(0.587785, -0.809017)
(-0.587785, -0.809017)
(-0.951057, 0.309017)
(0, 1)

Area: 2.37764
Area reversed: -2.37764
)ex"));
}
}  // namespace ex_c08_custom_non_std_example

// libs/geometry/example/c10_custom_cs_example.cpp
namespace ex_c10_custom_cs_example {
template<typename DegreeOrRadian>
struct martian
{
    using units = DegreeOrRadian;
};

// 2: give it also a family
struct martian_tag;

// 3: register to which coordinate system family it belongs to
//    this must be done in namespace boost::geometry::traits
}  // namespace ex_c10_custom_cs_example (reopened after the block)
 namespace boost { namespace geometry { namespace traits
{

template <typename DegreeOrRadian>
struct cs_tag<::ex_c10_custom_cs_example::martian<DegreeOrRadian> >
{
    using type = ::ex_c10_custom_cs_example::martian_tag;
};

}}}
namespace ex_c10_custom_cs_example { // namespaces


// 5: not worked out. To implement a specific distance strategy for Mars,
//    e.g. with the Mars radius given by default,
//    you will have to implement (/register) several other metafunctions:
//      tag, return_type, similar_type, comparable_type,
//    and structs:
//      get_similar, get_comparable, result_from_distance
//   See e.g. .../boost/geometry/extensions/gis/geographic/strategies/andoyer.hpp

int example_main()
{
    using mars_point = boost::geometry::model::point
        <
            double, 2, martian<boost::geometry::degree>
        >;

    // Declare two points
    // (Source: http://nssdc.gsfc.nasa.gov/planetary/mars_mileage_guide.html)
    // (Other sources: Wiki and Google give slightly different coordinates, resulting
    //  in other distance, 20 km off)
    mars_point const viking1(-48.23, 22.54); // Viking 1 landing site in Chryse Planitia
    mars_point const pathfinder(-33.55, 19.33); // Pathfinder landing site in Ares Vallis

    // To calculate distance, declare and construct a strategy with Mars mean radius, in KM
    // (Source: http://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html)
    boost::geometry::strategies::distance::spherical<> const spherical(3389.5);

    double d = boost::geometry::distance(viking1, pathfinder, spherical);

    ex_out << "Distance between Viking1 and Pathfinder landing sites: "
        << d << " km" << std::endl;

    // We would get 832.616 here, same order as the 835 (rounded on 5 km) listed
    // on the mentioned site

    // The distance can be calculated more accurately by an Ellipsoidal approach,
    // giving 834.444 km
    boost::geometry::srs::spheroid<double> spheroid(3396.2, 3376.2);
    boost::geometry::strategy::distance::geographic<> const ellipsoidal(spheroid);

    d = boost::geometry::distance(viking1, pathfinder, ellipsoidal);
    ex_out << "Ellipsoidal distance: " << d << " km" << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c10_custom_cs_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Distance between Viking1 and Pathfinder landing sites: 832.616 km
Ellipsoidal distance: 834.444 km
)ex"));
}
}  // namespace ex_c10_custom_cs_example

// libs/geometry/example/c11_custom_cs_transform_example.cpp
namespace ex_c11_custom_cs_transform_example {
struct cart {};
struct cart_shifted5 {};

// 2: register to which coordinate system family they belong
}  // namespace ex_c11_custom_cs_transform_example (reopened after the block)
 namespace boost { namespace geometry { namespace traits
{

template<> struct cs_tag<::ex_c11_custom_cs_transform_example::cart> { typedef cartesian_tag type; };
template<> struct cs_tag<::ex_c11_custom_cs_transform_example::cart_shifted5> { typedef cartesian_tag type; };

}}}
namespace ex_c11_custom_cs_transform_example { // namespaces


// 3: sample implementation of a shift
//    to convert coordinate system "cart" to "cart_shirted5"
struct shift
{
    template <typename P1, typename P2>
    inline bool apply(P1 const& p1, P2& p2) const
    {
        namespace bg = boost::geometry;
        bg::set<0>(p2, bg::get<0>(p1) + 5);
        bg::set<1>(p2, bg::get<1>(p1));
        return true;
    }
};


// 4: register the default strategy to transform any cart point to any cart_shifted5 point
}  // namespace ex_c11_custom_cs_transform_example (reopened after the block)
 namespace boost { namespace geometry  { namespace strategy { namespace transform { namespace services
{

template <typename P1, typename P2>
struct default_strategy<cartesian_tag, cartesian_tag, ::ex_c11_custom_cs_transform_example::cart, ::ex_c11_custom_cs_transform_example::cart_shifted5, 2, 2, P1, P2>
{
    typedef ::ex_c11_custom_cs_transform_example::shift type;
};

}}}}}
namespace ex_c11_custom_cs_transform_example { // namespaces


// 5: implement a distance strategy between the two different ones
struct shift_and_calc_distance
{
    template <typename P1, typename P2>
    inline double apply(P1 const& p1, P2 const& p2) const
    {
        P2 p1_shifted;
        boost::geometry::transform(p1, p1_shifted);
        return boost::geometry::distance(p1_shifted, p2);
    }
};

// 6: Define point types using this explicitly
typedef boost::geometry::model::point<double, 2, cart> point1;
typedef boost::geometry::model::point<double, 2, cart_shifted5> point2;

// 7: register the distance strategy
}  // namespace ex_c11_custom_cs_transform_example (reopened after the block)
 namespace boost { namespace geometry { namespace strategy { namespace distance { namespace services
{
    template <>
    struct tag<::ex_c11_custom_cs_transform_example::shift_and_calc_distance>
    {
        typedef strategy_tag_distance_point_point type;
    };

    template <typename P1, typename P2>
    struct return_type<::ex_c11_custom_cs_transform_example::shift_and_calc_distance, P1, P2>
    {
        typedef double type;
    };

    template <>
    struct default_strategy<point_tag, point_tag, ::ex_c11_custom_cs_transform_example::point1, ::ex_c11_custom_cs_transform_example::point2, cartesian_tag, cartesian_tag>
    {
        typedef ::ex_c11_custom_cs_transform_example::shift_and_calc_distance type;
    };


}}}}}
namespace ex_c11_custom_cs_transform_example {



int example_main()
{
    point1 p1_a(0, 0), p1_b(5, 5);
    point2 p2_a(2, 2), p2_b(6, 6);

    // Distances run for points on the same coordinate system.
    // This is possible by default because they are cartesian coordinate systems.
    double d1 = boost::geometry::distance(p1_a, p1_b);
    double d2 = boost::geometry::distance(p2_a, p2_b);

    ex_out << d1 << " " << d2 << std::endl;

    // Transform from a to b:
    boost::geometry::model::point<double, 2, cart_shifted5> p1_shifted;
    boost::geometry::transform(p1_a, p1_shifted);


    // Of course this can be calculated now, same CS
    double d3 = boost::geometry::distance(p1_shifted, p2_a);


    // Calculate distance between them. Note that inside distance the
    // transformation is called.
    double d4 = boost::geometry::distance(p1_a, p2_a);

    // The result should be the same.
    ex_out << d3 << " " << d4 << std::endl;

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c11_custom_cs_transform_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(7.07107 5.65685
3.60555 2.82843
)ex"));
}
}  // namespace ex_c11_custom_cs_transform_example

// libs/geometry/example/ml01_multipolygon_simplify.cpp
namespace ex_ml01_multipolygon_simplify {
int example_main()
{
    typedef boost::geometry::model::d2::point_xy<double> point_xy;
    typedef boost::geometry::model::polygon<point_xy > polygon;
    typedef boost::geometry::model::ring<point_xy > ring;
    typedef boost::geometry::model::multi_polygon<polygon > multi_polygon;

    multi_polygon original_1;
    multi_polygon simplified_1;

    // Values between 0..1 and simplified with 1/2048
    boost::geometry::read_wkt("MULTIPOLYGON(((0.561648 1,1 1,1 0,0.468083 0,0.52758 0.00800554,0.599683 0.0280924,0.601611 0.265374,0.622693 0.316765,0.69507 0.357497,0.695623 0.429711,0.655111 0.502298,0.696467 0.543147,0.840712 0.593546,0.882583 0.66546,0.852357 0.748213,0.84264 0.789567,0.832667 0.841202,0.832667 0.841202,0.740538 0.873004,0.617349 0.905045,0.566576 0.977697,0.561648 1)),((0 0.801979,0.0308575 0.786234,0.0705513 0.631135,0.141616 0.527248,0.233985 0.505872,0.264777 0.526263,0.336631 0.505009,0.356603 0.422321,0.355803 0.350038,0.375252 0.205364,0.415206 0.0709182,0.45479 0,0 0,0 0,0 0.801979)))", original_1);

    ex_out << "Original: \n" << boost::geometry::num_points(original_1) << " points.\n\n";

    boost::geometry::simplify(original_1, simplified_1, 1.0 / 2048.0);

    ex_out << "Polygon with values 0..1 and simplified with 1.0 / 2048.0 \n"
        << "Result: \n" << boost::geometry::wkt(simplified_1) << "\n" << boost::geometry::num_points(simplified_1) << " points.\n\n";

    // Multiply every points from original_1 by 2047
    multi_polygon original_2(original_1);
    BOOST_FOREACH(polygon& p, original_2)
    {
        BOOST_FOREACH(point_xy& pt, p.outer())
        {
            pt.x(pt.x() * 2047.0);
            pt.y(pt.y() * 2047.0);
        }

        BOOST_FOREACH(ring& r, p.inners())
        {
            BOOST_FOREACH(point_xy& pt, r)
            {
                pt.x(pt.x() * 2047.0);
                pt.y(pt.y() * 2047.0);
            }
        }
    }

    multi_polygon simplified_2;
    boost::geometry::simplify(original_2, simplified_2, 1.0);
    ex_out << "Same values but multiplied by 2047.0 and simplified with 1.0\n"
        << "Result: \n" << boost::geometry::wkt(simplified_2) << "\n" << boost::geometry::num_points(simplified_2) << " points.\n";

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: ml01_multipolygon_simplify") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Original: 
37 points.

Polygon with values 0..1 and simplified with 1.0 / 2048.0 
Result: 
MULTIPOLYGON(((1 0,0.468083 0,0.52758 0.00800554,0.599683 0.0280924,0.601611 0.265374,0.622693 0.316765,0.69507 0.357497,0.695623 0.429711,0.655111 0.502298,0.696467 0.543147,0.840712 0.593546,0.882583 0.66546,0.852357 0.748213,0.84264 0.789567,0.832667 0.841202,0.740538 0.873004,0.617349 0.905045,0.566576 0.977697,0.561648 1,1 1,1 0)),((0.45479 0,0 0,0 0.801979,0.0308575 0.786234,0.0705513 0.631135,0.141616 0.527248,0.233985 0.505872,0.264777 0.526263,0.336631 0.505009,0.356603 0.422321,0.355803 0.350038,0.375252 0.205364,0.415206 0.0709182,0.45479 0)))
35 points.

Same values but multiplied by 2047.0 and simplified with 1.0
Result: 
MULTIPOLYGON(((2047 0,958.166 0,1079.96 16.3873,1227.55 57.5051,1231.5 543.221,1274.65 648.418,1422.81 731.796,1423.94 879.618,1341.01 1028.2,1425.67 1111.82,1720.94 1214.99,1806.65 1362.2,1744.77 1531.59,1724.88 1616.24,1704.47 1721.94,1515.88 1787.04,1263.71 1852.63,1159.78 2001.35,1149.69 2047,2047 2047,2047 0)),((930.955 0,0 0,0 1641.65,63.1653 1609.42,144.419 1291.93,289.888 1079.28,478.967 1035.52,541.999 1077.26,689.084 1033.75,729.966 864.491,728.329 716.528,768.141 420.38,849.927 145.17,930.955 0)))
35 points.
)ex"));
}
}  // namespace ex_ml01_multipolygon_simplify

// libs/geometry/example/ml02_distance_strategy.cpp
namespace ex_ml02_distance_strategy {
using namespace boost::geometry;

int example_main()
{
  typedef model::d2::point_xy<double> point_xy;

  point_xy p1(0.0, 0.0);
  point_xy p2(5.0, 0.0);

  // 1) This is direct call to Pythagoras algo
  using strategy1_type = strategy::distance::pythagoras<double>;
  strategy1_type strategy1;
  auto d1 = strategy1.apply(p1, p2);

  // 2) This is what is effectively called by simplify
  using strategy2_type = strategy::distance::comparable::pythagoras<double>;
  strategy2_type strategy2;
  auto d2 = strategy2.apply(p1, p2);

  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: ml02_distance_strategy") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_ml02_distance_strategy

// libs/geometry/example/ml03_distance_formula.cpp
namespace ex_ml03_distance_formula {
using namespace boost::geometry;

int example_main()
{
    double lon1_deg = 0.;
    double lat1_deg = 73.114273316483;
    double distance_m = 19992866.6147806;
    double azi12_deg = 78.154765899661;

    // Create an alias of the formula.
    typedef formula::karney_direct<double, true, true, true, true, 8> karney_direct;

    // Structure to hold the resulting values.
    formula::result_direct<double> result;

    // WGS-84 spheroid.
    srs::spheroid<double> spheroid(6378137.0, 6356752.3142451793);

    result = karney_direct::apply(lon1_deg, lat1_deg, distance_m, azi12_deg, spheroid);

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: ml03_distance_formula") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_ml03_distance_formula
