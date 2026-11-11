// mcpplibs.mwrap.boost.math — the official examples (libs/math/example/*.cpp, part 1 of 4),
// ported mechanically: each program runs as a case and its output must equal
// what the same code printed when built from upstream/boost headers alone. The
// headers of the macros the examples use (BOOST_MATH_DECLARE_*, BOOST_MATH_ASSERT,
// BOOST_FLOAT64_C, BOOST_SERIALIZATION_NVP) are #included before the imports.
// Outputs with random seeds, timings, the float layout or the C++/C library's
// text are not compared (random_search, inspect_fp, naive_monte_carlo,
// reverse_mode_linear_regression, differential_evolution; root_finding_example:
// std::cbrt's last digit; autodiff_reverse_black_scholes: last digits of libm results;
// nonfinite_num_facet_trap: the iostream_category message).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <cmath>
#include <boost/math/tools/config.hpp>
#include <boost/assert.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/distributions/fwd.hpp>
#include <boost/serialization/nvp.hpp>
#include <cassert>
#include <boost/config.hpp>
#include <boost/math/policies/policy.hpp>
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

// libs/math/example/agm_example.cpp
namespace ex_agm_example {
#ifndef BOOST_MATH_STANDALONE
#endif

// This example computes the lemniscate constant to high precision using the agm:
using boost::math::tools::agm;
using boost::math::constants::pi;

int example_main() {
    using std::sqrt;
    
    #ifndef BOOST_MATH_STANDALONE
    using Real = boost::multiprecision::cpp_bin_float_100;
    #else
    using Real = long double;
    #endif

    Real G = agm(sqrt(Real(2)), Real(1));
    ex_out << std::setprecision(std::numeric_limits<Real>::max_digits10);
    ex_out << " Gauss's lemniscate constant = " << pi<Real>()/G << "\n";
    ex_out << "Expected lemniscate constant = " << "2.62205755429211981046483958989111941368275495143162316281682170380079058707041425023029553296142909344613\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: agm_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( Gauss's lemniscate constant = 2.62205755429211981046483958989111941368275495143162316281682170380079058707041425023029553296142909352
Expected lemniscate constant = 2.62205755429211981046483958989111941368275495143162316281682170380079058707041425023029553296142909344613
)ex"));
}
}  // namespace ex_agm_example

// libs/math/example/autodiff_black_scholes.cpp
namespace ex_autodiff_black_scholes {
using namespace boost::math::constants;
using namespace boost::math::differentiation;

// Equations and function/variable names are from
// https://en.wikipedia.org/wiki/Greeks_(finance)#Formulas_for_European_option_Greeks

// Standard normal probability density function
template <typename X>
X phi(X const& x) {
  return one_div_root_two_pi<X>() * exp(-0.5 * x * x);
}

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
  double const K = 100.0;  // Strike price.
  auto const variables = make_ftuple<double, 3, 3, 1, 1>(105, 5, 30.0 / 365, 1.25 / 100);
  auto const& S = std::get<0>(variables);      // Stock price.
  auto const& sigma = std::get<1>(variables);  // Volatility.
  auto const& tau = std::get<2>(variables);    // Time to expiration in years. (30 days).
  auto const& r = std::get<3>(variables);      // Interest rate.
  auto const call_price = black_scholes_option_price(CP::call, K, S, sigma, tau, r);
  auto const put_price = black_scholes_option_price(CP::put, K, S, sigma, tau, r);

  double const d1 = static_cast<double>((log(S / K) + (r + sigma * sigma / 2) * tau) / (sigma * sqrt(tau)));
  double const d2 = static_cast<double>((log(S / K) + (r - sigma * sigma / 2) * tau) / (sigma * sqrt(tau)));
  double const formula_call_delta = +Phi(+d1);
  double const formula_put_delta = -Phi(-d1);
  double const formula_vega = static_cast<double>(S * phi(d1) * sqrt(tau));
  double const formula_call_theta =
      static_cast<double>(-S * phi(d1) * sigma / (2 * sqrt(tau)) - r * K * exp(-r * tau) * Phi(+d2));
  double const formula_put_theta =
      static_cast<double>(-S * phi(d1) * sigma / (2 * sqrt(tau)) + r * K * exp(-r * tau) * Phi(-d2));
  double const formula_call_rho = static_cast<double>(+K * tau * exp(-r * tau) * Phi(+d2));
  double const formula_put_rho = static_cast<double>(-K * tau * exp(-r * tau) * Phi(-d2));
  double const formula_gamma = static_cast<double>(phi(d1) / (S * sigma * sqrt(tau)));
  double const formula_vanna = static_cast<double>(-phi(d1) * d2 / sigma);
  double const formula_charm =
      static_cast<double>(phi(d1) * (d2 * sigma * sqrt(tau) - 2 * r * tau) / (2 * tau * sigma * sqrt(tau)));
  double const formula_vomma = static_cast<double>(S * phi(d1) * sqrt(tau) * d1 * d2 / sigma);
  double const formula_veta = static_cast<double>(-S * phi(d1) * sqrt(tau) *
                                                  (r * d1 / (sigma * sqrt(tau)) - (1 + d1 * d2) / (2 * tau)));
  double const formula_speed =
      static_cast<double>(-phi(d1) * (d1 / (sigma * sqrt(tau)) + 1) / (S * S * sigma * sqrt(tau)));
  double const formula_zomma = static_cast<double>(phi(d1) * (d1 * d2 - 1) / (S * sigma * sigma * sqrt(tau)));
  double const formula_color =
      static_cast<double>(-phi(d1) / (2 * S * tau * sigma * sqrt(tau)) *
                          (1 + (2 * r * tau - d2 * sigma * sqrt(tau)) * d1 / (sigma * sqrt(tau))));
  double const formula_ultima =
      -formula_vega * static_cast<double>((d1 * d2 * (1 - d1 * d2) + d1 * d1 + d2 * d2) / (sigma * sigma));

  ex_out << std::setprecision(std::numeric_limits<double>::digits10)
            << "autodiff black-scholes call price = " << call_price.derivative(0, 0, 0, 0) << '\n'
            << "autodiff black-scholes put  price = " << put_price.derivative(0, 0, 0, 0) << '\n'
            << "\n## First-order Greeks\n"
            << "autodiff call delta = " << call_price.derivative(1, 0, 0, 0) << '\n'
            << " formula call delta = " << formula_call_delta << '\n'
            << "autodiff call vega  = " << call_price.derivative(0, 1, 0, 0) << '\n'
            << " formula call vega  = " << formula_vega << '\n'
            << "autodiff call theta = " << -call_price.derivative(0, 0, 1, 0)
            << '\n'  // minus sign due to tau = T-time
            << " formula call theta = " << formula_call_theta << '\n'
            << "autodiff call rho   = " << call_price.derivative(0, 0, 0, 1) << '\n'
            << " formula call rho   = " << formula_call_rho << '\n'
            << '\n'
            << "autodiff put delta = " << put_price.derivative(1, 0, 0, 0) << '\n'
            << " formula put delta = " << formula_put_delta << '\n'
            << "autodiff put vega  = " << put_price.derivative(0, 1, 0, 0) << '\n'
            << " formula put vega  = " << formula_vega << '\n'
            << "autodiff put theta = " << -put_price.derivative(0, 0, 1, 0) << '\n'
            << " formula put theta = " << formula_put_theta << '\n'
            << "autodiff put rho   = " << put_price.derivative(0, 0, 0, 1) << '\n'
            << " formula put rho   = " << formula_put_rho << '\n'
            << "\n## Second-order Greeks\n"
            << "autodiff call gamma = " << call_price.derivative(2, 0, 0, 0) << '\n'
            << "autodiff put  gamma = " << put_price.derivative(2, 0, 0, 0) << '\n'
            << "      formula gamma = " << formula_gamma << '\n'
            << "autodiff call vanna = " << call_price.derivative(1, 1, 0, 0) << '\n'
            << "autodiff put  vanna = " << put_price.derivative(1, 1, 0, 0) << '\n'
            << "      formula vanna = " << formula_vanna << '\n'
            << "autodiff call charm = " << -call_price.derivative(1, 0, 1, 0) << '\n'
            << "autodiff put  charm = " << -put_price.derivative(1, 0, 1, 0) << '\n'
            << "      formula charm = " << formula_charm << '\n'
            << "autodiff call vomma = " << call_price.derivative(0, 2, 0, 0) << '\n'
            << "autodiff put  vomma = " << put_price.derivative(0, 2, 0, 0) << '\n'
            << "      formula vomma = " << formula_vomma << '\n'
            << "autodiff call veta = " << call_price.derivative(0, 1, 1, 0) << '\n'
            << "autodiff put  veta = " << put_price.derivative(0, 1, 1, 0) << '\n'
            << "      formula veta = " << formula_veta << '\n'
            << "\n## Third-order Greeks\n"
            << "autodiff call speed = " << call_price.derivative(3, 0, 0, 0) << '\n'
            << "autodiff put  speed = " << put_price.derivative(3, 0, 0, 0) << '\n'
            << "      formula speed = " << formula_speed << '\n'
            << "autodiff call zomma = " << call_price.derivative(2, 1, 0, 0) << '\n'
            << "autodiff put  zomma = " << put_price.derivative(2, 1, 0, 0) << '\n'
            << "      formula zomma = " << formula_zomma << '\n'
            << "autodiff call color = " << call_price.derivative(2, 0, 1, 0) << '\n'
            << "autodiff put  color = " << put_price.derivative(2, 0, 1, 0) << '\n'
            << "      formula color = " << formula_color << '\n'
            << "autodiff call ultima = " << call_price.derivative(0, 3, 0, 0) << '\n'
            << "autodiff put  ultima = " << put_price.derivative(0, 3, 0, 0) << '\n'
            << "      formula ultima = " << formula_ultima << '\n';
  return 0;
    return 0;  // main's implicit return
}
/*
Output:
autodiff black-scholes call price = 56.5136030677739
autodiff black-scholes put  price = 51.4109161009333

## First-order Greeks
autodiff call delta = 0.773818444921273
 formula call delta = 0.773818444921274
autodiff call vega  = 9.05493427705736
 formula call vega  = 9.05493427705736
autodiff call theta = -275.73013426444
 formula call theta = -275.73013426444
autodiff call rho   = 2.03320550539396
 formula call rho   = 2.03320550539396

autodiff put delta = -0.226181555078726
 formula put delta = -0.226181555078726
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
autodiff put  veta = 26.7947073115641
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
autodiff call ultima = -0.0922426864775683
autodiff put  ultima = -0.0922426864775683
      formula ultima = -0.0922426864775685
**/

TEST_CASE("example: autodiff_black_scholes") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(autodiff black-scholes call price = 56.5136030677739
autodiff black-scholes put  price = 51.4109161009333

## First-order Greeks
autodiff call delta = 0.773818444921273
 formula call delta = 0.773818444921274
autodiff call vega  = 9.05493427705736
 formula call vega  = 9.05493427705736
autodiff call theta = -275.73013426444
 formula call theta = -275.73013426444
autodiff call rho   = 2.03320550539396
 formula call rho   = 2.03320550539396

autodiff put delta = -0.226181555078726
 formula put delta = -0.226181555078726
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
autodiff put  veta = 26.7947073115641
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
autodiff call ultima = -0.0922426864775683
autodiff put  ultima = -0.0922426864775683
      formula ultima = -0.0922426864775685
)ex"));
}
}  // namespace ex_autodiff_black_scholes

// libs/math/example/autodiff_multiprecision.cpp
namespace ex_autodiff_multiprecision {
using namespace boost::math::differentiation;

template <typename W, typename X, typename Y, typename Z>
promote<W, X, Y, Z> f(const W& w, const X& x, const Y& y, const Z& z) {
  using namespace std;
  return exp(w * sin(x * log(y) / z) + sqrt(w * z / (x * y))) + w * w / tan(z);
}

int example_main() {
  using float50 = boost::multiprecision::cpp_bin_float_50;

  constexpr unsigned Nw = 3;  // Max order of derivative to calculate for w
  constexpr unsigned Nx = 2;  // Max order of derivative to calculate for x
  constexpr unsigned Ny = 4;  // Max order of derivative to calculate for y
  constexpr unsigned Nz = 3;  // Max order of derivative to calculate for z
  // Declare 4 independent variables together into a std::tuple.
  auto const variables = make_ftuple<float50, Nw, Nx, Ny, Nz>(11, 12, 13, 14);
  auto const& w = std::get<0>(variables);  // Up to Nw derivatives at w=11
  auto const& x = std::get<1>(variables);  // Up to Nx derivatives at x=12
  auto const& y = std::get<2>(variables);  // Up to Ny derivatives at y=13
  auto const& z = std::get<3>(variables);  // Up to Nz derivatives at z=14
  auto const v = f(w, x, y, z);
  // Calculated from Mathematica symbolic differentiation.
  float50 const answer("1976.319600747797717779881875290418720908121189218755");
  ex_out << std::setprecision(std::numeric_limits<float50>::digits10)
            << "mathematica   : " << answer << '\n'
            << "autodiff      : " << v.derivative(Nw, Nx, Ny, Nz) << '\n'
            << std::setprecision(3)
            << "relative error: " << (v.derivative(Nw, Nx, Ny, Nz) / answer - 1) << '\n';
  return 0;
    return 0;  // main's implicit return
}
/*
Output:
mathematica   : 1976.3196007477977177798818752904187209081211892188
autodiff      : 1976.3196007477977177798818752904187209081211892188
relative error: 2.67e-50
**/

TEST_CASE("example: autodiff_multiprecision") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(mathematica   : 1976.3196007477977177798818752904187209081211892188
autodiff      : 1976.319600747797717779881875290418720908121189219
relative error: 1.44e-49
)ex"));
}
}  // namespace ex_autodiff_multiprecision

// libs/math/example/bernoulli_example.cpp
namespace ex_bernoulli_example {
#pragma warning (disable : 4100) // unreferenced formal parameter.
#pragma warning (disable : 4127) // conditional expression is constant.

//#define BOOST_MATH_OVERFLOW_ERROR_POLICY ignore_error



/* First 50 from 2 to 100 inclusive: */
/* TABLE[N[BernoulliB[n], 200], {n,2,100,2}] */

//SC_(0.1666666666666666666666666666666666666666), 
//SC_(-0.0333333333333333333333333333333333333333), 
//SC_(0.0238095238095238095238095238095238095238), 
//SC_(-0.0333333333333333333333333333333333333333), 
//SC_(0.0757575757575757575757575757575757575757), 
//SC_(-0.2531135531135531135531135531135531135531), 
//SC_(1.1666666666666666666666666666666666666666), 
//SC_(-7.0921568627450980392156862745098039215686), 
//SC_(54.9711779448621553884711779448621553884711), 

int example_main()
{
  //[bernoulli_example_1

/*`A simple example computes the value of B[sub 4] where the return type is `double`,
note that the argument to bernoulli_b2n is ['2] not ['4] since it computes B[sub 2N].


*/ 
  try
  { // It is always wise to use try'n'catch blocks around Boost.Math functions
    // so that any informative error messages can be displayed in the catch block.
  ex_out
    << std::setprecision(std::numeric_limits<double>::digits10)
    << boost::math::bernoulli_b2n<double>(2) << std::endl;

/*`So B[sub 4] == -1/30 == -0.0333333333333333 

If we use Boost.Multiprecision and its 50 decimal digit floating-point type `cpp_dec_float_50`,
we can calculate the value of much larger numbers like B[sub 200]
and also obtain much higher precision.
*/

  ex_out
    << std::setprecision(std::numeric_limits<boost::multiprecision::cpp_dec_float_50>::digits10)
    << boost::math::bernoulli_b2n<boost::multiprecision::cpp_dec_float_50>(100) << std::endl;
 
//] //[/bernoulli_example_1]

//[bernoulli_example_2
/*`We can compute and save all the float-precision Bernoulli numbers from one call.
*/
  std::vector<float> bn; // Space for 32-bit `float` precision Bernoulli numbers.

  // Start with Bernoulli number 0.
  boost::math::bernoulli_b2n<float>(0, 32, std::back_inserter(bn)); // Fill vector with even Bernoulli numbers.

  for(size_t i = 0; i < bn.size(); i++)
  { // Show vector of even Bernoulli numbers, showing all significant decimal digits.
      ex_out << std::setprecision(std::numeric_limits<float>::digits10)
          << i*2 << ' '           
          << bn[i]
          << std::endl;
  }
//] //[/bernoulli_example_2]

  }
  catch(const std::exception& ex)
  {
     ex_out << "Thrown Exception caught: " << ex.what() << std::endl;
  }


//[bernoulli_example_3    
/*`Of course, for any floating-point type, there is a maximum Bernoulli number that can be computed
  before it overflows the exponent.
  By default policy, if we try to compute too high a Bernoulli number, an exception will be thrown.
*/
  try
  {
    ex_out
    << std::setprecision(std::numeric_limits<float>::digits10)
    << "Bernoulli number " << 33 * 2 <<std::endl;

    ex_out << boost::math::bernoulli_b2n<float>(33) << std::endl;
  }
  catch (std::exception ex)
  {
    ex_out << "Thrown Exception caught: " << ex.what() << std::endl;
  }

/*`
and we will get a helpful error message (provided try'n'catch blocks are used).
*/

//] //[/bernoulli_example_3]

//[bernoulli_example_4
/*For example:
*/
   ex_out << "boost::math::max_bernoulli_b2n<float>::value = "  << boost::math::max_bernoulli_b2n<float>::value << std::endl;
   ex_out << "Maximum Bernoulli number using float is " << boost::math::bernoulli_b2n<float>( boost::math::max_bernoulli_b2n<float>::value) << std::endl;
   ex_out << "boost::math::max_bernoulli_b2n<double>::value = "  << boost::math::max_bernoulli_b2n<double>::value << std::endl;
   ex_out << "Maximum Bernoulli number using double is " << boost::math::bernoulli_b2n<double>( boost::math::max_bernoulli_b2n<double>::value) << std::endl;
  //] //[/bernoulli_example_4]

//[tangent_example_1

/*`We can compute and save a few Tangent numbers.
*/
  std::vector<float> tn; // Space for some `float` precision Tangent numbers.

  // Start with Bernoulli number 0.
  boost::math::tangent_t2n<float>(1, 6, std::back_inserter(tn)); // Fill vector with even Tangent numbers.

  for(size_t i = 0; i < tn.size(); i++)
  { // Show vector of even Tangent numbers, showing all significant decimal digits.
      ex_out << std::setprecision(std::numeric_limits<float>::digits10)
          << " "
          << tn[i];
  }
  ex_out << std::endl;

//] [/tangent_example_1]

// 1, 2, 16, 272, 7936, 353792, 22368256, 1903757312 



    return 0;  // main's implicit return
} // int example_main()

/*

//[bernoulli_output_1
  -3.6470772645191354362138308865549944904868234686191e+215
//] //[/bernoulli_output_1]

//[bernoulli_output_2

  0 1
  2 0.166667
  4 -0.0333333
  6 0.0238095
  8 -0.0333333
  10 0.0757576
  12 -0.253114
  14 1.16667
  16 -7.09216
  18 54.9712
  20 -529.124
  22 6192.12
  24 -86580.3
  26 1.42552e+006
  28 -2.72982e+007
  30 6.01581e+008
  32 -1.51163e+010
  34 4.29615e+011
  36 -1.37117e+013
  38 4.88332e+014
  40 -1.92966e+016
  42 8.41693e+017
  44 -4.03381e+019
  46 2.11507e+021
  48 -1.20866e+023
  50 7.50087e+024
  52 -5.03878e+026
  54 3.65288e+028
  56 -2.84988e+030
  58 2.38654e+032
  60 -2.14e+034
  62 2.0501e+036
//] //[/bernoulli_output_2]

//[bernoulli_output_3
 Bernoulli number 66
 Thrown Exception caught: Error in function boost::math::bernoulli_b2n<float>(n):
 Overflow evaluating function at 33
//] //[/bernoulli_output_3]
//[bernoulli_output_4
  boost::math::max_bernoulli_b2n<float>::value = 32
  Maximum Bernoulli number using float is -2.0938e+038
  boost::math::max_bernoulli_b2n<double>::value = 129
  Maximum Bernoulli number using double is 1.33528e+306
//] //[/bernoulli_output_4]

  
//[tangent_output_1
   1 2 16 272 7936 353792
//] [/tangent_output_1]



*/

TEST_CASE("example: bernoulli_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(-0.0333333333333333
-3.6470772645191354362138308865549944904868234686191e+215
0 1
2 0.166667
4 -0.0333333
6 0.0238095
8 -0.0333333
10 0.0757576
12 -0.253114
14 1.16667
16 -7.09216
18 54.9712
20 -529.124
22 6192.12
24 -86580.2
26 1.42552e+06
28 -2.72982e+07
30 6.01581e+08
32 -1.51163e+10
34 4.29615e+11
36 -1.37117e+13
38 4.88332e+14
40 -1.92966e+16
42 8.41693e+17
44 -4.03381e+19
46 2.11507e+21
48 -1.20866e+23
50 7.50087e+24
52 -5.03878e+26
54 3.65288e+28
56 -2.84988e+30
58 2.38654e+32
60 -2.14e+34
62 2.0501e+36
Bernoulli number 66
Thrown Exception caught: std::exception
boost::math::max_bernoulli_b2n<float>::value = 32
Maximum Bernoulli number using float is -2.0938e+38
boost::math::max_bernoulli_b2n<double>::value = 129
Maximum Bernoulli number using double is 1.33528e+306
 1 2 16 272 7936 353792
)ex"));
}
}  // namespace ex_bernoulli_example

// libs/math/example/bessel_zeros_interator_example.cpp
namespace ex_bessel_zeros_interator_example {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif


//[bessel_zeros_iterator_example_1

/*`[h5 Using Output Iterator to sum zeros of Bessel Functions]

This example demonstrates summing zeros of the Bessel functions.
To use the functions for finding zeros of the functions we need
 */


/*`We use the `cyl_bessel_j_zero` output iterator parameter `out_it`
to create a sum of ['1/zeros[super 2]] by defining a custom output iterator:
*/

template <class T>
struct output_summation_iterator
{
   output_summation_iterator(T* p) : p_sum(p)
   {}
   output_summation_iterator& operator*()
   { return *this; }
    output_summation_iterator& operator++()
   { return *this; }
   output_summation_iterator& operator++(int)
   { return *this; }
   output_summation_iterator& operator = (T const& val)
   {
     *p_sum += 1./ (val * val); // Summing 1/zero^2.
     return *this;
   }
private:
   T* p_sum;
};

//] [/bessel_zeros_iterator_example_1]

int example_main()
{
  try
  {
//[bessel_zeros_iterator_example_2

/*`The sum is calculated for many values, converging on the analytical exact value of `1/8`.
*/
    using boost::math::cyl_bessel_j_zero;
    double nu = 1.;
    double sum = 0;
    output_summation_iterator<double> it(&sum);  // sum of 1/zeros^2
    cyl_bessel_j_zero(nu, 1, 10000, it);

    double s = 1/(4 * (nu + 1)); // 0.125 = 1/8 is exact analytical solution.
    ex_out << std::setprecision(6) << "nu = " << nu << ", sum = " << sum
      << ", exact = " << s << std::endl;
    // nu = 1.00000, sum = 0.124990, exact = 0.125000
//] [/bessel_zeros_iterator_example_2]
   }
  catch (std::exception const& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }
  return 0;
      return 0;  // main's implicit return
} // int_main()

/*
 Output:

 nu = 1, sum = 0.12499, exact = 0.125
*/

TEST_CASE("example: bessel_zeros_interator_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(nu = 1, sum = 0.12499, exact = 0.125
)ex"));
}
}  // namespace ex_bessel_zeros_interator_example

// libs/math/example/binomial_example_nag.cpp
namespace ex_binomial_example_nag {
   using std::endl; using std::ios; using std::showpoint;
  using std::fixed; using std::setw;

int example_main()
{
  ex_out << "Using the binomial distribution to replicate a NAG library call." << endl;
  using boost::math::binomial_distribution;

  // This replicates the computation of the examples of using nag-binomial_dist
  // using g01bjc in section g01 Simple Calculations on Statistical Data.
  // http://www.nag.co.uk/numeric/cl/manual/pdf/G01/g01bjc.pdf
  // Program results section 8.3 page 3.g01bjc.3
    //8.2. Program Data
    //g01bjc Example Program Data
    //4 0.50 2 : n, p, k
    //19 0.44 13
    //100 0.75 67
    //2000 0.33 700
    //8.3. Program Results
    //g01bjc Example Program Results
    //n p k plek pgtk peqk
    //4 0.500 2 0.68750 0.31250 0.37500
    //19 0.440 13 0.99138 0.00862 0.01939
    //100 0.750 67 0.04460 0.95540 0.01700
    //2000 0.330 700 0.97251 0.02749 0.00312

  ex_out.setf(ios::showpoint); // Trailing zeros to show significant decimal digits.
  ex_out.precision(5); // Might calculate this from trials in distribution?
  ex_out << fixed;
  //  Binomial distribution.

  // Note  that  cdf(dist, k) is equivalent to NAG library plek probability of <= k
  // cdf(complement(dist, k)) is equivalent to NAG library pgtk probability of > k
  //             pdf(dist, k) is equivalent to NAG library peqk probability of == k

  ex_out << " n        p     k     plek     pgtk     peqk " << endl;
  binomial_distribution<>my_dist(4, 0.5);
  ex_out << setw(4) << (int)my_dist.trials() << "  " << my_dist.success_fraction()
  << "   " << 2 << "  " << cdf(my_dist, 2) << "  "
  << cdf(complement(my_dist, 2)) << "  " << pdf(my_dist, 2) << endl;

  binomial_distribution<>two(19, 0.440);
  ex_out << setw(4) << (int)two.trials() <<  "  "  << two.success_fraction()
    << "  " << 13 << "  " << cdf(two, 13) << "  "
    << cdf(complement(two, 13)) << "  " << pdf(two, 13) << endl;

  binomial_distribution<>three(100, 0.750);
  ex_out << setw(4) << (int)three.trials() << "  " << three.success_fraction()
    << "  " << 67 << "  " << cdf(three, 67) << "  " << cdf(complement(three, 67))
    << "  " << pdf(three, 67) << endl;
  binomial_distribution<>four(2000, 0.330);
  ex_out << setw(4) << (int)four.trials() <<  "  "  << four.success_fraction()
  << " " << 700 << "  "
    << cdf(four, 700) << "  " << cdf(complement(four, 700))
    << "  " << pdf(four, 700) << endl;

  return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

Example of using the binomial distribution to replicate a NAG library call.
 n        p     k     plek     pgtk     peqk
   4  0.50000   2  0.68750  0.31250  0.37500
  19  0.44000  13  0.99138  0.00862  0.01939
 100  0.75000  67  0.04460  0.95540  0.01700
2000  0.33000 700  0.97251  0.02749  0.00312


 */

TEST_CASE("example: binomial_example_nag") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Using the binomial distribution to replicate a NAG library call.
 n        p     k     plek     pgtk     peqk 
   4  0.50000   2  0.68750  0.31250  0.37500
  19  0.44000  13  0.99138  0.00862  0.01939
 100  0.75000  67  0.04460  0.95540  0.01700
2000  0.33000 700  0.97251  0.02749  0.00312
)ex"));
}
}  // namespace ex_binomial_example_nag

// libs/math/example/calculate_fourier_transform_daubechies_constants.cpp
namespace ex_calculate_fourier_transform_daubechies_constants {
using std::pow;
using boost::multiprecision::cpp_bin_float_100;
using boost::math::filters::daubechies_scaling_filter;
using boost::math::tools::polynomial;
using boost::math::constants::half;
using boost::math::constants::root_two;

template<typename Real, size_t N>
std::vector<Real> get_constants() {
   auto h = daubechies_scaling_filter<cpp_bin_float_100, N>();
   auto p = polynomial<cpp_bin_float_100>(h.begin(), h.end());

   auto q = polynomial({half<cpp_bin_float_100>(), half<cpp_bin_float_100>()});
   q = pow(q, N);
   auto l = p/q;
   return l.data(); 
}

template<typename Real>
void print_constants(std::vector<Real> const & l) {
   ex_out << std::setprecision(std::numeric_limits<Real>::digits10 -10);
   ex_out << "return std::array<Real, " << l.size() << ">{";
   for (size_t i = 0; i < l.size() - 1; ++i) {
       ex_out << "BOOST_MATH_BIG_CONSTANT(Real, std::numeric_limits<Real>::digits, " << l[i]/root_two<Real>() << "), ";
   }
   ex_out << "BOOST_MATH_BIG_CONSTANT(Real, std::numeric_limits<Real>::digits, " << l.back()/root_two<Real>() << ")};\n";
}

int example_main() {
   auto constants = get_constants<cpp_bin_float_100, 1>();
   print_constants(constants);
    return 0;  // main's implicit return
}

TEST_CASE("example: calculate_fourier_transform_daubechies_constants") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(return std::array<Real, 1>{BOOST_MATH_BIG_CONSTANT(Real, std::numeric_limits<Real>::digits, 0.999999999999999999999999999999999999999999999999999999999999999999999999998510098540667467)};
)ex"));
}
}  // namespace ex_calculate_fourier_transform_daubechies_constants

// libs/math/example/cohen_acceleration.cpp
namespace ex_cohen_acceleration {
using boost::math::tools::cohen_acceleration;
using boost::math::constants::pi;

template<typename Real>
class G {
public:
    G(){
        k_ = 0;
    }
    
    Real operator()() {
        k_ += 1;
        return 1/(k_*k_);
    }

private:
    Real k_;
};

int example_main() {
    using Real = float;
    auto g = G<Real>();
    Real computed = cohen_acceleration(g);
    Real expected = pi<Real>()*pi<Real>()/12;
    ex_out << std::setprecision(std::numeric_limits<Real>::max_digits10);

    ex_out << "Computed = " << computed << " = " << std::hexfloat << computed <<  "\n";
    ex_out << std::defaultfloat;
    ex_out << "Expected = " << expected << " = " << std::hexfloat << expected << "\n";

    // Compute with a specified number of terms:
    // Make sure to reset g:
    g = G<Real>();
    computed = cohen_acceleration(g, 5);
    ex_out << std::defaultfloat;
    ex_out << "Computed = " << computed << " = " << std::hexfloat << computed << " using 5 terms.\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: cohen_acceleration") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Computed = 0.822467148 = 0x1.a51a6ap-1
Expected = 0.822467089 = 0x1.a51a68p-1
Computed = 0.822490573 = 0x1.a51d7cp-1 using 5 terms.
)ex"));
}
}  // namespace ex_cohen_acceleration

// libs/math/example/distribution_construction.cpp
namespace ex_distribution_construction {
#ifdef _MSC_VER
#  pragma warning (disable : 4996) // disable -D_SCL_SECURE_NO_WARNINGS C++ 'Checked Iterators'
#endif


//[distribution_construction_1

/*`
The structure of distributions is rather different from some other statistical libraries,
for example, those written in less object-oriented language like FORTRAN and C that
provide a few arguments to each free function.

Boost.Math library instead provides each distribution as a template C++ class.
A distribution is constructed with a few arguments, and then
member and non-member functions are used to find values of the
distribution, often a function of a random variate.

For this demonstration, first we need some includes to access the
negative binomial distribution (and the binomial, beta and gamma distributions too).

To demonstrate the use with a high precision User-defined floating-point type
`cpp_bin_float`, we also need an include from Boost.Multiprecision.
(We could equally well have used a `cpp_dec_float` multiprecision type).

We choose a typedef `cpp_bin_float_50` to provide a 50 decimal digit type,
but we could equally have chosen at 128-bit type `cpp_bin_float_quad`, 
or on some platforms `__float128`, providing about 35 decimal digits.
*/

  using boost::math::negative_binomial_distribution; // default type is double.
  using boost::math::negative_binomial; // typedef provides default type is double.

/*`
Several examples of constructing distributions follow:
*/
//] [/distribution_construction_1 end of Quickbook in C++ markup]

int example_main()
{
  try
  {
//[distribution_construction_2
/*`
First, a negative binomial distribution with 8 successes
and a success fraction 0.25, 25% or 1 in 4, is constructed like this:
*/
  boost::math::negative_binomial_distribution<double> mydist0(8., 0.25);
  /*`
  But this is inconveniently long, so we might be tempted to write
  */
  using namespace boost::math;
  /*`
  but this might risk ambiguity with names in `std random` so
  [*much] better is explicit `using boost::math::` statements, for example:
  */
  using boost::math::negative_binomial_distribution;
  /*`
  and we can still reduce typing.

  Since the vast majority of applications use will be using `double` precision,
  the template argument to the distribution (`RealType`) defaults
  to type `double`, so we can also write:
  */

  negative_binomial_distribution<> mydist9(8., 0.25); // Uses default `RealType = double`.

  /*`
  But the name `negative_binomial_distribution` is still inconveniently long,
  so, for most distributions, a convenience `typedef` is provided, for example:

     typedef negative_binomial_distribution<double> negative_binomial; // Reserved name of type double.

  [caution
  This convenience typedef is [*not provided] if a clash would occur
  with the name of a function; currently only `beta` and `gamma`
  fall into this category.
  ]

  So, after a using statement,
  */

  using boost::math::negative_binomial;

  /*`
  we have a convenient typedef to `negative_binomial_distribution<double>`:
  */
  negative_binomial mydist(8., 0.25);

  /*`
  Some more examples using the convenience typedef:
  */
  negative_binomial mydist10(5., 0.4); // Both arguments double.
  /*`
  And automatic conversion of arguments takes place, so you can use integers and floats:
  */
  negative_binomial mydist11(5, 0.4); // Using provided typedef of type double, and int and double arguments.
  /*`
  This is probably the most common usage.
  Other combination are possible too:
  */
  negative_binomial mydist12(5., 0.4F); // Double and float arguments.
  negative_binomial mydist13(5, 1); // Both arguments integer.

  /*`
  Similarly for most other distributions like the binomial.
  */
  binomial mybinomial(1, 0.5); // is more concise than
  binomial_distribution<> mybinomd1(1, 0.5);

  /*`
  For cases when the typdef distribution name would clash with a math special function
  (currently only beta and gamma)
  the typedef is deliberately not provided, and the longer version of the name
  must be used, so for example, do not use:

     using boost::math::beta;
     beta mybetad0(1, 0.5); // Error beta is a math FUNCTION!

  Which produces the error messages:

  [pre
  error C2146: syntax error : missing ';' before identifier 'mybetad0'
  warning C4551: function call missing argument list
  error C3861: 'mybetad0': identifier not found
  ]

  Instead you should use:
  */
  using boost::math::beta_distribution;
  beta_distribution<> mybetad1(1, 0.5);
  /*`
  or for the gamma distribution:
  */
  gamma_distribution<> mygammad1(1, 0.5);

  /*`
  We can, of course, still provide the type explicitly thus:
  */

  // Explicit double precision:  both arguments are double:
  negative_binomial_distribution<double>        mydist1(8., 0.25);

  // Explicit float precision, double arguments are truncated to float:
  negative_binomial_distribution<float>         mydist2(8., 0.25);

  // Explicit float precision, integer & double arguments converted to float:
  negative_binomial_distribution<float>         mydist3(8, 0.25);

  // Explicit float precision, float arguments, so no conversion:
  negative_binomial_distribution<float>         mydist4(8.F, 0.25F);

  // Explicit float precision, integer arguments promoted to float.
  negative_binomial_distribution<float>         mydist5(8, 1);

  // Explicit double precision:
  negative_binomial_distribution<double>        mydist6(5., 0.4);

  // Explicit long double precision:
  negative_binomial_distribution<long double>   mydist7(8., 0.25);
     
  /*`
  And you can use your own template RealType,
  for example, `boost::math::cpp_bin_float_50` (an arbitrary 50 decimal digits precision type),
  then we can write:
  */
  using namespace boost::multiprecision;
  negative_binomial_distribution<cpp_bin_float_50>  mydist8(8, 0.25);

  // `integer` arguments are promoted to your RealType exactly, but
  // `double` argument are converted to RealType,
  // most likely losing precision!
  
  // So DON'T be tempted to write the 'obvious':
  negative_binomial_distribution<cpp_bin_float_50>  mydist20(8, 0.23456789012345678901234567890);
 // to avoid truncation of second parameter to `0.2345678901234567` and loss of precision.

 // Instead pass a quoted decimal digit string:
  negative_binomial_distribution<cpp_bin_float_50>  mydist21(8, cpp_bin_float_50("0.23456789012345678901234567890") );

  // Ensure that all potentially significant digits are shown.
  ex_out.precision(std::numeric_limits<cpp_bin_float_50>::digits10);
  // 
  cpp_bin_float_50 x("1.23456789012345678901234567890");
  ex_out << pdf(mydist8, x) << std::endl;
/*` showing  0.00012630010495970320103876754721976419438231705359935
             0.00012630010495970320103876754721976419438231528547467

[warning When using multiprecision, it is all too easy to get accidental truncation!]

For example, if you write
*/
  ex_out << pdf(mydist8, 1.23456789012345678901234567890) << std::endl;
/*`
showing  0.00012630010495970318465064569310967179576805651692929,
which is wrong at about the 17th decimal digit!

This is because the value provided is truncated to a `double`, effectively
  `double x = 1.23456789012345678901234567890;`

Then the now `double x` is passed to function `pdf`,
and this truncated `double` value is finally promoted to `cpp_bin_float_50`.

Another way of quietly getting the wrong answer is to write:
*/
  ex_out << pdf(mydist8, cpp_bin_float_50(1.23456789012345678901234567890)) << std::endl;
/*`
A correct way from a multi-digit string value is
*/
  ex_out << pdf(mydist8, cpp_bin_float_50("1.23456789012345678901234567890")) << std::endl;
/*`

[tip Getting about 17 decimal digits followed by many zeros is often a sign of accidental truncation.]
*/

/*`
[h4 Default arguments to distribution constructors.]

Note that default constructor arguments are only provided for some distributions.
So if you wrongly assume a default argument, you will get an error message, for example:

   negative_binomial_distribution<> mydist8;

[pre error C2512 no appropriate default constructor available.]

No default constructors are provided for the `negative binomial` distribution,
because it is difficult to chose any sensible default values for this distribution.

For other distributions, like the normal distribution,
it is obviously very useful to provide 'standard'
defaults for the mean (zero) and standard deviation (unity) thus:

    normal_distribution(RealType mean = 0, RealType sd = 1);

So in this case we can more tersely write:
*/
  using boost::math::normal;

  normal norm1;       // Standard normal distribution N[0,1].
  normal norm2(2);    // Mean = 2, std deviation = 1.
  normal norm3(2, 3); // Mean = 2, std deviation = 3.

  }
  catch(std::exception &ex)
  {
    ex_out << ex.what() << std::endl;
  }

  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*`There is no useful output from this demonstration program, of course. */

//] [/end of distribution_construction_2]

/*
//[distribution_construction_output

  0.00012630010495970320103876754721976419438231705359935
  0.00012630010495970318465064569310967179576805651692929
  0.00012630010495970318465064569310967179576805651692929
  0.00012630010495970320103876754721976419438231705359935

//] [/distribution_construction_output]


  0.00012630010495970320103876754721976419438231528547467
  0.0001263001049597031846506456931096717957680547488046
  0.0001263001049597031846506456931096717957680547488046
  0.00012630010495970320103876754721976419438231528547467


*/

TEST_CASE("example: distribution_construction") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0.00012630010495970320103876754721976419438231528547466
0.0001263001049597031846506456931096717957680547488046
0.0001263001049597031846506456931096717957680547488046
0.00012630010495970320103876754721976419438231528547466
)ex"));
}
}  // namespace ex_distribution_construction

// libs/math/example/estrin_vs_horner_accuracy.cpp
namespace ex_estrin_vs_horner_accuracy {
using boost::math::tools::evaluate_polynomial_estrin;
using boost::math::tools::evaluate_polynomial;
using boost::math::tools::ulps_plot;

void estrin_vs_horner(size_t n) {
  std::random_device rd;
  auto seed = rd();
  std::mt19937_64 gen(seed);
  std::uniform_real_distribution<float> dis(-1, 1);
  std::vector<float> coeffs_float(n);
  std::vector<double> coeffs_double(n);
  for (size_t i = 0; i < n; ++i) {
    coeffs_float[i] = dis(gen);
    coeffs_double[i] = coeffs_float[i];
  }

  auto hi_acc = [&](double x) {
    return evaluate_polynomial(coeffs_double.data(), x, coeffs_double.size());
  };
  auto estrin_float = [&](float x) { return evaluate_polynomial_estrin(coeffs_float, x); };
  auto horner_float = [&](float x) {
    return evaluate_polynomial(coeffs_float.data(), x, coeffs_float.size());
  };
  auto plot = ulps_plot<decltype(hi_acc), double, float>(hi_acc, float(-2.0),
                                                         float(2.0), 20000);
  plot.ulp_envelope(true);
  plot.add_fn(estrin_float, "steelblue");
  plot.add_fn(horner_float, "orange");
  plot.clip(10);
  plot.title("Horner (orange) vs Estrin (blue) accuracy for a polynomial of degree " + std::to_string(n));
  plot.write("horner_vs_estrin.svg");
}

int example_main() {
  size_t n = 30;
  estrin_vs_horner(n);
  return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: estrin_vs_horner_accuracy") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_estrin_vs_horner_accuracy

// libs/math/example/find_location_example.cpp
namespace ex_find_location_example {
  using boost::math::normal; // typedef provides default type is double.
  using boost::math::cauchy; // typedef provides default type is double.
  using boost::math::find_location; // for mean
  using boost::math::find_scale; // for standard deviation
  using boost::math::complement; // Needed if you want to use the complement version.
  using boost::math::policies::policy;

   using std::endl;
  using std::setw; using std::setprecision;
  using std::numeric_limits;

//] [/find_location1]

int example_main()
{
  ex_out << "Example: Find location (or mean)." << endl;
  try
  {
//[find_location2
/*`
For this example, we will use the standard normal distribution,
with mean (location) zero and standard deviation (scale) unity.
This is also the default for this implementation.
*/
  normal N01;  // Default 'standard' normal distribution with zero mean and
  double sd = 1.; // normal default standard deviation is 1.
/*`Suppose we want to find a different normal distribution whose mean is shifted
so that only fraction p (here 0.001 or 0.1%) are below a certain chosen limit
(here -2, two standard deviations).
*/
  double z = -2.; // z to give prob p
  double p = 0.001; // only 0.1% below z

  ex_out << "Normal distribution with mean = " << N01.location()
    << ", standard deviation " << N01.scale()
    << ", has " << "fraction <= " << z
    << ", p = "  << cdf(N01, z) << endl;
  ex_out << "Normal distribution with mean = " << N01.location()
    << ", standard deviation " << N01.scale()
    << ", has " << "fraction > " << z
    << ", p = "  << cdf(complement(N01, z)) << endl; // Note: uses complement.
/*`
[pre
Normal distribution with mean = 0, standard deviation 1, has fraction <= -2, p = 0.0227501
Normal distribution with mean = 0, standard deviation 1, has fraction > -2, p = 0.97725
]
We can now use ''find_location'' to give a new offset mean.
*/
   double l = find_location<normal>(z, p, sd);
   ex_out << "offset location (mean) = " << l << endl;
/*`
that outputs:
[pre
offset location (mean) = 1.09023
]
showing that we need to shift the mean just over one standard deviation from its previous value of zero.

Then we can check that we have achieved our objective
by constructing a new distribution
with the offset mean (but same standard deviation):
*/
  normal np001pc(l, sd); // Same standard_deviation (scale) but with mean (location) shifted.
/*`
And re-calculating the fraction below our chosen limit.
*/
ex_out << "Normal distribution with mean = " << l
    << " has " << "fraction <= " << z
    << ", p = "  << cdf(np001pc, z) << endl;
  ex_out << "Normal distribution with mean = " << l
    << " has " << "fraction > " << z
    << ", p = "  << cdf(complement(np001pc, z)) << endl;
/*`
[pre
Normal distribution with mean = 1.09023 has fraction <= -2, p = 0.001
Normal distribution with mean = 1.09023 has fraction > -2, p = 0.999
]

[h4 Controlling Error Handling from find_location]
We can also control the policy for handling various errors.
For example, we can define a new (possibly unwise)
policy to ignore domain errors ('bad' arguments).

Unless we are using the boost::math namespace, we will need:
*/
  using boost::math::policies::policy;
  using boost::math::policies::domain_error;
  using boost::math::policies::ignore_error;

/*`
Using a typedef is often convenient, especially if it is re-used,
although it is not required, as the various examples below show.
*/
  typedef policy<domain_error<ignore_error> > ignore_domain_policy;
  // find_location with new policy, using typedef.
  l = find_location<normal>(z, p, sd, ignore_domain_policy());
  // Default policy policy<>, needs "using boost::math::policies::policy;"
  l = find_location<normal>(z, p, sd, policy<>());
  // Default policy, fully specified.
  l = find_location<normal>(z, p, sd, boost::math::policies::policy<>());
  // A new policy, ignoring domain errors, without using a typedef.
  l = find_location<normal>(z, p, sd, policy<domain_error<ignore_error> >());
/*`
If we want to use a probability that is the __complements of our probability,
we should not even think of writing `find_location<normal>(z, 1 - p, sd)`,
but use the complement version, see __why_complements.
*/
  z = 2.;
  double q = 0.95; // = 1 - p; // complement.
  l = find_location<normal>(complement(z, q, sd));

  normal np95pc(l, sd); // Same standard_deviation (scale) but with mean(location) shifted
  ex_out << "Normal distribution with mean = " << l << " has "
    << "fraction <= " << z << " = "  << cdf(np95pc, z) << endl;
  ex_out << "Normal distribution with mean = " << l << " has "
    << "fraction > " << z << " = "  << cdf(complement(np95pc, z)) << endl;
  //] [/find_location2]
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

//[find_location_example_output
/*`
[pre
Example: Find location (mean).
Normal distribution with mean = 0, standard deviation 1, has fraction <= -2, p = 0.0227501
Normal distribution with mean = 0, standard deviation 1, has fraction > -2, p = 0.97725
offset location (mean) = 1.09023
Normal distribution with mean = 1.09023 has fraction <= -2, p = 0.001
Normal distribution with mean = 1.09023 has fraction > -2, p = 0.999
Normal distribution with mean = 0.355146 has fraction <= 2 = 0.95
Normal distribution with mean = 0.355146 has fraction > 2 = 0.05
]
*/
//] [/find_location_example_output]

TEST_CASE("example: find_location_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example: Find location (or mean).
Normal distribution with mean = 0, standard deviation 1, has fraction <= -2, p = 0.0227501
Normal distribution with mean = 0, standard deviation 1, has fraction > -2, p = 0.97725
offset location (mean) = 1.09023
Normal distribution with mean = 1.09023 has fraction <= -2, p = 0.001
Normal distribution with mean = 1.09023 has fraction > -2, p = 0.999
Normal distribution with mean = 0.355146 has fraction <= 2 = 0.95
Normal distribution with mean = 0.355146 has fraction > 2 = 0.05
)ex"));
}
}  // namespace ex_find_location_example

// libs/math/example/float_comparison_example.cpp
namespace ex_float_comparison_example {
int example_main()
{
  ex_out << "Compare floats using Boost.Math functions/classes" << std::endl;


//[compare_floats_using
/*`Some using statements will ensure that the functions we need are accessible.
*/

  using namespace boost::math;

//`or

  using boost::math::relative_difference;
  using boost::math::epsilon_difference;
  using boost::math::float_next;
  using boost::math::float_prior;

//] [/compare_floats_using]


//[compare_floats_example_1
/*`The following examples display values with all possibly significant digits.
Newer compilers should provide `std::numeric_limits<FPT>::max_digits10`
for this purpose, and here we use `float` precision where `max_digits10` = 9
to avoid displaying a distracting number of decimal digits.

[note Older compilers can use this formula to calculate `max_digits10`
from `std::numeric_limits<FPT>::digits10`:
__spaces `int max_digits10 = 2 + std::numeric_limits<FPT>::digits10 * 3010/10000;`
] [/note]

One can set the display including all trailing zeros
(helpful for this example to show all potentially significant digits),
and also to display `bool` values as words rather than integers:
*/
  ex_out.precision(std::numeric_limits<float>::max_digits10);
  ex_out << std::boolalpha << std::showpoint << std::endl;

//] [/compare_floats_example_1]

//[compare_floats_example_2]
/*`
When comparing values that are ['quite close] or ['approximately equal],
we could use either `float_distance` or `relative_difference`/`epsilon_difference`, for example
with type `float`, these two values are adjacent to each other:
*/

  float a = 1;
  float b = 1 + std::numeric_limits<float>::epsilon();
  ex_out << "a = " << a << std::endl;
  ex_out << "b = " << b << std::endl;
  ex_out << "float_distance = " << float_distance(a, b) << std::endl;
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  ex_out << "epsilon_difference = " << epsilon_difference(a, b) << std::endl;

/*`
Which produces the output:

[pre
a = 1.00000000
b = 1.00000012
float_distance = 1.00000000
relative_difference = 1.19209290e-007
epsilon_difference = 1.00000000
]
*/
  //] [/compare_floats_example_2]

//[compare_floats_example_3]
/*`
In the example above, it just so happens that the edit distance as measured by `float_distance`, and the
difference measured in units of epsilon were equal.  However, due to the way floating point
values are represented, that is not always the case:*/

  a = 2.0f / 3.0f;   // 2/3 inexactly represented as a float
  b = float_next(float_next(float_next(a))); // 3 floating point values above a
  ex_out << "a = " << a << std::endl;
  ex_out << "b = " << b << std::endl;
  ex_out << "float_distance = " << float_distance(a, b) << std::endl;
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  ex_out << "epsilon_difference = " << epsilon_difference(a, b) << std::endl;

/*`
Which produces the output:

[pre
a = 0.666666687
b = 0.666666865
float_distance = 3.00000000
relative_difference = 2.68220901e-007
epsilon_difference = 2.25000000
]

There is another important difference between `float_distance` and the
`relative_difference/epsilon_difference` functions in that `float_distance`
returns a signed result that reflects which argument is larger in magnitude,
where as `relative_difference/epsilon_difference` simply return an unsigned
value that represents how far apart the values are.  For example if we swap
the order of the arguments:
*/

  ex_out << "float_distance = " << float_distance(b, a) << std::endl;
  ex_out << "relative_difference = " << relative_difference(b, a) << std::endl;
  ex_out << "epsilon_difference = " << epsilon_difference(b, a) << std::endl;

  /*`
  The output is now:

  [pre
  float_distance = -3.00000000
  relative_difference = 2.68220901e-007
  epsilon_difference = 2.25000000
  ]
*/
  //] [/compare_floats_example_3]

//[compare_floats_example_4]
/*`
Zeros are always treated as equal, as are infinities as long as they have the same sign:*/

  a = 0;
  b = -0;  // signed zero
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  a = b = std::numeric_limits<float>::infinity();
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  ex_out << "relative_difference = " << relative_difference(a, -b) << std::endl;

/*`
Which produces the output:

[pre
relative_difference = 0.000000000
relative_difference = 0.000000000
relative_difference = 3.40282347e+038
]
*/
//] [/compare_floats_example_4]

//[compare_floats_example_5]
/*`
Note that finite values are always infinitely far away from infinities even if those finite values are very large:*/

  a = (std::numeric_limits<float>::max)();
  b = std::numeric_limits<float>::infinity();
  ex_out << "a = " << a << std::endl;
  ex_out << "b = " << b << std::endl;
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  ex_out << "epsilon_difference = " << epsilon_difference(a, b) << std::endl;

/*`
Which produces the output:

[pre
a = 3.40282347e+038
b = 1.#INF0000
relative_difference = 3.40282347e+038
epsilon_difference = 3.40282347e+038
]
*/
//] [/compare_floats_example_5]

//[compare_floats_example_6]
/*`
Finally, all denormalized values and zeros are treated as being effectively equal:*/

  a = std::numeric_limits<float>::denorm_min();
  b = a * 2;
  ex_out << "a = " << a << std::endl;
  ex_out << "b = " << b << std::endl;
  ex_out << "float_distance = " << float_distance(a, b) << std::endl;
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  ex_out << "epsilon_difference = " << epsilon_difference(a, b) << std::endl;
  a = 0;
  ex_out << "a = " << a << std::endl;
  ex_out << "b = " << b << std::endl;
  ex_out << "float_distance = " << float_distance(a, b) << std::endl;
  ex_out << "relative_difference = " << relative_difference(a, b) << std::endl;
  ex_out << "epsilon_difference = " << epsilon_difference(a, b) << std::endl;

/*`
Which produces the output:

[pre
a = 1.40129846e-045
b = 2.80259693e-045
float_distance = 1.00000000
relative_difference = 0.000000000
epsilon_difference = 0.000000000
a = 0.000000000
b = 2.80259693e-045
float_distance = 2.00000000
relative_difference = 0.000000000
epsilon_difference = 0.000000000]

Notice how, in the above example, two denormalized values that are a factor of 2 apart are
none the less only one representation apart!

*/
//] [/compare_floats_example_6]


#if 0
//[old_compare_floats_example_3
//`The simplest use is to compare two values with a tolerance thus:

  bool is_close = is_close_to(1.F, 1.F + epsilon, epsilon); // One epsilon apart is close enough.
  ex_out << "is_close_to(1.F, 1.F + epsilon, epsilon); is " << is_close << std::endl; // true

  is_close = is_close_to(1.F, 1.F + 2 * epsilon, epsilon); // Two epsilon apart isn't close enough.
  ex_out << "is_close_to(1.F, 1.F + epsilon, epsilon); is " << is_close << std::endl; // false

/*`
[note The type FPT of the tolerance and the type of the values [*must match].

So `is_close(0.1F, 1., 1.)` will fail to compile because "template parameter 'FPT' is ambiguous".
Always provide the same type, using `static_cast<FPT>` if necessary.]
*/


/*`An instance of class `close_at_tolerance` is more convenient
when multiple tests with the same conditions are planned.
A class that stores a tolerance of three epsilon (and the default ['strong] test) is:
*/

  close_at_tolerance<float> three_rounds(3 * epsilon); // 'strong' by default.

//`and we can confirm these settings:

  ex_out << "fraction_tolerance = "
    << three_rounds.fraction_tolerance()
    << std::endl; // +3.57627869e-007
  ex_out << "strength = "
    << (three_rounds.strength() == FPC_STRONG ? "strong" : "weak")
    << std::endl; // strong

//`To start, let us use two values that are truly equal (having identical bit patterns)

  float a = 1.23456789F;
  float b = 1.23456789F;

//`and make a comparison using our 3*epsilon `three_rounds` functor:

  bool close = three_rounds(a, b);
  ex_out << "three_rounds(a, b) = " << close << std::endl; // true

//`Unsurprisingly, the result is true, and the failed fraction is zero.

  ex_out << "failed_fraction = " << three_rounds.failed_fraction() << std::endl;

/*`To get some nearby values, it is convenient to use the Boost.Math __next_float functions,
for which we need an include


and some using declarations:
*/

  using boost::math::float_next;
  using boost::math::float_prior;
  using boost::math::nextafter;
  using boost::math::float_distance;

//`To add a few __ulp to one value:
  b = float_next(a); // Add just one ULP to a.
  b = float_next(b); // Add another one ULP.
  b = float_next(b); // Add another one ULP.
  // 3 epsilon would pass.
  b = float_next(b); // Add another one ULP.

//`and repeat our comparison:

  close = three_rounds(a, b);
  ex_out << "three_rounds(a, b) = " << close << std::endl; // false
  ex_out << "failed_fraction = " << three_rounds.failed_fraction()
    << std::endl;  // abs(u-v) / abs(v) = 3.86237957e-007

//`We can also 'measure' the number of bits different using the `float_distance` function:

  ex_out << "float_distance = " << float_distance(a, b) << std::endl; // 4

/*`Now consider two values that are much further apart
than one might expect from ['computational noise],
perhaps the result of two measurements of some physical property like length
where an uncertainty of a percent or so might be expected.
*/
  float fp1 = 0.01000F;
  float fp2 = 0.01001F; // Slightly different.

  float tolerance = 0.0001F;

  close_at_tolerance<float> strong(epsilon); // Default is strong.
  bool rs = strong(fp1, fp2);
  ex_out << "strong(fp1, fp2) is " << rs << std::endl;

//`Or we could contrast using the ['weak] criterion:
  close_at_tolerance<float> weak(epsilon, FPC_WEAK); // Explicitly weak.
  bool rw = weak(fp1, fp2); //
  ex_out << "weak(fp1, fp2) is " << rw << std::endl;

//`We can also construct, setting tolerance and strength, and compare in one statement:

  ex_out << a << " #= " << b << " is "
    << close_at_tolerance<float>(epsilon, FPC_STRONG)(a, b) << std::endl;
  ex_out << a << " ~= " << b << " is "
    << close_at_tolerance<float>(epsilon, FPC_WEAK)(a, b) << std::endl;

//`but this has little advantage over using function `is_close_to` directly.

//] [/old_compare_floats_example_3]


/*When the floating-point values become very small and near zero, using
//a relative test becomes unhelpful because one is dividing by zero or tiny,

//Instead, an absolute test is needed, comparing one (or usually both) values with zero,
//using a tolerance.
//This is provided by the `small_with_tolerance` class and `is_small` function.

  namespace boost {
  namespace math {
  namespace fpc {


  template<typename FPT>
  class small_with_tolerance
  {
  public:
  // Public typedefs.
  typedef bool result_type;

  // Constructor.
  explicit small_with_tolerance(FPT tolerance); // tolerance >= 0

  // Functor
  bool operator()(FPT value) const; // return true if <= absolute tolerance (near zero).
  };

  template<typename FPT>
  bool
  is_small(FPT value, FPT tolerance); // return true if value <= absolute tolerance (near zero).

  }}} // namespaces.

/*`
[note The type FPT of the tolerance and the type of the value [*must match].

So `is_small(0.1F, 0.000001)` will fail to compile because "template parameter 'FPT' is ambiguous".
Always provide the same type, using `static_cast<FPT>` if necessary.]

A few values near zero are tested with varying tolerance below.
*/
//[compare_floats_small_1

  float c = 0;
  ex_out << "0 is_small " << is_small(c, epsilon) << std::endl; // true

  c = std::numeric_limits<float>::denorm_min(); // 1.40129846e-045
  ex_out << "denorm_ min =" << c << ", is_small is " << is_small(c, epsilon) << std::endl; // true

  c = (std::numeric_limits<float>::min)(); // 1.17549435e-038
  ex_out << "min = " << c << ", is_small is " << is_small(c, epsilon) << std::endl; // true

  c = 1 * epsilon; // 1.19209290e-007
  ex_out << "epsilon = " << c << ", is_small is " << is_small(c, epsilon) << std::endl; // false

  c = 1 * epsilon; // 1.19209290e-007
  ex_out << "2 epsilon = " << c << ", is_small is " << is_small(c, 2 * epsilon) << std::endl; // true

  c = 2 * epsilon; //2.38418579e-007
  ex_out << "4 epsilon = " << c << ", is_small is " << is_small(c, 2 * epsilon) << std::endl; // false

  c = 0.00001F;
  ex_out << "0.00001 = " << c << ", is_small is " << is_small(c, 0.0001F) << std::endl; // true

  c = -0.00001F;
  ex_out << "0.00001 = " << c << ", is_small is " << is_small(c, 0.0001F) << std::endl; // true

/*`Using the class `small_with_tolerance` allows storage of the tolerance,
convenient if you make repeated tests with the same tolerance.
*/

  small_with_tolerance<float>my_test(0.01F);

  ex_out << "my_test(0.001F) is " << my_test(0.001F) << std::endl; // true
  ex_out << "my_test(0.001F) is " << my_test(0.01F) << std::endl; // false

  //] [/compare_floats_small_1]
#endif
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Example output is:

//[compare_floats_output
Compare floats using Boost.Test functions/classes

float epsilon = 1.19209290e-007
is_close_to(1.F, 1.F + epsilon, epsilon); is true
is_close_to(1.F, 1.F + epsilon, epsilon); is false
fraction_tolerance = 3.57627869e-007
strength = strong
three_rounds(a, b) = true
failed_fraction = 0.000000000
three_rounds(a, b) = false
failed_fraction = 3.86237957e-007
float_distance = 4.00000000
strong(fp1, fp2) is false
weak(fp1, fp2) is false
1.23456788 #= 1.23456836 is false
1.23456788 ~= 1.23456836 is false
0 is_small true
denorm_ min =1.40129846e-045, is_small is true
min = 1.17549435e-038, is_small is true
epsilon = 1.19209290e-007, is_small is false
2 epsilon = 1.19209290e-007, is_small is true
4 epsilon = 2.38418579e-007, is_small is false
0.00001 = 9.99999975e-006, is_small is true
0.00001 = -9.99999975e-006, is_small is true
my_test(0.001F) is true

my_test(0.001F) is false//] [/compare_floats_output]
*/

TEST_CASE("example: float_comparison_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Compare floats using Boost.Math functions/classes

a = 1.00000000
b = 1.00000012
float_distance = 1.00000000
relative_difference = 1.19209290e-07
epsilon_difference = 1.00000000
a = 0.666666687
b = 0.666666865
float_distance = 3.00000000
relative_difference = 2.68220901e-07
epsilon_difference = 2.25000000
float_distance = -3.00000000
relative_difference = 2.68220901e-07
epsilon_difference = 2.25000000
relative_difference = 0.00000000
relative_difference = 0.00000000
relative_difference = 3.40282347e+38
a = 3.40282347e+38
b = inf
relative_difference = 3.40282347e+38
epsilon_difference = 3.40282347e+38
a = 1.40129846e-45
b = 2.80259693e-45
float_distance = 1.00000000
relative_difference = 0.00000000
epsilon_difference = 0.00000000
a = 0.00000000
b = 2.80259693e-45
float_distance = 2.00000000
relative_difference = 0.00000000
epsilon_difference = 0.00000000
)ex"));
}
}  // namespace ex_float_comparison_example

// libs/math/example/hyperexponential_more_snips.cpp
namespace ex_hyperexponential_more_snips {
struct ds_info
{
   std::string name;
   double iat_sample_mean;
   double iat_sample_sd;
   boost::math::hyperexponential iat_he;
   double multi_lt_sample_mean;
   double multi_lt_sample_sd;
   boost::math::hyperexponential multi_lt_he;
   double single_lt_sample_mean;
   double single_lt_sample_sd;
   boost::math::hyperexponential single_lt_he;
};

// DS1 dataset
ds_info make_ds1()
{
   ds_info ds;

   ds.name = "DS1";

   // VM interarrival time distribution
   const double iat_fit_probs[] = { 0.34561,0.08648,0.56791 };
   const double iat_fit_rates[] = { 0.0008,0.00005,0.02894 };
   ds.iat_sample_mean = 2202.1;
   ds.iat_sample_sd = 2.2e+4;
   ds.iat_he = boost::math::hyperexponential(iat_fit_probs, iat_fit_rates);

   // Multi-core VM lifetime distribution
   const double multi_lt_fit_probs[] = { 0.24667,0.37948,0.37385 };
   const double multi_lt_fit_rates[] = { 0.00004,0.000002,0.00059 };
   ds.multi_lt_sample_mean = 257173;
   ds.multi_lt_sample_sd = 4.6e+5;
   ds.multi_lt_he = boost::math::hyperexponential(multi_lt_fit_probs, multi_lt_fit_rates);

   // Single-core VM lifetime distribution
   const double single_lt_fit_probs[] = { 0.09325,0.22251,0.68424 };
   const double single_lt_fit_rates[] = { 0.000003,0.00109,0.00109 };
   ds.single_lt_sample_mean = 28754.4;
   ds.single_lt_sample_sd = 1.6e+5;
   ds.single_lt_he = boost::math::hyperexponential(single_lt_fit_probs, single_lt_fit_rates);

   return ds;
}

// DS2 dataset
ds_info make_ds2()
{
   ds_info ds;

   ds.name = "DS2";

   // VM interarrival time distribution
   const double iat_fit_probs[] = { 0.38881,0.18227,0.42892 };
   const double iat_fit_rates[] = { 0.000006,0.05228,0.00081 };
   ds.iat_sample_mean = 41285.7;
   ds.iat_sample_sd = 1.1e+05;
   ds.iat_he = boost::math::hyperexponential(iat_fit_probs, iat_fit_rates);

   // Multi-core VM lifetime distribution
   const double multi_lt_fit_probs[] = { 0.42093,0.43960,0.13947 };
   const double multi_lt_fit_rates[] = { 0.00186,0.00008,0.0000008 };
   ds.multi_lt_sample_mean = 144669.0;
   ds.multi_lt_sample_sd = 7.9e+05;
   ds.multi_lt_he = boost::math::hyperexponential(multi_lt_fit_probs, multi_lt_fit_rates);

   // Single-core VM lifetime distribution
   const double single_lt_fit_probs[] = { 0.44885,0.30675,0.2444 };
   const double single_lt_fit_rates[] = { 0.00143,0.00005,0.0000004 };
   ds.single_lt_sample_mean = 599815.0;
   ds.single_lt_sample_sd = 1.7e+06;
   ds.single_lt_he = boost::math::hyperexponential(single_lt_fit_probs, single_lt_fit_rates);

   return ds;
}

// DS3 dataset
ds_info make_ds3()
{
   ds_info ds;

   ds.name = "DS3";

   // VM interarrival time distribution
   const double iat_fit_probs[] = { 0.39442,0.24644,0.35914 };
   const double iat_fit_rates[] = { 0.00030,0.00003,0.00257 };
   ds.iat_sample_mean = 11238.8;
   ds.iat_sample_sd = 3.0e+04;
   ds.iat_he = boost::math::hyperexponential(iat_fit_probs, iat_fit_rates);

   // Multi-core VM lifetime distribution
   const double multi_lt_fit_probs[] = { 0.37621,0.14838,0.47541 };
   const double multi_lt_fit_rates[] = { 0.00498,0.000005,0.00022 };
   ds.multi_lt_sample_mean = 30739.2;
   ds.multi_lt_sample_sd = 1.6e+05;
   ds.multi_lt_he = boost::math::hyperexponential(multi_lt_fit_probs, multi_lt_fit_rates);

   // Single-core VM lifetime distribution
   const double single_lt_fit_probs[] = { 0.34131,0.12544,0.53325 };
   const double single_lt_fit_rates[] = { 0.000297,0.000003,0.00410 };
   ds.single_lt_sample_mean = 44447.8;
   ds.single_lt_sample_sd = 2.2e+05;
   ds.single_lt_he = boost::math::hyperexponential(single_lt_fit_probs, single_lt_fit_rates);

   return ds;
}

void print_fitted(ds_info const& ds)
{
   const double secs_in_a_hour = 3600;
   const double secs_in_a_month = 30 * 24 * secs_in_a_hour;

   ex_out << "### " << ds.name << std::endl;
   ex_out << "* Fitted Request Interarrival Time" << std::endl;
   ex_out << " - Mean (SD): " << boost::math::mean(ds.iat_he) << " (" << boost::math::standard_deviation(ds.iat_he) << ") seconds." << std::endl;
   ex_out << " - 99th Percentile: " << boost::math::quantile(ds.iat_he, 0.99) << " seconds." << std::endl;
   ex_out << " - Probability that a VM will arrive within 30 minutes: " << boost::math::cdf(ds.iat_he, secs_in_a_hour / 2.0) << std::endl;
   ex_out << " - Probability that a VM will arrive after 1 hour: " << boost::math::cdf(boost::math::complement(ds.iat_he, secs_in_a_hour)) << std::endl;
   ex_out << "* Fitted Multi-core VM Lifetime" << std::endl;
   ex_out << " - Mean (SD): " << boost::math::mean(ds.multi_lt_he) << " (" << boost::math::standard_deviation(ds.multi_lt_he) << ") seconds." << std::endl;
   ex_out << " - 99th Percentile: " << boost::math::quantile(ds.multi_lt_he, 0.99) << " seconds." << std::endl;
   ex_out << " - Probability that a VM will last for less than 1 month: " << boost::math::cdf(ds.multi_lt_he, secs_in_a_month) << std::endl;
   ex_out << " - Probability that a VM will last for more than 3 months: " << boost::math::cdf(boost::math::complement(ds.multi_lt_he, 3.0*secs_in_a_month)) << std::endl;
   ex_out << "* Fitted Single-core VM Lifetime" << std::endl;
   ex_out << " - Mean (SD): " << boost::math::mean(ds.single_lt_he) << " (" << boost::math::standard_deviation(ds.single_lt_he) << ") seconds." << std::endl;
   ex_out << " - 99th Percentile: " << boost::math::quantile(ds.single_lt_he, 0.99) << " seconds." << std::endl;
   ex_out << " - Probability that a VM will last for less than 1 month: " << boost::math::cdf(ds.single_lt_he, secs_in_a_month) << std::endl;
   ex_out << " - Probability that a VM will last for more than 3 months: " << boost::math::cdf(boost::math::complement(ds.single_lt_he, 3.0*secs_in_a_month)) << std::endl;
}

int example_main()
{
   print_fitted(make_ds1());

   print_fitted(make_ds2());

   print_fitted(make_ds3());
    return 0;  // main's implicit return
}
//]

TEST_CASE("example: hyperexponential_more_snips") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(### DS1
* Fitted Request Interarrival Time
 - Mean (SD): 2181.24 (8093.68) seconds.
 - 99th Percentile: 43146.6 seconds.
 - Probability that a VM will arrive within 30 minutes: 0.839079
 - Probability that a VM will arrive after 1 hour: 0.0916349
* Fitted Multi-core VM Lifetime
 - Mean (SD): 196540 (389130) seconds.
 - 99th Percentile: 1.81811e+06 seconds.
 - Probability that a VM will last for less than 1 month: 0.997873
 - Probability that a VM will last for more than 3 months: 6.68407e-08
* Fitted Single-core VM Lifetime
 - Mean (SD): 31915.2 (140375) seconds.
 - 99th Percentile: 744233 seconds.
 - Probability that a VM will last for less than 1 month: 0.999961
 - Probability that a VM will last for more than 3 months: 6.8933e-12
### DS2
* Fitted Request Interarrival Time
 - Mean (SD): 65334.7 (131656) seconds.
 - 99th Percentile: 610084 seconds.
 - Probability that a VM will arrive within 30 minutes: 0.515556
 - Probability that a VM will arrive after 1 hour: 0.403728
* Fitted Multi-core VM Lifetime
 - Mean (SD): 180059 (635264) seconds.
 - 99th Percentile: 3.29408e+06 seconds.
 - Probability that a VM will last for less than 1 month: 0.982464
 - Probability that a VM will last for more than 3 months: 0.000277218
* Fitted Single-core VM Lifetime
 - Mean (SD): 617449 (1.63524e+06) seconds.
 - 99th Percentile: 7.99055e+06 seconds.
 - Probability that a VM will last for less than 1 month: 0.913339
 - Probability that a VM will last for more than 3 months: 0.0108961
### DS3
* Fitted Request Interarrival Time
 - Mean (SD): 9669.14 (21518) seconds.
 - 99th Percentile: 106818 seconds.
 - Probability that a VM will arrive within 30 minutes: 0.53315
 - Probability that a VM will arrive after 1 hour: 0.355189
* Fitted Multi-core VM Lifetime
 - Mean (SD): 31912.5 (104267) seconds.
 - 99th Percentile: 539438 seconds.
 - Probability that a VM will last for less than 1 month: 1
 - Probability that a VM will last for more than 3 months: 1.93199e-18
* Fitted Single-core VM Lifetime
 - Mean (SD): 43092.6 (161327) seconds.
 - 99th Percentile: 843081 seconds.
 - Probability that a VM will last for less than 1 month: 0.999947
 - Probability that a VM will last for more than 3 months: 9.27287e-12
)ex"));
}
}  // namespace ex_hyperexponential_more_snips

// libs/math/example/inverse_chi_squared_example.cpp
namespace ex_inverse_chi_squared_example {
using boost::math::inverse_chi_squared_distribution;  // inverse_chi_squared_distribution.
using boost::math::inverse_chi_squared; //typedef for nverse_chi_squared_distribution double.

    using std::endl;
using std::setprecision;
using std::setw;
using std::sqrt;

template <class RealType>
RealType naive_pdf1(RealType df, RealType x)
{ // Formula from Wikipedia http://en.wikipedia.org/wiki/Inverse-chi-square_distribution
  // definition 1 using tgamma for simplicity as a check.
   using namespace std; // For ADL of std functions.
   using boost::math::tgamma;
   RealType df2 = df / 2;
   RealType result = (pow(2., -df2) * pow(x, (-df2 -1)) * exp(-1/(2 * x) ) )
      / tgamma(df2);  // 
   return result;
}

template <class RealType>
RealType naive_pdf2(RealType df, RealType x)
{ // Formula from Wikipedia http://en.wikipedia.org/wiki/Inverse-chi-square_distribution
  // Definition 2, using tgamma for simplicity as a check.
  // Not scaled, so assumes scale = 1 and only uses nu aka df.
   using namespace std; // For ADL of std functions.
   using boost::math::tgamma;
   RealType df2 = df / 2;
   RealType result = (pow(df2, df2) * pow(x, (-df2 -1)) * exp(-df/(2*x) ) )
     / tgamma(df2);
   return result;
}

template <class RealType>
RealType naive_pdf3(RealType df, RealType scale, RealType x)
{ // Formula from Wikipedia http://en.wikipedia.org/wiki/Scaled-inverse-chi-square_distribution
  // *Scaled* version, definition 3, df aka nu, scale aka sigma^2
  // using tgamma for simplicity as a check.
   using namespace std; // For ADL of std functions.
   using boost::math::tgamma;
   RealType df2 = df / 2;
   RealType result = (pow(scale * df2, df2) * exp(-df2 * scale/x) ) 
     / (tgamma(df2) * pow(x, 1+df2));
   return result;
}

template <class RealType>
RealType naive_pdf4(RealType df, RealType scale, RealType x)
{ // Formula from http://mathworld.wolfram.com/InverseChi-SquaredDistribution.html
  // Weisstein, Eric W. "Inverse Chi-Squared Distribution." From MathWorld--A Wolfram Web Resource.
  // *Scaled* version, definition 3, df aka nu, scale aka sigma^2
  // using tgamma for simplicity as a check.
   using namespace std; // For ADL of std functions.
   using boost::math::tgamma;
   RealType nu = df; // Wolfram uses greek symbols nu,
   RealType xi = scale; // and xi.
   RealType result = 
     pow(2, -nu/2) *  exp(- (nu * xi)/(2 * x)) * pow(x, -1-nu/2) * pow((nu * xi), nu/2) 
     / tgamma(nu/2);
   return result;
}

int example_main()
{

  ex_out << "Example (basic) using Inverse chi squared distribution. " << endl;

  // TODO find a more practical/useful example.  Suggestions welcome?

#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
  int max_digits10 = 2 + (boost::math::policies::digits<double, boost::math::policies::policy<> >() * 30103UL) / 100000UL;
  ex_out << "BOOST_NO_CXX11_NUMERIC_LIMITS is defined" << endl; 
#else 
  int max_digits10 = std::numeric_limits<double>::max_digits10;
#endif
  ex_out << "Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = "
    << max_digits10 << endl; 
  ex_out.precision(max_digits10); // 

  inverse_chi_squared ichsqdef; // All defaults  - not very useful!
  ex_out << "default df = " << ichsqdef.degrees_of_freedom()
    << ", default scale = " <<  ichsqdef.scale() << endl; //  default df = 1, default scale = 0.5

   inverse_chi_squared ichsqdef4(4); // Unscaled version, default scale = 1 / degrees_of_freedom
   ex_out << "default df = " << ichsqdef4.degrees_of_freedom()
    << ", default scale = " <<  ichsqdef4.scale() << endl; //  default df = 4, default scale = 2

   inverse_chi_squared ichsqdef32(3, 2); // Scaled version, both degrees_of_freedom and scale specified.
   ex_out << "default df = " << ichsqdef32.degrees_of_freedom()
    << ", default scale = " <<  ichsqdef32.scale() << endl; // default df = 3, default scale = 2

  {
    ex_out.precision(3);
    double nu = 5.;
    //double scale1 = 1./ nu; // 1st definition sigma^2 = 1/df;
    //double scale2 = 1.; // 2nd definition sigma^2 = 1
    inverse_chi_squared ichsq(nu, 1/nu); // Not scaled
    inverse_chi_squared sichsq(nu, 1/nu); // scaled

    ex_out << "nu = " << ichsq.degrees_of_freedom() << ", scale = " << ichsq.scale() << endl;

    int width = 8;

    ex_out << "     x        pdf      pdf1   pdf2  pdf(scaled)    pdf       pdf      cdf     cdf" << endl;
    for (double x = 0.0; x < 1.; x += 0.1)
    {
      ex_out 
        << setw(width) << x 
        << ' ' << setw(width) << pdf(ichsq, x) // unscaled
        << ' ' << setw(width) << naive_pdf1(nu,  x) // Wiki def 1 unscaled matches graph 
        << ' ' << setw(width) << naive_pdf2(nu,  x) // scale = 1 - 2nd definition.
        << ' ' << setw(width) << naive_pdf3(nu, 1/nu, x) // scaled 
        << ' ' << setw(width) << naive_pdf4(nu, 1/nu, x) // scaled 
        << ' ' << setw(width) << pdf(sichsq, x)  // scaled
        << ' ' << setw(width) << cdf(sichsq, x)  // scaled
        << ' ' << setw(width) << cdf(ichsq, x)  // unscaled
       << endl;
    }
  }

  ex_out.precision(max_digits10);

  inverse_chi_squared ichisq(2., 0.5);
  ex_out << "pdf(ichisq, 1.) = " << pdf(ichisq, 1.) << endl;
  ex_out << "cdf(ichisq, 1.) = " << cdf(ichisq, 1.) << endl;

  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:
 Example (basic) using Inverse chi squared distribution. 
  Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = 17
  default df = 1, default scale = 1
  default df = 4, default scale = 0.25
  default df = 3, default scale = 2
  nu = 5, scale = 0.2
       x        pdf      pdf1   pdf2  pdf(scaled)    pdf       pdf      cdf     cdf
         0        0    -1.#J    -1.#J    -1.#J    -1.#J        0        0        0
       0.1     2.83     2.83 3.26e-007     2.83     2.83     2.83   0.0752   0.0752
       0.2     3.05     3.05  0.00774     3.05     3.05     3.05    0.416    0.416
       0.3      1.7      1.7    0.121      1.7      1.7      1.7    0.649    0.649
       0.4    0.941    0.941    0.355    0.941    0.941    0.941    0.776    0.776
       0.5    0.553    0.553    0.567    0.553    0.553    0.553    0.849    0.849
       0.6    0.345    0.345    0.689    0.345    0.345    0.345    0.893    0.893
       0.7    0.227    0.227    0.728    0.227    0.227    0.227    0.921    0.921
       0.8    0.155    0.155    0.713    0.155    0.155    0.155     0.94     0.94
       0.9     0.11     0.11    0.668     0.11     0.11     0.11    0.953    0.953
         1   0.0807   0.0807     0.61   0.0807   0.0807   0.0807    0.963    0.963
  pdf(ichisq, 1.) = 0.30326532985631671
  cdf(ichisq, 1.) = 0.60653065971263365


*/

TEST_CASE("example: inverse_chi_squared_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example (basic) using Inverse chi squared distribution. 
Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = 17
default df = 1, default scale = 1
default df = 4, default scale = 0.25
default df = 3, default scale = 2
nu = 5, scale = 0.2
     x        pdf      pdf1   pdf2  pdf(scaled)    pdf       pdf      cdf     cdf
       0        0     -nan     -nan     -nan     -nan        0        0        0
     0.1     2.83     2.83 3.26e-07     2.83     2.83     2.83   0.0752   0.0752
     0.2     3.05     3.05  0.00774     3.05     3.05     3.05    0.416    0.416
     0.3      1.7      1.7    0.121      1.7      1.7      1.7    0.649    0.649
     0.4    0.941    0.941    0.355    0.941    0.941    0.941    0.776    0.776
     0.5    0.553    0.553    0.567    0.553    0.553    0.553    0.849    0.849
     0.6    0.345    0.345    0.689    0.345    0.345    0.345    0.893    0.893
     0.7    0.227    0.227    0.728    0.227    0.227    0.227    0.921    0.921
     0.8    0.155    0.155    0.713    0.155    0.155    0.155     0.94     0.94
     0.9     0.11     0.11    0.668     0.11     0.11     0.11    0.953    0.953
       1   0.0807   0.0807     0.61   0.0807   0.0807   0.0807    0.963    0.963
pdf(ichisq, 1.) = 0.30326532985631671
cdf(ichisq, 1.) = 0.60653065971263342
)ex"));
}
}  // namespace ex_inverse_chi_squared_example

// libs/math/example/jacobi_zeta_example.cpp
namespace ex_jacobi_zeta_example {
#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
#  error "This example requires a C++ compiler that supports C++11 numeric_limits. Try C++11 or later."
#endif



int example_main()
{
  try
  {
    ex_out.precision(std::numeric_limits<double>::max_digits10); // Show all potentially significant digits.
    ex_out.setf(std::ios_base::showpoint); // Include any significant trailing zeros.

    using boost::math::jacobi_zeta; // jacobi_zeta(T1 k, T2 phi) |k| <=1, k = sqrt(m)
    using boost::multiprecision::cpp_bin_float_50;

    // Wolfram Mathworld function JacobiZeta[phi, m] where m = k^2
    // JacobiZeta[phi,m] gives the Jacobi zeta function Z(phi | m)

    // If phi = 2, and elliptic modulus k = 0.9 so m = 0.9 * 0.9 = 0.81

    // https://reference.wolfram.com/language/ref/JacobiZeta.html // Function information.
    // A simple computation using phi = 2. and m = 0.9 * 0.9
    // JacobiZeta[2, 0.9 * 0.9]
    // https://www.wolframalpha.com/input/?i=JacobiZeta%5B2,+0.9+*+0.9%5D
    // -0.248584...
    // To get the expected 17 decimal digits precision for a 64-bit double type,
    // we need to ask thus:
    // N[JacobiZeta[2, 0.9 * 0.9],17]
    // https://www.wolframalpha.com/input/?i=N%5BJacobiZeta%5B2,+0.9+*+0.9%5D,17%5D

    double k = 0.9;
    double m = k * k;
    double phi = 2.;

    ex_out << "m = k^2 = " << m << std::endl;  // m = k^2 =  0.81000000000000005
    ex_out << "jacobi_zeta(" << k << ", " << phi << " )  = " << jacobi_zeta(k, phi) << std::endl;
    // jacobi_zeta(0.90000000000000002, 2.0000000000000000 )  =
    //  -0.24858442708494899  Boost.Math
    //  -0.24858442708494893  Wolfram
    // that agree within the expected precision of 17 decimal digits for 64-bit type double.

    // We can also easily get a higher precision too:
    // For example, to get 50 decimal digit precision using WolframAlpha:
    // N[JacobiZeta[2, 0.9 * 0.9],50]
    // https://www.wolframalpha.com/input/?i=N%5BJacobiZeta%5B2,+0.9+*+0.9%5D,50%5D
    // -0.24858442708494893408462856109734087389683955309853

    // Using Boost.Multiprecision we can do them same almost as easily.

    // To check that we are not losing precision, we show all the significant digits of the arguments ad result:
    ex_out.precision(std::numeric_limits<cpp_bin_float_50>::digits10); // Show all significant digits.

    // We can force the computation to use 50 decimal digit precision thus:
    cpp_bin_float_50 k50("0.9");
    cpp_bin_float_50 phi50("2.");

    ex_out << "jacobi_zeta(" << k50 << ", " << phi50 << " )  = " << jacobi_zeta(k50, phi50) << std::endl;
    // jacobi_zeta(0.90000000000000000000000000000000000000000000000000,
    //   2.0000000000000000000000000000000000000000000000000 )
    //   = -0.24858442708494893408462856109734087389683955309853

    // and a comparison with Wolfram shows agreement to the expected precision.
    // -0.24858442708494893408462856109734087389683955309853  Boost.Math
    // -0.24858442708494893408462856109734087389683955309853  Wolfram

    // Taking care not to fall into the awaiting pit, we ensure that ALL arguments passed are of the
    // appropriate 50-digit precision and do NOT suffer from precision reduction to that of type double,
    // We do NOT write:
    ex_out << "jacobi_zeta<cpp_bin_float_50>(0.9, 2.)  = " << jacobi_zeta<cpp_bin_float_50>(0.9, 2) << std::endl;
    // jacobi_zeta(0.90000000000000000000000000000000000000000000000000,
    //   2.0000000000000000000000000000000000000000000000000 )
    //   = -0.24858442708494895921459900494815797085727097762164  << Wrong at about 17th digit!
    //     -0.24858442708494893408462856109734087389683955309853  Wolfram
  }
  catch (std::exception const& ex)
  {
    // Lacking try&catch blocks, the program will abort after any throw, whereas the
    // message below from the thrown exception will give some helpful clues as to the cause of the problem.
    ex_out << "\n""Message from thrown exception was:\n   " << ex.what() << std::endl;
    // An example of message:
    // ex_out << " = " << jacobi_zeta(2, 0.5) << std::endl;
    // Message from thrown exception was:
    // Error in function boost::math::ellint_k<long double>(long double) : Got k = 2, function requires |k| <= 1
    // Shows that first parameter is k and is out of range, as the definition in docs jacobi_zeta(T1 k, T2 phi);
  }
    return 0;  // main's implicit return
} // int example_main()

TEST_CASE("example: jacobi_zeta_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(m = k^2 = 0.81000000000000005
jacobi_zeta(0.90000000000000002, 2.0000000000000000 )  = -0.24858442708494896
jacobi_zeta(0.90000000000000000000000000000000000000000000000000, 2.0000000000000000000000000000000000000000000000000 )  = -0.24858442708494893408462856109734087389683955309853
jacobi_zeta<cpp_bin_float_50>(0.9, 2.)  = -0.24858442708494895921459900494815797085727097762164
)ex"));
}
}  // namespace ex_jacobi_zeta_example

// libs/math/example/lambert_w_simple_examples.cpp
namespace ex_lambert_w_simple_examples {
using boost::multiprecision::cpp_dec_float_50; // 50 decimal digits type.
using boost::multiprecision::cpp_dec_float_100; // 100 decimal digits type.
using boost::multiprecision::backends::cpp_dec_float;
using boost::multiprecision::number;
typedef number<cpp_dec_float<1000> > cpp_dec_float_1000; // 1000 decimal digit types

using boost::multiprecision::cpp_bin_float_double; // == double
using boost::multiprecision::cpp_bin_float_double_extended; // 80-bit long double emulation.
using boost::multiprecision::cpp_bin_float_quad; // 128-bit quad precision.

//[lambert_w_simple_examples_includes

using boost::math::lambert_w0;
using boost::math::lambert_wm1;
//] //[/lambert_w_simple_examples_includes]

// 
// using std::endl;

//! Show value of z to the full possibly-significant max_digits10 precision of type T.
template<typename T>
void show_value(T z)
{
  std::streamsize precision = ex_out.precision(std::numeric_limits<T>::max_digits10);  // Save.
  ex_out.precision(std::numeric_limits<T>::max_digits10); // Show all possibly significant digits.
  std::ios::fmtflags flags(ex_out.flags());
  ex_out.setf(std::ios_base::showpoint); // Include any trailing zeros.
  ex_out << z;
  // Restore:
  ex_out.precision(precision);
  ex_out.flags(flags);
} // template<typename T> void show_value(T z)

int example_main()
{
  try
  {
    ex_out << "Lambert W simple examples." << std::endl;

    using boost::math::constants::exp_minus_one; //-1/e, the branch point, a singularity ~= -0.367879.

    // using statements needed for changing error handling policy.
    using boost::math::policies::policy;
    using boost::math::policies::make_policy;
    using boost::math::policies::evaluation_error;
    using boost::math::policies::domain_error;
    using boost::math::policies::overflow_error;
    using boost::math::policies::ignore_error;
    using boost::math::policies::throw_on_error;

  {
//[lambert_w_simple_examples_0
    ex_out.precision(std::numeric_limits<double>::max_digits10);
    // Show all potentially significant decimal digits,
    ex_out << std::showpoint << std::endl;
    // and show significant trailing zeros too.

    double z = 10.;
    double r = lambert_w0(z); // Default policy for double.
    ex_out << "lambert_w0(z) = " << r << std::endl;
    // lambert_w0(z) = 1.7455280027406994
//] [/lambert_w_simple_examples_0]
  }
  {
    // Other floating-point types can be used too, here float.
    // It is convenient to use a function like `show_value`
    // to display all potentially significant decimal digits
    // for the type, including any significant trailing zeros.
    //[lambert_w_simple_examples_1
    float z = 10.F;
    float r;
    r = lambert_w0(z);        // Default policy digits10 = 7, digits2 = 24
    ex_out << "lambert_w0(";
    show_value(z);
    ex_out << ") = ";
    show_value(r);
    ex_out << std::endl;   // lambert_w0(10.0000000) = 1.74552798
   //] //[/lambert_w_simple_examples_1]
  }
   {
     // Example of an integer argument to lambert_w,
     // showing that an integer is correctly promoted to a double.
//[lambert_w_simple_examples_2
     ex_out.precision(std::numeric_limits<double>::max_digits10);
     double r = lambert_w0(10);                           // Pass an int argument "10" that should be promoted to double argument.
     ex_out << "lambert_w0(10) = " << r << std::endl;  // lambert_w0(10) = 1.7455280027406994
     double rp = lambert_w0(10);
     ex_out << "lambert_w0(10) = " << rp << std::endl;
     // lambert_w0(10) = 1.7455280027406994
     auto rr = lambert_w0(10);                            // C++11 needed.
     ex_out << "lambert_w0(10) = " << rr << std::endl;
     // lambert_w0(10) = 1.7455280027406994 too, showing that rr has been promoted to double.
//] //[/lambert_w_simple_examples_2]
   }
   {
     // Using multiprecision types to get much higher precision is painless.
     //[lambert_w_simple_examples_3
     cpp_dec_float_50 z("10");
     // Note construction using a decimal digit string "10",
     // NOT a floating-point double literal 10.
     cpp_dec_float_50 r;
     r = lambert_w0(z);
     ex_out << "lambert_w0("; show_value(z); ex_out << ") = ";
     show_value(r);
     ex_out << std::endl;
     // lambert_w0(10.000000000000000000000000000000000000000000000000000000000000000000000000000000) =
     //   1.7455280027406993830743012648753899115352881290809413313533156980404446940000000
     //] //[/lambert_w_simple_examples_3]
   }
   // Using multiprecision types to get multiprecision precision wrong!
   {
     //[lambert_w_simple_examples_4
     cpp_dec_float_50 z(0.7777777777777777777777777777777777777777777777777777777777777777777777777);
     // Compiler evaluates the nearest double-precision binary representation,
     // from the max_digits10 of the floating_point literal double 0.7777777777777777777777777777...,
     // so any extra digits in the multiprecision type
     // beyond max_digits10 (usually 17) are random and meaningless.
     cpp_dec_float_50 r;
     r = lambert_w0(z);
     ex_out << "lambert_w0(";
     show_value(z);
     ex_out << ") = "; show_value(r);
     ex_out << std::endl;
     // lambert_w0(0.77777777777777779011358916250173933804035186767578125000000000000000000000000000)
     //   = 0.48086152073210493501934682309060873341910109230469724725005039758139532631901386
     //] //[/lambert_w_simple_examples_4]
   }
   {
     //[lambert_w_simple_examples_4a
     cpp_dec_float_50 z(0.9); // Construct from floating_point literal double 0.9.
     cpp_dec_float_50 r;
     r = lambert_w0(0.9);
     ex_out << "lambert_w0(";
     show_value(z);
     ex_out << ") = "; show_value(r);
     ex_out << std::endl;
     // lambert_w0(0.90000000000000002220446049250313080847263336181640625000000000000000000000000000)
     //   = 0.52983296563343440510607251781038939952850341796875000000000000000000000000000000
     ex_out << "lambert_w0(0.9) = " << lambert_w0(static_cast<double>(0.9))
     // lambert_w0(0.9)
     //   = 0.52983296563343441
       << std::endl;
     //] //[/lambert_w_simple_examples_4a]
   }
   {
     // Using multiprecision types to get multiprecision precision right!
     //[lambert_w_simple_examples_4b
     cpp_dec_float_50 z("0.9");     // Construct from decimal digit string.
     cpp_dec_float_50 r;
     r = lambert_w0(z);
     ex_out << "lambert_w0(";
     show_value(z);
     ex_out << ") = "; show_value(r);
     ex_out << std::endl;
     // 0.90000000000000000000000000000000000000000000000000000000000000000000000000000000)
     // = 0.52983296563343441213336643954546304857788132269804249284012528304239956413801252
     //] //[/lambert_w_simple_examples_4b]
   }
   // Getting extreme precision (1000 decimal digits) Lambert W values.
   {
     ex_out.precision(std::numeric_limits<cpp_dec_float_1000>::digits10);
     cpp_dec_float_1000 z("2.0");
     cpp_dec_float_1000 r;
     r = lambert_w0(z);
     ex_out << "lambert_w0(z) = " << r << std::endl;
     // 0.8526055020137254913464724146953174668984533001514035087721073946525150656742630448965773783502494847334503972691804119834761668851953598826198984364998343940330324849743119327028383008883133161249045727544669202220292076639777316648311871183719040610274221013237163543451621208284315007250267190731048119566857455987975973474411544571619699938899354169616378479326962044241495398851839432070255805880208619490399218130868317114428351234208216131218024303904457925834743326836272959669122797896855064630871955955318383064292191644322931561534814178034773896739684452724587331245831001449498844495771266728242975586931792421997636537572767708722190588748148949667744956650966402600446780664924889043543203483210769017254907808218556111831854276511280553252641907484685164978750601216344998778097446525021666473925144772131644151718261199915247932015387685261438125313159125475113124470774926288823525823567568542843625471594347837868505309329628014463491611881381186810879712667681285740515197493390563
     // Wolfram alpha command N[productlog[0, 2.0],1000] gives the identical result:
     // 0.8526055020137254913464724146953174668984533001514035087721073946525150656742630448965773783502494847334503972691804119834761668851953598826198984364998343940330324849743119327028383008883133161249045727544669202220292076639777316648311871183719040610274221013237163543451621208284315007250267190731048119566857455987975973474411544571619699938899354169616378479326962044241495398851839432070255805880208619490399218130868317114428351234208216131218024303904457925834743326836272959669122797896855064630871955955318383064292191644322931561534814178034773896739684452724587331245831001449498844495771266728242975586931792421997636537572767708722190588748148949667744956650966402600446780664924889043543203483210769017254907808218556111831854276511280553252641907484685164978750601216344998778097446525021666473925144772131644151718261199915247932015387685261438125313159125475113124470774926288823525823567568542843625471594347837868505309329628014463491611881381186810879712667681285740515197493390563
   }
   {
//[lambert_w_simple_examples_error_policies
      // Define an error handling policy:
      typedef policy<
        domain_error<throw_on_error>,
        overflow_error<ignore_error> // possibly unwise?
      > my_throw_policy;

      ex_out.precision(std::numeric_limits<double>::max_digits10);
      // Show all potentially significant decimal digits,
      ex_out << std::showpoint << std::endl;
      // and show significant trailing zeros too.
      double z = +1;
      ex_out << "Lambert W (" << z << ") = " << lambert_w0(z) << std::endl;
      // Lambert W (1.0000000000000000) = 0.56714329040978384
      ex_out << "\nLambert W (" << z << ", my_throw_policy()) = "
        << lambert_w0(z, my_throw_policy()) << std::endl;
      // Lambert W (1.0000000000000000, my_throw_policy()) = 0.56714329040978384
    //] //[/lambert_w_simple_example_error_policies]
    }
    {
      // Show error reporting from passing a value to lambert_wm1 that is out of range,
      // (and probably was meant to be passed to lambert_0 instead).
//[lambert_w_simple_examples_out_of_range
      double z = +1.;
      double r = lambert_wm1(z);
      ex_out << "lambert_wm1(+1.) = " << r << std::endl;
 //] [/lambert_w_simple_examples_out_of_range]
     // Error in function boost::math::lambert_wm1<RealType>(<RealType>):
      // Argument z = 1 is out of range (z <= 0) for Lambert W-1 branch! (Try Lambert W0 branch?)
    }
  }
  catch (std::exception& ex)
  {
    ex_out << ex.what() << std::endl;
  }
    return 0;  // main's implicit return
}  // int example_main()

   /*

   Output:
//[lambert_w_simple_examples_error_message_1
Error in function boost::math::lambert_wm1<RealType>(<RealType>):
Argument z = 1 is out of range (z <= 0) for Lambert W-1 branch! (Try Lambert W0 branch?)
//] [/lambert_w_simple_examples_error_message_1]

   */

TEST_CASE("example: lambert_w_simple_examples") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Lambert W simple examples.

lambert_w0(z) = 1.7455280027406994
lambert_w0(10.0000000) = 1.74552798
lambert_w0(10) = 1.7455280027406994
lambert_w0(10) = 1.7455280027406994
lambert_w0(10) = 1.7455280027406994
lambert_w0(10.000000000000000000000000000000000000000000000000000000000000000000000000000000) = 1.7455280027406993830743012648753899115352881290809413313533154369268588640000000
lambert_w0(0.77777777777777779011358916250173933804035186767578125000000000000000000000000000) = 0.48086152073210493501934682309060873341910109230469724725005039758139532604898201
lambert_w0(0.90000000000000002220446049250313080847263336181640625000000000000000000000000000) = 0.52983296563343440510607251781038939952850341796875000000000000000000000000000000
lambert_w0(0.9) = 0.52983296563343441
lambert_w0(0.90000000000000000000000000000000000000000000000000000000000000000000000000000000) = 0.52983296563343441213336643954546304857788132269804249284012528304239956358442077
lambert_w0(z) = 0.8526055020137254913464724146953174668984533001514035087721073946525150656742630448965773783502494847334503972691804119834761668851953598826198984364998343940330324849743119327028383008883133161249045727544669202220292076639777316648311871183719040610274221013237163543451621208284315007250267190731048119566857455987975973474411544571619699938899354169616378479326962044241495398851839432070255805880208619490399218130868317114428351234208216131218024303904457925834743326836272959669122797896855064630871955955318383064292191644322931561534814178034773896739684452724587331245831001449498844495771266728242975586931792421997636537572767708722190588748148949667744956650966402600446780664924889043543203483210769017254907808218556111831854276511280553252641907484685164978750601216344998778097446525021666473925144772131644151718261199915247932015387685261438125313159125475113124470774926288823525823567568542843625471594347837868505309329628014463491611881381186810879712667681285740515197493390563

Lambert W (1.0000000000000000) = 0.56714329040978384

Lambert W (1.0000000000000000, my_throw_policy()) = 0.56714329040978384
Error in function boost::math::lambert_wm1<RealType>(<RealType>): Argument z = 1 is out of range (z <= 0) for Lambert W-1 branch! (Try Lambert W0 branch?)
)ex"));
}
}  // namespace ex_lambert_w_simple_examples

// libs/math/example/lexical_cast_native.cpp
namespace ex_lexical_cast_native {
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
  ex_out << "lexical_cast example (NOT using finite_num_facet)." << std::endl;
    
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
  
  // Some tests that are expected to fail on some platforms.
  // (But these tests are expected to pass using non_finite num_put and num_get facets).

  // Use the current 'native' default locale.
  std::locale default_locale (std::locale::classic ()); // Note the current (default C) locale.

  // Create plus and minus infinity.
  double plus_infinity = +std::numeric_limits<double>::infinity();
  double minus_infinity = -std::numeric_limits<double>::infinity();

  // and create a NaN (NotANumber).
  double NaN = +std::numeric_limits<double>::quiet_NaN ();

  // Output the nonfinite values using the current (default C) locale.
  // The default representations differ from system to system,
  // for example, using Microsoft compilers, 1.#INF, -1.#INF, and 1.#QNAN.
  ex_out << "Using default locale" << endl;
  ex_out << "+std::numeric_limits<double>::infinity() = " << plus_infinity << endl;
  ex_out << "-std::numeric_limits<double>::infinity() = " << minus_infinity << endl;
  ex_out << "+std::numeric_limits<double>::quiet_NaN () = " << NaN << endl;
      
  // Checks below are expected to fail on some platforms!

  // Now try some 'round-tripping', 'reading' "inf"
  double x = boost::lexical_cast<double>("inf");
  // and check we get a floating-point infinity.
  BOOST_MATH_ASSERT(x == std::numeric_limits<double>::infinity());

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

from MSVC 10, fails (as expected)

  lexical_cast_native.vcxproj -> J:\Cpp\fp_facet\fp_facet\Debug\lexical_cast_native.exe
  lexical_cast example (NOT using finite_num_facet).
  Using default locale
  +std::numeric_limits<double>::infinity() = 1.#INF
  -std::numeric_limits<double>::infinity() = -1.#INF
  +std::numeric_limits<double>::quiet_NaN () = 1.#QNAN
C:\Program Files\MSBuild\Microsoft.Cpp\v4.0\Microsoft.CppCommon.targets(183,5): error MSB3073: The command ""J:\Cpp\fp_facet\fp_facet\Debug\lexical_cast_native.exe"
C:\Program Files\MSBuild\Microsoft.Cpp\v4.0\Microsoft.CppCommon.targets(183,5): error MSB3073: :VCEnd" exited with code 3.


*/

TEST_CASE("example: lexical_cast_native") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(lexical_cast example (NOT using finite_num_facet).
Using default locale
+std::numeric_limits<double>::infinity() = inf
-std::numeric_limits<double>::infinity() = -inf
+std::numeric_limits<double>::quiet_NaN () = nan
)ex"));
}
}  // namespace ex_lexical_cast_native

// libs/math/example/neg_binom_confidence_limits.cpp
namespace ex_neg_binom_confidence_limits {
using boost::math::negative_binomial;

 using std::endl;
using std::setprecision;
using std::setw; using std::left; using std::fixed; using std::right;

/*`
First define a table of significance levels: these are the 
probabilities that the true occurrence frequency lies outside the calculated
interval:
*/

  double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };

/*`
Confidence value as % is (1 - alpha) * 100, so alpha 0.05 == 95% confidence
that the true occurrence frequency lies *inside* the calculated interval.

We need a function to calculate and print confidence limits
for an observed frequency of occurrence 
that follows a negative binomial distribution.

*/

void confidence_limits_on_frequency(unsigned trials, unsigned successes)
{
   // trials = Total number of trials.
   // successes = Total number of observed successes.
   // failures = trials - successes.
   // success_fraction = successes /trials.
   // Print out general info:
   ex_out <<
      "______________________________________________\n"
      "2-Sided Confidence Limits For Success Fraction\n"
      "______________________________________________\n\n";
   ex_out << setprecision(7);
   ex_out << setw(40) << left << "Number of trials" << " =  " << trials << "\n";
   ex_out << setw(40) << left << "Number of successes" << " =  " << successes << "\n";
   ex_out << setw(40) << left << "Number of failures" << " =  " << trials - successes << "\n";
   ex_out << setw(40) << left << "Observed frequency of occurrence" << " =  " << double(successes) / trials << "\n";

   // Print table header:
   ex_out << "\n\n"
           "___________________________________________\n"
           "Confidence        Lower          Upper\n"
           " Value (%)        Limit          Limit\n"
           "___________________________________________\n";


/*`
And now for the important part - the bounds themselves.
For each value of /alpha/, we call `find_lower_bound_on_p` and 
`find_upper_bound_on_p` to obtain lower and upper bounds respectively. 
Note that since we are calculating a two-sided interval,
we must divide the value of alpha in two.  Had we been calculating a 
single-sided interval, for example: ['"Calculate a lower bound so that we are P%
sure that the true occurrence frequency is greater than some value"]
then we would *not* have divided by two.
*/

   // Now print out the upper and lower limits for the alpha table values.
   for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // Calculate bounds:
      double lower = negative_binomial::find_lower_bound_on_p(trials, successes, alpha[i]/2);
      double upper = negative_binomial::find_upper_bound_on_p(trials, successes, alpha[i]/2);
      // Print limits:
      ex_out << fixed << setprecision(5) << setw(15) << right << lower;
      ex_out << fixed << setprecision(5) << setw(15) << right << upper << endl;
   }
   ex_out << endl;
} // void confidence_limits_on_frequency(unsigned trials, unsigned successes)

/*`

And then call confidence_limits_on_frequency with increasing numbers of trials,
but always the same success fraction 0.1, or 1 in 10.

*/

int example_main()
{
  confidence_limits_on_frequency(20, 2); // 20 trials, 2 successes, 2 in 20, = 1 in 10 = 0.1 success fraction.
  confidence_limits_on_frequency(200, 20); // More trials, but same 0.1 success fraction.
  confidence_limits_on_frequency(2000, 200); // Many more trials, but same 0.1 success fraction.

  return 0;
    return 0;  // main's implicit return
} // int example_main()

//] [/negative_binomial_confidence_limits_eg end of Quickbook in C++ markup]

/*

______________________________________________
2-Sided Confidence Limits For Success Fraction
______________________________________________
Number of trials                         =  20
Number of successes                      =  2
Number of failures                       =  18
Observed frequency of occurrence         =  0.1
___________________________________________
Confidence        Lower          Upper
 Value (%)        Limit          Limit
___________________________________________
    50.000        0.04812        0.13554
    75.000        0.03078        0.17727
    90.000        0.01807        0.22637
    95.000        0.01235        0.26028
    99.000        0.00530        0.33111
    99.900        0.00164        0.41802
    99.990        0.00051        0.49202
    99.999        0.00016        0.55574
______________________________________________
2-Sided Confidence Limits For Success Fraction
______________________________________________
Number of trials                         =  200
Number of successes                      =  20
Number of failures                       =  180
Observed frequency of occurrence         =  0.1000000
___________________________________________
Confidence        Lower          Upper
 Value (%)        Limit          Limit
___________________________________________
    50.000        0.08462        0.11350
    75.000        0.07580        0.12469
    90.000        0.06726        0.13695
    95.000        0.06216        0.14508
    99.000        0.05293        0.16170
    99.900        0.04343        0.18212
    99.990        0.03641        0.20017
    99.999        0.03095        0.21664
______________________________________________
2-Sided Confidence Limits For Success Fraction
______________________________________________
Number of trials                         =  2000
Number of successes                      =  200
Number of failures                       =  1800
Observed frequency of occurrence         =  0.1000000
___________________________________________
Confidence        Lower          Upper
 Value (%)        Limit          Limit
___________________________________________
    50.000        0.09536        0.10445
    75.000        0.09228        0.10776
    90.000        0.08916        0.11125
    95.000        0.08720        0.11352
    99.000        0.08344        0.11802
    99.900        0.07921        0.12336
    99.990        0.07577        0.12795
    99.999        0.07282        0.13206
*/

TEST_CASE("example: neg_binom_confidence_limits") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(______________________________________________
2-Sided Confidence Limits For Success Fraction
______________________________________________

Number of trials                         =  20
Number of successes                      =  2
Number of failures                       =  18
Observed frequency of occurrence         =  0.1


___________________________________________
Confidence        Lower          Upper
 Value (%)        Limit          Limit
___________________________________________
    50.000        0.04812        0.13554
    75.000        0.03078        0.17727
    90.000        0.01807        0.22637
    95.000        0.01235        0.26028
    99.000        0.00530        0.33111
    99.900        0.00164        0.41802
    99.990        0.00051        0.49202
    99.999        0.00016        0.55574

______________________________________________
2-Sided Confidence Limits For Success Fraction
______________________________________________

Number of trials                         =  200
Number of successes                      =  20
Number of failures                       =  180
Observed frequency of occurrence         =  0.1000000


___________________________________________
Confidence        Lower          Upper
 Value (%)        Limit          Limit
___________________________________________
    50.000        0.08462        0.11350
    75.000        0.07580        0.12469
    90.000        0.06726        0.13695
    95.000        0.06216        0.14508
    99.000        0.05293        0.16170
    99.900        0.04343        0.18212
    99.990        0.03641        0.20017
    99.999        0.03095        0.21664

______________________________________________
2-Sided Confidence Limits For Success Fraction
______________________________________________

Number of trials                         =  2000
Number of successes                      =  200
Number of failures                       =  1800
Observed frequency of occurrence         =  0.1000000


___________________________________________
Confidence        Lower          Upper
 Value (%)        Limit          Limit
___________________________________________
    50.000        0.09536        0.10445
    75.000        0.09228        0.10776
    90.000        0.08916        0.11125
    95.000        0.08720        0.11352
    99.000        0.08344        0.11802
    99.900        0.07921        0.12336
    99.990        0.07577        0.12795
    99.999        0.07282        0.13206

)ex"));
}
}  // namespace ex_neg_binom_confidence_limits

// libs/math/example/neumann_zeros_example_1.cpp
namespace ex_neumann_zeros_example_1 {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif


//[neumann_zeros_example_1

/*`[h5 Calculating zeros of the Neumann function.]
This example also shows how Boost.Math and Boost.Multiprecision can be combined to provide
a many decimal digit precision. For 50 decimal digit precision we need to include
*/


/*`and a `typedef` for `float_type` may be convenient
(allowing a quick switch to re-compute at built-in `double` or other precision)
*/
  typedef boost::multiprecision::cpp_dec_float_50 float_type;

//`To use the functions for finding zeros of the `cyl_neumann` function we need:

//] [/neumann_zerso_example_1]

int example_main()
{
  try
  {
    {
//[neumann_zeros_example_2
/*`The Neumann (Bessel Y) function zeros are evaluated very similarly:
*/
    using boost::math::cyl_neumann_zero;
    double zn = cyl_neumann_zero(2., 1);
    ex_out << "cyl_neumann_zero(2., 1) = " << zn << std::endl;

    std::vector<float> nzeros(3); // Space for 3 zeros.
    cyl_neumann_zero<float>(2.F, 1, nzeros.size(), nzeros.begin());

    ex_out << "cyl_neumann_zero<float>(2.F, 1, ";
    // Print the zeros to the output stream.
    std::copy(nzeros.begin(), nzeros.end(),
              std::ostream_iterator<float>(ex_out, ", "));

    ex_out << "\n""cyl_neumann_zero(static_cast<float_type>(220)/100, 1) = " 
      << cyl_neumann_zero(static_cast<float_type>(220)/100, 1) << std::endl;
    // 3.6154383428745996706772556069431792744372398748422

//] //[/neumann_zeros_example_2]
    }
  }
  catch (std::exception const& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }
    return 0;  // main's implicit return
} // int example_main()

/*
 Output:

cyl_neumann_zero(2., 1) = 3.38424
cyl_neumann_zero<float>(2.F, 1,
3.38424
6.79381
10.0235
3.61544
*/

TEST_CASE("example: neumann_zeros_example_1") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(cyl_neumann_zero(2., 1) = 3.38424
cyl_neumann_zero<float>(2.F, 1, 3.38424, 6.79381, 10.0235, 
cyl_neumann_zero(static_cast<float_type>(220)/100, 1) = 3.61544
)ex"));
}
}  // namespace ex_neumann_zeros_example_1

// libs/math/example/nonfinite_loopback_ok.cpp
namespace ex_nonfinite_loopback_ok {
#ifdef _MSC_VER
#   pragma warning(disable : 4702)
#   pragma warning(disable : 4127) // conditional expression is constant.
#endif

using boost::math::nonfinite_num_get;
using boost::math::nonfinite_num_put;


using std::endl;

using std::locale;

using std::stringstream;
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
  //locale old_locale; // Current global locale.
  //  Create tmp_locale and store the output nonfinite_num_put facet in it.
  //locale tmp_locale(old_locale, new nonfinite_num_put<char>);
  //  Create new_locale and store the input nonfinite_num_get facet in it.
  //locale new_locale(tmp_locale, new nonfinite_num_get<char>);
  //  Can only add one facet at a time, hence need a tmp_locale,
  //  unless we write:

  std::locale new_locale(std::locale(std::locale(std::locale(),
    new boost::math::nonfinite_num_put<char>),
    new boost::math::nonfinite_num_get<char>));

  stringstream ss; // Both input and output, so need both get and put facets.

  ss.imbue(new_locale);

  double inf = numeric_limits<double>::infinity();
  ss << inf; // Write out.
  double r;
  ss >> r; // Read back in.

  BOOST_MATH_ASSERT(inf == r); // OK MSVC <= 10.0!

    return 0;  // main's implicit return
} // int example_main()

/*

Output:

nonfinite_loopback_ok.vcxproj -> J:\Cpp\fp_facet\fp_facet\Debug\nonfinite_loopback_ok.exe

*/

TEST_CASE("example: nonfinite_loopback_ok") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_nonfinite_loopback_ok

// libs/math/example/numerical_derivative_example.cpp
namespace ex_numerical_derivative_example {
#ifdef _MSC_VER
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif



//[numeric_derivative_example
/*`The following example shows how multiprecision calculations can be used to
obtain full precision in a numerical derivative calculation that suffers from precision loss.

Consider some well-known central difference rules for numerically
computing the 1st derivative of a function [f'(x)] with [/x] real.

Need a reference here?  Introduction to Partial Differential Equations, Peter J. Olver
 December 16, 2012 

Here, the implementation uses a C++ template that can be instantiated with various
floating-point types such as `float`, `double`, `long double`, or even
a user-defined floating-point type like __multiprecision.

We will now use the derivative template with the built-in type `double` in
order to numerically compute the derivative of a function, and then repeat
with a 5 decimal digit higher precision user-defined floating-point type. 

Consider the function  shown below.
!!
(3)
We will now take the derivative of this function with respect to x evaluated
at x = 3= 2. In other words,

(4)

The expected result is

 0:74535 59924 99929 89880 . (5)
The program below uses the derivative template in order to perform
the numerical calculation of this derivative. The program also compares the
numerically-obtained result with the expected result and reports the absolute
relative error scaled to a deviation that can easily be related to the number of
bits of lost precision.

*/

/*` [note Requires the C++11 feature of
[@http://en.wikipedia.org/wiki/Anonymous_function#C.2B.2B anonymous functions]
for the derivative function calls like `[]( const double & x_) -> double`.
*/



template <typename value_type,  typename function_type>
value_type derivative (const value_type x, const value_type dx, function_type function)
{
  /*! \brief Compute the derivative of function using a 3-point central difference rule of O(dx^6).
    \tparam value_type, floating-point type, for example: `double` or `cpp_dec_float_50`
    \tparam function_type  
    
    \param x Value at which to evaluate derivative.
    \param dx Incremental step-size.
    \param function Function whose derivative is to computed.
  
    \return derivative at x.
  */

  static_assert(false == std::numeric_limits<value_type>::is_integer, "value_type must be a floating-point type!");

  const value_type dx2(dx * 2U);
  const value_type dx3(dx * 3U);
  // Difference terms.
  const value_type m1 ((function (x + dx) - function(x - dx)) / 2U);
  const value_type m2 ((function (x + dx2) - function(x - dx2)) / 4U);
  const value_type m3 ((function (x + dx3) - function(x - dx3)) / 6U);
  const value_type fifteen_m1 (m1 * 15U);
  const value_type six_m2 (m2 * 6U);
  const value_type ten_dx (dx * 10U);
  return ((fifteen_m1 - six_m2) + m3) / ten_dx;  // Derivative.
} // 

  using boost::multiprecision::number;
  using boost::multiprecision::cpp_dec_float;

// Re-compute using 5 extra decimal digits precision (22) than double (17).
#define MP_DIGITS10 unsigned (std::numeric_limits<double>::max_digits10 + 5)

typedef cpp_dec_float<MP_DIGITS10> mp_backend;
typedef number<mp_backend> mp_type;


int example_main()
{
  {
    const double d =
      derivative
      ( 1.5, // x = 3.2
        std::ldexp (1., -9), // step size 2^-9 = see below for choice.
        [](const double & x)->double // Function f(x).
        {
          return std::sqrt((x * x) - 1.) - std::acos(1. / x);
        }
      );
  
    // The 'exactly right' result is [sqrt]5 / 3 = 0.74535599249992989880.
    const double rel_error = (d - 0.74535599249992989880) / 0.74535599249992989880;
    const double bit_error = std::abs(rel_error) / std::numeric_limits<double>::epsilon();
    ex_out.precision (std::numeric_limits<double>::digits10); // Show all guaranteed decimal digits.
    ex_out << std::showpoint ; // Ensure that any trailing zeros are shown too.

    ex_out << " derivative : " << d << std::endl;
    ex_out << " expected   : " << 0.74535599249992989880 << std::endl;
    // Can compute an 'exact' value using multiprecision type.
    ex_out << " expected   : " << sqrt(static_cast<mp_type>(5))/3U << std::endl;
    ex_out << " bit_error : " << static_cast<unsigned long>(bit_error)  << std::endl;

    ex_out.precision(6);
    ex_out << "float_distance = " << boost::math::float_distance(0.74535599249992989880, d) << std::endl;

  }

  { // Compute using multiprecision type with an extra 5 decimal digits of precision.
    const mp_type mp =
      derivative(mp_type(mp_type(3) / 2U), // x = 3/2
        mp_type(mp_type(1) / 10000000U), // Step size 10^7.
        [](const mp_type & x)->mp_type
        {
          return sqrt((x * x) - 1.) - acos (1. / x); // Function
        }
    );

    const double d = mp.convert_to<double>(); // Convert to closest double.
    const double rel_error = (d - 0.74535599249992989880) / 0.74535599249992989880;
    const double bit_error = std::abs (rel_error) / std::numeric_limits<double>::epsilon();
    ex_out.precision (std::numeric_limits <double>::digits10); // All guaranteed decimal digits.
    ex_out << std::showpoint ; // Ensure that any trailing zeros are shown too.
    ex_out << " derivative : " << d << std::endl;
    // Can compute an 'exact' value using multiprecision type.
    ex_out << " expected   : " << sqrt(static_cast<mp_type>(5))/3U << std::endl;
    ex_out << " expected   : " << 0.74535599249992989880
    << std::endl;
    ex_out << " bit_error : "  << static_cast<unsigned long>(bit_error)  << std::endl;

    ex_out.precision(6);
    ex_out << "float_distance = " << boost::math::float_distance(0.74535599249992989880, d) << std::endl;

    
  }


    return 0;  // main's implicit return
} // int example_main()

/*`
The result of this program on a system with an eight-byte, 64-bit IEEE-754
conforming floating-point representation for `double` is:

 derivative : 0.745355992499951

 derivative : 0.745355992499943
 expected   : 0.74535599249993
 bit_error : 78

    derivative : 0.745355992499930
   expected   : 0.745355992499930
   bit_error : 0

The resulting bit error is 0. This means that the result of the derivative
calculation is bit-identical with the double representation of the expected result,
and this is the best result possible for the built-in type.

The derivative in this example has a known closed form. There are, however,
countless situations in numerical analysis (and not only for numerical deriva-
tives) for which the calculation at hand does not have a known closed-form
solution or for which the closed-form solution is highly inconvenient to use. In
such cases, this technique may be useful.

This example has shown how multiprecision can be used to add extra digits
to an ill-conditioned calculation that suffers from precision loss. When the result
of the multiprecision calculation is converted to a built-in type such as double,
the entire precision of the result in double is preserved.

 */

/*

  Description: Autorun "J:\Cpp\big_number\Debug\numerical_derivative_example.exe"
   derivative : 0.745355992499943
   expected   : 0.745355992499930
   expected   : 0.745355992499930
   bit_error : 78
  float_distance = 117.000
   derivative : 0.745355992499930
   expected   : 0.745355992499930
   expected   : 0.745355992499930
   bit_error : 0
  float_distance = 0.000000

 */

TEST_CASE("example: numerical_derivative_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( derivative : 0.745355992499943
 expected   : 0.745355992499930
 expected   : 0.745355992499930
 bit_error : 78
float_distance = 117.000
 derivative : 0.745355992499930
 expected   : 0.745355992499930
 expected   : 0.745355992499930
 bit_error : 0
float_distance = 0.00000
)ex"));
}
}  // namespace ex_numerical_derivative_example

// libs/math/example/owens_t_example.cpp
namespace ex_owens_t_example {
#ifdef _MSC_VER
#  pragma warning(disable : 4127) // conditional expression is constant.
#endif


int example_main()
{
  double h = 0.0,a;
  ex_out << std::setprecision(20);
  
  static const double a_vec[] = {
    0.5000000000000000E+00,
    0.1000000000000000E+01,
    0.2000000000000000E+01,
    0.3000000000000000E+01,
    0.5000000000000000E+00,
    0.1000000000000000E+01,
    0.2000000000000000E+01,
    0.3000000000000000E+01,
    0.5000000000000000E+00,
    0.1000000000000000E+01,
    0.2000000000000000E+01,
    0.3000000000000000E+01,
    0.5000000000000000E+00,
    0.1000000000000000E+01,
    0.2000000000000000E+01,
    0.3000000000000000E+01,
    0.5000000000000000E+00,
    0.1000000000000000E+01,
    0.2000000000000000E+01,
    0.3000000000000000E+01,
    0.1000000000000000E+02,
    0.1000000000000000E+03 };

  static const double h_vec[] = {
    0.1000000000000000E+01,
    0.1000000000000000E+01,
    0.1000000000000000E+01,
    0.1000000000000000E+01,
    0.5000000000000000E+00,
    0.5000000000000000E+00,
    0.5000000000000000E+00,
    0.5000000000000000E+00,
    0.2500000000000000E+00,
    0.2500000000000000E+00,
    0.2500000000000000E+00,
    0.2500000000000000E+00,
    0.1250000000000000E+00,
    0.1250000000000000E+00,
    0.1250000000000000E+00,
    0.1250000000000000E+00,
    0.7812500000000000E-02,
    0.7812500000000000E-02,
    0.7812500000000000E-02,
    0.7812500000000000E-02,
    0.7812500000000000E-02,
    0.7812500000000000E-02 };

  static const double t_vec[] = {
    0.4306469112078537E-01,
    0.6674188216570097E-01,
    0.7846818699308410E-01,
    0.7929950474887259E-01,
    0.6448860284750376E-01,
    0.1066710629614485E+00,
    0.1415806036539784E+00,
    0.1510840430760184E+00,
    0.7134663382271778E-01,
    0.1201285306350883E+00,
    0.1666128410939293E+00,
    0.1847501847929859E+00,
    0.7317273327500385E-01,
    0.1237630544953746E+00,
    0.1737438887583106E+00,
    0.1951190307092811E+00,
    0.7378938035365546E-01,
    0.1249951430754052E+00,
    0.1761984774738108E+00,
    0.1987772386442824E+00,
    0.2340886964802671E+00,
    0.2479460829231492E+00 };

  for(unsigned i = 0; i != 22; ++i)
    {
      h = h_vec[i];
      a = a_vec[i];
      const double t = boost::math::owens_t(h, a);
      ex_out << "h=" << h << "\ta=" << a << "\tcomp="
        << t << "\ttab=" << t_vec[i]
        << "\tdiff=" << std::fabs(t_vec[i]-t) << std::endl;;
    }

  return 0;
    return 0;  // main's implicit return
}


// EOF

TEST_CASE("example: owens_t_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(h=1	a=0.5	comp=0.043064691120785363043	tab=0.043064691120785369982	diff=6.9388939039072283776e-18
h=1	a=1	comp=0.066741882165700969209	tab=0.066741882165700969209	diff=0
h=1	a=2	comp=0.078468186993084096392	tab=0.078468186993084096392	diff=0
h=1	a=3	comp=0.079299504748872581827	tab=0.079299504748872595705	diff=1.3877787807814456755e-17
h=0.5	a=0.5	comp=0.064488602847503753712	tab=0.064488602847503753712	diff=0
h=0.5	a=1	comp=0.10667106296144851507	tab=0.10667106296144850119	diff=1.3877787807814456755e-17
h=0.5	a=2	comp=0.14158060365397839853	tab=0.14158060365397839853	diff=0
h=0.5	a=3	comp=0.15108404307601841499	tab=0.15108404307601838723	diff=2.7755575615628913511e-17
h=0.25	a=0.5	comp=0.071346633822717775431	tab=0.071346633822717775431	diff=0
h=0.25	a=1	comp=0.12012853063508829865	tab=0.12012853063508829865	diff=0
h=0.25	a=2	comp=0.16661284109392934805	tab=0.16661284109392929254	diff=5.5511151231257827021e-17
h=0.25	a=3	comp=0.18475018479298593865	tab=0.1847501847929859109	diff=2.7755575615628913511e-17
h=0.125	a=0.5	comp=0.073172733275003842834	tab=0.073172733275003856712	diff=1.3877787807814456755e-17
h=0.125	a=1	comp=0.12376305449537457204	tab=0.12376305449537459979	diff=2.7755575615628913511e-17
h=0.125	a=2	comp=0.17374388875831062173	tab=0.17374388875831059398	diff=2.7755575615628913511e-17
h=0.125	a=3	comp=0.19511903070928104942	tab=0.19511903070928110493	diff=5.5511151231257827021e-17
h=0.0078125	a=0.5	comp=0.073789380353655453781	tab=0.073789380353655453781	diff=0
h=0.0078125	a=1	comp=0.12499514307540519897	tab=0.12499514307540519897	diff=0
h=0.0078125	a=2	comp=0.17619847747381081615	tab=0.1761984774738107884	diff=2.7755575615628913511e-17
h=0.0078125	a=3	comp=0.19877723864428234468	tab=0.19877723864428240019	diff=5.5511151231257827021e-17
h=0.0078125	a=10	comp=0.23408869648026708887	tab=0.23408869648026708887	diff=0
h=0.0078125	a=100	comp=0.24794608292314920805	tab=0.24794608292314920805	diff=0
)ex"));
}
}  // namespace ex_owens_t_example

// libs/math/example/policy_eg_3.cpp
namespace ex_policy_eg_3 {
#ifdef _MSC_VER
# pragma warning (disable : 4305) // 'initializing' : truncation from 'long double' to 'const eval_type'
# pragma warning (disable : 4244) //  conversion from 'long double' to 'const eval_type'
#endif

 using std::endl;

//[policy_eg_3

using boost::math::binomial_distribution;

// Begin by defining a policy type, that gives the behaviour we want:

//using namespace boost::math::policies; or explicitly
using boost::math::policies::policy;

using boost::math::policies::promote_float;
using boost::math::policies::discrete_quantile;
using boost::math::policies::integer_round_nearest;

typedef policy<
   promote_float<false>, // Do not promote to double.
   discrete_quantile<integer_round_nearest> // Round result to nearest integer.
> mypolicy;
//
// Then define a new distribution that uses it:
typedef boost::math::binomial_distribution<float, mypolicy> mybinom;

//  And now use it to get the quantile:

int example_main()
{
   ex_out << "quantile(mybinom(200, 0.25), 0.05) is: " <<
      quantile(mybinom(200, 0.25), 0.05) << endl;
    return 0;  // main's implicit return
}

//]

/*

Output:

  quantile(mybinom(200, 0.25), 0.05) is: 40

*/

TEST_CASE("example: policy_eg_3") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(quantile(mybinom(200, 0.25), 0.05) is: 40
)ex"));
}
}  // namespace ex_policy_eg_3

// libs/math/example/policy_eg_7.cpp
namespace ex_policy_eg_7 {
  using std::endl;

//[policy_eg_7

// using boost::math::normal; // Would create an ambiguity between
// boost::math::normal_distribution<RealType> boost::math::normal and
// 'anonymous-namespace'::normal'.

namespace
{ // anonymous or unnamed (rather than named as in policy_eg_6.cpp).

  using namespace boost::math::policies;
   // using boost::math::policies::errno_on_error; // etc.
  typedef policy<
     // return infinity and set errno rather than throw:
     overflow_error<errno_on_error>,
     // Don't promote double -> long double internally:
     promote_double<false>,
     // Return the closest integer result for discrete quantiles:
     discrete_quantile<integer_round_nearest>
  > my_policy;

  BOOST_MATH_DECLARE_DISTRIBUTIONS(double, my_policy)

} // close namespace my_namespace

int example_main()
{
   // Construct distribution with something we know will overflow.
   normal norm(10, 2); // using 'anonymous-namespace'::normal
   errno = 0;
   ex_out << "Result of quantile(norm, 0) is: " 
      << quantile(norm, 0) << endl;
   ex_out << "errno = " << errno << endl;
   errno = 0;
   ex_out << "Result of quantile(norm, 1) is: " 
      << quantile(norm, 1) << endl;
   ex_out << "errno = " << errno << endl;
   //
   // Now try a discrete distribution:
   binomial binom(20, 0.25);
   ex_out << "Result of quantile(binom, 0.05) is: " 
      << quantile(binom, 0.05) << endl;
   ex_out << "Result of quantile(complement(binom, 0.05)) is: " 
      << quantile(complement(binom, 0.05)) << endl;
    return 0;  // main's implicit return
}

//] //[/policy_eg_7]

/*

Output:

  Result of quantile(norm, 0) is: -1.#INF
  errno = 34
  Result of quantile(norm, 1) is: 1.#INF
  errno = 34
  Result of quantile(binom, 0.05) is: 1
  Result of quantile(complement(binom, 0.05)) is: 8

*/

TEST_CASE("example: policy_eg_7") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Result of quantile(norm, 0) is: -inf
errno = 34
Result of quantile(norm, 1) is: inf
errno = 34
Result of quantile(binom, 0.05) is: 1
Result of quantile(complement(binom, 0.05)) is: 8
)ex"));
}
}  // namespace ex_policy_eg_7

// libs/math/example/policy_ref_snip10.cpp
namespace ex_policy_ref_snip10 {
 using std::endl;

//[policy_ref_snip10

using boost::math::tgamma;

using namespace boost::math::policies;

double t = tgamma(12, policy<digits10<5> >());  // Concise make_policy.

//] //[/policy_ref_snip10]



int example_main()
{
   ex_out << "tgamma(12, policy<digits10<5> >())  = "<< t << endl;
    return 0;  // main's implicit return
}

/*

Output:

 tgamma(12, policy<digits10<5> >())  = 3.99168e+007

*/

TEST_CASE("example: policy_ref_snip10") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(tgamma(12, policy<digits10<5> >())  = 3.99168e+07
)ex"));
}
}  // namespace ex_policy_ref_snip10

// libs/math/example/policy_ref_snip2.cpp
namespace ex_policy_ref_snip2 {
  using std::endl;

//[policy_ref_snip2

using boost::math::normal_distribution;

using namespace boost::math::policies;

// Define a specific policy:
typedef policy<
      overflow_error<ignore_error>
      > my_policy;
      
// Define the distribution, using my_policy:
typedef normal_distribution<double, my_policy> my_norm;

// Construct a my_norm distribution, using default mean and standard deviation,
// and get a 0.05 or 5% quantile:
double q = quantile(my_norm(), 0.05); // = -1.64485

//] //[/policy_ref_snip2]

int example_main()
{
  my_norm n; // Construct a my_norm distribution,
  // using default mean zero and standard deviation unity.
  double q = quantile(n, 0.05); // and get a quantile.
  ex_out << "quantile(my_norm(), 0.05) = " << q << endl;
    return 0;  // main's implicit return
}

/*

Output:

  quantile(my_norm(), 0.05) = -1.64485
*/

TEST_CASE("example: policy_ref_snip2") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(quantile(my_norm(), 0.05) = -1.64485
)ex"));
}
}  // namespace ex_policy_ref_snip2

// libs/math/example/policy_ref_snip6.cpp
namespace ex_policy_ref_snip6 {
using boost::math::negative_binomial;

// Use the default rounding policy integer_round_outwards.
// Lower quantile rounded down:
double x = quantile(negative_binomial(20, 0.3), 0.05); // rounded up 27 from 27.3898
// Upper quantile rounded up:
double y = quantile(complement(negative_binomial(20, 0.3), 0.05)); // rounded down to 69 from 68.1584

//] //[/policy_ref_snip6]

 using std::endl;

int example_main()
{
   ex_out << "quantile(negative_binomial(20, 0.3), 0.05) = "<< x <<endl
     << "quantile(complement(negative_binomial(20, 0.3), 0.05)) = " << y << endl;
    return 0;  // main's implicit return
}

/*
Output:

  quantile(negative_binomial(20, 0.3), 0.05) = 27
  quantile(complement(negative_binomial(20, 0.3), 0.05)) = 69
*/

TEST_CASE("example: policy_ref_snip6") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(quantile(negative_binomial(20, 0.3), 0.05) = 27
quantile(complement(negative_binomial(20, 0.3), 0.05)) = 69
)ex"));
}
}  // namespace ex_policy_ref_snip6

// libs/math/example/polynomial_arithmetic.cpp
namespace ex_polynomial_arithmetic {
//] [polynomial_arithmetic_0



//[polynomial_arithmetic_1
/*`and some using statements are convenient:
*/

using std::string;
using std::exception;

using std::abs;
using std::pair;

using namespace boost::math;
using namespace boost::math::tools; // for polynomial
using boost::lexical_cast;

//] [/polynomial_arithmetic_1]

template <typename T>
string sign_str(T const &x)
{
  return x < 0 ? "-" : "+";
}

template <typename T>
string inner_coefficient(T const &x)
{
  string result(" " + sign_str(x) + " ");
  if (abs(x) != T(1))
      result += lexical_cast<string>(abs(x));
  return result;
}

/*! Output in formula format.
For example: from a polynomial in Boost container storage  [ 10, -6, -4, 3 ]
show as human-friendly formula notation: 3x^3 - 4x^2 - 6x + 10.
*/
template <typename T>
string formula_format(polynomial<T> const &a)
{
  string result;
  if (a.size() == 0)
      result += lexical_cast<string>(T(0));
  else
  {
    // First one is a special case as it may need unary negate.
    unsigned i = a.size() - 1;
    if (a[i] < 0)
        result += "-";
    if (abs(a[i]) != T(1))
        result += lexical_cast<string>(abs(a[i]));

    if (i > 0)
    {
      result += "x";
      if (i > 1)
      {
          result += "^" + lexical_cast<string>(i);
          i--;
          for (; i != 1; i--)
              if (a[i])
                result += inner_coefficient(a[i]) + "x^" + lexical_cast<string>(i);

          if (a[i])
            result += inner_coefficient(a[i]) + "x";
      }
      i--;

      if (a[i])
        result += " " + sign_str(a[i]) + " " + lexical_cast<string>(abs(a[i]));
    }
  }
  return result;
} // string formula_format(polynomial<T> const &a)


int example_main()
{
  ex_out << "Example: Polynomial arithmetic.\n\n";

  try
  {
//[polynomial_arithmetic_2
/*`Store the coefficients in a convenient way to access them,
then create some polynomials using construction from an iterator range,
and finally output in a 'pretty' formula format.

[tip Although we might conventionally write a polynomial from left to right
in descending order of degree, Boost.Math stores in [*ascending order of degree].]

  Read/write for humans:    3x^3 - 4x^2 - 6x + 10
  Boost polynomial storage: [ 10, -6, -4, 3 ]
*/
  std::array<double, 4> const d3a = {{10, -6, -4, 3}};
  polynomial<double> const a(d3a.begin(), d3a.end());

  // With C++11 and later, you can also use initializer_list construction.
  polynomial<double> const b{{-2.0, 1.0}};

  // formula_format() converts from Boost storage to human notation.
  ex_out << "a = " << formula_format(a)
  << "\nb = " << formula_format(b) << "\n\n";

//] [/polynomial_arithmetic_2]

//[polynomial_arithmetic_3
  // Now we can do arithmetic with the usual infix operators: + - * / and %.
  polynomial<double> s = a + b;
  ex_out << "a + b = " << formula_format(s) << "\n";
  polynomial<double> d = a - b;
  ex_out << "a - b = " << formula_format(d) << "\n";
  polynomial<double> p = a * b;
  ex_out << "a * b = " << formula_format(p) << "\n";
  polynomial<double> q = a / b;
  ex_out << "a / b = " << formula_format(q) << "\n";
  polynomial<double> r = a % b;
  ex_out << "a % b = " << formula_format(r) << "\n";
//] [/polynomial_arithmetic_3]

//[polynomial_arithmetic_4
/*`
Division is a special case where you can calculate two for the price of one.

Actually, quotient and remainder are always calculated together due to the nature
of the algorithm: the infix operators return one result and throw the other
away.

If you are doing a lot of division and want both the quotient and remainder, then
you don't want to do twice the work necessary.

In that case you can call the underlying function, [^quotient_remainder],
to get both results together as a pair.
*/
  pair< polynomial<double>, polynomial<double> > result;
  result = quotient_remainder(a, b);
// Reassure ourselves that the result is the same.
  BOOST_MATH_ASSERT(result.first == q);
  BOOST_MATH_ASSERT(result.second == r);
//] [/polynomial_arithmetic_4]
//[polynomial_arithmetic_5
  /* 
We can use the right and left shift operators to add and remove a factor of x.
This has the same semantics as left and right shift for integers where it is a 
factor of 2. x is the smallest prime factor of a polynomial as is 2 for integers.
*/
    ex_out << "Right and left shift operators.\n";
    ex_out << "\n" << formula_format(p) << "\n";
    ex_out << "... right shift by 1 ...\n";
    p >>= 1;
    ex_out << formula_format(p) << "\n";
    ex_out << "... left shift by 2 ...\n";
    p <<= 2;
    ex_out << formula_format(p) << "\n";    
  
/*
We can also give a meaning to odd and even for a polynomial that is consistent
with these operations: a polynomial is odd if it has a non-zero constant value, 
even otherwise. That is:
    x^2 + 1     odd
    x^2         even    
   */
    ex_out << std::boolalpha;
    ex_out << "\nPrint whether a polynomial is odd.\n";
    ex_out << formula_format(s) << "   odd? " << odd(s) << "\n";
    // We cheekily use the internal details to subtract the constant, making it even.
    s -= s.data().front();
    ex_out << formula_format(s) << "   odd? " << odd(s) << "\n";
    // And of course you can check if it is even:
    ex_out << formula_format(s) << "   even? " << even(s) << "\n";
    
    
    //] [/polynomial_arithmetic_5]
    //[polynomial_arithmetic_6]
    /* For performance and convenience, we can test whether a polynomial is zero 
     * by implicitly converting to bool with the same semantics as int.    */
    polynomial<double> zero; // Default construction is 0.
    ex_out << "zero: " << (zero ? "not zero" : "zero") << "\n";
    ex_out << "r: " << (r ? "not zero" : "zero") << "\n";
    /* We can also set a polynomial to zero without needing a another zero 
     * polynomial to assign to it. */
    r.set_zero();
    ex_out << "r: " << (r ? "not zero" : "zero") << "\n";    
    //] [/polynomial_arithmetic_6]
}
catch (exception const &e)
{
  ex_out << "\nMessage from thrown exception was:\n   " << e.what() << "\n";
}
return 0;
    return 0;  // main's implicit return
} // int example_main()

/*
//[polynomial_output_1

a = 3x^3 - 4x^2 - 6x + 10
b = x - 2

//] [/polynomial_output_1]


//[polynomial_output_2

a + b = 3x^3 - 4x^2 - 5x + 8
a - b = 3x^3 - 4x^2 - 7x + 12
a * b = 3x^4 - 10x^3 + 2x^2 + 22x - 20
a / b = 3x^2 + 2x - 2
a % b = 6

//] [/polynomial_output_2]

*/

TEST_CASE("example: polynomial_arithmetic") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example: Polynomial arithmetic.

a = 3x^3 - 4x^2 - 6x + 10
b = x - 2

a + b = 3x^3 - 4x^2 - 5x + 8
a - b = 3x^3 - 4x^2 - 7x + 12
a * b = 3x^4 - 10x^3 + 2x^2 + 22x - 20
a / b = 3x^2 + 2x - 2
a % b = 6
Right and left shift operators.

3x^4 - 10x^3 + 2x^2 + 22x - 20
... right shift by 1 ...
3x^3 - 10x^2 + 2x + 22
... left shift by 2 ...
3x^5 - 10x^4 + 2x^3 + 22x^2

Print whether a polynomial is odd.
3x^3 - 4x^2 - 5x + 8   odd? true
3x^3 - 4x^2 - 5x   odd? false
3x^3 - 4x^2 - 5x   even? true
zero: zero
r: not zero
r: zero
)ex"));
}
}  // namespace ex_polynomial_arithmetic

// libs/math/example/root_finding_fifth.cpp
namespace ex_root_finding_fifth {
using boost::math::policies::policy;
using boost::math::tools::newton_raphson_iterate;
using boost::math::tools::halley_iterate;
using boost::math::tools::eps_tolerance; // Binary functor for specified number of bits.
using boost::math::tools::bracket_and_solve_root;
using boost::math::tools::toms748_solve;



//] [/root_finding_headers]

 using std::endl;
using std::setw; using std::setprecision;
using std::numeric_limits;

/*
//[root_finding_fifth_1
Let's suppose we want to find the fifth root of a number.

The equation we want to solve is:

__spaces ['f](x) = x[fifth]

We will first solve this without using any information
about the slope or curvature of the fifth function.

If your differentiation is a little rusty
(or you are faced with an equation whose complexity is daunting,
then you can get help, for example from the invaluable

http://www.wolframalpha.com/ site

entering the command

  differentiate x^5

or the Wolfram Language command

  D[x^5, x]

gives the output

  d/dx(x^5) = 5 x^4

and to get the second differential, enter

  second differentiate x^5

or the Wolfram Language

  D[x^5, {x, 2}]

to get the output

  d^2/dx^2(x^5) = 20 x^3

or

  20 x^3

To get a reference value we can enter

  fifth root 3126

or

  N[3126^(1/5), 50]

to get a result with a precision of  50 decimal digits

  5.0003199590478625588206333405631053401128722314376

(We could also get a reference value using Boost.Multiprecision).

We then show how adding what we can know, for this function, about the slope,
the 1st derivation /f'(x)/, will speed homing in on the solution,
and then finally how adding the curvature /f''(x)/ as well will improve even more.

The 1st and 2nd derivatives of x[fifth] are:

__spaces ['f]\'(x) = 2x[sup2]

__spaces ['f]\'\'(x) = 6x

*/

//] [/root_finding_fifth_1]

//[root_finding_fifth_functor_noderiv

template <class T>
struct fifth_functor_noderiv
{ // fifth root of x using only function - no derivatives.
  fifth_functor_noderiv(T const& to_find_root_of) : value(to_find_root_of)
  { // Constructor stores value to find root of.
    // For example: calling fifth_functor<T>(x) to get fifth root of x.
  }
  T operator()(T const& x)
  { //! \returns f(x) - value.
    T fx = x*x*x*x*x - value; // Difference (estimate x^5 - value).
    return fx;
  }
private:
  T value; // to be 'fifth_rooted'.
};

//] [/root_finding_fifth_functor_noderiv]

//ex_out  << ", std::numeric_limits<" << typeid(T).name()  << ">::digits = " << digits
//   << ", accuracy " << get_digits << " bits."<< endl;


/*`Implementing the fifth root function itself is fairly trivial now:
the hardest part is finding a good approximation to begin with.
In this case we'll just divide the exponent by five.
(There are better but more complex guess algorithms used in 'real-life'.)

fifth root function is 'Really Well Behaved' in that it is monotonic
and has only one root
(we leave negative values 'as an exercise for the student').
*/

//[root_finding_fifth_noderiv

template <class T>
T fifth_noderiv(T x)
{ //! \returns fifth root of x using bracket_and_solve (no derivatives).
  using namespace std;  // Help ADL of std functions.
  using namespace boost::math::tools; // For bracket_and_solve_root.

  int exponent;
  frexp(x, &exponent); // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent / 5); // Rough guess is to divide the exponent by five.
  T factor = 2; // To multiply and divide guess to bracket.
  // digits used to control how accurate to try to make the result.
  // int digits =  3 * std::numeric_limits<T>::digits / 4; // 3/4 maximum possible binary digits accuracy for type T.
  int digits = std::numeric_limits<T>::digits; // Maximum possible binary digits accuracy for type T.

  //std::uintmax_t maxit = (std::numeric_limits<std::uintmax_t>::max)();
  // (std::numeric_limits<std::uintmax_t>::max)() = 18446744073709551615
  // which is more than anyone might wish to wait for!!!
  // so better to choose some reasonable estimate of how many iterations may be needed.

  const std::uintmax_t maxit = 50; // Chosen max iterations,
  // but updated on exit with actual iteration count.

  // We could also have used a maximum iterations provided by any policy:
  // std::uintmax_t max_it = policies::get_max_root_iterations<Policy>();

  std::uintmax_t it = maxit; // Initially our chosen max iterations,

  bool is_rising = true; // So if result if guess^5 is too low, try increasing guess.
  eps_tolerance<double> tol(digits);
  std::pair<T, T> r =
    bracket_and_solve_root(fifth_functor_noderiv<T>(x), guess, factor, is_rising, tol, it);
  // because the iteration count is updating,
  // you can't call with a literal maximum iterations value thus:
  //bracket_and_solve_root(fifth_functor_noderiv<T>(x), guess, factor, is_rising, tol, 20);

  // Can show how many iterations (this information is lost outside fifth_noderiv).
  ex_out << "Iterations " << it << endl;
  if (it >= maxit)
  { // Failed to converge (or is jumping between bracket values).
    ex_out << "Unable to locate solution in chosen iterations:"
      " Current best guess is between " << r.first << " and " << r.second << endl;
  }
  T distance = boost::math::float_distance(r.first, r.second);
  if (distance > 0)
  { //
    ex_out << distance << " bits separate the bracketing values." << std::endl;
    for (int i = 0; i < distance; i++)
    { // Show all the values within the bracketing values.
      ex_out << boost::math::float_advance(r.first, i) << std::endl;
    }
  }
  else
  { // distance == 0 and  r.second == r.first
    ex_out << "Converged to a single value " << r.first << std::endl;
  }

  return r.first + (r.second - r.first) / 2;  // return midway between bracketed interval.
} // T fifth_noderiv(T x)

//] [/root_finding_fifth_noderiv]



// maxit = 10
// Unable to locate solution in chosen iterations: Current best guess is between 3.0365889718756613 and 3.0365889718756627


/*`
We now solve the same problem, but using more information about the function,
to show how this can speed up finding the best estimate of the root.

For this function, the 1st differential (the slope of the tangent to a curve at any point) is known.

[@http://en.wikipedia.org/wiki/Derivative#Derivatives_of_elementary_functions Derivatives]
gives some reminders.

Using the rule that the derivative of x^n for positive n (actually all nonzero n) is nx^n-1,
allows use to get the 1st differential as 3x^2.

To see how this extra information is used to find the root, view this demo:
[@http://en.wikipedia.org/wiki/Newton%27s_methodNewton Newton-Raphson iterations].

We need to define a different functor that returns
both the evaluation of the function to solve, along with its first derivative:

To \'return\' two values, we use a pair of floating-point values:
*/

//[root_finding_fifth_functor_1stderiv

template <class T>
struct fifth_functor_1stderiv
{ // Functor returning function and 1st derivative.

  fifth_functor_1stderiv(T const& target) : value(target)
  { // Constructor stores the value to be 'fifth_rooted'.
  }

  std::pair<T, T> operator()(T const& z) // z is best estimate so far.
  { // Return both f(x) and first derivative f'(x).
    T fx = z*z*z*z*z - value; // Difference estimate fx = x^5 - value.
    T d1x = 5 * z*z*z*z; // 1st derivative d1x = 5x^4.
    return std::make_pair(fx, d1x); // 'return' both fx and d1x.
  }
private:
  T value; // to be 'fifth_rooted'.
}; // fifth_functor_1stderiv

//] [/root_finding_fifth_functor_1stderiv]


/*`Our fifth root function using fifth_functor_1stderiv is now:*/

//[root_finding_fifth_1deriv

template <class T>
T fifth_1deriv(T x)
{ //! \return fifth root of x using 1st derivative and Newton_Raphson.
  using namespace std; // For frexp, ldexp, numeric_limits.
  using namespace boost::math::tools; // For newton_raphson_iterate.

  int exponent;
  frexp(x, &exponent); // Get exponent of x (ignore mantissa).
  T guess = ldexp(1., exponent / 5); // Rough guess is to divide the exponent by three.
  // Set an initial bracket interval.
  T min = ldexp(0.5, exponent / 5); // Minimum possible value is half our guess.
  T max = ldexp(2., exponent / 5);// Maximum possible value is twice our guess.

  // digits used to control how accurate to try to make the result.
  int digits = std::numeric_limits<T>::digits; // Maximum possible binary digits accuracy for type T.

  const std::uintmax_t maxit = 20; // Optionally limit the number of iterations.
  std::uintmax_t it = maxit; // limit the number of iterations.
  //ex_out << "Max Iterations " << maxit << endl; //
  T result = newton_raphson_iterate(fifth_functor_1stderiv<T>(x), guess, min, max, digits, it);
  // Can check and show how many iterations (updated by newton_raphson_iterate).
  ex_out << it << " iterations (from max of " << maxit << ")" << endl;
  return result;
} // fifth_1deriv

//] [/root_finding_fifth_1deriv]

//  int get_digits = (digits * 2) /3; // Two thirds of maximum possible accuracy.

//std::uintmax_t maxit = (std::numeric_limits<std::uintmax_t>::max)();
// the default (std::numeric_limits<std::uintmax_t>::max)() = 18446744073709551615
// which is more than we might wish to wait for!!!  so we can reduce it

/*`
Finally need to define yet another functor that returns
both the evaluation of the function to solve,
along with its first and second derivatives:

f''(x) = 3 * 3x

To \'return\' three values, we use a tuple of three floating-point values:
*/

//[root_finding_fifth_functor_2deriv

template <class T>
struct fifth_functor_2deriv
{ // Functor returning both 1st and 2nd derivatives.
  fifth_functor_2deriv(T const& to_find_root_of) : value(to_find_root_of)
  { // Constructor stores value to find root of, for example:
  }

  // using boost::math::tuple; // to return three values.
  std::tuple<T, T, T> operator()(T const& x)
  { // Return both f(x) and f'(x) and f''(x).
    T fx = x*x*x*x*x - value; // Difference (estimate x^3 - value).
    T dx = 5 * x*x*x*x; // 1st derivative = 5x^4.
    T d2x = 20 * x*x*x; // 2nd derivative = 20 x^3
    return std::make_tuple(fx, dx, d2x); // 'return' fx, dx and d2x.
  }
private:
  T value; // to be 'fifth_rooted'.
}; // struct fifth_functor_2deriv

//] [/root_finding_fifth_functor_2deriv]


/*`Our fifth function is now:*/

//[root_finding_fifth_2deriv

template <class T>
T fifth_2deriv(T x)
{ // return fifth root of x using 1st and 2nd derivatives and Halley.
  using namespace std;  // Help ADL of std functions.
  using namespace boost::math; // halley_iterate

  int exponent;
  frexp(x, &exponent); // Get exponent of z (ignore mantissa).
  T guess = ldexp(1., exponent / 5); // Rough guess is to divide the exponent by three.
  T min = ldexp(0.5, exponent / 5); // Minimum possible value is half our guess.
  T max = ldexp(2., exponent / 5); // Maximum possible value is twice our guess.

  int digits = std::numeric_limits<T>::digits / 2; // Half maximum possible binary digits accuracy for type T.
  const std::uintmax_t maxit = 50;
  std::uintmax_t it = maxit;
  T result = halley_iterate(fifth_functor_2deriv<T>(x), guess, min, max, digits, it);
  // Can show how many iterations (updated by halley_iterate).
  ex_out << it << " iterations (from max of " << maxit << ")" << endl;

  return result;
} // fifth_2deriv(x)

//] [/root_finding_fifth_2deriv]

int example_main()
{

  //[root_finding_example_1
  ex_out << "fifth Root finding (fifth) Example." << endl;
  // Show all possibly significant decimal digits.
  ex_out.precision(std::numeric_limits<double>::max_digits10);
  // or use   ex_out.precision(max_digits10 = 2 + std::numeric_limits<double>::digits * 3010/10000);
  try
  { // Always use try'n'catch blocks with Boost.Math to get any error messages.

    double v27 = 3125; // Example of a value that has an exact integer fifth root.
    // exact value of fifth root is exactly 5.

    ex_out << "Fifth root  of " << v27 << " is " << 5 << std::endl;

    double v28 = v27+1; // Example of a value whose fifth root is *not* exactly representable.
    // Value of fifth root is 5.0003199590478625588206333405631053401128722314376 (50 decimal digits precision)
    // and to std::numeric_limits<double>::max_digits10 double precision (usually 17) is

    double root5v2 = static_cast<double>(5.0003199590478625588206333405631053401128722314376);

    ex_out << "Fifth root  of " << v28 << " is "  << root5v2 << std::endl;

    // Using bracketing:
    double r = fifth_noderiv(v27);
    ex_out << "fifth_noderiv(" << v27 << ") = " << r << endl;

    r = fifth_noderiv(v28);
    ex_out << "fifth_noderiv(" << v28 << ") = " << r << endl;

    // Using 1st differential Newton-Raphson:
    r = fifth_1deriv(v27);
    ex_out << "fifth_1deriv(" << v27 << ") = " << r << endl;
    r = fifth_1deriv(v28);
    ex_out << "fifth_1deriv(" << v28 << ") = " << r << endl;

    // Using Halley with 1st and 2nd differentials.
    r = fifth_2deriv(v27);
    ex_out << "fifth_2deriv(" << v27 << ") = " << r << endl;
    r = fifth_2deriv(v28);
    ex_out << "fifth_2deriv(" << v28 << ") = " << r << endl;
  }
  catch (const std::exception& e)
  { // Always useful to include try & catch blocks because default policies
    // are to throw exceptions on arguments that cause errors like underflow, overflow.
    // Lacking try & catch blocks, the program will abort without a message below,
    // which may give some helpful clues as to the cause of the exception.
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
  //] [/root_finding_example_1
  return 0;
    return 0;  // main's implicit return
}  // int example_main()

//[root_finding_example_output
/*`
Normal output is:

[pre
1>  Description: Autorun "J:\Cpp\MathToolkit\test\Math_test\Release\root_finding_fifth.exe"
1>  fifth Root finding (fifth) Example.
1>  Fifth root  of 3125 is 5
1>  Fifth root  of 3126 is 5.0003199590478626
1>  Iterations 10
1>  Converged to a single value 5
1>  fifth_noderiv(3125) = 5
1>  Iterations 11
1>  2 bits separate the bracketing values.
1>  5.0003199590478609
1>  5.0003199590478618
1>  fifth_noderiv(3126) = 5.0003199590478618
1>  6 iterations (from max of 20)
1>  fifth_1deriv(3125) = 5
1>  7 iterations (from max of 20)
1>  fifth_1deriv(3126) = 5.0003199590478626
1>  4 iterations (from max of 50)
1>  fifth_2deriv(3125) = 5
1>  4 iterations (from max of 50)
1>  fifth_2deriv(3126) = 5.0003199590478626
[/pre]

to get some (much!) diagnostic output we can add

#define BOOST_MATH_INSTRUMENT

[pre
1>  fifth Root finding (fifth) Example.
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:537 a = 4 b = 8 fa = -2101 fb = 29643 count = 18
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:340  a = 4.264742943548387 b = 8
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:352  a = 4.264742943548387 b = 5.1409225585147951
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:259  a = 4.264742943548387 b = 5.1409225585147951 d = 8 e = 4 fa = -1714.2037505671719 fb = 465.91652114644285 fd = 29643 fe = -2101
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:267 q11 = -3.735257056451613 q21 = -0.045655399937094755 q31 = 0.68893005658139972 d21 = -2.9047328414222999 d31 = -0.18724955838500826
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:275 q22 = -0.15074699539567221 q32 = 0.007740525571111408 d32 = -0.13385363287680208 q33 = 0.074868009790687237 c = 5.0362815354915851
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:388  a = 4.264742943548387 b = 5.0362815354915851
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:259  a = 4.264742943548387 b = 5.0362815354915851 d = 5.1409225585147951 e = 8 fa = -1714.2037505671719 fb = 115.03721886368339 fd = 465.91652114644285 fe = 29643
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:267 q11 = -0.045655399937094755 q21 = -0.034306988726112195 q31 = 0.7230181097615842 d21 = -0.1389480117493222 d31 = -0.048520482181613811
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:275 q22 = -0.00036345624935100459 q32 = 0.011175908093791367 d32 = -0.0030375853617102483 q33 = 0.00014618657296010219 c = 4.999083147976723
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:408  a = 4.999083147976723 b = 5.0362815354915851
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:433  a = 4.999083147976723 b = 5.0008904277935091
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:434  tol = -0.00036152225583956088
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:259  a = 4.999083147976723 b = 5.0008904277935091 d = 5.0362815354915851 e = 4.264742943548387 fa = -2.8641119933622576 fb = 2.7835781082976609 fd = 115.03721886368339 fe = -1714.2037505671719
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:267 q11 = -0.048520482181613811 q21 = -0.00087760104664616457 q31 = 0.00091652546535745522 d21 = -0.036268708744722128 d31 = -0.00089075435142862297
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:275 q22 = -1.9862562616034592e-005 q32 = 3.1952597740788757e-007 d32 = -1.2833778805050512e-005 q33 = 1.1763429980834706e-008 c = 5.0000000047314881
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:388  a = 4.999083147976723 b = 5.0000000047314881
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:259  a = 4.999083147976723 b = 5.0000000047314881 d = 5.0008904277935091 e = 5.0362815354915851 fa = -2.8641119933622576 fb = 1.4785900475544622e-005 fd = 2.7835781082976609 fe = 115.03721886368339
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:267 q11 = -0.00087760104664616457 q21 = -4.7298032238887272e-009 q31 = 0.00091685202154135855 d21 = -0.00089042779182425238 d31 = -4.7332236912279757e-009
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:275 q22 = -1.6486403607318402e-012 q32 = 1.7346209428817704e-012 d32 = -1.6858463963666777e-012 q33 = 9.0382569995250912e-016 c = 5
1>  I:\modular-boost\boost/math/tools/toms748_solve.hpp:592 max_iter = 10 count = 7
1>  Iterations 20
1>  0 bits separate brackets.
1>  fifth_noderiv(3125) = 5
]
*/
//] [/root_finding_example_output]

TEST_CASE("example: root_finding_fifth") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(fifth Root finding (fifth) Example.
Fifth root  of 3125 is 5
Fifth root  of 3126 is 5.0003199590478626
Iterations 10
Converged to a single value 5
fifth_noderiv(3125) = 5
Iterations 11
3 bits separate the bracketing values.
5.00031995904786
5.0003199590478609
5.0003199590478618
fifth_noderiv(3126) = 5.0003199590478609
7 iterations (from max of 20)
fifth_1deriv(3125) = 5
7 iterations (from max of 20)
fifth_1deriv(3126) = 5.0003199590478626
4 iterations (from max of 50)
fifth_2deriv(3125) = 5
4 iterations (from max of 50)
fifth_2deriv(3126) = 5.0003199590478626
)ex"));
}
}  // namespace ex_root_finding_fifth

// libs/math/example/students_t_example1.cpp
namespace ex_students_t_example1 {
double standard = 38.9;

const int values = 3;
double value[values] = {38.9, 37.4, 37.1};

// Is there any evidence for systematic error?

// The Students't distribution function is described at
// http://en.wikipedia.org/wiki/Student%27s_t_distribution
   using boost::math::students_t;  // Probability of students_t(df, t).

       using std::endl;
   using std::setprecision;
   using std::sqrt;

int example_main()
{
  ex_out << "Example 1 using Student's t function. " << endl;

  // Example/test using tabulated value
  // (deliberately coded as naively as possible).

  // Null hypothesis is that there is no difference (greater or less)
  // between measured and standard.

  double degrees_of_freedom = values-1; // 3-1 = 2
  ex_out << "Measurement 1 = " << value[0] << ", measurement 2 = " << value[1] << ", measurement 3 = " << value[2] << endl;
  double mean = (value[0] + value[1] + value[2]) / static_cast<double>(values);
  ex_out << "Standard = " << standard << ", mean = " << mean << ", (mean - standard) = " << mean - standard  << endl;
  double sd = sqrt(((value[0] - mean) * (value[0] - mean) + (value[1] - mean) * (value[1] - mean) + (value[2] - mean) * (value[2] - mean))/ static_cast<double>(values-1));
  ex_out << "Standard deviation = " << sd << endl;
  if (sd == 0.)
  {
      ex_out << "Measured mean is identical to SRM value," << endl;
      ex_out << "so probability of no difference between measured and standard (the 'null hypothesis') is unity." << endl;
      return 0;
  }

  double t = (mean - standard) * std::sqrt(static_cast<double>(values)) / sd;
  ex_out << "Student's t = " << t << endl;
  ex_out.precision(2); // Useful accuracy is only a few decimal digits.
  ex_out << "Probability of Student's t is " << cdf(students_t(degrees_of_freedom), std::abs(t)) << endl;
  //  0.91, is 1 tailed.
  // So there is insufficient evidence of a difference to meet a 95% (1 in 20) criterion.

  return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:

Example 1 using Student's t function. 
Measurement 1 = 38.9, measurement 2 = 37.4, measurement 3 = 37.1
Standard = 38.9, mean = 37.8, (mean - standard) = -1.1
Standard deviation = 0.964365
Student's t = -1.97566
Probability of Student's t is 0.91

*/

TEST_CASE("example: students_t_example1") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example 1 using Student's t function. 
Measurement 1 = 38.9, measurement 2 = 37.4, measurement 3 = 37.1
Standard = 38.9, mean = 37.8, (mean - standard) = -1.1
Standard deviation = 0.964365
Student's t = -1.97566
Probability of Student's t is 0.91
)ex"));
}
}  // namespace ex_students_t_example1

// libs/math/example/students_t_two_samples.cpp
namespace ex_students_t_two_samples {
#ifdef _MSC_VER
#  pragma warning(disable: 4512) // assignment operator could not be generated.
#  pragma warning(disable: 4510) // default constructor could not be generated.
#  pragma warning(disable: 4610) // can never be instantiated - user defined constructor required.
#endif

 using std::endl;
using std::left; using std::fixed; using std::right; using std::scientific;
using std::setw;
using std::setprecision;

   using boost::math::students_t;


void two_samples_t_test_equal_sd(
        double Sm1, // Sm1 = Sample Mean 1.
        double Sd1,   // Sd1 = Sample Standard Deviation 1.
        unsigned Sn1,   // Sn1 = Sample Size 1.
        double Sm2,   // Sm2 = Sample Mean 2.
        double Sd2,   // Sd2 = Sample Standard Deviation 2.
        unsigned Sn2,   // Sn2 = Sample Size 2.
        double alpha)   // alpha = Significance Level.
{
   // A Students t test applied to two sets of data.
   // We are testing the null hypothesis that the two
   // samples have the same mean and that any difference
   // if due to chance.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda353.htm
   //
   using namespace std;
   // using namespace boost::math;

   using boost::math::students_t;

   // Print header:
   ex_out <<
      "_______________________________________________\n"
      "Student t test for two samples (equal variances)\n"
      "_______________________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << setw(55) << left << "Number of Observations (Sample 1)" << "=  " << Sn1 << "\n";
   ex_out << setw(55) << left << "Sample 1 Mean" << "=  " << Sm1 << "\n";
   ex_out << setw(55) << left << "Sample 1 Standard Deviation" << "=  " << Sd1 << "\n";
   ex_out << setw(55) << left << "Number of Observations (Sample 2)" << "=  " << Sn2 << "\n";
   ex_out << setw(55) << left << "Sample 2 Mean" << "=  " << Sm2 << "\n";
   ex_out << setw(55) << left << "Sample 2 Standard Deviation" << "=  " << Sd2 << "\n";
   //
   // Now we can calculate and output some stats:
   //
   // Degrees of freedom:
   double v = Sn1 + Sn2 - 2;
   ex_out << setw(55) << left << "Degrees of Freedom" << "=  " << v << "\n";
   // Pooled variance and hence standard deviation:
   double sp = sqrt(((Sn1-1) * Sd1 * Sd1 + (Sn2-1) * Sd2 * Sd2) / v);
   ex_out << setw(55) << left << "Pooled Standard Deviation" << "=  " << sp << "\n";
   // t-statistic:
   double t_stat = (Sm1 - Sm2) / (sp * sqrt(1.0 / Sn1 + 1.0 / Sn2));
   ex_out << setw(55) << left << "T Statistic" << "=  " << t_stat << "\n";
   //
   // Define our distribution, and get the probability:
   //
   students_t dist(v);
   double q = cdf(complement(dist, fabs(t_stat)));
   ex_out << setw(55) << left << "Probability that difference is due to chance" << "=  "
      << setprecision(3) << scientific << 2 * q << "\n\n";
   //
   // Finally print out results of alternative hypothesis:
   //
   ex_out << setw(55) << left <<
      "Results for Alternative Hypothesis and alpha" << "=  "
      << setprecision(4) << fixed << alpha << "\n\n";
   ex_out << "Alternative Hypothesis              Conclusion\n";
   ex_out << "Sample 1 Mean != Sample 2 Mean       " ;
   if(q < alpha / 2)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Sample 1 Mean <  Sample 2 Mean       ";
   if(cdf(dist, t_stat) < alpha)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Sample 1 Mean >  Sample 2 Mean       ";
   if(cdf(complement(dist, t_stat)) < alpha)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << endl << endl;
}

void two_samples_t_test_unequal_sd(
        double Sm1,   // Sm1 = Sample Mean 1.
        double Sd1,   // Sd1 = Sample Standard Deviation 1.
        unsigned Sn1,   // Sn1 = Sample Size 1.
        double Sm2,   // Sm2 = Sample Mean 2.
        double Sd2,   // Sd2 = Sample Standard Deviation 2.
        unsigned Sn2,   // Sn2 = Sample Size 2.
        double alpha)   // alpha = Significance Level.
{
   // A Students t test applied to two sets of data.
   // We are testing the null hypothesis that the two
   // samples have the same mean and 
   // that any difference is due to chance.
   // See http://www.itl.nist.gov/div898/handbook/eda/section3/eda353.htm
   //
   using namespace std;
   using boost::math::students_t;

   // Print header:
   ex_out <<
      "_________________________________________________\n"
      "Student t test for two samples (unequal variances)\n"
      "_________________________________________________\n\n";
   ex_out << setprecision(5);
   ex_out << setw(55) << left << "Number of Observations (Sample 1)" << "=  " << Sn1 << "\n";
   ex_out << setw(55) << left << "Sample 1 Mean" << "=  " << Sm1 << "\n";
   ex_out << setw(55) << left << "Sample 1 Standard Deviation" << "=  " << Sd1 << "\n";
   ex_out << setw(55) << left << "Number of Observations (Sample 2)" << "=  " << Sn2 << "\n";
   ex_out << setw(55) << left << "Sample 2 Mean" << "=  " << Sm2 << "\n";
   ex_out << setw(55) << left << "Sample 2 Standard Deviation" << "=  " << Sd2 << "\n";
   //
   // Now we can calculate and output some stats:
   //
   // Degrees of freedom:
   double v = Sd1 * Sd1 / Sn1 + Sd2 * Sd2 / Sn2;
   v *= v;
   double t1 = Sd1 * Sd1 / Sn1;
   t1 *= t1;
   t1 /=  (Sn1 - 1);
   double t2 = Sd2 * Sd2 / Sn2;
   t2 *= t2;
   t2 /= (Sn2 - 1);
   v /= (t1 + t2);
   ex_out << setw(55) << left << "Degrees of Freedom" << "=  " << v << "\n";
   // t-statistic:
   double t_stat = (Sm1 - Sm2) / sqrt(Sd1 * Sd1 / Sn1 + Sd2 * Sd2 / Sn2);
   ex_out << setw(55) << left << "T Statistic" << "=  " << t_stat << "\n";
   //
   // Define our distribution, and get the probability:
   //
   students_t dist(v);
   double q = cdf(complement(dist, fabs(t_stat)));
   ex_out << setw(55) << left << "Probability that difference is due to chance" << "=  "
      << setprecision(3) << scientific << 2 * q << "\n\n";
   //
   // Finally print out results of alternative hypothesis:
   //
   ex_out << setw(55) << left <<
      "Results for Alternative Hypothesis and alpha" << "=  "
      << setprecision(4) << fixed << alpha << "\n\n";
   ex_out << "Alternative Hypothesis              Conclusion\n";
   ex_out << "Sample 1 Mean != Sample 2 Mean       " ;
   if(q < alpha / 2)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Sample 1 Mean <  Sample 2 Mean       ";
   if(cdf(dist, t_stat) < alpha)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << "Sample 1 Mean >  Sample 2 Mean       ";
   if(cdf(complement(dist, t_stat)) < alpha)
      ex_out << "NOT REJECTED\n";
   else
      ex_out << "REJECTED\n";
   ex_out << endl << endl;
}

int example_main()
{
   //
   // Run tests for Car Mileage sample data
   // http://www.itl.nist.gov/div898/handbook/eda/section3/eda3531.htm
   // from the NIST website http://www.itl.nist.gov.  The data compares
   // miles per gallon of US cars with miles per gallon of Japanese cars.
   //
   two_samples_t_test_equal_sd(20.14458, 6.414700, 249, 30.48101, 6.107710, 79, 0.05);
   two_samples_t_test_unequal_sd(20.14458, 6.414700, 249, 30.48101, 6.107710, 79, 0.05);

   return 0;
    return 0;  // main's implicit return
} // int example_main()

/*
Output is:

  _______________________________________________
  Student t test for two samples (equal variances)
  _______________________________________________
  
  Number of Observations (Sample 1)                      =  249
  Sample 1 Mean                                          =  20.145
  Sample 1 Standard Deviation                            =  6.4147
  Number of Observations (Sample 2)                      =  79
  Sample 2 Mean                                          =  30.481
  Sample 2 Standard Deviation                            =  6.1077
  Degrees of Freedom                                     =  326
  Pooled Standard Deviation                              =  6.3426
  T Statistic                                            =  -12.621
  Probability that difference is due to chance           =  5.273e-030
  
  Results for Alternative Hypothesis and alpha           =  0.0500
  
  Alternative Hypothesis              Conclusion
  Sample 1 Mean != Sample 2 Mean       NOT REJECTED
  Sample 1 Mean <  Sample 2 Mean       NOT REJECTED
  Sample 1 Mean >  Sample 2 Mean       REJECTED
  
  
  _________________________________________________
  Student t test for two samples (unequal variances)
  _________________________________________________
  
  Number of Observations (Sample 1)                      =  249
  Sample 1 Mean                                          =  20.14458
  Sample 1 Standard Deviation                            =  6.41470
  Number of Observations (Sample 2)                      =  79
  Sample 2 Mean                                          =  30.48101
  Sample 2 Standard Deviation                            =  6.10771
  Degrees of Freedom                                     =  136.87499
  T Statistic                                            =  -12.94627
  Probability that difference is due to chance           =  1.571e-025
  
  Results for Alternative Hypothesis and alpha           =  0.0500
  
  Alternative Hypothesis              Conclusion
  Sample 1 Mean != Sample 2 Mean       NOT REJECTED
  Sample 1 Mean <  Sample 2 Mean       NOT REJECTED
  Sample 1 Mean >  Sample 2 Mean       REJECTED
  


*/

TEST_CASE("example: students_t_two_samples") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(_______________________________________________
Student t test for two samples (equal variances)
_______________________________________________

Number of Observations (Sample 1)                      =  249
Sample 1 Mean                                          =  20.145
Sample 1 Standard Deviation                            =  6.4147
Number of Observations (Sample 2)                      =  79
Sample 2 Mean                                          =  30.481
Sample 2 Standard Deviation                            =  6.1077
Degrees of Freedom                                     =  326
Pooled Standard Deviation                              =  6.3426
T Statistic                                            =  -12.621
Probability that difference is due to chance           =  5.273e-30

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis              Conclusion
Sample 1 Mean != Sample 2 Mean       NOT REJECTED
Sample 1 Mean <  Sample 2 Mean       NOT REJECTED
Sample 1 Mean >  Sample 2 Mean       REJECTED


_________________________________________________
Student t test for two samples (unequal variances)
_________________________________________________

Number of Observations (Sample 1)                      =  249
Sample 1 Mean                                          =  20.14458
Sample 1 Standard Deviation                            =  6.41470
Number of Observations (Sample 2)                      =  79
Sample 2 Mean                                          =  30.48101
Sample 2 Standard Deviation                            =  6.10771
Degrees of Freedom                                     =  136.87499
T Statistic                                            =  -12.94627
Probability that difference is due to chance           =  1.571e-25

Results for Alternative Hypothesis and alpha           =  0.0500

Alternative Hypothesis              Conclusion
Sample 1 Mean != Sample 2 Mean       NOT REJECTED
Sample 1 Mean <  Sample 2 Mean       NOT REJECTED
Sample 1 Mean >  Sample 2 Mean       REJECTED


)ex"));
}
}  // namespace ex_students_t_two_samples
