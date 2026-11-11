// mcpplibs.mwrap.boost.geometry — the documentation's rtree examples
// (libs/geometry/doc/index/src/examples/rtree/, libs/geometry/index/example/) ported mechanically: each program runs as a case
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
#include <cstdlib>
#include <boost/assert.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <cmath>
import std;
import mcpplibs.mwrap.boost.geometry;
import mcpplibs.mwrap.boost.algorithm;
import mcpplibs.mwrap.boost.core;
import mcpplibs.mwrap.boost.interprocess;
import mcpplibs.mwrap.boost.timer;
import mcpplibs.mwrap.boost.range;
import mcpplibs.mwrap.boost.container;

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

// libs/geometry/doc/index/src/examples/rtree/interprocess.cpp
namespace ex_rtree_interprocess {
int example_main(int argc, char *argv[])
{
    using namespace boost::interprocess;
    namespace bg = boost::geometry;
    namespace bgm = bg::model;
    namespace bgi = bg::index;

    typedef bgm::point<float, 2, bg::cs::cartesian> P;
    typedef bgm::box<P> B;

    typedef bgi::linear<32, 8> Par;
    typedef bgi::indexable<B> I;
    typedef bgi::equal_to<B> E;
    typedef allocator<B, managed_shared_memory::segment_manager> Alloc;
    typedef bgi::rtree<B, Par, I, E, Alloc> Rtree;

    //Parent process
    if ( argc == 1 )
    {
        struct shm_remove
        {
            shm_remove() { shared_memory_object::remove("MySharedMemory"); }
            ~shm_remove(){ shared_memory_object::remove("MySharedMemory"); }
        } remover;

        managed_shared_memory segment(create_only, "MySharedMemory", 65536);

        ex_out << "Parent: Constructing container\n";

        Rtree * tree = segment.construct<Rtree>("Rtree")(Par(), I(), E(), Alloc(segment.get_segment_manager()));

        ex_out << "Parent: Filling container with 100 boxes\n";

        for ( float i = 0 ; i < 100 ; i += 1 )
            tree->insert(B(P(i, i), P(i+0.5f, i+0.5f)));

        ex_out << "Parent: Tree content\n";
        Rtree::bounds_type bb = tree->bounds();
        ex_out << "[(" << bg::get<0>(bb.min_corner()) << ", " << bg::get<1>(bb.min_corner())
                  << ")(" << bg::get<0>(bb.max_corner()) << ", " << bg::get<1>(bb.max_corner()) << ")]\n";

        ex_out << "Parent: Running child process\n";

        std::string s(argv[0]); s += " child ";
        if ( 0 != std::system(s.c_str()) )
            return 1;

        if ( segment.find<Rtree>("Rtree").first )
            return 1;

        ex_out << "Parent: Container was properly destroyed by the child process\n";
   }
   //Child process
   else
   {
      managed_shared_memory segment(open_only, "MySharedMemory");

      ex_out << "Child: Searching of the container in shared memory\n";

      Rtree * tree = segment.find<Rtree>("Rtree").first;

      ex_out << "Child: Querying for objects intersecting box = [(45, 45)(55, 55)]\n";

      std::vector<B> result;
      unsigned k = tree->query(bgi::intersects(B(P(45, 45), P(55, 55))), std::back_inserter(result));

      ex_out << "Child: Found objects:\n";
      ex_out << k << "\n";
      BOOST_FOREACH(B const& b, result)
      {
        ex_out << "[(" << bg::get<0>(b.min_corner()) << ", " << bg::get<1>(b.min_corner())
                  << ")(" << bg::get<0>(b.max_corner()) << ", " << bg::get<1>(b.max_corner()) << ")]\n";
      }
      ex_out << "\n";

      ex_out << "Child: Destroying container\n";

      segment.destroy<Rtree>("Rtree");
   }

   return 0;
    return 0;  // main's implicit return
};

//]

TEST_CASE("example: rtree/interprocess") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 1);
    CHECK(ex_out.str() == std::string_view(R"ex(Parent: Constructing container
Parent: Filling container with 100 boxes
Parent: Tree content
[(0, 0)(99.5, 99.5)]
Parent: Running child process
)ex"));
}
}  // namespace ex_rtree_interprocess

// libs/geometry/doc/index/src/examples/rtree/iterative_query.cpp
namespace ex_rtree_iterative_query {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int example_main()
{
    typedef bg::model::point<double, 2, bg::cs::cartesian> point;
    typedef point value;
    typedef bgi::rtree< value, bgi::linear<16> > rtree_t;

    // create the rtree using default constructor
    rtree_t rtree;

    // create some values
    for ( double f = 0 ; f < 10 ; f += 1 )
    {
        // insert new value
        rtree.insert(point(f, f));
    }

    // query point
    point pt(5.1, 5.1);

    // iterate over nearest Values
    for ( rtree_t::const_query_iterator
            it = rtree.qbegin(bgi::nearest(pt, 100)) ;
            it != rtree.qend() ;
            ++it )
    {
        double d = bg::distance(pt, *it);

        ex_out << bg::wkt(*it) << ", distance= " << d << std::endl;

        // break if the distance is too big
        if ( d > 2 )
        {
            ex_out << "break!" << std::endl;
            break;
        }
    }

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/iterative_query") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(POINT(5 5), distance= 0.141421
POINT(6 6), distance= 1.27279
POINT(4 4), distance= 1.55563
POINT(7 7), distance= 2.68701
break!
)ex"));
}
}  // namespace ex_rtree_iterative_query

// libs/geometry/doc/index/src/examples/rtree/mapped_file.cpp
namespace ex_rtree_mapped_file {
namespace bi = boost::interprocess;
namespace bg = boost::geometry;
namespace bgm = bg::model;
namespace bgi = bg::index;

int example_main()
{
    typedef bgm::point<float, 2, bg::cs::cartesian> point_t;

    typedef point_t value_t;
    typedef bgi::linear<32, 8> params_t;
    typedef bgi::indexable<value_t> indexable_t;
    typedef bgi::equal_to<value_t> equal_to_t;
    typedef bi::allocator<value_t, bi::managed_mapped_file::segment_manager> allocator_t;
    typedef bgi::rtree<value_t, params_t, indexable_t, equal_to_t, allocator_t> rtree_t;

    {
        bi::managed_mapped_file file(bi::open_or_create, "data.bin", 1024*1024);
        allocator_t alloc(file.get_segment_manager());
        rtree_t * rtree_ptr = file.find_or_construct<rtree_t>("rtree")(params_t(), indexable_t(), equal_to_t(), alloc);

        ex_out << rtree_ptr->size() << std::endl;

        rtree_ptr->insert(point_t(1.0, 1.0));
        rtree_ptr->insert(point_t(2.0, 2.0));

        ex_out << rtree_ptr->size() << std::endl;
    }

    {
        bi::managed_mapped_file file(bi::open_or_create, "data.bin", 1024*1024);
        allocator_t alloc(file.get_segment_manager());
        rtree_t * rtree_ptr = file.find_or_construct<rtree_t>("rtree")(params_t(), indexable_t(), equal_to_t(), alloc);

        ex_out << rtree_ptr->size() << std::endl;

        rtree_ptr->insert(point_t(3.0, 3.0));
        rtree_ptr->insert(point_t(4.0, 4.0));

        ex_out << rtree_ptr->size() << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/mapped_file") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(0
2
2
4
)ex"));
#endif
}
}  // namespace ex_rtree_mapped_file

// libs/geometry/doc/index/src/examples/rtree/polygons_shared_ptr.cpp
namespace ex_rtree_polygons_shared_ptr {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int example_main()
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef bg::model::polygon<point, false, false> polygon; // ccw, open polygon
    typedef boost::shared_ptr<polygon> shp;
    typedef std::pair<box, shp> value;

    // create the rtree using default constructor
    bgi::rtree< value, bgi::linear<16, 4> > rtree;

    ex_out << "filling index with polygons shared pointers:" << std::endl;

    // create some polygons and fill the spatial index
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // create a polygon
        shp p(new polygon());
        for ( float a = 0 ; a < 6.28316f ; a += 1.04720f )
        {
            float x = i + int(10*::cos(a))*0.1f;
            float y = i + int(10*::sin(a))*0.1f;
            p->outer().push_back(point(x, y));
        }

        // display new polygon
        ex_out << bg::wkt<polygon>(*p) << std::endl;

        // calculate polygon bounding box
        box b = bg::return_envelope<box>(*p);
        // insert new value
        rtree.insert(std::make_pair(b, p));
    }

    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

    // note: in Boost.Geometry the WKT representation of a box is polygon

    // note: the values store the bounding boxes of polygons
    // the polygons aren't used for querying but are printed

    // display results
    ex_out << "spatial query box:" << std::endl;
    ex_out << bg::wkt<box>(query_box) << std::endl;
    ex_out << "spatial query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_s)
        ex_out << bg::wkt<polygon>(*v.second) << std::endl;

    ex_out << "knn query point:" << std::endl;
    ex_out << bg::wkt<point>(point(0, 0)) << std::endl;
    ex_out << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        ex_out << bg::wkt<polygon>(*v.second) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/polygons_shared_ptr") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(filling index with polygons shared pointers:
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8,1 0))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2,2 1))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2,4 3))
POLYGON((5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2,5 4))
POLYGON((6 5,5.4 5.8,4.5 5.8,4 5,4.6 4.2,5.5 4.2,6 5))
POLYGON((7 6,6.4 6.8,5.5 6.8,5 6,5.6 5.2,6.5 5.2,7 6))
POLYGON((8 7,7.4 7.8,6.5 7.8,6 7,6.6 6.2,7.5 6.2,8 7))
POLYGON((9 8,8.4 8.8,7.5 8.8,7 8,7.6 7.2,8.5 7.2,9 8))
POLYGON((10 9,9.4 9.8,8.5 9.8,8 9,8.6 8.2,9.5 8.2,10 9))
spatial query box:
POLYGON((0 0,0 5,5 5,5 0,0 0))
spatial query result:
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8,1 0))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2,2 1))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2,4 3))
POLYGON((5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2,5 4))
POLYGON((6 5,5.4 5.8,4.5 5.8,4 5,4.6 4.2,5.5 4.2,6 5))
knn query point:
POINT(0 0)
knn query result:
POLYGON((5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2,5 4))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2,4 3))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8,1 0))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2,2 1))
)ex"));
#endif
}
}  // namespace ex_rtree_polygons_shared_ptr

// libs/geometry/doc/index/src/examples/rtree/polygons_vector.cpp
namespace ex_rtree_polygons_vector {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

int example_main()
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef bg::model::polygon<point, false, false> polygon; // ccw, open polygon
    typedef std::pair<box, unsigned> value;

    // polygons
    std::vector<polygon> polygons;

    // create some polygons
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // create a polygon
        polygon p;
        for ( float a = 0 ; a < 6.28316f ; a += 1.04720f )
        {
            float x = i + int(10*::cos(a))*0.1f;
            float y = i + int(10*::sin(a))*0.1f;
            p.outer().push_back(point(x, y));
        }

        // add polygon
        polygons.push_back(p);
    }

    // display polygons
    ex_out << "generated polygons:" << std::endl;
    BOOST_FOREACH(polygon const& p, polygons)
        ex_out << bg::wkt<polygon>(p) << std::endl;

    // create the rtree using default constructor
    bgi::rtree< value, bgi::rstar<16, 4> > rtree;

    // fill the spatial index
    for ( unsigned i = 0 ; i < polygons.size() ; ++i )
    {
        // calculate polygon bounding box
        box b = bg::return_envelope<box>(polygons[i]);
        // insert new value
        rtree.insert(std::make_pair(b, i));
    }

    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

    // note: in Boost.Geometry the WKT representation of a box is polygon

    // note: the values store the bounding boxes of polygons
    // the polygons aren't used for querying but are printed

    // display results
    ex_out << "spatial query box:" << std::endl;
    ex_out << bg::wkt<box>(query_box) << std::endl;
    ex_out << "spatial query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_s)
        ex_out << bg::wkt<polygon>(polygons[v.second]) << std::endl;

    ex_out << "knn query point:" << std::endl;
    ex_out << bg::wkt<point>(point(0, 0)) << std::endl;
    ex_out << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        ex_out << bg::wkt<polygon>(polygons[v.second]) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/polygons_vector") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(generated polygons:
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8,1 0))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2,2 1))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2,4 3))
POLYGON((5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2,5 4))
POLYGON((6 5,5.4 5.8,4.5 5.8,4 5,4.6 4.2,5.5 4.2,6 5))
POLYGON((7 6,6.4 6.8,5.5 6.8,5 6,5.6 5.2,6.5 5.2,7 6))
POLYGON((8 7,7.4 7.8,6.5 7.8,6 7,6.6 6.2,7.5 6.2,8 7))
POLYGON((9 8,8.4 8.8,7.5 8.8,7 8,7.6 7.2,8.5 7.2,9 8))
POLYGON((10 9,9.4 9.8,8.5 9.8,8 9,8.6 8.2,9.5 8.2,10 9))
spatial query box:
POLYGON((0 0,0 5,5 5,5 0,0 0))
spatial query result:
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8,1 0))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2,2 1))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2,4 3))
POLYGON((5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2,5 4))
POLYGON((6 5,5.4 5.8,4.5 5.8,4 5,4.6 4.2,5.5 4.2,6 5))
knn query point:
POINT(0 0)
knn query result:
POLYGON((5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2,5 4))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2,4 3))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8,1 0))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2,2 1))
)ex"));
#endif
}
}  // namespace ex_rtree_polygons_vector

// libs/geometry/doc/index/src/examples/rtree/quick_start.cpp
namespace ex_rtree_quick_start {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
//]

int example_main()
{
    //[rtree_quickstart_valuetype
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef std::pair<box, unsigned> value;
    //]

    //[rtree_quickstart_create
    // create the rtree using default constructor
    bgi::rtree< value, bgi::quadratic<16> > rtree;
    //]

    //[rtree_quickstart_insert
    // create some values
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // create a box
        box b(point(i + 0.0f, i + 0.0f), point(i + 0.5f, i + 0.5f));
        // insert new value
        rtree.insert(std::make_pair(b, i));
    }
    //]
    
    //[rtree_quickstart_spatial_query
    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));
    //]

    //[rtree_quickstart_nearest_query
    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));
    //]

    // note: in Boost.Geometry WKT representation of a box is polygon

    //[rtree_quickstart_output
    // display results
    ex_out << "spatial query box:" << std::endl;
    ex_out << bg::wkt<box>(query_box) << std::endl;
    ex_out << "spatial query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_s)
        ex_out << bg::wkt<box>(v.first) << " - " << v.second << std::endl;

    ex_out << "knn query point:" << std::endl;
    ex_out << bg::wkt<point>(point(0, 0)) << std::endl;
    ex_out << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        ex_out << bg::wkt<box>(v.first) << " - " << v.second << std::endl;
    //]

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/quick_start") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(spatial query box:
POLYGON((0 0,0 5,5 5,5 0,0 0))
spatial query result:
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0)) - 0
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1)) - 1
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2)) - 2
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3)) - 3
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4)) - 4
POLYGON((5 5,5 5.5,5.5 5.5,5.5 5,5 5)) - 5
knn query point:
POINT(0 0)
knn query result:
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4)) - 4
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3)) - 3
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2)) - 2
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0)) - 0
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1)) - 1
)ex"));
}
}  // namespace ex_rtree_quick_start

// libs/geometry/doc/index/src/examples/rtree/range_adaptors.cpp
namespace ex_rtree_range_adaptors {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

// Define a function object converting a value_type of indexed Range into std::pair<>.
// This is a generic implementation but of course it'd be possible to use some
// specific types. One could also take Value as template parameter and access
// first_type and second_type members, etc.
template <typename First, typename Second>
struct pair_maker
{
    typedef std::pair<First, Second> result_type;
    template<typename T>
    inline result_type operator()(T const& v) const
    {
        return result_type(v.value(), v.index());
    }
};

int example_main()
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;

    typedef std::vector<box> container;
    typedef container::size_type size_type;

    typedef std::pair<box, size_type> value;

    // create a container of boxes
    container boxes;
    for ( size_type i = 0 ; i < 10 ; ++i )
    {
        // add a box into the container
        box b(point(i + 0.0f, i + 0.0f), point(i + 0.5f, i + 0.5f));
        boxes.push_back(b);
    }

    // create the rtree passing a Range
    bgi::rtree< value, bgi::quadratic<16> >
        rtree(boxes | boost::adaptors::indexed()
                    | boost::adaptors::transformed(pair_maker<box, size_type>()));

    // print the number of values using boxes[0] as indexable
    ex_out << rtree.count(boxes[0]) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/range_adaptors") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(1
)ex"));
}
}  // namespace ex_rtree_range_adaptors

// libs/geometry/doc/index/src/examples/rtree/value_index.cpp
namespace ex_rtree_value_index {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

template <typename Container>
class my_indexable
{
    typedef typename Container::size_type size_t;
    typedef typename Container::const_reference cref;
    Container const& container;

public:
    typedef cref result_type;
    explicit my_indexable(Container const& c) : container(c) {}
    result_type operator()(size_t i) const { return container[i]; }
};

int example_main()
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef std::vector<box>::size_type value;
    typedef bgi::rstar<16, 4> parameters;
    typedef my_indexable< std::vector<box> > indexable_getter;

    // boxes
    std::vector<box> boxes;

    // create some boxes
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // add a box
        boxes.push_back(box(point(i+0.0f, i+0.0f), point(i+0.5f, i+0.5f)));
    }

    // display boxes
    ex_out << "generated boxes:" << std::endl;
    BOOST_FOREACH(box const& b, boxes)
        ex_out << bg::wkt<box>(b) << std::endl;

    // create the rtree
    parameters params;
    indexable_getter ind(boxes);
    bgi::rtree<value, parameters, indexable_getter> rtree(params, ind);

    // fill the spatial index
    for ( size_t i = 0 ; i < boxes.size() ; ++i )
        rtree.insert(i);

    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

    // note: in Boost.Geometry the WKT representation of a box is polygon

    // display results
    ex_out << "spatial query box:" << std::endl;
    ex_out << bg::wkt<box>(query_box) << std::endl;
    ex_out << "spatial query result:" << std::endl;
    BOOST_FOREACH(value i, result_s)
        ex_out << bg::wkt<box>(boxes[i]) << std::endl;

    ex_out << "knn query point:" << std::endl;
    ex_out << bg::wkt<point>(point(0, 0)) << std::endl;
    ex_out << "knn query result:" << std::endl;
    BOOST_FOREACH(value i, result_n)
        ex_out << bg::wkt<box>(boxes[i]) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/value_index") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(generated boxes:
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0))
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1))
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2))
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3))
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4))
POLYGON((5 5,5 5.5,5.5 5.5,5.5 5,5 5))
POLYGON((6 6,6 6.5,6.5 6.5,6.5 6,6 6))
POLYGON((7 7,7 7.5,7.5 7.5,7.5 7,7 7))
POLYGON((8 8,8 8.5,8.5 8.5,8.5 8,8 8))
POLYGON((9 9,9 9.5,9.5 9.5,9.5 9,9 9))
spatial query box:
POLYGON((0 0,0 5,5 5,5 0,0 0))
spatial query result:
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0))
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1))
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2))
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3))
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4))
POLYGON((5 5,5 5.5,5.5 5.5,5.5 5,5 5))
knn query point:
POINT(0 0)
knn query result:
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4))
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3))
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2))
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0))
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1))
)ex"));
}
}  // namespace ex_rtree_value_index

// libs/geometry/doc/index/src/examples/rtree/value_shared_ptr.cpp
namespace ex_rtree_value_shared_ptr {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

}  // namespace ex_rtree_value_shared_ptr (reopened after the block)
 namespace boost { namespace geometry { namespace index {

template <typename Box>
struct indexable< boost::shared_ptr<Box> >
{
    typedef boost::shared_ptr<Box> V;

    typedef Box const& result_type;
    result_type operator()(V const& v) const { return *v; }
};

}}}
namespace ex_rtree_value_shared_ptr { // namespace boost::geometry::index

int example_main()
{
    typedef bg::model::point<float, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef boost::shared_ptr<box> shp;
    typedef shp value;

    // create the rtree using default constructor
    bgi::rtree< value, bgi::linear<16, 4> > rtree;

    ex_out << "filling index with boxes shared pointers:" << std::endl;

    // fill the spatial index
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        // create a box
        shp b(new box(point(i+0.0f, i+0.0f), point(i+0.5f, i+0.5f)));

        // display new box
        ex_out << bg::wkt<box>(*b) << std::endl;

        // insert new value
        rtree.insert(b);
    }

    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

    // note: in Boost.Geometry the WKT representation of a box is polygon

    // display results
    ex_out << "spatial query box:" << std::endl;
    ex_out << bg::wkt<box>(query_box) << std::endl;
    ex_out << "spatial query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_s)
        ex_out << bg::wkt<box>(*v) << std::endl;

    ex_out << "knn query point:" << std::endl;
    ex_out << bg::wkt<point>(point(0, 0)) << std::endl;
    ex_out << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        ex_out << bg::wkt<box>(*v) << std::endl;

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/value_shared_ptr") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(filling index with boxes shared pointers:
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0))
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1))
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2))
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3))
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4))
POLYGON((5 5,5 5.5,5.5 5.5,5.5 5,5 5))
POLYGON((6 6,6 6.5,6.5 6.5,6.5 6,6 6))
POLYGON((7 7,7 7.5,7.5 7.5,7.5 7,7 7))
POLYGON((8 8,8 8.5,8.5 8.5,8.5 8,8 8))
POLYGON((9 9,9 9.5,9.5 9.5,9.5 9,9 9))
spatial query box:
POLYGON((0 0,0 5,5 5,5 0,0 0))
spatial query result:
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0))
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1))
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2))
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3))
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4))
POLYGON((5 5,5 5.5,5.5 5.5,5.5 5,5 5))
knn query point:
POINT(0 0)
knn query result:
POLYGON((4 4,4 4.5,4.5 4.5,4.5 4,4 4))
POLYGON((3 3,3 3.5,3.5 3.5,3.5 3,3 3))
POLYGON((2 2,2 2.5,2.5 2.5,2.5 2,2 2))
POLYGON((0 0,0 0.5,0.5 0.5,0.5 0,0 0))
POLYGON((1 1,1 1.5,1.5 1.5,1.5 1,1 1))
)ex"));
}
}  // namespace ex_rtree_value_shared_ptr

// libs/geometry/doc/index/src/examples/rtree/variants_map.cpp
namespace ex_rtree_variants_map {
namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

typedef bg::model::point<float, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;
typedef bg::model::polygon<point, false, false> polygon; // ccw, open polygon
typedef bg::model::ring<point, false, false> ring; // ccw, open ring
typedef bg::model::linestring<point> linestring;
typedef boost::variant<polygon, ring, linestring> geometry;

typedef std::map<unsigned, geometry> map;
typedef std::pair<box, map::iterator> value;

template <class Container>
void fill(unsigned i, Container & container)
{
    for ( float a = 0 ; a < 6.28316f ; a += 1.04720f )
    {
        float x = i + int(10*::cos(a))*0.1f;
        float y = i + int(10*::sin(a))*0.1f;
        container.push_back(point(x, y));
    }
}

struct print_visitor : public boost::static_visitor<>
{
    void operator()(polygon const& g) const { ex_out << bg::wkt<polygon>(g) << std::endl; }
    void operator()(ring const& g) const { ex_out << bg::wkt<ring>(g) << std::endl; }
    void operator()(linestring const& g) const { ex_out << bg::wkt<linestring>(g) << std::endl; }
};

struct envelope_visitor : public boost::static_visitor<box>
{
    box operator()(polygon const& g) const { return bg::return_envelope<box>(g); }
    box operator()(ring const& g) const { return bg::return_envelope<box>(g); }
    box operator()(linestring const& g) const { return bg::return_envelope<box>(g); }
};


int example_main()
{
    // geometries container
    map geometries;

    // create some geometries
    for ( unsigned i = 0 ; i < 10 ; ++i )
    {
        unsigned c = rand() % 3;

        if ( 0 == c )
        {
            // create polygon
            polygon p;
            fill(i, p.outer());
            geometries.insert(std::make_pair(i, geometry(p)));
        }
        else if ( 1 == c )
        {
            // create ring
            ring r;
            fill(i, r);
            geometries.insert(std::make_pair(i, geometry(r)));
        }
        else if ( 2 == c )
        {
            // create linestring
            linestring l;
            fill(i, l);
            geometries.insert(std::make_pair(i, geometry(l)));
        }
    }

    // display geometries
    ex_out << "generated geometries:" << std::endl;
    BOOST_FOREACH(map::value_type const& p, geometries)
        boost::apply_visitor(print_visitor(), p.second);

    // create the rtree using default constructor
    bgi::rtree< value, bgi::quadratic<16, 4> > rtree;

    // fill the spatial index
    for ( map::iterator it = geometries.begin() ; it != geometries.end() ; ++it )
    {
        // calculate polygon bounding box
        box b = boost::apply_visitor(envelope_visitor(), it->second);
        // insert new value
        rtree.insert(std::make_pair(b, it));
    }

    // find values intersecting some area defined by a box
    box query_box(point(0, 0), point(5, 5));
    std::vector<value> result_s;
    rtree.query(bgi::intersects(query_box), std::back_inserter(result_s));

    // find 5 nearest values to a point
    std::vector<value> result_n;
    rtree.query(bgi::nearest(point(0, 0), 5), std::back_inserter(result_n));

    // note: in Boost.Geometry the WKT representation of a box is polygon

    // note: the values store the bounding boxes of geometries
    // the geometries aren't used for querying but are printed

    // display results
    ex_out << "spatial query box:" << std::endl;
    ex_out << bg::wkt<box>(query_box) << std::endl;
    ex_out << "spatial query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_s)
        boost::apply_visitor(print_visitor(), v.second->second);

    ex_out << "knn query point:" << std::endl;
    ex_out << bg::wkt<point>(point(0, 0)) << std::endl;
    ex_out << "knn query result:" << std::endl;
    BOOST_FOREACH(value const& v, result_n)
        boost::apply_visitor(print_visitor(), v.second->second);

    return 0;
    return 0;  // main's implicit return
}

//]

TEST_CASE("example: rtree/variants_map") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(generated geometries:
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2))
LINESTRING(5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2)
POLYGON((6 5,5.4 5.8,4.5 5.8,4 5,4.6 4.2,5.5 4.2))
POLYGON((7 6,6.4 6.8,5.5 6.8,5 6,5.6 5.2,6.5 5.2))
POLYGON((8 7,7.4 7.8,6.5 7.8,6 7,6.6 6.2,7.5 6.2,8 7))
POLYGON((9 8,8.4 8.8,7.5 8.8,7 8,7.6 7.2,8.5 7.2,9 8))
POLYGON((10 9,9.4 9.8,8.5 9.8,8 9,8.6 8.2,9.5 8.2))
spatial query box:
POLYGON((0 0,0 5,5 5,5 0,0 0))
spatial query result:
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2))
LINESTRING(5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2)
POLYGON((6 5,5.4 5.8,4.5 5.8,4 5,4.6 4.2,5.5 4.2))
knn query point:
POINT(0 0)
knn query result:
LINESTRING(5 4,4.4 4.8,3.5 4.8,3 4,3.6 3.2,4.5 3.2)
POLYGON((4 3,3.4 3.8,2.5 3.8,2 3,2.6 2.2,3.5 2.2))
POLYGON((3 2,2.4 2.8,1.5 2.8,1 2,1.6 1.2,2.5 1.2,3 2))
POLYGON((1 0,0.4 0.8,-0.5 0.8,-1 0,-0.4 -0.8,0.5 -0.8))
POLYGON((2 1,1.4 1.8,0.5 1.8,0 1,0.6 0.2,1.5 0.2))
)ex"));
#endif
}
}  // namespace ex_rtree_variants_map
