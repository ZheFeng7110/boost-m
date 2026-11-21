// boost.unordered container statistics — pure include:
// libs/unordered/doc/modules/ROOT/pages/hash_quality.adoc#container-statistics
// needs BOOST_UNORDERED_ENABLE_STATS defined before the headers, which changes
// the containers' layout; the module is built without it, so this TU uses the
// headers alone (and does not import).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#define BOOST_UNORDERED_ENABLE_STATS
#include <string>
#include <boost/unordered/unordered_flat_map.hpp>

TEST_CASE("doc: hash_quality#container-statistics") {
    boost::unordered_flat_map<std::string, int> m;
    for (int i = 0; i < 1000; ++i) m.emplace(std::to_string(i), i);
    for (int i = 0; i < 1000; ++i) (void)m.find(std::to_string(i));
    for (int i = 1000; i < 1100; ++i) (void)m.find(std::to_string(i));
    auto stats = m.get_stats();
    // Insertions include the re-insertions of every rehash; each emplace of a
    // new key first does an unsuccessful lookup.
    CHECK(stats.insertion.count >= 1000);
    CHECK(stats.successful_lookup.count == 1000);
    CHECK(stats.unsuccessful_lookup.count == 1000 + 100);
    CHECK(stats.insertion.probe_length.average >= 1.0);
    CHECK(stats.successful_lookup.num_comparisons.average >= 1.0);
    CHECK(stats.unsuccessful_lookup.probe_length.deviation >= 0.0);
    m.reset_stats();
    CHECK(m.get_stats().insertion.count == 0);
}
