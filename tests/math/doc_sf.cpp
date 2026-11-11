// mcpplibs.mwrap.boost.math — the special-function pages (libs/math/doc/sf/):
// one case per section, each calling the functions it documents at points
// with known values (closed forms, tabulated values from DLMF / A&S) or
// through identities between them.
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
const double pi = bmc::pi<double>();
}  // namespace

// ---- Airy ----
TEST_CASE("doc: ai") { CHECK(close(bm::airy_ai(0.0), 0.355028053887817239)); }
TEST_CASE("doc: aip") { CHECK(close(bm::airy_ai_prime(0.0), -0.258819403792806798)); }
TEST_CASE("doc: bi") { CHECK(close(bm::airy_bi(0.0), 0.614926627446000735)); }
TEST_CASE("doc: bip") { CHECK(close(bm::airy_bi_prime(0.0), 0.448288357353826357)); }
TEST_CASE("doc: airy_root") {
    CHECK(close(bm::airy_ai_zero<double>(1), -2.338107410459767, 1e-13));
    CHECK(close(bm::airy_bi_zero<double>(1), -1.173713222709128, 1e-13));
    std::vector<double> zeros;
    bm::airy_ai_zero<double>(1, 3, std::back_inserter(zeros));
    CHECK(zeros.size() == 3);
    CHECK(close(zeros[2], -5.520559828095551, 1e-12));
}

// ---- Bernoulli and tangent numbers ----
TEST_CASE("doc: bernoulli_numbers#bernoulli_numbers") {
    CHECK(close(bm::bernoulli_b2n<double>(1), 1.0 / 6));
    CHECK(close(bm::bernoulli_b2n<double>(2), -1.0 / 30));
    std::vector<double> b;
    bm::bernoulli_b2n<double>(0, 4, std::back_inserter(b));
    CHECK(close(b[3], 1.0 / 42));
}
TEST_CASE("doc: bernoulli_numbers#tangent_numbers") {
    CHECK(bm::tangent_t2n<double>(1) == 1);
    CHECK(bm::tangent_t2n<double>(2) == 2);
    CHECK(bm::tangent_t2n<double>(3) == 16);
}
TEST_CASE("doc: number_series#bernoulli_numbers") {
    CHECK(close(bm::unchecked_bernoulli_b2n<double>(3), 1.0 / 42));
    CHECK(bm::max_bernoulli_b2n<double>::value > 100);
}
TEST_CASE("doc: number_series#tangent_numbers") {
    std::vector<double> t;
    bm::tangent_t2n<double>(1, 4, std::back_inserter(t));
    CHECK(t == std::vector<double>{1, 2, 16, 272});
}
TEST_CASE("doc: fibonacci_numbers") {
    CHECK(bm::fibonacci<unsigned long long>(10) == 55);
    CHECK(bm::unchecked_fibonacci<unsigned long long>(20) == 6765);
    bm::fibonacci_generator<int> gen;
    gen();
    gen();
    CHECK(gen() == 1);  // 0, 1, 1, ...
}
TEST_CASE("doc: primes") {
    CHECK(bm::prime(0) == 2);
    CHECK(bm::prime(4) == 11);
    CHECK(bm::prime(9999) == 104729);  // the 10000th prime, the last of the table
}

// ---- Bessel ----
TEST_CASE("doc: bessel_first") {
    CHECK(close(bm::cyl_bessel_j(0, 1.0), 0.7651976865579666, 1e-14));
    CHECK(close(bm::cyl_neumann(0, 1.0), 0.08825696421567697, 1e-13));
}
TEST_CASE("doc: mbessel") {
    CHECK(close(bm::cyl_bessel_i(0, 1.0), 1.2660658777520082, 1e-14));
    CHECK(close(bm::cyl_bessel_k(0, 1.0), 0.42102443824070834, 1e-14));
}
TEST_CASE("doc: sph_bessel") {
    CHECK(close(bm::sph_bessel(0, 1.0), std::sin(1.0)));
    CHECK(close(bm::sph_neumann(0, 1.0), -std::cos(1.0)));
}
TEST_CASE("doc: cyl_hankel") {
    auto h = bm::cyl_hankel_1(0, 1.0);
    CHECK(close(h.real(), bm::cyl_bessel_j(0, 1.0)));
    CHECK(close(h.imag(), bm::cyl_neumann(0, 1.0)));
    CHECK(close(bm::cyl_hankel_2(0, 1.0).imag(), -bm::cyl_neumann(0, 1.0)));
}
TEST_CASE("doc: sph_hankel") {
    auto h = bm::sph_hankel_1(0, 1.0);
    CHECK(close(h.real(), std::sin(1.0)));
    CHECK(close(h.imag(), -std::cos(1.0)));
}

// ---- beta ----
TEST_CASE("doc: beta_function") { CHECK(close(bm::beta(2.0, 3.0), 1.0 / 12)); }
TEST_CASE("doc: beta_derivative") { CHECK(close(bm::ibeta_derivative(2.0, 3.0, 0.5), 1.5)); }
TEST_CASE("doc: ibeta_function") {
    CHECK(close(bm::ibeta(1.0, 1.0, 0.3), 0.3));
    CHECK(close(bm::ibetac(1.0, 1.0, 0.3), 0.7));
    CHECK(close(bm::beta(2.0, 3.0, 0.5), 11.0 / 192));  // non-normalised
}
TEST_CASE("doc: ibeta_inv_function") {
    CHECK(close(bm::ibeta_inv(2.0, 3.0, bm::ibeta(2.0, 3.0, 0.25)), 0.25, 1e-13));
    CHECK(close(bm::ibeta_inva(3.0, 0.25, bm::ibeta(2.0, 3.0, 0.25)), 2.0, 1e-10));
    CHECK(close(bm::ibeta_invb(2.0, 0.25, bm::ibeta(2.0, 3.0, 0.25)), 3.0, 1e-10));
}

// ---- splines, polynomials ----
TEST_CASE("doc: cardinal_b_splines") {
    CHECK(close(bm::cardinal_b_spline<1>(0.0), 1.0));
    CHECK(close(bm::cardinal_b_spline<1>(0.5), 0.5));
    CHECK(close(bm::cardinal_b_spline<3>(0.0), 2.0 / 3));
    CHECK(close(bm::forward_cardinal_b_spline<1>(1.0), 1.0));
}
TEST_CASE("doc: chebyshev") {
    CHECK(close(bm::chebyshev_t(2, 0.5), -0.5));
    CHECK(close(bm::chebyshev_u(2, 0.5), 0.0));
    CHECK(close(bm::chebyshev_t_prime(2, 0.5), 2.0));
}
TEST_CASE("doc: gegenbauer") {
    CHECK(close(bm::gegenbauer(1, 1.0, 0.5), 1.0));      // 2 lambda x
    CHECK(close(bm::gegenbauer(2, 0.5, 0.3), bm::legendre_p(2, 0.3)));  // lambda = 1/2
}
TEST_CASE("doc: hermite") {
    CHECK(bm::hermite(2, 1.0) == 2);
    CHECK(bm::hermite_next(2, 1.0, bm::hermite(2, 1.0), bm::hermite(1, 1.0)) == bm::hermite(3, 1.0));
}
TEST_CASE("doc: jacobi") {
    CHECK(close(bm::jacobi(1u, 0.0, 0.0, 0.5), 0.5));  // alpha = beta = 0: Legendre
    CHECK(close(bm::jacobi(3u, 0.0, 0.0, 0.3), bm::legendre_p(3, 0.3)));
}
TEST_CASE("doc: laguerre") {
    CHECK(close(bm::laguerre(1, 0.5), 0.5));
    CHECK(close(bm::laguerre(2, 1, 0.0), 3.0));  // associated: C(n+m, n)
}
TEST_CASE("doc: legendre") {
    CHECK(close(bm::legendre_p(2, 0.5), -0.125));
    CHECK(close(bm::legendre_q(0, 0.5), std::atanh(0.5)));
    CHECK(close(bm::legendre_p(1, 1, 0.5), -std::sqrt(0.75)));  // with the Condon-Shortley phase
}
TEST_CASE("doc: legendre_stieltjes") {
    bm::legendre_stieltjes<double> e3(3);
    auto zeros = e3.zeros();
    REQUIRE(!zeros.empty());
    for (double z : zeros)
        CHECK(std::abs(e3(z)) < 1e-12);
}
TEST_CASE("doc: daubechies") {
    auto h = bm::filters::daubechies_scaling_filter<double, 2>();
    CHECK(h.size() == 4);
    double sum = 0;
    for (double x : h)
        sum += x;
    CHECK(close(sum, std::sqrt(2.0), 1e-14));
#if !defined(_WIN32)  // segfaults on windows-llvm-msvc: an open issue (design record §11.6)
    bm::daubechies_scaling<double, 2> phi(10);  // 2^10 grid refinements (x = 1 is a grid point)
    CHECK(close(phi(1.0), (1 + std::sqrt(3.0)) / 2, 1e-12));
#endif
}
TEST_CASE("doc: sph_harm") {
    CHECK(close(bm::spherical_harmonic_r(0, 0, 0.3, 0.7), 0.5 / std::sqrt(pi)));
    CHECK(close(std::abs(bm::spherical_harmonic(1, 0, 0.0, 0.0)), std::sqrt(3 / (4 * pi))));
}

// ---- gamma family ----
TEST_CASE("doc: tgamma") {
    CHECK(close(bm::tgamma(5.0), 24));
    CHECK(close(bm::tgamma1pm1(1e-10), -0.5772156649015329e-10, 1e-8));
}
TEST_CASE("doc: lgamma") {
    CHECK(close(bm::lgamma(5.0), std::log(24.0)));
    int sign = 0;
    bm::lgamma(-0.5, &sign);
    CHECK(sign == -1);
}
TEST_CASE("doc: digamma") { CHECK(close(bm::digamma(1.0), -0.5772156649015329, 1e-14)); }
TEST_CASE("doc: trigamma") { CHECK(close(bm::trigamma(1.0), pi * pi / 6)); }
TEST_CASE("doc: polygamma") {
    CHECK(close(bm::polygamma(1, 1.0), pi * pi / 6));
    CHECK(close(bm::polygamma(2, 1.0), -2 * 1.2020569031595942, 1e-13));
}
TEST_CASE("doc: gamma_ratios") {
    CHECK(close(bm::tgamma_ratio(5.0, 3.0), 12));
    CHECK(close(bm::tgamma_delta_ratio(5.0, 1.0), 0.2));
}
TEST_CASE("doc: igamma") {
    CHECK(close(bm::gamma_p(1.0, 1.0), 1 - std::exp(-1.0)));
    CHECK(close(bm::gamma_q(1.0, 1.0), std::exp(-1.0)));
    CHECK(close(bm::tgamma_lower(2.0, 1.0) + bm::tgamma(2.0, 1.0), 1.0));
}
TEST_CASE("doc: igamma_inv") {
    CHECK(close(bm::gamma_p_inv(1.0, 1 - std::exp(-1.0)), 1.0, 1e-13));
    CHECK(close(bm::gamma_q_inv(1.0, std::exp(-1.0)), 1.0, 1e-13));
    CHECK(close(bm::gamma_p_inva(1.0, 1 - std::exp(-1.0)), 1.0, 1e-10));
}
TEST_CASE("doc: gamma_derivatives") { CHECK(close(bm::gamma_p_derivative(1.0, 1.0), std::exp(-1.0))); }
TEST_CASE("doc: sf_factorial") {
    CHECK(bm::factorial<double>(5) == 120);
    CHECK(bm::unchecked_factorial<double>(10) == 3628800);
    CHECK(bm::max_factorial<double>::value == 170);
}
TEST_CASE("doc: sf_double_factorial") {
    CHECK(bm::double_factorial<double>(5) == 15);
    CHECK(bm::double_factorial<double>(6) == 48);
}
TEST_CASE("doc: sf_rising_factorial") { CHECK(bm::rising_factorial(5.0, 2) == 30); }
TEST_CASE("doc: sf_falling_factorial") { CHECK(bm::falling_factorial(5.0, 2) == 20); }
TEST_CASE("doc: sf_binomial") { CHECK(bm::binomial_coefficient<double>(5, 2) == 10); }

// ---- elliptic integrals ----
TEST_CASE("doc: ellint_carlson") {
    CHECK(close(bm::ellint_rf(0.0, 1.0, 2.0), 1.3110287771460599, 1e-14));
    CHECK(close(bm::ellint_rc(0.0, 0.25), pi, 1e-14));
    CHECK(close(bm::ellint_rd(0.0, 2.0, 1.0), 1.7972103521033884, 1e-14));
    CHECK(close(bm::ellint_rj(2.0, 3.0, 4.0, 5.0), 0.14297579667156753, 1e-14));
    CHECK(close(bm::ellint_rg(0.0, 16.0, 16.0), pi, 1e-14));
}
TEST_CASE("doc: ellint_1") {
    CHECK(close(bm::ellint_1(0.5), 1.6857503548125961, 1e-14));
    CHECK(close(bm::ellint_1(0.0, 0.7), 0.7));
}
TEST_CASE("doc: ellint_2") {
    CHECK(close(bm::ellint_2(0.5), 1.4674622093394272, 1e-14));
    CHECK(close(bm::ellint_2(0.0, 0.7), 0.7));
}
TEST_CASE("doc: ellint_3") {
    CHECK(close(bm::ellint_3(0.5, 0.0), bm::ellint_1(0.5)));
    CHECK(close(bm::ellint_3(0.5, 0.0, 0.7), bm::ellint_1(0.5, 0.7)));
}
TEST_CASE("doc: ellint_d") {
    double k = 0.5;
    CHECK(close(bm::ellint_d(k), (bm::ellint_1(k) - bm::ellint_2(k)) / (k * k), 1e-13));
}
TEST_CASE("doc: heuman_lambda") {
    CHECK(bm::heuman_lambda(0.5, 0.0) == 0);
    double a = bm::heuman_lambda(0.5, 0.5), b = bm::heuman_lambda(0.5, 1.0);
    CHECK(a > 0);
    CHECK(b > a);  // increasing in phi
}
TEST_CASE("doc: jacobi_zeta") { CHECK(std::abs(bm::jacobi_zeta(0.5, pi / 2)) < 1e-14); }

// ---- error functions, exponential integrals ----
TEST_CASE("doc: error_function") {
    CHECK(close(bm::erf(1.0), 0.8427007929497149, 1e-15));
    CHECK(close(bm::erfc(1.0), 0.15729920705028513, 1e-14));
}
TEST_CASE("doc: error_inv") {
    CHECK(close(bm::erf_inv(bm::erf(0.5)), 0.5, 1e-14));
    CHECK(close(bm::erfc_inv(bm::erfc(2.0)), 2.0, 1e-14));
}
TEST_CASE("doc: expint_i") { CHECK(close(bm::expint(1.0), 1.8951178163559368, 1e-14)); }
TEST_CASE("doc: expint_n") { CHECK(close(bm::expint(1, 1.0), 0.21938393439552029, 1e-14)); }

// ---- hypergeometric ----
TEST_CASE("doc: hypergeometric_0f1") { CHECK(close(bm::hypergeometric_0F1(0.5, 0.25), std::cosh(1.0), 1e-14)); }
TEST_CASE("doc: hypergeometric_1f0") { CHECK(close(bm::hypergeometric_1F0(2.0, 0.5), 4.0)); }
TEST_CASE("doc: hypergeometric_1f1") {
    CHECK(close(bm::hypergeometric_1F1(1.0, 1.0, 1.0), std::exp(1.0), 1e-14));
    CHECK(close(bm::hypergeometric_1F1(1.0, 2.0, 1.0), std::exp(1.0) - 1, 1e-14));
}
TEST_CASE("doc: hypergeometric_2f0") { CHECK(close(bm::hypergeometric_2F0(-1.0, 2.0, 0.5), 0.0)); }
TEST_CASE("doc: hypergeometric_pfq") {
    CHECK(close(bm::hypergeometric_pFq({1.0}, {1.0}, 1.0), std::exp(1.0), 1e-14));
    CHECK(close(bm::hypergeometric_pFq({}, {0.5}, 0.25), std::cosh(1.0), 1e-14));
}

// ---- inverse hyperbolic functions ----
TEST_CASE("doc: inv_hyper#acosh") { CHECK(bm::acosh(1.0) == 0); }
TEST_CASE("doc: inv_hyper#asinh") { CHECK(close(bm::asinh(1.0), std::log(1 + std::sqrt(2.0)))); }
TEST_CASE("doc: inv_hyper#atanh") { CHECK(close(bm::atanh(0.5), 0.5493061443340549)); }

// ---- Jacobi elliptic functions (k = 0: the circular functions) ----
namespace {
const double u = 0.7;
}
TEST_CASE("doc: jacobi_elliptic") {
    double cn, dn;
    double sn = bm::jacobi_elliptic(0.5, u, &cn, &dn);
    CHECK(close(sn * sn + cn * cn, 1.0, 1e-14));
    CHECK(close(dn * dn + 0.25 * sn * sn, 1.0, 1e-14));
}
TEST_CASE("doc: jac_over") {
    // k = 1: sn = tanh, cn = dn = sech
    CHECK(close(bm::jacobi_sn(1.0, u), std::tanh(u), 1e-13));
    CHECK(close(bm::jacobi_cn(1.0, u), 1 / std::cosh(u), 1e-13));
}
TEST_CASE("doc: jacobi_sn") { CHECK(close(bm::jacobi_sn(0.0, u), std::sin(u))); }
TEST_CASE("doc: jacobi_cn") { CHECK(close(bm::jacobi_cn(0.0, u), std::cos(u))); }
TEST_CASE("doc: jacobi_dn") { CHECK(close(bm::jacobi_dn(0.0, u), 1.0)); }
TEST_CASE("doc: jacobi_cd") { CHECK(close(bm::jacobi_cd(0.0, u), std::cos(u))); }
TEST_CASE("doc: jacobi_cs") { CHECK(close(bm::jacobi_cs(0.0, u), 1 / std::tan(u))); }
TEST_CASE("doc: jacobi_dc") { CHECK(close(bm::jacobi_dc(0.0, u), 1 / std::cos(u))); }
TEST_CASE("doc: jacobi_ds") { CHECK(close(bm::jacobi_ds(0.0, u), 1 / std::sin(u))); }
TEST_CASE("doc: jacobi_nc") { CHECK(close(bm::jacobi_nc(0.0, u), 1 / std::cos(u))); }
TEST_CASE("doc: jacobi_nd") { CHECK(close(bm::jacobi_nd(0.0, u), 1.0)); }
TEST_CASE("doc: jacobi_ns") { CHECK(close(bm::jacobi_ns(0.0, u), 1 / std::sin(u))); }
TEST_CASE("doc: jacobi_sc") { CHECK(close(bm::jacobi_sc(0.0, u), std::tan(u))); }
TEST_CASE("doc: jacobi_sd") { CHECK(close(bm::jacobi_sd(0.0, u), std::sin(u))); }

// ---- Jacobi theta functions (q = 0.1) ----
namespace {
const double q = 0.1;
}
TEST_CASE("doc: jacobi_theta1") {
    CHECK(std::abs(bm::jacobi_theta1(0.0, q)) < 1e-15);
    CHECK(close(bm::jacobi_theta1(pi / 2, q), bm::jacobi_theta2(0.0, q), 1e-14));
}
TEST_CASE("doc: jacobi_theta2") { CHECK(close(bm::jacobi_theta2(0.0, q), 2 * std::pow(q, 0.25) * (1 + q * q + std::pow(q, 6)), 1e-10)); }
TEST_CASE("doc: jacobi_theta3") { CHECK(close(bm::jacobi_theta3(0.0, q), 1 + 2 * q + 2 * std::pow(q, 4), 1e-8)); }
TEST_CASE("doc: jacobi_theta4") { CHECK(close(bm::jacobi_theta4(0.0, q), 1 - 2 * q + 2 * std::pow(q, 4), 1e-8)); }
TEST_CASE("doc: jacobi_theta_overview") {
    // Jacobi's identity theta3^4 = theta2^4 + theta4^4, and the tau / m1 forms
    double t2 = bm::jacobi_theta2(0.0, q), t3 = bm::jacobi_theta3(0.0, q), t4 = bm::jacobi_theta4(0.0, q);
    CHECK(close(std::pow(t3, 4), std::pow(t2, 4) + std::pow(t4, 4), 1e-13));
    double tau = -std::log(q) / pi;
    CHECK(close(bm::jacobi_theta3tau(0.3, tau), bm::jacobi_theta3(0.3, q), 1e-13));
    CHECK(close(bm::jacobi_theta3m1(0.3, q) + 1, bm::jacobi_theta3(0.3, q), 1e-13));
}

// ---- Lambert W, logistic, and the elementary functions ----
TEST_CASE("doc: lambert_w") {
    CHECK(close(bm::lambert_w0(std::exp(1.0)), 1.0, 1e-15));
    CHECK(close(bm::lambert_wm1(-std::exp(-2.0) * 2), -2.0, 1e-13));
    CHECK(close(bm::lambert_w0_prime(0.0), 1.0));
}
TEST_CASE("doc: logaddexp") { CHECK(close(bm::logaddexp(std::log(2.0), std::log(3.0)), std::log(5.0))); }
TEST_CASE("doc: logistic_sigmoid") { CHECK(close(bm::logistic_sigmoid(0.0), 0.5)); }
TEST_CASE("doc: logit") {
    CHECK(close(bm::logit(0.5), 0.0));
    CHECK(close(bm::logistic_sigmoid(bm::logit(0.2)), 0.2, 1e-15));
}
TEST_CASE("doc: owens_t") { CHECK(close(bm::owens_t(0.0, 1.0), 0.125, 1e-15)); }
TEST_CASE("doc: zeta") {
    CHECK(close(bm::zeta(2.0), pi * pi / 6, 1e-15));
    CHECK(close(bm::zeta(0.0), -0.5));
}
TEST_CASE("doc: ct_pow") {
    CHECK(bm::pow<3>(2.0) == 8);
    static_assert(bm::pow<2>(3) == 9);
}
TEST_CASE("doc: cbrt") { CHECK(close(bm::cbrt(27.0), 3.0)); }
TEST_CASE("doc: cos_pi") { CHECK(bm::cos_pi(0.5) == 0); }
TEST_CASE("doc: sin_pi") { CHECK(bm::sin_pi(0.5) == 1); }
TEST_CASE("doc: expm1") { CHECK(close(bm::expm1(1e-10), 1.00000000005e-10, 1e-15)); }
TEST_CASE("doc: log1p") { CHECK(close(bm::log1p(1e-10), 9.9999999995e-11, 1e-15)); }
TEST_CASE("doc: powm1") { CHECK(close(bm::powm1(2.0, 3.0), 7.0)); }
TEST_CASE("doc: sqrt1pm1") { CHECK(close(bm::sqrt1pm1(3.0), 1.0)); }
TEST_CASE("doc: rsqrt") { CHECK(close(bm::rsqrt(4.0), 0.5)); }
TEST_CASE("doc: sinc-pi") {
    CHECK(bm::sinc_pi(0.0) == 1);
    CHECK(close(bm::sinc_pi(1.0), std::sin(1.0)));
}
TEST_CASE("doc: sinhc-pi") {
    CHECK(bm::sinhc_pi(0.0) == 1);
    CHECK(close(bm::sinhc_pi(1.0), std::sinh(1.0)));
}
TEST_CASE("doc: ccmath") {
    static_assert(bm::ccmath::sqrt(4.0) == 2.0);
    static_assert(bm::ccmath::abs(-3) == 3);
    static_assert(bm::ccmath::floor(2.5) == 2.0);
    CHECK(bm::ccmath::isnan(bm::ccmath::sqrt(-1.0)));
    CHECK(bm::ccmath::fmod(7.0, 3.0) == 1.0);
}
