// mcpplibs.mwrap.boost.geometry — the documentation's strategy, I/O, view and arithmetic examples
// (libs/geometry/doc/src/examples/{strategies,io,views,arithmetic}/, quick_start.cpp) ported mechanically: each program runs as a case
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

#include <boost/config.hpp>
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

// libs/geometry/doc/src/examples/strategies/buffer_distance_asymmetric.cpp
namespace ex_strategies_buffer_distance_asymmetric {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using linestring = boost::geometry::model::linestring<point>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the asymmetric distance strategy
    boost::geometry::strategy::buffer::distance_asymmetric<double> distance_strategy(1.0, 0.5);

    // Declare other strategies
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::end_round end_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi linestring
    boost::geometry::model::multi_linestring<linestring> ml;
    boost::geometry::read_wkt("MULTILINESTRING((3 5,5 10,7 5),(7 7,11 10,15 7,19 10))", ml);

    // Create the buffered geometry with left/right a different distance
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(ml, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_distance_asymmetric.svg", ml, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_distance_asymmetric") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_distance_asymmetric

// libs/geometry/doc/src/examples/strategies/buffer_distance_symmetric.cpp
namespace ex_strategies_buffer_distance_symmetric {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using linestring = boost::geometry::model::linestring<point>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the symmetric distance strategy
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.5);

    // Declare other strategies
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::end_round end_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi linestring
    boost::geometry::model::multi_linestring<linestring> ml;
    boost::geometry::read_wkt("MULTILINESTRING((3 5,5 10,7 5),(7 7,11 10,15 7,19 10))", ml);

    // Create the buffered geometry with left/right the same distance
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(ml, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_distance_symmetric.svg", ml, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_distance_symmetric") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_distance_symmetric

// libs/geometry/doc/src/examples/strategies/buffer_end_flat.cpp
namespace ex_strategies_buffer_end_flat {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using linestring = boost::geometry::model::linestring<point>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the flat-end strategy
    boost::geometry::strategy::buffer::end_flat end_strategy;

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(1.0);
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi linestring
    boost::geometry::model::multi_linestring<linestring> ml;
    boost::geometry::read_wkt("MULTILINESTRING((3 5,5 10,7 5),(7 7,11 10,15 7,19 10))", ml);

    // Create the buffered geometry with flat ends
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(ml, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_end_flat.svg", ml, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_end_flat") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_end_flat

// libs/geometry/doc/src/examples/strategies/buffer_end_round.cpp
namespace ex_strategies_buffer_end_round {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using linestring = boost::geometry::model::linestring<point>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the round-end strategy with 36 points for a full circle
    boost::geometry::strategy::buffer::end_round end_strategy(36);

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(1.0);
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi linestring
    boost::geometry::model::multi_linestring<linestring> ml;
    boost::geometry::read_wkt("MULTILINESTRING((3 5,5 10,7 5),(7 7,11 10,15 7,19 10))", ml);

    // Create the buffered geometry with rounded ends
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(ml, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_end_round.svg", ml, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_end_round") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_end_round

// libs/geometry/doc/src/examples/strategies/buffer_geographic_point_circle.cpp
namespace ex_strategies_buffer_geographic_point_circle {
int example_main()
{
    namespace bg = boost::geometry;

    using point = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;

    // Declare the geographic_point_circle strategy (with 36 points)
    // Default template arguments (taking Andoyer strategy)
    bg::strategy::buffer::geographic_point_circle<> point_strategy(36);

    // Declare the distance strategy (one kilometer, around the point, on Earth)
    bg::strategy::buffer::distance_symmetric<double> distance_strategy(1000.0);

    // Declare other necessary strategies, unused for point
    bg::strategy::buffer::join_round join_strategy;
    bg::strategy::buffer::end_round end_strategy;
    bg::strategy::buffer::side_straight side_strategy;

    // Declare/fill a point on Earth, near Amsterdam
    point p;
    bg::read_wkt("POINT(4.9 52.1)", p);

    // Create the buffer of a point on the Earth
    bg::model::multi_polygon<bg::model::polygon<point>> result;
    bg::buffer(p, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);

    ex_out << "Area: " << bg::area(result) / (1000 * 1000) << " square kilometer" << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

//[buffer_geographic_point_circle_output
/*`
Output:
[pre
Area: 3.12542 square kilometer
]
*/
//]

TEST_CASE("example: strategies/buffer_geographic_point_circle") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 3.12567 square kilometer
)ex"));
}
}  // namespace ex_strategies_buffer_geographic_point_circle

// libs/geometry/doc/src/examples/strategies/buffer_join_miter.cpp
namespace ex_strategies_buffer_join_miter {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the join_miter strategy
    boost::geometry::strategy::buffer::join_miter join_strategy;

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.5);
    boost::geometry::strategy::buffer::end_flat end_strategy;
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi polygon
    boost::geometry::model::multi_polygon<polygon> mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((5 5,7 8,9 5,5 5)),((8 7,8 10,11 10,11 7,8 7)))", mp);

    // Create the buffered geometry with sharp corners
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(mp, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_join_miter.svg", mp, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_join_miter") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_join_miter

// libs/geometry/doc/src/examples/strategies/buffer_join_round.cpp
namespace ex_strategies_buffer_join_round {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the join_round strategy with 72 points for a full circle
    boost::geometry::strategy::buffer::join_round join_strategy(72);

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(1.0);
    boost::geometry::strategy::buffer::end_flat end_strategy;
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi polygon
    boost::geometry::model::multi_polygon<polygon> mp;
    boost::geometry::read_wkt("MULTIPOLYGON(((5 5,7 8,9 5,5 5)),((8 7,8 10,11 10,11 7,8 7)))", mp);

    // Create the buffered geometry with rounded corners
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(mp, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_join_round.svg", mp, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_join_round") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_join_round

// libs/geometry/doc/src/examples/strategies/buffer_point_circle.cpp
namespace ex_strategies_buffer_point_circle {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the point_circle strategy
    boost::geometry::strategy::buffer::point_circle point_strategy(360);

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.7);
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::end_round end_strategy;
    boost::geometry::strategy::buffer::side_straight side_strategy;

    // Declare/fill of a multi point
    boost::geometry::model::multi_point<point> mp;
    boost::geometry::read_wkt("MULTIPOINT((3 3),(3 4),(4 4),(7 3))", mp);

    // Create the buffer of a multi point
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(mp, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_point_circle.svg", mp, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_point_circle") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_point_circle

// libs/geometry/doc/src/examples/strategies/buffer_point_square.cpp
namespace ex_strategies_buffer_point_square {
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
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the point_square strategy
    boost::geometry::strategy::buffer::point_square point_strategy;

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.5);
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::end_round end_strategy;
    boost::geometry::strategy::buffer::side_straight side_strategy;

    // Declare/fill of a multi point
    boost::geometry::model::multi_point<point> mp;
    boost::geometry::read_wkt("MULTIPOINT((3 3),(3 4),(4 4),(7 3))", mp);

    // Create the buffer of a multi point
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(mp, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
     create_svg_buffer("buffer_point_square.svg", mp, result); 

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: strategies/buffer_point_square") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_strategies_buffer_point_square

// libs/geometry/doc/src/examples/io/from_wkt.cpp
namespace ex_io_from_wkt {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using box_type = boost::geometry::model::box<point_type>;
    using segment_type = boost::geometry::model::segment<point_type>;
    using polygon_type = boost::geometry::model::polygon<point_type>;
    using linestring_type = boost::geometry::model::linestring<point_type>;

    auto const a = boost::geometry::from_wkt<point_type>("POINT(1 2)");
    auto const d = boost::geometry::from_wkt<box_type>("BOX(0 0,3 3)");
    auto const e = boost::geometry::from_wkt<segment_type>("SEGMENT(1 0,3 4)");
    auto const c = boost::geometry::from_wkt<polygon_type>("POLYGON((0 0,0 7,4 2,2 0,0 0))");
    auto const b = boost::geometry::from_wkt<linestring_type>("LINESTRING(0 0,2 2,3 1)");

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: io/from_wkt") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_io_from_wkt

// libs/geometry/doc/src/examples/io/read_wkt.cpp
namespace ex_io_read_wkt {
int example_main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;

    point_type a;
    boost::geometry::model::linestring<point_type> b;
    boost::geometry::model::polygon<point_type> c;
    boost::geometry::model::box<point_type> d;
    boost::geometry::model::segment<point_type> e;

    boost::geometry::read_wkt("POINT(1 2)", a);
    boost::geometry::read_wkt("LINESTRING(0 0,2 2,3 1)", b);
    boost::geometry::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", c);
    boost::geometry::read_wkt("BOX(0 0,3 3)", d);
    boost::geometry::read_wkt("SEGMENT(1 0,3 4)", e);

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: io/read_wkt") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_io_read_wkt

// libs/geometry/doc/src/examples/io/svg.cpp
namespace ex_io_svg {
int example_main()
{
    // Specify the basic type
    using point_type = boost::geometry::model::d2::point_xy<double>;

    // Declare some geometries and set their values
    point_type a;
    boost::geometry::assign_values(a, 3, 6);

    boost::geometry::model::polygon<point_type> b;
    boost::geometry::read_wkt("POLYGON((0 0,0 7,4 2,2 0,0 0))", b);

    boost::geometry::model::linestring<point_type> c;
    c.push_back(point_type(3, 4));
    c.push_back(point_type(4, 5));

    // Declare a stream and an SVG mapper
    std::ofstream svg("my_map.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400);

    // Add geometries such that all these geometries fit exactly on the map
    mapper.add(a);
    mapper.add(b);
    mapper.add(c);

    // Draw the geometries on the SVG map, using a specific SVG style
    mapper.map(a, "fill-opacity:0.5;fill:rgb(153,204,0);stroke:rgb(153,204,0);stroke-width:2", 5);
    mapper.map(b, "fill-opacity:0.3;fill:rgb(51,51,153);stroke:rgb(51,51,153);stroke-width:2");
    mapper.map(c, "opacity:0.4;fill:none;stroke:rgb(212,0,0);stroke-width:5");

    // Destructor of map will be called - adding </svg>
    // Destructor of stream will be called, closing the file

    return 0;
    return 0;  // main's implicit return
}

//]


//[svg_mapper_output
/*`
Output:

[$img/io/svg_mapper.png]
*/
//]

TEST_CASE("example: io/svg") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_io_svg

// libs/geometry/doc/src/examples/io/svg_mapper_custom.cpp
namespace ex_io_svg_mapper_custom {
int example_main()
{
    // Specify the basic type
    using point_type = boost::geometry::model::d2::point_xy<double>;

    // Declare linestrings and set their values
    boost::geometry::model::linestring<point_type> a, b, c;
    a.push_back({1, 0});
    a.push_back({3, 3});

    b.push_back({5, 0});
    b.push_back({3, 2});

    c.push_back({4, 5});
    c.push_back({3, 4});

    // Declare a stream and an SVG mapper
    // The factor of 0.95 zooms out to give a convenient margin
    std::ofstream svg("my_map.svg");
    boost::geometry::svg_mapper<point_type> mapper(svg, 400, 400, 0.95);

    // Write a marker definition.
    svg << "<defs>";
    svg << "<marker id=\"arrowhead\" markerWidth=\"5\" markerHeight=\"3.5\""
           " refX=\"0\" refY=\"1.75\" orient=\"auto\">"
           " <polygon points=\"0 0, 5 1.75, 0 3.5\"/></marker>";
    svg << "</defs>";

    // Add geometries such that all these geometries fit exactly on the map
    mapper.add(a);
    mapper.add(b);
    mapper.add(c);

    // Group the first two geometries
    svg << "<g>";
    mapper.map(a, "opacity:0.5;stroke-width:1;stroke:gray;marker-end:url(#arrowhead)");
    mapper.map(b, "opacity:0.5;stroke-width:3;stroke:gray;marker-end:url(#arrowhead)");
    svg << "</g>";

    mapper.map(c, "opacity:0.5;stroke-width:5;stroke:red;marker-end:url(#arrowhead)");

    return 0;
    return 0;  // main's implicit return
}

//]


//[svg_mapper_custom_output
/*`
Output:

[$img/io/svg_mapper_custom.png]
*/
//]

TEST_CASE("example: io/svg_mapper_custom") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_io_svg_mapper_custom

// libs/geometry/doc/src/examples/io/to_wkt.cpp
namespace ex_io_to_wkt {
int example_main()
{
    namespace geom = boost::geometry;
    using point_type = geom::model::d2::point_xy<double>;

    point_type point = geom::make<point_type>(3, 2);
    geom::model::polygon<point_type> polygon;
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(0, 0));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(0, 4));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(4, 4));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(4, 0));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(0, 0));

    ex_out << boost::geometry::to_wkt(point) << std::endl;
    ex_out << boost::geometry::to_wkt(polygon) << std::endl;

    point_type point_frac = geom::make<point_type>(3.141592654, 27.18281828);
    geom::model::polygon<point_type> polygon_frac;
    geom::append(geom::exterior_ring(polygon_frac), geom::make<point_type>(0.00000, 0.00000));
    geom::append(geom::exterior_ring(polygon_frac), geom::make<point_type>(0.00000, 4.00001));
    geom::append(geom::exterior_ring(polygon_frac), geom::make<point_type>(4.00001, 4.00001));
    geom::append(geom::exterior_ring(polygon_frac), geom::make<point_type>(4.00001, 0.00000));
    geom::append(geom::exterior_ring(polygon_frac), geom::make<point_type>(0.00000, 0.00000));

    ex_out << boost::geometry::to_wkt(point_frac, 3) << std::endl;
    ex_out << boost::geometry::to_wkt(polygon_frac, 3) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[to_wkt_output
/*`
Output:
[pre
POINT(3 2)
POLYGON((0 0,0 4,4 4,4 0,0 0))
POINT(3.14 27.2)
POLYGON((0 0,0 4,4 4,4 0,0 0))
]


*/
//]

TEST_CASE("example: io/to_wkt") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(POINT(3 2)
POLYGON((0 0,0 4,4 4,4 0,0 0))
POINT(3.14 27.2)
POLYGON((0 0,0 4,4 4,4 0,0 0))
)ex"));
}
}  // namespace ex_io_to_wkt

// libs/geometry/doc/src/examples/io/wkt.cpp
namespace ex_io_wkt {
int example_main()
{
    namespace geom = boost::geometry;
    using point_type = geom::model::d2::point_xy<double>;

    point_type point = geom::make<point_type>(3, 6);
    geom::model::polygon<point_type> polygon;
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(0, 0));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(0, 4));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(4, 4));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(4, 0));
    geom::append(geom::exterior_ring(polygon), geom::make<point_type>(0, 0));

    ex_out << boost::geometry::wkt(point) << std::endl;
    ex_out << boost::geometry::wkt(polygon) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[wkt_output
/*`
Output:
[pre
POINT(3 2)
POLYGON((0 0,0 4,4 4,4 0,0 0))
POINT(3.14 27.2)
POLYGON((0 0,0 4,4 4,4 0,0 0))
]


*/
//]

TEST_CASE("example: io/wkt") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(POINT(3 6)
POLYGON((0 0,0 4,4 4,4 0,0 0))
)ex"));
}
}  // namespace ex_io_wkt

// libs/geometry/doc/src/examples/views/box_view.cpp
namespace ex_views_box_view {
int example_main()
{
    using box_type = boost::geometry::model::box
        <
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        >;

    // Define the Boost.Range compatible type:
    using box_view = boost::geometry::box_view<box_type>;

    box_type box;
    boost::geometry::assign_values(box, 0, 0, 4, 4);

    box_view view(box);

    // Iterating in clockwise direction over the points of this box
    for (boost::range_iterator<box_view const>::type it = boost::begin(view);
        it != boost::end(view); ++it)
    {
        ex_out << " " << boost::geometry::dsv(*it);
    }
    ex_out << std::endl;

    // Note that a box_view is tagged as a ring, so supports area etc.
    ex_out << "Area: " << boost::geometry::area(view) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[box_view_output
/*`
Output:
[pre
 (0, 0) (0, 4) (4, 4) (4, 0) (0, 0)
Area: 16
]
*/
//]

TEST_CASE("example: views/box_view") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( (0, 0) (0, 4) (4, 4) (4, 0) (0, 0)
Area: 16
)ex"));
}
}  // namespace ex_views_box_view

// libs/geometry/doc/src/examples/views/segment_view.cpp
namespace ex_views_segment_view {
int example_main()
{
    using segment_type = boost::geometry::model::segment
        <
            boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>
        >;

    using segment_view = boost::geometry::segment_view<segment_type>;

    segment_type segment;
    boost::geometry::assign_values(segment, 0, 0, 1, 1);

    segment_view view(segment);

    // Iterating over the points of this segment
    for (boost::range_iterator<segment_view const>::type it = boost::begin(view);
        it != boost::end(view); ++it)
    {
        ex_out << " " << boost::geometry::dsv(*it);
    }
    ex_out << std::endl;

    // Note that a segment_view is tagged as a linestring, so supports length etc.
    ex_out << "Length: " << boost::geometry::length(view) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[segment_view_output
/*`
Output:
[pre
 (0, 0) (1, 1)
Length: 1.41421
]
*/
//]

TEST_CASE("example: views/segment_view") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( (0, 0) (1, 1)
Length: 1.41421
)ex"));
}
}  // namespace ex_views_segment_view

// libs/geometry/doc/src/examples/arithmetic/cross_product.cpp
namespace ex_arithmetic_cross_product {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

int example_main()
{
     //Example 1 2D Vector
     
     bg::model::point<double, 2, bg::cs::cartesian> p1(7.0, 2.0);
     bg::model::point<double, 2, bg::cs::cartesian> p2(4.0, 5.0);
           
     bg::model::point<double, 2, bg::cs::cartesian> r1;
       
     r1 = bg::cross_product(p1,p2);

     ex_out << "Cross Product 1: "<< r1.get<0>() << std::endl;  //Note that the second point (r1.get<1>) would be undefined in this case
        
     //Example 2 - 3D Vector 
     
     bg::model::point<double, 3, bg::cs::cartesian> p3(4.0, 6.0, 5.0);
     bg::model::point<double, 3, bg::cs::cartesian> p4(7.0, 2.0, 3.0);
           
     bg::model::point<double, 3, bg::cs::cartesian> r2;
       
     r2 = bg::cross_product(p3,p4);

     ex_out << "Cross Product 2: ("<< r2.get<0>() <<","<< r2.get<1>() <<","<< r2.get<2>() << ")"<< std::endl; 
     
     return 0;
    return 0;  // main's implicit return
}

//]

//[cross_product_output
/*`
Output:
[pre
Cross Product 1: 27
Cross Product 2: (8,23,-34)
]
*/
//]

TEST_CASE("example: arithmetic/cross_product") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Cross Product 1: 27
Cross Product 2: (8,23,-34)
)ex"));
}
}  // namespace ex_arithmetic_cross_product

// libs/geometry/doc/src/examples/arithmetic/dot_product.cpp
namespace ex_arithmetic_dot_product {
namespace bg = boost::geometry; /*< Convenient namespace alias >*/

}  // namespace ex_arithmetic_dot_product (reopened after the registration)
#ifndef EX_REGISTERED_5e756957de5c
#define EX_REGISTERED_5e756957de5c
BOOST_GEOMETRY_REGISTER_BOOST_ARRAY_CS(cs::cartesian)
#endif
namespace ex_arithmetic_dot_product {


int example_main()
{            
     double dp1,dp2,dp3,dp4;
     bg::model::point<double, 3, bg::cs::cartesian> point1(1.0, 2.0, 3.0);
     bg::model::point<double, 3, bg::cs::cartesian> point2(4.0, 5.0, 6.0);

     //Example 1
     dp1 = bg::dot_product(point1, point2);

     ex_out << "Dot Product 1: "<< dp1 << std::endl; 

     bg::model::point<double, 2, bg::cs::cartesian> point3(3.0, 2.0);
     bg::model::point<double, 2, bg::cs::cartesian> point4(4.0, 7.0);

     //Example 2	
     dp2 = bg::dot_product(point3, point4);

     ex_out << "Dot Product 2: "<< dp2 << std::endl; 

     boost::array<double, 2> a =  {1, 2};
     boost::array<double, 2> b =  {2, 3};

     //Example 3
     dp3 = bg::dot_product(a, b);

     ex_out << "Dot Product 3: "<< dp3 << std::endl; 

     return 0;

    return 0;  // main's implicit return
}

//]

//[dot_product_output
/*`
Output:
[pre
Dot Product 1: 32
Dot Product 2: 26
Dot Product 3: 8
]
*/
//]

TEST_CASE("example: arithmetic/dot_product") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Dot Product 1: 32
Dot Product 2: 26
Dot Product 3: 8
)ex"));
}
}  // namespace ex_arithmetic_dot_product

// libs/geometry/doc/src/examples/quick_start.cpp
namespace ex_quick_start {
#if defined(_MSC_VER)
// We deliberately mix float/double's here so turn off warning
//#pragma warning( disable : 4244 )
#endif // defined(_MSC_VER)


//[quickstart_include


using namespace boost::geometry;
//]



//[quickstart_register_c_array

}  // namespace ex_quick_start (reopened after the registration)
#ifndef EX_REGISTERED_273c1c0b66a3
#define EX_REGISTERED_273c1c0b66a3
BOOST_GEOMETRY_REGISTER_C_ARRAY_CS(cs::cartesian)
#endif
namespace ex_quick_start {
//]

//[quickstart_register_boost_tuple

}  // namespace ex_quick_start (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_quick_start {
//]

// Small QRect simulations following http://doc.trolltech.com/4.4/qrect.html
// Todo: once work the traits out further, would be nice if there is a real example of this.
// However for the example it makes no difference, it will work any way.
struct QPoint
{
    int x, y;
    // In Qt these are methods but for example below it makes no difference
};

struct QRect
{
    int x, y, width, height;
    QRect(int _x, int _y, int w, int h)
        : x(_x), y(_y), width(w), height(h)
    {}
    // In Qt these are methods but that will work as well, requires changing traits below
};


// Would be get/set with x(),y(),setX(),setY()
}  // namespace ex_quick_start (reopened after the registration)
#ifndef EX_REGISTERED_1f178a47de3e
#define EX_REGISTERED_1f178a47de3e
BOOST_GEOMETRY_REGISTER_POINT_2D(::ex_quick_start::QPoint, int, cs::cartesian, x, y)
#endif
namespace ex_quick_start {


// Register the QT rectangle. The macro(s) does not offer (yet) enough flexibility to do this in one line,
// but the traits classes do their job perfectly.
}  // namespace ex_quick_start (reopened after the block)
 namespace boost { namespace geometry { namespace traits
{

template <> struct tag<::ex_quick_start::QRect> { using type = box_tag; };
template <> struct point_type<::ex_quick_start::QRect> { using type = ::ex_quick_start::QPoint; };

template <size_t C, size_t D>
struct indexed_access<::ex_quick_start::QRect, C, D>
{
    static inline int get(const ::ex_quick_start::QRect& qr)
    {
        // Would be: x(), y(), width(), height()
        return C == min_corner && D == 0 ? qr.x
                : C == min_corner && D == 1 ? qr.y
                : C == max_corner && D == 0 ? qr.x + qr.width
                : C == max_corner && D == 1 ? qr.y + qr.height
                : 0;
    }

    static inline void set(::ex_quick_start::QRect& qr, const int& value)
    {
        // Would be: setX, setY, setWidth, setHeight
        if (C == min_corner && D == 0) qr.x = value;
        else if (C == min_corner && D == 1) qr.y = value;
        else if (C == max_corner && D == 0) qr.width = value - qr.x;
        else if (C == max_corner && D == 1) qr.height = value - qr.y;
    }
};


}}}
namespace ex_quick_start {


int example_main(void)
{
    //[quickstart_distance
    model::d2::point_xy<int> p1(1, 1), p2(2, 2);
    ex_out << "Distance p1-p2 is: " << distance(p1, p2) << std::endl;
    //]

    //[quickstart_distance_c_array
    int a[2] = {1,1};
    int b[2] = {2,3};
    double d = distance(a, b);
    ex_out << "Distance a-b is: " << d << std::endl;
    //]

    //[quickstart_point_in_polygon
    double points[][2] = {{2.0, 1.3}, {4.1, 3.0}, {5.3, 2.6}, {2.9, 0.7}, {2.0, 1.3}};
    model::polygon<model::d2::point_xy<double>> poly;
    append(poly, points);
    boost::tuple<double, double> p = boost::make_tuple(3.7, 2.0);
    ex_out << "Point p is in polygon? " << std::boolalpha << within(p, poly) << std::endl;
    //]

    //[quickstart_area
    ex_out << "Area: " << area(poly) << std::endl;
    //]

    //[quickstart_distance_mixed
    double d2 = distance(a, p);
    ex_out << "Distance a-p is: " << d2 << std::endl;
    //]

    //[quick_start_spherical
    using spherical_point = boost::geometry::model::point
        <
            double, 2, boost::geometry::cs::spherical_equatorial<boost::geometry::degree>
        >;

    spherical_point amsterdam(4.90, 52.37);
    spherical_point paris(2.35, 48.86);

    double const earth_radius = 3959; // miles
    ex_out << "Distance in miles: " << distance(amsterdam, paris) * earth_radius << std::endl;
    //]

    /***
    Now extension
    point_ll_deg  amsterdam, paris;
    parse(amsterdam, "52 22 23 N", "4 53 32 E");
    parse(paris, "48 52 0 N", "2 19 59 E");
    ex_out << "Distance A'dam-Paris: " << distance(amsterdam, paris) / 1000.0 << " kilometers " << std::endl;
    ***/

    //[quickstart_qt
    QRect r1(100, 200, 15, 15);
    QRect r2(110, 210, 20, 20);
    if (overlaps(r1, r2))
    {
        assign_values(r2, 200, 300, 220, 320);
    }
    //]

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: quick_start") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Distance p1-p2 is: 1.41421
Distance a-b is: 2.23607
Point p is in polygon? true
Area: 3.015
Distance a-p is: 2.87924
Distance in miles: 267.02
)ex"));
}
}  // namespace ex_quick_start
