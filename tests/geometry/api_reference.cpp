// mcpplibs.mwrap.boost.geometry — the reference pages of libs/geometry/doc/generated/
// (one case per page section, named after it): the models, coordinate systems
// and units, the core metafunctions, views and iterators, exceptions,
// de9im masks, svg_mapper, and every strategy page (area, buffer, centroid,
// densify, distance, line_interpolate, side, simplify, transform, within),
// each constructed as its synopsis shows and used through its algorithm.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.geometry;

namespace bg = boost::geometry;
using point = bg::model::d2::point_xy<double>;
using box = bg::model::box<point>;
using linestring = bg::model::linestring<point>;
using polygon = bg::model::polygon<point>;
using ring = bg::model::ring<point>;
using spoint = bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::degree>>;
using gpoint = bg::model::point<double, 2, bg::cs::geographic<bg::degree>>;

namespace {
template <class G>
G wkt(const char* s) {
    G g;
    bg::read_wkt(s, g);
    return g;
}
bool close_to(double a, double b, double eps = 1e-6) { return std::abs(a - b) <= eps * std::max(1.0, std::abs(b)); }
}  // namespace

// ---- models ----

TEST_CASE("doc: model_point") {
    bg::model::point<double, 3, bg::cs::cartesian> p(1, 2, 3);
    CHECK(p.get<0>() == 1);
    p.set<2>(5);
    CHECK(bg::get<2>(p) == 5);
    static_assert(bg::dimension<decltype(p)>::value == 3);
}
TEST_CASE("doc: model_d2_point_xy") {
    point p(1, 2);
    p.x(3);
    CHECK(p.x() == 3);
    CHECK(p.y() == 2);
}
TEST_CASE("doc: model_d3_point_xyz") {
    bg::model::d3::point_xyz<int> p(1, 2, 3);
    p.z(9);
    CHECK(p.x() + p.y() + p.z() == 12);
}
TEST_CASE("doc: model_box") {
    box b(point(0, 0), point(2, 3));
    CHECK(b.max_corner().y() == 3);
    CHECK(bg::area(b) == 6);
}
TEST_CASE("doc: model_linestring") {
    linestring ls{{0, 0}, {3, 4}};
    CHECK(bg::length(ls) == 5);
}
TEST_CASE("doc: model_ring") {
    ring r{{0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}};
    CHECK(bg::area(r) == 1);
}
TEST_CASE("doc: model_polygon") {
    polygon p{{{0, 0}, {0, 5}, {5, 5}, {5, 0}, {0, 0}}, {{1, 1}, {2, 1}, {2, 2}, {1, 2}, {1, 1}}};
    CHECK(p.inners().size() == 1);
    CHECK(bg::area(p) == 24);
}
TEST_CASE("doc: model_multi_point") {
    bg::model::multi_point<point> mp{{0, 0}, {1, 1}};
    CHECK(bg::num_points(mp) == 2);
}
TEST_CASE("doc: model_multi_linestring") {
    bg::model::multi_linestring<linestring> ml{{{0, 0}, {1, 0}}, {{0, 0}, {0, 2}}};
    CHECK(bg::length(ml) == 3);
}
TEST_CASE("doc: model_multi_polygon") {
    bg::model::multi_polygon<polygon> mp{{{{0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}}}, {{{5, 5}, {5, 6}, {6, 6}, {6, 5}, {5, 5}}}};
    CHECK(bg::area(mp) == 2);
}
TEST_CASE("doc: model_polyhedral_surface") {
    using pt3 = bg::model::point<double, 3, bg::cs::cartesian>;
    bg::model::polyhedral_surface<bg::model::polygon<pt3>> ps;
    ps.resize(1);
    CHECK(ps.size() == 1);
    static_assert(std::is_same_v<bg::tag_t<decltype(ps)>, bg::polyhedral_surface_tag>);
}
TEST_CASE("doc: model_segment") {
    bg::model::segment<point> s(point(0, 0), point(0, 4));
    CHECK(bg::length(s) == 4);
}
TEST_CASE("doc: model_referring_segment") {
    point a(0, 0), b(3, 4);
    bg::model::referring_segment<point> s(a, b);
    b.x(6);  // it refers to the points
    CHECK(bg::get<1, 0>(s) == 6);
}

// ---- coordinate systems and units ----

TEST_CASE("doc: cs_cartesian") {
    static_assert(std::is_same_v<bg::cs_tag_t<point>, bg::cartesian_tag>);
}
TEST_CASE("doc: cs_spherical") {
    using p = bg::model::point<double, 2, bg::cs::spherical<bg::degree>>;
    static_assert(std::is_same_v<bg::cs_tag_t<p>, bg::spherical_polar_tag>);
}
TEST_CASE("doc: cs_spherical_equatorial") {
    static_assert(std::is_same_v<bg::cs_tag_t<spoint>, bg::spherical_equatorial_tag>);
    CHECK(close_to(bg::distance(spoint(0, 0), spoint(0, 90)), bg::math::pi<double>() / 2));
}
TEST_CASE("doc: cs_geographic") {
    static_assert(std::is_same_v<bg::cs_tag_t<gpoint>, bg::geographic_tag>);
    CHECK(close_to(bg::distance(gpoint(0, 0), gpoint(0, 1)), 110574.3886, 1e-4));
}
TEST_CASE("doc: cs_polar") {
    using p = bg::model::point<double, 2, bg::cs::polar<bg::radian>>;
    static_assert(std::is_same_v<bg::coordinate_system_t<p>, bg::cs::polar<bg::radian>>);
}
TEST_CASE("doc: degree") {
    static_assert(std::is_same_v<bg::coordinate_system_t<spoint>::units, bg::degree>);
}
TEST_CASE("doc: radian") {
    using p = bg::model::point<double, 2, bg::cs::spherical_equatorial<bg::radian>>;
    CHECK(close_to(bg::distance(p(0, 0), p(bg::math::pi<double>() / 2, 0)), bg::math::pi<double>() / 2));
}

// ---- core metafunctions ----

TEST_CASE("doc: closure") {
    static_assert(bg::closure<polygon>::value == bg::closed);
    static_assert(bg::closure<bg::model::polygon<point, true, false>>::value == bg::open);
}
TEST_CASE("doc: coordinate_system") {
    static_assert(std::is_same_v<bg::coordinate_system<point>::type, bg::cs::cartesian>);
}
TEST_CASE("doc: coordinate_type") {
    static_assert(std::is_same_v<bg::coordinate_type<bg::model::d2::point_xy<float>>::type, float>);
}
TEST_CASE("doc: cs_tag") {
    static_assert(std::is_same_v<bg::cs_tag<gpoint>::type, bg::geographic_tag>);
}
TEST_CASE("doc: dimension") {
    static_assert(bg::dimension<polygon>::value == 2);
}
TEST_CASE("doc: interior_type") {
    static_assert(std::is_same_v<bg::interior_type<polygon>::type, std::vector<ring>>);
}
TEST_CASE("doc: point_order") {
    static_assert(bg::point_order<polygon>::value == bg::clockwise);
    static_assert(bg::point_order<bg::model::polygon<point, false>>::value == bg::counterclockwise);
}
TEST_CASE("doc: point_type") {
    static_assert(std::is_same_v<bg::point_type<linestring>::type, point>);
}
TEST_CASE("doc: ring_type") {
    static_assert(std::is_same_v<bg::ring_type<polygon>::type, ring>);
}
TEST_CASE("doc: tag") {
    static_assert(std::is_same_v<bg::tag<box>::type, bg::box_tag>);
    static_assert(std::is_same_v<bg::tag<ring>::type, bg::ring_tag>);
}
TEST_CASE("doc: tag_cast") {
    static_assert(std::is_same_v<bg::tag_cast<bg::polygon_tag, bg::areal_tag>::type, bg::areal_tag>);
    static_assert(std::is_same_v<bg::tag_cast<bg::point_tag, bg::areal_tag>::type, bg::point_tag>);
}

// ---- views and iterators ----

TEST_CASE("doc: box_view") {
    box b(point(0, 0), point(1, 2));
    bg::box_view<box> view(b);
    CHECK(boost::size(view) == 5);
}
TEST_CASE("doc: segment_view") {
    bg::model::segment<point> s(point(0, 0), point(1, 1));
    bg::segment_view<decltype(s)> view(s);
    CHECK(boost::size(view) == 2);
}
TEST_CASE("doc: closeable_view") {
    using open_ring = bg::model::ring<point, true, false>;
    open_ring r{{0, 0}, {0, 1}, {1, 1}};
    bg::closeable_view<open_ring const, bg::open>::type view(r);
    CHECK(boost::size(view) == 4);  // the first point again
    bg::closeable_view<ring const, bg::closed>::type same(ring{{0, 0}, {0, 1}, {0, 0}});
    CHECK(boost::size(same) == 3);
}
TEST_CASE("doc: reversible_view") {
    linestring ls{{0, 0}, {1, 1}, {2, 2}};
    bg::reversible_view<linestring const, bg::iterate_reverse>::type view(ls);
    CHECK(bg::get<0>(*boost::begin(view)) == 2);
    bg::reversible_view<linestring const, bg::iterate_forward>::type same(ls);
    CHECK(bg::get<0>(*boost::begin(same)) == 0);
}
TEST_CASE("doc: identity_view") {
    linestring ls{{0, 0}, {1, 1}};
    boost::sub_range<linestring const> r(ls);
    CHECK(boost::size(r) == 2);
    static_assert(bg::is_areal<polygon>::value);
}
TEST_CASE("doc: closing_iterator") {
    ring r{{0, 0}, {0, 1}, {1, 1}};
    bg::closing_iterator<ring const> it(r), end(r, true);
    std::size_t n = 0;
    for (; it != end; ++it)
        ++n;
    CHECK(n == 4);
}
TEST_CASE("doc: ever_circling_iterator") {
    std::vector<int> v{1, 2, 3};
    bg::ever_circling_iterator<std::vector<int>::iterator> it(v.begin(), v.end());
    std::vector<int> got;
    for (int i = 0; i < 7; ++i, ++it)
        got.push_back(*it);
    CHECK(got == std::vector<int>{1, 2, 3, 1, 2, 3, 1});
}

// ---- exceptions ----

TEST_CASE("doc: exception") {
    static_assert(std::is_base_of_v<std::exception, bg::exception>);
    CHECK(std::string(bg::exception().what()).size() > 0);
}
TEST_CASE("doc: centroid_exception") {
    static_assert(std::is_base_of_v<bg::exception, bg::centroid_exception>);
    polygon empty;
    CHECK_THROWS_AS(bg::return_centroid<point>(empty), bg::centroid_exception);
}

// ---- de9im ----

TEST_CASE("doc: de9im_mask") {
    polygon a = wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    polygon b = wkt<polygon>("POLYGON((1 1,1 3,3 3,3 1,1 1))");
    CHECK(bg::relate(a, b, bg::de9im::mask("T*T***T**")));  // overlaps
    CHECK_FALSE(bg::relate(a, b, bg::de9im::mask("FF*FF****")));  // disjoint
}
TEST_CASE("doc: de9im_matrix") {
    polygon a = wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    point p(1, 1);
    bg::de9im::matrix m = bg::relation(p, a);
    CHECK(m.str() == "0FFFFF212");
}
TEST_CASE("doc: de9im_static_mask") {
    using within_mask = bg::de9im::static_mask<'T', '*', 'F', '*', '*', 'F', '*', '*', '*'>;  // T*F**F***
    CHECK(bg::relate(point(1, 1), wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))"), within_mask()));
}

// ---- svg_mapper ----

TEST_CASE("doc: svg_mapper") {
    std::ostringstream svg;
    {
        bg::svg_mapper<point> mapper(svg, 100, 100);
        polygon p = wkt<polygon>("POLYGON((0 0,0 7,4 2,2 0,0 0))");
        mapper.add(p);
        mapper.map(p, "fill:rgb(0,0,255)");
        mapper.text(point(1, 1), "label", "font-size:8pt");
    }
    CHECK(svg.str().starts_with("<?xml"));
    CHECK(svg.str().find("fill:rgb(0,0,255)") != std::string::npos);
    CHECK(svg.str().find("label") != std::string::npos);
    CHECK(svg.str().ends_with("</svg>\n"));
}

// ---- strategies ----

TEST_CASE("doc: strategy_area_cartesian") {
    bg::strategy::area::cartesian<> s;
    CHECK(bg::area(wkt<polygon>("POLYGON((0 0,0 7,4 2,2 0,0 0))"), s) == 16);
}
TEST_CASE("doc: strategy_area_spherical") {
    using sp = bg::model::polygon<spoint>;
    bg::strategy::area::spherical<> unit;           // unit sphere
    bg::strategy::area::spherical<> earth(6371.0);  // km
    sp p = wkt<sp>("POLYGON((0 0,0 45,45 0,0 0))");
    double a1 = bg::area(p, unit), a2 = bg::area(p, earth);
    CHECK(close_to(a2 / a1, 6371.0 * 6371.0));
}
TEST_CASE("doc: strategy_area_geographic") {
    using gp = bg::model::polygon<gpoint>;
    gp p = wkt<gp>("POLYGON((0 0,0 1,1 1,1 0,0 0))");
    bg::strategy::area::geographic<> s;
    bg::strategy::area::geographic<bg::strategy::andoyer> s2(bg::srs::spheroid<double>(6378137.0, 6356752.3142451793));
    CHECK(close_to(bg::area(p, s), bg::area(p, s2), 1e-3));
    CHECK(bg::area(p, s) > 1.2e10);
}
TEST_CASE("doc: strategy_buffer_distance_asymmetric") {
    bg::strategy::buffer::distance_asymmetric<double> d(1.0, 0.5);
    bg::model::multi_polygon<polygon> out;
    bg::buffer(linestring{{0, 0}, {4, 0}}, out, d, bg::strategy::buffer::side_straight(),
               bg::strategy::buffer::join_round(), bg::strategy::buffer::end_flat(), bg::strategy::buffer::point_circle());
    CHECK(close_to(bg::area(out), 4 * 1.5, 1e-9));
}
TEST_CASE("doc: strategy_buffer_distance_symmetric") {
    bg::strategy::buffer::distance_symmetric<double> d(1.0);
    bg::model::multi_polygon<polygon> out;
    bg::buffer(linestring{{0, 0}, {4, 0}}, out, d, bg::strategy::buffer::side_straight(),
               bg::strategy::buffer::join_round(), bg::strategy::buffer::end_flat(), bg::strategy::buffer::point_circle());
    CHECK(close_to(bg::area(out), 8, 1e-9));
}
namespace {
double buffered_area(auto const& geometry, auto join, auto end, auto pt) {
    bg::model::multi_polygon<polygon> out;
    bg::buffer(geometry, out, bg::strategy::buffer::distance_symmetric<double>(1.0), bg::strategy::buffer::side_straight(),
               join, end, pt);
    return bg::area(out);
}
}  // namespace
TEST_CASE("doc: strategy_buffer_end_flat") {
    CHECK(close_to(buffered_area(linestring{{0, 0}, {2, 0}}, bg::strategy::buffer::join_miter(), bg::strategy::buffer::end_flat(),
                             bg::strategy::buffer::point_square()), 4));
}
TEST_CASE("doc: strategy_buffer_end_round") {
    double a = buffered_area(linestring{{0, 0}, {2, 0}}, bg::strategy::buffer::join_miter(), bg::strategy::buffer::end_round(360),
                             bg::strategy::buffer::point_square());
    CHECK(close_to(a, 4 + bg::math::pi<double>(), 1e-3));
}
TEST_CASE("doc: strategy_buffer_join_miter") {
    polygon sq = wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    CHECK(close_to(buffered_area(sq, bg::strategy::buffer::join_miter(), bg::strategy::buffer::end_flat(),
                             bg::strategy::buffer::point_square()), 16));
}
TEST_CASE("doc: strategy_buffer_join_round") {
    polygon sq = wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    double a = buffered_area(sq, bg::strategy::buffer::join_round(360), bg::strategy::buffer::end_flat(),
                             bg::strategy::buffer::point_square());
    CHECK(close_to(a, 4 + 8 + bg::math::pi<double>(), 1e-3));
}
TEST_CASE("doc: strategy_buffer_point_circle") {
    double a = buffered_area(point(0, 0), bg::strategy::buffer::join_round(), bg::strategy::buffer::end_round(),
                             bg::strategy::buffer::point_circle(360));
    CHECK(close_to(a, bg::math::pi<double>(), 1e-3));
}
TEST_CASE("doc: strategy_buffer_point_square") {
    double a = buffered_area(point(0, 0), bg::strategy::buffer::join_round(), bg::strategy::buffer::end_round(),
                             bg::strategy::buffer::point_square());
    CHECK(close_to(a, 4));
}
TEST_CASE("doc: strategy_buffer_side_straight") {
    CHECK(close_to(buffered_area(linestring{{0, 0}, {3, 0}}, bg::strategy::buffer::join_miter(), bg::strategy::buffer::end_flat(),
                             bg::strategy::buffer::point_square()), 6));
}
TEST_CASE("doc: strategy_buffer_geographic_point_circle") {
    bg::model::multi_polygon<bg::model::polygon<gpoint>> out;
    bg::strategy::buffer::geographic_point_circle<> circle(36);
    bg::buffer(gpoint(4.9, 52.1), out, bg::strategy::buffer::distance_symmetric<double>(1000.0),
               bg::strategy::buffer::side_straight(), bg::strategy::buffer::join_round(),
               bg::strategy::buffer::end_round(), circle);
    CHECK(close_to(bg::area(out), bg::math::pi<double>() * 1000 * 1000, 1e-2));
}
TEST_CASE("doc: strategy_centroid_average") {
    bg::model::multi_point<point> mp{{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    point c;
    bg::centroid(mp, c, bg::strategy::centroid::average<>());
    CHECK(bg::get<0>(c) == 1);
    CHECK(bg::get<1>(c) == 1);
}
TEST_CASE("doc: strategy_centroid_bashein_detmer") {
    point c;
    bg::centroid(wkt<polygon>("POLYGON((0 0,0 4,4 4,4 0,0 0))"), c, bg::strategy::centroid::bashein_detmer<point, point>());
    CHECK(close_to(bg::get<0>(c), 2));
}
TEST_CASE("doc: strategy_densify_cartesian") {
    linestring out;
    bg::densify(linestring{{0, 0}, {10, 0}}, out, 2.5, bg::strategy::densify::cartesian<>());
    REQUIRE(out.size() >= 5);
    for (std::size_t i = 0; i + 1 < out.size(); ++i)
        CHECK(bg::distance(out[i], out[i + 1]) <= 2.5 + 1e-12);
}
TEST_CASE("doc: strategy_densify_spherical") {
    bg::model::linestring<spoint> out;
    bg::densify(bg::model::linestring<spoint>{{0, 0}, {0, 90}}, out, bg::math::pi<double>() / 8,
                bg::strategy::densify::spherical<>());
    CHECK(out.size() >= 5);
}
TEST_CASE("doc: strategy_densify_geographic") {
    bg::model::linestring<gpoint> out;
    bg::densify(bg::model::linestring<gpoint>{{0, 0}, {0, 1}}, out, 10000.0, bg::strategy::densify::geographic<>());
    CHECK(out.size() == 13);  // 110.6 km in steps of at most 10 km
}
TEST_CASE("doc: strategy_distance_pythagoras") {
    CHECK(bg::distance(point(0, 0), point(3, 4), bg::strategy::distance::pythagoras<>()) == 5);
}
TEST_CASE("doc: strategy_distance_pythagoras_box_box") {
    box a(point(0, 0), point(1, 1)), b(point(4, 5), point(6, 6));
    CHECK(bg::distance(a, b, bg::strategy::distance::pythagoras_box_box<>()) == 5);
}
TEST_CASE("doc: strategy_distance_pythagoras_point_box") {
    CHECK(bg::distance(point(4, 5), box(point(0, 0), point(1, 1)), bg::strategy::distance::pythagoras_point_box<>()) == 5);
}
TEST_CASE("doc: strategy_distance_projected_point") {
    CHECK(bg::distance(point(1, 1), linestring{{0, 0}, {2, 0}}, bg::strategy::distance::projected_point<>()) == 1);
}
TEST_CASE("doc: strategy_distance_haversine") {
    bg::strategy::distance::haversine<double> earth(6371.0);
    CHECK(close_to(bg::distance(spoint(0, 0), spoint(0, 90), earth), 6371.0 * bg::math::pi<double>() / 2));
}
TEST_CASE("doc: strategy_distance_cross_track") {
    bg::strategy::distance::cross_track<> unit;
    CHECK(close_to(bg::distance(spoint(0, 10), bg::model::linestring<spoint>{{-10, 0}, {10, 0}}, unit),
               10 * bg::math::d2r<double>(), 1e-9));
}
TEST_CASE("doc: strategy_distance_cross_track_point_box") {
    bg::model::box<spoint> b(spoint(0, 0), spoint(10, 10));
    double d = bg::distance(spoint(5, 20), b, bg::strategy::distance::cross_track_point_box<>());
    CHECK(close_to(d, 10 * bg::math::d2r<double>(), 1e-9));
}
TEST_CASE("doc: strategy_line_interpolate_cartesian") {
    point p;
    bg::line_interpolate(linestring{{0, 0}, {10, 0}}, 2.5, p, bg::strategy::line_interpolate::cartesian<>());
    CHECK(bg::get<0>(p) == 2.5);
}
TEST_CASE("doc: strategy_line_interpolate_spherical") {
    spoint p;
    bg::line_interpolate(bg::model::linestring<spoint>{{0, 0}, {90, 0}}, bg::math::pi<double>() / 4, p,
                         bg::strategy::line_interpolate::spherical<>());
    CHECK(close_to(bg::get<0>(p), 45));
}
TEST_CASE("doc: strategy_line_interpolate_geographic") {
    gpoint p;
    bg::line_interpolate(bg::model::linestring<gpoint>{{0, 0}, {0, 1}}, 110574.3886 / 2, p,
                         bg::strategy::line_interpolate::geographic<>());
    CHECK(close_to(bg::get<1>(p), 0.5, 1e-4));
}
TEST_CASE("doc: strategy_side_side_by_triangle") {
    using side = bg::strategy::side::side_by_triangle<>;
    CHECK(side::apply(point(0, 0), point(1, 0), point(0, 1)) == 1);   // left
    CHECK(side::apply(point(0, 0), point(1, 0), point(0, -1)) == -1);  // right
    CHECK(side::apply(point(0, 0), point(1, 0), point(2, 0)) == 0);
}
TEST_CASE("doc: strategy_side_side_by_cross_track") {
    using side = bg::strategy::side::side_by_cross_track<>;
    CHECK(side::apply(spoint(0, 0), spoint(10, 0), spoint(5, 5)) == 1);
}
TEST_CASE("doc: strategy_side_spherical_side_formula") {
    using side = bg::strategy::side::spherical_side_formula<>;
    CHECK(side::apply(spoint(0, 0), spoint(10, 0), spoint(5, -5)) == -1);
}
TEST_CASE("doc: strategy_side_geographic") {
    bg::strategy::side::geographic<> side;
    CHECK(side.apply(gpoint(0, 0), gpoint(10, 0), gpoint(5, 5)) == 1);
}
TEST_CASE("doc: strategy_simplify_douglas_peucker") {
    linestring out;
    bg::simplify(linestring{{0, 0}, {1, 0.1}, {2, 0}, {3, 5}}, out, 0.5);
    CHECK(out.size() == 3);
    using dp = bg::strategy::simplify::douglas_peucker<point, bg::strategy::distance::projected_point<>>;
    linestring out2;
    bg::simplify(linestring{{0, 0}, {1, 0.1}, {2, 0}, {3, 5}}, out2, 0.5, dp());
    CHECK(bg::equals(out, out2));
}
TEST_CASE("doc: strategy_transform_translate_transformer") {
    point out;
    bg::transform(point(1, 1), out, bg::strategy::transform::translate_transformer<double, 2, 2>(1.5, 1.5));
    CHECK(out.x() == 2.5);
}
TEST_CASE("doc: strategy_transform_scale_transformer") {
    point out;
    bg::transform(point(1, 2), out, bg::strategy::transform::scale_transformer<double, 2, 2>(3.0));
    CHECK(out.y() == 6);
}
TEST_CASE("doc: strategy_transform_rotate_transformer") {
    point out;
    bg::transform(point(1, 0), out, bg::strategy::transform::rotate_transformer<bg::degree, double, 2, 2>(90.0));
    CHECK(close_to(out.x(), 0, 1e-12));
    CHECK(close_to(out.y(), -1));  // clockwise
}
TEST_CASE("doc: strategy_transform_matrix_transformer") {
    bg::strategy::transform::matrix_transformer<double, 2, 2> m(2, 0, 1, 0, 3, 2, 0, 0, 1);  // scale, then translate
    point out;
    bg::transform(point(1, 1), out, m);
    CHECK(out.x() == 3);
    CHECK(out.y() == 5);
}
TEST_CASE("doc: strategy_transform_inverse_transformer") {
    bg::strategy::transform::translate_transformer<double, 2, 2> t(1, 2);
    bg::strategy::transform::inverse_transformer<double, 2, 2> inv(t);
    point out;
    bg::transform(point(1, 2), out, inv);
    CHECK(close_to(out.x(), 0, 1e-12));
    CHECK(close_to(out.y(), 0, 1e-12));
}
TEST_CASE("doc: strategy_transform_map_transformer") {
    // a world box onto a 100 x 100 device, y mirrored
    box world(point(0, 0), point(10, 10));
    bg::strategy::transform::map_transformer<double, 2, 2, true, false> m(world, 100, 100);
    point out;
    bg::transform(point(10, 10), out, m);
    CHECK(close_to(out.x(), 100));
    CHECK(close_to(out.y(), 0, 1e-12));
}
TEST_CASE("doc: strategy_within_winding") {
    // The page's member functions: apply per segment, then result. (bg::within
    // does not take this agnostic strategy in 1.92.0, upstream included; it
    // takes cartesian_winding.)
    ring r = wkt<ring>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    bg::strategy::within::winding<point> w;
    auto run = [&](point p) {
        decltype(w)::state_type st;
        for (std::size_t i = 0; i + 1 < r.size(); ++i)
            if (!w.apply(p, r[i], r[i + 1], st))
                break;
        return w.result(st);
    };
    CHECK(run(point(1, 1)) == 1);
    CHECK(run(point(3, 1)) == -1);
    CHECK(bg::within(point(1, 1), r, bg::strategy::within::cartesian_winding<point, point>()));
}
TEST_CASE("doc: strategy_within_franklin") {
    polygon sq = wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    CHECK(bg::within(point(1, 1), sq, bg::strategy::within::franklin<point>()));
    CHECK_FALSE(bg::within(point(3, 1), sq, bg::strategy::within::franklin<point>()));
}
TEST_CASE("doc: strategy_within_crossings_multiply") {
    polygon sq = wkt<polygon>("POLYGON((0 0,0 2,2 2,2 0,0 0))");
    CHECK(bg::within(point(1, 1), sq, bg::strategy::within::crossings_multiply<point>()));
}
TEST_CASE("doc: srs_spheroid") {
    bg::srs::spheroid<double> wgs84(6378137.0, 6356752.3142451793);
    CHECK(bg::get_radius<0>(wgs84) == 6378137.0);
    CHECK(close_to(bg::get_radius<2>(wgs84), 6356752.3142451793));
    bg::strategy::distance::andoyer<bg::srs::spheroid<double>> andoyer(wgs84);
    CHECK(close_to(bg::distance(gpoint(0, 0), gpoint(0, 1), andoyer), 110574.3886, 1e-3));
}
