// mcpplibs.mwrap.boost.decimal — the remaining reference pages
// (libs/decimal/doc/modules/ROOT/pages/{cmath,charconv,strings,basics,cfenv,
// design,format}.adoc): the <cmath> families and the decimal-specific
// functions (quantize, samequantum, decompose, frexp10, normalize, payloads,
// comparetotal, rescale), <charconv>, string conversions, rounding modes,
// stream input of non-finite values, and std::format.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.decimal;

using namespace boost::decimal;
using d64 = decimal64_t;

namespace {
// (not `near`: <windows.h>, which doctest includes there, defines it as a macro)
bool close_to(d64 a, double b, double tol = 1e-12) { return std::abs(static_cast<double>(a) - b) <= tol * std::max(1.0, std::abs(b)); }
}  // namespace

TEST_CASE("doc: cmath#cmath") {
    CHECK(abs(d64(-2)) == d64(2));
    CHECK(fabs(d64(-2)) == d64(2));
    CHECK(fmod(d64(7), d64(3)) == d64(1));
    CHECK(remainder(d64(7), d64(4)) == d64(-1));
    CHECK(fmax(d64(1), d64(2)) == d64(2));
    CHECK(fmin(d64(1), d64(2)) == d64(1));
    CHECK(fdim(d64(5), d64(3)) == d64(2));
    CHECK(fma(d64(2), d64(3), d64(4)) == d64(10));
}
TEST_CASE("doc: cmath#cmath_exp") {
    CHECK(close_to(exp(d64(1)), std::exp(1.0)));
    CHECK(close_to(exp2(d64(10)), 1024.0));
    CHECK(close_to(log(d64(10)), std::log(10.0)));
    CHECK(close_to(log10(d64(1000)), 3.0));
    CHECK(close_to(log2(d64(8)), 3.0));
    CHECK(close_to(log1p(d64(0)), 0.0));
    CHECK(close_to(expm1(d64(0)), 0.0));
}
TEST_CASE("doc: cmath#cmath_power_funcs") {
    CHECK(pow(d64(2), d64(10)) == d64(1024));
    CHECK(pow(d64(2), 3) == d64(8));
    CHECK(sqrt(d64(144)) == d64(12));
    CHECK(close_to(cbrt(d64(27)), 3.0));
    CHECK(close_to(hypot(d64(3), d64(4)), 5.0));
}
TEST_CASE("doc: cmath#cmath_trig_funcs") {
    // decimal64's trigonometric functions are accurate to about 1e-11 here
    CHECK(close_to(sin(d64(0.5)), std::sin(0.5), 1e-10));
    CHECK(close_to(cos(d64(0.5)), std::cos(0.5), 1e-10));
    CHECK(close_to(tan(d64(0.5)), std::tan(0.5), 1e-10));
    CHECK(close_to(asin(d64(0.5)), std::asin(0.5)));
    CHECK(close_to(acos(d64(0.5)), std::acos(0.5)));
    CHECK(close_to(atan(d64(0.5)), std::atan(0.5)));
    CHECK(close_to(atan2(d64(1), d64(2)), std::atan2(1.0, 2.0)));
}
TEST_CASE("doc: cmath#cmath_hyperbolic_funcs") {
    CHECK(close_to(sinh(d64(0.5)), std::sinh(0.5)));
    CHECK(close_to(cosh(d64(0.5)), std::cosh(0.5)));
    CHECK(close_to(tanh(d64(0.5)), std::tanh(0.5)));
    CHECK(close_to(asinh(d64(0.5)), std::asinh(0.5)));
    CHECK(close_to(acosh(d64(1.5)), std::acosh(1.5)));
    CHECK(close_to(atanh(d64(0.5)), std::atanh(0.5)));
}
TEST_CASE("doc: cmath#cmath_error_funcs") {
    CHECK(close_to(erf(d64(0.5)), std::erf(0.5), 1e-10));
    CHECK(close_to(erfc(d64(0.5)), std::erfc(0.5), 1e-10));
    CHECK(close_to(tgamma(d64(5)), 24.0, 1e-10));
    CHECK(close_to(lgamma(d64(5)), std::lgamma(5.0), 1e-10));
}
TEST_CASE("doc: cmath#cmath_nearest_int") {
    CHECK(ceil(d64(1.2)) == d64(2));
    CHECK(floor(d64(-1.2)) == d64(-2));
    CHECK(trunc(d64(-1.7)) == d64(-1));
    CHECK(round(d64(2.5)) == d64(3));
    CHECK(lround(d64(2.4)) == 2);
    CHECK(llround(d64(-2.6)) == -3);
    CHECK(nearbyint(d64(2.5)) == d64(2));  // to nearest even
    CHECK(rint(d64(3.5)) == d64(4));
}
TEST_CASE("doc: cmath#cmath_manipulation_funcs") {
    int e = 0;
    d64 m = frexp(d64(8), &e);
    CHECK(ldexp(m, e) == d64(8));
    CHECK(scalbn(d64(3), 2) == d64(300));  // radix 10
    CHECK(copysign(d64(2), d64(-1)) == d64(-2));
    CHECK(nextafter(d64(1), d64(2)) > d64(1));
    d64 ip;
    d64 frac = modf(d64(3.25), &ip);
    CHECK(ip == d64(3));
    CHECK(frac == d64(25u, -2));
    // logb/ilogb return the biased exponent in 1.92 (logb(1000) == 401 for
    // decimal64, also with the headers alone); differences are unaffected.
    CHECK(logb(d64(1000)) - logb(d64(1)) == d64(3));
    CHECK(ilogb(d64(1000)) - ilogb(d64(1)) == 3);
}
TEST_CASE("doc: cmath#cmath_classification") {
    d64 inf = std::numeric_limits<d64>::infinity();
    d64 qnan = std::numeric_limits<d64>::quiet_NaN();
    CHECK(isinf(inf));
    CHECK(isnan(qnan));
    CHECK(isfinite(d64(1)));
    CHECK(isnormal(d64(1)));
    CHECK(signbit(d64(-1)));
    CHECK(fpclassify(d64(0)) == FP_ZERO);
    CHECK(isless(d64(1), d64(2)));
    CHECK(isunordered(qnan, d64(1)));
}
TEST_CASE("doc: cmath#cmath_spec_fun") {
    CHECK(close_to(assoc_legendre(2, 0, d64(0.5)), -0.125, 1e-10));
    CHECK(close_to(legendre(2, d64(0.5)), -0.125, 1e-10));
    CHECK(close_to(hermite(2, d64(1)), 2.0, 1e-10));
    CHECK(close_to(laguerre(1, d64(0.5)), 0.5, 1e-10));
    CHECK(close_to(riemann_zeta(d64(2)), 1.6449340668482264, 1e-10));
}
TEST_CASE("doc: cmath#comparetotal") {
    CHECK(comparetotal(d64(1), d64(2)));
    CHECK_FALSE(comparetotal(d64(2), d64(1)));
    CHECK(comparetotal(-std::numeric_limits<d64>::quiet_NaN(), d64(0)));  // -NaN orders first
}
TEST_CASE("doc: cmath#samequantum") {
    CHECK(samequantum(d64(1u, -1), d64(9u, -1)));
    CHECK_FALSE(samequantum(d64(1u, -1), d64(10u, -2)));
    CHECK(samequantumd64(d64(1u, 2), d64(5u, 2)));
}
TEST_CASE("doc: cmath#quantexp") {
    // quantexp returns the biased exponent (-2 + 398 for decimal64).
    CHECK(quantexp(d64(123u, -2)) - quantexp(d64(123u, 0)) == -2);
    CHECK(quantexpd64(d64(5u, 3)) - quantexpd64(d64(5u, 0)) == 3);  // the page says quantexp64
}
TEST_CASE("doc: cmath#quantized") {
    // Round 3.14159 to the quantum of 0.01. The page says quantized; 1.92
    // spells it quantize.
    d64 r = quantize(d64(314159u, -5), d64(1u, -2));
    CHECK(r == d64(314u, -2));
    CHECK(quantexp(r) == quantexp(d64(1u, -2)));
}
TEST_CASE("doc: cmath#issignaling") {
    CHECK(issignaling(std::numeric_limits<d64>::signaling_NaN()));
    CHECK_FALSE(issignaling(std::numeric_limits<d64>::quiet_NaN()));
}
TEST_CASE("doc: cmath#read-payload") {
    d64 n = nand64("42");
    CHECK(isnan(n));
    CHECK(read_payload(n) == 42u);
}
TEST_CASE("doc: cmath#decompose") {
    auto c = decompose(d64(300u, 0));
    CHECK(c.sign == false);
    CHECK(c.sig * std::pow(10.0, c.exp) == 300.0);
}
TEST_CASE("doc: cmath#frexp10") {
    int e = 0;
    auto sig = frexp10(d64(300), &e);
    CHECK(double(sig) * std::pow(10.0, e) == 300.0);
    CHECK(sig >= 1000000000000000u);  // normalized to the full precision of decimal64
}
TEST_CASE("doc: cmath#normalize") {
    d64 a(3u, 2), b(300u, 0);
    CHECK(a == b);
    CHECK(to_bid(normalize(a)) == to_bid(normalize(b)));  // normalized cohorts are bitwise equal
}
TEST_CASE("doc: cmath#rescale") {
    CHECK(rescale(d64(31415u, -4)) == d64(3));
    CHECK(rescale(d64(31415u, -4), 2) == d64(31u, -1));  // 2 significant digits
}

TEST_CASE("doc: charconv#charconv_limits") {
    // The page says limits<T>; 1.92 spells it formatting_limits<T>.
    static_assert(formatting_limits<decimal32_t>::max_chars > 0);
    static_assert(formatting_limits<decimal128_t>::max_chars >= formatting_limits<decimal64_t>::max_chars);
}
TEST_CASE("doc: charconv#chars_format") {
    char buf[64];
    auto r = to_chars(buf, buf + sizeof buf, d64(1234u, -1), chars_format::scientific);
    CHECK(std::string_view(buf, r.ptr) == "1.234e+02");
    r = to_chars(buf, buf + sizeof buf, d64(1234u, -1), chars_format::fixed);
    CHECK(std::string_view(buf, r.ptr) == "123.4");
}
TEST_CASE("doc: charconv#from_chars") {
    d64 v;
    std::string_view s = "2.5e3";
    auto r = from_chars(s.data(), s.data() + s.size(), v);
    CHECK(r.ec == std::errc());
    CHECK(v == d64(2500));
}
TEST_CASE("doc: charconv#from_chars_result") {
    d64 v;
    std::string_view s = "abc";
    auto r = from_chars(s.data(), s.data() + s.size(), v);
    CHECK(r.ec == std::errc::invalid_argument);
    CHECK_FALSE(static_cast<bool>(r));
}
TEST_CASE("doc: charconv#to_chars") {
    char buf[64];
    auto r = to_chars(buf, buf + sizeof buf, d64(-5));
    CHECK(std::string_view(buf, r.ptr).front() == '-');
    r = to_chars(buf, buf + sizeof buf, d64(1u, -1), chars_format::fixed, 3);
    CHECK(std::string_view(buf, r.ptr) == "0.100");
}
TEST_CASE("doc: charconv#to_chars_result") {
    char small[2];
    auto r = to_chars(small, small + 2, d64(123456));
    CHECK(r.ec == std::errc::value_too_large);
}

TEST_CASE("doc: strings#strings") {
    decimal32_t a("1.5");
    decimal32_t b(std::string_view("1.5"));
    CHECK(a == b);
}
TEST_CASE("doc: strings#conversions-from-std-string") {
    std::size_t idx = 0;
    CHECK(stod64("3.75xyz", &idx) == d64(375u, -2));
    CHECK(idx == 4);
    CHECK(stod32("1") == decimal32_t(1));
    CHECK(stod128f("2") == decimal_fast128_t(2));
    CHECK_THROWS_AS(stod64("nope"), std::invalid_argument);
}
TEST_CASE("doc: strings#to-string") {
    CHECK(to_string(d64(15u, -1)) == "1.5");
    CHECK(to_string(decimal32_t(100)) == "100");
}

TEST_CASE("doc: basics#construction-from-coefficient-and-exponent") {
    decimal32_t a{1, 1};
    decimal32_t b{-2, -1};
    decimal32_t c{2U, -1, construction_sign::negative};
    decimal32_t d{5, 5};
    decimal32_t e{1234, -3};
    CHECK(a == decimal32_t(10));
    CHECK(b == c);
    CHECK(d == decimal32_t(500000));
    CHECK(e == decimal32_t("1.234"));
    decimal64_t overflow_value{100, 10000};
    decimal64_t underflow_value{100, -10000};
    CHECK(isinf(overflow_value));
    CHECK(underflow_value == decimal64_t(0));
}
TEST_CASE("doc: basics#construction-from-integer") {
    decimal32_t f{-4};
    CHECK(f == decimal32_t(-4));
}
TEST_CASE("doc: basics#construction-from-binary-floating-point") {
    decimal128_t pi{3.14};
    CHECK(close_to(static_cast<d64>(pi), 3.14));
}
TEST_CASE("doc: cfenv#cfenv") {
    CHECK(fegetround() == rounding_mode::fe_dec_default);
    auto old = fesetround(rounding_mode::fe_dec_toward_zero);
    (void)old;
    // Without consteval detection the mode cannot change: fegetround reports it.
    auto now = fegetround();
    CHECK((now == rounding_mode::fe_dec_toward_zero || now == rounding_mode::fe_dec_default));
    fesetround(rounding_mode::fe_dec_default);
}
TEST_CASE("doc: design#non-finite-deviation") {
    std::istringstream is("INF");
    decimal32_t x;
    is >> x;
    CHECK(isinf(x));
    // The page asserts tellg() == 3; reading "INF" to its end sets eofbit, so
    // tellg() returns -1 (with the headers alone too). The input is consumed:
    CHECK(is.eof());
    std::istringstream more("inf 2");
    decimal32_t y, z;
    more >> y >> z;
    CHECK(isinf(y));
    CHECK(z == decimal32_t(2));
}
TEST_CASE("doc: design#floating-point-exception-flags-are-not-supported") {
    // Operations do not raise <cfenv> flags; results carry the condition.
    d64 r = d64(1) / d64(0);
    CHECK(isinf(r));
    CHECK(isnan(d64(0) / d64(0)));
}
TEST_CASE("doc: format#format") {
    CHECK(std::format("{}", d64(314u, -2)) == "3.14");
    CHECK(std::format("{:.1e}", d64(314u, -2)) == "3.1e+00");
}
