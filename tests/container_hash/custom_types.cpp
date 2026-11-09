// mcpplibs.mwrap.boost.container_hash — every way a user type becomes
// hashable by boost::hash, through the module: a hash_value overload found by
// ADL (free function and hidden friend), a specialization of boost::hash, a
// user range (begin/end) hashed element-wise and the is_range opt-out trait,
// a user tuple-like type (tuple_size/get) and the is_tuple_like trait, the
// is_unordered_range trait for a user set type, and hash_combine-based
// composition of user types inside std containers.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.container_hash;

namespace user {
struct money {
    long cents;
    bool operator==(const money&) const = default;
};
std::size_t hash_value(const money& m) { return boost::hash<long>()(m.cents); }

class account {
public:
    account(std::string id, money b) : id_(std::move(id)), balance_(b) {}
    bool operator==(const account&) const = default;
    friend std::size_t hash_value(const account& a) {
        std::size_t seed = 0;
        boost::hash_combine(seed, a.id_);
        boost::hash_combine(seed, a.balance_);
        return seed;
    }
private:
    std::string id_;
    money balance_;
};

struct special {
    int v;
};

// A user range of ints.
class ring {
public:
    explicit ring(std::vector<int> v) : v_(std::move(v)) {}
    const int* begin() const { return v_.data(); }
    const int* end() const { return v_.data() + v_.size(); }
private:
    std::vector<int> v_;
};

// A range that hashes by identity instead (opts out of element-wise hashing).
class handle_list {
public:
    const int* begin() const { return nullptr; }
    const int* end() const { return nullptr; }
    int id = 7;
    friend std::size_t hash_value(const handle_list& h) { return std::size_t(h.id) * 1000003u; }
};

// A tuple-like user type.
struct pt3 {
    int x, y, z;
};
template <std::size_t I>
int get(const pt3& p) {
    if constexpr (I == 0) return p.x;
    else if constexpr (I == 1) return p.y;
    else return p.z;
}

// A user set: element order does not affect the hash.
struct bag {
    std::vector<int> items;
    const int* begin() const { return items.data(); }
    const int* end() const { return items.data() + items.size(); }
};
}  // namespace user

template <>
struct boost::hash<user::special> {
    std::size_t operator()(const user::special& s) const noexcept { return std::size_t(s.v) ^ 0xABCDu; }
};
template <>
struct boost::container_hash::is_range<user::handle_list> : std::false_type {};
template <>
struct std::tuple_size<user::pt3> : std::integral_constant<std::size_t, 3> {};
template <>
struct boost::container_hash::is_unordered_range<user::bag> : std::true_type {};

TEST_CASE("hash_value found by ADL: free function and hidden friend") {
    boost::hash<user::money> hm;
    CHECK(hm({5}) == boost::hash<long>()(5));
    user::account a("x", {1}), b("x", {1}), c("y", {1});
    boost::hash<user::account> ha;
    CHECK(ha(a) == ha(b));
    CHECK(ha(a) != ha(c));
}

TEST_CASE("specialization of boost::hash") {
    CHECK(boost::hash<user::special>()({1}) == (1u ^ 0xABCDu));
    std::vector<user::special> v{{1}, {2}};
    std::size_t seed = 0;
    boost::hash_combine(seed, v[0]);  // hash_combine uses boost::hash<T>
    CHECK(seed != 0);
}

TEST_CASE("user ranges: element-wise, and the is_range opt-out") {
    static_assert(boost::container_hash::is_range<user::ring>::value);
    user::ring r({1, 2, 3});
    std::vector<int> same{1, 2, 3};
    CHECK(boost::hash<user::ring>()(r) == boost::hash<std::vector<int>>()(same));
    static_assert(!boost::container_hash::is_range<user::handle_list>::value);
    CHECK(boost::hash<user::handle_list>()(user::handle_list{}) == 7u * 1000003u);
}

TEST_CASE("user tuple-like types") {
    static_assert(boost::container_hash::is_tuple_like<user::pt3>::value);
    user::pt3 p{1, 2, 3};
    CHECK(boost::hash<user::pt3>()(p) == boost::hash<std::tuple<int, int, int>>()(std::tuple(1, 2, 3)));
}

TEST_CASE("user unordered ranges") {
    user::bag a{{1, 2, 3}}, b{{3, 1, 2}};
    CHECK(boost::hash<user::bag>()(a) == boost::hash<user::bag>()(b));
    std::vector<int> va{1, 2, 3}, vb{3, 1, 2};
    CHECK(boost::hash<std::vector<int>>()(va) != boost::hash<std::vector<int>>()(vb));
}

TEST_CASE("user types inside std containers") {
    std::unordered_map<user::account, int, boost::hash<user::account>> m;
    m[user::account("a", {1})] = 1;
    m[user::account("a", {1})] += 1;
    CHECK(m.size() == 1);
    using key = std::pair<user::money, std::string>;
    std::unordered_set<key, boost::hash<key>> s{{{1}, "x"}, {{1}, "y"}};
    CHECK(s.size() == 2);
}
