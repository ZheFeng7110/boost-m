// mcpplibs.mwrap.boost.math — the numerical-methods pages: interpolators,
// quadrature, root finding and minimisation, optimisation, differentiation,
// statistics, filters and the internal tools (series, continued fractions,
// polynomials, rational functions, recurrences, minimax, expansions, AGM,
// color maps, ...). Each case runs the documented call on a problem with a
// known answer.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
import std;
import mcpplibs.mwrap.boost.math;

namespace bm = boost::math;

namespace {
bool close(double a, double b, double rel = 1e-10) {
    return std::abs(a - b) <= rel * std::max(1.0, std::max(std::abs(a), std::abs(b)));
}
const double pi = bm::constants::pi<double>();
}  // namespace

// ---- interpolators ----
TEST_CASE("doc: barycentric") {
    std::vector<double> x{0, 1, 2, 3}, y{0, 1, 4, 9};
    bm::interpolators::barycentric_rational<double> f(x.data(), y.data(), x.size());
    CHECK(close(f(2.0), 4));
    CHECK(close(f(1.5), 2.25, 1e-2));
}
TEST_CASE("doc: vector_barycentric") {
    using v2 = std::array<double, 2>;
    std::vector<double> t{0, 1, 2, 3};
    std::vector<v2> p{{0, 0}, {1, 1}, {2, 4}, {3, 9}};
    bm::interpolators::vector_barycentric_rational<std::vector<double>, std::vector<v2>> f(std::move(t), std::move(p));
    v2 r = f(2.0);
    CHECK(close(r[1], 4));
}
TEST_CASE("doc: bezier_polynomial") {
    using v2 = std::array<double, 2>;
    bm::interpolators::bezier_polynomial<std::vector<v2>> b(std::vector<v2>{{0, 0}, {1, 2}, {2, 0}});
    CHECK(close(b(0.5)[1], 1));
    CHECK(close(b(1.0)[0], 2));
}
TEST_CASE("doc: bilinear_uniform") {
    std::vector<double> v{0, 1, 1, 2};  // f(x, y) = x + y on a 2 x 2 grid
    bm::interpolators::bilinear_uniform<std::vector<double>> f(std::move(v), 2, 2);
    CHECK(close(f(0.5, 0.5), 1));
}
TEST_CASE("doc: cardinal_cubic_b") {
    std::vector<double> v(20);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = std::sin(0.1 * i);
    bm::interpolators::cardinal_cubic_b_spline<double> s(v.begin(), v.end(), 0.0, 0.1);
    CHECK(close(s(0.55), std::sin(0.55), 1e-4));
    CHECK(close(s.prime(0.55), std::cos(0.55), 1e-3));
}
TEST_CASE("doc: cardinal_quadratic_b") {
    std::vector<double> v(20);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = 2.0 * i;
    bm::interpolators::cardinal_quadratic_b_spline<double> s(v.data(), v.size(), 0.0, 1.0);
    CHECK(close(s(3.5), 7, 1e-6));
}
TEST_CASE("doc: cardinal_quintic_b") {
    std::vector<double> v(20);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = 3.0;
    bm::interpolators::cardinal_quintic_b_spline<double> s(v.data(), v.size(), 0.0, 1.0);
    CHECK(close(s(5.5), 3, 1e-8));
}
TEST_CASE("doc: catmull_rom") {
    using v2 = std::array<double, 2>;
    std::vector<v2> p{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    bm::catmull_rom<v2> cr(std::move(p));
    v2 a = cr(0.0);
    CHECK(close(a[0], 0));
    CHECK(cr.max_parameter() > 0);
}
TEST_CASE("doc: cubic_hermite") {
    std::vector<double> x{0, 1, 2}, y{0, 1, 4}, dy{0, 2, 4};
    bm::interpolators::cubic_hermite<std::vector<double>> f(std::move(x), std::move(y), std::move(dy));
    CHECK(close(f(1.5), 2.25));  // x^2 exactly
}
TEST_CASE("doc: quintic_hermite") {
    std::vector<double> x{0, 1, 2}, y{0, 1, 4}, dy{0, 2, 4}, d2y{2, 2, 2};
    bm::interpolators::quintic_hermite<std::vector<double>> f(std::move(x), std::move(y), std::move(dy), std::move(d2y));
    CHECK(close(f(0.5), 0.25));
}
TEST_CASE("doc: makima") {
    std::vector<double> x{0, 1, 2, 3, 4}, y{0, 1, 2, 3, 4};
    bm::interpolators::makima<std::vector<double>> f(std::move(x), std::move(y));
    CHECK(close(f(2.5), 2.5));
}
TEST_CASE("doc: pchip") {
    std::vector<double> x{0, 1, 2, 3}, y{0, 1, 1, 2};
    bm::interpolators::pchip<std::vector<double>> f(std::move(x), std::move(y));
    CHECK(close(f(1.5), 1));  // monotone: flat where the data is flat
}
TEST_CASE("doc: whittaker_shannon") {
    std::vector<double> v{1, 1, 1, 1, 1, 1, 1, 1};
    bm::interpolators::whittaker_shannon<std::vector<double>> f(std::move(v), 0.0, 1.0);
    CHECK(close(f(3.0), 1));
}

// ---- quadrature ----
TEST_CASE("doc: trapezoidal") {
    double I = bm::quadrature::trapezoidal([](double x) { return 1 / (5 - 4 * std::cos(x)); }, 0.0, 2 * pi);
    CHECK(close(I, 2 * pi / 3, 1e-10));
}
TEST_CASE("doc: gauss") {
    double I = bm::quadrature::gauss<double, 7>::integrate([](double x) { return x * x; }, 0.0, 1.0);
    CHECK(close(I, 1.0 / 3, 1e-14));
}
TEST_CASE("doc: gauss_kronrod") {
    double err;
    double I = bm::quadrature::gauss_kronrod<double, 15>::integrate([](double x) { return std::exp(x); }, 0.0, 1.0, 5, 1e-12, &err);
    CHECK(close(I, std::exp(1.0) - 1, 1e-13));
}
TEST_CASE("doc: de_overview") {
    bm::quadrature::tanh_sinh<double> integrator;
    CHECK(close(integrator.integrate([](double x) { return std::sqrt(1 - x * x); }, -1.0, 1.0), pi / 2, 1e-12));
}
TEST_CASE("doc: de_tanh_sinh") {
    bm::quadrature::tanh_sinh<double> integrator;
    double I = integrator.integrate([](double x) { return std::log(x); }, 0.0, 1.0);  // endpoint singularity
    CHECK(close(I, -1, 1e-12));
}
TEST_CASE("doc: de_tanh_sinh_2_arg") {
    bm::quadrature::tanh_sinh<double> integrator;
    // the two-argument form: f(x, distance to the nearest endpoint)
    double I = integrator.integrate([](double x, double xc) { return x <= 0.5 ? 1 / std::sqrt(x) : 1 / std::sqrt(1 - xc); }, 0.0, 1.0);
    CHECK(close(I, 2, 1e-10));
}
TEST_CASE("doc: de_sinh_sinh") {
    bm::quadrature::sinh_sinh<double> integrator;
    CHECK(close(integrator.integrate([](double x) { return std::exp(-x * x); }), std::sqrt(pi), 1e-12));
}
TEST_CASE("doc: de_exp_sinh") {
    bm::quadrature::exp_sinh<double> integrator;
    CHECK(close(integrator.integrate([](double x) { return std::exp(-x); }, 0.0, std::numeric_limits<double>::infinity()), 1, 1e-12));
}
TEST_CASE("doc: de_levels") {
    double err, L1;
    std::size_t levels;
    bm::quadrature::tanh_sinh<double> integrator(4);  // at most 4 levels of refinement
    integrator.integrate([](double x) { return std::cos(x); }, 0.0, 1.0, 1e-9, &err, &L1, &levels);
    CHECK(levels <= 4);
    CHECK(close(L1, std::sin(1.0), 1e-6));
}
TEST_CASE("doc: de_caveats") {
    // integrands that vanish to zero fast near the endpoints still work; the
    // error estimate is reported
    bm::quadrature::tanh_sinh<double> integrator;
    double err;
    double I = integrator.integrate([](double x) { return x * x; }, 0.0, 1.0, std::sqrt(std::numeric_limits<double>::epsilon()), &err);
    CHECK(close(I, 1.0 / 3, 1e-12));
    CHECK(err < 1e-6);
}
TEST_CASE("doc: gpu_usage") {
    // the free function form (no integrator object), as on devices
    CHECK(close(bm::quadrature::gauss_kronrod<double, 31>::integrate([](double x) { return x; }, 0.0, 2.0), 2, 1e-14));
}
TEST_CASE("doc: naive_monte_carlo") {
    std::vector<std::pair<double, double>> bounds{{0.0, 1.0}, {0.0, 1.0}};
    bm::quadrature::naive_monte_carlo<double, std::function<double(std::vector<double> const&)>> mc(
        [](std::vector<double> const& x) { return x[0] * x[1]; }, bounds, 1e-2, true, 1);
    auto task = mc.integrate();
    CHECK(close(task.get(), 0.25, 0.1));
}
TEST_CASE("doc: fourier_integrals") {
    bm::quadrature::ooura_fourier_sin<double> sin_integrator;
    auto [I, rel_err] = sin_integrator.integrate([](double x) { return 1 / x; }, 1.0);
    CHECK(close(I, pi / 2, 1e-8));
    bm::quadrature::ooura_fourier_cos<double> cos_integrator;
    auto [J, e2] = cos_integrator.integrate([](double x) { return 1 / (x * x + 1); }, 1.0);
    CHECK(close(J, pi / (2 * std::exp(1.0)), 1e-8));
}
TEST_CASE("doc: wavelet_transforms") {
    auto f = [](double) { return 1.0; };
    bm::quadrature::daubechies_wavelet_transform<decltype(f), double, 4> Wf(f);
    CHECK(std::abs(Wf(1.0, 0.0)) < 1e-6);  // the wavelet has a vanishing mean
    bm::daubechies_wavelet<double, 4> psi;
    CHECK(psi.support().first < psi.support().second);
}

// ---- roots and minima ----
TEST_CASE("doc: roots_deriv") {
    auto f = [](double x) { return std::make_tuple(x * x * x - 27, 3 * x * x); };
    int digits = std::numeric_limits<double>::digits;
    CHECK(close(bm::tools::newton_raphson_iterate(f, 2.0, 0.0, 10.0, digits), 3, 1e-14));
    auto g = [](double x) { return std::make_tuple(x * x * x - 27, 3 * x * x, 6 * x); };
    CHECK(close(bm::tools::halley_iterate(g, 2.0, 0.0, 10.0, digits), 3, 1e-14));
    CHECK(close(bm::tools::schroder_iterate(g, 2.0, 0.0, 10.0, digits), 3, 1e-14));
}
TEST_CASE("doc: roots_noderiv") {
    auto f = [](double x) { return x * x - 2; };
    std::uintmax_t it = 50;
    auto r = bm::tools::toms748_solve(f, 0.0, 2.0, bm::tools::eps_tolerance<double>(), it);
    CHECK(close((r.first + r.second) / 2, std::sqrt(2.0), 1e-14));
}
TEST_CASE("doc: bisect") {
    auto r = bm::tools::bisect([](double x) { return x - 0.25; }, 0.0, 1.0, bm::tools::eps_tolerance<double>(30));
    CHECK(close(r.first, 0.25, 1e-8));
}
TEST_CASE("doc: bracket_solve") {
    std::uintmax_t it = 50;
    auto r = bm::tools::bracket_and_solve_root([](double x) { return x * x * x - 8; }, 1.0, 2.0, true,
                                              bm::tools::eps_tolerance<double>(), it);
    CHECK(close(r.first, 2, 1e-12));
}
TEST_CASE("doc: TOMS748") {
    std::uintmax_t it = 50;
    auto r = bm::tools::toms748_solve([](double x) { return std::exp(x) - 2; }, 0.0, 1.0, bm::tools::eps_tolerance<double>(), it);
    CHECK(close(r.first, std::log(2.0), 1e-14));
    CHECK(it < 50);
}
TEST_CASE("doc: root_termination") {
    bm::tools::eps_tolerance<double> tol(10);  // 10 bits
    CHECK(tol(1.0, 1.0001));
    CHECK_FALSE(tol(1.0, 1.1));
    bm::tools::equal_floor efl;
    CHECK(efl(2.3, 2.9));
}
TEST_CASE("doc: brent_minima") {
    auto r = bm::tools::brent_find_minima([](double x) { return (x - 1) * (x - 1) + 3; }, -4.0, 4.0, 20);
    CHECK(close(r.first, 1, 1e-5));
    CHECK(close(r.second, 3, 1e-10));
}
TEST_CASE("doc: cubic_roots") {
    auto r = bm::tools::cubic_roots(1.0, -6.0, 11.0, -6.0);  // (x-1)(x-2)(x-3)
    CHECK(close(r[0], 1, 1e-12));
    CHECK(close(r[2], 3, 1e-12));
}
TEST_CASE("doc: quartic_roots") {
    auto r = bm::tools::quartic_roots(1.0, 0.0, -5.0, 0.0, 4.0);  // (x^2-1)(x^2-4)
    std::vector<double> v(r.begin(), r.end());
    std::sort(v.begin(), v.end());
    CHECK(close(v[0], -2, 1e-12));
    CHECK(close(v[3], 2, 1e-12));
}
TEST_CASE("doc: cbrt_comparison") {
    // the comparison page's subject: the same root by each method
    int digits = std::numeric_limits<double>::digits;
    auto f = [](double x) { return std::make_tuple(x * x * x - 2, 3 * x * x, 6 * x); };
    double n = bm::tools::newton_raphson_iterate([](double x) { return std::make_tuple(x * x * x - 2, 3 * x * x); }, 1.0, 0.0, 2.0, digits);
    double h = bm::tools::halley_iterate(f, 1.0, 0.0, 2.0, digits);
    CHECK(close(n, std::cbrt(2.0), 1e-15));
    CHECK(close(h, std::cbrt(2.0), 1e-15));
}

// ---- optimisation ----
namespace {
double sphere(std::vector<double> const& v) {
    double s = 0;
    for (double x : v)
        s += (x - 1) * (x - 1);
    return s;
}
template <class Params>
void bounds(Params& p) {
    p.lower_bounds = {-5, -5};
    p.upper_bounds = {5, 5};
}
}  // namespace
TEST_CASE("doc: differential_evolution") {
    bm::optimization::differential_evolution_parameters<std::vector<double>> p;
    bounds(p);
    std::mt19937_64 gen(1);
    auto x = bm::optimization::differential_evolution(sphere, p, gen);
    CHECK(sphere(x) < 1e-6);
}
TEST_CASE("doc: jso") {
    bm::optimization::jso_parameters<std::vector<double>> p;
    bounds(p);
    std::mt19937_64 gen(1);
    auto x = bm::optimization::jso(sphere, p, gen);
    CHECK(sphere(x) < 1e-6);
}
TEST_CASE("doc: random_search") {
    bm::optimization::random_search_parameters<std::vector<double>> p;
    bounds(p);
    p.max_function_calls = 20000;
    std::mt19937_64 gen(1);
    auto x = bm::optimization::random_search(sphere, p, gen);
    CHECK(sphere(x) < 0.1);
}
namespace {
using rv = bm::differentiation::reverse_mode::rvar<double, 1>;
rv quadratic(std::vector<rv>& x) {
    rv s = 0;
    for (auto& xi : x)
        s += (xi - 1.0) * (xi - 1.0);
    return s;
}
}  // namespace
TEST_CASE("doc: gradient_descent") {
    std::vector<rv> x{rv(3.0), rv(-2.0)};
    auto opt = bm::optimization::make_gradient_descent(&quadratic, x, 0.1);
    bm::optimization::minimize(opt);
    CHECK(close(x[0].item(), 1, 1e-2));
}
TEST_CASE("doc: nesterov") {
    std::vector<rv> x{rv(3.0), rv(-2.0)};
    auto opt = bm::optimization::make_nag(&quadratic, x, 0.05);
    bm::optimization::minimize(opt);
    CHECK(close(x[1].item(), 1, 1e-2));
}
TEST_CASE("doc: lbfgs") {
    std::vector<rv> x{rv(3.0), rv(-2.0)};
    auto opt = bm::optimization::make_lbfgs(&quadratic, x);
    bm::optimization::minimize(opt);
    CHECK(close(x[0].item(), 1, 1e-3));
}
TEST_CASE("doc: minimize") {
    std::vector<rv> x{rv(0.0)};
    auto opt = bm::optimization::make_gradient_descent(&quadratic, x, 0.1);
    auto result = bm::optimization::minimize(opt);
    CHECK(result.converged);
}
TEST_CASE("doc: policies") {
    // the optimisers' policies: initialisation, objective evaluation, line search
    std::vector<rv> x{rv(5.0)};
    auto opt = bm::optimization::make_lbfgs(&quadratic, x);
    auto result = bm::optimization::minimize(opt);
    CHECK(result.num_iter > 0);
}

// ---- differentiation ----
TEST_CASE("doc: numerical_differentiation#diff") {
    double d = bm::differentiation::finite_difference_derivative([](double x) { return std::sin(x); }, 1.0);
    CHECK(close(d, std::cos(1.0), 1e-12));
    double c = bm::differentiation::complex_step_derivative([](std::complex<double> z) { return std::exp(z); }, 0.5);
    CHECK(close(c, std::exp(0.5), 1e-15));
}
TEST_CASE("doc: lanczos_smoothing#diff") {
    std::vector<double> v(100);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = 0.5 * i;  // slope 0.5 per sample
    bm::differentiation::discrete_lanczos_derivative lanczos(1.0);
    auto d = lanczos(v);
    CHECK(close(d[50], 0.5, 1e-10));
}
TEST_CASE("doc: autodiff#autodiff") {
    using namespace bm::differentiation;
    auto x = make_fvar<double, 3>(2.0);
    auto y = x * x * x;
    CHECK(close(y.derivative(1), 12));
    CHECK(close(y.derivative(2), 12));
    CHECK(close(y.derivative(3), 6));
}
TEST_CASE("doc: autodiff_reverse#autodiff") {
    using namespace bm::differentiation::reverse_mode;
    rvar<double, 1> x = 3.0, y = 4.0;
    rvar<double, 1> z = x * y + sin(x);
    z.backward();
    CHECK(close(x.adjoint(), 4 + std::cos(3.0)));
    CHECK(close(y.adjoint(), 3));
}

// ---- statistics ----
TEST_CASE("doc: univariate_statistics") {
    std::vector<double> v{1, 2, 3, 4};
    CHECK(bm::statistics::mean(v) == 2.5);
    CHECK(close(bm::statistics::sample_variance(v), 5.0 / 3));
    CHECK(bm::statistics::median(v) == 2.5);
    CHECK(close(bm::statistics::gini_coefficient(v), 0.25));
}
TEST_CASE("doc: bivariate_statistics") {
    std::vector<double> x{1, 2, 3}, y{2, 4, 6};
    CHECK(close(bm::statistics::correlation_coefficient(x, y), 1));
    CHECK(close(bm::statistics::covariance(x, y), 4.0 / 3));
}
TEST_CASE("doc: signal_statistics") {
    std::vector<double> v{1, 0, 0, 0};
    CHECK(close(bm::statistics::hoyer_sparsity(v), 1));
    CHECK(bm::statistics::absolute_gini_coefficient(v) > 0.5);
}
TEST_CASE("doc: linear_regression") {
    std::vector<double> x{0, 1, 2, 3}, y{1, 3, 5, 7};
    auto [c0, c1, r2] = bm::statistics::simple_ordinary_least_squares_with_R_squared(x, y);
    CHECK(close(c0, 1));
    CHECK(close(c1, 2));
    CHECK(close(r2, 1));
}
TEST_CASE("doc: t_test") {
    std::vector<double> v{5.1, 4.9, 5.3, 5.0, 4.8};
    auto [t, p] = bm::statistics::one_sample_t_test(v, 5.0);
    CHECK(std::abs(t) < 1);
    CHECK(p > 0.1);  // no evidence against a mean of 5
    CHECK(p <= 1);
}
TEST_CASE("doc: z_test") {
    std::vector<double> v{5.1, 4.9, 5.3, 5.0, 4.8};
    auto [z, p] = bm::statistics::one_sample_z_test(v, 5.0);
    CHECK(std::isfinite(p));
    CHECK(std::isfinite(z));
}
TEST_CASE("doc: ljung_box") {
    std::vector<double> v{1, -1, 1, -1, 1, -1, 1, -1, 1, -1};
    auto [Q, p] = bm::statistics::ljung_box(v, 1);
    CHECK(Q > 5);
    CHECK(p < 0.05);
}
TEST_CASE("doc: runs_test") {
    std::vector<double> v{1, 2, 1, 2, 1, 2, 1, 2};
    auto [t, p] = bm::statistics::runs_above_and_below_median(v);
    CHECK(std::isfinite(t));
    CHECK(p >= 0);
}
TEST_CASE("doc: anderson_darling") {
    std::vector<double> v(100);
    for (std::size_t i = 0; i < v.size(); ++i)
        v[i] = bm::quantile(bm::normal(), (i + 0.5) / v.size());
    double A2 = bm::statistics::anderson_darling_normality_statistic(v, 0.0, 1.0);
    CHECK(A2 < 0.1);
}
TEST_CASE("doc: chatterjee_correlation") {
    std::vector<double> x{1, 2, 3, 4, 5}, y{1, 4, 9, 16, 25};
    CHECK(bm::statistics::chatterjee_correlation(x, y) > 0.3);
}

// ---- filters, internals ----
TEST_CASE("doc: daubechies_filters") {
    auto h = bm::filters::daubechies_scaling_filter<double, 3>();
    auto g = bm::filters::daubechies_wavelet_filter<double, 3>();
    CHECK(h.size() == 6);
    double s = 0;
    for (double x : g)
        s += x;
    CHECK(std::abs(s) < 1e-14);
}
TEST_CASE("doc: agm") { CHECK(close(bm::tools::agm(1.0, std::sqrt(2.0)), 1.1981402347355922, 1e-15)); }
TEST_CASE("doc: series_evaluation") {
    struct exp_series {
        using result_type = double;
        double x, term = 1;
        int k = 0;
        double operator()() {
            double r = term;
            term *= x / ++k;
            return r;
        }
    };
    std::uintmax_t max_terms = 100;
    exp_series e1{1.0}, e2{2.0};
    double r = bm::tools::sum_series(e1, std::numeric_limits<double>::epsilon(), max_terms);
    CHECK(close(r, std::exp(1.0), 1e-15));
    max_terms = 100;
    CHECK(close(bm::tools::kahan_sum_series(e2, 53, max_terms), std::exp(2.0), 1e-14));
}
TEST_CASE("doc: cf") {
    // golden ratio: [1; 1, 1, ...]
    struct ones {
        using result_type = double;
        double operator()() { return 1.0; }
    } gen;
    // 1 + 1/(1 + 1/(1 + ...)) = the golden ratio
    double g = bm::tools::continued_fraction_b(gen, std::numeric_limits<double>::epsilon());
    CHECK(close(g, (1 + std::sqrt(5.0)) / 2, 1e-12));
}
TEST_CASE("doc: polynomials") {
    bm::tools::polynomial<double> p{{1, 2}};  // 1 + 2x
    auto q = p * p;
    CHECK(q.degree() == 2);
    CHECK(q(1.0) == 9);
    auto [quot, rem] = bm::tools::quotient_remainder(q, p);
    CHECK(quot == p);
    CHECK(rem == bm::tools::polynomial<double>(0.0));
}
TEST_CASE("doc: rational") {
    std::array<double, 3> c{1, 2, 3};
    CHECK(bm::tools::evaluate_polynomial(c, 2.0) == 17);
    std::array<double, 2> num{1, 1}, den{1, 0};
    CHECK(bm::tools::evaluate_rational(num, den, 3.0) == 4);
    CHECK(bm::tools::evaluate_even_polynomial(c, 2.0) == 1 + 8 + 48);
}
TEST_CASE("doc: estrin") {
    std::array<double, 4> c{1, 1, 1, 1};
    CHECK(close(bm::tools::evaluate_polynomial_estrin(c, 2.0), 15));
}
TEST_CASE("doc: recurrence") {
    // Bessel J recurrence, run backwards and normalised: J1(1) / J0(1)
    auto r = [](int n) { return std::make_tuple(1.0, -2.0 * n, 1.0); };
    std::uintmax_t max_iter = 1000;
    double ratio = bm::tools::function_ratio_from_backwards_recurrence(r, std::numeric_limits<double>::epsilon(), max_iter);
    CHECK(std::isfinite(ratio));
}
TEST_CASE("doc: tuples") {
    auto t = bm::make_tuple(1, 2.0);
    CHECK(bm::get<0>(t) == 1);
    static_assert(std::tuple_size_v<decltype(t)> == 2);
}
TEST_CASE("doc: simple_continued_fraction") {
    auto cf = bm::tools::simple_continued_fraction(std::sqrt(2.0));
    auto const& a = cf.partial_denominators();
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 2);
}
TEST_CASE("doc: centered_continued_fraction") {
    auto cf = bm::tools::centered_continued_fraction(bm::constants::pi<double>());
    CHECK(cf.partial_denominators()[0] == 3);
}
TEST_CASE("doc: luroth_expansion") {
    auto l = bm::tools::luroth_expansion(0.5);
    CHECK(l.digits().size() >= 1);
}
TEST_CASE("doc: engel_expansion") {
    auto e = bm::tools::engel_expansion(std::exp(1.0) - 1);
    auto const& d = e.digits();
    CHECK(d[0] == 1);
    CHECK(d[1] == 2);
    CHECK(d[2] == 3);
}
TEST_CASE("doc: cohen_acceleration") {
    struct alternating {  // 1 - 1/2 + 1/3 - ... = log 2
        using result_type = double;
        int k = 0;
        double operator()() { return 1.0 / ++k; }
    };
    alternating g;
    CHECK(close(bm::tools::cohen_acceleration(g), std::log(2.0), 1e-12));
}
TEST_CASE("doc: color_maps") {
    auto c = bm::tools::viridis(0.5);
    CHECK(c[0] >= 0);
    CHECK(c[1] <= 1);
    auto px = bm::tools::to_8bit_rgba(c);
    CHECK(px[3] == 255);
}
TEST_CASE("doc: minimax") {
    // the minimax tool is a separate program; what it produces is used through
    // evaluate_rational, as above
    std::array<double, 2> num{0, 1}, den{1, 0};
    CHECK(bm::tools::evaluate_rational(num, den, 0.5) == 0.5);
}
TEST_CASE("doc: error_test") {
    CHECK(close(bm::relative_difference(1.0, 1.0 + 1e-10), 1e-10, 1e-5));
}
