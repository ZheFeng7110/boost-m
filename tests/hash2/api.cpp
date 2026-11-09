// mcpplibs.mwrap.boost.hash2 — the "Hashing Bytes" and "Hashing C++ Objects"
// pages (libs/hash2/doc/hash2/{hashing_bytes,hashing_objects}.adoc): the
// HashAlgorithm requirements over every provided algorithm, update/result
// semantics, compile-time hashing, and hash_append over each kind of type.
// Described classes are mixed_describe.cpp, user types custom_types.cpp.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdint>
import std;
import mcpplibs.mwrap.boost.hash2;

namespace h2 = boost::hash2;

TEST_CASE_TEMPLATE("doc: hashing_bytes#hash-algorithm-requirements", H, h2::fnv1a_32, h2::fnv1a_64, h2::siphash_32,
                   h2::siphash_64, h2::xxhash_32, h2::xxhash_64, h2::xxh3_128, h2::md5_128, h2::sha1_160,
                   h2::sha2_256, h2::sha2_224, h2::sha2_512, h2::sha2_384, h2::sha2_512_224, h2::sha2_512_256,
                   h2::sha3_256, h2::sha3_512, h2::shake_128, h2::ripemd_160, h2::ripemd_128, h2::blake2b_512,
                   h2::blake2s_256, h2::hmac_md5_128, h2::hmac_sha2_256) {
    using R = typename H::result_type;
    H a;                                 // default construction
    H b(std::uint64_t(7));               // integral seed
    const unsigned char seed[3] = {1, 2, 3};
    H c(seed, sizeof seed);              // byte seed
    H d(a);                              // copy
    d = b;                               // assignment
    const char msg[] = "message";
    a.update(msg, 7);
    H e;
    e.update(msg, 7);
    R ra = a.result(), re = e.result();
    CHECK(ra == re);                     // deterministic
    CHECK(R(b.result()) != R(H().result()));  // the seed matters
    (void)c;
    (void)d;
}

TEST_CASE("doc: hashing_bytes#update") {
    // Splitting the message across update calls does not change the result.
    unsigned char message[6] = {'a', 'b', 'c', 'd', 'e', 'f'};
    h2::sha2_256 one, two, three;
    one.update(message, 6);
    two.update(message, 4);
    two.update(message + 4, 2);
    for (int i = 0; i < 6; ++i) three.update(&message[i], 1);
    auto r1 = one.result();
    CHECK(r1 == two.result());
    CHECK(r1 == three.result());
}

TEST_CASE("doc: hashing_bytes#result") {
    // result() may be called repeatedly: each call continues a pseudo-random
    // sequence (the documented random() helper).
    static h2::fnv1a_64 hash;
    auto random = [] { return hash.result(); };
    std::uint64_t a = random(), b = random();
    CHECK(a != b);
    h2::fnv1a_64 h;
    h.update("x", 1);
    CHECK(h2::get_integral_result<std::uint32_t>(h) != 0u);
    h2::md5_128 m;
    CHECK(h2::to_string(m.result()) == "d41d8cd98f00b204e9800998ecf8427e");  // MD5("")
}

TEST_CASE("doc: hashing_bytes#compile-time-hashing") {
    constexpr auto r = [] {
        h2::fnv1a_32 h;
        constexpr unsigned char data[] = {'a', 'b', 'c'};
        h.update(data, 3);
        return h.result();
    }();
    static_assert(r == 0x1A47E90Bu);  // FNV-1a 32 of "abc"
    h2::fnv1a_32 rt;
    rt.update("abc", 3);
    CHECK(rt.result() == r);
}

TEST_CASE("doc: hashing_objects#hashing_objects") {
    // Flavors: the byte order and size_type used for scalars and sizes.
    static_assert(std::is_same_v<h2::default_flavor::size_type, std::uint32_t>);
    static_assert(h2::little_endian_flavor::byte_order == h2::endian::little);
    static_assert(h2::big_endian_flavor::byte_order == h2::endian::big);
    h2::fnv1a_32 a, b;
    h2::hash_append(a, h2::big_endian_flavor(), std::uint16_t(0x0102));
    const unsigned char bytes[] = {0x01, 0x02};
    b.update(bytes, 2);
    CHECK(a.result() == b.result());
}

TEST_CASE("doc: hashing_objects#integral-types") {
    h2::fnv1a_32 h1;
    std::uint32_t v1 = 0x01020304;
    h2::hash_append(h1, h2::little_endian_flavor(), v1);
    h2::fnv1a_32 h2_;
    unsigned char v2[] = {0x04, 0x03, 0x02, 0x01};
    h2_.update(v2, sizeof(v2));
    CHECK(h1.result() == h2_.result());
}

TEST_CASE("doc: hashing_objects#floating-point-types") {
    h2::fnv1a_32 h1, h2_;
    h2::hash_append(h1, {}, 3.14f);
    h2::hash_append(h2_, {}, std::uint32_t(0x4048f5c3));
    CHECK(h1.result() == h2_.result());
    h2::fnv1a_32 p, n;
    h2::hash_append(p, {}, +0.0);
    h2::hash_append(n, {}, -0.0);
    CHECK(p.result() == n.result());  // +0 and -0 hash equal
}

namespace {
enum E : int { v1 = 123 };
int x1 = 0;
}  // namespace

TEST_CASE("doc: hashing_objects#enumeration-types") {
    h2::fnv1a_32 h1, h2_;
    h2::hash_append(h1, {}, v1);
    h2::hash_append(h2_, {}, 123);
    CHECK(h1.result() == h2_.result());
}

TEST_CASE("doc: hashing_objects#pointers") {
    h2::fnv1a_32 h1, h2_;
    h2::hash_append(h1, {}, &x1);
    h2::hash_append(h2_, {}, reinterpret_cast<std::uintptr_t>(&x1));
    CHECK(h1.result() == h2_.result());
}

// result() advances the algorithm (repeated calls give a pseudo-random
// sequence), so each result is taken once. The documentation's arrays and
// ranges examples call h1.result() twice and their second assert does not
// hold, with the headers as through the module.
TEST_CASE("doc: hashing_objects#arrays") {
    h2::fnv1a_32 h1, h2_, h3;
    int v[4] = {1, 2, 3, 4};
    h2::hash_append(h1, {}, v);
    h2::hash_append_range(h2_, {}, v + 0, v + 4);
    for (int x : v) h2::hash_append(h3, {}, x);
    auto r1 = h1.result();
    CHECK(r1 == h2_.result());
    CHECK(r1 == h3.result());
}

TEST_CASE("doc: hashing_objects#ranges") {
    h2::fnv1a_32 h1, h2_, h3, h4;
    std::vector<int> v1 = {1, 2, 3, 4};
    std::list<int> v2 = {1, 2, 3, 4};
    h2::hash_append(h1, {}, v1);
    h2::hash_append(h2_, {}, v2);
    h2::hash_append_range(h3, {}, v1.data(), v1.data() + v1.size());
    h2::hash_append_size(h3, {}, v1.size());
    h2::hash_append_range(h4, {}, v2.begin(), v2.end());
    h2::hash_append_size(h4, {}, std::distance(v2.begin(), v2.end()));
    auto r1 = h1.result(), r2 = h2_.result();
    CHECK(r1 == r2);
    CHECK(r1 == h3.result());
    CHECK(r2 == h4.result());
    // Unordered ranges hash order-independently.
    std::unordered_set<int> s1{1, 2, 3}, s2{3, 2, 1};
    h2::fnv1a_32 u1, u2;
    h2::hash_append(u1, {}, s1);
    h2::hash_append(u2, {}, s2);
    CHECK(u1.result() == u2.result());
    std::string str = "abc";
    h2::fnv1a_32 st1, st2;
    h2::hash_append(st1, {}, str);
    h2::hash_append(st2, {}, std::string_view(str));
    CHECK(st1.result() == st2.result());
}

TEST_CASE("doc: hashing_objects#tuples") {
    h2::fnv1a_32 h1, h2_;
    std::tuple<int, int, int> v1 = {1, 2, 3};
    h2::hash_append(h1, {}, v1);
    h2::hash_append(h2_, {}, std::get<0>(v1));
    h2::hash_append(h2_, {}, std::get<1>(v1));
    h2::hash_append(h2_, {}, std::get<2>(v1));
    CHECK(h1.result() == h2_.result());
    h2::fnv1a_32 p1, p2;
    h2::hash_append(p1, {}, std::pair(1, 2));
    h2::hash_append(p2, {}, std::array<int, 2>{1, 2});
    CHECK(p1.result() == p2.result());
}

TEST_CASE("hash_append_unordered_range") {
    std::vector<int> v{3, 1, 2};
    h2::fnv1a_64 a, b;
    h2::hash_append_unordered_range(a, {}, v.begin(), v.end());
    std::sort(v.begin(), v.end());
    h2::hash_append_unordered_range(b, {}, v.begin(), v.end());
    CHECK(a.result() == b.result());
}
