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
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
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

// libs/geometry/doc/src/examples/geometries/register/ring_templated.cpp
namespace ex_geometries_register_ring_templated {
}  // namespace ex_geometries_register_ring_templated (reopened after the registration)
#ifndef EX_REGISTERED_0c7fb480a66b
#define EX_REGISTERED_0c7fb480a66b
BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(std::deque)
#endif
namespace ex_geometries_register_ring_templated {

int example_main()
{
    std::deque<boost::geometry::model::d2::point_xy<double>> ring(3);
    boost::geometry::assign_values(ring[0], 0, 0);
    boost::geometry::assign_values(ring[2], 4, 1);
    boost::geometry::assign_values(ring[1], 1, 4);

    // Boost.Geometry algorithms work on any deque now
    boost::geometry::correct(ring);
    ex_out << "Area: "  << boost::geometry::area(ring) << std::endl;
    ex_out << "Contents: "  << boost::geometry::wkt(ring) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]


//[register_ring_templated_output
/*`
Output:
[pre
Area: 7.5
Line: ((0, 0), (1, 4), (4, 1), (0, 0))
]
*/
//]

TEST_CASE("example: geometries/register/ring_templated") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Area: 7.5
Contents: POLYGON((0 0,1 4,4 1,0 0))
)ex"));
}
}  // namespace ex_geometries_register_ring_templated
