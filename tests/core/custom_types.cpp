// mcpplibs.mwrap.boost.core — user types at Boost.Core's customization points:
// boost::is_placeholder specialized for a user placeholder (the documentation
// example, libs/core/doc/is_placeholder.qbk, through boost::bind), a user
// fancy pointer through pointer_traits / to_address, a user allocator through
// allocator_access, empty_value compressing user empty types, invoke_swap
// finding a user swap by ADL, ref/unwrap_ref, span over a user contiguous
// container, functor<> over a user function, and demangle of user templates.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.core;
import mcpplibs.mwrap.boost.bind;

namespace user {
struct arg1_type {};
constexpr arg1_type arg1{};

int f(int x) { return x + 1; }

// A fancy pointer: wraps T* and is not a raw pointer.
template <class T>
struct fancy_ptr {
    T* p = nullptr;
    using element_type = T;
    template <class U>
    using rebind = fancy_ptr<U>;
    T& operator*() const { return *p; }
    T* operator->() const noexcept { return p; }
    static fancy_ptr pointer_to(T& r) noexcept { return {&r}; }
};

template <class T>
struct arena_allocator {
    using value_type = T;
    using pointer = fancy_ptr<T>;
    using size_type = std::uint32_t;
    arena_allocator() = default;
    template <class U>
    arena_allocator(const arena_allocator<U>&) noexcept {}
    pointer allocate(std::size_t n) { return {std::allocator<T>().allocate(n)}; }
    void deallocate(pointer p, std::size_t n) noexcept { std::allocator<T>().deallocate(p.p, n); }
    template <class U>
    bool operator==(const arena_allocator<U>&) const noexcept { return true; }
};

struct empty_hasher {
    std::size_t operator()(int v) const noexcept { return std::size_t(v) * 31; }
};

struct buffer {
    int a = 1, b = 2;
    int swaps = 0;
    friend void swap(buffer& x, buffer& y) noexcept {
        std::swap(x.a, y.a);
        ++x.swaps;
        ++y.swaps;
    }
};

class small_vec {
public:
    int* data() noexcept { return items; }
    const int* data() const noexcept { return items; }
    std::size_t size() const noexcept { return 3; }
    int* begin() { return items; }
    int* end() { return items + 3; }
private:
    int items[3] = {7, 8, 9};
};

void close_handle(int* h) noexcept { *h = -1; }

template <class T, int N>
struct tagged {};
}  // namespace user

template <>
struct boost::is_placeholder<user::arg1_type> {
    static constexpr int value = 1;
};

TEST_CASE("doc: is_placeholder#example") {
    CHECK(boost::bind(user::f, user::arg1)(-1) == 0);
    CHECK(boost::bind(user::f, user::arg1)(41) == 42);
}

TEST_CASE("pointer_traits and to_address over a user fancy pointer") {
    int v = 5;
    user::fancy_ptr<int> p{&v};
    static_assert(std::is_same_v<boost::pointer_traits<user::fancy_ptr<int>>::element_type, int>);
    static_assert(std::is_same_v<boost::pointer_traits<user::fancy_ptr<int>>::rebind_to<char>::type,
                                 user::fancy_ptr<char>>);
    CHECK(boost::to_address(p) == &v);
    CHECK(boost::pointer_traits<user::fancy_ptr<int>>::pointer_to(v).p == &v);
}

TEST_CASE("allocator_access over a user allocator") {
    using A = user::arena_allocator<std::string>;
    static_assert(std::is_same_v<boost::allocator_pointer_t<A>, user::fancy_ptr<std::string>>);
    static_assert(std::is_same_v<boost::allocator_size_type_t<A>, std::uint32_t>);
    static_assert(std::is_same_v<boost::allocator_rebind_t<A, int>, user::arena_allocator<int>>);
    static_assert(std::is_same_v<boost::allocator_void_pointer_t<A>, user::fancy_ptr<void>>);
    A a;
    auto p = boost::allocator_allocate(a, 2);
    boost::allocator_construct_n(a, boost::to_address(p), 2);
    CHECK(p.p[1].empty());
    boost::allocator_destroy_n(a, boost::to_address(p), 2);
    boost::allocator_deallocate(a, p, 2);
    CHECK(boost::allocator_max_size(a) > 0);
}

TEST_CASE("empty_value compresses user empty types") {
    struct holder : boost::empty_value<user::empty_hasher, 0>, boost::empty_value<std::equal_to<int>, 1> {
        int* storage = nullptr;
        holder() : boost::empty_value<user::empty_hasher, 0>(boost::empty_init),
                   boost::empty_value<std::equal_to<int>, 1>(boost::empty_init) {}
    };
    static_assert(sizeof(holder) == sizeof(int*));
    holder h;
    CHECK(h.boost::empty_value<user::empty_hasher, 0>::get()(2) == 62);
    static_assert(boost::use_empty_value_base<user::empty_hasher>::value);
}

TEST_CASE("invoke_swap finds the user swap by ADL") {
    user::buffer x, y;
    y.a = 9;
    boost::core::invoke_swap(x, y);
    CHECK(x.a == 9);
    CHECK(x.swaps == 1);
}

TEST_CASE("ref / unwrap_ref with user objects") {
    user::buffer b;
    auto r = boost::ref(b);
    r.get().a = 4;
    CHECK(b.a == 4);
    static_assert(boost::is_reference_wrapper<decltype(r)>::value);
    static_assert(std::is_same_v<boost::unwrap_reference<decltype(r)>::type, user::buffer>);
    CHECK(&boost::unwrap_ref(r) == &b);
    auto cr = boost::cref(b);
    CHECK(cr.get().a == 4);
}

TEST_CASE("span over a user contiguous container") {
    user::small_vec v;
    boost::span<int> s(v.data(), v.size());
    CHECK(s[1] == 8);
    boost::span<const int> cs(s);
    CHECK(std::accumulate(cs.begin(), cs.end(), 0) == 24);
    boost::span<int, 3> fixed(v.data(), 3);
    CHECK(fixed.back() == 9);
}

TEST_CASE("functor<> over a user function") {
    int h = 3;
    std::unique_ptr<int, boost::core::functor<user::close_handle>> guard(&h);
    guard.reset();
    CHECK(h == -1);
    static_assert(std::is_empty_v<boost::core::functor<user::close_handle>>);
}

TEST_CASE("demangle and type_name of user templates") {
    std::string d = boost::core::demangle(typeid(user::tagged<int, 3>).name());
    CHECK(d.find("user::tagged<int, 3>") != std::string::npos);
    CHECK(boost::core::type_name<user::tagged<long, 1>>().find("tagged") != std::string::npos);
}
