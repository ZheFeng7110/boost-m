// mcpplibs.mwrap.boost.math — the pages on floating-point utilities
// (fp_utilities/), constants, the complex inverse trigonometric functions of
// complex-tr1.qbk, quaternions and octonions, condition numbers and norms.
// (The C99 / TR1 pages describe extern "C" functions of the compiled
// libboost_math_tr1, which the package does not build: coverage.toml, gap.)
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.math;

namespace bm = boost::math;
namespace bmc = boost::math::constants;

namespace {
bool close(double a, double b, double rel = 1e-12) {
    return std::abs(a - b) <= rel * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}
bool close(std::complex<double> a, std::complex<double> b, double rel = 1e-12) {
    return close(a.real(), b.real(), rel) && close(a.imag(), b.imag(), rel);
}
}  // namespace

// ---- floating-point utilities ----
TEST_CASE("doc: float_next") {
    CHECK(bm::float_next(1.0) == 1.0 + std::numeric_limits<double>::epsilon());
    CHECK(bm::float_next(0.0) == std::numeric_limits<double>::denorm_min());
}
TEST_CASE("doc: float_prior") { CHECK(bm::float_prior(1.0) == 1.0 - std::numeric_limits<double>::epsilon() / 2); }
TEST_CASE("doc: nextafter") {
    CHECK(bm::nextafter(1.0, 2.0) == bm::float_next(1.0));
    CHECK(bm::nextafter(1.0, 0.0) == bm::float_prior(1.0));
}
TEST_CASE("doc: float_distance") {
    CHECK(bm::float_distance(1.0, bm::float_next(bm::float_next(1.0))) == 2);
    CHECK(bm::float_distance(1.0, 1.0) == 0);
}
TEST_CASE("doc: float_advance") {
    CHECK(bm::float_advance(1.0, 3) == bm::float_next(bm::float_next(bm::float_next(1.0))));
    CHECK(bm::float_advance(1.0, -1) == bm::float_prior(1.0));
}
TEST_CASE("doc: ulp") {
    CHECK(bm::ulp(1.0) == std::numeric_limits<double>::epsilon());
    CHECK(bm::ulp(2.0) == 2 * std::numeric_limits<double>::epsilon());
}
TEST_CASE("doc: float_comparison") {
    double a = 1.0, b = bm::float_next(1.0);
    CHECK(bm::relative_difference(a, b) == std::numeric_limits<double>::epsilon());
    CHECK(bm::epsilon_difference(a, b) == 1);
    CHECK(bm::epsilon_difference(1.0, 1.0) == 0);
}
TEST_CASE("doc: fpclass") {
    double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
    CHECK(bm::isinf(inf));
    CHECK(bm::isnan(nan));
    CHECK(bm::isfinite(1.0));
    CHECK(bm::isnormal(1.0));
    CHECK_FALSE(bm::isnormal(std::numeric_limits<double>::denorm_min()));
    CHECK(bm::fpclassify(0.0) == FP_ZERO);
}
TEST_CASE("doc: sign_functions") {
    CHECK(bm::sign(-2.0) == -1);
    CHECK(bm::sign(0.0) == 0);
    CHECK(bm::signbit(-0.0));
    CHECK(bm::copysign(3.0, -1.0) == -3.0);
    CHECK(bm::changesign(2.0) == -2.0);
}
TEST_CASE("doc: round") {
    CHECK(bm::round(2.5) == 3);
    CHECK(bm::iround(-2.5) == -3);
    CHECK(bm::lround(2.4) == 2);
}
TEST_CASE("doc: trunc") {
    CHECK(bm::trunc(-2.7) == -2);
    CHECK(bm::itrunc(2.7) == 2);
}
TEST_CASE("doc: modf") {
    double ip;
    CHECK(close(bm::modf(2.75, &ip), 0.75));
    CHECK(ip == 2);
    int ii;
    bm::modf(-3.5, &ii);
    CHECK(ii == -3);
}
TEST_CASE("doc: cond") {
    // the condition number of summation: sum |x| / |sum x|
    bm::tools::summation_condition_number<double> kahan;
    for (double x : {1.0, -1.0, 1e-8})
        kahan += x;
    CHECK(close(kahan.sum(), 1e-8, 1e-6));
    CHECK(kahan() > 1e7);
    CHECK(close(bm::tools::evaluation_condition_number([](double x) { return std::exp(x); }, 2.0), 2.0, 1e-6));
}
TEST_CASE("doc: norms") {
    std::vector<double> v{3, -4};
    CHECK(bm::tools::l2_norm(v) == 5);
    CHECK(bm::tools::l1_norm(v) == 7);
    CHECK(bm::tools::sup_norm(v) == 4);
    CHECK(bm::tools::lp_norm(v, 3) == std::cbrt(91.0));
    CHECK(bm::tools::l0_pseudo_norm(v) == 2);
    CHECK(close(bm::tools::l2_distance(v, std::vector<double>{0, 0}), 5));
}

// ---- constants ----
TEST_CASE("doc: templ") {
    CHECK(bmc::pi<float>() == 3.14159265f);
    CHECK(close(bmc::pi<double>(), 3.141592653589793));
    CHECK(close(bmc::root_two<double>(), std::sqrt(2.0)));
}
TEST_CASE("doc: non_templ") {
    CHECK(bm::double_constants::pi == bmc::pi<double>());
    CHECK(bm::float_constants::e == bmc::e<float>());
    CHECK(close(bm::double_constants::half_pi * 2, bm::double_constants::pi));
}
TEST_CASE("doc: user_def") {
    // a constant through the generic interface: the value of a constexpr
    // expression in the requested precision
    constexpr double third = bmc::third<double>();
    static_assert(third > 0.333 && third < 0.334);
    CHECK(close(bmc::euler<double>(), 0.5772156649015329));
}
TEST_CASE("doc: new_const") {
    CHECK(close(bmc::phi<double>(), (1 + std::sqrt(5.0)) / 2));
    CHECK(close(bmc::catalan<double>(), 0.915965594177219));
}
TEST_CASE("doc: constants_faq") {
    // the constants are exact to the last bit of the type, not computed at run time
    CHECK(bmc::pi<double>() == std::numbers::pi);
    CHECK(bmc::e<double>() == std::numbers::e);
}

// ---- complex inverse trigonometric / hyperbolic functions ----
namespace {
const std::complex<double> z(0.3, 0.4);
}
TEST_CASE("doc: acos") { CHECK(close(std::cos(bm::acos(z)), z)); }
TEST_CASE("doc: asin") { CHECK(close(std::sin(bm::asin(z)), z)); }
TEST_CASE("doc: atan") { CHECK(close(std::tan(bm::atan(z)), z)); }
TEST_CASE("doc: complex-tr1#acosh") { CHECK(close(std::cosh(bm::acosh(z)), z)); }
TEST_CASE("doc: complex-tr1#asinh") { CHECK(close(std::sinh(bm::asinh(z)), z)); }
TEST_CASE("doc: complex-tr1#atanh") { CHECK(close(std::tanh(bm::atanh(z)), z)); }

// ---- quaternions ----
using quat = boost::math::quaternion<double>;
TEST_CASE("doc: quat") {
    quat i(0, 1, 0, 0), j(0, 0, 1, 0), k(0, 0, 0, 1);
    CHECK(i * j == k);
    CHECK(j * i == -k);
    CHECK(i * i == quat(-1));
}
TEST_CASE("doc: spec") {
    static_assert(std::is_same_v<boost::math::quaternion<float>::value_type, float>);
    boost::math::quaternion<long double> q(1.0L);
    CHECK(q.R_component_1() == 1.0L);
}
TEST_CASE("doc: mem_typedef") { static_assert(std::is_same_v<quat::value_type, double>); }
TEST_CASE("doc: create") {
    quat a(1, 2, 3, 4);
    quat b(std::complex<double>(1, 2), std::complex<double>(3, 4));
    CHECK(a == b);
    boost::math::quaternion<float> c(a);
    CHECK(c.R_component_4() == 4.0f);
}
TEST_CASE("doc: quat_mem_fun") {
    quat a(1, 2, 3, 4);
    CHECK(a.real() == 1);
    CHECK(a.unreal() == quat(0, 2, 3, 4));
    CHECK(a.C_component_2() == std::complex<double>(3, 4));
}
TEST_CASE("doc: value_op") {
    quat a(1, 2, 3, 4);
    a += 1.0;
    a *= quat(0, 1, 0, 0);
    a -= quat(0, 0, 0, 1);
    a /= 2.0;
    CHECK(a == quat(-1, 1, 2, -2));  // ((1+2i+3j+4k) + 1) i - k, halved
}
TEST_CASE("doc: quat_non_mem") {
    quat a(1, 2, 3, 4);
    CHECK(close(boost::math::abs(a), std::sqrt(30.0)));
    CHECK(boost::math::norm(a) == 30);
    CHECK(boost::math::conj(a) == quat(1, -2, -3, -4));
    CHECK(boost::math::sup(a) == 4);
    CHECK(boost::math::l1(a) == 10);
    std::ostringstream os;
    os << a;
    CHECK(os.str() == "(1,2,3,4)");
}
TEST_CASE("doc: trans") {
    quat i(0, 1, 0, 0);
    quat e = boost::math::exp(i * boost::math::constants::pi<double>());
    CHECK(close(e.R_component_1(), -1));
    CHECK(std::abs(e.R_component_2()) < 1e-15);
    CHECK(boost::math::pow(i, 2) == quat(-1));
    CHECK(close(boost::math::cos(quat(0.5)).R_component_1(), std::cos(0.5)));
}

// ---- octonions ----
using oct = boost::math::octonion<double>;
TEST_CASE("doc: octonion") {
    oct e1(0, 1), e2(0, 0, 1);
    CHECK(e1 * e1 == oct(-1));
    CHECK(e1 * e2 == -(e2 * e1));
}
TEST_CASE("doc: oct_specialization") { static_assert(std::is_same_v<boost::math::octonion<float>::value_type, float>); }
TEST_CASE("doc: oct_typedefs") { static_assert(std::is_same_v<oct::value_type, double>); }
TEST_CASE("doc: oct_create") {
    oct a(1, 2, 3, 4, 5, 6, 7, 8);
    oct b(quat(1, 2, 3, 4), quat(5, 6, 7, 8));
    CHECK(a == b);
}
TEST_CASE("doc: oct_mem_fun") {
    oct a(1, 2, 3, 4, 5, 6, 7, 8);
    CHECK(a.real() == 1);
    CHECK(a.H_component_2() == quat(5, 6, 7, 8));
    CHECK(a.R_component_8() == 8);
}
TEST_CASE("doc: oct_value_ops") {
    oct a(1);
    a += oct(0, 1);
    a *= 2.0;
    CHECK(a == oct(2, 2));
}
TEST_CASE("doc: oct_non_mem") {
    oct a(1, 1, 1, 1, 1, 1, 1, 1);
    CHECK(boost::math::norm(a) == 8);
    CHECK(close(boost::math::abs(a), std::sqrt(8.0)));
    CHECK(boost::math::conj(a) == oct(1, -1, -1, -1, -1, -1, -1, -1));
}
TEST_CASE("doc: oct_trans") {
    oct z(0.5);
    CHECK(close(boost::math::exp(z).R_component_1(), std::exp(0.5)));
    CHECK(close(boost::math::sinh(z).R_component_1(), std::sinh(0.5)));
}
