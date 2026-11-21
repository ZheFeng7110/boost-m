// mcpplibs.mwrap.boost.container_hash — the official examples
// (libs/container_hash/examples/*.cpp) as test cases. point2.cpp, which
// describes its class with BOOST_DESCRIBE_CLASS, is include_describe.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.container_hash;
import mcpplibs.mwrap.boost.unordered;

namespace library {
struct book {
    int id;
    std::string author;
    std::string title;
    book(int i, std::string const& a, std::string const& t) : id(i), author(a), title(t) {}
};
bool operator==(book const& a, book const& b) { return a.id == b.id; }
std::size_t hash_value(book const& b) {
    boost::hash<int> hasher;
    return hasher(b.id);
}
}  // namespace library

TEST_CASE("example: books") {
    library::book knife(3458, "Zane Grey", "The Hash Knife Outfit");
    library::book dandelion(1354, "Paul J. Shanley", "Hash & Dandelion Greens");
    boost::hash<library::book> book_hasher;
    CHECK(book_hasher(knife) == boost::hash<int>()(3458));
    std::unordered_set<library::book, boost::hash<library::book>> books;
    books.insert(knife);
    books.insert(library::book(2443, "Lindgren, Torgny", "Hash"));
    books.insert(library::book(1953, "Snyder, Bernadette M.",
                               "Heavenly Hash: A Tasty Mix of a Mother's Meditations"));
    CHECK(books.find(knife) != books.end());
    CHECK(books.find(dandelion) == books.end());
}

namespace ex_point {
class point {
    int x;
    int y;
public:
    point() : x(0), y(0) {}
    point(int x, int y) : x(x), y(y) {}
    bool operator==(point const& other) const { return x == other.x && y == other.y; }
    friend std::size_t hash_value(point const& p) {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.x);
        boost::hash_combine(seed, p.y);
        return seed;
    }
};
}  // namespace ex_point

TEST_CASE("example: point") {
    using ex_point::point;
    boost::hash<point> point_hasher;
    point p1(0, 0), p2(1, 2), p3(4, 1), p4 = p1;
    CHECK(point_hasher(p1) == point_hasher(p4));
    CHECK(point_hasher(p1) != point_hasher(p2));
    CHECK(point_hasher(p1) != point_hasher(p3));
}

namespace foo {
template <class T>
class custom_type {
    T value;
public:
    custom_type(T x) : value(x) {}
    std::size_t hash() const {
        boost::hash<T> hasher;
        return hasher(value);
    }
};
template <class T>
std::size_t hash_value(foo::custom_type<T> x) {
    return x.hash();
}
}  // namespace foo

TEST_CASE("example: portable") {
    foo::custom_type<int> x(1), y(2), z(1);
    boost::hash<foo::custom_type<int>> hasher;
    CHECK(hasher(x) == hasher(x));
    CHECK(hasher(x) != hasher(y));
    CHECK(hasher(x) == hasher(z));
}

template <typename A, typename B>
class my_pair {
    A value1;
    B value2;
public:
    my_pair(A const& v1, B const& v2) : value1(v1), value2(v2) {}
    bool operator==(my_pair const& other) const { return value1 == other.value1 && value2 == other.value2; }
    friend std::size_t hash_value(my_pair const& p) {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.value1);
        boost::hash_combine(seed, p.value2);
        return seed;
    }
};

TEST_CASE("example: template") {
    using pair = my_pair<int, float>;
    boost::unordered_set<pair> pair_set;
    pair_set.emplace(10, 0.5f);
    CHECK(pair_set.find(pair(10, 0.5f)) != pair_set.end());
    CHECK(pair_set.find(pair(10, 0.6f)) == pair_set.end());
}

TEST_CASE("doc: hash-combine") {
    // Order matters, and combining is equivalent to hashing the sequence.
    std::size_t a = 0, b = 0;
    boost::hash_combine(a, 1);
    boost::hash_combine(a, 2);
    boost::hash_combine(b, 2);
    boost::hash_combine(b, 1);
    CHECK(a != b);
    std::vector<int> v{1, 2};
    CHECK(boost::hash_value(v) == boost::hash_range(v.begin(), v.end()));
}

TEST_CASE("doc: hash-range") {
    std::vector<std::string> v{"a", "b", "c"};
    std::size_t seed = 0;
    boost::hash_range(seed, v.begin(), v.end());
    CHECK(seed == boost::hash_range(v.begin(), v.end()));
    std::size_t s2 = 0;
    for (auto& x : v) boost::hash_combine(s2, x);
    CHECK(s2 == seed);
    boost::hash_unordered_range(seed, v.begin(), v.end());
    std::vector<std::string> r{"c", "a", "b"};
    CHECK(boost::hash_unordered_range(v.begin(), v.end()) == boost::hash_unordered_range(r.begin(), r.end()));
}
