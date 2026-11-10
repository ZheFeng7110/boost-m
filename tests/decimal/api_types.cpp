// mcpplibs.mwrap.boost.decimal — the six type pages of the reference
// (libs/decimal/doc/modules/ROOT/pages/decimal{32,64,128}_t.adoc and
// decimal_fast{32,64,128}_t.adoc): each section's members are checked by a
// function template, instantiated once per type under the case name of that
// type's page section, plus the fast_types and generic_decimal operator pages.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.decimal;

using namespace boost::decimal;

namespace {
template <class T>
void description() {
    static_assert(std::numeric_limits<T>::is_specialized);
    static_assert(!std::numeric_limits<T>::is_integer);
    static_assert(std::numeric_limits<T>::radix == 10);
    T zero{};
    CHECK(zero == T(0));
}
template <class T>
void integral() {
    constexpr T a(42);
    static_assert(a == T(42));
    CHECK(T(-7) < T(0));
    CHECK(T(123u, 2) == T(12300));        // coefficient, exponent
    CHECK(T(5u, -1, true) == T(-5) / T(10));  // negative sign flag
    CHECK(T(-25, -1) == T(-25) / T(10));     // signed coefficient
    T b;
    if constexpr (std::is_assignable_v<T&, int>) b = 9;  // the IEEE types assign from integers
    else b = T(9);                                      // the fast types do not
    CHECK(b == T(9));
}
template <class T>
void string() {
    T a("3.25");
    CHECK(a == T(325u, -2));
    T b(std::string_view("-1e3"));
    CHECK(b == T(-1000));
    CHECK_THROWS(T("not a number"));
}
template <class T>
void binary_float() {
    T a(0.5);
    CHECK(static_cast<double>(a) == 0.5);
    CHECK(static_cast<float>(T(2.25f)) == 2.25f);
    CHECK(static_cast<long double>(T(1.5)) == 1.5L);
    T inf(std::numeric_limits<double>::infinity());
    CHECK(isinf(inf));
}
template <class T>
void to_integral() {
    T v(1234u, -2);  // 12.34
    CHECK(static_cast<int>(v) == 12);
    CHECK(static_cast<long long>(T(-9)) == -9);
    CHECK(static_cast<unsigned>(T(77)) == 77u);
    CHECK(static_cast<unsigned long long>(T(5)) == 5ull);
}
template <class T>
void to_other() {
    T v(75u, -1);
    CHECK(static_cast<decimal128_t>(v) == decimal128_t(75u, -1));
    CHECK(static_cast<decimal64_t>(v) == decimal64_t(75u, -1));
    CHECK(static_cast<decimal_fast32_t>(v) == decimal_fast32_t(75u, -1));
}
template <class T>
void incdec() {
    T v(1);
    CHECK(++v == T(2));
    CHECK(v++ == T(2));
    CHECK(v == T(3));
    CHECK(--v == T(2));
    CHECK(v-- == T(2));
    CHECK(v == T(1));
}
template <class T>
void compound() {
    T v(10);
    v += T(5);
    v -= 3;
    v *= T(2);
    v /= 4;
    CHECK(v == T(6));
}
}  // namespace


// fast_types.adoc / generic_decimal.adoc: the arithmetic and comparison
// operators, between equal and mixed types (promotion to the wider one).
namespace {
template <class T>
void arith() {
    T a(6), b(4);
    CHECK(a + b == T(10));
    CHECK(a - b == T(2));
    CHECK(a * b == T(24));
    CHECK(a / b == T(15u, -1));
    CHECK(-a == T(-6));
    CHECK(+a == a);
    CHECK(a > b);
    CHECK(a >= b);
    CHECK(b < a);
    CHECK(b <= a);
    CHECK(a != b);
    CHECK(a + 1 == T(7));  // mixed with an integer
    CHECK(2 * a == T(12));
}
}  // namespace

TEST_CASE("doc: decimal32_t#decimal32_t") { description<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#construction-from-integral-type") { integral<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#construction-from-string") { string<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#construction-to-and-from-binary-floating-point-type") { binary_float<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#conversion-to-integral-type") { to_integral<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#conversion-to-other-decimal-types") { to_other<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#increment-and-decrement-operators") { incdec<decimal32_t>(); }
TEST_CASE("doc: decimal32_t#compound-operators") { compound<decimal32_t>(); }
TEST_CASE("doc: decimal64_t#decimal64_t") { description<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#construction-from-integral-type") { integral<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#construction-from-string") { string<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#construction-to-and-from-binary-floating-point-type") { binary_float<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#conversion-to-integral-type") { to_integral<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#conversion-to-other-decimal-types") { to_other<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#increment-and-decrement-operators") { incdec<decimal64_t>(); }
TEST_CASE("doc: decimal64_t#compound-operators") { compound<decimal64_t>(); }
TEST_CASE("doc: decimal128_t#decimal128_t") { description<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#construction-from-integral-type") { integral<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#construction-from-string") { string<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#construction-to-and-from-binary-floating-point-type") { binary_float<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#conversion-to-integral-type") { to_integral<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#conversion-to-other-decimal-types") { to_other<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#increment-and-decrement-operators") { incdec<decimal128_t>(); }
TEST_CASE("doc: decimal128_t#compound-operators") { compound<decimal128_t>(); }
TEST_CASE("doc: decimal_fast32_t#decimal_fast32_t") { description<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#construction-from-integral-type") { integral<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#construction-from-string") { string<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#construction-to-and-from-binary-floating-point-type") { binary_float<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#conversion-to-integral-type") { to_integral<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#conversion-to-other-decimal-types") { to_other<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#increment-and-decrement-operators") { incdec<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast32_t#compound-operators") { compound<decimal_fast32_t>(); }
TEST_CASE("doc: decimal_fast64_t#decimal_fast64_t") { description<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#construction-from-integral-type") { integral<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#construction-from-string") { string<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#construction-to-and-from-binary-floating-point-type") { binary_float<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#conversion-to-integral-type") { to_integral<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#conversion-to-other-decimal-types") { to_other<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#increment-and-decrement-operators") { incdec<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast64_t#compound-operators") { compound<decimal_fast64_t>(); }
TEST_CASE("doc: decimal_fast128_t#decimal_fast128_t") { description<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#construction-from-integral-type") { integral<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#construction-from-string") { string<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#construction-to-and-from-binary-floating-point-type") { binary_float<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#conversion-to-integral-type") { to_integral<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#conversion-to-other-decimal-types") { to_other<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#increment-and-decrement-operators") { incdec<decimal_fast128_t>(); }
TEST_CASE("doc: decimal_fast128_t#compound-operators") { compound<decimal_fast128_t>(); }
TEST_CASE("doc: fast_types#addition") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: fast_types#subtraction") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: fast_types#multiplication") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: fast_types#division") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: fast_types#unary-minus") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: fast_types#unary-plus") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: fast_types#comparison-operators") { arith<decimal_fast64_t>(); }
TEST_CASE("doc: generic_decimal#addition") { arith<decimal64_t>(); }
TEST_CASE("doc: generic_decimal#subtraction") { arith<decimal64_t>(); }
TEST_CASE("doc: generic_decimal#multiplication") { arith<decimal64_t>(); }
TEST_CASE("doc: generic_decimal#division") { arith<decimal64_t>(); }
TEST_CASE("doc: generic_decimal#unary-minus") { arith<decimal64_t>(); }
TEST_CASE("doc: generic_decimal#unary-plus") { arith<decimal64_t>(); }
TEST_CASE("doc: generic_decimal#comparison-operators") { arith<decimal64_t>(); }
TEST_CASE("doc: fast_types#overview") {
    // The fast types trade the IEEE 754 encoding for speed; values convert
    // losslessly to the matching IEEE type and compare equal.
    decimal_fast32_t f(3u, -1);
    decimal32_t d(f);
    CHECK(static_cast<decimal_fast32_t>(d) == f);
    static_assert(sizeof(decimal_fast32_t) >= sizeof(decimal32_t));
}
TEST_CASE("mixed decimal arithmetic promotes to the wider type") {
    auto r = decimal32_t(1) + decimal64_t(2);
    static_assert(std::is_same_v<decltype(r), decimal64_t>);
    auto s = decimal64_t(1) * decimal128_t(3);
    static_assert(std::is_same_v<decltype(s), decimal128_t>);
    CHECK(s == decimal128_t(3));
}
