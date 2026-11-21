// mcpplibs.mwrap.boost.dynamic_bitset — the class template's customization
// points: user-chosen Block types (unsigned char, std::uint16_t, unsigned
// long long) and a user allocator (counting allocations), plus block-range
// round trips through a user container.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.dynamic_bitset;

namespace user {
inline std::size_t allocations = 0;

template <class T>
struct counting_allocator {
    using value_type = T;
    counting_allocator() = default;
    template <class U>
    counting_allocator(const counting_allocator<U>&) noexcept {}
    T* allocate(std::size_t n) {
        ++allocations;
        return std::allocator<T>().allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
    template <class U>
    bool operator==(const counting_allocator<U>&) const noexcept { return true; }
};
}  // namespace user

TEST_CASE_TEMPLATE("user Block types", Block, unsigned char, std::uint16_t, unsigned long long) {
    using bits = boost::dynamic_bitset<Block>;
    static_assert(bits::bits_per_block == std::numeric_limits<Block>::digits);
    bits b(40, 0xA5ul);
    CHECK(b.count() == 4);
    CHECK(b.num_blocks() == (40 + bits::bits_per_block - 1) / bits::bits_per_block);
    b.set(39);
    CHECK(b.find_next(7) == 39);
    std::vector<Block> blocks;
    boost::to_block_range(b, std::back_inserter(blocks));
    bits back(blocks.begin(), blocks.end());
    back.resize(40);
    CHECK(back == b);
    std::ostringstream os;
    os << bits(4, 6ul);
    CHECK(os.str() == "0110");
}

TEST_CASE("user allocator") {
    using bits = boost::dynamic_bitset<unsigned, user::counting_allocator<unsigned>>;
    user::allocations = 0;
    bits b(1000);
    CHECK(user::allocations >= 1);
    b.set(999);
    bits c = b;
    CHECK(c.test(999));
    CHECK(c.get_allocator() == user::counting_allocator<unsigned>());
    std::size_t before = user::allocations;
    c.resize(5000);
    CHECK(user::allocations > before);
}

TEST_CASE("bitsets as keys and values in user structures") {
    struct permission_set {
        std::string role;
        boost::dynamic_bitset<> perms;
    };
    std::vector<permission_set> roles{{"reader", boost::dynamic_bitset<>(4, 0b0001ul)},
                                      {"editor", boost::dynamic_bitset<>(4, 0b0011ul)},
                                      {"admin", boost::dynamic_bitset<>(4, 0b1111ul)}};
    auto at_least = [&](const boost::dynamic_bitset<>& need) {
        std::vector<std::string> out;
        for (auto& r : roles)
            if (need.is_subset_of(r.perms)) out.push_back(r.role);
        return out;
    };
    CHECK(at_least(boost::dynamic_bitset<>(4, 0b0010ul)) == std::vector<std::string>{"editor", "admin"});
    std::map<boost::dynamic_bitset<>, std::string> by_mask;
    for (auto& r : roles) by_mask[r.perms] = r.role;
    CHECK(by_mask.begin()->second == "reader");
}
