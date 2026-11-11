// mcpplibs.mwrap.boost.geometry — the rtree pages of libs/geometry/doc/index/:
// creation and modification (the balancing algorithms, static and dynamic
// parameters, copy/move/swap, insert/remove of values, iterator pairs and
// ranges, inserter), queries (member and free query, query iterators, the
// spatial predicates, nearest, satisfies, negation and conjunction, breaking
// out of a nearest iteration, query results as a range), and the reference of
// predicates.qbk, rtree.qbk and rtree_functions.qbk.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.geometry;

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
using point = bg::model::point<double, 2, bg::cs::cartesian>;
using box = bg::model::box<point>;
using value = std::pair<box, int>;
using rtree = bgi::rtree<value, bgi::quadratic<16>>;

namespace {
box unit_at(double x, double y) { return box(point(x, y), point(x + 1, y + 1)); }
std::vector<value> grid(int n) {  // n x n boxes, id = x * n + y
    std::vector<value> v;
    for (int x = 0; x < n; ++x)
        for (int y = 0; y < n; ++y)
            v.emplace_back(unit_at(2.0 * x, 2.0 * y), x * n + y);
    return v;
}
std::set<int> ids(const std::vector<value>& v) {
    std::set<int> s;
    for (auto const& x : v)
        s.insert(x.second);
    return s;
}
}  // namespace

TEST_CASE("doc: creation-and-modification") {
    auto values = grid(4);
    // the balancing algorithms, compile-time and run-time parameters
    bgi::rtree<value, bgi::linear<16>> l(values);
    bgi::rtree<value, bgi::quadratic<16>> q(values);
    bgi::rtree<value, bgi::rstar<16>> r(values);
    bgi::rtree<value, bgi::dynamic_linear> dl(values, bgi::dynamic_linear(16));
    bgi::rtree<value, bgi::dynamic_quadratic> dq(values, bgi::dynamic_quadratic(16));
    bgi::rtree<value, bgi::dynamic_rstar> dr(values, bgi::dynamic_rstar(16));
    for (std::size_t s : {l.size(), q.size(), r.size(), dl.size(), dq.size(), dr.size()})
        CHECK(s == 16);
    CHECK(dr.parameters().get_max_elements() == 16);

    // copy, move, swap
    bgi::rtree<value, bgi::rstar<8>> rt1(values.begin(), values.end());
    bgi::rtree<value, bgi::rstar<8>> rt2(rt1);
    rt2 = rt1;
    bgi::rtree<value, bgi::rstar<8>> rt3(std::move(rt1));
    CHECK(rt3.size() == 16);
    CHECK(rt1.empty());
    rt3 = std::move(rt2);
    rt3.swap(rt1);
    CHECK(rt1.size() == 16);
    CHECK(rt3.empty());

    // insert and remove: members and free functions, value, iterators, range
    rtree t;
    value v = std::make_pair(unit_at(100, 100), 99);
    t.insert(v);
    bgi::insert(t, std::make_pair(unit_at(200, 200), 98));
    CHECK(t.size() == 2);
    CHECK(t.remove(v) == 1);
    CHECK(bgi::remove(t, std::make_pair(unit_at(200, 200), 98)) == 1);
    CHECK(t.empty());
    t.insert(values.begin(), values.end());
    CHECK(t.remove(values.begin(), values.begin() + 4) == 4);
    t.insert(values);
    CHECK(t.size() == 16 + 12);
    t.clear();
    CHECK(t.empty());

    // inserter, and a tree from a query result
    rtree from_copy;
    std::copy(values.begin(), values.end(), bgi::inserter(from_copy));
    CHECK(from_copy.size() == 16);
    rtree from_query;
    from_copy.query(bgi::intersects(box(point(0, 0), point(3, 3))), bgi::inserter(from_query));
    CHECK(from_query.size() == 4);
    rtree from_adaptor(from_copy | bgi::adaptors::queried(bgi::intersects(box(point(0, 0), point(1, 1)))));
    CHECK(from_adaptor.size() == 1);
}

TEST_CASE("doc: queries") {
    rtree rt(grid(4));
    box region(point(0, 0), point(2.5, 2.5));
    std::vector<value> a, b, c, d;
    rt.query(bgi::intersects(region), std::back_inserter(a));
    bgi::query(rt, bgi::intersects(region), std::back_inserter(b));
    std::copy(rt.qbegin(bgi::intersects(region)), rt.qend(), std::back_inserter(c));
    std::copy(bgi::qbegin(rt, bgi::intersects(region)), bgi::qend(rt), std::back_inserter(d));
    CHECK(ids(a) == std::set<int>{0, 1, 4, 5});
    CHECK(ids(b) == ids(a));
    CHECK(ids(c) == ids(a));
    CHECK(ids(d) == ids(a));

    // nearest: k values closest to a point, a box, a segment
    std::vector<value> near_pt, near_box, near_seg;
    rt.query(bgi::nearest(point(0, 0), 1), std::back_inserter(near_pt));
    rt.query(bgi::nearest(box(point(6.5, 6.5), point(7, 7)), 1), std::back_inserter(near_box));
    rt.query(bgi::nearest(bg::model::segment<point>(point(-1, 2.5), point(-1, 2.6)), 1), std::back_inserter(near_seg));
    CHECK(ids(near_pt) == std::set<int>{0});
    CHECK(ids(near_box) == std::set<int>{15});
    CHECK(ids(near_seg) == std::set<int>{1});

    // satisfies with a function, a function object and a lambda; negation
    auto is_odd = [](value const& v) { return v.second % 2 == 1; };
    std::vector<value> odd, odd2;
    rt.query(bgi::intersects(region) && bgi::satisfies(is_odd), std::back_inserter(odd));
    rt.query(bgi::intersects(region) && !bgi::satisfies([](value const& v) { return v.second % 2 == 0; }),
             std::back_inserter(odd2));
    CHECK(ids(odd) == std::set<int>{1, 5});
    CHECK(ids(odd2) == ids(odd));

    // a combination with nearest; breaking out of an iterative nearest query
    std::vector<value> combo;
    bgi::query(rt, bgi::nearest(point(0, 0), 3) && bgi::within(box(point(-1, -1), point(10, 3.5))), std::back_inserter(combo));
    CHECK(combo.size() == 3);
    std::size_t visited = 0;
    for (auto it = rt.qbegin(bgi::nearest(point(0, 0), 10000)); it != rt.qend(); ++it) {
        if (bg::distance(point(0, 0), it->first) > 3)
            break;
        ++visited;
    }
    CHECK(visited == 4);  // (0,0), (0,2), (2,0), (2,2)
}

// ---- predicates.qbk ----

namespace {
std::set<int> query(const rtree& rt, auto pred) {
    std::vector<value> r;
    rt.query(pred, std::back_inserter(r));
    return ids(r);
}
const rtree& tree() {
    static rtree rt(grid(3));  // boxes at 0..1, 2..3, 4..5 in each axis
    return rt;
}
}  // namespace

TEST_CASE("doc: contains-geometry-const") {
    CHECK(query(tree(), bgi::contains(point(0.5, 0.5))) == std::set<int>{0});
}
TEST_CASE("doc: covered-by-geometry-const") {
    CHECK(query(tree(), bgi::covered_by(box(point(0, 0), point(3, 1)))) == std::set<int>{0, 3});
}
TEST_CASE("doc: covers-geometry-const") {
    CHECK(query(tree(), bgi::covers(point(1, 1))) == std::set<int>{0});  // its corner
}
TEST_CASE("doc: disjoint-geometry-const") {
    CHECK(query(tree(), bgi::disjoint(box(point(0, 0), point(3.5, 5.5)))) == std::set<int>{6, 7, 8});
    CHECK(query(tree(), !bgi::intersects(box(point(0, 0), point(3.5, 5.5)))) == std::set<int>{6, 7, 8});
}
TEST_CASE("doc: intersects-geometry-const") {
    CHECK(query(tree(), bgi::intersects(bg::model::segment<point>(point(0.5, 0.5), point(4.5, 0.5)))) ==
          std::set<int>{0, 3, 6});
}
TEST_CASE("doc: nearest-geometry-const-std-size-t") {
    CHECK(query(tree(), bgi::nearest(point(10, 10), 2)).size() == 2);
    CHECK(query(tree(), bgi::nearest(point(10, 10), 1)) == std::set<int>{8});
}
TEST_CASE("doc: overlaps-geometry-const") {
    CHECK(query(tree(), bgi::overlaps(box(point(0.5, 0.5), point(2.5, 0.75)))) == std::set<int>{0, 3});
}
TEST_CASE("doc: satisfies-unarypredicate-const") {
    struct is_center {
        bool operator()(value const& v) const { return v.second == 4; }
    };
    CHECK(query(tree(), bgi::satisfies(is_center())) == std::set<int>{4});
}
TEST_CASE("doc: within-geometry-const") {
    CHECK(query(tree(), bgi::within(box(point(-1, -1), point(3.5, 1.5)))) == std::set<int>{0, 3});
}

// ---- rtree.qbk (members) ----

TEST_CASE("doc: begin") {
    std::size_t n = 0;
    for (auto it = tree().begin(); it != tree().end(); ++it)
        ++n;
    CHECK(n == 9);
}
TEST_CASE("doc: end") {
    rtree empty;
    CHECK(empty.begin() == empty.end());
}
TEST_CASE("doc: qbegin-predicates-const") {
    auto it = tree().qbegin(bgi::nearest(point(0, 0), 2));
    REQUIRE(it != tree().qend());
    CHECK(it->second == 0);  // nearest first
}
TEST_CASE("doc: qend") {
    std::size_t n = std::distance(tree().qbegin(bgi::intersects(box(point(0, 0), point(5, 1)))), tree().qend());
    CHECK(n == 3);
}
TEST_CASE("doc: query-predicates-const-outiter") {
    std::vector<value> out;
    CHECK(tree().query(bgi::intersects(point(2.5, 2.5)), std::back_inserter(out)) == 1);
    CHECK(out.front().second == 4);
}

// ---- rtree_functions.qbk ----

TEST_CASE("doc: begin-rtree-const") {
    CHECK(std::distance(bgi::begin(tree()), bgi::end(tree())) == 9);
}
TEST_CASE("doc: end-rtree-const") {
    rtree empty;
    CHECK(bgi::begin(empty) == bgi::end(empty));
}
TEST_CASE("doc: qbegin-rtree-const-predicates-const") {
    auto it = bgi::qbegin(tree(), bgi::nearest(point(10, 0), 1));
    CHECK(it->second == 6);
}
TEST_CASE("doc: qend-rtree-const") {
    CHECK(std::distance(bgi::qbegin(tree(), bgi::within(box(point(-1, -1), point(10, 10)))), bgi::qend(tree())) == 9);
}
TEST_CASE("doc: query-rtree-const-predicates-const-outiter") {
    std::vector<value> out;
    CHECK(bgi::query(tree(), bgi::contains(point(4.5, 4.5)), std::back_inserter(out)) == 1);
    CHECK(out.front().second == 8);
}
