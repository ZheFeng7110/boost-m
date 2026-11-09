// mcpplibs.mwrap.boost.unordered — the documentation's code sections
// (libs/unordered/doc/modules/ROOT/pages/*.adoc): the regular containers
// (closed addressing, open addressing flat/node), iterator invalidation and
// reserve/rehash, piecewise emplacement, the concurrent containers'
// visitation API, bulk visitation and interoperability with the
// non-concurrent containers, and erase_if / merge / extract.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.unordered;
import mcpplibs.mwrap.boost.tuple;

TEST_CASE("doc: regular#iterator-invalidation") {
    using map = boost::unordered_map<std::string, int>;
    map x;
    x["one"] = 1;
    x["two"] = 2;
    x["three"] = 3;
    CHECK(x.at("one") == 1);
    CHECK(x.find("missing") == x.end());
    int sum = 0;
    for (const map::value_type& i : x) sum += i.second;
    CHECK(sum == 6);
    std::size_t n = 1000;
    x.reserve(n);
    CHECK(x.bucket_count() >= std::size_t(std::ceil(n / x.max_load_factor())));
    auto it = x.find("two");
    for (std::size_t i = 0; i < 500; ++i) x.emplace(std::to_string(i), int(i));  // no rehash: it stays valid
    CHECK(it->second == 2);
    x.rehash(std::size_t(std::ceil(4000 / x.max_load_factor())));
    CHECK(x.at("two") == 2);
}

TEST_CASE("doc: structures#closed-addressing-containers") {
    boost::unordered_set<int> s{1, 2, 3};
    CHECK(s.bucket_count() > 0);
    CHECK(s.bucket_size(s.bucket(1)) >= 1);
    auto b = s.bucket(2);
    CHECK(std::find(s.begin(b), s.end(b), 2) != s.end(b));
    boost::unordered_multimap<int, char> mm{{1, 'a'}, {1, 'b'}};
    CHECK(mm.count(1) == 2);
    boost::unordered_flat_set<int> fs{1, 2};
    boost::unordered_node_map<int, std::string> nm{{1, "x"}};
    auto node = nm.extract(1);
    CHECK(node.key() == 1);
    CHECK(nm.empty());
    nm.insert(std::move(node));
    CHECK(nm.size() == 1);
    CHECK(fs.contains(2));
}

TEST_CASE("doc: compliance#piecewise-pair-emplacement") {
    boost::unordered_multimap<std::string, std::complex<double>> x;
    x.emplace(std::piecewise_construct, std::make_tuple("key"), std::make_tuple(1, 2));
    x.emplace(boost::unordered::piecewise_construct, boost::make_tuple("key"), boost::make_tuple(1, 2));
    CHECK(x.count("key") == 2);
    CHECK(x.find("key")->second == std::complex<double>(1, 2));
}

TEST_CASE("doc: compliance#concurrent-containers") {
    // No iterators: lookups return counts and act through visitation.
    boost::concurrent_flat_map<int, int> m;
    CHECK(m.insert({1, 10}));
    CHECK_FALSE(m.insert({1, 11}));
    CHECK(m.visit(1, [](auto& x) { x.second += 1; }) == 1);
    CHECK(m.insert_or_visit({1, 0}, [](auto& y) { ++y.second; }) == false);
    int v = 0;
    m.cvisit(1, [&](const auto& x) { v = x.second; });
    CHECK(v == 12);
}

TEST_CASE("doc: concurrent#visitation-based-api") {
    boost::concurrent_flat_map<int, int> m;
    std::vector<int> input(10000);
    std::iota(input.begin(), input.end(), 0);
    auto calculation = [](int x) { return x * 2; };
    const int num_threads = 8;
    std::vector<std::jthread> threads;
    std::size_t chunk = input.size() / num_threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i] {
            std::size_t start = i * chunk;
            std::size_t end = (i == num_threads - 1) ? input.size() : (i + 1) * chunk;
            for (std::size_t n = start; n < end; ++n) m.emplace(input[n], calculation(input[n]));
        });
    }
    threads.clear();  // join
    CHECK(m.size() == input.size());
    std::optional<std::pair<const int, int>> o;
    bool found = m.visit(42, [&](const auto& x) { o.emplace(x); });  // the pair has a const key: emplace, not =
    CHECK(found);
    CHECK(o->second == 84);
    boost::concurrent_flat_map<int, int> m2;
    m.visit(7, [&](const auto& x) {
        if (x.second == 14) m2.insert(x);  // a different container: allowed
    });
    CHECK(m2.size() == 1);
    std::ostringstream log;
    m.insert_and_cvisit(
        std::pair{1, 1}, [&](const auto& y) { log << "(" << y.first << ", " << y.second << ") inserted\n"; },
        [&](const auto& y) { log << "(" << y.first << ", " << y.second << ") already exists\n"; });
    CHECK(log.str() == "(1, 2) already exists\n");
}

TEST_CASE("doc: concurrent#whole-table-visitation") {
    boost::concurrent_flat_map<int, int> m;
    for (int i = 0; i < 100; ++i) m.emplace(i, i);
    m.visit_all([](auto& x) { x.second = 0; });
    int sum = 0;
    m.cvisit_all([&](const auto& x) { sum += x.second; });
    CHECK(sum == 0);
    int key = 0, value = 0;
    m.visit(5, [](auto& x) { x.second = 77; });
    m.cvisit_while([&](const auto& x) {
        if (x.second == 77) {
            key = x.first;
            value = x.second;
            return false;  // stop
        }
        return true;
    });
    CHECK(key == 5);
    CHECK(value == 77);
    CHECK(m.erase_if([](auto& x) { return x.first >= 50; }) == 50);
}

TEST_CASE("doc: concurrent#bulk-visitation") {
    boost::concurrent_flat_map<int, int> m;
    for (int i = 0; i < 1000; ++i) m.emplace(i, i * i);
    std::vector<int> keys{3, 5, 2000, 7};
    int found = 0;
    m.visit(keys.begin(), keys.end(), [&](const auto&) { ++found; });
    CHECK(found == 3);
}

TEST_CASE("doc: concurrent#interoperability-with-non-concurrent-containers") {
    boost::concurrent_flat_map<int, int> cm;
    cm.emplace(1, 1);
    boost::unordered_flat_map<int, int> m(std::move(cm));  // O(1) move
    CHECK(m.size() == 1);
    CHECK(cm.empty());
    m.emplace(2, 2);
    boost::concurrent_flat_map<int, int> back(std::move(m));
    CHECK(back.size() == 2);
}

TEST_CASE("doc: hash_equality#hash_equality") {
    struct point {
        int x;
        int y;
        bool operator==(point const&) const = default;
    };
    struct point_hash {
        std::size_t operator()(point const& p) const {
            std::size_t seed = 0;
            boost::hash_combine(seed, p.x);
            boost::hash_combine(seed, p.y);
            return seed;
        }
    };
    boost::unordered_multiset<point, point_hash> points;
    points.insert({1, 2});
    points.insert({1, 2});
    CHECK(points.count({1, 2}) == 2);
}

TEST_CASE("doc: hash_quality#hash-post-mixing-and-the-avalanching-property") {
    struct my_string_hash_function {
        using is_avalanching = std::true_type;  // no post-mixing
        std::size_t operator()(const std::string& x) const { return boost::hash<std::string>()(x); }
    };
    boost::unordered_flat_map<std::string, int, my_string_hash_function> m{{"a", 1}};
    CHECK(m.at("a") == 1);
    static_assert(boost::unordered::hash_is_avalanching<my_string_hash_function>::value);
    static_assert(!boost::unordered::hash_is_avalanching<std::hash<int>>::value);
}

namespace {
struct sv_hash {
    using is_transparent = void;
    std::size_t operator()(std::string_view s) const { return boost::hash<std::string_view>()(s); }
};
}  // namespace

TEST_CASE("erase_if, merge, transparent lookup") {
    boost::unordered_flat_map<std::string, int, sv_hash, std::equal_to<>> m{{"a", 1}, {"bb", 2}};
    CHECK(m.find(std::string_view("bb"))->second == 2);  // heterogeneous lookup
    CHECK(boost::unordered::erase_if(m, [](auto& p) { return p.second > 1; }) == 1);
    boost::unordered_map<int, int> a{{1, 1}}, b{{2, 2}, {1, 9}};
    a.merge(b);
    CHECK(a.size() == 2);
    CHECK(b.size() == 1);  // the duplicate key stays in b
}
