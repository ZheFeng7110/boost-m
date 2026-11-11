// mcpplibs.mwrap.boost.math — the policy pages (policies/policy.qbk,
// policy_tutorial.qbk) and the overview / background pages: error handling,
// result types, namespaces, the implementation and testing tutorials, the
// Lanczos approximation, concept archetypes, <boost/cstdfloat.hpp> types, GPU
// and performance notes. Policies are passed as arguments and through
// make_policy; the BOOST_MATH_*_POLICY macros of a translation unit cannot
// reach a module built once (docs/usage.md).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <boost/math/special_functions/math_fwd.hpp>  // BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS
#include <boost/math/cstdfloat/cstdfloat_types.hpp>  // BOOST_FLOAT64_C and friends
import std;
import mcpplibs.mwrap.boost.math;

namespace bm = boost::math;
namespace pol = boost::math::policies;

namespace {
bool close(double a, double b, double rel = 1e-12) {
    return std::abs(a - b) <= rel * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}
using ignore_all = pol::policy<pol::domain_error<pol::ignore_error>, pol::pole_error<pol::ignore_error>,
                               pol::overflow_error<pol::ignore_error>, pol::evaluation_error<pol::ignore_error>>;
}  // namespace

// ---- policy.qbk ----
TEST_CASE("doc: error_handling_policies") {
    CHECK_THROWS_AS(bm::tgamma(-1.0), std::domain_error);
    CHECK(std::isnan(bm::tgamma(-1.0, ignore_all())));
    CHECK(std::isinf(bm::tgamma(200.0, pol::make_policy(pol::overflow_error<pol::ignore_error>()))));
    errno = 0;
    bm::tgamma(-1.0, pol::make_policy(pol::pole_error<pol::errno_on_error>()));  // a pole
    CHECK(errno == EDOM);
}
TEST_CASE("doc: precision_pol") {
    double full = bm::tgamma(3.5);
    double low = bm::tgamma(3.5, pol::make_policy(pol::digits10<5>()));
    CHECK(close(full, low, 1e-5));
    CHECK(close(bm::tgamma(3.5, pol::make_policy(pol::digits2<20>())), full, 1e-5));
}
TEST_CASE("doc: iteration_pol") {
    using few = pol::policy<pol::max_series_iterations<5>, pol::evaluation_error<pol::ignore_error>>;
    static_assert(pol::get_max_series_iterations<few>() == 5);
    CHECK(pol::get_max_root_iterations<pol::policy<pol::max_root_iterations<7>>>() == 7);
}
TEST_CASE("doc: discrete_quant_ref") {
    bm::binomial b(20, 0.5);
    double qi = bm::quantile(b, 0.3);  // integer_round_outwards (the default)
    CHECK(qi == std::floor(qi));
    using real_q = pol::policy<pol::discrete_quantile<pol::real>>;
    bm::binomial_distribution<double, real_q> br(20, 0.5);
    double q = bm::quantile(br, 0.3);
    CHECK(q > 8);
    CHECK(q < 10);
    CHECK(q != std::floor(q));
}
TEST_CASE("doc: assert_undefined") {
    // the mean of a Cauchy distribution is undefined: with assert_undefined<true>
    // (the default) bm::mean(cauchy()) does not compile; without, a domain error
    using lenient = pol::policy<pol::assert_undefined<false>, pol::domain_error<pol::ignore_error>>;
    CHECK(std::isnan(bm::mean(bm::cauchy_distribution<double, lenient>())));
}
TEST_CASE("doc: namespace_pol") {
    // BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS: a namespace of the functions with
    // one policy baked in (the expansion is in this file)
}
namespace ignore_ns {
BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS(ignore_all)
}
TEST_CASE("doc: pol_ref_ref") {
    CHECK(std::isnan(ignore_ns::tgamma(-1.0)));
    CHECK(close(ignore_ns::tgamma(5.0), 24));
    static_assert(std::is_same_v<pol::normalise<pol::policy<pol::digits10<5>>>::type,
                                 pol::normalise<pol::policy<pol::digits10<5>>>::type>);
}
TEST_CASE("doc: policy_defaults") {
    using d = pol::policy<>;
    static_assert(std::is_same_v<d::domain_error_type, pol::domain_error<pol::throw_on_error>>);
    static_assert(std::is_same_v<d::overflow_error_type, pol::overflow_error<pol::throw_on_error>>);
    static_assert(std::is_same_v<d::underflow_error_type, pol::underflow_error<pol::ignore_error>>);
    static_assert(d::promote_double_type::value);
}

// ---- policy_tutorial.qbk ----
TEST_CASE("doc: what_is_a_policy") {
    CHECK(close(bm::erf(0.5, pol::policy<>()), bm::erf(0.5)));
}
TEST_CASE("doc: policy_tut_defaults") {
    static_assert(std::is_same_v<pol::policy<>::evaluation_error_type, pol::evaluation_error<pol::throw_on_error>>);
}
TEST_CASE("doc: ad_hoc_dist_policies") {
    using my_pol = pol::policy<pol::overflow_error<pol::ignore_error>>;
    bm::normal_distribution<double, my_pol> n;
    CHECK(std::isinf(bm::quantile(n, 1.0)));
}
TEST_CASE("doc: ad_hoc_sf_policies") {
    CHECK(close(bm::tgamma(4.0, pol::make_policy(pol::promote_double<false>())), 6));
    double t = bm::tgamma(0.0, pol::make_policy(pol::pole_error<pol::ignore_error>()));
    CHECK((std::isinf(t) || std::isnan(t)));
}
TEST_CASE("doc: namespace_policies") {
    using p = pol::policy<pol::domain_error<pol::ignore_error>>;
    CHECK(std::isnan(bm::acosh(0.5, p())));
}
TEST_CASE("doc: understand_dis_quant") {
    bm::poisson pd(5);
    using up = pol::policy<pol::discrete_quantile<pol::integer_round_up>>;
    using down = pol::policy<pol::discrete_quantile<pol::integer_round_down>>;
    double qu = bm::quantile(bm::poisson_distribution<double, up>(5), 0.5);
    double qd = bm::quantile(bm::poisson_distribution<double, down>(5), 0.5);
    CHECK(qu >= qd);
    CHECK(bm::quantile(pd, 0.5) == 5);
}

// ---- overview ----
TEST_CASE("doc: error_handling") {
    try {
        bm::tgamma(-2.0);
        CHECK(false);
    } catch (std::domain_error const& e) {
        CHECK(std::string(e.what()).find("tgamma") != std::string::npos);
    }
    CHECK_THROWS_AS(bm::tgamma(1000.0), std::overflow_error);
    CHECK_THROWS_AS(bm::cyl_bessel_j(2.5, -1.0), std::domain_error);  // non-integer order, negative x
}
TEST_CASE("doc: result_type") {
    static_assert(std::is_same_v<decltype(bm::tgamma(2)), double>);  // integers promote to double
    static_assert(std::is_same_v<decltype(bm::tgamma(2.0f)), float>);
    static_assert(std::is_same_v<decltype(bm::beta(1.0f, 2.0)), double>);
    static_assert(std::is_same_v<bm::tools::promote_args_t<float, long double>, long double>);
}
TEST_CASE("doc: namespaces") {
    CHECK(bm::constants::pi<double>() > 3);
    CHECK(bm::tools::evaluate_polynomial(std::array<double, 2>{1, 1}, 1.0) == 2);
    static_assert(std::is_same_v<bm::policies::policy<>, pol::policy<>>);
}
TEST_CASE("doc: gpu") {
    // the device-capable functions are the same functions, called from host code here
    CHECK(close(bm::sin_pi(0.25), std::sqrt(0.5)));
    CHECK(close(bm::cdf(bm::normal(), 0.0), 0.5));
}
TEST_CASE("doc: tradoffs") {
    // promote_double<false> trades accuracy for speed
    double a = bm::tgamma(0.5, pol::make_policy(pol::promote_double<false>()));
    CHECK(close(a, std::sqrt(bm::constants::pi<double>()), 1e-14));
}

// ---- background ----
TEST_CASE("doc: sf_implementation") {
    // the implementation notes: functions are accurate across their domain
    CHECK(close(bm::lgamma(1e-300), 690.77552789821368, 1e-14));
    CHECK(close(bm::erfc(10.0), 2.0884875837625446e-45, 1e-14));
}
TEST_CASE("doc: lanczos") {
    using L = bm::lanczos::lanczos13m53;
    static_assert(std::is_same_v<bm::lanczos::lanczos<double, pol::policy<>>::type, L>);
    // Gamma(z) = sqrt(2 pi) (z + g - 1/2)^(z - 1/2) e^-(z + g - 1/2) L(z), with the
    // library's L(z) scaled by e^g: lanczos_sum_expG_scaled
    double z = 5.5, g = L::g(), zgh = z + g - 0.5;
    double approx = L::lanczos_sum_expG_scaled(z) * std::pow(zgh / std::exp(1.0), z - 0.5);
    CHECK(close(approx, bm::tgamma(z), 1e-13));
}
TEST_CASE("doc: special_tut_impl") {
    // the tutorial's new special function: an implementation through the
    // library's tools (a series via sum_series), checked against erf
    struct erf_series {  // erf(x) = 2/sqrt(pi) * sum (-1)^n x^(2n+1) / (n! (2n+1))
        using result_type = double;
        double x, term;
        int n = 0;
        explicit erf_series(double x_) : x(x_), term(x_) {}
        double operator()() {
            double r = term / (2 * n + 1);
            ++n;
            term *= -x * x / n;
            return r;
        }
    };
    std::uintmax_t max = 200;
    erf_series series(0.5);
    double s = bm::tools::sum_series(series, std::numeric_limits<double>::epsilon(), max);
    CHECK(close(2 / std::sqrt(bm::constants::pi<double>()) * s, bm::erf(0.5), 1e-14));
}
TEST_CASE("doc: special_tut_test") {
    // the testing tutorial: spot values with a relative-error tolerance in epsilons
    double e = bm::epsilon_difference(bm::tgamma(0.5), std::sqrt(bm::constants::pi<double>()));
    CHECK(e < 5);
}

// ---- concepts ----
TEST_CASE("doc: archetypes") {
    // real_concept: a real type with only the operations the library may use.
    // (std_real_concept is not usable through the module: its overloads are
    // added to namespace std, which the module cannot re-export alone.)
    using real = bm::concepts::real_concept;
    CHECK(close(bm::erf(real(0.5)).value(), bm::erf(0.5), 1e-15));
    CHECK(close(bm::tgamma(real(4)).value(), 6, 1e-15));
}
TEST_CASE("doc: use_mpfr") {
    // another real type through the same functions: long double here (MPFR is not part of the package)
    CHECK(close(static_cast<double>(bm::tgamma(4.5L)), bm::tgamma(4.5), 1e-15));
}

// ---- cstdfloat ----
TEST_CASE("doc: overloading") {
    static_assert(sizeof(boost::float64_t) == 8);
    static_assert(std::is_same_v<boost::float32_t, float>);
    CHECK(close(bm::tgamma(boost::float64_t(5)), 24));
}
TEST_CASE("doc: typeinfo") {
    static_assert(std::numeric_limits<boost::float64_t>::digits == 53);
    CHECK(std::string(typeid(boost::float32_t).name()) == typeid(float).name());
}
TEST_CASE("doc: float128") {
    // the 128-bit type is there only with quadmath (gcc's libquadmath)
#if defined(BOOST_FLOAT128_C)
    CHECK(sizeof(boost::float128_t) == 16);
#else
    CHECK(sizeof(boost::floatmax_t) >= sizeof(boost::float64_t));
#endif
}
TEST_CASE("doc: macros") {
    CHECK(BOOST_FLOAT64_C(1.5) == 1.5);
    CHECK(BOOST_FLOAT32_C(0.5) == 0.5f);
}
TEST_CASE("doc: overview") {
    boost::float_least64_t a = 1;
    boost::float_fast32_t b = 2;
    CHECK(a + b == 3);
}
