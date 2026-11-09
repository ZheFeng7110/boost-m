// mcpplibs.mwrap.boost.unordered — user types in every container family:
// user keys hashed through boost::hash (hash_value by ADL) and through user
// hasher/equality types (including stateful ones), a user hasher marked
// is_avalanching, transparent user hashers for heterogeneous lookup, a user
// allocator, and move-only / non-default-constructible mapped types; the
// node and concurrent containers holding user types.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.unordered;

namespace user {
struct point {
    int x, y;
    bool operator==(const point&) const = default;
    friend std::size_t hash_value(const point& p) {
        std::size_t seed = 0;
        boost::hash_combine(seed, p.x);
        boost::hash_combine(seed, p.y);
        return seed;
    }
};

// examples/fnv1.hpp-style user hasher (FNV-1a), stateful seed.
struct fnv1a {
    std::uint64_t basis = 0xcbf29ce484222325ull;
    std::size_t operator()(std::string_view s) const {
        std::uint64_t h = basis;
        for (unsigned char c : s) {
            h ^= c;
            h *= 0x100000001b3ull;
        }
        return std::size_t(h);
    }
};

struct mod_hash {  // terrible distribution unless post-mixed
    std::size_t operator()(int v) const { return std::size_t(v) * 1024; }
};

struct id_hash {
    using is_transparent = void;
    using is_avalanching = std::true_type;
    std::size_t operator()(std::string_view s) const { return std::hash<std::string_view>()(s); }
};

struct resource {
    std::unique_ptr<int> data;
    explicit resource(int v) : data(std::make_unique<int>(v)) {}
};

inline std::size_t allocs = 0;
template <class T>
struct counting_allocator {
    using value_type = T;
    counting_allocator() = default;
    template <class U>
    counting_allocator(const counting_allocator<U>&) noexcept {}
    T* allocate(std::size_t n) {
        ++allocs;
        return std::allocator<T>().allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
    template <class U>
    bool operator==(const counting_allocator<U>&) const noexcept { return true; }
};
}  // namespace user

TEST_CASE_TEMPLATE("user keys through boost::hash in every set family", Set, boost::unordered_set<user::point>,
                   boost::unordered_flat_set<user::point>, boost::unordered_node_set<user::point>) {
    Set s;
    s.insert({1, 2});
    s.insert({1, 2});
    s.insert({2, 1});
    CHECK(s.size() == 2);
    CHECK(s.contains({2, 1}));
}

TEST_CASE("stateful user hasher") {
    boost::unordered_map<std::string, int, user::fnv1a> m(16, user::fnv1a{12345});
    m["a"] = 1;
    CHECK(m.hash_function().basis == 12345);
    CHECK(m.at("a") == 1);
}

TEST_CASE("post-mixing rescues a poor hash; avalanching skips it") {
    boost::unordered_flat_set<int, user::mod_hash> s;
    for (int i = 0; i < 2000; ++i) s.insert(i);
    CHECK(s.size() == 2000);
    boost::unordered_flat_map<std::string, int, user::id_hash, std::equal_to<>> m{{"k", 1}};
    CHECK(m.find(std::string_view("k")) != m.end());  // transparent lookup
    CHECK(m.count("k") == 1);
}

TEST_CASE("move-only mapped types") {
    boost::unordered_node_map<int, user::resource> m;
    m.emplace(1, 10);
    m.try_emplace(2, 20);
    CHECK(*m.at(2).data == 20);
    auto nh = m.extract(1);
    CHECK(*nh.mapped().data == 10);
    boost::unordered_flat_map<int, user::resource> f;
    f.emplace(3, 30);
    CHECK(*f.at(3).data == 30);
}

TEST_CASE("user allocator") {
    user::allocs = 0;
    boost::unordered_flat_map<int, int, boost::hash<int>, std::equal_to<int>,
                              user::counting_allocator<std::pair<const int, int>>>
        m;
    for (int i = 0; i < 100; ++i) m.emplace(i, i);
    CHECK(user::allocs > 0);
}

TEST_CASE("concurrent containers of user types") {
    boost::concurrent_flat_map<user::point, std::string> m;
    m.emplace(user::point{0, 0}, "origin");
    std::vector<std::jthread> ts;
    for (int t = 0; t < 4; ++t)
        ts.emplace_back([&m, t] {
            for (int i = 0; i < 250; ++i) m.emplace(user::point{t, i + 1}, "p");
        });
    ts.clear();
    CHECK(m.size() == 1001);
    std::string name;
    m.cvisit(user::point{0, 0}, [&](const auto& kv) { name = kv.second; });
    CHECK(name == "origin");
    boost::concurrent_flat_set<user::point> s;
    s.insert({1, 1});
    CHECK(s.contains({1, 1}));
}
