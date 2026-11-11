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

#include <boost/geometry/geometries/register/multi_linestring.hpp>
#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <boost/geometry/geometries/register/multi_polygon.hpp>
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

// libs/geometry/doc/src/examples/geometries/register/multi_linestring.cpp
namespace ex_geometries_register_multi_linestring {
using linestring_type = boost::geometry::model::linestring
    <
        boost::tuple<float, float>
    >;

}  // namespace ex_geometries_register_multi_linestring (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_geometries_register_multi_linestring {
}  // namespace ex_geometries_register_multi_linestring (reopened after the registration)
#ifndef EX_REGISTERED_e815ffeefe9a
#define EX_REGISTERED_e815ffeefe9a
BOOST_GEOMETRY_REGISTER_MULTI_LINESTRING(std::deque<::ex_geometries_register_multi_linestring::linestring_type>)
#endif
namespace ex_geometries_register_multi_linestring {

int example_main()
{
    // Normal usage of std::
    std::deque<linestring_type> lines(2);
    boost::geometry::read_wkt("LINESTRING(0 0,1 1)", lines[0]);
    boost::geometry::read_wkt("LINESTRING(2 2,3 3)", lines[1]);

    // Usage of Boost.Geometry
    ex_out << "LENGTH: "  << boost::geometry::length(lines) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[register_multi_linestring_output
/*`
Output:
[pre
LENGTH: 2.82843
]
*/
//]

TEST_CASE("example: geometries/register/multi_linestring") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(LENGTH: 2.82843
)ex"));
}
}  // namespace ex_geometries_register_multi_linestring

// libs/geometry/doc/src/examples/geometries/register/multi_polygon.cpp
namespace ex_geometries_register_multi_polygon {
using polygon_type = boost::geometry::model::polygon
    <
        boost::tuple<float, float>
    >;

}  // namespace ex_geometries_register_multi_polygon (reopened after the registration)
#ifndef EX_REGISTERED_5bf9c05224bf
#define EX_REGISTERED_5bf9c05224bf
BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS(cs::cartesian)
#endif
namespace ex_geometries_register_multi_polygon {
}  // namespace ex_geometries_register_multi_polygon (reopened after the registration)
#ifndef EX_REGISTERED_0a8ab4ee30d6
#define EX_REGISTERED_0a8ab4ee30d6
BOOST_GEOMETRY_REGISTER_MULTI_POLYGON(std::vector<::ex_geometries_register_multi_polygon::polygon_type>)
#endif
namespace ex_geometries_register_multi_polygon {

int example_main()
{
    // Normal usage of std::
    std::vector<polygon_type> polygons(2);
    boost::geometry::read_wkt("POLYGON((0 0,0 1,1 1,1 0,0 0))", polygons[0]);
    boost::geometry::read_wkt("POLYGON((3 0,3 1,4 1,4 0,3 0))", polygons[1]);

    // Usage of Boost.Geometry
    ex_out << "AREA: "  << boost::geometry::area(polygons) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[register_multi_polygon_output
/*`
Output:
[pre
AREA: 2
]
*/
//]

TEST_CASE("example: geometries/register/multi_polygon") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(AREA: 2
)ex"));
}
}  // namespace ex_geometries_register_multi_polygon
