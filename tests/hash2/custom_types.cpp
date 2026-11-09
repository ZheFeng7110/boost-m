// mcpplibs.mwrap.boost.hash2 — the library's customization points through the
// module: tag_invoke(hash_append_tag) for a user type with non-salient state
// (hashing_objects.adoc#user-defined-types), a user HashAlgorithm satisfying
// the documented requirements and used with hash_append and the result
// helpers, a user flavor (byte order and size_type), and a user hash adapter
// for unordered containers (examples.adoc#use-with-unordered-containers).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdint>
import std;
import mcpplibs.mwrap.boost.hash2;
import mcpplibs.mwrap.boost.unordered;

namespace h2 = boost::hash2;

namespace user {
class X {
    std::string a;
    int b;
    void const* c;  // not part of the salient state
public:
    X(std::string a, int b, void const* c) : a(std::move(a)), b(b), c(c) {}
    friend bool operator==(X const& x1, X const& x2) { return x1.a == x2.a && x1.b == x2.b; }
    template <class Provider, class Hash, class Flavor>
    friend void tag_invoke(boost::hash2::hash_append_tag const&, Provider const&, Hash& h, Flavor const& f,
                           X const* v) {
        boost::hash2::hash_append(h, f, v->a);
        boost::hash2::hash_append(h, f, v->b);
    }
};

// A user HashAlgorithm (djb2-xor, 32 bits) meeting the requirements.
class djb2 {
public:
    using result_type = std::uint32_t;
    static constexpr std::size_t block_size = 1;
    constexpr djb2() = default;
    explicit constexpr djb2(std::uint64_t seed) : st_(std::uint32_t(5381 ^ seed ^ (seed >> 32))) {}
    djb2(void const* seed, std::size_t n) { update(seed, n); }
    void update(void const* data, std::size_t n) {
        auto p = static_cast<unsigned char const*>(data);
        for (std::size_t i = 0; i < n; ++i) st_ = (st_ * 33) ^ p[i];
    }
    constexpr result_type result() {
        result_type r = st_;
        st_ = st_ * 33 + 1;  // further calls give further values
        return r;
    }
private:
    std::uint32_t st_ = 5381;
};

struct net_flavor {  // big-endian, 64-bit sizes
    using size_type = std::uint64_t;
    static constexpr auto byte_order = boost::hash2::endian::big;
};

// examples.adoc#use-with-unordered-containers: a std::hash-like adapter.
template <class T, class H = h2::siphash_64>
struct hash2_adapter {
    std::uint64_t seed = 0x1234;
    std::size_t operator()(T const& v) const {
        H h(seed);
        h2::hash_append(h, {}, v);
        return h2::get_integral_result<std::size_t>(h);
    }
};
}  // namespace user

TEST_CASE("doc: hashing_objects#user-defined-types") {
    int i = 0, j = 0;
    user::X a("k", 1, &i), b("k", 1, &j), c("k", 2, &i);
    auto hash = [](user::X const& x) {
        h2::sha2_256 h;
        h2::hash_append(h, {}, x);
        return h.result();
    };
    CHECK(a == b);
    CHECK(hash(a) == hash(b));  // c (non-salient) is not hashed
    CHECK(hash(a) != hash(c));
    std::vector<user::X> v{a, c};  // ranges of user types
    h2::fnv1a_32 r1, r2;
    h2::hash_append(r1, {}, v);
    h2::hash_append_range(r2, {}, v.begin(), v.end());
    h2::hash_append_size(r2, {}, v.size());
    CHECK(r1.result() == r2.result());
}

TEST_CASE("a user HashAlgorithm") {
    user::djb2 h;
    h2::hash_append(h, {}, std::string("abc"));
    user::djb2 g;
    h2::hash_append(g, {}, std::string("abc"));
    CHECK(h.result() == g.result());
    user::djb2 seeded(std::uint64_t(42));
    h2::hash_append(seeded, {}, std::string("abc"));
    CHECK(seeded.result() != user::djb2(h).result());
    CHECK(h2::get_integral_result<std::uint64_t>(g) != 0u);  // extended from 32 bits
    int k = 0;
    user::X x("x", 1, &k);
    user::djb2 hx;
    h2::hash_append(hx, {}, x);  // user algorithm x user type
    (void)hx.result();
}

TEST_CASE("a user flavor") {
    h2::fnv1a_64 a, b;
    std::vector<std::uint16_t> v{0x0102};
    h2::hash_append(a, user::net_flavor{}, v);
    const unsigned char expected[] = {0x01, 0x02, 0, 0, 0, 0, 0, 0, 0, 1};  // element, then 64-bit size
    b.update(expected, sizeof expected);
    CHECK(a.result() == b.result());
}

TEST_CASE("doc: examples#use-with-unordered-containers") {
    int k = 0;
    boost::unordered_flat_map<user::X, int, user::hash2_adapter<user::X>> m;
    m.emplace(user::X("a", 1, &k), 10);
    m.emplace(user::X("a", 1, nullptr), 20);  // equal salient state: same key
    CHECK(m.size() == 1);
    std::unordered_set<std::string, user::hash2_adapter<std::string, h2::xxhash_64>> s{"x", "y"};
    CHECK(s.count("x") == 1);
}
