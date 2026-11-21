// mcpplibs.mwrap.boost.math — the statistical distribution pages
// (libs/math/doc/distributions/): every distribution is constructed as its
// page shows and checked through its accessors, pdf/cdf/quantile at points
// with closed forms, complements, and the non-member properties; plus the
// tutorial pages (distribution objects, complements, the algorithms that find
// parameters, the empirical CDF).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.math;

namespace bm = boost::math;

namespace {
bool close(double a, double b, double rel = 1e-12) {
    return std::abs(a - b) <= rel * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}
const double pi = boost::math::constants::pi<double>();
// cdf / quantile round trip and complement consistency, for any distribution
template <class D>
void consistent(D const& d, double x) {
    double p = bm::cdf(d, x);
    CHECK(close(bm::cdf(bm::complement(d, x)), 1 - p, 1e-10));
    CHECK(close(bm::quantile(d, p), x, 1e-8));
}
}  // namespace

TEST_CASE("doc: arcine_dist") {
    bm::arcsine_distribution<> d(0, 1);
    CHECK(close(bm::cdf(d, 0.5), 0.5));
    CHECK(close(bm::pdf(d, 0.5), 2 / pi));
    CHECK(close(bm::mean(d), 0.5));
    consistent(d, 0.3);
}
TEST_CASE("doc: bernoulli_dist") {
    bm::bernoulli d(0.25);
    CHECK(bm::pdf(d, 1) == 0.25);
    CHECK(bm::cdf(d, 0) == 0.75);
    CHECK(d.success_fraction() == 0.25);
}
TEST_CASE("doc: beta_dist") {
    bm::beta_distribution<> d(2, 3);
    CHECK(close(bm::mean(d), 0.4));
    CHECK(close(bm::cdf(d, 0.5), bm::ibeta(2.0, 3.0, 0.5)));
    CHECK(close(bm::beta_distribution<>::find_alpha(bm::mean(d), bm::variance(d)), 2, 1e-10));
    consistent(d, 0.3);
}
TEST_CASE("doc: binomial_dist") {
    bm::binomial d(10, 0.5);
    CHECK(close(bm::pdf(d, 5), 252.0 / 1024));
    CHECK(close(bm::cdf(d, 0), 1.0 / 1024));
    CHECK(bm::mean(d) == 5);
    CHECK(close(bm::binomial::find_lower_bound_on_p(10, 5, 0.05), 0.2224, 1e-3));
}
TEST_CASE("doc: binom_conf") {
    // Clopper-Pearson interval for 5 successes in 10 trials, 95%
    double lo = bm::binomial::find_lower_bound_on_p(10, 5, 0.025);
    double hi = bm::binomial::find_upper_bound_on_p(10, 5, 0.025);
    CHECK(close(lo, 0.18709, 1e-4));
    CHECK(close(hi, 0.81291, 1e-4));
}
TEST_CASE("doc: binom_size_eg") {
    // the fewest trials so that seeing at most 1 success has probability <= 5%
    double n = bm::binomial::find_minimum_number_of_trials(1, 0.5, 0.05);
    CHECK(n >= 1);
    CHECK(bm::cdf(bm::binomial(std::ceil(n), 0.5), 1) <= 0.05 + 1e-12);
    double m = bm::binomial::find_maximum_number_of_trials(0, 0.01, 0.05);
    CHECK(m > 0);
}
TEST_CASE("doc: cauchy_dist") {
    bm::cauchy d(0, 1);
    CHECK(close(bm::cdf(d, 1), 0.75));
    CHECK(close(bm::pdf(d, 0), 1 / pi));
    CHECK(bm::median(d) == 0);
    consistent(d, 2.0);
}
TEST_CASE("doc: chi_squared_dist") {
    bm::chi_squared d(2);
    CHECK(close(bm::cdf(d, 2), 1 - std::exp(-1.0)));
    CHECK(bm::mean(d) == 2);
    consistent(d, 3.0);
}
TEST_CASE("doc: chi_sq_intervals") {
    // confidence interval for a standard deviation: sd 0.5 from 100 observations
    double sd = 0.5, n = 100, alpha = 0.05;
    bm::chi_squared d(n - 1);
    double lo = std::sqrt((n - 1) * sd * sd / bm::quantile(bm::complement(d, alpha / 2)));
    double hi = std::sqrt((n - 1) * sd * sd / bm::quantile(d, alpha / 2));
    CHECK(lo < sd);
    CHECK(hi > sd);
    CHECK(close(lo, 0.439, 1e-2));
}
TEST_CASE("doc: chi_sq_size") {
    double df = bm::chi_squared::find_degrees_of_freedom(0.1, 0.05, 0.05, 1);
    CHECK(df > 0);
}
TEST_CASE("doc: chi_sq_test") {
    bm::chi_squared d(10);
    double t = 18.307;  // the 95% critical value
    CHECK(close(bm::cdf(bm::complement(d, t)), 0.05, 1e-3));
}
TEST_CASE("doc: dist_algorithms") {
    // find_location / find_scale: a normal with P(X < 5) = 0.95 and sd 1
    double loc = bm::find_location<bm::normal>(5.0, 0.95, 1.0);
    CHECK(close(bm::cdf(bm::normal(loc, 1.0), 5.0), 0.95, 1e-12));
    double scale = bm::find_scale<bm::normal>(5.0, 0.95, 0.0);
    CHECK(close(bm::cdf(bm::normal(0.0, scale), 5.0), 0.95, 1e-12));
}
TEST_CASE("doc: future") {
    // the reference's "future" section names distributions still to come; the
    // implemented ones share the same non-member interface
    static_assert(std::is_same_v<bm::normal::value_type, double>);
    CHECK(bm::support(bm::normal()).first <= -std::numeric_limits<double>::max());
}
TEST_CASE("doc: complements") {
    bm::normal n;
    double x = 6.0;  // far in the tail: 1 - cdf loses everything
    CHECK(bm::cdf(bm::complement(n, x)) > 0);
    CHECK(close(bm::cdf(bm::complement(n, x)), 9.8658765e-10, 1e-6));
    CHECK(close(bm::quantile(bm::complement(n, 0.05)), 1.6448536269514722, 1e-12));
}
TEST_CASE("doc: objects") {
    bm::students_t t(10);
    bm::normal_distribution<float> nf;
    static_assert(std::is_same_v<decltype(bm::cdf(nf, 1.0f)), float>);
    CHECK(t.degrees_of_freedom() == 10);
    CHECK(close(bm::cdf(t, 0.0), 0.5));
}
TEST_CASE("doc: empirical_cdf") {
    std::vector<double> v{3, 1, 2, 4};
    bm::empirical_cumulative_distribution_function<std::vector<double>> F(std::move(v));
    CHECK(F(0.5) == 0);
    CHECK(F(2.0) == 0.5);
    CHECK(F(10.0) == 1);
}
TEST_CASE("doc: exp_dist") {
    bm::exponential d(2);
    CHECK(close(bm::cdf(d, 1), 1 - std::exp(-2.0)));
    CHECK(close(bm::mean(d), 0.5));
    consistent(d, 0.7);
}
TEST_CASE("doc: extreme_dist") {
    bm::extreme_value d(0, 1);
    CHECK(close(bm::cdf(d, 0), std::exp(-1.0)));
    consistent(d, 0.5);
}
TEST_CASE("doc: f_dist") {
    bm::fisher_f d(5, 10);
    CHECK(close(bm::mean(d), 10.0 / 8));
    consistent(d, 2.0);
}
TEST_CASE("doc: f_eg") {
    // two-sample variance test: F = s1^2 / s2^2
    double F = 2.0, df1 = 20, df2 = 20;
    bm::fisher_f d(df1, df2);
    double p = bm::cdf(bm::complement(d, F));
    CHECK(p < 0.07);
    CHECK(p > 0.05);
}
TEST_CASE("doc: gamma_dist") {
    bm::gamma_distribution<> d(1, 2);  // shape 1: exponential with mean 2
    CHECK(close(bm::cdf(d, 2), 1 - std::exp(-1.0)));
    CHECK(close(bm::mean(d), 2));
    consistent(d, 1.5);
}
TEST_CASE("doc: geometric_dist") {
    bm::geometric d(0.5);
    CHECK(close(bm::pdf(d, 0), 0.5));
    CHECK(close(bm::cdf(d, 1), 0.75));
    CHECK(close(bm::mean(d), 1));
}
TEST_CASE("doc: hyperexponential_dist") {
    std::array<double, 2> probs{0.5, 0.5}, rates{1, 2};
    bm::hyperexponential d(probs, rates);
    CHECK(close(bm::mean(d), 0.5 * 1 + 0.5 * 0.5));
    CHECK(close(bm::cdf(d, 1), 0.5 * (1 - std::exp(-1.0)) + 0.5 * (1 - std::exp(-2.0))));
    CHECK(d.num_phases() == 2);
}
TEST_CASE("doc: hypergeometric_dist") {
    bm::hypergeometric d(5, 10, 20);  // 5 defective of 20, sample 10
    CHECK(close(bm::mean(d), 2.5));
    double sum = 0;
    for (unsigned k = 0; k <= 5; ++k)
        sum += bm::pdf(d, k);
    CHECK(close(sum, 1, 1e-12));
}
TEST_CASE("doc: inverse_chi_squared_dist") {
    bm::inverse_chi_squared d(10);
    CHECK(close(bm::mean(d), 1.0 / 8));
    consistent(d, 0.1);
}
TEST_CASE("doc: inverse_gamma_dist") {
    bm::inverse_gamma d(3, 2);
    CHECK(close(bm::mean(d), 1));
    consistent(d, 0.8);
}
TEST_CASE("doc: inverse_gaussian_dist") {
    bm::inverse_gaussian d(1, 2);
    CHECK(close(bm::mean(d), 1));
    consistent(d, 0.9);
}
TEST_CASE("doc: kolmogorov_smirnov_dist") {
    bm::kolmogorov_smirnov_distribution<> d(10);
    CHECK(bm::cdf(d, 0.0) == 0);
    CHECK(bm::cdf(d, 1.0) > 0.99);
    consistent(d, 0.3);
}
TEST_CASE("doc: laplace_dist") {
    bm::laplace d(0, 1);
    CHECK(close(bm::cdf(d, 0), 0.5));
    CHECK(close(bm::pdf(d, 0), 0.5));
    consistent(d, 1.3);
}
TEST_CASE("doc: logistic_dist") {
    bm::logistic d(0, 1);
    CHECK(close(bm::cdf(d, 0), 0.5));
    CHECK(close(bm::pdf(d, 0), 0.25));
    consistent(d, 2.0);
}
TEST_CASE("doc: lognormal_dist") {
    bm::lognormal d(0, 1);
    CHECK(close(bm::median(d), 1));
    CHECK(close(bm::mean(d), std::exp(0.5)));
    consistent(d, 2.5);
}
TEST_CASE("doc: nag_library") {
    // the NAG comparison page: binomial with the NAG convention (P(X >= k))
    bm::binomial d(20, 0.5);
    CHECK(close(bm::cdf(bm::complement(d, 9)), 1 - bm::cdf(d, 9), 1e-12));
    CHECK(close(bm::cdf(bm::complement(d, 9)) - bm::pdf(d, 10), 1 - bm::cdf(d, 10), 1e-12));
}
TEST_CASE("doc: nc_beta_dist") {
    bm::non_central_beta d(2, 3, 0);  // lambda = 0: the beta distribution
    CHECK(close(bm::cdf(d, 0.4), bm::cdf(bm::beta_distribution<>(2, 3), 0.4), 1e-10));
    CHECK(bm::non_central_beta(2, 3, 1).non_centrality() == 1);
}
TEST_CASE("doc: nc_chi_squared_dist") {
    bm::non_central_chi_squared d(4, 2);
    CHECK(close(bm::mean(d), 6));
    consistent(d, 5.0);
}
TEST_CASE("doc: nc_f_dist") {
    bm::non_central_f d(5, 10, 0);
    CHECK(close(bm::cdf(d, 2.0), bm::cdf(bm::fisher_f(5, 10), 2.0), 1e-10));
}
TEST_CASE("doc: nc_t_dist") {
    bm::non_central_t d(10, 0);
    CHECK(close(bm::cdf(d, 1.0), bm::cdf(bm::students_t(10), 1.0), 1e-10));
    CHECK(bm::mean(bm::non_central_t(10, 1)) > 1);
}
TEST_CASE("doc: negative_binomial_dist") {
    bm::negative_binomial d(1, 0.5);  // r = 1: geometric
    CHECK(close(bm::pdf(d, 0), 0.5));
    CHECK(close(bm::cdf(d, 2), 0.875));
    CHECK(close(bm::mean(d), 1));
}
TEST_CASE("doc: neg_binom_size_eg") {
    double n = bm::negative_binomial::find_minimum_number_of_trials(5, 0.5, 0.05);
    CHECK(n >= 5);
    CHECK(bm::negative_binomial::find_lower_bound_on_p(20, 5, 0.05) < 0.25);
}
TEST_CASE("doc: nmp") {
    bm::normal n(2, 3);
    CHECK(bm::mean(n) == 2);
    CHECK(bm::standard_deviation(n) == 3);
    CHECK(bm::variance(n) == 9);
    CHECK(bm::mode(n) == 2);
    CHECK(bm::median(n) == 2);
    CHECK(bm::skewness(n) == 0);
    CHECK(bm::kurtosis(n) == 3);
    CHECK(bm::kurtosis_excess(n) == 0);
    CHECK(close(bm::hazard(n, 2), bm::pdf(n, 2) / 0.5));
    CHECK(close(bm::chf(n, 2), std::log(2.0)));
    CHECK(bm::range(n).second >= std::numeric_limits<double>::max());
    CHECK(close(bm::entropy(bm::normal(0, 1)), 0.5 * std::log(2 * pi * std::exp(1.0))));
}
TEST_CASE("doc: normal_dist") {
    bm::normal n;
    CHECK(close(bm::cdf(n, 0), 0.5));
    CHECK(close(bm::pdf(n, 0), 1 / std::sqrt(2 * pi)));
    CHECK(close(bm::quantile(n, 0.975), 1.959963984540054, 1e-13));
    consistent(n, 1.0);
}
TEST_CASE("doc: pareto") {
    bm::pareto d(1, 2);
    CHECK(close(bm::cdf(d, 2), 0.75));
    CHECK(close(bm::mean(d), 2));
    consistent(d, 3.0);
}
TEST_CASE("doc: poisson_dist") {
    bm::poisson d(2);
    CHECK(close(bm::pdf(d, 0), std::exp(-2.0)));
    CHECK(close(bm::cdf(d, 1), 3 * std::exp(-2.0)));
    CHECK(bm::mean(d) == 2);
}
TEST_CASE("doc: rayleigh") {
    bm::rayleigh d(1);
    CHECK(close(bm::cdf(d, 1), 1 - std::exp(-0.5)));
    CHECK(close(bm::mean(d), std::sqrt(pi / 2)));
    consistent(d, 1.5);
}
TEST_CASE("doc: skew_normal_dist") {
    bm::skew_normal d(0, 1, 0);  // shape 0: the normal
    CHECK(close(bm::cdf(d, 1.0), bm::cdf(bm::normal(), 1.0), 1e-12));
    CHECK(bm::skewness(bm::skew_normal(0, 1, 3)) > 0);
}
TEST_CASE("doc: students_t_dist") {
    bm::students_t d(1);  // one degree of freedom: Cauchy
    CHECK(close(bm::cdf(d, 1), 0.75, 1e-12));
    consistent(bm::students_t(10), 2.0);
}
TEST_CASE("doc: tut_mean_intervals") {
    // interval for a mean of 9.1 from 30 samples with sd 0.5
    double mean = 9.1, sd = 0.5, n = 30;
    bm::students_t dist(n - 1);
    double t = bm::quantile(bm::complement(dist, 0.025));
    double w = t * sd / std::sqrt(n);
    CHECK(close(w, 0.1867, 1e-3));
    CHECK(mean - w < 9.0);
}
TEST_CASE("doc: tut_mean_size") {
    double df = bm::students_t::find_degrees_of_freedom(0.1, 0.05, 0.05, 1);
    CHECK(df > 100);
}
TEST_CASE("doc: tut_mean_test") {
    // Student's t-test of the mean against 5: t statistic and its probability
    double mean = 5.2, sd = 0.5, n = 25;
    double t = (mean - 5) * std::sqrt(n) / sd;
    bm::students_t dist(n - 1);
    CHECK(close(t, 2.0));
    CHECK(close(2 * bm::cdf(bm::complement(dist, t)), 0.05694, 1e-3));
}
TEST_CASE("doc: two_sample_students_t") {
    double m1 = 20, s1 = 6, n1 = 10, m2 = 25, s2 = 6, n2 = 10;
    double v = n1 + n2 - 2;
    double sp = std::sqrt(((n1 - 1) * s1 * s1 + (n2 - 1) * s2 * s2) / v);
    double t = (m1 - m2) / (sp * std::sqrt(1 / n1 + 1 / n2));
    bm::students_t dist(v);
    double p = 2 * bm::cdf(bm::complement(dist, std::abs(t)));
    CHECK(p > 0.05);
    CHECK(p < 0.1);
}
TEST_CASE("doc: triangular_dist") {
    bm::triangular d(0, 0.5, 1);
    CHECK(close(bm::cdf(d, 0.5), 0.5));
    CHECK(close(bm::pdf(d, 0.5), 2));
    consistent(d, 0.3);
}
TEST_CASE("doc: uniform_dist") {
    bm::uniform d(1, 3);
    CHECK(close(bm::cdf(d, 2), 0.5));
    CHECK(close(bm::pdf(d, 2), 0.5));
    consistent(d, 2.5);
}
TEST_CASE("doc: weibull_dist") {
    bm::weibull d(1, 2);  // shape 1: exponential with mean 2
    CHECK(close(bm::cdf(d, 2), 1 - std::exp(-1.0)));
    consistent(d, 1.0);
}
TEST_CASE("doc: variates") {
    // a distribution's quantile turns uniform variates into its own
    std::mt19937 gen(42);
    std::uniform_real_distribution<double> u(0, 1);
    bm::exponential d(1);
    double sum = 0;
    for (int i = 0; i < 20000; ++i)
        sum += bm::quantile(d, u(gen));
    CHECK(close(sum / 20000, 1.0, 0.05));
}
