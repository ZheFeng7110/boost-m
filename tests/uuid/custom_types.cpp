// mcpplibs.mwrap.boost.uuid — user types at UUID's customization points:
// basic_random_generator over a user UniformRandomNumberGenerator, a user
// namespace UUID for the name generators, uuids as keys in user containers
// and members of user types (with hashing), time_generator_v1 over a user
// node id and a shared std::atomic state, and to_chars into a user buffer.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.uuid;

using namespace boost::uuids;

namespace user {
// A deterministic UniformRandomNumberGenerator (splitmix64).
class splitmix {
public:
    using result_type = std::uint64_t;
    splitmix() = default;  // basic_random_generator default-constructs its own engine member
    explicit splitmix(std::uint64_t seed) : s_(seed) {}
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return ~result_type(0); }
    result_type operator()() {
        std::uint64_t z = (s_ += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
private:
    std::uint64_t s_ = 0;
};

struct asset {
    uuid id;
    std::string name;
    bool operator==(const asset& o) const { return id == o.id; }
};
struct asset_hash {
    std::size_t operator()(const asset& a) const noexcept { return hash_value(a.id); }
};

// The application's own namespace UUID for name-based ids.
constexpr uuid app_namespace = uuid_from_string("a0b1c2d3-e4f5-4607-8899-aabbccddeeff");

struct fixed_buffer {
    char bytes[40]{};
};
}  // namespace user

TEST_CASE("basic_random_generator over a user engine") {
    user::splitmix e1(1), e2(1), e3(2);
    basic_random_generator<user::splitmix> g1(e1), g2(e2), g3(e3);
    uuid a = g1(), b = g2(), c = g3();
    CHECK(a == b);  // same seed, same uuid
    CHECK(a != c);
    CHECK(a.version() == uuid::version_random_number_based);
    CHECK(a.variant() == uuid::variant_rfc_4122);
    basic_random_generator<user::splitmix> by_ptr(&e3);
    CHECK(by_ptr() != c);  // the engine advanced
}

TEST_CASE("name generators in a user namespace") {
    name_generator_sha1 ids(user::app_namespace);
    uuid alice = ids("alice"), bob = ids("bob");
    CHECK(alice != bob);
    CHECK(ids("alice") == alice);
    name_generator_md5 legacy(user::app_namespace);
    CHECK(legacy("alice") != alice);
    CHECK(legacy("alice").version() == uuid::version_name_based_md5);
    const std::uint8_t blob[] = {1, 2, 3};
    CHECK(ids(blob, sizeof blob) == ids(blob, sizeof blob));
}

TEST_CASE("uuids in user types and containers") {
    random_generator gen;
    std::unordered_set<user::asset, user::asset_hash> assets;
    user::asset a{gen(), "a"}, b{gen(), "b"};
    assets.insert(a);
    assets.insert(b);
    assets.insert(a);
    CHECK(assets.size() == 2);
    std::map<uuid, user::asset> by_id{{a.id, a}, {b.id, b}};
    CHECK(by_id.at(b.id).name == "b");
    std::unordered_map<uuid, int> counts;
    ++counts[a.id];
    ++counts[a.id];
    CHECK(counts[a.id] == 2);
}

TEST_CASE("time_generator_v1 with a user node and a shared atomic state") {
    uuid::node_type node{0x02, 0, 0, 0, 0, 0x01};  // locally administered
    std::atomic<time_generator_v1::state_type> state{time_generator_v1::state_type{0, 0x0101}};
    time_generator_v1 g1(node, state), g2(node, state);
    uuid a = g1(), b = g2();
    CHECK(a != b);  // the shared state keeps the two generators apart
    CHECK(a.node_identifier() == node);
    CHECK(b.node_identifier() == node);
}

TEST_CASE("to_chars into a user buffer") {
    user::fixed_buffer buf;
    CHECK(to_chars(user::app_namespace, buf.bytes, buf.bytes + 36));
    CHECK(std::string(buf.bytes, 36) == "a0b1c2d3-e4f5-4607-8899-aabbccddeeff");
}
