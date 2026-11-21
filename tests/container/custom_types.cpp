// mcpplibs.mwrap.boost.container — user types and the containers' options:
// move-only and non-default-constructible elements, recursive containers of an
// incomplete user type, user comparators for the flat containers, a user
// allocator through allocator_traits, vector/deque/small_vector options
// (growth factor, stored size, block size, inplace alignment), and a user
// memory_resource for the pmr aliases.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.container;

namespace bc = boost::container;

namespace user {
struct widget {
    explicit widget(int v) : v(v) {}
    widget(widget&&) noexcept = default;
    widget& operator=(widget&&) noexcept = default;
    int v;
};

struct node {  // recursive: a vector of the type being defined
    std::string name;
    bc::vector<node> children;
    bc::flat_map<std::string, node> named;
};

struct by_len {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() != b.size() ? a.size() < b.size() : a < b;
    }
};

inline int allocs = 0;
template <class T>
struct tracking_allocator {
    using value_type = T;
    tracking_allocator() = default;
    template <class U>
    tracking_allocator(const tracking_allocator<U>&) noexcept {}
    T* allocate(std::size_t n) {
        ++allocs;
        return std::allocator<T>().allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
    template <class U>
    bool operator==(const tracking_allocator<U>&) const noexcept { return true; }
};

class counting_resource : public bc::pmr::memory_resource {
public:
    std::size_t bytes = 0;
private:
    void* do_allocate(std::size_t n, std::size_t align) override {
        bytes += n;
        return bc::pmr::new_delete_resource()->allocate(n, align);
    }
    void do_deallocate(void* p, std::size_t n, std::size_t align) override {
        bc::pmr::new_delete_resource()->deallocate(p, n, align);
    }
    bool do_is_equal(const bc::pmr::memory_resource& o) const noexcept override { return this == &o; }
};
}  // namespace user

TEST_CASE("move-only, non-default-constructible elements") {
    bc::vector<user::widget> v;
    v.emplace_back(1);
    v.emplace(v.begin(), 0);
    CHECK(v[0].v == 0);
    bc::stable_vector<user::widget> sv;
    sv.emplace_back(5);
    bc::devector<user::widget> d;
    d.emplace_front(7);
    CHECK(d.front().v == 7);
    bc::flat_map<int, user::widget> fm;
    fm.emplace(1, user::widget(9));
    CHECK(fm.at(1).v == 9);
}

TEST_CASE("recursive containers of an incomplete user type") {
    user::node root{"root", {}, {}};
    root.children.push_back({"a", {}, {}});
    root.children[0].children.push_back({"a1", {}, {}});
    root.named.emplace("x", user::node{"x", {}, {}});
    CHECK(root.children[0].children[0].name == "a1");
    CHECK(root.named.at("x").name == "x");
}

TEST_CASE("user comparator in flat containers") {
    bc::flat_set<std::string, user::by_len> s{"ccc", "a", "bb"};
    CHECK(*s.begin() == "a");
    CHECK(*(s.end() - 1) == "ccc");
    bc::flat_multimap<std::string, int, user::by_len> m{{"zz", 1}, {"q", 2}};
    CHECK(m.begin()->second == 2);
}

TEST_CASE("user allocator") {
    user::allocs = 0;
    bc::vector<int, user::tracking_allocator<int>> v;
    for (int i = 0; i < 100; ++i) v.push_back(i);
    CHECK(user::allocs > 0);
    bc::list<int, user::tracking_allocator<int>> l{1, 2, 3};
    CHECK(l.size() == 3);
}

TEST_CASE("container options") {
    using growth = bc::vector_options_t<bc::growth_factor<bc::growth_factor_100>, bc::stored_size<std::uint16_t>>;
    bc::vector<int, void, growth> v;
    for (int i = 0; i < 1000; ++i) v.push_back(i);
    static_assert(sizeof(v) < sizeof(bc::vector<int>));  // the size/capacity are stored as uint16_t
    CHECK(v.size() == 1000);
    using dq = bc::deque_options_t<bc::block_size<128u>>;
    bc::deque<int, void, dq> d(300, 1);
    CHECK(d.size() == 300);
    using svo = bc::small_vector_options_t<bc::inplace_alignment<16u>>;
    bc::small_vector<char, 8, void, svo> s(8, 'x');
    CHECK(reinterpret_cast<std::uintptr_t>(s.data()) % 16 == 0);
    using sto = bc::static_vector_options_t<bc::throw_on_overflow<false>>;
    bc::static_vector<int, 2, sto> st{1, 2};
    CHECK(st.size() == st.capacity());
}

TEST_CASE("a user memory_resource") {
    user::counting_resource mr;
    {
        bc::pmr::vector<bc::pmr::string> v(&mr);
        v.emplace_back(200, 'x');
        v.emplace_back(300, 'y');
        bc::pmr::flat_map<int, int> fm(&mr);
        fm.emplace(1, 1);
    }
    CHECK(mr.bytes >= 500);
    bc::pmr::polymorphic_allocator<int> a(&mr);
    CHECK(a.resource() == &mr);
}
