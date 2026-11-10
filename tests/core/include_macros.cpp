// boost.core macro components — pure include: a named module cannot export
// macros (docs/usage.md §3), and this TU does not import, so no include+import
// mixing on gcc. Covers alignof (BOOST_CORE_ALIGNOF), explicit_operator_bool,
// lightweight_test(_trait), no_exceptions_support (BOOST_TRY...), scoped_enum
// and BOOST_CORE_TYPEID.
//
// lightweight_test keeps a process-wide error count that boost::report_errors()
// returns; the last case (cases run in declaration order) checks that count.
// Two failures are made on purpose (BOOST_ERROR, a failing BOOST_TEST_NE)
// to see them counted; lightweight_test reports them on stderr.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <boost/core/alignof.hpp>
#include <boost/core/explicit_operator_bool.hpp>
#include <boost/core/lightweight_test.hpp>
#include <boost/core/lightweight_test_trait.hpp>
#include <boost/core/no_exceptions_support.hpp>
#include <boost/core/scoped_enum.hpp>
#include <boost/core/typeinfo.hpp>
#include <boost/core/underlying_type.hpp>
#include <boost/type_traits/is_same.hpp>

TEST_CASE("doc: alignof#example") {
    constexpr std::size_t alignment_of_double = BOOST_CORE_ALIGNOF(double);
    static_assert(alignment_of_double == alignof(double));
}

namespace {
template <class T>
class my_ptr {
public:
    explicit my_ptr(T* p = nullptr) : m_p(p) {}
    BOOST_EXPLICIT_OPERATOR_BOOL()
    bool operator!() const { return !m_p; }
private:
    T* m_p;
};
struct flag {
    bool on;
    BOOST_CONSTEXPR_EXPLICIT_OPERATOR_BOOL()
    constexpr bool operator!() const noexcept { return !on; }
};
}  // namespace

TEST_CASE("doc: explicit_operator_bool#examples") {
    int i = 0;
    my_ptr<int> p(&i), q;
    CHECK(static_cast<bool>(p));
    CHECK_FALSE(static_cast<bool>(q));
    if (p) CHECK(true);
    static_assert(!std::is_convertible_v<my_ptr<int>, int>);  // no implicit conversion
    static_assert(static_cast<bool>(flag{true}));
}

namespace {
int sqr(int x) { return x * x; }
template <class T, class U>
struct X2 {
    typedef T type;
};
}  // namespace

TEST_CASE("doc: lightweight_test#example") {
    BOOST_TEST(sqr(2) == 4);
    BOOST_TEST_EQ(sqr(-3), 9);
}
TEST_CASE("doc: lightweight_test#boost-test") { BOOST_TEST(1 + 1 == 2); }
TEST_CASE("doc: lightweight_test#boost-test-not") { BOOST_TEST_NOT(1 + 1 == 3); }
TEST_CASE("doc: lightweight_test#boost-error") { BOOST_ERROR("deliberate failure #1"); }
TEST_CASE("doc: lightweight_test#boost-test-eq") { BOOST_TEST_EQ(std::string("a"), "a"); }
TEST_CASE("doc: lightweight_test#boost-test-ne") {
    BOOST_TEST_NE(1, 2);
    BOOST_TEST_NE(3, 3);  // deliberate failure #2
}
TEST_CASE("doc: lightweight_test#boost-test-lt") { BOOST_TEST_LT(1, 2); }
TEST_CASE("doc: lightweight_test#boost-test-le") { BOOST_TEST_LE(2, 2); }
TEST_CASE("doc: lightweight_test#boost-test-gt") { BOOST_TEST_GT(3, 2); }
TEST_CASE("doc: lightweight_test#boost-test-ge") { BOOST_TEST_GE(3, 3); }
TEST_CASE("doc: lightweight_test#boost-test-cstr-eq") {
    const char a[] = "abc";
    std::string b = "abc";
    BOOST_TEST_CSTR_EQ(a, b.c_str());
}
TEST_CASE("doc: lightweight_test#boost-test-cstr-ne") { BOOST_TEST_CSTR_NE("abc", "abd"); }
TEST_CASE("doc: lightweight_test#boost-test-with") {
    BOOST_TEST_WITH(1.0, 1.0 + 1e-12, [](double a, double b) { return a - b < 1e-9 && b - a < 1e-9; });
}
TEST_CASE("doc: lightweight_test#boost-test-all-eq") {
    std::vector<int> a{1, 2}, b{1, 2};
    BOOST_TEST_ALL_EQ(a.begin(), a.end(), b.begin(), b.end());
}
TEST_CASE("doc: lightweight_test#boost-test-all-with") {
    std::vector<int> a{1, 2}, b{2, 4};
    BOOST_TEST_ALL_WITH(a.begin(), a.end(), b.begin(), b.end(), [](int x, int y) { return 2 * x == y; });
}
TEST_CASE("doc: lightweight_test#boost-test-throws") {
    BOOST_TEST_THROWS(throw std::runtime_error("x"), std::runtime_error);
}
TEST_CASE("doc: lightweight_test#boost-test-no-throw") { BOOST_TEST_NO_THROW(sqr(3)); }
TEST_CASE("doc: lightweight_test#lwt-init") { boost::core::lwt_init(); }
TEST_CASE("doc: lightweight_test#example-2") {
    BOOST_TEST_TRAIT_TRUE((boost::is_same<X2<int, long>::type, int>));
    BOOST_TEST_TRAIT_SAME(X2<int, long>::type, int);
}
TEST_CASE("doc: lightweight_test#boost-test-trait-true") { BOOST_TEST_TRAIT_TRUE((std::is_integral<int>)); }
TEST_CASE("doc: lightweight_test#boost-test-trait-false") { BOOST_TEST_TRAIT_FALSE((std::is_integral<float>)); }
TEST_CASE("doc: lightweight_test#boost-test-trait-same") { BOOST_TEST_TRAIT_SAME(int, signed int); }

namespace {
int classify(int what) {
    int r = 0;
    BOOST_TRY {
        if (what == 1) throw std::bad_alloc();
        if (what == 2) throw std::runtime_error("e");
        r = 3;
    }
    BOOST_CATCH(const std::bad_alloc&) {
        r = 1;
    }
    BOOST_CATCH(const std::exception&) {
        r = 2;
    }
    BOOST_CATCH_END
    return r;
}
int rethrows() {
    BOOST_TRY {
        throw std::bad_alloc();
    }
    BOOST_CATCH(const std::bad_alloc&) {
        BOOST_RETHROW
    }
    BOOST_CATCH_END
    return 0;
}
}  // namespace

TEST_CASE("doc: no_exceptions_support#example-use") {
    CHECK(classify(0) == 3);
    CHECK(classify(1) == 1);
    CHECK(classify(2) == 2);
    CHECK_THROWS_AS(rethrows(), std::bad_alloc);
}

namespace {
BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(future_errc, unsigned int){
    broken_promise, future_already_retrieved, promise_already_satisfied, no_state}
BOOST_SCOPED_ENUM_DECLARE_END(future_errc)

BOOST_SCOPED_ENUM_DECLARE_BEGIN(enum_scoped){a, b}
BOOST_SCOPED_ENUM_DECLARE_END(enum_scoped)

enum enum_regular { REGULAR_A, REGULAR_B };
int regular_or_int(enum_regular) { return 1; }
int regular_or_int(int) { return 2; }
int scoped_or_int(enum_scoped) { return 3; }
int scoped_or_int(int) { return 4; }

BOOST_SCOPED_ENUM_UT_DECLARE_BEGIN(algae, char){green, red, cyan}
BOOST_SCOPED_ENUM_DECLARE_END(algae)
}  // namespace

TEST_CASE("doc: scoped_enum#overview") {
    future_errc ev = future_errc::no_state;
    int r = 0;
    switch (boost::native_value(ev)) {
    case future_errc::no_state: r = 1; break;
    default: break;
    }
    CHECK(r == 1);
    unsigned int val = boost::underlying_cast<unsigned int>(ev);
    CHECK(val == 3u);
    static_assert(std::is_same_v<boost::underlying_type<future_errc>::type, unsigned int>);
    algae sample(algae::red);
    sample = algae::green;
    CHECK(boost::native_value(sample) == algae::green);
}
TEST_CASE("doc: scoped_enum#deprecated-syntax") {
    CHECK(regular_or_int(REGULAR_A) == 1);
    CHECK(scoped_or_int(enum_scoped::a) == 3);  // C++11: the scoped overload
    CHECK(scoped_or_int(enum_scoped(enum_scoped::a)) == 3);
    static_assert(std::is_same_v<BOOST_SCOPED_ENUM_NATIVE(enum_scoped), enum_scoped>);
}

TEST_CASE("BOOST_CORE_TYPEID") {
    using T = X2<int, long>;  // a macro argument cannot carry the comma
    boost::core::typeinfo const& ti = BOOST_CORE_TYPEID(T);
    CHECK(boost::core::demangled_name(ti).find("X2<int,") != std::string::npos);  // MSVC: no space
}

TEST_CASE("doc: lightweight_test#report-errors") {
    // The two deliberate failures above, and nothing else.
    CHECK(boost::report_errors() == 2);
}
