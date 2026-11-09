// mcpplibs.mwrap.boost.circular_buffer — user element types: non-trivial
// destructors run exactly once when elements are overwritten or popped,
// move-only elements, a user allocator, and a user aggregate with an ordering.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.circular_buffer;

namespace user {
inline int live = 0;
struct tracked {
    int id;
    explicit tracked(int i = 0) : id(i) { ++live; }
    tracked(const tracked& o) : id(o.id) { ++live; }
    tracked(tracked&& o) noexcept : id(o.id) { ++live; }
    tracked& operator=(const tracked&) = default;
    tracked& operator=(tracked&&) noexcept = default;
    ~tracked() { --live; }
};

template <class T>
struct tagged_allocator {
    using value_type = T;
    int tag = 0;
    tagged_allocator() = default;
    explicit tagged_allocator(int t) : tag(t) {}
    template <class U>
    tagged_allocator(const tagged_allocator<U>& o) noexcept : tag(o.tag) {}
    T* allocate(std::size_t n) { return std::allocator<T>().allocate(n); }
    void deallocate(T* p, std::size_t n) noexcept { std::allocator<T>().deallocate(p, n); }
    template <class U>
    bool operator==(const tagged_allocator<U>& o) const noexcept { return tag == o.tag; }
};

struct sample {
    double t;
    double value;
    auto operator<=>(const sample&) const = default;
};
}  // namespace user

TEST_CASE("element lifetime through overwrite, pop, clear") {
    user::live = 0;
    {
        boost::circular_buffer<user::tracked> c(3);
        for (int i = 0; i < 10; ++i) c.push_back(user::tracked(i));
        CHECK(user::live == 3);
        CHECK(c.front().id == 7);
        c.pop_front();
        CHECK(user::live == 2);
        c.push_front(user::tracked(42));
        c.rotate(c.begin() + 1);
        CHECK(c.back().id == 42);
        boost::circular_buffer<user::tracked> copy = c;
        CHECK(user::live == 6);
        copy.clear();
        CHECK(user::live == 3);
    }
    CHECK(user::live == 0);
}

TEST_CASE("move-only elements") {
    boost::circular_buffer<std::unique_ptr<int>> c(2);
    c.push_back(std::make_unique<int>(1));
    c.push_back(std::make_unique<int>(2));
    c.push_back(std::make_unique<int>(3));  // overwrites 1
    CHECK(*c.front() == 2);
    std::unique_ptr<int> taken = std::move(c.back());
    c.pop_back();
    CHECK(*taken == 3);
    boost::circular_buffer<std::unique_ptr<int>> moved = std::move(c);
    CHECK(moved.size() == 1);
}

TEST_CASE("user allocator") {
    using buf = boost::circular_buffer<int, user::tagged_allocator<int>>;
    buf c(4, user::tagged_allocator<int>(7));
    CHECK(c.get_allocator().tag == 7);
    c.push_back(1);
    buf d(c);
    CHECK(d.get_allocator() == c.get_allocator());
    boost::circular_buffer_space_optimized<int, user::tagged_allocator<int>> s(
        boost::circular_buffer_space_optimized<int, user::tagged_allocator<int>>::capacity_type(8),
        user::tagged_allocator<int>(3));
    s.push_back(5);
    CHECK(s.get_allocator().tag == 3);
}

TEST_CASE("sliding window over user samples") {
    boost::circular_buffer<user::sample> window(4);
    for (int i = 0; i < 10; ++i) window.push_back({double(i), double(i * i)});
    double mean = 0;
    for (const auto& s : window) mean += s.value;
    mean /= double(window.size());
    CHECK(mean == doctest::Approx((36 + 49 + 64 + 81) / 4.0));
    CHECK(std::is_sorted(window.begin(), window.end()));
    auto it = std::lower_bound(window.begin(), window.end(), user::sample{8, 0});
    CHECK(it->t == 8);
}
