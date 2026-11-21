// mcpplibs.mwrap.boost.math — the official examples (libs/math/example/*.cpp, part 2 of 4),
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone. The
// headers of the macros the examples use (BOOST_MATH_DECLARE_*, BOOST_MATH_ASSERT,
// BOOST_FLOAT64_C, BOOST_SERIALIZATION_NVP) are #included before the imports.
// Outputs with random seeds, timings, the float layout or the C++/C library's
// text are not compared (random_search, inspect_fp, naive_monte_carlo,
// reverse_mode_linear_regression, differential_evolution; root_finding_example:
// std::cbrt's last digit; autodiff_reverse_black_scholes: last digits of libm results;
// nonfinite_num_facet_trap: the iostream_category message). The outputs are
// those of the oracle's platform, x86_64 Linux: elsewhere the width of long
// double and the libm's last digits differ, and a case checks the exit status.
#if defined(__x86_64__) && defined(__linux__)
#define EX_OUTPUT_PLATFORM 1
#else
#define EX_OUTPUT_PLATFORM 0
#endif
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <boost/math/tools/config.hpp>
#include <boost/assert.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/distributions/fwd.hpp>
#include <boost/serialization/nvp.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <cmath>
#include <cerrno>
import std;
import mcpplibs.mwrap.boost.math;
import mcpplibs.mwrap.boost.multiprecision;
import mcpplibs.mwrap.boost.lexical_cast;
import mcpplibs.mwrap.boost.array;
import mcpplibs.mwrap.boost.random;
import mcpplibs.mwrap.boost.range;
import mcpplibs.mwrap.boost.endian;
import mcpplibs.mwrap.boost.format;
import mcpplibs.mwrap.boost.serialization;
import mcpplibs.mwrap.boost.type_traits;

// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\n';
    return 0;
}

// The examples write their SVG and data files to the current directory: the
// cases run in the temporary directory.
inline const bool ex_cwd_is_tmp = (std::filesystem::current_path(std::filesystem::temp_directory_path()), true);

// libs/math/example/airy_ulps_plot.cpp
namespace ex_airy_ulps_plot {
using boost::math::tools::ulps_plot;

int example_main() {
    using PreciseReal = long double;
    using CoarseReal = float;

    typedef boost::math::policies::policy<
      boost::math::policies::promote_float<false>,
      boost::math::policies::promote_double<false> >
      no_promote_policy;

    auto ai_coarse = [](CoarseReal x) {
        return boost::math::airy_ai<CoarseReal>(x, no_promote_policy());
    };
    auto ai_precise = [](PreciseReal x) {
        return boost::math::airy_ai<PreciseReal>(x, no_promote_policy());
    };

    std::string filename = "airy_ai_" + boost::core::demangle(typeid(CoarseReal).name()) + ".svg";
    int samples = 10000;
    // How many pixels wide do you want your .svg?
    int width = 700;
    // Near a root, we have unbounded relative error. So for functions with roots, we define an ULP clip:
    PreciseReal clip = 2.5;
    // Should we perturb the abscissas? i.e., should we compute the high precision function f at x,
    // and the low precision function at the nearest representable x̂ to x?
    // Or should we compute both the high precision and low precision function at a low precision representable x̂?
    bool perturb_abscissas = false;
    auto plot = ulps_plot<decltype(ai_precise), PreciseReal, CoarseReal>(ai_precise, CoarseReal(-3), CoarseReal(3), samples, perturb_abscissas);
    // Note the argument chaining:
    plot.clip(clip).width(width);
    // Sometimes it's useful to set a title, but in many cases it's more useful to just use a caption.
    //std::string title = "Airy Ai ULP plot at " + boost::core::demangle(typeid(CoarseReal).name()) + " precision";
    //plot.title(title);
    plot.vertical_lines(6);
    plot.add_fn(ai_coarse);
    // You can write the plot to a stream:
    //ex_out << plot;
    // Or to a file:
    plot.write(filename);

    // Don't like the default dark theme?
    plot.background_color("white").font_color("black");
    filename =  "airy_ai_" + boost::core::demangle(typeid(CoarseReal).name()) + "_white.svg";
    plot.write(filename);

    // Don't like the envelope?
    plot.ulp_envelope(false);
    filename =  "airy_ai_" + boost::core::demangle(typeid(CoarseReal).name()) + "_white_no_envelope.svg";
    plot.write(filename);
    return 0;  // main's implicit return
}

TEST_CASE("example: airy_ulps_plot") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
#endif
}
}  // namespace ex_airy_ulps_plot

// libs/math/example/autodiff_black_scholes_brief.cpp
namespace ex_autodiff_black_scholes_brief {
using namespace boost::math::constants;
using namespace boost::math::differentiation;

// Equations and function/variable names are from
// https://en.wikipedia.org/wiki/Greeks_(finance)#Formulas_for_European_option_Greeks

// Standard normal cumulative distribution function
template <typename X>
X Phi(X const& x) {
  return 0.5 * erfc(-one_div_root_two<X>() * x);
}

enum class CP { call, put };

// Assume zero annual dividend yield (q=0).
template <typename Price, typename Sigma, typename Tau, typename Rate>
promote<Price, Sigma, Tau, Rate> black_scholes_option_price(CP cp,
                                                            double K,
                                                            Price const& S,
                                                            Sigma const& sigma,
                                                            Tau const& tau,
                                                            Rate const& r) {
  using namespace std;
  auto const d1 = (log(S / K) + (r + sigma * sigma / 2) * tau) / (sigma * sqrt(tau));
  auto const d2 = (log(S / K) + (r - sigma * sigma / 2) * tau) / (sigma * sqrt(tau));
  switch (cp) {
    case CP::call:
      return S * Phi(d1) - exp(-r * tau) * K * Phi(d2);
    case CP::put:
      return exp(-r * tau) * K * Phi(-d2) - S * Phi(-d1);
    default:
      throw std::runtime_error("Invalid CP value.");
  }
}

int example_main() {
  double const K = 100.0;                    // Strike price.
  auto const S = make_fvar<double, 2>(105);  // Stock price.
  double const sigma = 5;                    // Volatility.
  double const tau = 30.0 / 365;             // Time to expiration in years. (30 days).
  double const r = 1.25 / 100;               // Interest rate.
  auto const call_price = black_scholes_option_price(CP::call, K, S, sigma, tau, r);
  auto const put_price = black_scholes_option_price(CP::put, K, S, sigma, tau, r);

  ex_out << "black-scholes call price = " << call_price.derivative(0) << '\n'
            << "black-scholes put  price = " << put_price.derivative(0) << '\n'
            << "call delta = " << call_price.derivative(1) << '\n'
            << "put  delta = " << put_price.derivative(1) << '\n'
            << "call gamma = " << call_price.derivative(2) << '\n'
            << "put  gamma = " << put_price.derivative(2) << '\n';
  return 0;
    return 0;  // main's implicit return
}
/*
Output:
black-scholes call price = 56.5136
black-scholes put  price = 51.4109
call delta = 0.773818
put  delta = -0.226182
call gamma = 0.00199852
put  gamma = 0.00199852
**/

TEST_CASE("example: autodiff_black_scholes_brief") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(black-scholes call price = 56.5136
black-scholes put  price = 51.4109
call delta = 0.773818
put  delta = -0.226182
call gamma = 0.00199852
put  gamma = 0.00199852
)ex"));
#endif
}
}  // namespace ex_autodiff_black_scholes_brief

// libs/math/example/autodiff_reverse_black_scholes.cpp
namespace ex_autodiff_reverse_black_scholes {
using namespace boost::math::differentiation::reverse_mode;
using namespace boost::math::constants;

template<typename Real>
Real phi(Real const& x)
{
    return one_div_root_two_pi<Real>() * exp(-0.5 * x * x);
}

template<typename Real>
Real Phi(Real const& x)
{
    return 0.5 * erfc(-one_div_root_two<Real>() * x);
}

enum class CP { call, put };

template<typename T>
T black_scholes_option_price(CP cp, double K, T const& S, T const& sigma, T const& tau, T const& r)
{
    using namespace std;
    auto const d1 = (log(S / K) + (r + sigma * sigma / 2) * tau) / (sigma * sqrt(tau));
    auto const d2 = (log(S / K) + (r - sigma * sigma / 2) * tau) / (sigma * sqrt(tau));
    switch (cp) {
    case CP::call:
        return S * Phi<T>(d1) - exp(-r * tau) * K * Phi<T>(d2);
    case CP::put:
        return exp(-r * tau) * K * Phi<T>(-d2) - S * Phi<T>(-d1);
    default:
        throw runtime_error("Invalid CP value.");
    }
}

int example_main()
{
    double const    K         = 100.0;
    double          S_val     = 105.0;
    double          sigma_val = 5.0;
    double          tau_val   = 30.0 / 365;
    double          r_val     = 1.25 / 100;
    rvar<double, 3> S         = make_rvar<double, 3>(S_val);
    rvar<double, 3> sigma     = make_rvar<double, 3>(sigma_val);
    rvar<double, 3> tau       = make_rvar<double, 3>(tau_val);
    rvar<double, 3> r         = make_rvar<double, 3>(r_val);

    rvar<double, 3> call_price
        = black_scholes_option_price<rvar<double, 3>>(CP::call, K, S, sigma, tau, r);
    rvar<double, 3> put_price
        = black_scholes_option_price<rvar<double, 3>>(CP::put, K, S, sigma, tau, r);

    double const d1 = ((log(S_val / K) + (r_val + sigma_val * sigma_val / 2) * tau_val)
                       / (sigma_val * sqrt(tau_val)));
    double const d2 = ((log(S_val / K) + (r_val - sigma_val * sigma_val / 2) * tau_val)
                       / (sigma_val * sqrt(tau_val)));
    double const formula_call_delta = +Phi(+d1);
    double const formula_put_delta  = -Phi(-d1);
    double const formula_vega       = (S_val * phi(d1) * sqrt(tau_val));
    double const formula_call_theta = (-S_val * phi(d1) * sigma_val / (2 * sqrt(tau_val))
                                       - r_val * K * exp(-r_val * tau_val) * Phi(+d2));

    double const formula_put_theta = (-S_val * phi(d1) * sigma_val / (2 * sqrt(tau_val))
                                      + r_val * K * exp(-r_val * tau_val) * Phi(-d2));
    double const formula_call_rho  = (+K * tau_val * exp(-r_val * tau_val) * Phi(+d2));
    double const formula_put_rho   = (-K * tau_val * exp(-r_val * tau_val) * Phi(-d2));
    double const formula_gamma     = (phi(d1) / (S_val * sigma_val * sqrt(tau_val)));
    double const formula_vanna     = (-phi(d1) * d2 / sigma_val);
    double const formula_charm  = (phi(d1) * (d2 * sigma_val * sqrt(tau_val) - 2 * r_val * tau_val)
                                  / (2 * tau_val * sigma_val * sqrt(tau_val)));
    double const formula_vomma  = (S_val * phi(d1) * sqrt(tau_val) * d1 * d2 / sigma_val);
    double const formula_veta   = (-S_val * phi(d1) * sqrt(tau_val)
                                 * (r_val * d1 / (sigma_val * sqrt(tau_val))
                                    - (1 + d1 * d2) / (2 * tau_val)));
    double const formula_speed  = (-phi(d1) * (d1 / (sigma_val * sqrt(tau_val)) + 1)
                                  / (S_val * S_val * sigma_val * sqrt(tau_val)));
    double const formula_zomma  = (phi(d1) * (d1 * d2 - 1)
                                  / (S_val * sigma_val * sigma_val * sqrt(tau_val)));
    double const formula_color  = (-phi(d1) / (2 * S_val * tau_val * sigma_val * sqrt(tau_val))
                                  * (1
                                     + (2 * r_val * tau_val - d2 * sigma_val * sqrt(tau_val)) * d1
                                           / (sigma_val * sqrt(tau_val))));
    double const formula_ultima = -formula_vega
                                  * ((d1 * d2 * (1 - d1 * d2) + d1 * d1 + d2 * d2)
                                     / (sigma_val * sigma_val));

    auto call_greeks = grad(call_price, &S, &sigma, &tau, &r);
    auto put_greeks  = grad(put_price, &S, &sigma, &tau, &r);

    auto call_greeks_2nd_order = grad_nd<2>(call_price, &S, &sigma, &tau, &r);
    auto put_greeks_2nd_order  = grad_nd<2>(put_price, &S, &sigma, &tau, &r);

    auto call_greeks_3rd_order = grad_nd<3>(call_price, &S, &sigma, &tau, &r);
    auto put_greeks_3rd_order  = grad_nd<3>(put_price, &S, &sigma, &tau, &r);

    ex_out << std::setprecision(std::numeric_limits<double>::digits10)
              << "autodiff black-scholes call price = " << call_price.item() << "\n"
              << "autodiff black-scholes put price = " << put_price.item() << "\n"
              << "\n ## First-order Greeks \n"
              << "autodiff call delta = " << call_greeks[0].item() << "\n"
              << "formula call delta = " << formula_call_delta << "\n"
              << "autodiff call vega  = " << call_greeks[1].item() << '\n'
              << " formula call vega  = " << formula_vega << '\n'
              << "autodiff call theta = " << -call_greeks[2].item() << '\n'
              << " formula call theta = " << formula_call_theta << '\n'
              << "autodiff call rho   = " << call_greeks[3].item() << 'n'
              << " formula call rho   = " << formula_call_rho << '\n'
              << '\n'
              << "autodiff put delta = " << put_greeks[0].item() << 'n'
              << " formula put delta = " << formula_put_delta << '\n'
              << "autodiff put vega  = " << put_greeks[1].item() << '\n'
              << " formula put vega  = " << formula_vega << '\n'
              << "autodiff put theta = " << -put_greeks[2].item() << '\n'
              << " formula put theta = " << formula_put_theta << '\n'
              << "autodiff put rho   = " << put_greeks[3].item() << '\n'
              << " formula put rho   = " << formula_put_rho << '\n'

              << "\n## Second-order Greeks\n"
              << "autodiff call gamma = " << call_greeks_2nd_order[0][0].item() << '\n'
              << "autodiff put  gamma = " << put_greeks_2nd_order[0][0].item() << '\n'
              << "      formula gamma = " << formula_gamma << '\n'
              << "autodiff call vanna = " << call_greeks_2nd_order[0][1].item() << '\n'
              << "autodiff put  vanna = " << put_greeks_2nd_order[0][1].item() << '\n'
              << "      formula vanna = " << formula_vanna << '\n'
              << "autodiff call charm = " << -call_greeks_2nd_order[0][2].item() << '\n'
              << "autodiff put  charm = " << -put_greeks_2nd_order[0][2].item() << '\n'
              << "      formula charm = " << formula_charm << '\n'
              << "autodiff call vomma = " << call_greeks_2nd_order[1][1].item() << '\n'
              << "autodiff put  vomma = " << put_greeks_2nd_order[1][1].item() << '\n'
              << "      formula vomma = " << formula_vomma << '\n'
              << "autodiff call veta = " << call_greeks_2nd_order[1][2].item() << '\n'
              << "autodiff put  veta = " << put_greeks_2nd_order[1][2].item() << '\n'
              << "      formula veta = " << formula_veta << '\n'

              << "\n## Third-order Greeks\n"
              << "autodiff call speed = " << call_greeks_3rd_order[0][0][0] << '\n'
              << "autodiff put  speed = " << put_greeks_3rd_order[0][0][0] << '\n'
              << "      formula speed = " << formula_speed << '\n'
              << "autodiff call zomma = " << call_greeks_3rd_order[0][0][1] << '\n'
              << "autodiff put  zomma = " << put_greeks_3rd_order[0][0][1] << '\n'
              << "      formula zomma = " << formula_zomma << '\n'
              << "autodiff call color = " << call_greeks_3rd_order[0][0][2] << '\n'
              << "autodiff put  color = " << put_greeks_3rd_order[0][0][2] << '\n'
              << "      formula color = " << formula_color << '\n'
              << "autodiff call ultima = " << call_greeks_3rd_order[1][1][1] << '\n'
              << "autodiff put  ultima = " << put_greeks_3rd_order[1][1][1] << '\n'
              << "      formula ultima = " << formula_ultima << '\n';
    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: autodiff_reverse_black_scholes") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(autodiff black-scholes call price = 56.5136030677739
autodiff black-scholes put price = 51.4109161009333

 ## First-order Greeks 
autodiff call delta = 0.773818444921274
formula call delta = 0.773818444921274
autodiff call vega  = 9.05493427705736
 formula call vega  = 9.05493427705736
autodiff call theta = -275.73013426444
 formula call theta = -275.73013426444
autodiff call rho   = 2.03320550539396n formula call rho   = 2.03320550539396

autodiff put delta = -0.226181555078726n formula put delta = -0.226181555078726
autodiff put vega  = 9.05493427705736
 formula put vega  = 9.05493427705736
autodiff put theta = -274.481417851526
 formula put theta = -274.481417851526
autodiff put rho   = -6.17753255212599
 formula put rho   = -6.17753255212599

## Second-order Greeks
autodiff call gamma = 0.00199851912993254
autodiff put  gamma = 0.00199851912993254
      formula gamma = 0.00199851912993254
autodiff call vanna = 0.0410279463126531
autodiff put  vanna = 0.0410279463126531
      formula vanna = 0.0410279463126531
autodiff call charm = -1.2505564233679
autodiff put  charm = -1.2505564233679
      formula charm = -1.2505564233679
autodiff call vomma = -0.928114149313108
autodiff put  vomma = -0.928114149313108
      formula vomma = -0.928114149313107
autodiff call veta = 26.7947073115641
autodiff put  veta = 26.794707311564
      formula veta = 26.7947073115641

## Third-order Greeks
autodiff call speed = -2.90117322380992e-05
autodiff put  speed = -2.90117322380992e-05
      formula speed = -2.90117322380992e-05
autodiff call zomma = -0.000604548369901419
autodiff put  zomma = -0.000604548369901419
      formula zomma = -0.000604548369901419
autodiff call color = -0.0184014426606065
autodiff put  color = -0.0184014426606065
      formula color = -0.0184014426606065
autodiff call ultima = -0.0922426864775689
autodiff put  ultima = -0.092242686477569
      formula ultima = -0.0922426864775685
)ex"));
#endif
}
}  // namespace ex_autodiff_reverse_black_scholes

// libs/math/example/bessel_errors_example.cpp
namespace ex_bessel_errors_example {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif


// Weisstein, Eric W. "Bessel Function Zeros." From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/BesselFunctionZeros.html
// Test values can be calculated using [@wolframalpha.com WolframAlpha]
// See also http://dlmf.nist.gov/10.21

//[bessel_errors_example_1

/*`[h5 Error messages from 'bad' input]

Another example demonstrates calculating zeros of the Bessel functions
showing the error messages from 'bad' input is handled by throwing exceptions.

To use the functions for finding zeros of the functions we need:
*/

//] [/bessel_errors_example_1]

int example_main()
{
//[bessel_errors_example_2

/*`[tip It is always wise to place all code using Boost.Math inside try'n'catch blocks;
this will ensure that helpful error messages can be shown when exceptional conditions arise.]

Examples below show messages from several 'bad' arguments that throw a `domain_error` exception.
*/
  try
  { // Try a zero order v.
    float dodgy_root = boost::math::cyl_bessel_j_zero(0.F, 0);
    ex_out << "boost::math::cyl_bessel_j_zero(0.F, 0) " << dodgy_root << std::endl;
    // Thrown exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int):
    // Requested the 0'th zero of J0, but the rank must be > 0 !
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

/*`[note The type shown in the error message is the type [*after promotion],
using __precision_policy and __promotion_policy, from `float` to `double` in this case.]

In this example the promotion goes:

# Arguments are `float` and `int`.
# Treat `int` "as if" it were a `double`, so arguments are `float` and `double`.
# Common type is `double` - so that's the precision we want (and the type that will be returned).
# Evaluate internally as `double` for full `float` precision.

See full code for other examples that promote from `double` to `long double`.

Other examples of 'bad' inputs like infinity and NaN are below.
Some compiler warnings indicate that 'bad' values are detected at compile time.
*/

  try
  { // order v = inf
     ex_out << "boost::math::cyl_bessel_j_zero(inf, 1) " << std::endl;
     double inf = std::numeric_limits<double>::infinity();
     double inf_root = boost::math::cyl_bessel_j_zero(inf, 1);
     ex_out << "boost::math::cyl_bessel_j_zero(inf, 1) " << inf_root << std::endl;
     // Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, unsigned):
     // Order argument is 1.#INF, but must be finite >= 0 !
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

  try
  { // order v = NaN, rank m = 1
     ex_out << "boost::math::cyl_bessel_j_zero(nan, 1) " << std::endl;
     double nan = std::numeric_limits<double>::quiet_NaN();
     double nan_root = boost::math::cyl_bessel_j_zero(nan, 1);
     ex_out << "boost::math::cyl_bessel_j_zero(nan, 1) " << nan_root << std::endl;
     // Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, unsigned):
     // Order argument is 1.#QNAN, but must be finite >= 0 !
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

/*`The output from other examples are shown appended to the full code listing.
*/
//] [/bessel_errors_example_2]
  try
  {   // Try a zero rank m.
    ex_out << "boost::math::cyl_neumann_zero(0.0, 0) " << std::endl;
    double dodgy_root = boost::math::cyl_bessel_j_zero(0.0, 0);
    //  warning C4146: unary minus operator applied to unsigned type, result still unsigned.
    ex_out << "boost::math::cyl_neumann_zero(0.0, -1) " << dodgy_root << std::endl;
    //  boost::math::cyl_neumann_zero(0.0, -1) 6.74652e+009
    // This *should* fail because m is unreasonably large.

  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

  try
  { // m = inf
   ex_out << "boost::math::cyl_bessel_j_zero(0.0, inf) " << std::endl;
   double inf = std::numeric_limits<double>::infinity();
     double inf_root = boost::math::cyl_bessel_j_zero(0.0, inf);
     // warning C4244: 'argument' : conversion from 'double' to 'int', possible loss of data.
     ex_out << "boost::math::cyl_bessel_j_zero(0.0, inf) " << inf_root << std::endl;
     // Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int):
     // Requested the 0'th zero, but must be > 0 !

  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

  try
  { // m = NaN
     double nan = std::numeric_limits<double>::quiet_NaN();
     double nan_root = boost::math::airy_ai_zero<double>(nan);
     // warning C4244: 'argument' : conversion from 'double' to 'int', possible loss of data.
     ex_out << "boost::math::airy_ai_zero<double>(nan) " << nan_root << std::endl;
     // Thrown exception Error in function boost::math::airy_ai_zero<double>(double,double):
     // The requested rank of the zero is 0, but must be 1 or more !
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }
     return 0;  // main's implicit return
} // int example_main()

/*
Output:

  Description: Autorun "J:\Cpp\big_number\Debug\bessel_errors_example.exe"
  Thrown exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int): Requested the 0'th zero of J0, but the rank must be > 0 !
  boost::math::cyl_bessel_j_zero(inf, 1) 
  Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is 1.#INF, but must be finite >= 0 !
  boost::math::cyl_bessel_j_zero(nan, 1) 
  Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is 1.#QNAN, but must be finite >= 0 !
  boost::math::cyl_neumann_zero(0.0, 0) 
  Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the 0'th zero of J0, but the rank must be > 0 !
  boost::math::cyl_bessel_j_zero(0.0, inf) 
  Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -2147483648'th zero, but the rank must be positive !
  Thrown exception Error in function boost::math::airy_ai_zero<double>(double,double): The requested rank of the zero is 0, but must be 1 or more !

 
*/

TEST_CASE("example: bessel_errors_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Thrown exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int): Requested the 0'th zero of J0, but the rank must be > 0 !
boost::math::cyl_bessel_j_zero(inf, 1) 
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is inf, but must be finite >= 0 !
boost::math::cyl_bessel_j_zero(nan, 1) 
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is nan, but must be finite >= 0 !
boost::math::cyl_neumann_zero(0.0, 0) 
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the 0'th zero of J0, but the rank must be > 0 !
boost::math::cyl_bessel_j_zero(0.0, inf) 
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -2147483648'th zero, but the rank must be positive !
Thrown exception Error in function boost::math::airy_ai_zero<long double>(long double, int): Requested the -2147483648'th zero, but the rank must be 1 or more !
)ex"));
#endif
}
}  // namespace ex_bessel_errors_example

// libs/math/example/big_seventh.cpp
namespace ex_big_seventh {
#ifdef _MSC_VER
#pragma warning(disable : 4512) // assignment operator could not be generated.
#pragma warning(disable : 4996)
#endif

//[big_seventh_example_1

/*`[h5 Using Boost.Multiprecision `cpp_float` types for numerical calculations with higher precision than built-in `long double`.]

The Boost.Multiprecision library can be used for computations requiring precision
exceeding that of standard built-in types such as `float`, `double`
and `long double`. For extended-precision calculations, Boost.Multiprecision
supplies several template data types called `cpp_bin_float_`.

The number of decimal digits of precision is fixed at compile-time via template parameter.

To use these floating-point types and 
[@https://www.boost.org/doc/libs/release/libs/math/doc/html/constants.html Boost.Math collection of high-precision constants],
we need some includes:
*/


// that includes some predefined typedefs that can be used thus:
// using boost::multiprecision::cpp_bin_float_quad;
// using boost::multiprecision::cpp_bin_float_50;
// using boost::multiprecision::cpp_bin_float_100;


/*` So now we can demonstrate with some trivial calculations:
*/

//] //[big_seventh_example_1]

void no_et()
{
  using namespace boost::multiprecision;

  ex_out.setf(std::ios_base::boolalpha);
  
   typedef number<backends::cpp_bin_float<113, backends::digit_base_2, void, std::int16_t, -16382, 16383>, et_on> cpp_bin_float_quad_et_on;
   typedef number<backends::cpp_bin_float<113, backends::digit_base_2, void, std::int16_t, -16382, 16383>, et_off> cpp_bin_float_quad_et_off;

   typedef number<backends::cpp_bin_float<113, backends::digit_base_2, void, std::int16_t, -16382, 16383>, et_off> cpp_bin_float_oct;


  cpp_bin_float_quad  x("42.");
  ex_out << "cpp_bin_float_quad x =  " << x << std::endl;

  cpp_bin_float_quad_et_on q("42.");

  ex_out << "std::is_same<cpp_bin_float_quad, cpp_bin_float_quad_et_off>::value is " << std::is_same<cpp_bin_float_quad, cpp_bin_float_quad_et_off>::value << std::endl; 
  ex_out << "std::is_same<cpp_bin_float_quad, cpp_bin_float_quad_et_on>::value is " << std::is_same<cpp_bin_float_quad, cpp_bin_float_quad_et_on>::value << std::endl; 

  ex_out << "cpp_bin_float_quad_et_on   q =  " << q << std::endl;
  cpp_bin_float_50   y("42.");  // typedef number<backends::cpp_bin_float<50> >  cpp_bin_float_50;

  ex_out << "cpp_bin_float_50   y =  " << y << std::endl;

  typedef number<backends::cpp_bin_float<50>, et_off > cpp_bin_float_50_no_et;
  typedef number<backends::cpp_bin_float<50>, et_on > cpp_bin_float_50_et;

  cpp_bin_float_50_no_et z("42.");  // typedef number<backends::cpp_bin_float<50> >  cpp_bin_float_50;

  ex_out << "cpp_bin_float_50_no_et   z =  " << z << std::endl;

  ex_out << " std::is_same<cpp_bin_float_50, cpp_bin_float_50_no_et>::value is " << std::is_same<cpp_bin_float_50, cpp_bin_float_50_no_et>::value << std::endl; 
  ex_out << " std::is_same<cpp_bin_float_50_et, cpp_bin_float_50_no_et>::value is " << std::is_same<cpp_bin_float_50_et, cpp_bin_float_50_no_et>::value << std::endl; 

} // void no_et()

int example_main()
{

  no_et();

  return 0;


   //[big_seventh_example_2
   /*`Using `typedef cpp_bin_float_50` hides the complexity of multiprecision,
allows us to define variables with 50 decimal digit precision just like built-in `double`.
*/
   using boost::multiprecision::cpp_bin_float_50;

   cpp_bin_float_50 seventh = cpp_bin_float_50(1) / 7; // 1 / 7

   /*`By default, output would only show the standard 6 decimal digits,
 so set precision to show all 50 significant digits, including any trailing zeros.
*/
   ex_out.precision(std::numeric_limits<cpp_bin_float_50>::digits10);
   ex_out << std::showpoint << std::endl; // Append any trailing zeros.
   ex_out << seventh << std::endl;
   /*`which outputs:

  0.14285714285714285714285714285714285714285714285714

We can also use __math_constants like [pi],
guaranteed to be initialized with the very last bit of precision (__ULP) for the floating-point type.
*/
   ex_out << "pi = " << boost::math::constants::pi<cpp_bin_float_50>() << std::endl;
   cpp_bin_float_50 circumference = boost::math::constants::pi<cpp_bin_float_50>() * 2 * seventh;
   ex_out << "c =  " << circumference << std::endl;

   /*`which outputs

  pi = 3.1415926535897932384626433832795028841971693993751

  c =  0.89759790102565521098932668093700082405633411410717
*/
   //]  [/big_seventh_example_2]

   //[big_seventh_example_3
   /*`So using `cpp_bin_float_50` looks like a simple 'drop-in' for the __fundamental_type like 'double',
but beware of loss of precision from construction or conversion from `double` or other lower precision types.
This is a mistake that is very easy to make, 
and very difficult to detect because the loss of precision is only visible after the 17th decimal digit.

We can show this by constructing from `double`, (avoiding the schoolboy-error `double d7 = 1 / 7;` giving zero!)
*/

   double d7 = 1. / 7; //
   ex_out << "d7 = " << d7 << std::endl;

   cpp_bin_float_50 seventh_0 = cpp_bin_float_50(1 / 7); // Avoid the schoolboy-error 1 / 7 == 0!)
   ex_out << "seventh_0 = " << seventh_0 << std::endl;
   // seventh_double0 = 0.0000000000000000000000000000000000000000000000000

   cpp_bin_float_50 seventh_double = cpp_bin_float_50(1. / 7);      // Construct from double!
   ex_out << "seventh_double = " << seventh_double << std::endl; // Boost.Multiprecision post-school error!
   // seventh_double = 0.14285714285714284921269268124888185411691665649414

   /*`Did you spot the mistake?  After the 17th decimal digit, result is random!

14285714285714 should be recurring.
*/

   cpp_bin_float_50 seventh_big(1); // 1
   seventh_big /= 7;
   ex_out << "seventh_big = " << seventh_big << std::endl; //
   // seventh_big     = 0.14285714285714285714285714285714285714285714285714
   /*`Note the recurring 14285714285714 pattern as expected.

As one would expect, the variable can be `const` (but sadly [*not yet `constexpr`]).
*/

   const cpp_bin_float_50 seventh_const(cpp_bin_float_50(1) / 7);
   ex_out << "seventh_const = " << seventh_const << std::endl; 
  // seventh_const = 0.14285714285714285714285714285714285714285714285714

/*`The full output is:
*/

//]  [/big_seventh_example_3

//[big_seventh_example_constexpr

// Sadly we cannot (yet) write:
// constexpr cpp_bin_float_50 any_constexpr(0);

// constexpr cpp_bin_float_50 seventh_constexpr (cpp_bin_float_50(1) / 7);
// ex_out << "seventh_constexpr = " << seventh_constexpr << std::endl; //
// constexpr cpp_bin_float_50 seventh_constexpr(seventh_const);

//]  [/big_seventh_example_constexpr

   return 0;
    return 0;  // main's implicit return
} // int example_main()

/*
//[big_seventh_example_output

0.14285714285714285714285714285714285714285714285714
pi = 3.1415926535897932384626433832795028841971693993751
c =  0.89759790102565521098932668093700082405633411410717
d7 = 0.14285714285714284921269268124888185411691665649414
seventh_0 = 0.0000000000000000000000000000000000000000000000000
seventh_double = 0.14285714285714284921269268124888185411691665649414
seventh_big = 0.14285714285714285714285714285714285714285714285714
seventh_const = 0.14285714285714285714285714285714285714285714285714

//] //[big_seventh_example_output]

*/

TEST_CASE("example: big_seventh") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(cpp_bin_float_quad x =  42
std::is_same<cpp_bin_float_quad, cpp_bin_float_quad_et_off>::value is true
std::is_same<cpp_bin_float_quad, cpp_bin_float_quad_et_on>::value is false
cpp_bin_float_quad_et_on   q =  42
cpp_bin_float_50   y =  42
cpp_bin_float_50_no_et   z =  42
 std::is_same<cpp_bin_float_50, cpp_bin_float_50_no_et>::value is true
 std::is_same<cpp_bin_float_50_et, cpp_bin_float_50_no_et>::value is false
)ex"));
#endif
}
}  // namespace ex_big_seventh

// libs/math/example/binomial_quiz_example.cpp
namespace ex_binomial_quiz_example {
  using boost::math::binomial;

   using std::endl;
  using std::ios; using std::flush; using std::left; using std::right; using std::fixed;
  using std::setw; using std::setprecision;
  


//][/binomial_quiz_example1]

int example_main()
{
  try
  {
  ex_out << "Binomial distribution example - guessing in a quiz." << endl;
//[binomial_quiz_example2
/*`
The number of correct answers, X, is distributed as a binomial random variable
with binomial distribution parameters: questions n and success fraction probability p.
So we construct a binomial distribution:
*/
  int questions = 16; // All the questions in the quiz.
  int answers = 4; // Possible answers to each question.
  double success_fraction = 1. / answers; // If a random guess, p = 1/4 = 0.25.
  binomial quiz(questions, success_fraction);
/*`
and display the distribution parameters we used thus:
*/
  ex_out << "In a quiz with " << quiz.trials()
    << " questions and with a probability of guessing right of "
    << quiz.success_fraction() * 100 << " %"
    << " or 1 in " << static_cast<int>(1. / quiz.success_fraction()) << endl;
/*`
Show a few probabilities of just guessing:
*/
  ex_out << "Probability of getting none right is " << pdf(quiz, 0) << endl; // 0.010023
  ex_out << "Probability of getting exactly one right is " << pdf(quiz, 1) << endl;
  ex_out << "Probability of getting exactly two right is " << pdf(quiz, 2) << endl;
  int pass_score = 11;
  ex_out << "Probability of getting exactly " << pass_score << " answers right by chance is "
    << pdf(quiz, pass_score) << endl;
  ex_out << "Probability of getting all " << questions << " answers right by chance is "
    << pdf(quiz, questions) << endl;
/*`
[pre
Probability of getting none right is 0.0100226
Probability of getting exactly one right is 0.0534538
Probability of getting exactly two right is 0.133635
Probability of getting exactly 11 right is 0.000247132
Probability of getting exactly all 16 answers right by chance is 2.32831e-010
]
These don't give any encouragement to guessers!

We can tabulate the 'getting exactly right' ( == ) probabilities thus:
*/
  ex_out << "\n" "Guessed Probability" << right << endl;
  for (int successes = 0; successes <= questions; successes++)
  {
    double probability = pdf(quiz, successes);
    ex_out << setw(2) << successes << "      " << probability << endl;
  }
  ex_out << endl;
/*`
[pre
Guessed Probability
 0      0.0100226
 1      0.0534538
 2      0.133635
 3      0.207876
 4      0.225199
 5      0.180159
 6      0.110097
 7      0.0524273
 8      0.0196602
 9      0.00582526
10      0.00135923
11      0.000247132
12      3.43239e-005
13      3.5204e-006
14      2.51457e-007
15      1.11759e-008
16      2.32831e-010
]
Then we can add the probabilities of some 'exactly right' like this:
*/
  ex_out << "Probability of getting none or one right is " << pdf(quiz, 0) + pdf(quiz, 1) << endl;

/*`
[pre
Probability of getting none or one right is 0.0634764
]
But if more than a couple of scores are involved, it is more convenient (and may be more accurate)
to use the Cumulative Distribution Function (cdf) instead:
*/
  ex_out << "Probability of getting none or one right is " << cdf(quiz, 1) << endl;
/*`
[pre
Probability of getting none or one right is 0.0634764
]
Since the cdf is inclusive, we can get the probability of getting up to 10 right ( <= )
*/
  ex_out << "Probability of getting <= 10 right (to fail) is " << cdf(quiz, 10) << endl;
/*`
[pre
Probability of getting <= 10 right (to fail) is 0.999715
]
To get the probability of getting 11 or more right (to pass),
it is tempting to use ``1 - cdf(quiz, 10)`` to get the probability of > 10
*/
  ex_out << "Probability of getting > 10 right (to pass) is " << 1 - cdf(quiz, 10) << endl;
/*`
[pre
Probability of getting > 10 right (to pass) is 0.000285239
]
But this should be resisted in favor of using the __complements function (see __why_complements).
*/
  ex_out << "Probability of getting > 10 right (to pass) is " << cdf(complement(quiz, 10)) << endl;
/*`
[pre
Probability of getting > 10 right (to pass) is 0.000285239
]
And we can check that these two, <= 10 and > 10,  add up to unity.
*/
BOOST_MATH_ASSERT((cdf(quiz, 10) + cdf(complement(quiz, 10))) == 1.);
/*`
If we want a < rather than a <= test, because the CDF is inclusive, we must subtract one from the score.
*/
  ex_out << "Probability of getting less than " << pass_score
    << " (< " << pass_score << ") answers right by guessing is "
    << cdf(quiz, pass_score -1) << endl;
/*`
[pre
Probability of getting less than 11 (< 11) answers right by guessing is 0.999715
]
and similarly to get a >= rather than a > test
we also need to subtract one from the score (and can again check the sum is unity).
This is because if the cdf is /inclusive/,
then its complement must be /exclusive/ otherwise there would be one possible
outcome counted twice!
*/
  ex_out << "Probability of getting at least " << pass_score
    << "(>= " << pass_score << ") answers right by guessing is "
    << cdf(complement(quiz, pass_score-1))
    << ", only 1 in " << 1/cdf(complement(quiz, pass_score-1)) << endl;

  BOOST_MATH_ASSERT((cdf(quiz, pass_score -1) + cdf(complement(quiz, pass_score-1))) == 1);

/*`
[pre
Probability of getting at least 11 (>= 11) answers right by guessing is 0.000285239, only 1 in 3505.83
]
Finally we can tabulate some probabilities:
*/
  ex_out << "\n" "At most (<=)""\n""Guessed OK   Probability" << right << endl;
  for (int score = 0; score <= questions; score++)
  {
    ex_out << setw(2) << score << "           " << setprecision(10)
      << cdf(quiz, score) << endl;
  }
  ex_out << endl;
/*`
[pre
At most (<=)
Guessed OK   Probability
 0           0.01002259576
 1           0.0634764398
 2           0.1971110499
 3           0.4049871101
 4           0.6301861752
 5           0.8103454274
 6           0.9204427481
 7           0.9728700437
 8           0.9925302796
 9           0.9983555346
10           0.9997147608
11           0.9999618928
12           0.9999962167
13           0.9999997371
14           0.9999999886
15           0.9999999998
16           1
]
*/
  ex_out << "\n" "At least (>)""\n""Guessed OK   Probability" << right << endl;
  for (int score = 0; score <= questions; score++)
  {
    ex_out << setw(2) << score << "           "  << setprecision(10)
      << cdf(complement(quiz, score)) << endl;
  }
/*`
[pre
At least (>)
Guessed OK   Probability
 0           0.9899774042
 1           0.9365235602
 2           0.8028889501
 3           0.5950128899
 4           0.3698138248
 5           0.1896545726
 6           0.07955725188
 7           0.02712995629
 8           0.00746972044
 9           0.001644465374
10           0.0002852391917
11           3.810715862e-005
12           3.783265129e-006
13           2.628657967e-007
14           1.140870154e-008
15           2.328306437e-010
16           0
]
We now consider the probabilities of *ranges* of correct guesses.

First, calculate the probability of getting a range of guesses right,
by adding the exact probabilities of each from low ... high.
*/
  int low = 3; // Getting at least 3 right.
  int high = 5; // Getting as most 5 right.
  double sum = 0.;
  for (int i = low; i <= high; i++)
  {
    sum += pdf(quiz, i);
  }
  ex_out.precision(4);
  ex_out << "Probability of getting between "
    << low << " and " << high << " answers right by guessing is "
    << sum  << endl; // 0.61323
/*`
[pre
Probability of getting between 3 and 5 answers right by guessing is 0.6132
]
Or, usually better, we can use the difference of cdfs instead:
*/
  ex_out << "Probability of getting between " << low << " and " << high << " answers right by guessing is "
    <<  cdf(quiz, high) - cdf(quiz, low - 1) << endl; // 0.61323
/*`
[pre
Probability of getting between 3 and 5 answers right by guessing is 0.6132
]
And we can also try a few more combinations of high and low choices:
*/
  low = 1; high = 6;
  ex_out << "Probability of getting between " << low << " and " << high << " answers right by guessing is "
    <<  cdf(quiz, high) - cdf(quiz, low - 1) << endl; // 1 and 6 P= 0.91042
  low = 1; high = 8;
  ex_out << "Probability of getting between " << low << " and " << high << " answers right by guessing is "
    <<  cdf(quiz, high) - cdf(quiz, low - 1) << endl; // 1 <= x 8 P = 0.9825
  low = 4; high = 4;
  ex_out << "Probability of getting between " << low << " and " << high << " answers right by guessing is "
    <<  cdf(quiz, high) - cdf(quiz, low - 1) << endl; // 4 <= x 4 P = 0.22520

/*`
[pre
Probability of getting between 1 and 6 answers right by guessing is 0.9104
Probability of getting between 1 and 8 answers right by guessing is 0.9825
Probability of getting between 4 and 4 answers right by guessing is 0.2252
]
[h4 Using Binomial distribution moments]
Using moments of the distribution, we can say more about the spread of results from guessing.
*/
  ex_out << "By guessing, on average, one can expect to get " << mean(quiz) << " correct answers." << endl;
  ex_out << "Standard deviation is " << standard_deviation(quiz) << endl;
  ex_out << "So about 2/3 will lie within 1 standard deviation and get between "
    <<  ceil(mean(quiz) - standard_deviation(quiz))  << " and "
    << floor(mean(quiz) + standard_deviation(quiz)) << " correct." << endl;
  ex_out << "Mode (the most frequent) is " << mode(quiz) << endl;
  ex_out << "Skewness is " << skewness(quiz) << endl;

/*`
[pre
By guessing, on average, one can expect to get 4 correct answers.
Standard deviation is 1.732
So about 2/3 will lie within 1 standard deviation and get between 3 and 5 correct.
Mode (the most frequent) is 4
Skewness is 0.2887
]
[h4 Quantiles]
The quantiles (percentiles or percentage points) for a few probability levels:
*/
  ex_out << "Quartiles " << quantile(quiz, 0.25) << " to "
    << quantile(complement(quiz, 0.25)) << endl; // Quartiles
  ex_out << "1 standard deviation " << quantile(quiz, 0.33) << " to "
    << quantile(quiz, 0.67) << endl; // 1 sd
  ex_out << "Deciles " << quantile(quiz, 0.1)  << " to "
    << quantile(complement(quiz, 0.1))<< endl; // Deciles
  ex_out << "5 to 95% " << quantile(quiz, 0.05)  << " to "
    << quantile(complement(quiz, 0.05))<< endl; // 5 to 95%
  ex_out << "2.5 to 97.5% " << quantile(quiz, 0.025) << " to "
    <<  quantile(complement(quiz, 0.025)) << endl; // 2.5 to 97.5%
  ex_out << "2 to 98% " << quantile(quiz, 0.02)  << " to "
    << quantile(complement(quiz, 0.02)) << endl; //  2 to 98%

  ex_out << "If guessing then percentiles 1 to 99% will get " << quantile(quiz, 0.01)
    << " to " << quantile(complement(quiz, 0.01)) << " right." << endl;
/*`
Notice that these output integral values because the default policy is `integer_round_outwards`.
[pre
Quartiles 2 to 5
1 standard deviation 2 to 5
Deciles 1 to 6
5 to 95% 0 to 7
2.5 to 97.5% 0 to 8
2 to 98% 0 to 8
]
*/

//] [/binomial_quiz_example2]

//[discrete_quantile_real
/*`
Quantiles values are controlled by the __understand_dis_quant  quantile policy chosen.
The default is `integer_round_outwards`,
so the lower quantile is rounded down, and the upper quantile is rounded up.

But we might believe that the real values tell us a little more - see __math_discrete.

We could control the policy for *all* distributions by

  #define BOOST_MATH_DISCRETE_QUANTILE_POLICY real

  at the head of the program would make this policy apply
to this *one, and only*, translation unit.

Or we can now create a (typedef for) policy that has discrete quantiles real
(here avoiding any 'using namespaces ...' statements):
*/
  using boost::math::policies::policy;
  using boost::math::policies::discrete_quantile;
  using boost::math::policies::real;
  using boost::math::policies::integer_round_outwards; // Default.
  typedef boost::math::policies::policy<discrete_quantile<real> > real_quantile_policy;
/*`
Add a custom binomial distribution called ``real_quantile_binomial`` that uses ``real_quantile_policy``
*/
  using boost::math::binomial_distribution;
  typedef binomial_distribution<double, real_quantile_policy> real_quantile_binomial;
/*`
Construct an object of this custom distribution:
*/
  real_quantile_binomial quiz_real(questions, success_fraction);
/*`
And use this to show some quantiles - that now have real rather than integer values.
*/
  ex_out << "Quartiles " << quantile(quiz, 0.25) << " to "
    << quantile(complement(quiz_real, 0.25)) << endl; // Quartiles 2 to 4.6212
  ex_out << "1 standard deviation " << quantile(quiz_real, 0.33) << " to "
    << quantile(quiz_real, 0.67) << endl; // 1 sd 2.6654 4.194
  ex_out << "Deciles " << quantile(quiz_real, 0.1)  << " to "
    << quantile(complement(quiz_real, 0.1))<< endl; // Deciles 1.3487 5.7583
  ex_out << "5 to 95% " << quantile(quiz_real, 0.05)  << " to "
    << quantile(complement(quiz_real, 0.05))<< endl; // 5 to 95% 0.83739 6.4559
  ex_out << "2.5 to 97.5% " << quantile(quiz_real, 0.025) << " to "
    <<  quantile(complement(quiz_real, 0.025)) << endl; // 2.5 to 97.5% 0.42806 7.0688
  ex_out << "2 to 98% " << quantile(quiz_real, 0.02)  << " to "
    << quantile(complement(quiz_real, 0.02)) << endl; //  2 to 98% 0.31311 7.7880

  ex_out << "If guessing, then percentiles 1 to 99% will get " << quantile(quiz_real, 0.01)
    << " to " << quantile(complement(quiz_real, 0.01)) << " right." << endl;
/*`
[pre
Real Quantiles
Quartiles 2 to 4.621
1 standard deviation 2.665 to 4.194
Deciles 1.349 to 5.758
5 to 95% 0.8374 to 6.456
2.5 to 97.5% 0.4281 to 7.069
2 to 98% 0.3131 to 7.252
If guessing then percentiles 1 to 99% will get 0 to 7.788 right.
]
*/

//] [/discrete_quantile_real]
  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because
    // default policies are to throw exceptions on arguments that cause
    // errors like underflow, overflow.
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  return 0;
    return 0;  // main's implicit return
} // int example_main()



/*

Output is:

BAutorun "i:\boost-06-05-03-1300\libs\math\test\Math_test\debug\binomial_quiz_example.exe"
Binomial distribution example - guessing in a quiz.
In a quiz with 16 questions and with a probability of guessing right of 25 % or 1 in 4
Probability of getting none right is 0.0100226
Probability of getting exactly one right is 0.0534538
Probability of getting exactly two right is 0.133635
Probability of getting exactly 11 answers right by chance is 0.000247132
Probability of getting all 16 answers right by chance is 2.32831e-010
Guessed Probability
 0      0.0100226
 1      0.0534538
 2      0.133635
 3      0.207876
 4      0.225199
 5      0.180159
 6      0.110097
 7      0.0524273
 8      0.0196602
 9      0.00582526
10      0.00135923
11      0.000247132
12      3.43239e-005
13      3.5204e-006
14      2.51457e-007
15      1.11759e-008
16      2.32831e-010
Probability of getting none or one right is 0.0634764
Probability of getting none or one right is 0.0634764
Probability of getting <= 10 right (to fail) is 0.999715
Probability of getting > 10 right (to pass) is 0.000285239
Probability of getting > 10 right (to pass) is 0.000285239
Probability of getting less than 11 (< 11) answers right by guessing is 0.999715
Probability of getting at least 11(>= 11) answers right by guessing is 0.000285239, only 1 in 3505.83
At most (<=)
Guessed OK   Probability
 0           0.01002259576
 1           0.0634764398
 2           0.1971110499
 3           0.4049871101
 4           0.6301861752
 5           0.8103454274
 6           0.9204427481
 7           0.9728700437
 8           0.9925302796
 9           0.9983555346
10           0.9997147608
11           0.9999618928
12           0.9999962167
13           0.9999997371
14           0.9999999886
15           0.9999999998
16           1
At least (>)
Guessed OK   Probability
 0           0.9899774042
 1           0.9365235602
 2           0.8028889501
 3           0.5950128899
 4           0.3698138248
 5           0.1896545726
 6           0.07955725188
 7           0.02712995629
 8           0.00746972044
 9           0.001644465374
10           0.0002852391917
11           3.810715862e-005
12           3.783265129e-006
13           2.628657967e-007
14           1.140870154e-008
15           2.328306437e-010
16           0
Probability of getting between 3 and 5 answers right by guessing is 0.6132
Probability of getting between 3 and 5 answers right by guessing is 0.6132
Probability of getting between 1 and 6 answers right by guessing is 0.9104
Probability of getting between 1 and 8 answers right by guessing is 0.9825
Probability of getting between 4 and 4 answers right by guessing is 0.2252
By guessing, on average, one can expect to get 4 correct answers.
Standard deviation is 1.732
So about 2/3 will lie within 1 standard deviation and get between 3 and 5 correct.
Mode (the most frequent) is 4
Skewness is 0.2887
Quartiles 2 to 5
1 standard deviation 2 to 5
Deciles 1 to 6
5 to 95% 0 to 7
2.5 to 97.5% 0 to 8
2 to 98% 0 to 8
If guessing then percentiles 1 to 99% will get 0 to 8 right.
Quartiles 2 to 4.621
1 standard deviation 2.665 to 4.194
Deciles 1.349 to 5.758
5 to 95% 0.8374 to 6.456
2.5 to 97.5% 0.4281 to 7.069
2 to 98% 0.3131 to 7.252
If guessing, then percentiles 1 to 99% will get 0 to 7.788 right.

*/

TEST_CASE("example: binomial_quiz_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Binomial distribution example - guessing in a quiz.
In a quiz with 16 questions and with a probability of guessing right of 25 % or 1 in 4
Probability of getting none right is 0.0100226
Probability of getting exactly one right is 0.0534538
Probability of getting exactly two right is 0.133635
Probability of getting exactly 11 answers right by chance is 0.000247132
Probability of getting all 16 answers right by chance is 2.32831e-10

Guessed Probability
 0      0.0100226
 1      0.0534538
 2      0.133635
 3      0.207876
 4      0.225199
 5      0.180159
 6      0.110097
 7      0.0524273
 8      0.0196602
 9      0.00582526
10      0.00135923
11      0.000247132
12      3.43239e-05
13      3.5204e-06
14      2.51457e-07
15      1.11759e-08
16      2.32831e-10

Probability of getting none or one right is 0.0634764
Probability of getting none or one right is 0.0634764
Probability of getting <= 10 right (to fail) is 0.999715
Probability of getting > 10 right (to pass) is 0.000285239
Probability of getting > 10 right (to pass) is 0.000285239
Probability of getting less than 11 (< 11) answers right by guessing is 0.999715
Probability of getting at least 11(>= 11) answers right by guessing is 0.000285239, only 1 in 3505.83

At most (<=)
Guessed OK   Probability
 0           0.01002259576
 1           0.0634764398
 2           0.1971110499
 3           0.4049871101
 4           0.6301861752
 5           0.8103454274
 6           0.9204427481
 7           0.9728700437
 8           0.9925302796
 9           0.9983555346
10           0.9997147608
11           0.9999618928
12           0.9999962167
13           0.9999997371
14           0.9999999886
15           0.9999999998
16           1


At least (>)
Guessed OK   Probability
 0           0.9899774042
 1           0.9365235602
 2           0.8028889501
 3           0.5950128899
 4           0.3698138248
 5           0.1896545726
 6           0.07955725188
 7           0.02712995629
 8           0.00746972044
 9           0.001644465374
10           0.0002852391917
11           3.810715862e-05
12           3.783265129e-06
13           2.628657967e-07
14           1.140870154e-08
15           2.328306437e-10
16           0
Probability of getting between 3 and 5 answers right by guessing is 0.6132
Probability of getting between 3 and 5 answers right by guessing is 0.6132
Probability of getting between 1 and 6 answers right by guessing is 0.9104
Probability of getting between 1 and 8 answers right by guessing is 0.9825
Probability of getting between 4 and 4 answers right by guessing is 0.2252
By guessing, on average, one can expect to get 4 correct answers.
Standard deviation is 1.732
So about 2/3 will lie within 1 standard deviation and get between 3 and 5 correct.
Mode (the most frequent) is 4
Skewness is 0.2887
Quartiles 2 to 5
1 standard deviation 2 to 5
Deciles 1 to 6
5 to 95% 0 to 7
2.5 to 97.5% 0 to 8
2 to 98% 0 to 8
If guessing then percentiles 1 to 99% will get 0 to 8 right.
Quartiles 2 to 4.621
1 standard deviation 2.665 to 4.194
Deciles 1.349 to 5.758
5 to 95% 0.8374 to 6.456
2.5 to 97.5% 0.4281 to 7.069
2 to 98% 0.3131 to 7.252
If guessing, then percentiles 1 to 99% will get 0 to 7.788 right.
)ex"));
#endif
}
}  // namespace ex_binomial_quiz_example

// libs/math/example/cardinal_cubic_b_spline_example.cpp
namespace ex_cardinal_cubic_b_spline_example {
int example_main()
{
    // We begin with an array of samples:
    std::vector<double> v(500);
    // And decide on a stepsize:
    double step = 0.01;

    // Initialize the vector with a function we'd like to interpolate:
    for (size_t i = 0; i < v.size(); ++i)
    {
        v[i] = sin(i*step);
    }
    // We could define an arbitrary start time, but for now we'll just use 0:
    boost::math::interpolators::cardinal_cubic_b_spline<double> spline(v.data(), v.size(), 0 /* start time */, step);

    // Now we can evaluate the spline wherever we please.
    std::mt19937 gen;
    boost::random::uniform_real_distribution<double> abscissa(0, v.size()*step);
    for (size_t i = 0; i < 10; ++i)
    {
        double x = abscissa(gen);
        ex_out << "sin(" << x << ") = " << sin(x) << ", spline interpolation gives " << spline(x) << std::endl;
        ex_out << "cos(" << x << ") = " << cos(x) << ", spline derivative interpolation gives " << spline.prime(x) << std::endl;
    }

    // The next example is less trivial:
    // We will try to figure out when the population of the United States crossed 100 million.
    // Since the census is taken every 10 years, the data is equally spaced, so we can use the cubic b spline.
    // Data taken from https://en.wikipedia.org/wiki/United_States_Census
    // We'll start at the year 1860:
    double t0 = 1860;
    double time_step = 10;
    std::vector<double> population{31443321,  /* 1860 */
                                   39818449,  /* 1870 */
                                   50189209,  /* 1880 */
                                   62947714,  /* 1890 */
                                   76212168,  /* 1900 */
                                   92228496,  /* 1910 */
                                   106021537, /* 1920 */
                                   122775046, /* 1930 */
                                   132164569, /* 1940 */
                                   150697361, /* 1950 */
                                   179323175};/* 1960 */

    // An eyeball estimate indicates that the population crossed 100 million around 1915.
    // Let's see what interpolation says:
    boost::math::interpolators::cardinal_cubic_b_spline<double> p(population.data(), population.size(), t0, time_step);

    // Now create a function which has a zero at p = 100,000,000:
    auto f = [=](double t){ return p(t) - 100000000; };

    // Boost includes a bisection algorithm, which is robust, though not as fast as some others
    // we provide, but let's try that first.  We need a termination condition for it, which
    // takes the two endpoints of the range and returns either true (stop) or false (keep going),
    // we could use a predefined one such as boost::math::tools::eps_tolerance<double>, but that
    // won't stop until we have full double precision which is overkill, since we just need the
    // endpoint to yield the same month.  While we're at it, we'll keep track of the number of
    // iterations required too, though this is strictly optional:

    auto termination = [](double left, double right)
    {
       double left_month = std::round((left - std::floor(left)) * 12 + 1);
       double right_month = std::round((right - std::floor(right)) * 12 + 1);
       return (left_month == right_month) && (std::floor(left) == std::floor(right));
    };
    std::uintmax_t iterations = 1000;
    auto result =  boost::math::tools::bisect(f, 1910.0, 1920.0, termination, iterations);
    auto time = result.first;  // termination condition ensures that both endpoints yield the same result
    auto month = std::round((time - std::floor(time))*12  + 1);
    auto year = std::floor(time);
    ex_out << "The population of the United States surpassed 100 million on the ";
    ex_out << month << "th month of " << year << std::endl;
    ex_out << "Found in " << iterations << " iterations" << std::endl;

    // Since the cubic B spline offers the first derivative, we could equally have used Newton iterations,
    // this takes "number of bits correct" as a termination condition - 20 should be plenty for what we need,
    // and once again, we track how many iterations are taken:

    auto f_n = [=](double t) { return std::make_pair(p(t) - 100000000, p.prime(t)); };
    iterations = 1000;
    time = boost::math::tools::newton_raphson_iterate(f_n, 1910.0, 1900.0, 2000.0, 20, iterations);
    month = std::round((time - std::floor(time))*12  + 1);
    year = std::floor(time);
    ex_out << "The population of the United States surpassed 100 million on the ";
    ex_out << month << "th month of " << year << std::endl;
    ex_out << "Found in " << iterations << " iterations" << std::endl;

    return 0;  // main's implicit return
}

//] [/cubic_b_spline_example]

//[cubic_b_spline_example_out
/*` Program output is:
[pre
sin(4.07362) = -0.802829, spline interpolation gives - 0.802829
cos(4.07362) = -0.596209, spline derivative interpolation gives - 0.596209
sin(0.677385) = 0.626758, spline interpolation gives 0.626758
cos(0.677385) = 0.779214, spline derivative interpolation gives 0.779214
sin(4.52896) = -0.983224, spline interpolation gives - 0.983224
cos(4.52896) = -0.182402, spline derivative interpolation gives - 0.182402
sin(4.17504) = -0.85907, spline interpolation gives - 0.85907
cos(4.17504) = -0.511858, spline derivative interpolation gives - 0.511858
sin(0.634934) = 0.593124, spline interpolation gives 0.593124
cos(0.634934) = 0.805111, spline derivative interpolation gives 0.805111
sin(4.84434) = -0.991307, spline interpolation gives - 0.991307
cos(4.84434) = 0.131567, spline derivative interpolation gives 0.131567
sin(4.56688) = -0.989432, spline interpolation gives - 0.989432
cos(4.56688) = -0.144997, spline derivative interpolation gives - 0.144997
sin(1.10517) = 0.893541, spline interpolation gives 0.893541
cos(1.10517) = 0.448982, spline derivative interpolation gives 0.448982
sin(3.1618) = -0.0202022, spline interpolation gives - 0.0202022
cos(3.1618) = -0.999796, spline derivative interpolation gives - 0.999796
sin(1.54084) = 0.999551, spline interpolation gives 0.999551
cos(1.54084) = 0.0299566, spline derivative interpolation gives 0.0299566
The population of the United States surpassed 100 million on the 11th month of 1915
Found in 12 iterations
The population of the United States surpassed 100 million on the 11th month of 1915
Found in 3 iterations
]
*/
//]

TEST_CASE("example: cardinal_cubic_b_spline_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(sin(4.07362) = -0.802829, spline interpolation gives -0.802829
cos(4.07362) = -0.596209, spline derivative interpolation gives -0.596209
sin(0.677385) = 0.626758, spline interpolation gives 0.626758
cos(0.677385) = 0.779214, spline derivative interpolation gives 0.779214
sin(4.52896) = -0.983224, spline interpolation gives -0.983224
cos(4.52896) = -0.182402, spline derivative interpolation gives -0.182402
sin(4.17504) = -0.85907, spline interpolation gives -0.85907
cos(4.17504) = -0.511858, spline derivative interpolation gives -0.511858
sin(0.634934) = 0.593124, spline interpolation gives 0.593124
cos(0.634934) = 0.805111, spline derivative interpolation gives 0.805111
sin(4.84434) = -0.991307, spline interpolation gives -0.991307
cos(4.84434) = 0.131567, spline derivative interpolation gives 0.131567
sin(4.56688) = -0.989432, spline interpolation gives -0.989432
cos(4.56688) = -0.144997, spline derivative interpolation gives -0.144997
sin(1.10517) = 0.893541, spline interpolation gives 0.893541
cos(1.10517) = 0.448982, spline derivative interpolation gives 0.448982
sin(3.1618) = -0.0202022, spline interpolation gives -0.0202022
cos(3.1618) = -0.999796, spline derivative interpolation gives -0.999796
sin(1.54084) = 0.999551, spline interpolation gives 0.999551
cos(1.54084) = 0.0299566, spline derivative interpolation gives 0.0299566
The population of the United States surpassed 100 million on the 11th month of 1915
Found in 12 iterations
The population of the United States surpassed 100 million on the 11th month of 1915
Found in 3 iterations
)ex"));
#endif
}
}  // namespace ex_cardinal_cubic_b_spline_example

// libs/math/example/constants_eg1.cpp
namespace ex_constants_eg1 {
using std::endl;

using std::numeric_limits;

/*! \brief Examples of a template function using constants.
    \details This example shows using of constants from function calls like @c pi(),
    rather than the 'cute' plain @c pi use in non-template applications.

    \tparam Real radius parameter that can be a built-in like float, double,
      or a user-defined type like multiprecision.
    \returns Area = pi * radius ^ 2
*/

//[math_constants_eg1
template<class Real>
Real area(Real r)
{
  using namespace boost::math::constants;

  return pi<Real>() * r * r;
}
//] [/math_constants_eg1]

int example_main()
{

  { // Boost.Math constants using function calls like pi().
    // using namespace boost::math::constants;

   using boost::math::constants::pi;
   using boost::math::constants::one_div_two_pi;

#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
     std::size_t max_digits10 = 2 + std::numeric_limits<double>::digits * 3010/10000;
#else
   std::size_t max_digits10 = std::numeric_limits<double>::max_digits10;
#endif

  ex_out.precision(max_digits10);
  ex_out << "double pi =  boost::math::double_constants::pi = " << pi<double>() << endl;
  //   double pi =  boost::math::double_constants::pi = 3.1415926535897931
  double r = 1.234567890123456789;
  double d = pi<double>() * r * r;

  ex_out << "d = " << d << ", r = " << r << endl;

  float rf = 0.987654321987654321f;

  float pif = boost::math::constants::pi<float>();
  ex_out << "pidf =  boost::math::constants::pi() = " << pif << endl;
  //   pidf =  boost::math::float_constants::pi = 3.1415927410125732

    //float df = pi * rf * rf; // conversion from 'const double' to 'float', possible loss of data.
  float df = pif * rf * rf;

  ex_out << "df = " << df << ", rf = " << rf << endl;

  ex_out << "one_div_two_pi " << one_div_two_pi<double>() << endl;

  using boost::math::constants::one_div_two_pi;

  ex_out << "one_div_root_two_pi " << one_div_two_pi<double>() << endl;
  }

  { // Boost math new constants using namespace selected values, like pi.

    //using namespace boost::math::float_constants;
    using namespace boost::math::double_constants;

    double my2pi = two_pi; // Uses boost::math::double_constants::two_pi;

    ex_out << "double my2pi = " << my2pi << endl;

    using boost::math::float_constants::e;
    float my_e = e;
    ex_out << "float my_e  " << my_e << endl;

    double my_pi = boost::math::double_constants::pi;
    ex_out << "double my_pi = boost::math::double_constants::pi =  " << my_pi << endl;

    // If you try to use two namespaces, this may, of course, create ambiguity:
    // it is not too difficult to do this inadvertently.
    using namespace boost::math::float_constants;
    //ex_out << pi << endl; // error C2872: 'pi' : ambiguous symbol.

  }
  {

//[math_constants_ambiguity
     // If you use more than one namespace, this will, of course, create ambiguity:
     using namespace boost::math::double_constants;
     using namespace boost::math::constants;

      //double my_pi = pi(); // error C2872: 'pi' : ambiguous symbol
      //double my_pi2 = pi; // Context does not allow for disambiguation of overloaded function

     // It is also possible to create ambiguity inadvertently,
     // perhaps in other peoples code,
     // by making the scope of a namespace declaration wider than necessary,
     // therefore is it prudent to avoid this risk by localising the scope of such definitions.
//] [/math_constants_ambiguity]

  }

  { // You can, of course, use both methods of access if both are fully qualified, for examples:

    //ex_out.precision(std::numeric_limits<double>::max_digits10);// Ideally.
    ex_out.precision(2 + std::numeric_limits<double>::digits * 3010/10000); // If no max_digits10.

    double my_pi1 = boost::math::constants::pi<double>();
    double my_pid = boost::math::double_constants::pi;
    ex_out << "boost::math::constants::pi<double>() = " << my_pi1 << endl
         << "boost::math::double_constants::pi = " << my_pid << endl;

    // ex_out.precision(std::numeric_limits<float>::max_digits10); // Ideally.
    ex_out.precision(2 + std::numeric_limits<double>::digits * 3010/10000); // If no max_digits10.
    float my_pif = boost::math::float_constants::pi;
    ex_out << "boost::math::float_constants::pi = " << my_pif << endl;

  }

  { // Use with templates

    // \warning it is important to be very careful with the type provided as parameter.
    // For example, naively providing an @b integer instead of a floating-point type can be disastrous.
    // ex_out << "Area = " << area(2) << endl; // warning : 'return' : conversion from 'double' to 'int', possible loss of data
    // Failure to heed this warning can lead to very wrong answers!
    //  Area = 12 !!  = 3 * 2 * 2
//[math_constants_template_integer_type
    //ex_out << "Area = " << area(2) << endl; //   Area = 12!
    ex_out << "Area = " << area(2.) << endl;  //   Area = 12.566371

    // You can also avoid this by being explicit about the type of @c area.
    ex_out << "Area = " << area<double>(2) << endl;

//] [/math_constants_template_integer_type]


  }
/*
{
    using  boost::math::constants::pi;
    //double my_pi3 = pi<double>(); // OK
    //double my_pi4 = pi<>(); cannot find template type.
    //double my_pi4 = pi(); // Can't find a function.

  }
*/

    return 0;  // main's implicit return
} // int example_main()

/*[constants_eq1_output

Output:

  double pi =  boost::math::double_constants::pi = 3.1415926535897931
  d = 4.7882831840285398, r = 1.2345678901234567
  pidf =  boost::math::constants::pi() = 3.1415927410125732
  df = 3.0645015239715576, rf = 0.98765432834625244
  one_div_two_pi 0.15915494309189535
  one_div_root_two_pi 0.15915494309189535
  double my2pi = 6.2831853071795862
  float my_e  2.7182817459106445
  double my_pi = boost::math::double_constants::pi =  3.1415926535897931
  boost::math::constants::pi<double>() = 3.1415926535897931
  boost::math::double_constants::pi = 3.1415926535897931
  boost::math::float_constants::pi = 3.1415927410125732
  Area = 12.566370614359172
  Area = 12.566370614359172


] [/constants_eq1_output]
*/

TEST_CASE("example: constants_eg1") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(double pi =  boost::math::double_constants::pi = 3.1415926535897931
d = 4.7882831840285398, r = 1.2345678901234567
pidf =  boost::math::constants::pi() = 3.1415927410125732
df = 3.0645015239715576, rf = 0.98765432834625244
one_div_two_pi 0.15915494309189535
one_div_root_two_pi 0.15915494309189535
double my2pi = 6.2831853071795862
float my_e  2.7182817459106445
double my_pi = boost::math::double_constants::pi =  3.1415926535897931
boost::math::constants::pi<double>() = 3.1415926535897931
boost::math::double_constants::pi = 3.1415926535897931
boost::math::float_constants::pi = 3.1415927410125732
Area = 12.566370614359172
Area = 12.566370614359172
)ex"));
#endif
}
}  // namespace ex_constants_eg1

// libs/math/example/f_test.cpp
namespace ex_f_test {
#ifdef _MSC_VER
#  pragma warning(disable: 4512) // assignment operator could not be generated.
#  pragma warning(disable: 4510) // default constructor could not be generated.
#  pragma warning(disable: 4610) // can never be instantiated - user defined constructor required.
#  pragma warning(disable: 4180) // qualifier has no effect (in Fusion).
#endif

 using std::endl;
using std::left; using std::fixed; using std::right; using std::scientific;
using std::setw;
using std::setprecision;


void f_test(
       double sd1,     // Sample 1 std deviation
       double sd2,     // Sample 2 std deviation
       double N1,      // Sample 1 size
       double N2,      // Sample 2 size
       double alpha)  // Significance level
{
   //
   // An F test applied to two sets of data.
   // We are testing the null hypothesis that the
   // standard deviation of the samples is equal, and
   // that any variation is down to chance.  We can
   // also test the alternative hypothesis that any
   // difference is not down to chance.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda359.htm
   //
   // Avoid "using namespace boost::math;" because of potential name ambiguity.
   using boost::math::fisher_f;

   // Print header:
   ex_out <<
      "____________________________________\n"
      "F test for equal standard deviations\n"
      "____________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << "Sample 1:\n";
   ex_out << setw(55) << left << "Number of Observations" << "=  " << N1 << "\n";
   ex_out << setw(55) << left << "Sample Standard Deviation" << "=  " << sd1 << "\n\n";
   ex_out << "Sample 2:\n";
   ex_out << setw(55) << left << "Number of Observations" << "=  " << N2 << "\n";
   ex_out << setw(55) << left << "Sample Standard Deviation" << "=  " << sd2 << "\n\n";
   //
   // Now we can calculate and output some stats:
   //
   // F-statistic:
   double F = (sd1 / sd2);
   F *= F;
   ex_out << setw(55) << left << "Test Statistic" << "=  " << F << "\n\n";
   //
   // Finally define our distribution, and get the probability:
   //
   fisher_f dist(N1 - 1, N2 - 1);
   double p = cdf(dist, F);
   ex_out << setw(55) << left << "CDF of test statistic: " << "=  "
      << setprecision(3) << scientific << p << "\n";
   double ucv = quantile(complement(dist, alpha));
   double ucv2 = quantile(complement(dist, alpha / 2));
   double lcv = quantile(dist, alpha);
   double lcv2 = quantile(dist, alpha / 2);
   ex_out << setw(55) << left << "Upper Critical Value at alpha: " << "=  "
      << setprecision(3) << scientific << ucv << "\n";
   ex_out << setw(55) << left << "Upper Critical Value at alpha/2: " << "=  "
      << setprecision(3) << scientific << ucv2 << "\n";
   ex_out << setw(55) << left << "Lower Critical Value at alpha: " << "=  "
      << setprecision(3) << scientific << lcv << "\n";
   ex_out << setw(55) << left << "Lower Critical Value at alpha/2: " << "=  "
      << setprecision(3) << scientific << lcv2 << "\n\n";
   //
   // Finally print out results of null and alternative hypothesis:
   //
   ex_out << setw(55) << left <<
      "Results for Alternative Hypothesis and alpha" << "=  "
      << setprecision(4) << fixed << alpha << "\n\n";
   ex_out << "Alternative Hypothesis                                    Conclusion\n";
   ex_out << "Standard deviations are unequal (two sided test)          ";
   if((ucv2 < F) || (lcv2 > F))
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Standard deviation 1 is less than standard deviation 2    ";
   if(lcv > F)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Standard deviation 1 is greater than standard deviation 2 ";
   if(ucv < F)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << endl << endl;
}

int example_main()
{
   //
   // Run tests for ceramic strength data:
   // see http://www.itl.nist.gov/div898/handbook/eda/section4/eda42a1.htm
   // The data for this case study were collected by Said Jahanmir of the
   // NIST Ceramics Division in 1996 in connection with a NIST/industry
   // ceramics consortium for strength optimization of ceramic strength.
   //
   f_test(65.54909, 61.85425, 240, 240, 0.05);
   //
   // And again for the process change comparison:
   // see http://www.itl.nist.gov/div898/handbook/prc/section3/prc32.htm
   // A new procedure to assemble a device is introduced and tested for
   // possible improvement in time of assembly. The question being addressed
   // is whether the standard deviation of the new assembly process (sample 2) is
   // better (i.e., smaller) than the standard deviation for the old assembly
   // process (sample 1).
   //
   f_test(4.9082, 2.5874, 11, 9, 0.05);
   return 0;
    return 0;  // main's implicit return
}

/*

Output:

  f_test.cpp
  F-test_example1.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Debug\F_test_example1.exe
  ____________________________________
  F test for equal standard deviations
  ____________________________________
  
  Sample 1:
  Number of Observations                                 =  240
  Sample Standard Deviation                              =  65.549
  
  Sample 2:
  Number of Observations                                 =  240
  Sample Standard Deviation                              =  61.854
  
  Test Statistic                                         =  1.123
  
  CDF of test statistic:                                 =  8.148e-001
  Upper Critical Value at alpha:                         =  1.238e+000
  Upper Critical Value at alpha/2:                       =  1.289e+000
  Lower Critical Value at alpha:                         =  8.080e-001
  Lower Critical Value at alpha/2:                       =  7.756e-001
  
  Results for Alternative Hypothesis and alpha           =  0.0500
  
  Alternative Hypothesis                                    Conclusion
  Standard deviations are unequal (two sided test)          REJECTED
  Standard deviation 1 is less than standard deviation 2    REJECTED
  Standard deviation 1 is greater than standard deviation 2 REJECTED
  
  
  ____________________________________
  F test for equal standard deviations
  ____________________________________
  
  Sample 1:
  Number of Observations                                 =  11.00000
  Sample Standard Deviation                              =  4.90820
  
  Sample 2:
  Number of Observations                                 =  9.00000
  Sample Standard Deviation                              =  2.58740
  
  Test Statistic                                         =  3.59847
  
  CDF of test statistic:                                 =  9.589e-001
  Upper Critical Value at alpha:                         =  3.347e+000
  Upper Critical Value at alpha/2:                       =  4.295e+000
  Lower Critical Value at alpha:                         =  3.256e-001
  Lower Critical Value at alpha/2:                       =  2.594e-001
  
  Results for Alternative Hypothesis and alpha           =  0.0500
  
  Alternative Hypothesis                                    Conclusion
  Standard deviations are unequal (two sided test)          REJECTED
  Standard deviation 1 is less than standard deviation 2    REJECTED
  Standard deviation 1 is greater than standard deviation 2 NOT REJECTED
  
  
  ____________________________________
  F test for equal standard deviations
  ____________________________________
  
  Sample 1:
  Number of Observations                                 =  240
  Sample Standard Deviation                              =  65.549
  
  Sample 2:
  Number of Observations                                 =  240
  Sample Standard Deviation                              =  61.854
  
  Test Statistic                                         =  1.123
  
  CDF of test statistic:                                 =  8.148e-001
  Upper Critical Value at alpha:                         =  1.238e+000
  Upper Critical Value at alpha/2:                       =  1.289e+000
  Lower Critical Value at alpha:                         =  8.080e-001
  Lower Critical Value at alpha/2:                       =  7.756e-001
  
  Results for Alternative Hypothesis and alpha           =  0.0500
  
  Alternative Hypothesis                                    Conclusion
  Standard deviations are unequal (two sided test)          REJECTED
  Standard deviation 1 is less than standard deviation 2    REJECTED
  Standard deviation 1 is greater than standard deviation 2 REJECTED
  
  
  ____________________________________
  F test for equal standard deviations
  ____________________________________
  
  Sample 1:
  Number of Observations                                 =  11.00000
  Sample Standard Deviation                              =  4.90820
  
  Sample 2:
  Number of Observations                                 =  9.00000
  Sample Standard Deviation                              =  2.58740
  
  Test Statistic                                         =  3.59847
  
  CDF of test statistic:                                 =  9.589e-001
  Upper Critical Value at alpha:                         =  3.347e+000
  Upper Critical Value at alpha/2:                       =  4.295e+000
  Lower Critical Value at alpha:                         =  3.256e-001
  Lower Critical Value at alpha/2:                       =  2.594e-001
  
  Results for Alternative Hypothesis and alpha           =  0.0500
  
  Alternative Hypothesis                                    Conclusion
  Standard deviations are unequal (two sided test)          REJECTED
  Standard deviation 1 is less than standard deviation 2    REJECTED
  Standard deviation 1 is greater than standard deviation 2 NOT REJECTED
  
 

*/

TEST_CASE("example: f_test") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(____________________________________
F test for equal standard deviations
____________________________________

Sample 1:
Number of Observations                                 =  240
Sample Standard Deviation                              =  65.549

Sample 2:
Number of Observations                                 =  240
Sample Standard Deviation                              =  61.854

Test Statistic                                         =  1.123

CDF of test statistic:                                 =  8.148e-01
Upper Critical Value at alpha:                         =  1.238e+00
Upper Critical Value at alpha/2:                       =  1.289e+00
Lower Critical Value at alpha:                         =  8.080e-01
Lower Critical Value at alpha/2:                       =  7.756e-01

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis                                    Conclusion
Standard deviations are unequal (two sided test)          REJECTED
Standard deviation 1 is less than standard deviation 2    REJECTED
Standard deviation 1 is greater than standard deviation 2 REJECTED


____________________________________
F test for equal standard deviations
____________________________________

Sample 1:
Number of Observations                                 =  11.00000
Sample Standard Deviation                              =  4.90820

Sample 2:
Number of Observations                                 =  9.00000
Sample Standard Deviation                              =  2.58740

Test Statistic                                         =  3.59847

CDF of test statistic:                                 =  9.589e-01
Upper Critical Value at alpha:                         =  3.347e+00
Upper Critical Value at alpha/2:                       =  4.295e+00
Lower Critical Value at alpha:                         =  3.256e-01
Lower Critical Value at alpha/2:                       =  2.594e-01

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis                                    Conclusion
Standard deviations are unequal (two sided test)          REJECTED
Standard deviation 1 is less than standard deviation 2    REJECTED
Standard deviation 1 is greater than standard deviation 2 NOT REJECTED


)ex"));
#endif
}
}  // namespace ex_f_test

// libs/math/example/find_mean_and_sd_normal.cpp
namespace ex_find_mean_and_sd_normal {
  using boost::math::normal; // typedef provides default type is double.
  using boost::math::cauchy; // typedef provides default type is double.
  using boost::math::find_location;
  using boost::math::find_scale;
  using boost::math::complement;
  using boost::math::policies::policy;

   using std::endl; using std::left; using std::showpoint; using std::noshowpoint;
  using std::setw; using std::setprecision;
  using std::numeric_limits;
  
//] [/normal_std Quickbook]

int example_main()
{
  ex_out << "Find_location (mean) and find_scale (standard deviation) examples." << endl;
  try
  {

//[normal_find_location_and_scale_eg

/*`
[h4 Using find_location and find_scale to meet dispensing and measurement specifications]

Consider an example from K Krishnamoorthy,
Handbook of Statistical Distributions with Applications,
ISBN 1-58488-635-8, (2006) p 126, example 10.3.7.

"A machine is set to pack 3 kg of ground beef per pack.
Over a long period of time it is found that the average packed was 3 kg
with a standard deviation of 0.1 kg.
Assume the packing is normally distributed."

We start by constructing a normal distribution with the given parameters:
*/

double mean = 3.; // kg
double standard_deviation = 0.1; // kg
normal packs(mean, standard_deviation);
/*`We can then find the fraction (or %) of packages that weigh more than 3.1 kg.
*/

double max_weight = 3.1; // kg
ex_out << "Percentage of packs > " << max_weight << " is "
<< cdf(complement(packs, max_weight)) * 100. << endl; // P(X > 3.1)

/*`We might want to ensure that 95% of packs are over a minimum weight specification,
then we want the value of the mean such that P(X < 2.9) = 0.05.

Using the mean of 3 kg, we can estimate
the fraction of packs that fail to meet the specification of 2.9 kg.
*/

double minimum_weight = 2.9;
ex_out <<"Fraction of packs <= " << minimum_weight << " with a mean of " << mean
  << " is " << cdf(complement(packs, minimum_weight)) << endl;
// fraction of packs <= 2.9 with a mean of 3 is 0.841345

/*`This is 0.84 - more than the target fraction of 0.95.
If we want 95% to be over the minimum weight,
what should we set the mean weight to be?

Using the KK StatCalc program supplied with the book and the method given on page 126 gives 3.06449.

We can confirm this by constructing a new distribution which we call 'xpacks'
with a safety margin mean of 3.06449 thus:
*/
double over_mean = 3.06449;
normal xpacks(over_mean, standard_deviation);
ex_out << "Fraction of packs >= " << minimum_weight
<< " with a mean of " << xpacks.mean()
  << " is " << cdf(complement(xpacks, minimum_weight)) << endl;
// fraction of packs >= 2.9 with a mean of 3.06449 is 0.950005

/*`Using this Math Toolkit, we can calculate the required mean directly thus:
*/
double under_fraction = 0.05;  // so 95% are above the minimum weight mean - sd = 2.9
double low_limit = standard_deviation;
double offset = mean - low_limit - quantile(packs, under_fraction);
double nominal_mean = mean + offset;
// mean + (mean - low_limit - quantile(packs, under_fraction));

normal nominal_packs(nominal_mean, standard_deviation);
ex_out << "Setting the packer to " << nominal_mean << " will mean that "
  << "fraction of packs >= " << minimum_weight
  << " is " << cdf(complement(nominal_packs, minimum_weight)) << endl;
// Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95

/*`
This calculation is generalized as the free function called `find_location`,
see __algorithms.

To use this we will need to
*/

  using boost::math::find_location;
/*`and then use find_location function to find safe_mean,
& construct a new normal distribution called 'goodpacks'.
*/
double safe_mean = find_location<normal>(minimum_weight, under_fraction, standard_deviation);
normal good_packs(safe_mean, standard_deviation);
/*`with the same confirmation as before:
*/
ex_out << "Setting the packer to " << nominal_mean << " will mean that "
  << "fraction of packs >= " << minimum_weight
  << " is " << cdf(complement(good_packs, minimum_weight)) << endl;
// Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95

/*`
[h4 Using Cauchy-Lorentz instead of normal distribution]

After examining the weight distribution of a large number of packs, we might decide that,
after all, the assumption of a normal distribution is not really justified.
We might find that the fit is better to a __cauchy_distrib.
This distribution has wider 'wings', so that whereas most of the values
are closer to the mean than the normal, there are also more values than 'normal'
that lie further from the mean than the normal.

This might happen because a larger than normal lump of meat is either included or excluded.

We first create a __cauchy_distrib with the original mean and standard deviation,
and estimate the fraction that lie below our minimum weight specification.
*/

cauchy cpacks(mean, standard_deviation);
ex_out << "Cauchy Setting the packer to " << mean << " will mean that "
  << "fraction of packs >= " << minimum_weight
  << " is " << cdf(complement(cpacks, minimum_weight)) << endl;
// Cauchy Setting the packer to 3 will mean that fraction of packs >= 2.9 is 0.75

/*`Note that far fewer of the packs meet the specification, only 75% instead of 95%.
Now we can repeat the find_location, using the cauchy distribution as template parameter,
in place of the normal used above.
*/

double lc = find_location<cauchy>(minimum_weight, under_fraction, standard_deviation);
ex_out << "find_location<cauchy>(minimum_weight, over fraction, standard_deviation); " << lc << endl;
// find_location<cauchy>(minimum_weight, over fraction, packs.standard_deviation()); 3.53138
/*`Note that the safe_mean setting needs to be much higher, 3.53138 instead of 3.06449,
so we will make rather less profit.

And again confirm that the fraction meeting specification is as expected.
*/
cauchy goodcpacks(lc, standard_deviation);
ex_out << "Cauchy Setting the packer to " << lc << " will mean that "
  << "fraction of packs >= " << minimum_weight
  << " is " << cdf(complement(goodcpacks, minimum_weight)) << endl;
// Cauchy Setting the packer to 3.53138 will mean that fraction of packs >= 2.9 is 0.95

/*`Finally we could estimate the effect of a much tighter specification,
that 99% of packs met the specification.
*/

ex_out << "Cauchy Setting the packer to "
  << find_location<cauchy>(minimum_weight, 0.99, standard_deviation)
  << " will mean that "
  << "fraction of packs >= " << minimum_weight
  << " is " << cdf(complement(goodcpacks, minimum_weight)) << endl;

/*`Setting the packer to 3.13263 will mean that fraction of packs >= 2.9 is 0.99,
but will more than double the mean loss from 0.0644 to 0.133 kg per pack.

Of course, this calculation is not limited to packs of meat, it applies to dispensing anything,
and it also applies to a 'virtual' material like any measurement.

The only caveat is that the calculation assumes that the standard deviation (scale) is known with
a reasonably low uncertainty, something that is not so easy to ensure in practice.
And that the distribution is well defined, __normal_distrib or __cauchy_distrib, or some other.

If one is simply dispensing a very large number of packs,
then it may be feasible to measure the weight of hundreds or thousands of packs.
With a healthy 'degrees of freedom', the confidence intervals for the standard deviation
are not too wide, typically about + and - 10% for hundreds of observations.

For other applications, where it is more difficult or expensive to make many observations,
the confidence intervals are depressingly wide.

See [link math_toolkit.stat_tut.weg.cs_eg.chi_sq_intervals Confidence Intervals on the standard deviation]
for a worked example
[@../../example/chi_square_std_dev_test.cpp chi_square_std_dev_test.cpp]
of estimating these intervals.


[h4 Changing the scale or standard deviation]

Alternatively, we could invest in a better (more precise) packer
(or measuring device) with a lower standard deviation, or scale.

This might cost more, but would reduce the amount we have to 'give away'
in order to meet the specification.

To estimate how much better (how much smaller standard deviation) it would have to be,
we need to get the 5% quantile to be located at the under_weight limit, 2.9
*/
double p = 0.05; // wanted p th quantile.
ex_out << "Quantile of " << p << " = " << quantile(packs, p)
  << ", mean = " << packs.mean() << ", sd = " << packs.standard_deviation() << endl;
/*`
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1

With the current packer (mean = 3, sd = 0.1), the 5% quantile is at 2.8551 kg,
a little below our target of 2.9 kg.
So we know that the standard deviation is going to have to be smaller.

Let's start by guessing that it (now 0.1) needs to be halved, to a standard deviation of 0.05 kg.
*/
normal pack05(mean, 0.05);
ex_out << "Quantile of " << p << " = " << quantile(pack05, p)
  << ", mean = " << pack05.mean() << ", sd = " << pack05.standard_deviation() << endl;
// Quantile of 0.05 = 2.91776, mean = 3, sd = 0.05

ex_out <<"Fraction of packs >= " << minimum_weight << " with a mean of " << mean
  << " and standard deviation of " << pack05.standard_deviation()
  << " is " << cdf(complement(pack05, minimum_weight)) << endl;
// Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.97725
/*`
So 0.05 was quite a good guess, but we are a little over the 2.9 target,
so the standard deviation could be a tiny bit more. So we could do some
more guessing to get closer, say by increasing standard deviation to 0.06 kg,
constructing another new distribution called pack06.
*/
normal pack06(mean, 0.06);
ex_out << "Quantile of " << p << " = " << quantile(pack06, p)
  << ", mean = " << pack06.mean() << ", sd = " << pack06.standard_deviation() << endl;
// Quantile of 0.05 = 2.90131, mean = 3, sd = 0.06

ex_out <<"Fraction of packs >= " << minimum_weight << " with a mean of " << mean
  << " and standard deviation of " << pack06.standard_deviation()
  << " is " << cdf(complement(pack06, minimum_weight)) << endl;
// Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.95221
/*`
Now we are getting really close, but to do the job properly,
we might need to use root finding method, for example the tools provided,
and used elsewhere, in the Math Toolkit, see __root_finding_without_derivatives

But in this (normal) distribution case, we can and should be even smarter
and make a direct calculation.
*/

/*`Our required limit is minimum_weight = 2.9 kg, often called the random variate z.
For a standard normal distribution, then probability p = N((minimum_weight - mean) / sd).

We want to find the standard deviation that would be required to meet this limit,
so that the p th quantile is located at z (minimum_weight).
In this case, the 0.05 (5%) quantile is at 2.9 kg pack weight, when the mean is 3 kg,
ensuring that 0.95 (95%) of packs are above the minimum weight.

Rearranging, we can directly calculate the required standard deviation:
*/
normal N01; // standard normal distribution with mean zero and unit standard deviation.
p = 0.05;
double qp = quantile(N01, p);
double sd95 = (minimum_weight - mean) / qp;

ex_out << "For the "<< p << "th quantile to be located at "
  << minimum_weight << ", would need a standard deviation of " << sd95 << endl;
// For the 0.05th quantile to be located at 2.9, would need a standard deviation of 0.0607957

/*`We can now construct a new (normal) distribution pack95 for the 'better' packer,
and check that our distribution will meet the specification.
*/

normal pack95(mean, sd95);
ex_out <<"Fraction of packs >= " << minimum_weight << " with a mean of " << mean
  << " and standard deviation of " << pack95.standard_deviation()
  << " is " << cdf(complement(pack95, minimum_weight)) << endl;
// Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0607957 is 0.95

/*`This calculation is generalized in the free function find_scale,
as shown below, giving the same standard deviation.
*/
double ss = find_scale<normal>(minimum_weight, under_fraction, packs.mean());
ex_out << "find_scale<normal>(minimum_weight, under_fraction, packs.mean()); " << ss << endl;
// find_scale<normal>(minimum_weight, under_fraction, packs.mean()); 0.0607957

/*`If we had defined an over_fraction, or percentage that must pass specification
*/
double over_fraction = 0.95;
/*`And (wrongly) written

 double sso = find_scale<normal>(minimum_weight, over_fraction, packs.mean());

With the default policy, we would get a message like

[pre
Message from thrown exception was:
   Error in function boost::math::find_scale<Dist, Policy>(double, double, double, Policy):
   Computed scale (-0.060795683191176959) is <= 0! Was the complement intended?
]

But this would return a *negative* standard deviation - obviously impossible.
The probability should be 1 - over_fraction, not over_fraction, thus:
*/

double ss1o = find_scale<normal>(minimum_weight, 1 - over_fraction, packs.mean());
ex_out << "find_scale<normal>(minimum_weight, under_fraction, packs.mean()); " << ss1o << endl;
// find_scale<normal>(minimum_weight, under_fraction, packs.mean()); 0.0607957

/*`But notice that using '1 - over_fraction' - will lead to a
loss of accuracy, especially if over_fraction was close to unity. (See __why_complements).
In this (very common) case, we should instead use the __complements,
giving the most accurate result.
*/

double ssc = find_scale<normal>(complement(minimum_weight, over_fraction, packs.mean()));
ex_out << "find_scale<normal>(complement(minimum_weight, over_fraction, packs.mean())); " << ssc << endl;
// find_scale<normal>(complement(minimum_weight, over_fraction, packs.mean())); 0.0607957

/*`Note that our guess of 0.06 was close to the accurate value of 0.060795683191176959.

We can again confirm our prediction thus:
*/

normal pack95c(mean, ssc);
ex_out <<"Fraction of packs >= " << minimum_weight << " with a mean of " << mean
  << " and standard deviation of " << pack95c.standard_deviation()
  << " is " << cdf(complement(pack95c, minimum_weight)) << endl;
// Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0607957 is 0.95

/*`Notice that these two deceptively simple questions:

* Do we over-fill to make sure we meet a minimum specification (or under-fill to avoid an overdose)?

and/or

* Do we measure better?

are actually extremely common.

The weight of beef might be replaced by a measurement of more or less anything,
from drug tablet content, Apollo landing rocket firing, X-ray treatment doses...

The scale can be variation in dispensing or uncertainty in measurement.
*/
//] [/normal_find_location_and_scale_eg Quickbook end]

  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies
    // are to throw exceptions on arguments that cause errors like underflow, overflow.
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << endl;
  }
  return 0;
    return 0;  // main's implicit return
}  // int example_main()


/*

Output is:

//[normal_find_location_and_scale_output

Find_location (mean) and find_scale (standard deviation) examples.
Percentage of packs > 3.1 is 15.8655
Fraction of packs <= 2.9 with a mean of 3 is 0.841345
Fraction of packs >= 2.9 with a mean of 3.06449 is 0.950005
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95
Cauchy Setting the packer to 3 will mean that fraction of packs >= 2.9 is 0.75
find_location<cauchy>(minimum_weight, over fraction, standard_deviation); 3.53138
Cauchy Setting the packer to 3.53138 will mean that fraction of packs >= 2.9 is 0.95
Cauchy Setting the packer to -0.282052 will mean that fraction of packs >= 2.9 is 0.95
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1
Quantile of 0.05 = 2.91776, mean = 3, sd = 0.05
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.97725
Quantile of 0.05 = 2.90131, mean = 3, sd = 0.06
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.95221
For the 0.05th quantile to be located at 2.9, would need a standard deviation of 0.0607957
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0607957 is 0.95
find_scale<normal>(minimum_weight, under_fraction, packs.mean()); 0.0607957
find_scale<normal>(minimum_weight, under_fraction, packs.mean()); 0.0607957
find_scale<normal>(complement(minimum_weight, over_fraction, packs.mean())); 0.0607957
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0607957 is 0.95

//] [/normal_find_location_and_scale_eg_output]

*/

TEST_CASE("example: find_mean_and_sd_normal") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Find_location (mean) and find_scale (standard deviation) examples.
Percentage of packs > 3.1 is 15.8655
Fraction of packs <= 2.9 with a mean of 3 is 0.841345
Fraction of packs >= 2.9 with a mean of 3.06449 is 0.950005
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95
Cauchy Setting the packer to 3 will mean that fraction of packs >= 2.9 is 0.75
find_location<cauchy>(minimum_weight, over fraction, standard_deviation); 3.53138
Cauchy Setting the packer to 3.53138 will mean that fraction of packs >= 2.9 is 0.95
Cauchy Setting the packer to -0.282052 will mean that fraction of packs >= 2.9 is 0.95
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1
Quantile of 0.05 = 2.91776, mean = 3, sd = 0.05
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.97725
Quantile of 0.05 = 2.90131, mean = 3, sd = 0.06
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.95221
For the 0.05th quantile to be located at 2.9, would need a standard deviation of 0.0607957
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0607957 is 0.95
find_scale<normal>(minimum_weight, under_fraction, packs.mean()); 0.0607957
find_scale<normal>(minimum_weight, under_fraction, packs.mean()); 0.0607957
find_scale<normal>(complement(minimum_weight, over_fraction, packs.mean())); 0.0607957
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0607957 is 0.95
)ex"));
#endif
}
}  // namespace ex_find_mean_and_sd_normal

// libs/math/example/fourier_transform_daubechies_ulp_plot.cpp
namespace ex_fourier_transform_daubechies_ulp_plot {
using boost::math::fourier_transform_daubechies_scaling;
using boost::math::tools::ulps_plot;

template<int p>
void real_part() {
   auto phi_real_hi_acc = [](double omega) {
       auto z = fourier_transform_daubechies_scaling<double, p>(omega);
       return z.real();
   };

   auto phi_real_lo_acc = [](float omega) {
      auto z = fourier_transform_daubechies_scaling<float, p>(omega);
      return z.real();
   };
   auto plot = ulps_plot<decltype(phi_real_hi_acc), double, float>(phi_real_hi_acc, float(0.0), float(100.0), 20000);
   plot.ulp_envelope(false);
   plot.add_fn(phi_real_lo_acc);
   plot.clip(100);
   plot.title("Accuracy of 𝔑(𝓕[𝜙](ω)) with " + std::to_string(p) + " vanishing moments.");
   plot.write("real_ft_daub_scaling_"  + std::to_string(p) + ".svg");
 
}

template<int p>
void imaginary_part() {
   auto phi_imag_hi_acc = [](double omega) {
       auto z = fourier_transform_daubechies_scaling<double, p>(omega);
       return z.imag();
   };

   auto phi_imag_lo_acc = [](float omega) {
      auto z = fourier_transform_daubechies_scaling<float, p>(omega);
      return z.imag();
   };
   auto plot = ulps_plot<decltype(phi_imag_hi_acc), double, float>(phi_imag_hi_acc, float(0.0), float(100.0), 20000);
   plot.ulp_envelope(false);
   plot.add_fn(phi_imag_lo_acc);
   plot.clip(100);
   plot.title("Accuracy of 𝕴(𝓕[𝜙](ω)) with " + std::to_string(p) + " vanishing moments.");
   plot.write("imag_ft_daub_scaling_"  + std::to_string(p) + ".svg");
 
}


int example_main() {
   real_part<3>();
   imaginary_part<3>();
   real_part<6>();
   imaginary_part<6>();
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: fourier_transform_daubechies_ulp_plot") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
#endif
}
}  // namespace ex_fourier_transform_daubechies_ulp_plot

// libs/math/example/hyperexponential_snips.cpp
namespace ex_hyperexponential_snips {
#ifdef _MSC_VER
#  pragma warning (disable : 4996) // disable -D_SCL_SECURE_NO_WARNINGS C++ 'Checked Iterators'
#endif


#ifndef BOOST_NO_CXX11_HDR_ARRAY
#endif

int example_main()
{
   {
//[hyperexponential_snip1
//=#include <boost/math/distributions/hyperexponential.hpp>
//=#include <iostream>
//=int example_main()
//={
   const double rates[] = { 1.0 / 10.0, 1.0 / 12.0 };

   boost::math::hyperexponential he(rates);

   ex_out << "Average lifetime: "
      << boost::math::mean(he)
      << " years" << std::endl;
   ex_out << "Probability that the appliance will work for more than 15 years: "
      << boost::math::cdf(boost::math::complement(he, 15.0))
      << std::endl;
//=}
//]
   }
   using namespace boost::math;
#ifndef BOOST_NO_CXX11_HDR_ARRAY
   {
   //[hyperexponential_snip2
   std::array<double, 2> phase_prob = { 0.5, 0.5 };
   std::array<double, 2> rates = { 1.0 / 10, 1.0 / 12 };

   hyperexponential he(phase_prob.begin(), phase_prob.end(), rates.begin(), rates.end());
   //]
   }

   {
   //[hyperexponential_snip3
   // We could be using any standard library container here... vector, deque, array, list etc:
   std::array<double, 2> phase_prob = { 0.5, 0.5 };
   std::array<double, 2> rates      = { 1.0 / 10, 1.0 / 12 };

   hyperexponential he1(phase_prob, rates);    // Construct from standard library container.

   double phase_probs2[] = { 0.5, 0.5 };
   double rates2[]       = { 1.0 / 10, 1.0 / 12 };

   hyperexponential he2(phase_probs2, rates2);  // Construct from native C++ array.
   //]
   }
   {
   //[hyperexponential_snip4
   // We could be using any standard library container here... vector, deque, array, list etc:
   std::array<double, 2> rates = { 1.0 / 10, 1.0 / 12 };

   hyperexponential he(rates.begin(), rates.end());

   BOOST_MATH_ASSERT(he.probabilities()[0] == 0.5); // Phase probabilities will be equal and normalised to unity.
   //]
   }
   {
   //[hyperexponential_snip5
   std::array<double, 2> rates = { 1.0 / 10, 1.0 / 12 };

   hyperexponential he(rates);

   BOOST_MATH_ASSERT(he.probabilities()[0] == 0.5); // Phase probabilities will be equal and normalised to unity.
   //]
   }
#endif
#if !defined(BOOST_NO_CXX11_HDR_INITIALIZER_LIST) && !(defined(BOOST_GCC_VERSION) && (BOOST_GCC_VERSION < 40500))
   {
   //[hyperexponential_snip6
   hyperexponential he = { { 0.5, 0.5 }, { 1.0 / 10, 1.0 / 12 } };
   //]
   }
   {
   //[hyperexponential_snip7
   hyperexponential he = { 1.0 / 10, 1.0 / 12 };

   BOOST_MATH_ASSERT(he.probabilities()[0] == 0.5);
   //]
   }
#endif
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: hyperexponential_snips") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Average lifetime: 11 years
Probability that the appliance will work for more than 15 years: 0.254817
)ex"));
#endif
}
}  // namespace ex_hyperexponential_snips

// libs/math/example/inverse_chi_squared_find_df_example.cpp
namespace ex_inverse_chi_squared_find_df_example {
using boost::math::inverse_chi_squared_distribution;  // inverse_chi_squared_distribution.
using boost::math::inverse_chi_squared; //typedef for nverse_chi_squared_distribution double.

    using std::endl;
using std::setprecision;
using std::setw;
using std::sqrt;

int example_main()
{
  ex_out << "Example using Inverse chi squared distribution to find df. " << endl;
  try
  {
    ex_out.precision(std::numeric_limits<double>::max_digits10); // 
    int i = std::numeric_limits<double>::max_digits10;
    ex_out << "Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = " << i << endl; 

    ex_out.precision(3);
    double nu = 10.;
    double scale1 = 1./ nu; // 1st definition sigma^2 = 1/df;
    double scale2 = 1.; // 2nd definition sigma^2 = 1
    inverse_chi_squared sichsq(nu, 1/nu); // Explicitly scaled to default scale = 1/df.
    inverse_chi_squared ichsq(nu); // Implicitly scaled to default scale = 1/df.
    // Try degrees of freedom estimator

    //double df = chi_squared::find_degrees_of_freedom(-diff, alpha[i], alpha[i], variance);

    ex_out << "ichsq.degrees_of_freedom() = " << ichsq.degrees_of_freedom() << endl;

    double diff = 0.5;  // difference from variance to detect (delta).
    double variance = 1.; // true variance
    double alpha = 0.9;
    double beta = 0.9;

    ex_out << "diff = " << diff 
      << ", variance = " << variance << ", ratio = " << diff/variance
      << ", alpha = " << alpha << ", beta = " << beta << endl;

    /* inverse_chi_square_df_estimator is not in the code base anymore?

    using boost::math::detail::inverse_chi_square_df_estimator;
    using boost::math::policies::default_policy;
    inverse_chi_square_df_estimator<> a_df(alpha, beta, variance, diff);

    ex_out << "df    est" << endl;
    for (double df = 1; df < 3; df += 0.1)
    {
      double est_df = a_df(1);
      ex_out << df << "    " << a_df(df) << endl;
    }
    */

    //template <class F, class T, class Tol, class Policy>std::pair<T, T> 
    // bracket_and_solve_root(F f, const T& guess, T factor, bool rising, Tol tol, std::uintmax_t& max_iter, const Policy& pol)


    // TODO: Not implemented
    //double df = inverse_chi_squared_distribution<>::find_degrees_of_freedom(diff, alpha, beta, variance, 0);
    //ex_out << df << endl;
  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies 
    // are to throw exceptions on arguments that cause errors like underflow, overflow. 
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:

  Example using Inverse chi squared distribution to find df. 
  Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = 17
  10
  
  Message from thrown exception was:
     Error in function boost::math::inverse_chi_squared_distribution<double>::inverse_chi_squared_distribution: Degrees of freedom argument is 1.#INF, but must be > 0 !
diff = 0.5, variance = 1, ratio = 0.5, alpha = 0.1, beta = 0.1
  df    est
  1    1
  ratio+1 = 1.5, quantile(0.1) = 0.00618, cdf = 6.5e-037, result = -0.1
  1.1    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.00903, cdf = 1.2e-025, result = -0.1
  1.2    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0125, cdf = 8.25e-019, result = -0.1
  1.3    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0166, cdf = 2.17e-014, result = -0.1
  1.4    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0212, cdf = 2.2e-011, result = -0.1
  1.5    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0265, cdf = 3e-009, result = -0.1
  1.6    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0323, cdf = 1.11e-007, result = -0.1
  1.7    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0386, cdf = 1.7e-006, result = -0.1
  1.8    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0454, cdf = 1.41e-005, result = -0.1
  1.9    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0527, cdf = 7.55e-005, result = -0.1
  2    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0604, cdf = 0.000291, result = -0.1
  2.1    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0685, cdf = 0.00088, result = -0.1
  2.2    -0.1
  ratio+1 = 1.5, quantile(0.1) = 0.0771, cdf = 0.0022, result = -0.0999
  2.3    -0.0999
  ratio+1 = 1.5, quantile(0.1) = 0.0859, cdf = 0.00475, result = -0.0997
  2.4    -0.0997
  ratio+1 = 1.5, quantile(0.1) = 0.0952, cdf = 0.00911, result = -0.0993
  2.5    -0.0993
  ratio+1 = 1.5, quantile(0.1) = 0.105, cdf = 0.0159, result = -0.0984
  2.6    -0.0984
  ratio+1 = 1.5, quantile(0.1) = 0.115, cdf = 0.0257, result = -0.0967
  2.7    -0.0967
  ratio+1 = 1.5, quantile(0.1) = 0.125, cdf = 0.039, result = -0.094
  2.8    -0.094
  ratio+1 = 1.5, quantile(0.1) = 0.135, cdf = 0.056, result = -0.0897
  2.9    -0.0897
  ratio+1 = 1.5, quantile(0.1) = 20.6, cdf = 1, result = 0.9

    ichsq.degrees_of_freedom() = 10
  diff = 0.5, variance = 1, ratio = 0.5, alpha = 0.9, beta = 0.9
  df    est
  1    1
  ratio+1 = 1.5, quantile(0.9) = 0.729, cdf = 0.269, result = -0.729
  1.1    -0.729
  ratio+1 = 1.5, quantile(0.9) = 0.78, cdf = 0.314, result = -0.693
  1.2    -0.693
  ratio+1 = 1.5, quantile(0.9) = 0.83, cdf = 0.36, result = -0.655
  1.3    -0.655
  ratio+1 = 1.5, quantile(0.9) = 0.879, cdf = 0.405, result = -0.615
  1.4    -0.615
  ratio+1 = 1.5, quantile(0.9) = 0.926, cdf = 0.449, result = -0.575
  1.5    -0.575
  ratio+1 = 1.5, quantile(0.9) = 0.973, cdf = 0.492, result = -0.535
  1.6    -0.535
  ratio+1 = 1.5, quantile(0.9) = 1.02, cdf = 0.534, result = -0.495
  1.7    -0.495
  ratio+1 = 1.5, quantile(0.9) = 1.06, cdf = 0.574, result = -0.455
  1.8    -0.455
  ratio+1 = 1.5, quantile(0.9) = 1.11, cdf = 0.612, result = -0.417
  1.9    -0.417
  ratio+1 = 1.5, quantile(0.9) = 1.15, cdf = 0.648, result = -0.379
  2    -0.379
  ratio+1 = 1.5, quantile(0.9) = 1.19, cdf = 0.681, result = -0.342
  2.1    -0.342
  ratio+1 = 1.5, quantile(0.9) = 1.24, cdf = 0.713, result = -0.307
  2.2    -0.307
  ratio+1 = 1.5, quantile(0.9) = 1.28, cdf = 0.742, result = -0.274
  2.3    -0.274
  ratio+1 = 1.5, quantile(0.9) = 1.32, cdf = 0.769, result = -0.242
  2.4    -0.242
  ratio+1 = 1.5, quantile(0.9) = 1.36, cdf = 0.793, result = -0.212
  2.5    -0.212
  ratio+1 = 1.5, quantile(0.9) = 1.4, cdf = 0.816, result = -0.184
  2.6    -0.184
  ratio+1 = 1.5, quantile(0.9) = 1.44, cdf = 0.836, result = -0.157
  2.7    -0.157
  ratio+1 = 1.5, quantile(0.9) = 1.48, cdf = 0.855, result = -0.133
  2.8    -0.133
  ratio+1 = 1.5, quantile(0.9) = 1.52, cdf = 0.872, result = -0.11
  2.9    -0.11
  ratio+1 = 1.5, quantile(0.9) = 29.6, cdf = 1, result = 0.1


  */
  

TEST_CASE("example: inverse_chi_squared_find_df_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Example using Inverse chi squared distribution to find df. 
Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = 17
ichsq.degrees_of_freedom() = 10
diff = 0.5, variance = 1, ratio = 0.5, alpha = 0.9, beta = 0.9
)ex"));
#endif
}
}  // namespace ex_inverse_chi_squared_find_df_example

// libs/math/example/laplace_example.cpp
namespace ex_laplace_example {
  using boost::math::laplace; // typedef provides default type is double.
  using boost::math::normal; // typedef provides default type is double.

   using std::endl; using std::left; using std::showpoint; using std::noshowpoint;
  using std::setw; using std::setprecision;
  using std::numeric_limits;

int example_main()
{
  ex_out << "Example: Laplace distribution." << endl;

  try
  {
    { // Traditional tables and values.
/*`Let's start by printing some traditional tables.
*/      
      double step = 1.; // in z 
      double range = 4; // min and max z = -range to +range.
      //int precision = 17; // traditional tables are only computed to much lower precision.
      int precision = 4; // traditional table at much lower precision.
      int width = 10; // for use with setw.

      // Construct standard laplace & normal distributions l & s
        normal s; // (default location or mean = zero, and scale or standard deviation = unity)
        ex_out << "Standard normal distribution, mean or location = "<< s.location()
          << ", standard deviation or scale = " << s.scale() << endl;
        laplace l; // (default mean = zero, and standard deviation = unity)
        ex_out << "Laplace normal distribution, location = "<< l.location()
          << ", scale = " << l.scale() << endl;

/*` First the probability distribution function (pdf).
*/
      ex_out << "Probability distribution function values" << endl;
      ex_out << " z  PDF  normal     laplace    (difference)" << endl;
      ex_out.precision(5);
      for (double z = -range; z < range + step; z += step)
      {
        ex_out << left << setprecision(3) << setw(6) << z << " " 
          << setprecision(precision) << setw(width) << pdf(s, z) << "  "
          << setprecision(precision) << setw(width) << pdf(l, z)<<  "  ("
          << setprecision(precision) << setw(width) << pdf(l, z) - pdf(s, z) // difference.
          << ")" << endl;
      }
      ex_out.precision(6); // default
/*`Notice how the laplace is less at z = 1 , but has 'fatter' tails at 2 and 3. 

   And the area under the normal curve from -[infin] up to z,
   the cumulative distribution function (cdf).
*/
      // For a standard distribution 
      ex_out << "Standard location = "<< s.location()
        << ", scale = " << s.scale() << endl;
      ex_out << "Integral (area under the curve) from - infinity up to z " << endl;
      ex_out << " z  CDF  normal     laplace    (difference)" << endl;
      for (double z = -range; z < range + step; z += step)
      {
        ex_out << left << setprecision(3) << setw(6) << z << " " 
          << setprecision(precision) << setw(width) << cdf(s, z) << "  "
          << setprecision(precision) << setw(width) << cdf(l, z) <<  "  ("
          << setprecision(precision) << setw(width) << cdf(l, z) - cdf(s, z) // difference.
          << ")" << endl;
      }
      ex_out.precision(6); // default

/*`
Pretty-printing a traditional 2-dimensional table is left as an exercise for the student,
but why bother now that the Boost Math Toolkit lets you write
*/
    double z = 2.; 
    ex_out << "Area for gaussian z = " << z << " is " << cdf(s, z) << endl; // to get the area for z.
    ex_out << "Area for laplace z = " << z << " is " << cdf(l, z) << endl; // 
/*`
Correspondingly, we can obtain the traditional 'critical' values for significance levels.
For the 95% confidence level, the significance level usually called alpha,
is 0.05 = 1 - 0.95 (for a one-sided test), so we can write
*/
     ex_out << "95% of gaussian area has a z below " << quantile(s, 0.95) << endl;
     ex_out << "95% of laplace area has a z below " << quantile(l, 0.95) << endl;
   // 95% of area has a z below 1.64485
   // 95% of laplace area has a z below 2.30259
/*`and a two-sided test (a comparison between two levels, rather than a one-sided test)

*/
     ex_out << "95% of gaussian area has a z between " << quantile(s, 0.975)
       << " and " << -quantile(s, 0.975) << endl;
     ex_out << "95% of laplace area has a z between " << quantile(l, 0.975)
       << " and " << -quantile(l, 0.975) << endl;
   // 95% of area has a z between 1.95996 and -1.95996
   // 95% of laplace area has a z between 2.99573 and -2.99573
/*`Notice how much wider z has to be to enclose 95% of the area.
*/
  }
//] [/[laplace_example1]
  }
  catch(const std::exception& e)
  { // Always useful to include try & catch blocks because default policies 
    // are to throw exceptions on arguments that cause errors like underflow, overflow. 
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:

Example: Laplace distribution.
Standard normal distribution, mean or location = 0, standard deviation or scale = 1
Laplace normal distribution, location = 0, scale = 1
Probability distribution function values
 z  PDF  normal     laplace    (difference)
-4     0.0001338   0.009158    (0.009024  )
-3     0.004432    0.02489     (0.02046   )
-2     0.05399     0.06767     (0.01368   )
-1     0.242       0.1839      (-0.05803  )
0      0.3989      0.5         (0.1011    )
1      0.242       0.1839      (-0.05803  )
2      0.05399     0.06767     (0.01368   )
3      0.004432    0.02489     (0.02046   )
4      0.0001338   0.009158    (0.009024  )
Standard location = 0, scale = 1
Integral (area under the curve) from - infinity up to z 
 z  CDF  normal     laplace    (difference)
-4     3.167e-005  0.009158    (0.009126  )
-3     0.00135     0.02489     (0.02354   )
-2     0.02275     0.06767     (0.04492   )
-1     0.1587      0.1839      (0.02528   )
0      0.5         0.5         (0         )
1      0.8413      0.8161      (-0.02528  )
2      0.9772      0.9323      (-0.04492  )
3      0.9987      0.9751      (-0.02354  )
4      1           0.9908      (-0.009126 )
Area for gaussian z = 2 is 0.97725
Area for laplace z = 2 is 0.932332
95% of gaussian area has a z below 1.64485
95% of laplace area has a z below 2.30259
95% of gaussian area has a z between 1.95996 and -1.95996
95% of laplace area has a z between 2.99573 and -2.99573

*/

TEST_CASE("example: laplace_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Example: Laplace distribution.
Standard normal distribution, mean or location = 0, standard deviation or scale = 1
Laplace normal distribution, location = 0, scale = 1
Probability distribution function values
 z  PDF  normal     laplace    (difference)
-4     0.0001338   0.009158    (0.009024  )
-3     0.004432    0.02489     (0.02046   )
-2     0.05399     0.06767     (0.01368   )
-1     0.242       0.1839      (-0.05803  )
0      0.3989      0.5         (0.1011    )
1      0.242       0.1839      (-0.05803  )
2      0.05399     0.06767     (0.01368   )
3      0.004432    0.02489     (0.02046   )
4      0.0001338   0.009158    (0.009024  )
Standard location = 0, scale = 1
Integral (area under the curve) from - infinity up to z 
 z  CDF  normal     laplace    (difference)
-4     3.167e-05   0.009158    (0.009126  )
-3     0.00135     0.02489     (0.02354   )
-2     0.02275     0.06767     (0.04492   )
-1     0.1587      0.1839      (0.02528   )
0      0.5         0.5         (0         )
1      0.8413      0.8161      (-0.02528  )
2      0.9772      0.9323      (-0.04492  )
3      0.9987      0.9751      (-0.02354  )
4      1           0.9908      (-0.009126 )
Area for gaussian z = 2 is 0.97725
Area for laplace z = 2 is 0.932332
95% of gaussian area has a z below 1.64485
95% of laplace area has a z below 2.30259
95% of gaussian area has a z between 1.95996 and -1.95996
95% of laplace area has a z between 2.99573 and -2.99573
)ex"));
#endif
}
}  // namespace ex_laplace_example

// libs/math/example/lexical_cast_nonfinite_facets.cpp
namespace ex_lexical_cast_nonfinite_facets {
using boost::math::nonfinite_num_get;
using boost::math::nonfinite_num_put;

using boost::lexical_cast;


using std::endl;


using std::setw;
using std::left;
using std::right;
using std::internal;

using std::string;

using std::istringstream;

using std::numeric_limits;

using std::locale;


int example_main()
{
  ex_out << "finite_num_facet with lexical_cast example." << std::endl;

  // Example of using non_finite num_put and num_get facets with lexical_cast.
  //locale old_locale;
  //locale tmp_locale(old_locale, new nonfinite_num_put<char>);
  //// Create a new temporary output locale, and add the output nonfinite_num_put facet.

  //locale new_locale(tmp_locale, new nonfinite_num_get<char>);
  // Create a new output locale (from the tmp locale), and add the input nonfinite_num_get facet.

  // Note that you can only add facets one at a time, 
  // unless you chain thus:
  
  std::locale new_locale(std::locale(std::locale(),
    new boost::math::nonfinite_num_put<char>),
    new boost::math::nonfinite_num_get<char>);
  
  locale::global(new_locale); // Newly constructed streams
  // (including those streams inside lexical_cast)
  // now use new_locale with nonfinite facets.

  // Output using the new locale.
  ex_out << "Using C99_out_locale " << endl;
  ex_out.imbue(new_locale);
  // Necessary because ex_out already constructed using default C locale,
  // and default facets for nonfinites.

    // Create plus and minus infinity.
  double plus_infinity = +std::numeric_limits<double>::infinity();
  double minus_infinity = -std::numeric_limits<double>::infinity();

  // and create a NaN (NotANumber)
  double NaN = +std::numeric_limits<double>::quiet_NaN ();
  ex_out << "+std::numeric_limits<double>::infinity() = " << plus_infinity << endl;
  ex_out << "-std::numeric_limits<double>::infinity() = " << minus_infinity << endl;
  ex_out << "+std::numeric_limits<double>::quiet_NaN () = " << NaN << endl;

  // Now try some 'round-tripping', 'reading' "inf".
  double x = boost::lexical_cast<double>("inf");
  // and check we get a floating-point infinity.
  BOOST_MATH_ASSERT(x == std::numeric_limits<double>::infinity());
  ex_out << "boost::lexical_cast<double>(\"inf\") = " << x << endl;

  // Check we can convert the other way from floating-point infinity,
  string s = boost::lexical_cast<string>(numeric_limits<double>::infinity());
  // to a C99 string representation as "inf".
  BOOST_MATH_ASSERT(s == "inf");

  // Finally try full 'round-tripping' (in both directions):
  BOOST_MATH_ASSERT(lexical_cast<double>(lexical_cast<string>(numeric_limits<double>::infinity()))
    == numeric_limits<double>::infinity());
  BOOST_MATH_ASSERT(lexical_cast<string>(lexical_cast<double>("inf")) == "inf");

    return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

Output:
  finite_num_facet with lexical_cast example.
  Using C99_out_locale 
  +std::numeric_limits<double>::infinity() = inf
  -std::numeric_limits<double>::infinity() = -inf
  +std::numeric_limits<double>::quiet_NaN () = nan
  boost::lexical_cast<double>("inf") = inf


*/

TEST_CASE("example: lexical_cast_nonfinite_facets") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(finite_num_facet with lexical_cast example.
Using C99_out_locale 
+std::numeric_limits<double>::infinity() = inf
-std::numeric_limits<double>::infinity() = -inf
+std::numeric_limits<double>::quiet_NaN () = nan
boost::lexical_cast<double>("inf") = inf
)ex"));
#endif
}
}  // namespace ex_lexical_cast_nonfinite_facets

// libs/math/example/neg_binomial_sample_sizes.cpp
namespace ex_neg_binomial_sample_sizes {
using boost::math::negative_binomial;

// Default RealType is double so this permits use of:
double find_minimum_number_of_trials(
double k,     // number of failures (events), k >= 0.
double p,     // fraction of trails for which event occurs, 0 <= p <= 1.
double probability); // probability threshold, 0 <= probability <= 1.


using std::endl;
using std::fixed;
using std::right;
using std::setprecision;
using std::setw; 

//[neg_binomial_sample_sizes

/*`
It centres around a routine that prints out a table of 
minimum sample sizes (number of trials) for various probability thresholds:
*/
  void find_number_of_trials(double failures, double p);
/*`
First define a table of significance levels: these are the maximum 
acceptable probability that /failure/ or fewer events will be observed.
*/
  double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
/*`
Confidence value as % is (1 - alpha) * 100, so alpha 0.05 == 95% confidence
that the desired number of failures will be observed.
The values range from a very low 0.5 or 50% confidence up to an extremely high
confidence of 99.999.

Much of the rest of the program is pretty-printing, the important part
is in the calculation of minimum number of trials required for each
value of alpha using:

  (int)ceil(negative_binomial::find_minimum_number_of_trials(failures, p, alpha[i]);

find_minimum_number_of_trials returns a double,
so `ceil` rounds this up to ensure we have an integral minimum number of trials.
*/
  
void find_number_of_trials(double failures, double p)
{
   // trials = number of trials
   // failures = number of failures before achieving required success(es).
   // p        = success fraction (0 <= p <= 1.).
   //
   // Calculate how many trials we need to ensure the
   // required number of failures DOES exceed "failures".

  ex_out << "\n""Target number of failures = " << (int)failures;
  ex_out << ",   Success fraction = " << fixed << setprecision(1) << 100 * p << "%" << endl;
   // Print table header:
   ex_out << "____________________________\n"
           "Confidence        Min Number\n"
           " Value (%)        Of Trials \n"
           "____________________________\n";
   // Now print out the data for the alpha table values.
  for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   { // Confidence values %:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]) << "      "
      // find_minimum_number_of_trials
      << setw(6) << right
      << (int)ceil(negative_binomial::find_minimum_number_of_trials(failures, p, alpha[i]))
      << endl;
   }
   ex_out << endl;
} // void find_number_of_trials(double failures, double p)

/*` finally we can produce some tables of minimum trials for the chosen confidence levels:
*/

int example_main()
{
    find_number_of_trials(5, 0.5);
    find_number_of_trials(50, 0.5);
    find_number_of_trials(500, 0.5);
    find_number_of_trials(50, 0.1);
    find_number_of_trials(500, 0.1);
    find_number_of_trials(5, 0.9);

    return 0;
    return 0;  // main's implicit return
} // int example_main()

//]  [/neg_binomial_sample_sizes.cpp end of Quickbook in C++ markup]

/*

Output is:
Target number of failures = 5,   Success fraction = 50.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000          11
      75.000          14
      90.000          17
      95.000          18
      99.000          22
      99.900          27
      99.990          31
      99.999          36
  
  
  Target number of failures = 50,   Success fraction = 50.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000         101
      75.000         109
      90.000         115
      95.000         119
      99.000         128
      99.900         137
      99.990         146
      99.999         154
  
  
  Target number of failures = 500,   Success fraction = 50.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000        1001
      75.000        1023
      90.000        1043
      95.000        1055
      99.000        1078
      99.900        1104
      99.990        1126
      99.999        1146
  
  
  Target number of failures = 50,   Success fraction = 10.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000          56
      75.000          58
      90.000          60
      95.000          61
      99.000          63
      99.900          66
      99.990          68
      99.999          71
  
  
  Target number of failures = 500,   Success fraction = 10.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000         556
      75.000         562
      90.000         567
      95.000         570
      99.000         576
      99.900         583
      99.990         588
      99.999         594
  
  
  Target number of failures = 5,   Success fraction = 90.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000          57
      75.000          73
      90.000          91
      95.000         103
      99.000         127
      99.900         159
      99.990         189
      99.999         217
  
  
  Target number of failures = 5,   Success fraction = 95.0%
  ____________________________
  Confidence        Min Number
   Value (%)        Of Trials 
  ____________________________
      50.000         114
      75.000         148
      90.000         184
      95.000         208
      99.000         259
      99.900         324
      99.990         384
      99.999         442

*/

TEST_CASE("example: neg_binomial_sample_sizes") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(
Target number of failures = 5,   Success fraction = 50.0%
____________________________
Confidence        Min Number
 Value (%)        Of Trials 
____________________________
    50.000          11
    75.000          14
    90.000          17
    95.000          18
    99.000          22
    99.900          27
    99.990          31
    99.999          36


Target number of failures = 50,   Success fraction = 50.0%
____________________________
Confidence        Min Number
 Value (%)        Of Trials 
____________________________
    50.000         101
    75.000         109
    90.000         115
    95.000         119
    99.000         128
    99.900         137
    99.990         146
    99.999         154


Target number of failures = 500,   Success fraction = 50.0%
____________________________
Confidence        Min Number
 Value (%)        Of Trials 
____________________________
    50.000        1001
    75.000        1023
    90.000        1043
    95.000        1055
    99.000        1078
    99.900        1104
    99.990        1126
    99.999        1146


Target number of failures = 50,   Success fraction = 10.0%
____________________________
Confidence        Min Number
 Value (%)        Of Trials 
____________________________
    50.000          56
    75.000          58
    90.000          60
    95.000          61
    99.000          63
    99.900          66
    99.990          68
    99.999          71


Target number of failures = 500,   Success fraction = 10.0%
____________________________
Confidence        Min Number
 Value (%)        Of Trials 
____________________________
    50.000         556
    75.000         562
    90.000         567
    95.000         570
    99.000         576
    99.900         583
    99.990         588
    99.999         594


Target number of failures = 5,   Success fraction = 90.0%
____________________________
Confidence        Min Number
 Value (%)        Of Trials 
____________________________
    50.000          57
    75.000          73
    90.000          91
    95.000         103
    99.000         127
    99.900         159
    99.990         189
    99.999         217

)ex"));
#endif
}
}  // namespace ex_neg_binomial_sample_sizes

// libs/math/example/nonfinite_facet_simple.cpp
namespace ex_nonfinite_facet_simple {
#ifdef _MSC_VER
#  pragma warning (disable : 4127)  // conditional expression is constant.
#endif


using std::endl;


using std::setw;
using std::left;
using std::right;
using std::internal;

using std::string;

using std::istringstream;

using std::numeric_limits;

using std::locale;

// from Johan Rade Floating Point Utilities.

int example_main()
{
  ex_out << "Nonfinite_num_facet very simple example." << std::endl;

  if((std::numeric_limits<double>::has_infinity == 0) || (std::numeric_limits<double>::infinity() == 0))
  {
    ex_out << "Infinity not supported on this platform." << std::endl;
    return 0;
  }

  if((std::numeric_limits<double>::has_quiet_NaN == 0) || (std::numeric_limits<double>::quiet_NaN() == 0))
  {
    ex_out << "NaN not supported on this platform." << std::endl;
    return 0;
  }

  std::locale default_locale (std::locale::classic ()); // Note the current (default C) locale.

  // Create plus and minus infinity.
  double plus_infinity = +std::numeric_limits<double>::infinity();
  double minus_infinity = -std::numeric_limits<double>::infinity();

  // and create a NaN (NotANumber)
  double NaN = +std::numeric_limits<double>::quiet_NaN ();

  double negated_NaN = (boost::math::changesign)(std::numeric_limits<double>::quiet_NaN ());


  // Output the nonfinite values using the current (default C) locale.
  // The default representations differ from system to system,
  // for example, using Microsoft compilers, 1.#INF, -1.#INF, and 1.#QNAN,
  // Linux "inf", "-inf", "nan"
  ex_out << "Using C locale" << endl;
  ex_out << "+std::numeric_limits<double>::infinity() = " << plus_infinity << endl;
  ex_out << "-std::numeric_limits<double>::infinity() = " << minus_infinity << endl;
  ex_out << "+std::numeric_limits<double>::quiet_NaN () = " << NaN << endl;

  // Display negated NaN.
  ex_out << "negated NaN " << negated_NaN << endl; // "-1.IND" or "-nan".
  
  // Create a new output locale, and add the nonfinite_num_put facet
  std::locale C99_out_locale (default_locale, new boost::math::nonfinite_num_put<char>);
  // and imbue the ex_out stream with the new locale.
  ex_out.imbue (C99_out_locale);

  // Or for the same effect more concisely:
  ex_out.imbue (locale(locale(), new boost::math::nonfinite_num_put<char>));

  // Output using the new locale:
  ex_out << "Using C99_out_locale " << endl;
  ex_out << "+std::numeric_limits<double>::infinity() = " << plus_infinity << endl;
  ex_out << "-std::numeric_limits<double>::infinity() = " << minus_infinity << endl;
  ex_out << "+std::numeric_limits<double>::quiet_NaN () = " << NaN << endl;
  // Expect "inf", "-inf", "nan".

  // Display negated NaN.
  ex_out << "negated NaN " << negated_NaN << endl; // Expect "-nan".

  // Create a string with the expected C99 representation of plus infinity.
  std::string inf = "inf";
  { // Try to read an infinity value using the default C locale.
    // Create an input stream which will provide "inf"
    std::istringstream iss (inf);

     // Create a double ready to take the input,
    double infinity;
    // and read "inf" from the stringstream:
    iss >> infinity; 

    // This will not work on all platforms!  (Intel-Linux-13.0.1 fails EXIT STATUS: 139)
    if (! iss)
    { // Reading infinity went wrong!
      ex_out << "C locale input format error!" << std::endl;
    }
  } // Using default C locale.

  { // Now retry using C99 facets.
  // Create a new input locale and add the nonfinite_num_get facet.
  std::locale C99_in_locale (default_locale, new boost::math::nonfinite_num_get<char>);

  // Create an input stream which will provide "inf".
  std::istringstream iss (inf);
  // Imbue the stream with the C99 input locale.
  iss.imbue (C99_in_locale);

  // Create a double ready to take the input,
  double infinity;
  // and read from the stringstream:
  iss >> infinity; 

  if (! iss)
  { // Reading infinity went wrong!
    ex_out << "C99 input format error!" << std::endl;
  }
  // Expect to get an infinity, which will display still using the C99 locale as "inf"
  ex_out << "infinity in C99 representation is " << infinity << endl; 

  // To check, we can switch back to the default C locale.
  ex_out.imbue (default_locale);
  ex_out <<  "infinity in default C representation is " << infinity << endl; 
  } // using C99 locale.

  {
    // A 'loop-back example, output to a stringstream, and reading it back in.
    // Create C99 input and output locales. 
    std::locale C99_out_locale (default_locale, new boost::math::nonfinite_num_put<char>);
    std::locale C99_in_locale (default_locale, new boost::math::nonfinite_num_get<char>);

    std::ostringstream oss;
    oss.imbue(C99_out_locale);
    oss << plus_infinity;

    std::istringstream iss(oss.str()); // So stream contains "inf".
    iss.imbue (C99_in_locale);

    std::string s;

    iss >> s;

    ex_out.imbue(C99_out_locale);
    if (oss.str() != s)
    {
      ex_out << plus_infinity << " != " << s << " loopback failed!" << endl;
    }
    else
    {
      ex_out << plus_infinity << " == " << s << " as expected." << endl;
    }
  }


  // Example varying the width and position of the nonfinite representations.
  // With the nonfinite_num_put and _get facets, the width of the output is constant.

  #ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
  ex_out << "BOOST_NO_CXX11_NUMERIC_LIMITS is defined, so no max_digits10 available." << endl;
  std::streamsize  max_digits10 = 2 + std::numeric_limits<double>::digits * 30103UL / 100000UL;
#else
  // Can use new C++0X max_digits10 (the maximum potentially significant digits).
  std::streamsize  max_digits10 = std::numeric_limits<double>::max_digits10;
#endif
  ex_out << "std::numeric_limits<double>::max_digits10 is " << max_digits10 << endl;
  ex_out.precision(max_digits10);

  double pi = 3.141592653589793238462643383279502884197169399375105820974944;
  // Expect 17 (probably) decimal digits (regardless of locale).
  // ex_out has the default locale.
  ex_out << "pi = " << pi << endl; // pi = 3.1415926535897931
  ex_out.imbue (C99_out_locale); // Use ex_out with the C99 locale
  // (expect the same output for a double).
  ex_out << "pi = " << pi << endl; // pi = 3.1415926535897931

  ex_out << "infinity in C99 representation is " << plus_infinity << endl; 

  //int width = 2; // Check effect if width too small is OK.
  // (There was a disturbed layout on older MSVC?).
  int width = 20;

  // Similarly if we can switch back to the default C locale.
  ex_out.imbue (default_locale);
  ex_out <<  "infinity in default C representation is " << plus_infinity << endl; 
  ex_out <<  "infinity in default C representation (setw(" << width << ") is |" << setw(width) << plus_infinity <<'|' << endl; 
  ex_out <<  "infinity in default C representation (setw(" << width << ") is |" << left << setw(width) << plus_infinity <<'|' << endl; 
  ex_out <<  "infinity in default C representation (setw(" << width << ") is |" << internal << setw(width) << plus_infinity <<'|' << endl; 

  ex_out.imbue (C99_out_locale);
  ex_out << "infinity in C99 representation (setw(" << width << ") is |" << right << setw(width) << plus_infinity <<'|'<< endl; 
  ex_out << "infinity in C99 representation (setw(" << width << ") is |" << left << setw(width) << plus_infinity <<'|'<< endl; 
  ex_out << "infinity in C99 representation (setw(" << width << ") is |" << internal << setw(width) << plus_infinity <<'|'<< endl; 

  return 0;
    return 0;  // main's implicit return
} // int example_main()

// end of test_nonfinite_num_facets.cpp

/*

Output:

simple_nonfinite_facet.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\nonfinite_facet_simple.exe
  Nonfinite_num_facet very simple example.
  Using C locale
  +std::numeric_limits<double>::infinity() = 1.#INF
  -std::numeric_limits<double>::infinity() = -1.#INF
  +std::numeric_limits<double>::quiet_NaN () = 1.#QNAN
  Using C99_out_locale 
  +std::numeric_limits<double>::infinity() = inf
  -std::numeric_limits<double>::infinity() = -inf
  +std::numeric_limits<double>::quiet_NaN () = nan
  infinity in C99 representation is inf
  infinity in default C representation is 1.#INF
  3
  3
  inf == inf as expected.
  std::numeric_limits<double>::max_digits10 is 17
  pi = 3.1415926535897931
  C locale input format error!
  pi = 3.1415926535897931
  infinity in C99 representation is inf
  infinity in default C representation is 1.#INF
  infinity in default C representation (setw(20) is               1.#INF|
  infinity in default C representation (setw(20) is 1.#INF              |
  infinity in default C representation (setw(20) is               1.#INF|
  infinity in C99 representation (setw(20) is                  inf|
  infinity in C99 representation (setw(20) is inf                 |
  infinity in C99 representation (setw(20) is                  inf|

*/

TEST_CASE("example: nonfinite_facet_simple") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Nonfinite_num_facet very simple example.
Using C locale
+std::numeric_limits<double>::infinity() = inf
-std::numeric_limits<double>::infinity() = -inf
+std::numeric_limits<double>::quiet_NaN () = nan
negated NaN -nan
Using C99_out_locale 
+std::numeric_limits<double>::infinity() = inf
-std::numeric_limits<double>::infinity() = -inf
+std::numeric_limits<double>::quiet_NaN () = nan
negated NaN -nan
C locale input format error!
infinity in C99 representation is inf
infinity in default C representation is inf
inf == inf as expected.
std::numeric_limits<double>::max_digits10 is 17
pi = 3.1415926535897931
pi = 3.1415926535897931
infinity in C99 representation is inf
infinity in default C representation is inf
infinity in default C representation (setw(20) is |                 inf|
infinity in default C representation (setw(20) is |inf                 |
infinity in default C representation (setw(20) is |                 inf|
infinity in C99 representation (setw(20) is |                 inf|
infinity in C99 representation (setw(20) is |inf                 |
infinity in C99 representation (setw(20) is |                 inf|
)ex"));
#endif
}
}  // namespace ex_nonfinite_facet_simple

// libs/math/example/nonfinite_num_facet.cpp
namespace ex_nonfinite_num_facet {
#ifdef _MSC_VER
#   pragma warning(disable : 4127) // conditional expression is constant.
#endif


using std::endl;

using std::numeric_limits;



static const char sep = ','; // Separator of bracketed float and double values.

// Use max_digits10 (or equivalent) to obtain 
// all potentially significant decimal digits for the floating-point types.
    
#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
  std::streamsize  max_digits10_float = 2 + std::numeric_limits<float>::digits * 30103UL / 100000UL;
  std::streamsize  max_digits10_double = 2 + std::numeric_limits<double>::digits * 30103UL / 100000UL;
#else
  // Can use new C++0X max_digits10 (the maximum potentially significant digits).
  std::streamsize  max_digits10_float = std::numeric_limits<float>::max_digits10;
  std::streamsize  max_digits10_double = std::numeric_limits<double>::max_digits10;
#endif


/* A class with a float and a double */
struct foo
{
  foo () : fvalue (3.1415927F), dvalue (3.1415926535897931)
  {
  }
  // Set both the values to -infinity :
  void minus_infinity ()
  {
    fvalue = -std::numeric_limits<float>::infinity ();
    dvalue = -std::numeric_limits<double>::infinity ();
    return;
  }
  // Set the values to +infinity :
  void plus_infinity ()
  {
    fvalue = +std::numeric_limits<float>::infinity ();
    dvalue = +std::numeric_limits<double>::infinity ();
    return;
  }
  // Set the values to NaN :
  void nan ()
  {
    fvalue = +std::numeric_limits<float>::quiet_NaN ();
    dvalue = +std::numeric_limits<double>::quiet_NaN ();
    return;
  }
  // Print a foo:
  void print (std::ostream & a_out, const std::string & a_title)
  {
    if (a_title.empty ()) a_out << "foo";
    else a_out << a_title;
    a_out << " : " << std::endl;
    a_out << "|-- " << "fvalue = ";
    
    a_out.precision (max_digits10_float);
    a_out << fvalue << std::endl;
    a_out << "`-- " << "dvalue = ";
    a_out.precision (max_digits10_double);
    a_out << dvalue << std::endl;
    return;
  }

  // I/O operators for a foo structure of a float and a double :
  friend std::ostream & operator<< (std::ostream & a_out, const foo & a_foo);
  friend std::istream & operator>> (std::istream & a_in, foo & a_foo);

  // Attributes :
  float  fvalue; // Single precision floating number.
  double dvalue; // Double precision floating number.
};

std::ostream & operator<< (std::ostream & a_out, const foo & a_foo)
{ // Output bracketed FPs, for example "(3.1415927,3.1415926535897931)"
  a_out.precision (max_digits10_float);
  a_out << "(" << a_foo.fvalue << sep ;
  a_out.precision (max_digits10_double);
  a_out << a_foo.dvalue << ")";
  return a_out;
}

std::istream & operator>> (std::istream & a_in, foo & a_foo)
{ // Input bracketed floating-point values into a foo structure,
  // for example from "(3.1415927,3.1415926535897931)"
  char c = 0;
  a_in.get (c);
  if (c != '(')
  {
    ex_out << "ERROR: operator>> No ( " << std::endl;
    a_in.setstate(std::ios::failbit);
    return a_in;
  }
  float f;
  a_in >> std::ws >> f;
  if (! a_in)
  {
    return a_in;
  }
  a_in >> std::ws;
  a_in.get (c);
  if (c != sep)
  {
    ex_out << "ERROR: operator>> c='" << c << "'" << std::endl;
    ex_out << "ERROR: operator>> No '" << sep << "'" << std::endl;
    a_in.setstate(std::ios::failbit);
    return a_in;
  }
  double d;
  a_in >> std::ws >> d;
  if (! a_in)
  {
    return a_in;
  }
  a_in >> std::ws;
  a_in.get (c);
  if (c != ')')
  {
    ex_out << "ERROR: operator>> No ) " << std::endl;
    a_in.setstate(std::ios::failbit);
    return a_in;
  }
  a_foo.fvalue = f;
  a_foo.dvalue = d;
  return a_in;
} // std::istream & operator>> (std::istream & a_in, foo & a_foo)

int example_main()
{
  ex_out << "nonfinite_num_facet simple example." << std::endl;

   if((std::numeric_limits<double>::has_infinity == false) || (std::numeric_limits<double>::infinity() == 0))
  {
    ex_out << "Infinity not supported on this platform." << std::endl;
    return 0;
  }

  if((std::numeric_limits<double>::has_quiet_NaN == false) || (std::numeric_limits<double>::quiet_NaN() == 0))
  {
    ex_out << "NaN not supported on this platform." << std::endl;
    return 0;
  }

#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
  ex_out << "BOOST_NO_CXX11_NUMERIC_LIMITS is defined, so no max_digits10 available either:"
     "\n we'll have to calculate our own version." << endl;
#endif
  ex_out << "std::numeric_limits<float>::max_digits10 is " << max_digits10_float << endl;
  ex_out << "std::numeric_limits<double>::max_digits10 is " << max_digits10_double << endl;

   std::locale the_default_locale (std::locale::classic ());

  {
    ex_out << "Write to a string buffer (using default locale) :" << std::endl;
    foo f0; // pi
    foo f1; f1.minus_infinity ();
    foo f2; f2.plus_infinity ();
    foo f3; f3.nan ();

    f0.print (ex_out, "f0"); // pi
    f1.print (ex_out, "f1"); // +inf
    f2.print (ex_out, "f2"); // -inf
    f3.print (ex_out, "f3"); // NaN

    std::ostringstream oss;
    std::locale C99_out_locale (the_default_locale, new boost::math::nonfinite_num_put<char>);
    oss.imbue (C99_out_locale);
    oss.precision (15);
    oss << f0 << f1 << f2 << f3;
    ex_out << "Output in C99 format is: \"" << oss.str () << "\"" << std::endl;
    ex_out << "Output done." << std::endl;
  }

  {
    std::string the_string = "(3.1415927,3.1415926535897931)(-inf,-inf)(inf,inf)(nan,nan)"; // C99 format
    // Must have correct separator!
    ex_out << "Read C99 format from a string buffer containing \"" << the_string << "\""<< std::endl;

    std::locale C99_in_locale (the_default_locale, new boost::math::nonfinite_num_get<char>);
    std::istringstream iss (the_string);
    iss.imbue (C99_in_locale);

    foo f0, f1, f2, f3;
    iss >> f0 >> f1 >> f2 >> f3;
    if (! iss)
    {
       ex_out << "Input Format error !" << std::endl;
    }
    else
    {
      ex_out << "Input OK." << std::endl;
      ex_out << "Display in default locale format " << endl;
      f0.print (ex_out, "f0");
      f1.print (ex_out, "f1");
      f2.print (ex_out, "f2");
      f3.print (ex_out, "f3");
    }
    ex_out << "Input done." << std::endl;
  }
  
  ex_out << "End nonfinite_num_facet.cpp" << std::endl;
  return 0;
    return 0;  // main's implicit return
} // int example_main()

 // end of test_nonfinite_num_facets.cpp

/*

Output:

nonfinite_num_facet simple example.
  std::numeric_limits<float>::max_digits10 is 8
  std::numeric_limits<double>::max_digits10 is 17
  Write to a string buffer (using default locale) :
  f0 : 
  |-- fvalue = 3.1415927
  `-- dvalue = 3.1415926535897931
  f1 : 
  |-- fvalue = -1.#INF
  `-- dvalue = -1.#INF
  f2 : 
  |-- fvalue = 1.#INF
  `-- dvalue = 1.#INF
  f3 : 
  |-- fvalue = 1.#QNAN
  `-- dvalue = 1.#QNAN
  Output in C99 format is: "(3.1415927,3.1415926535897931)(-inf,-inf)(inf,inf)(nan,nan)"
  Output done.
  Read C99 format from a string buffer containing "(3.1415927,3.1415926535897931)(-inf,-inf)(inf,inf)(nan,nan)"
  Display in default locale format 
  f0 : 
  |-- fvalue = 3.1415927
  `-- dvalue = 3.1415926535897931
  f1 : 
  |-- fvalue = -1.#INF
  `-- dvalue = -1.#INF
  f2 : 
  |-- fvalue = 1.#INF
  `-- dvalue = 1.#INF
  f3 : 
  |-- fvalue = 1.#QNAN
  `-- dvalue = 1.#QNAN
  Input done.
  End nonfinite_num_facet.cpp

*/

TEST_CASE("example: nonfinite_num_facet") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(nonfinite_num_facet simple example.
std::numeric_limits<float>::max_digits10 is 9
std::numeric_limits<double>::max_digits10 is 17
Write to a string buffer (using default locale) :
f0 : 
|-- fvalue = 3.14159274
`-- dvalue = 3.1415926535897931
f1 : 
|-- fvalue = -inf
`-- dvalue = -inf
f2 : 
|-- fvalue = inf
`-- dvalue = inf
f3 : 
|-- fvalue = nan
`-- dvalue = nan
Output in C99 format is: "(3.14159274,3.1415926535897931)(-inf,-inf)(inf,inf)(nan,nan)"
Output done.
Read C99 format from a string buffer containing "(3.1415927,3.1415926535897931)(-inf,-inf)(inf,inf)(nan,nan)"
Input OK.
Display in default locale format 
f0 : 
|-- fvalue = 3.14159274
`-- dvalue = 3.1415926535897931
f1 : 
|-- fvalue = -inf
`-- dvalue = -inf
f2 : 
|-- fvalue = inf
`-- dvalue = inf
f3 : 
|-- fvalue = nan
`-- dvalue = nan
Input done.
End nonfinite_num_facet.cpp
)ex"));
#endif
}
}  // namespace ex_nonfinite_num_facet

// libs/math/example/nonfinite_signaling_NaN.cpp
namespace ex_nonfinite_signaling_NaN {
#ifdef _MSC_VER
#   pragma warning(disable : 4702)
#endif

using boost::math::nonfinite_num_get;
using boost::math::nonfinite_num_put;


using std::endl;

using std::locale;

using std::string;

  using std::stringstream;
  using std::istringstream;

using std::numeric_limits;

int example_main()
{
    if((std::numeric_limits<double>::has_infinity == false) || (std::numeric_limits<double>::infinity() == 0))
  {
    ex_out << "Infinity not supported on this platform." << std::endl;
    return 0;
  }

  if((std::numeric_limits<double>::has_quiet_NaN == false) || (std::numeric_limits<double>::quiet_NaN() == 0))
  {
    ex_out << "NaN not supported on this platform." << std::endl;
    return 0;
  }

  locale default_locale; // Current global locale.
  // Try to use the default locale first.
  // On MSVC this doesn't work.

  { // Try Quiet NaN
    stringstream ss; // Both input and output.
    ss.imbue(default_locale); // Redundant, of course.
    string infs;
    if(numeric_limits<double>::has_quiet_NaN)
    {  // Make sure quiet NaN is specialised for type double.
      double qnan = numeric_limits<double>::quiet_NaN();
      ss << qnan; // Output quiet_NaN.
      infs = ss.str();  //
    }
    else
    { // Need to provide a suitable string for quiet NaN.
     infs =  "1.#QNAN";
      ss << infs;
    }
    double r;
    ss >> r; // Read back in.

    ex_out << "quiet_NaN output was " << infs << endl; // "1.#QNAN"
    ex_out << "quiet_NaN input was " << r << endl; // "1"
  }

#if (!defined BOOST_BORLANDC && !defined BOOST_CODEGEARC)
  // These compilers trap when trying to create a signaling_NaN!
  { // Try Signaling NaN
    stringstream ss; // Both input and output.
    ss.imbue(default_locale); // Redundant, of course.
    string infs;
    if(numeric_limits<double>::has_signaling_NaN)
    {  // Make sure signaling NaN is specialised for type double.
      double qnan = numeric_limits<double>::signaling_NaN();
      ss << qnan; // Output signaling_NaN.
      infs = ss.str();  //
    }
    else
    { // Need to provide a suitable string for signaling NaN.
     infs =  "1.#SNAN";
      ss << infs;
    }
    double r;
    ss >> r; // Read back in.

    ex_out << "signaling_NaN output was " << infs << endl; // "1.#QNAN" (or "1.#SNAN"?)
    ex_out << "signaling_NaN input was " << r << endl; // "1"
  }
#endif // Not Borland or CodeGear.

  // Create legacy_locale and store the nonfinite_num_get facet (with legacy flag) in it.
  locale legacy_locale(default_locale, new nonfinite_num_get<char>(boost::math::legacy));
  // Note that the legacy flag has no effect on the nonfinite_num_put output facet.

  ex_out << "Use legacy locale." << endl;

  { // Try infinity.
    stringstream ss; // Both input and output.
    ss.imbue(legacy_locale);
    string infs;
    if(numeric_limits<double>::has_infinity)
    {  // Make sure infinity is specialised for type double.
      double inf = numeric_limits<double>::infinity();
      ss << inf; // Output infinity.
      infs = ss.str();  //
    }
    else
    { // Need to provide a suitable string for infinity.
     infs =  "1.#INF";
      ss << infs;
    }
    double r;
    ss >> r; // Read back in.

    ex_out << "infinity output was " << infs << endl; // "1.#INF"
    ex_out << "infinity input was " << r << endl; // "1.#INF"
  }

  { // Try input of "1.#SNAN".
    //double inf = numeric_limits<double>::signaling_NaN(); // Assigns "1.#QNAN" on MSVC.
    // So must use explicit string "1.#SNAN" instead.
    stringstream ss; // Both input and output.
    ss.imbue(legacy_locale);
    string s = "1.#SNAN";

    ss << s; // Write out.
    double r;

    ss >> r; // Read back in.

    ex_out << "SNAN output was " << s << endl; // "1.#SNAN"
    ex_out << "SNAN input was " << r << endl;  // "1.#QNAN"
  }

  { // Try input of "1.#IND" .
    stringstream ss; // Both input and output.
    ss.imbue(legacy_locale);
    string s = "1.#IND";
    ss << s; // Write out.
    double r;
    ss >> r; // Read back in.

    ex_out << "IND output was " << s << endl; // "1.#IND"
    ex_out << "IND input was " << r << endl;  // "1.#QNAN"
  }

    return 0;  // main's implicit return
} // int example_main()

/*

Output:
  nonfinite_signaling_NaN.vcxproj -> J:\Cpp\fp_facet\fp_facet\Debug\nonfinite_signaling_NaN.exe

  quiet_NaN output was 1.#QNAN
  quiet_NaN input was 1
  signaling_NaN output was 1.#QNAN
  signaling_NaN input was 1
  Use legacy locale.
  infinity output was 1.#INF
  infinity input was 1.#INF
  SNAN output was 1.#SNAN
  SNAN input was 1.#QNAN
  IND output was 1.#IND
  IND input was 1.#QNAN


*/

TEST_CASE("example: nonfinite_signaling_NaN") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(quiet_NaN output was nan
quiet_NaN input was 0
signaling_NaN output was nan
signaling_NaN input was 0
Use legacy locale.
infinity output was inf
infinity input was inf
SNAN output was 1.#SNAN
SNAN input was nan
IND output was 1.#IND
IND input was nan
)ex"));
#endif
}
}  // namespace ex_nonfinite_signaling_NaN

// libs/math/example/ooura_fourier_integrals_cosine_example.cpp
namespace ex_ooura_fourier_integrals_cosine_example {
int example_main()
{
  try
  {
  ex_out.precision(std::numeric_limits<double>::max_digits10); // Show all potentially significant digits.

  using boost::math::quadrature::ooura_fourier_cos;
  using boost::math::constants::half_pi;
  using boost::math::constants::e;

 //[ooura_fourier_integrals_cosine_example_1
  auto integrator = ooura_fourier_cos<double>();
  // Use the default tolerance root_epsilon and eight levels for type double.

  auto f = [](double x)
  { // More complex example function.
    return 1 / (x * x + 1);
  };

  double omega = 1;

  auto [result, relative_error] = integrator.integrate(f, omega);
  ex_out << "Integral = " << result << ", relative error estimate " << relative_error << std::endl;

  //] [/ooura_fourier_integrals_cosine_example_1]

  //[ooura_fourier_integrals_cosine_example_2

  constexpr double expected = half_pi<double>() / e<double>();
  ex_out << "pi/(2e) =  " << expected << ", difference " << result - expected << std::endl;
  //] [/ooura_fourier_integrals_cosine_example_2]
  }
  catch (std::exception const & ex)
  {
    // Lacking try&catch blocks, the program will abort after any throw, whereas the
    // message below from the thrown exception will give some helpful clues as to the cause of the problem.
    ex_out << "\n""Message from thrown exception was:\n   " << ex.what() << std::endl;
  }

    return 0;  // main's implicit return
} // int example_main()

/*

//[ooura_fourier_integrals_example_cosine_output_1
``
Integral = 0.57786367489546109, relative error estimate 6.4177395404415149e-09
pi/(2e) =  0.57786367489546087, difference 2.2204460492503131e-16
``
//] [/ooura_fourier_integrals_example_cosine_output_1]


//[ooura_fourier_integrals_example_cosine_diagnostic_output_1
``
ooura_fourier_cos with relative error goal 1.4901161193847656e-08 & 8 levels.
epsilon for type = 2.2204460492503131e-16
h = 1.000000000000000, I_h = 0.588268622591776 = 0x1.2d318b7e96dbe00p-1, absolute error estimate = nan
h = 0.500000000000000, I_h = 0.577871642184837 = 0x1.27decab8f07b200p-1, absolute error estimate = 1.039698040693926e-02
h = 0.250000000000000, I_h = 0.577863671186883 = 0x1.27ddbf42969be00p-1, absolute error estimate = 7.970997954576120e-06
h = 0.125000000000000, I_h = 0.577863674895461 = 0x1.27ddbf6271dc000p-1, absolute error estimate = 3.708578555361441e-09
Integral = 5.778636748954611e-01, relative error estimate 6.417739540441515e-09
pi/(2e)  = 5.778636748954609e-01, difference 2.220446049250313e-16
``
//] [/ooura_fourier_integrals_example_cosine_diagnostic_output_1]

*/

TEST_CASE("example: ooura_fourier_integrals_cosine_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Integral = 0.57786367489546098, relative error estimate 6.417739348316094e-09
pi/(2e) =  0.57786367489546087, difference 1.1102230246251565e-16
)ex"));
#endif
}
}  // namespace ex_ooura_fourier_integrals_cosine_example

// libs/math/example/policy_eg_1.cpp
namespace ex_policy_eg_1 {
 using std::endl;

//[policy_eg_1

using boost::math::tgamma;

// Define the policy to use:
using namespace boost::math::policies; // may be convenient, or

using boost::math::policies::policy;
// Types of error whose action can be altered by policies:.
using boost::math::policies::evaluation_error;
using boost::math::policies::domain_error;
using boost::math::policies::overflow_error;
using boost::math::policies::underflow_error;
using boost::math::policies::pole_error;
// Actions on error (in enum error_policy_type):
using boost::math::policies::errno_on_error;
using boost::math::policies::ignore_error;
using boost::math::policies::throw_on_error;
using boost::math::policies::user_error;

typedef policy<
   domain_error<errno_on_error>,
   pole_error<errno_on_error>,
   overflow_error<errno_on_error>,
   evaluation_error<errno_on_error> 
> c_policy;
//
// Now use the policy when calling tgamma:

// http://msdn.microsoft.com/en-us/library/t3ayayh1.aspx 
// Microsoft errno declared in STDLIB.H as "extern int errno;" 

int example_main()
{
   errno = 0; // Reset.
   ex_out << "Result of tgamma(30000) is: " 
      << tgamma(30000, c_policy()) << endl; // Too big parameter
   ex_out << "errno = " << errno << endl; // errno 34 Numerical result out of range.
   ex_out << "Result of tgamma(-10) is: " 
      << boost::math::tgamma(-10, c_policy()) << endl; // Negative parameter.
   ex_out << "errno = " << errno << endl; // error 33 Numerical argument out of domain.
    return 0;  // main's implicit return
} // int example_main()

//]

/* Output

policy_eg_1.cpp
  Generating code
  Finished generating code
  policy_eg_1.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\policy_eg_1.exe
  Result of tgamma(30000) is: 1.#INF
  errno = 34
  Result of tgamma(-10) is: 1.#QNAN
  errno = 33

*/

TEST_CASE("example: policy_eg_1") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Result of tgamma(30000) is: inf
errno = 34
Result of tgamma(-10) is: nan
errno = 33
)ex"));
#endif
}
}  // namespace ex_policy_eg_1

// libs/math/example/policy_eg_4.cpp
namespace ex_policy_eg_4 {
  using std::endl;

//[policy_eg_4

/*`
Suppose we want `C::foo()` to behave in a C-compatible way and set
`::errno` on error rather than throwing any exceptions.

We'll begin by including the needed header for our function:
*/

//using boost::math::tgamma; // Not needed because using C::tgamma.

/*`
Open up the "C" namespace that we'll use for our functions, and
define the policy type we want: in this case a C-style one that sets
::errno and returns a standard value, rather than throwing exceptions.

Any policies we don't specify here will inherit the defaults.
*/

namespace C
{ // To hold our C-style policy.
  //using namespace boost::math::policies; or explicitly:
  using boost::math::policies::policy;

  using boost::math::policies::domain_error;
  using boost::math::policies::pole_error;
  using boost::math::policies::overflow_error;
  using boost::math::policies::evaluation_error;
  using boost::math::policies::errno_on_error;

  typedef policy<
     domain_error<errno_on_error>,
     pole_error<errno_on_error>,
     overflow_error<errno_on_error>,
     evaluation_error<errno_on_error>
  > c_policy;

/*`
All we need do now is invoke the BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS
macro passing our policy type c_policy as the single argument:
*/

BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS(c_policy)

} // close namespace C

/*`
We now have a set of forwarding functions defined in namespace C
that all look something like this:

``
template <class RealType>
inline typename boost::math::tools::promote_args<RT>::type
   tgamma(RT z)
{
   return boost::math::tgamma(z, c_policy());
}
``

So that when we call `C::tgamma(z)`, we really end up calling
`boost::math::tgamma(z, C::c_policy())`:
*/

int example_main()
{
   errno = 0;
   ex_out << "Result of tgamma(30000) is: "
      << C::tgamma(30000) << endl; // Note using C::tgamma
   ex_out << "errno = " << errno << endl; // errno = 34
   ex_out << "Result of tgamma(-10) is: "
      << C::tgamma(-10) << endl;
   ex_out << "errno = " << errno << endl; // errno = 33, overwriting previous value of 34.
    return 0;  // main's implicit return
}

/*`

Which outputs:

[pre
Result of C::tgamma(30000) is: 1.#INF
errno = 34
Result of C::tgamma(-10) is: 1.#QNAN
errno = 33
]

This mechanism is particularly useful when we want to define a project-wide policy,
and don't want to modify the Boost source,
or to set project wide build macros (possibly fragile and easy to forget).

*/
//] //[/policy_eg_4]

TEST_CASE("example: policy_eg_4") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Result of tgamma(30000) is: inf
errno = 34
Result of tgamma(-10) is: nan
errno = 33
)ex"));
#endif
}
}  // namespace ex_policy_eg_4

// libs/math/example/policy_ref_snip11.cpp
namespace ex_policy_ref_snip11 {
 using std::endl;

// Setting (approximate) precision 25 bits in a single function call using make_policy.

//[policy_ref_snip11

using boost::math::normal_distribution;

using namespace boost::math::policies;

const int bits = 25; // approximate precision.

double q = quantile(
      normal_distribution<double, policy<digits2<bits> > >(), 
      0.05); // 5% quantile.

//] //[/policy_ref_snip11]

int example_main()
{
  std::streamsize p = 2 + (bits * 30103UL) / 100000UL; 
  // Approximate number of significant decimal digits for 25 bits.
  ex_out.precision(p); 
  ex_out << bits << " binary bits is approximately equivalent to " << p << " decimal digits " << endl;
  ex_out << "quantile(normal_distribution<double, policy<digits2<25> > >(), 0.05  = "
     << q << endl; // -1.64485
    return 0;  // main's implicit return
}

/*
Output:
  25 binary bits is approximately equivalent to 9 decimal digits 
  quantile(normal_distribution<double, policy<digits2<25> > >(), 0.05  = -1.64485363
  */

TEST_CASE("example: policy_ref_snip11") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(25 binary bits is approximately equivalent to 9 decimal digits 
quantile(normal_distribution<double, policy<digits2<25> > >(), 0.05  = -1.64485363
)ex"));
#endif
}
}  // namespace ex_policy_ref_snip11

// libs/math/example/policy_ref_snip3.cpp
namespace ex_policy_ref_snip3 {
double some_value = 2.;

//[policy_ref_snip3


using namespace boost::math::policies;
using boost::math::tgamma;

// Define a new policy *not* internally promoting RealType to double:
typedef policy<
      promote_double<false> 
      > my_policy;
      
// Call the function, applying the new policy:
double t1 = tgamma(some_value, my_policy());

// Alternatively we could use helper function make_policy,
// and concisely define everything at the call site:
double t2 = tgamma(some_value, make_policy(promote_double<false>()));

//] //[\policy_ref_snip3]

  using std::endl;

int example_main()
{
   ex_out << "tgamma(some_value, my_policy()) = " << t1 
     << ", tgamma(some_value, make_policy(promote_double<false>()) = " << t2 << endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: policy_ref_snip3") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(tgamma(some_value, my_policy()) = 1, tgamma(some_value, make_policy(promote_double<false>()) = 1
)ex"));
#endif
}
}  // namespace ex_policy_ref_snip3

// libs/math/example/policy_ref_snip7.cpp
namespace ex_policy_ref_snip7 {
using boost::math::negative_binomial_distribution;

using namespace boost::math::policies;

typedef negative_binomial_distribution<
      double, 
      policy<discrete_quantile<integer_round_inwards> > 
   > dist_type;
   
// Lower quantile rounded up:
double x = quantile(dist_type(20, 0.3), 0.05); // 28 rounded up from 27.3898
// Upper quantile rounded down:
double y = quantile(complement(dist_type(20, 0.3), 0.05)); // 68 rounded down from 68.1584

//] //[/policy_ref_snip7]

 using std::endl;

int example_main()
{
   ex_out << "using policy<discrete_quantile<integer_round_inwards> > " << endl
   << "quantile(dist_type(20, 0.3), 0.05) = " << x << endl 
   << "quantile(complement(dist_type(20, 0.3), 0.05)) =  " << y << endl;
    return 0;  // main's implicit return
}

/*

Output:
  using policy<discrete_quantile<integer_round_inwards> > 
  quantile(dist_type(20, 0.3), 0.05) = 28
  quantile(complement(dist_type(20, 0.3), 0.05)) =  68


*/

TEST_CASE("example: policy_ref_snip7") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(using policy<discrete_quantile<integer_round_inwards> > 
quantile(dist_type(20, 0.3), 0.05) = 28
quantile(complement(dist_type(20, 0.3), 0.05)) =  68
)ex"));
#endif
}
}  // namespace ex_policy_ref_snip7

// libs/math/example/random_search_example.cpp
namespace ex_random_search_example {
#if __APPLE__ || __linux__

using boost::math::optimization::random_search_parameters;
using boost::math::optimization::random_search;
using namespace std::chrono_literals;

template <class Real> Real rastrigin(std::vector<Real> const &v) {
  using std::cos;
  using boost::math::constants::two_pi;
  Real A = 10;
  Real y = 10 * v.size();
  for (auto x : v) {
    y += x * x - A * cos(two_pi<Real>() * x);
  }
  return y;
}

std::atomic<bool> cancel = false;

void ctrl_c_handler(int){
    cancel = true;
    ex_out << "Cancellation requested-this could take a second . . ." << std::endl;
}

int example_main() {
  ex_out << "Running random search on Rastrigin function (global minimum = (0,0,...,0))\n";
  signal(SIGINT, ctrl_c_handler);
  using ArgType = std::vector<double>;
  auto params = random_search_parameters<ArgType>();
  params.lower_bounds.resize(3, -5.12);
  params.upper_bounds.resize(3, 5.12);
  params.max_function_calls = 100000000;
  // Leave one thread available to respond to ctrl-C:
  params.threads = std::thread::hardware_concurrency() - 1;
  std::random_device rd;
  std::mt19937_64 gen(rd());

  // By definition, the value of the function which a target value is provided must be <= target_value.
  double target_value = 1e-3;
  std::atomic<double> current_minimum_cost;
  ex_out << "Hit ctrl-C to gracefully terminate the optimization." << std::endl;
  auto f = [&]() {
    return random_search(rastrigin<double>, params, gen, target_value, &cancel, &current_minimum_cost);
  };
  auto future = std::async(std::launch::async, f);
  std::future_status status = future.wait_for(3ms);
  while (!cancel && (status != std::future_status::ready)) {
    status = future.wait_for(1ms);
    ex_out << "Current cost is " << current_minimum_cost << "\r";
  }

  auto local_minima = future.get();
  ex_out << "Local minimum is {";
  for (size_t i = 0; i < local_minima.size() - 1; ++i) {
    ex_out << local_minima[i] << ", ";
  }
  ex_out << local_minima.back() << "}.\n";
  ex_out << "Final cost: " << current_minimum_cost << "\n";
    return 0;  // main's implicit return
}
#else
#warning "Signal handling for the random search example only works on Linux and Mac."
int example_main() {
    return 0;
    return 0;  // main's implicit return
}
#endif

TEST_CASE("example: random_search_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("Running random search on Rastrigin function (global minimum = (0,0,...,0))\n"
        "Hit ctrl-C to gracefully terminate the optimization.\n"
        "Current cost is 1.77814\r"
        "Current cost is 1.77814\r"
        "Current cost is 1.77814\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 1.09981\r"
        "Current cost is 0.573937\r"
        "Current cost is 0.573937\r"
        "Current cost is 0.573937\r"
        "Current cost is 0.573937\r"
        "Current cost is 0.573937\r"
        "Current cost is 0.573937\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.125633\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.102823\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.0511466\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Current cost is 0.00826807\r"
        "Local minimum is {-0.00552378, 0.00141513, -0.00302722}.\n"
        "Final cost: 0.00826807\n", 130539)));
#endif
}
}  // namespace ex_random_search_example

// libs/math/example/root_finding_start_locations.cpp
namespace ex_root_finding_start_locations {
template <class T>
struct cbrt_functor_noderiv
{
   //  cube root of x using only function - no derivatives.
   cbrt_functor_noderiv(T const& to_find_root_of) : a(to_find_root_of)
   { /* Constructor just stores value a to find root of. */
   }
   T operator()(T const& x)
   {
      T fx = x*x*x - a; // Difference (estimate x^3 - a).
      return fx;
   }
private:
   T a; // to be 'cube_rooted'.
};
//] [/root_finding_noderiv_1

template <class T>
std::uintmax_t cbrt_noderiv(T x, T guess)
{
   // return cube root of x using bracket_and_solve (no derivatives).
   using namespace std;                          // Help ADL of std functions.
   using namespace boost::math::tools;           // For bracket_and_solve_root.

   T factor = 2;                                 // How big steps to take when searching.

   const std::uintmax_t maxit = 20;            // Limit to maximum iterations.
   std::uintmax_t it = maxit;                  // Initially our chosen max iterations, but updated with actual.
   bool is_rising = true;                        // So if result if guess^3 is too low, then try increasing guess.
   int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
   // Some fraction of digits is used to control how accurate to try to make the result.
   int get_digits = digits - 3;                  // We have to have a non-zero interval at each step, so
   // maximum accuracy is digits - 1.  But we also have to
   // allow for inaccuracy in f(x), otherwise the last few
   // iterations just thrash around.
   eps_tolerance<T> tol(get_digits);             // Set the tolerance.
   bracket_and_solve_root(cbrt_functor_noderiv<T>(x), guess, factor, is_rising, tol, it);
   return it;
}

template <class T>
struct cbrt_functor_deriv
{ // Functor also returning 1st derivative.
   cbrt_functor_deriv(T const& to_find_root_of) : a(to_find_root_of)
   { // Constructor stores value a to find root of,
      // for example: calling cbrt_functor_deriv<T>(a) to use to get cube root of a.
   }
   std::pair<T, T> operator()(T const& x)
   {
      // Return both f(x) and f'(x).
      T fx = x*x*x - a;                // Difference (estimate x^3 - value).
      T dx = 3 * x*x;                 // 1st derivative = 3x^2.
      return std::make_pair(fx, dx);   // 'return' both fx and dx.
   }
private:
   T a;                               // Store value to be 'cube_rooted'.
};

template <class T>
std::uintmax_t cbrt_deriv(T x, T guess)
{
   // return cube root of x using 1st derivative and Newton_Raphson.
   using namespace boost::math::tools;
   T min = guess / 100;                     // We don't really know what this should be!
   T max = guess * 100;                     // We don't really know what this should be!
   const int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
   int get_digits = static_cast<int>(digits * 0.6);    // Accuracy doubles with each step, so stop when we have
   // just over half the digits correct.
   const std::uintmax_t maxit = 20;
   std::uintmax_t it = maxit;
   newton_raphson_iterate(cbrt_functor_deriv<T>(x), guess, min, max, get_digits, it);
   return it;
}

template <class T>
struct cbrt_functor_2deriv
{
   // Functor returning both 1st and 2nd derivatives.
   cbrt_functor_2deriv(T const& to_find_root_of) : a(to_find_root_of)
   { // Constructor stores value a to find root of, for example:
      // calling cbrt_functor_2deriv<T>(x) to get cube root of x,
   }
   std::tuple<T, T, T> operator()(T const& x)
   {
      // Return both f(x) and f'(x) and f''(x).
      T fx = x*x*x - a;                     // Difference (estimate x^3 - value).
      T dx = 3 * x*x;                       // 1st derivative = 3x^2.
      T d2x = 6 * x;                        // 2nd derivative = 6x.
      return std::make_tuple(fx, dx, d2x);  // 'return' fx, dx and d2x.
   }
private:
   T a; // to be 'cube_rooted'.
};

template <class T>
std::uintmax_t cbrt_2deriv(T x, T guess)
{ 
   // return cube root of x using 1st and 2nd derivatives and Halley.
   //using namespace std;  // Help ADL of std functions.
   using namespace boost::math::tools;
   T min = guess / 100;                     // We don't really know what this should be!
   T max = guess * 100;                     // We don't really know what this should be!
   const int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
   // digits used to control how accurate to try to make the result.
   int get_digits = static_cast<int>(digits * 0.4);    // Accuracy triples with each step, so stop when just
   // over one third of the digits are correct.
   std::uintmax_t maxit = 20;
   halley_iterate(cbrt_functor_2deriv<T>(x), guess, min, max, get_digits, maxit);
   return maxit;
}

template <class T>
std::uintmax_t cbrt_2deriv_s(T x, T guess)
{ 
   // return cube root of x using 1st and 2nd derivatives and Halley.
   //using namespace std;  // Help ADL of std functions.
   using namespace boost::math::tools;
   T min = guess / 100;                     // We don't really know what this should be!
   T max = guess * 100;                     // We don't really know what this should be!
   const int digits = std::numeric_limits<T>::digits;  // Maximum possible binary digits accuracy for type T.
   // digits used to control how accurate to try to make the result.
   int get_digits = static_cast<int>(digits * 0.4);    // Accuracy triples with each step, so stop when just
   // over one third of the digits are correct.
   std::uintmax_t maxit = 20;
   schroder_iterate(cbrt_functor_2deriv<T>(x), guess, min, max, get_digits, maxit);
   return maxit;
}

template <typename T = double>
struct elliptic_root_functor_noderiv
{ 
   elliptic_root_functor_noderiv(T const& arc, T const& radius) : m_arc(arc), m_radius(radius)
   { // Constructor just stores value a to find root of.
   }
   T operator()(T const& x)
   {
      // return the difference between required arc-length, and the calculated arc-length for an
      // ellipse with radii m_radius and x:
      T a = (std::max)(m_radius, x);
      T b = (std::min)(m_radius, x);
      T k = sqrt(1 - b * b / (a * a));
      return 4 * a * boost::math::ellint_2(k) - m_arc;
   }
private:
   T m_arc;     // length of arc.
   T m_radius;  // one of the two radii of the ellipse
}; // template <class T> struct elliptic_root_functor_noderiv

template <class T = double>
std::uintmax_t elliptic_root_noderiv(T radius, T arc, T guess)
{ // return the other radius of an ellipse, given one radii and the arc-length
   using namespace std;  // Help ADL of std functions.
   using namespace boost::math::tools; // For bracket_and_solve_root.

   T factor = 2;                       // How big steps to take when searching.

   const std::uintmax_t maxit = 50;  // Limit to maximum iterations.
   std::uintmax_t it = maxit;        // Initially our chosen max iterations, but updated with actual.
   bool is_rising = true;              // arc-length increases if one radii increases, so function is rising
   // Define a termination condition, stop when nearly all digits are correct, but allow for
   // the fact that we are returning a range, and must have some inaccuracy in the elliptic integral:
   eps_tolerance<T> tol(std::numeric_limits<T>::digits - 2);
   // Call bracket_and_solve_root to find the solution, note that this is a rising function:
   bracket_and_solve_root(elliptic_root_functor_noderiv<T>(arc, radius), guess, factor, is_rising, tol, it);
   return it;
} 

template <class T = double>
struct elliptic_root_functor_1deriv
{ // Functor also returning 1st derivative.
   static_assert(boost::is_integral<T>::value == false, "Only floating-point type types can be used!");

   elliptic_root_functor_1deriv(T const& arc, T const& radius) : m_arc(arc), m_radius(radius)
   { // Constructor just stores value a to find root of.
   }
   std::pair<T, T> operator()(T const& x)
   {
      // Return the difference between required arc-length, and the calculated arc-length for an
      // ellipse with radii m_radius and x, plus it's derivative.
      // See http://www.wolframalpha.com/input/?i=d%2Fda+[4+*+a+*+EllipticE%281+-+b^2%2Fa^2%29]
      // We require two elliptic integral calls, but from these we can calculate both
      // the function and it's derivative:
      T a = (std::max)(m_radius, x);
      T b = (std::min)(m_radius, x);
      T a2 = a * a;
      T b2 = b * b;
      T k = sqrt(1 - b2 / a2);
      T Ek = boost::math::ellint_2(k);
      T Kk = boost::math::ellint_1(k);
      T fx = 4 * a * Ek - m_arc;
      T dfx = 4 * (a2 * Ek - b2 * Kk) / (a2 - b2);
      return std::make_pair(fx, dfx);
   }
private:
   T m_arc;     // length of arc.
   T m_radius;  // one of the two radii of the ellipse
};  // struct elliptic_root__functor_1deriv

template <class T = double>
std::uintmax_t elliptic_root_1deriv(T radius, T arc, T guess)
{
   using namespace std;  // Help ADL of std functions.
   using namespace boost::math::tools; // For newton_raphson_iterate.

   static_assert(boost::is_integral<T>::value == false, "Only floating-point type types can be used!");

   T min = 0;   // Minimum possible value is zero.
   T max = arc; // Maximum possible value is the arc length.

   // Accuracy doubles at each step, so stop when just over half of the digits are
   // correct, and rely on that step to polish off the remainder:
   int get_digits = static_cast<int>(std::numeric_limits<T>::digits * 0.6);
   const std::uintmax_t maxit = 20;
   std::uintmax_t it = maxit;
   newton_raphson_iterate(elliptic_root_functor_1deriv<T>(arc, radius), guess, min, max, get_digits, it);
   return it;
}

template <class T = double>
struct elliptic_root_functor_2deriv
{ // Functor returning both 1st and 2nd derivatives.
   static_assert(boost::is_integral<T>::value == false, "Only floating-point type types can be used!");

   elliptic_root_functor_2deriv(T const& arc, T const& radius) : m_arc(arc), m_radius(radius) {}
   std::tuple<T, T, T> operator()(T const& x)
   {
      // Return the difference between required arc-length, and the calculated arc-length for an
      // ellipse with radii m_radius and x, plus it's derivative.
      // See http://www.wolframalpha.com/input/?i=d^2%2Fda^2+[4+*+a+*+EllipticE%281+-+b^2%2Fa^2%29]
      // for the second derivative.
      T a = (std::max)(m_radius, x);
      T b = (std::min)(m_radius, x);
      T a2 = a * a;
      T b2 = b * b;
      T k = sqrt(1 - b2 / a2);
      T Ek = boost::math::ellint_2(k);
      T Kk = boost::math::ellint_1(k);
      T fx = 4 * a * Ek - m_arc;
      T dfx = 4 * (a2 * Ek - b2 * Kk) / (a2 - b2);
      T dfx2 = 4 * b2 * ((a2 + b2) * Kk - 2 * a2 * Ek) / (a * (a2 - b2) * (a2 - b2));
      return std::make_tuple(fx, dfx, dfx2);
   }
private:
   T m_arc;     // length of arc.
   T m_radius;  // one of the two radii of the ellipse
};

template <class T = double>
std::uintmax_t elliptic_root_2deriv(T radius, T arc, T guess)
{
   using namespace std;                // Help ADL of std functions.
   using namespace boost::math::tools; // For halley_iterate.

   static_assert(boost::is_integral<T>::value == false, "Only floating-point type types can be used!");

   T min = 0;                                   // Minimum possible value is zero.
   T max = arc;                                 // radius can't be larger than the arc length.

   // Accuracy triples at each step, so stop when just over one-third of the digits
   // are correct, and the last iteration will polish off the remaining digits:
   int get_digits = static_cast<int>(std::numeric_limits<T>::digits * 0.4);
   const std::uintmax_t maxit = 20;
   std::uintmax_t it = maxit;
   halley_iterate(elliptic_root_functor_2deriv<T>(arc, radius), guess, min, max, get_digits, it);
   return it;
} // nth_2deriv Halley
//]
// Using 1st and 2nd derivatives using Schroder algorithm.

template <class T = double>
std::uintmax_t elliptic_root_2deriv_s(T radius, T arc, T guess)
{ // return nth root of x using 1st and 2nd derivatives and Schroder.

   using namespace std;  // Help ADL of std functions.
   using namespace boost::math::tools; // For schroder_iterate.

   static_assert(boost::is_integral<T>::value == false, "Only floating-point type types can be used!");

   T min = 0; // Minimum possible value is zero.
   T max = arc; // radius can't be larger than the arc length.

   int digits = std::numeric_limits<T>::digits; // Maximum possible binary digits accuracy for type T.
   int get_digits = static_cast<int>(digits * 0.4);
   const std::uintmax_t maxit = 20;
   std::uintmax_t it = maxit;
   schroder_iterate(elliptic_root_functor_2deriv<T>(arc, radius), guess, min, max, get_digits, it);
   return it;
} // T elliptic_root_2deriv_s Schroder


int example_main()
{
   try
   {
      double to_root = 500;
      double answer = 7.93700525984;

      ex_out << "[table\n"
         << "[[Initial Guess=][-500% ([approx]1.323)][-100% ([approx]3.97)][-50% ([approx]3.96)][-20% ([approx]6.35)][-10% ([approx]7.14)][-5% ([approx]7.54)]"
         "[5% ([approx]8.33)][10% ([approx]8.73)][20% ([approx]9.52)][50% ([approx]11.91)][100% ([approx]15.87)][500 ([approx]47.6)]]\n";
      ex_out << "[[bracket_and_solve_root]["
         << cbrt_noderiv(to_root, answer / 6)
         << "][" << cbrt_noderiv(to_root, answer / 2)
         << "][" << cbrt_noderiv(to_root, answer - answer * 0.5)
         << "][" << cbrt_noderiv(to_root, answer - answer * 0.2)
         << "][" << cbrt_noderiv(to_root, answer - answer * 0.1)
         << "][" << cbrt_noderiv(to_root, answer - answer * 0.05)
         << "][" << cbrt_noderiv(to_root, answer + answer * 0.05)
         << "][" << cbrt_noderiv(to_root, answer + answer * 0.1)
         << "][" << cbrt_noderiv(to_root, answer + answer * 0.2)
         << "][" << cbrt_noderiv(to_root, answer + answer * 0.5)
         << "][" << cbrt_noderiv(to_root, answer + answer)
         << "][" << cbrt_noderiv(to_root, answer + answer * 5) << "]]\n";

      ex_out << "[[newton_iterate]["
         << cbrt_deriv(to_root, answer / 6)
         << "][" << cbrt_deriv(to_root, answer / 2)
         << "][" << cbrt_deriv(to_root, answer - answer * 0.5)
         << "][" << cbrt_deriv(to_root, answer - answer * 0.2)
         << "][" << cbrt_deriv(to_root, answer - answer * 0.1)
         << "][" << cbrt_deriv(to_root, answer - answer * 0.05)
         << "][" << cbrt_deriv(to_root, answer + answer * 0.05)
         << "][" << cbrt_deriv(to_root, answer + answer * 0.1)
         << "][" << cbrt_deriv(to_root, answer + answer * 0.2)
         << "][" << cbrt_deriv(to_root, answer + answer * 0.5)
         << "][" << cbrt_deriv(to_root, answer + answer)
         << "][" << cbrt_deriv(to_root, answer + answer * 5) << "]]\n";

      ex_out << "[[halley_iterate]["
         << cbrt_2deriv(to_root, answer / 6)
         << "][" << cbrt_2deriv(to_root, answer / 2)
         << "][" << cbrt_2deriv(to_root, answer - answer * 0.5)
         << "][" << cbrt_2deriv(to_root, answer - answer * 0.2)
         << "][" << cbrt_2deriv(to_root, answer - answer * 0.1)
         << "][" << cbrt_2deriv(to_root, answer - answer * 0.05)
         << "][" << cbrt_2deriv(to_root, answer + answer * 0.05)
         << "][" << cbrt_2deriv(to_root, answer + answer * 0.1)
         << "][" << cbrt_2deriv(to_root, answer + answer * 0.2)
         << "][" << cbrt_2deriv(to_root, answer + answer * 0.5)
         << "][" << cbrt_2deriv(to_root, answer + answer)
         << "][" << cbrt_2deriv(to_root, answer + answer * 5) << "]]\n";

      ex_out << "[[schr'''&#xf6;'''der_iterate]["
         << cbrt_2deriv_s(to_root, answer / 6)
         << "][" << cbrt_2deriv_s(to_root, answer / 2)
         << "][" << cbrt_2deriv_s(to_root, answer - answer * 0.5)
         << "][" << cbrt_2deriv_s(to_root, answer - answer * 0.2)
         << "][" << cbrt_2deriv_s(to_root, answer - answer * 0.1)
         << "][" << cbrt_2deriv_s(to_root, answer - answer * 0.05)
         << "][" << cbrt_2deriv_s(to_root, answer + answer * 0.05)
         << "][" << cbrt_2deriv_s(to_root, answer + answer * 0.1)
         << "][" << cbrt_2deriv_s(to_root, answer + answer * 0.2)
         << "][" << cbrt_2deriv_s(to_root, answer + answer * 0.5)
         << "][" << cbrt_2deriv_s(to_root, answer + answer)
         << "][" << cbrt_2deriv_s(to_root, answer + answer * 5) << "]]\n]\n\n";


      double radius_a = 10;
      double arc_length = 500;
      double radius_b = 123.6216507967705;

      ex_out << std::setprecision(4) << "[table\n"
         << "[[Initial Guess=][-500% ([approx]" << radius_b / 6 << ")][-100% ([approx]" << radius_b / 2 << ")][-50% ([approx]"
         << radius_b - radius_b * 0.5 << ")][-20% ([approx]" << radius_b - radius_b * 0.2 << ")][-10% ([approx]" << radius_b - radius_b * 0.1 << ")][-5% ([approx]" << radius_b - radius_b * 0.05 << ")]"
         "[5% ([approx]" << radius_b + radius_b * 0.05 << ")][10% ([approx]" << radius_b + radius_b * 0.1 << ")][20% ([approx]" << radius_b + radius_b * 0.2 << ")][50% ([approx]" << radius_b + radius_b * 0.5 
         << ")][100% ([approx]" << radius_b + radius_b << ")][500 ([approx]" << radius_b + radius_b * 5 << ")]]\n";
      ex_out << "[[bracket_and_solve_root]["
         << elliptic_root_noderiv(radius_a, arc_length, radius_b / 6)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b / 2)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b - radius_b * 0.5)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b - radius_b * 0.2)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b - radius_b * 0.1)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b - radius_b * 0.05)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b + radius_b * 0.05)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b + radius_b * 0.1)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b + radius_b * 0.2)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b + radius_b * 0.5)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b + radius_b)
         << "][" << elliptic_root_noderiv(radius_a, arc_length, radius_b + radius_b * 5) << "]]\n";

      ex_out << "[[newton_iterate]["
         << elliptic_root_1deriv(radius_a, arc_length, radius_b / 6)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b / 2)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b - radius_b * 0.5)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b - radius_b * 0.2)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b - radius_b * 0.1)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b - radius_b * 0.05)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b + radius_b * 0.05)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b + radius_b * 0.1)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b + radius_b * 0.2)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b + radius_b * 0.5)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b + radius_b)
         << "][" << elliptic_root_1deriv(radius_a, arc_length, radius_b + radius_b * 5) << "]]\n";

      ex_out << "[[halley_iterate]["
         << elliptic_root_2deriv(radius_a, arc_length, radius_b / 6)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b / 2)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b - radius_b * 0.5)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b - radius_b * 0.2)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b - radius_b * 0.1)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b - radius_b * 0.05)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b + radius_b * 0.05)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b + radius_b * 0.1)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b + radius_b * 0.2)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b + radius_b * 0.5)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b + radius_b)
         << "][" << elliptic_root_2deriv(radius_a, arc_length, radius_b + radius_b * 5) << "]]\n";

      ex_out << "[[schr'''&#xf6;'''der_iterate]["
         << elliptic_root_2deriv_s(radius_a, arc_length, radius_b / 6)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b / 2)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b - radius_b * 0.5)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b - radius_b * 0.2)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b - radius_b * 0.1)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b - radius_b * 0.05)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b + radius_b * 0.05)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b + radius_b * 0.1)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b + radius_b * 0.2)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b + radius_b * 0.5)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b + radius_b)
         << "][" << elliptic_root_2deriv_s(radius_a, arc_length, radius_b + radius_b * 5) << "]]\n]\n\n";

      return boost::exit_success;
   }
   catch(std::exception ex)
   {
      ex_out << "exception thrown: " << ex.what() << std::endl;
      return boost::exit_failure;
   }
    return 0;  // main's implicit return
} // int example_main()

TEST_CASE("example: root_finding_start_locations") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex([table
[[Initial Guess=][-500% ([approx]1.323)][-100% ([approx]3.97)][-50% ([approx]3.96)][-20% ([approx]6.35)][-10% ([approx]7.14)][-5% ([approx]7.54)][5% ([approx]8.33)][10% ([approx]8.73)][20% ([approx]9.52)][50% ([approx]11.91)][100% ([approx]15.87)][500 ([approx]47.6)]]
[[bracket_and_solve_root][12][8][8][10][11][11][11][11][11][11][7][13]]
[[newton_iterate][12][7][7][5][5][4][4][5][5][6][7][9]]
[[halley_iterate][6][4][4][3][3][3][3][3][3][4][4][6]]
[[schr'''&#xf6;'''der_iterate][11][6][6][4][4][3][3][3][4][5][5][8]]
]

[table
[[Initial Guess=][-500% ([approx]20.6)][-100% ([approx]61.81)][-50% ([approx]61.81)][-20% ([approx]98.9)][-10% ([approx]111.3)][-5% ([approx]117.4)][5% ([approx]129.8)][10% ([approx]136)][20% ([approx]148.3)][50% ([approx]185.4)][100% ([approx]247.2)][500 ([approx]741.7)]]
[[bracket_and_solve_root][9][7][7][8][8][7][7][8][7][8][6][10]]
[[newton_iterate][4][4][4][3][3][3][3][3][3][4][4][4]]
[[halley_iterate][4][3][3][3][3][2][2][3][3][3][3][3]]
[[schr'''&#xf6;'''der_iterate][4][3][3][3][3][2][2][3][3][3][3][3]]
]

)ex"));
#endif
}
}  // namespace ex_root_finding_start_locations

// libs/math/example/students_t_example2.cpp
namespace ex_students_t_example2 {
   using boost::math::students_t;  // Probability of students_t(df, t).

   
   using std::endl;
   using std::setprecision;
   using std::setw;
   using std::sqrt;

// This example of a one-sided test is from:
//
// from Statistics for Analytical Chemistry, 3rd ed. (1994), pp 59-60
// J. C. Miller and J. N. Miller, Ellis Horwood ISBN 0 13 0309907.

// An acid-base titrimetric method has a significant indicator error and
// thus tends to give results with a positive systematic error (+bias).
// To test this an exactly 0.1 M solution of acid is used to titrate
// 25.00 ml of exactly 0.1 M solution of alkali,
// with the following results (ml):

double reference = 25.00; // 'True' result.
const int values = 6; // titrations.
double data [values] = {25.06, 25.18, 24.87, 25.51, 25.34, 25.41};

int example_main()
{
   ex_out << "Example2 using Student's t function. ";
#if defined(__FILE__) && defined(__TIMESTAMP__) && defined(_MSC_FULL_VER)
   ex_out << "  " << __FILE__ << ' ' << __TIMESTAMP__ << ' '<< _MSC_FULL_VER;
#endif
   ex_out << endl;

   double sum = 0.;
   for (int value = 0; value < values; value++)
   { // Echo data and calculate mean.
      sum += data[value];
      ex_out << setw(4) << value << ' ' << setw(14) << data[value] << endl;
   }
   double mean = sum /static_cast<double>(values);
   ex_out << "Mean = " << mean << endl; // 25.2283

   double sd = 0.;
   for (int value = 0; value < values; value++)
   { // Calculate standard deviation.
      sd +=(data[value] - mean) * (data[value] - mean);
   }
   int degrees_of_freedom = values - 1; // Use the n-1 formula.
   sd /= degrees_of_freedom; // == variance.
   sd= sqrt(sd);
   ex_out << "Standard deviation = " << sd<< endl; // = 0.238279

   double t = (mean - reference) * sqrt(static_cast<double>(values))/ sd; //
   ex_out << "Student's t = " << t << ", with " << degrees_of_freedom << " degrees of freedom." << endl; // = 2.34725

   ex_out << "Probability of positive bias is " << cdf(students_t(degrees_of_freedom), t) << "."<< endl; // =  0.967108.
   // A 1-sided test because only testing for a positive bias.
   // If > 0.95 then greater than 1 in 20 conventional (arbitrary) requirement.

   return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:

------ Build started: Project: students_t_example2, Configuration: Debug Win32 ------
Compiling...
students_t_example2.cpp
Linking...
Autorun "i:\boost-06-05-03-1300\libs\math\test\Math_test\debug\students_t_example2.exe"
Example2 using Student's t function.   ..\..\..\..\..\..\boost-sandbox\libs\math_functions\example\students_t_example2.cpp Sat Aug 12 16:55:59 2006 140050727
   0          25.06
   1          25.18
   2          24.87
   3          25.51
   4          25.34
   5          25.41
Mean = 25.2283
Standard deviation = 0.238279
Student's t = 2.34725, with 5 degrees of freedom.
Probability of positive bias is 0.967108.
Build Time 0:03
Build log was saved at "file://i:\boost-06-05-03-1300\libs\math\test\Math_test\students_t_example2\Debug\BuildLog.htm"
students_t_example2 - 0 error(s), 0 warning(s)
========== Build: 1 succeeded, 0 failed, 0 up-to-date, 0 skipped ==========

*/

TEST_CASE("example: students_t_example2") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Example2 using Student's t function. 
   0          25.06
   1          25.18
   2          24.87
   3          25.51
   4          25.34
   5          25.41
Mean = 25.2283
Standard deviation = 0.238279
Student's t = 2.34725, with 5 degrees of freedom.
Probability of positive bias is 0.967108.
)ex"));
#endif
}
}  // namespace ex_students_t_example2

// libs/math/example/test_nonfinite_loopback.cpp
namespace ex_test_nonfinite_loopback {
using std::endl;
using std::locale;
using std::string;
  using std::stringstream;
using std::numeric_limits;

int example_main()
{
   locale default_locale; // Current global locale.
  // Try to use the default locale first.
  // On MSVC this doesn't work.

  { // Try infinity.
    stringstream ss; // Both input and output.
    ss.imbue(default_locale); // Redundant, of course.
    string infs;
    if(numeric_limits<double>::has_infinity)
    {  // Make sure infinity is specialised for type double.
      double inf = numeric_limits<double>::infinity();
      ss << inf; // Output infinity.
      infs = ss.str();  //
    }
    else
    { // Need to provide a suitable string for infinity.
     infs =  "1.#INF"; // Might suit MSVC?
      ss << infs;
    }
    double r;
    ss >> r; // Read back in.

    ex_out << "infinity output was " << infs << endl; // "1.#INF"
    ex_out << "infinity input was " << r << endl; // "1"
  }

    { // Try Quiet NaN
    stringstream ss; // Both input and output.
    ss.imbue(default_locale); // Redundant, of course.
    string infs;
    if(numeric_limits<double>::has_quiet_NaN)
    {  // Make sure quiet NaN is specialised for type double.
      double qnan = numeric_limits<double>::quiet_NaN();
      ss << qnan; // Output quiet_NaN.
      infs = ss.str();  //
    }
    else
    { // Need to provide a suitable string for quiet_NAN.
     infs =  "1.#QNAN";
      ss << infs;
    }
    double r;
    ss >> r; // Read back in.

    ex_out << "quiet_NaN output was " << infs << endl; // "1.#QNAN"
    ex_out << "quiet_NaN input was " << r << endl; // "1#"
  }


    return 0;  // main's implicit return
} // int example_main()

/*

Output (MSVC Version 10.0):


  infinity output was 1.#INF
  infinity input was 1
  quiet_NaN output was 1.#QNAN
  quiet_NaN input was 1


*/

TEST_CASE("example: test_nonfinite_loopback") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(infinity output was inf
infinity input was 0
quiet_NaN output was nan
quiet_NaN input was 0
)ex"));
#endif
}
}  // namespace ex_test_nonfinite_loopback
