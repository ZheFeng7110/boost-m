// mcpplibs.mwrap.boost.core — the documented components of Boost.Core
// (libs/core/doc/*.qbk) that are not macros: addressof, allocator_access,
// bit, checked_delete, default_allocator, demangle, empty_value, enable_if,
// exchange, fclose_deleter, first_scalar, identity, ignore_unused, is_same,
// noinit_adaptor, noncopyable, null_deleter, nvp / serialization helpers,
// pointer_in_range, pointer_traits, snprintf, span, typeinfo,
// uncaught_exceptions, use_default, verbose_terminate_handler. The macro
// components are include_macros.cpp. Cases are "doc: <page>#<section>".
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
import std;
import mcpplibs.mwrap.boost.core;

namespace {
struct useless_type {};
class nonaddressable {
public:
    useless_type operator&() const { return {}; }
};
template <class T>
struct X {};
}  // namespace

TEST_CASE("doc: addressof#example") {
    nonaddressable x;
    nonaddressable* xp = boost::addressof(x);
    CHECK(static_cast<void*>(xp) == static_cast<void*>(std::addressof(x)));
    constexpr static int ci = 3;
    static_assert(boost::addressof(ci) == &ci);
}

namespace {
template <class T, class A = boost::default_allocator<T>>
class container : boost::empty_value<typename boost::allocator_rebind<A, T>::type> {
public:
    using value_type = T;
    using allocator_type = A;
    using size_type = typename boost::allocator_size_type<A>::type;
    using difference_type = typename boost::allocator_difference_type<A>::type;
    using pointer = typename boost::allocator_pointer<A>::type;
    using const_pointer = typename boost::allocator_const_pointer<A>::type;
    const A& alloc() const { return this->get(); }
};

template <class A>
auto create(A& a, std::size_t n) {
    auto p = a.allocate(n);
    try {
        boost::allocator_construct_n(a, boost::to_address(p), n);
    } catch (...) {
        a.deallocate(p, n);
        throw;
    }
    return p;
}
}  // namespace

TEST_CASE("doc: allocator_access#examples") {
    using C = container<int>;
    static_assert(std::is_same_v<C::size_type, std::size_t>);
    static_assert(std::is_same_v<C::pointer, int*>);
    static_assert(std::is_same_v<C::const_pointer, const int*>);
    static_assert(sizeof(C) == 1);  // the empty allocator takes no space
    std::allocator<std::string> a;
    auto p = create(a, 3);
    CHECK(p[2].empty());
    boost::allocator_destroy_n(a, p, 3);
    boost::allocator_deallocate(a, p, 3);
    static_assert(std::is_same_v<boost::allocator_value_type_t<std::allocator<long>>, long>);
    static_assert(std::is_same_v<boost::allocator_rebind_t<std::allocator<long>, char>, std::allocator<char>>);
    static_assert(boost::allocator_is_always_equal_t<std::allocator<int>>::value);
    CHECK(boost::allocator_max_size(a) > 0);
    auto q = boost::allocator_allocate(a, 1);
    boost::allocator_construct(a, q, "x");
    CHECK(*q == "x");
    boost::allocator_destroy(a, q);
    boost::allocator_deallocate(a, q, 1);
    auto copy = boost::allocator_select_on_container_copy_construction(a);
    CHECK(copy == a);
}

TEST_CASE("doc: bit#endian") {
    using boost::core::endian;
    CHECK(endian::little != endian::big);
    CHECK((endian::native == endian::little || endian::native == endian::big));
    static_assert(boost::core::popcount(0xF0u) == 4);
    static_assert(boost::core::countl_zero(std::uint8_t(1)) == 7);
    static_assert(boost::core::countr_zero(8u) == 3);
    static_assert(boost::core::countl_one(std::uint8_t(0xF0)) == 4);
    static_assert(boost::core::countr_one(7u) == 3);
    static_assert(boost::core::bit_width(5u) == 3);
    static_assert(boost::core::bit_ceil(5u) == 8);
    static_assert(boost::core::bit_floor(5u) == 4);
    static_assert(boost::core::has_single_bit(64u));
    static_assert(boost::core::rotl(std::uint8_t(0x81), 1) == 0x03);
    static_assert(boost::core::rotr(std::uint8_t(0x81), 1) == 0xC0);
    CHECK(boost::core::byteswap(std::uint32_t(0x11223344)) == 0x44332211u);
    CHECK(boost::core::bit_cast<std::uint32_t>(1.0f) == 0x3F800000u);
}

namespace {
inline int deleted = 0;
struct counted {
    ~counted() { ++deleted; }
};
}  // namespace

TEST_CASE("doc: checked_delete#checked-deleter") {
    deleted = 0;
    boost::checked_delete(new counted);
    boost::checked_deleter<counted>()(new counted);
    CHECK(deleted == 2);
    std::unique_ptr<counted, boost::checked_deleter<counted>> p(new counted);
    p.reset();
    CHECK(deleted == 3);
}

TEST_CASE("doc: checked_delete#checked-array-deleter") {
    deleted = 0;
    boost::checked_array_delete(new counted[2]);
    boost::checked_array_deleter<counted>()(new counted[3]);
    CHECK(deleted == 5);
}

TEST_CASE("doc: default_allocator#examples") {
    std::vector<int, boost::default_allocator<int>> v{1, 2, 3};
    CHECK(v.size() == 3);
    boost::default_allocator<int> a;
    boost::default_allocator<long> b(a);
    CHECK(a == b);
    CHECK_FALSE(a != b);
    static_assert(std::is_same_v<boost::default_allocator<int>::rebind<char>::other,
                                 boost::default_allocator<char>>);
    auto sp = std::allocate_shared<std::string>(boost::default_allocator<std::string>(), "hi");
    CHECK(*sp == "hi");
}

TEST_CASE("doc: demangle#example") {
    char const* name = typeid(X<int>).name();
    std::string d = boost::core::demangle(name);
    CHECK(d.find("X<int>") != std::string::npos);
}

TEST_CASE("doc: demangle#example-2") {
    char const* name = typeid(X<int>).name();
    boost::core::scoped_demangled_name demangled(name);
    REQUIRE(demangled.get() != nullptr);
    CHECK(std::string(demangled.get()).find("X<int>") != std::string::npos);
    const char* raw = boost::core::demangle_alloc(name);
    CHECK(raw != nullptr);
    boost::core::demangle_free(raw);
}

namespace {
template <class Ptr, class Compare, class Allocator>
class storage : boost::empty_value<Compare, 0>, boost::empty_value<Allocator, 1> {
public:
    storage() : boost::empty_value<Compare, 0>(boost::empty_init_t()),
                boost::empty_value<Allocator, 1>(boost::empty_init_t()), ptr_() {}
    storage(const Compare& c, const Allocator& a)
        : boost::empty_value<Compare, 0>(boost::empty_init_t(), c),
          boost::empty_value<Allocator, 1>(boost::empty_init_t(), a), ptr_() {}
    const Ptr& pointer() const { return ptr_; }
    Ptr& pointer() { return ptr_; }
    const Compare& compare() const { return boost::empty_value<Compare, 0>::get(); }
    const Allocator& allocator() const { return boost::empty_value<Allocator, 1>::get(); }
private:
    Ptr ptr_;
};
}  // namespace

TEST_CASE("doc: empty_value#examples") {
    using S = storage<int*, std::less<int>, std::allocator<int>>;
#if !defined(_MSC_VER)  // the MSVC ABI compresses one empty base only (no __declspec(empty_bases))
    static_assert(sizeof(S) == sizeof(int*));  // both empty bases are compressed
#endif
    S s;
    CHECK(s.pointer() == nullptr);
    CHECK(s.compare()(1, 2));
    S s2{std::less<int>(), std::allocator<int>()};
    CHECK(s2.allocator() == std::allocator<int>());
    boost::empty_value<int> nonempty(boost::empty_init, 7);
    CHECK(nonempty.get() == 7);
}

// ---- enable_if -------------------------------------------------------------

namespace ei {
template <class T>
typename boost::enable_if_c<std::is_arithmetic_v<T>, T>::type foo(T t) { return t; }

template <class T>
typename boost::enable_if<std::is_integral<T>, const char*>::type kind(T) { return "integral"; }
template <class T>
typename boost::disable_if<std::is_integral<T>, const char*>::type kind(T) { return "other"; }

template <class T, class Enable = void>
struct A {
    static constexpr int v = 0;
};
template <class T>
struct A<T, typename boost::enable_if<std::is_integral<T>>::type> {
    static constexpr int v = 1;
};
template <class T>
struct A<T, typename boost::enable_if<std::is_floating_point<T>>::type> {
    static constexpr int v = 2;
};

// Lazy: the result type is only computed when the condition holds.
template <class T>
struct mult_traits {
    using type = typename T::result;
};
struct has_result {
    using result = long;
};
template <class T>
typename boost::lazy_enable_if<std::is_class<T>, mult_traits<T>>::type lazy(T) { return 1; }
inline int lazy(int) { return 2; }

template <class T, class = typename boost::enable_if_c<sizeof(T) == 1>::type>
constexpr bool one_byte(T) { return true; }

template <class T>
typename boost::enable_if_has_type<typename T::value_type, int>::type has_value_type(const T&) { return 1; }
inline int has_value_type(...) { return 0; }
}  // namespace ei

TEST_CASE("doc: enable_if#background") { CHECK(ei::foo(1.5) == 1.5); }
TEST_CASE("doc: enable_if#the-enable-if-templates") {
    static_assert(std::is_same_v<boost::enable_if_c<true, int>::type, int>);
    static_assert(std::is_same_v<boost::disable_if_c<false>::type, void>);
    CHECK(std::string(ei::kind(1)) == "integral");
    CHECK(std::string(ei::kind(1.0)) == "other");
}
TEST_CASE("doc: enable_if#using_enable_if") { CHECK(ei::foo(3) == 3); }
TEST_CASE("doc: enable_if#enable_if_0x") { static_assert(ei::one_byte('a')); }
TEST_CASE("doc: enable_if#enabling-template-class-specializations") {
    static_assert(ei::A<std::string>::v == 0);
    static_assert(ei::A<int>::v == 1);
    static_assert(ei::A<double>::v == 2);
}
TEST_CASE("doc: enable_if#overlapping-enabler-conditions") {
    // integral and floating point are disjoint, so the two partial
    // specializations above never overlap; kind() uses enable/disable pairs.
    CHECK(std::string(ei::kind('c')) == "integral");
}
TEST_CASE("doc: enable_if#enable_if_lazy") {
    CHECK(ei::lazy(ei::has_result{}) == 1);
    CHECK(ei::lazy(0) == 2);  // mult_traits<int> is never instantiated
    static_assert(std::is_same_v<boost::lazy_disable_if_c<false, ei::mult_traits<ei::has_result>>::type, long>);
}
TEST_CASE("doc: enable_if#compiler-workarounds") {
    CHECK(ei::has_value_type(std::vector<int>{}) == 1);
    CHECK(ei::has_value_type(3) == 0);
}

// ---- exchange --------------------------------------------------------------

namespace {
struct Node {
    int* head_;
    int* tail_;
    Node(int* h, int* t) : head_(h), tail_(t) {}
    Node(Node&& other)
        : head_(boost::exchange(other.head_, nullptr)), tail_(boost::exchange(other.tail_, nullptr)) {}
};
}  // namespace

TEST_CASE("doc: exchange#examples") {
    int a = 0, b = 0;
    Node n(&a, &b);
    Node m(std::move(n));
    CHECK(m.head_ == &a);
    CHECK(n.head_ == nullptr);
    CHECK(n.tail_ == nullptr);
}
TEST_CASE("doc: exchange#functions") {
    std::string s = "old";
    CHECK(boost::exchange(s, "new") == "old");
    CHECK(s == "new");
    constexpr int r = [] {
        int v = 1;
        boost::exchange(v, 2);
        return v;
    }();
    static_assert(r == 2);
}

TEST_CASE("doc: fclose_deleter#example") {
    auto path = std::filesystem::temp_directory_path() / "boost-m-core-fclose.txt";
    {
        std::unique_ptr<std::FILE, boost::fclose_deleter> f{std::fopen(path.string().c_str(), "w")};
        REQUIRE(f);
        std::fputs("x", f.get());
    }  // fclose'd here
    CHECK(std::filesystem::file_size(path) == 1);
    std::filesystem::remove(path);
}

TEST_CASE("doc: first_scalar#examples") {
    std::allocator<int> allocator;
    std::allocator_traits<std::allocator<int>>::rebind_alloc<int[2][3]> other(allocator);
    auto ptr = other.allocate(4);
    boost::alloc_construct_n(other, boost::first_scalar(boost::to_address(ptr)), 24);
    int* first = boost::first_scalar(ptr);
    CHECK(first == &ptr[0][0][0]);
    CHECK(std::all_of(first, first + 24, [](int v) { return v == 0; }));
    boost::alloc_destroy_n(other, first, 24);
    other.deallocate(ptr, 4);
}

namespace {
template <class Range, class Projection = boost::identity>
std::string print(Range&& range, Projection projection = {}) {
    std::ostringstream os;
    for (auto&& x : range) os << projection(x) << ' ';
    return os.str();
}
}  // namespace

TEST_CASE("doc: identity#example") {
    std::vector<int> v{1, 2};
    CHECK(print(v) == "1 2 ");
    CHECK(print(v, [](int x) { return x * 10; }) == "10 20 ");
    int i = 4;
    static_assert(std::is_same_v<decltype(boost::identity()(i)), int&>);
}

namespace {
int fun(int foo, int bar) {
    boost::ignore_unused(bar);
    return foo + 2;
}
}  // namespace

TEST_CASE("doc: ignore_unused#usage") {
    int v1 = 0;
    double v2 = 0;
    boost::ignore_unused(v1, v2);
    boost::ignore_unused<int, double>();
}
TEST_CASE("doc: ignore_unused#example") { CHECK(fun(1, 2) == 3); }

TEST_CASE("doc: is_same#example") {
    struct Y {
        using type = int&;
    };
    static_assert(boost::core::is_same<Y::type, int&>::value);
    static_assert(!boost::core::is_same<int, long>::value);
}

TEST_CASE("doc: noinit_adaptor#examples") {
    std::vector<int, boost::noinit_adaptor<std::allocator<int>>> v(5);
    std::iota(v.begin(), v.end(), 1);
    CHECK(v.back() == 5);
    auto sp = std::allocate_shared<int[]>(boost::noinit_adapt(std::allocator<int>()), 4);
    sp[3] = 9;
    CHECK(sp[3] == 9);
}

namespace {
class NC : private boost::noncopyable {};
}  // namespace

TEST_CASE("doc: noncopyable#example") {
    static_assert(!std::is_copy_constructible_v<NC>);
    static_assert(!std::is_copy_assignable_v<NC>);
    static_assert(std::is_default_constructible_v<NC>);
}

TEST_CASE("doc: null_deleter#example") {
    std::ostringstream os;
    {
        std::shared_ptr<std::ostream> s(&os, boost::null_deleter());
        *s << "kept";
    }
    CHECK(os.str() == "kept");  // the stream was not deleted
}

namespace {
struct recorder {
    std::vector<std::string> names;
    template <class T>
    recorder& operator&(const boost::nvp<T>& p) {
        names.push_back(p.name());
        return *this;
    }
};
struct pt {
    int x_ = 1, y_ = 2;
    template <class A>
    void serialize(A& archive, unsigned) {
        archive & boost::make_nvp("x", x_) & boost::make_nvp("y", y_);
    }
};
}  // namespace

TEST_CASE("doc: nvp#examples") {
    recorder r;
    pt p;
    p.serialize(r, 0);
    CHECK(r.names == std::vector<std::string>{"x", "y"});
    int v = 5;
    auto n = boost::make_nvp("v", v);
    CHECK(n.value() == 5);
    n.value() = 6;
    CHECK(v == 6);
}

TEST_CASE("doc: serialization#core-version-type") {
    boost::serialization::core_version_type ver(3);
    unsigned int u = ver;
    CHECK(u == 3);
    CHECK(ver.version_ == 3);
}

TEST_CASE("doc: pointer_in_range#examples") {
    int buffer[8];
    CHECK(boost::pointer_in_range(buffer + 3, buffer, buffer + 8));
    CHECK_FALSE(boost::pointer_in_range(buffer + 8, buffer, buffer + 8));
    int other = 0;
    CHECK_FALSE(boost::pointer_in_range(&other, buffer, buffer + 8));
}

TEST_CASE("doc: pointer_traits#examples") {
    std::allocator<std::string> a;
    auto p = a.allocate(1);
    std::allocator_traits<std::allocator<std::string>>::construct(a, boost::to_address(p), "s");
    CHECK(*p == "s");
    std::destroy_at(p);
    a.deallocate(p, 1);
    static_assert(std::is_same_v<boost::pointer_traits<int*>::element_type, int>);
    static_assert(std::is_same_v<boost::pointer_traits<int*>::rebind_to<char>::type, char*>);
    auto up = std::make_unique<int>(4);
    CHECK(boost::to_address(up.get()) == up.get());
}

TEST_CASE("doc: snprintf#example") {
    char buf[10];
    int i = 12345;
    int n = boost::core::snprintf(buf, sizeof(buf), "%d", i);
    CHECK(n == 5);
    CHECK(std::string(buf) == "12345");
    n = boost::core::snprintf(buf, sizeof(buf), "%d-%d", i, i);
    CHECK(n == 11);  // would overflow: the returned length exceeds the buffer
    CHECK(std::strlen(buf) == 9);
}

namespace {
// The documentation's sha1 shape, with a toy digest.
auto digest(boost::span<const unsigned char> input, boost::span<unsigned char, 4> output) {
    std::fill(output.begin(), output.end(), 0);
    for (std::size_t i = 0; i < input.size(); ++i) output[i % 4] ^= input[i];
    return output;
}
}  // namespace

TEST_CASE("doc: span#examples") {
    const unsigned char data[] = {1, 2, 3, 4, 5};
    unsigned char out[4];
    auto r = digest(data, out);
    CHECK(r.size() == 4);
    CHECK(out[0] == (1 ^ 5));
    std::vector<int> v{1, 2, 3, 4};
    boost::span<int> s(v);
    CHECK(s.subspan(1, 2).front() == 2);
    CHECK(s.first(2).back() == 2);
    CHECK(s.last<1>()[0] == 4);
    CHECK(boost::as_bytes(s).size() == 4 * sizeof(int));
    boost::as_writable_bytes(s)[0] = std::byte{0};
    auto ms = boost::make_span(v.data(), 2);
    CHECK(ms.size() == 2);
    static_assert(boost::span<int, 3>::extent == 3);
}

TEST_CASE("doc: typeinfo#example") {
    using T = X<void const*>;
    boost::core::typeinfo const& ti = typeid(T);  // BOOST_CORE_TYPEID: include_macros.cpp
    CHECK(boost::core::demangled_name(ti).find("X<void const") != std::string::npos);  // MSVC: "void const *"
}

namespace {
inline int on_unwind = 0, on_normal = 0;
class my_class {
    const unsigned int m_exception_count;
public:
    my_class() : m_exception_count(boost::core::uncaught_exceptions()) {}
    ~my_class() noexcept(false) {
        if (m_exception_count == boost::core::uncaught_exceptions()) ++on_normal;
        else ++on_unwind;
    }
};
}  // namespace

TEST_CASE("doc: uncaught_exceptions#example") {
    { my_class c; }
    try {
        my_class c;
        throw 1;
    } catch (int) {
    }
    CHECK(on_normal == 1);
    CHECK(on_unwind == 1);
}

namespace {
template <class Value = boost::use_default, class Ref = boost::use_default>
struct adaptor {
    using value = std::conditional_t<std::is_same_v<Value, boost::use_default>, int, Value>;
    using ref = std::conditional_t<std::is_same_v<Ref, boost::use_default>, value&, Ref>;
};
}  // namespace

TEST_CASE("doc: use_default#example") {
    static_assert(std::is_same_v<adaptor<>::ref, int&>);
    static_assert(std::is_same_v<adaptor<long>::ref, long&>);
    static_assert(std::is_same_v<adaptor<boost::use_default, const int&>::ref, const int&>);
}

TEST_CASE("doc: verbose_terminate_handler#example") {
    // Installing the handler is what the example does; calling it would end
    // the process, which a test cannot observe in-process.
    std::terminate_handler previous = std::set_terminate(boost::core::verbose_terminate_handler);
    CHECK(std::get_terminate() == &boost::core::verbose_terminate_handler);
    std::set_terminate(previous);
}

TEST_CASE("string_view (libs/core/doc/string_view.qbk)") {
    boost::core::string_view sv("hello world");
    CHECK(sv.size() == 11);
    CHECK(sv.substr(6) == "world");
    CHECK(sv.find('o') == 4);
    CHECK(sv.rfind('o') == 7);
    CHECK(sv.starts_with("hell"));
    CHECK(sv.ends_with("ld"));
    CHECK(sv.contains("lo w"));
    CHECK(sv.find_first_of("wr") == 6);
    CHECK(sv.find_last_not_of('d') == 9);
    std::string s(sv);
    CHECK(s == "hello world");
    std::string_view stdsv = sv;  // converts to and from std::string_view
    boost::core::string_view back(stdsv);
    CHECK(back == sv);
    std::ostringstream os;
    os << sv.substr(0, 5);
    CHECK(os.str() == "hello");
    sv.remove_prefix(6);
    CHECK(sv.compare("world") == 0);
    static_assert(boost::core::string_view("abc").size() == 3);
    boost::core::wstring_view w(L"w");
    CHECK(w.size() == 1);
}
