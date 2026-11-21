// mcpplibs.mwrap.boost.bloom — user element types and user hashers: a user
// struct hashed through boost::hash (hash_value found by ADL), a user hasher
// marked as avalanching (is_avalanching, so the filter skips its own mixing),
// a user hasher without the mark (mixed by the filter), and a user allocator.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.bloom;
import mcpplibs.mwrap.boost.container_hash;

namespace user {
struct ip4 {
    std::uint8_t a, b, c, d;
    bool operator==(const ip4&) const = default;
    friend std::size_t hash_value(const ip4& x) {
        std::size_t seed = 0;
        boost::hash_combine(seed, (x.a << 24) | (x.b << 16) | (x.c << 8) | x.d);
        return seed;
    }
};

struct avalanching_hash {
    using is_avalanching = std::true_type;
    std::size_t operator()(std::uint64_t x) const noexcept {
        x ^= x >> 33;
        x *= 0xff51afd7ed558ccdull;
        x ^= x >> 33;
        x *= 0xc4ceb9fe1a85ec53ull;
        x ^= x >> 33;
        return std::size_t(x);
    }
};

struct identity_hash {  // poor quality: the filter post-mixes it
    std::size_t operator()(std::uint64_t x) const noexcept { return std::size_t(x); }
};

inline std::size_t bytes_allocated = 0;
template <class T>
struct counting_allocator {
    using value_type = T;
    counting_allocator() = default;
    template <class U>
    counting_allocator(const counting_allocator<U>&) noexcept {}
    T* allocate(std::size_t n) {
        bytes_allocated += n * sizeof(T);
        return std::allocator<T>().allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
    template <class U>
    bool operator==(const counting_allocator<U>&) const noexcept { return true; }
};
}  // namespace user

TEST_CASE("user struct through boost::hash") {
    boost::bloom::filter<user::ip4, 5> f(10'000, 0.01);
    f.insert({10, 0, 0, 1});
    f.insert({192, 168, 1, 1});
    CHECK(f.may_contain({10, 0, 0, 1}));
    int fp = 0;
    for (int i = 0; i < 256; ++i) fp += f.may_contain({172, 16, 0, std::uint8_t(i)});
    CHECK(fp < 20);
}

TEST_CASE("user hashers, avalanching or not") {
    boost::bloom::filter<std::uint64_t, 4, boost::bloom::block<unsigned char, 1>, 0, user::avalanching_hash> a(8'000, 0.01);
    boost::bloom::filter<std::uint64_t, 4, boost::bloom::block<unsigned char, 1>, 0, user::identity_hash> b(8'000, 0.01);
    for (std::uint64_t i = 0; i < 500; ++i) {
        a.insert(i * 4096);  // structured keys: bad for an unmixed identity hash
        b.insert(i * 4096);
    }
    int fpa = 0, fpb = 0;
    for (std::uint64_t i = 0; i < 2000; ++i) {
        fpa += a.may_contain(i * 4096 + 1);
        fpb += b.may_contain(i * 4096 + 1);
    }
    CHECK(fpa < 100);
    CHECK(fpb < 100);  // the filter mixes a non-avalanching hash itself
    CHECK(a.hash_function()(1) == user::avalanching_hash()(1));
}

TEST_CASE("user allocator") {
    user::bytes_allocated = 0;
    boost::bloom::filter<int, 2, boost::bloom::block<unsigned char, 1>, 0, boost::hash<int>,
                         user::counting_allocator<unsigned char>>
        f(80'000);
    CHECK(user::bytes_allocated >= 10'000);  // 80'000 bits
    f.insert(7);
    CHECK(f.may_contain(7));
}
