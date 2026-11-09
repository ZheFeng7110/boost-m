// mcpplibs.mwrap.boost.bimap — the tutorial and rationale sections of the
// Boost.Bimap documentation (libs/bimap/doc/{quick_tutorial,tutorial,
// rationale}.qbk) whose code is written inline; sections that import an
// example are covered by that example's case (coverage.toml).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.bimap;

using namespace boost::bimaps;

TEST_CASE("doc: one-minute-tutorial") {
    using bm_type = bimap<int, std::string>;
    bm_type bm;
    bm.insert(bm_type::value_type(1, "one"));
    bm.insert(bm_type::value_type(2, "two"));
    bm.left.insert(bm_type::left_value_type(3, "three"));
    bm.right.insert(bm_type::right_value_type("four", 4));
    CHECK(bm.size() == 4);
    CHECK(bm.left.at(1) == "one");
    CHECK(bm.right.at("two") == 2);
    for (bm_type::const_iterator it = bm.begin(); it != bm.end(); ++it)
        CHECK(bm.left.at(it->left) == it->right);
    // The left view works like std::map<int, std::string>.
    bm_type::left_const_iterator l = bm.left.find(3);
    REQUIRE(l != bm.left.end());
    CHECK(l->second == "three");
    // Inserting a duplicate key in either view fails.
    CHECK_FALSE(bm.insert(bm_type::value_type(1, "uno")).second);
    CHECK_FALSE(bm.insert(bm_type::value_type(9, "one")).second);
}

TEST_CASE("doc: bimap-mapping-framework") {
    // A bimap has three views: left (a map from left), right (a map from
    // right) and the relation set itself.
    bimap<std::string, int> bm;
    bm.insert({"a", 1});
    CHECK(bm.left.begin()->first == "a");
    CHECK(bm.right.begin()->first == 1);
    CHECK(bm.begin()->left == "a");
    CHECK(bm.begin()->right == 1);
}

TEST_CASE("doc: freedom-of-choice") {
    // Each side picks its own collection type.
    bimap<unordered_set_of<std::string>, multiset_of<int>> a;
    a.insert({"x", 1});
    a.insert({"y", 1});  // multiset_of: repeated rights allowed
    CHECK(a.right.count(1) == 2);
    bimap<set_of<int>, list_of<std::string>> b;
    b.insert({1, "z"});
    b.insert({2, "z"});
    CHECK(b.size() == 2);
    bimap<vector_of<int>, unconstrained_set_of<int>> c;
    c.left.push_back(decltype(c)::left_value_type(7, 70));
    CHECK(c.left.size() == 1);
}

TEST_CASE("doc: complete-instantiation-scheme") {
    struct a_tag {};
    struct b_tag {};
    using bm_type = bimap<tagged<int, a_tag>, multiset_of<tagged<std::string, b_tag>>, set_of_relation<>,
                          with_info<double>>;
    bm_type bm;
    bm.insert(bm_type::value_type(1, "one", 1.0));
    CHECK(bm.by<a_tag>().at(1) == "one");
    CHECK(bm.by<b_tag>().find("one")->info == 1.0);
}

TEST_CASE("doc: bimaps-with-user-defined-names") {
    struct id {};
    struct name {};
    using people = bimap<tagged<int, id>, tagged<std::string, name>>;
    people p;
    p.insert(people::value_type(1, "ada"));
    CHECK(p.by<id>().at(1) == "ada");
    CHECK(p.by<name>().at("ada") == 1);
    people::map_by<name>::const_iterator it = p.by<name>().find("ada");
    CHECK(it->get<id>() == 1);
}

TEST_CASE("doc: insertion") {
    using bm_type = bimap<int, std::string>;
    bm_type bm;
    bm.insert(bm_type::value_type(1, "one"));
    bm.left.insert(bm_type::left_value_type(2, "two"));
    bm.right.insert(bm_type::right_value_type("three", 3));
    bm.left.insert(std::make_pair(4, "four"));
    CHECK(bm.size() == 4);
    auto r = bm.insert(bm_type::value_type(1, "uno"));
    CHECK_FALSE(r.second);
    CHECK(r.first->right == "one");
}

TEST_CASE("doc: iterator-value-type") {
    using bm_type = bimap<int, std::string>;
    bm_type bm;
    bm.insert(bm_type::value_type(1, "a"));
    // The left view's value_type has first/second, the relation left/right.
    bm_type::left_reference lr = *bm.left.begin();
    CHECK(lr.first == 1);
    CHECK(lr.second == "a");
    bm_type::right_reference rr = *bm.right.begin();
    CHECK(rr.first == "a");
    CHECK(rr.second == 1);
    bm_type::reference rel = *bm.begin();
    CHECK(rel.left == 1);
    CHECK(rel.right == "a");
    // Keys are not modifiable through a set_of view.
    static_assert(std::is_const_v<std::remove_reference_t<decltype(lr.first)>>);
}

TEST_CASE("doc: operator-and-at") {
    bimap<std::string, unordered_set_of<int>> bm;
    bm.insert({"one", 1});
    CHECK(bm.left.at("one") == 1);
    CHECK_THROWS_AS(bm.left.at("two"), std::out_of_range);
    bimap<std::string, list_of<int>> freq;  // operator[] needs a mutable data side
    freq.left["a"] = 1;
    freq.left["a"] += 2;
    CHECK(freq.left.at("a") == 3);
}

TEST_CASE("doc: general-design") {
    // The relation type is a pair-like with left/right and views as maps.
    using bm_type = bimap<char, int>;
    bm_type::value_type v('c', 3);
    CHECK(v.left == 'c');
    CHECK(v.right == 3);
    using left_pair = bm_type::left_map::value_type;
    left_pair lp('d', 4);
    CHECK(lp.first == 'd');
}

TEST_CASE("doc: the-student-and-the-mentor") {
    // The rationale's student/mentor bidirectional assignment.
    struct student {};
    struct mentor {};
    using assignments = bimap<multiset_of<tagged<std::string, mentor>>, tagged<std::string, student>>;
    assignments a;
    a.insert(assignments::value_type("dijkstra", "alice"));
    a.insert(assignments::value_type("dijkstra", "bob"));
    a.insert(assignments::value_type("knuth", "carol"));
    CHECK(a.by<mentor>().count("dijkstra") == 2);
    CHECK(a.by<student>().at("carol") == "knuth");
}
