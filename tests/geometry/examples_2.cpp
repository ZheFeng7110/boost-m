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
#include <boost/foreach.hpp>
#include <boost/geometry/geometries/register/ring.hpp>
#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
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

// libs/geometry/example/c07_custom_ring_pointer_example.cpp
namespace ex_c07_custom_ring_pointer_example {
}  // namespace ex_c07_custom_ring_pointer_example (reopened after the registration)
#ifndef EX_REGISTERED_7f7457e36ae3
#define EX_REGISTERED_7f7457e36ae3
BOOST_GEOMETRY_REGISTER_RING_TEMPLATED(std::vector)
#endif
namespace ex_c07_custom_ring_pointer_example {


// Sample point, having x/y
struct my_point
{
    my_point(double a = 0, double b = 0)
        : x(a), y(b)
    {}
    double x,y;
};


}  // namespace ex_c07_custom_ring_pointer_example (reopened after the block)
 namespace boost { namespace geometry { namespace traits {

template<> struct tag<::ex_c07_custom_ring_pointer_example::my_point>
{ typedef point_tag type; };

template<> struct coordinate_type<::ex_c07_custom_ring_pointer_example::my_point>
{ typedef double type; };

template<> struct coordinate_system<::ex_c07_custom_ring_pointer_example::my_point>
{ typedef cs::cartesian type; };

template<> struct dimension<::ex_c07_custom_ring_pointer_example::my_point> : boost::mpl::int_<2> {};

template<>
struct access<::ex_c07_custom_ring_pointer_example::my_point, 0>
{
    static double get(::ex_c07_custom_ring_pointer_example::my_point const& p)
    {
        return p.x;
    }

    static void set(::ex_c07_custom_ring_pointer_example::my_point& p, double const& value)
    {
        p.x = value;
    }
};

template<>
struct access<::ex_c07_custom_ring_pointer_example::my_point, 1>
{
    static double get(::ex_c07_custom_ring_pointer_example::my_point const& p)
    {
        return p.y;
    }

    static void set(::ex_c07_custom_ring_pointer_example::my_point& p, double const& value)
    {
        p.y = value;
    }
};

}}}
namespace ex_c07_custom_ring_pointer_example { // namespace boost::geometry::traits



int example_main()
{
    typedef std::vector<my_point*> ring_type;

    ring_type a, b;

    a.push_back(new my_point(0, 1));
    a.push_back(new my_point(2, 5));
    a.push_back(new my_point(5, 3));
    a.push_back(new my_point(0, 1));

    b.push_back(new my_point(3, 0));
    b.push_back(new my_point(0, 3));
    b.push_back(new my_point(4, 5));
    b.push_back(new my_point(3, 0));

    double aa = boost::geometry::area(a);
    double ab = boost::geometry::area(b);

    ex_out << "a: " << aa << std::endl;
    ex_out << "b: " << ab << std::endl;

    // This will NOT work because would need dynamicly allocating memory for point* in algorithms:
    //std::vector<ring_type> unioned;
    //boost::geometry::union<ring_type>(a, b, unioned);

    // BEGIN TODO
    // This compiles (and once worked) using pointers, but has to be fixed or deprecated
    // The problem is now the cart_intersect/side where a temporary point is generated
    //typedef boost::geometry::model::ring<boost::geometry::model::d2::point_xy<double> > ring_2d;
    //std::vector<ring_2d> unioned;
    //std::vector<ring_2d> intersected;

    //boost::geometry::intersection(a, b, intersected);
    //boost::geometry::union_(a, b, unioned);

    //double ai = 0, au = 0;
    //BOOST_FOREACH(ring_2d const& ring, intersected)
    //{
    //    ai += boost::geometry::area(ring);
    //}
    //BOOST_FOREACH(ring_2d const& ring, unioned)
    //{
    //    au += boost::geometry::area(ring);
    //}

    //ex_out << "a: " << aa << std::endl;
    //ex_out << "b: " << ab << std::endl;
    //ex_out << "a & b: " << ai << std::endl;
    //ex_out << "a | b: " << au << std::endl;
    //ex_out << "a + b - (a & b): " << (aa + ab - ai) << std::endl;
    // END TODO


    // free
    BOOST_FOREACH(my_point* p, a)
    {
        delete p;
    }

    BOOST_FOREACH(my_point* p, b)
    {
        delete p;
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: c07_custom_ring_pointer_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(a: 8
b: 9
)ex"));
}
}  // namespace ex_c07_custom_ring_pointer_example
