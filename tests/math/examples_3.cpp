// mcpplibs.mwrap.boost.math — the official examples (libs/math/example/*.cpp, part 3 of 4),
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
#include <boost/math/tools/config.hpp>
#include <boost/assert.hpp>
#include <boost/math/tools/assert.hpp>
#include <boost/math/cstdfloat/cstdfloat_types.hpp>
#include <boost/math/special_functions/math_fwd.hpp>
#include <boost/math/distributions/fwd.hpp>
#include <boost/serialization/nvp.hpp>
#include <cassert>
#include <cmath>
#include <boost/config.hpp>
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

// libs/math/example/airy_zeros_example.cpp
namespace ex_airy_zeros_example {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif


// Weisstein, Eric W. "Bessel Function Zeros." From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/BesselFunctionZeros.html
// Test values can be calculated using [@wolframalpha.com WolframAplha]
// See also http://dlmf.nist.gov/10.21

//[airy_zeros_example_1

/*`This example demonstrates calculating zeros of the Airy functions.
It also shows how Boost.Math and Boost.Multiprecision can be combined to provide
a many decimal digit precision. For 50 decimal digit precision we need to include
*/


/*`and a `typedef` for `float_type` may be convenient
(allowing a quick switch to re-compute at built-in `double` or other precision)
*/
  typedef boost::multiprecision::cpp_dec_float_50 float_type;

//`To use the functions for finding zeros of the functions we need


/*`This example shows obtaining both a single zero of the Airy functions,
and then placing multiple zeros into a container like `std::vector` by providing an iterator.
The signature of the single-value Airy Ai function is:

  template <class T>
  T airy_ai_zero(unsigned m); // 1-based index of the zero.

The signature of multiple zeros Airy Ai function is:

  template <class T, class OutputIterator>
  OutputIterator airy_ai_zero(
                           unsigned start_index, // 1-based index of the zero.
                           unsigned number_of_zeros, // How many zeros to generate.
                           OutputIterator out_it); // Destination for zeros.

There are also versions which allows control of the __policy_section for error handling and precision.

  template <class T, class OutputIterator, class Policy>
  OutputIterator airy_ai_zero(
                           unsigned start_index, // 1-based index of the zero.
                           unsigned number_of_zeros, // How many zeros to generate.
                           OutputIterator out_it, // Destination for zeros.
                           const Policy& pol);  // Policy to use.
*/
//] [/airy_zeros_example_1]

int example_main()
{
  try
  {
//[airy_zeros_example_2

/*`[tip It is always wise to place code using Boost.Math inside `try'n'catch` blocks;
this will ensure that helpful error messages are shown when exceptional conditions arise.]

First, evaluate a single Airy zero.

The precision is controlled by the template parameter `T`,
so this example has `double` precision, at least 15 but up to 17 decimal digits
(for the common 64-bit double).
*/
    double aiz1 = boost::math::airy_ai_zero<double>(1);
    ex_out << "boost::math::airy_ai_zero<double>(1) = " << aiz1 << std::endl; 
    double aiz2 = boost::math::airy_ai_zero<double>(2);
    ex_out << "boost::math::airy_ai_zero<double>(2) = " << aiz2 << std::endl;
    double biz3 = boost::math::airy_bi_zero<double>(3);
    ex_out << "boost::math::airy_bi_zero<double>(3) = " << biz3 << std::endl;

/*`Other versions of `airy_ai_zero` and `airy_bi_zero`
allow calculation of multiple zeros with one call,
placing the results in a container, often `std::vector`.
For example, generate and display the first five `double` roots
[@http://mathworld.wolfram.com/AiryFunctionZeros.html Wolfram Airy Functions Zeros].
*/
    unsigned int n_roots = 5U;
    std::vector<double> roots;
    boost::math::airy_ai_zero<double>(1U, n_roots, std::back_inserter(roots));
    ex_out << "airy_ai_zeros:" << std::endl;
    std::copy(roots.begin(),
              roots.end(),
              std::ostream_iterator<double>(ex_out, "\n"));

/*`The first few real roots of Ai(x) are approximately -2.33811, -4.08795, -5.52056, -6.7867144, -7.94413, -9.02265 ...

Or we can use Boost.Multiprecision to generate 50 decimal digit roots.

We set the precision of the output stream, and show trailing zeros to display a fixed 50 decimal digits.
*/
    ex_out.precision(std::numeric_limits<float_type>::digits10); // float_type has 50 decimal digits.
    ex_out << std::showpoint << std::endl; // Show trailing zeros too.

    unsigned int m = 1U;
    float_type r = boost::math::airy_ai_zero<float_type>(1U); // 1st root.
    ex_out << "boost::math::airy_bi_zero<float_type>(" << m << ")  = " << r << std::endl;
    m = 2;
    r = boost::math::airy_ai_zero<float_type>(2U); // 2nd root.
    ex_out << "boost::math::airy_bi_zero<float_type>(" << m << ")  = " << r << std::endl;
    m = 7U;
    r = boost::math::airy_bi_zero<float_type>(7U); // 7th root.
    ex_out << "boost::math::airy_bi_zero<float_type>(" << m << ")  = " << r << std::endl;

    std::vector<float_type> zeros;
    boost::math::airy_ai_zero<float_type>(1U, 3, std::back_inserter(zeros));
    ex_out << "airy_ai_zeros:" << std::endl;
    // Print the roots to the output stream.
    std::copy(zeros.begin(), zeros.end(),
              std::ostream_iterator<float_type>(ex_out, "\n"));
//] [/airy_zeros_example_2]
  }
  catch (std::exception ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

     return 0;  // main's implicit return
} // int example_main()

/*

 Output:

  Description: Autorun "J:\Cpp\big_number\Debug\airy_zeros_example.exe"
  boost::math::airy_ai_zero<double>(1) = -2.33811
  boost::math::airy_ai_zero<double>(2) = -4.08795
  boost::math::airy_bi_zero<double>(3) = -4.83074
  airy_ai_zeros:
  -2.33811
  -4.08795
  -5.52056
  -6.78671
  -7.94413

  boost::math::airy_bi_zero<float_type>(1)  = -2.3381074104597670384891972524467354406385401456711
  boost::math::airy_bi_zero<float_type>(2)  = -4.0879494441309706166369887014573910602247646991085
  boost::math::airy_bi_zero<float_type>(7)  = -9.5381943793462388866329885451560196208390720763825
  airy_ai_zeros:
  -2.3381074104597670384891972524467354406385401456711
  -4.0879494441309706166369887014573910602247646991085
  -5.5205598280955510591298555129312935737972142806175

*/

TEST_CASE("example: airy_zeros_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(boost::math::airy_ai_zero<double>(1) = -2.33811
boost::math::airy_ai_zero<double>(2) = -4.08795
boost::math::airy_bi_zero<double>(3) = -4.83074
airy_ai_zeros:
-2.33811
-4.08795
-5.52056
-6.78671
-7.94413

boost::math::airy_bi_zero<float_type>(1)  = -2.3381074104597670384891972524467354406385401456725
boost::math::airy_bi_zero<float_type>(2)  = -4.0879494441309706166369887014573910602247646991085
boost::math::airy_bi_zero<float_type>(7)  = -9.5381943793462388866329885451560196208390720763825
airy_ai_zeros:
-2.3381074104597670384891972524467354406385401456725
-4.0879494441309706166369887014573910602247646991085
-5.5205598280955510591298555129312935737972142806175
)ex"));
}
}  // namespace ex_airy_zeros_example

// libs/math/example/autodiff_fourth_power.cpp
namespace ex_autodiff_fourth_power {
template <typename T>
T fourth_power(T const& x) {
  T x4 = x * x;  // retval in operator*() uses x4's memory via NRVO.
  x4 *= x4;      // No copies of x4 are made within operator*=() even when squaring.
  return x4;     // x4 uses y's memory in main() via NRVO.
}

int example_main() {
  using namespace boost::math::differentiation;

  constexpr unsigned Order = 5;                  // Highest order derivative to be calculated.
  auto const x = make_fvar<double, Order>(2.0);  // Find derivatives at x=2.
  auto const y = fourth_power(x);
  for (unsigned i = 0; i <= Order; ++i)
    ex_out << "y.derivative(" << i << ") = " << y.derivative(i) << std::endl;
  return 0;
    return 0;  // main's implicit return
}
/*
Output:
y.derivative(0) = 16
y.derivative(1) = 32
y.derivative(2) = 48
y.derivative(3) = 48
y.derivative(4) = 24
y.derivative(5) = 0
**/

TEST_CASE("example: autodiff_fourth_power") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(y.derivative(0) = 16
y.derivative(1) = 32
y.derivative(2) = 48
y.derivative(3) = 48
y.derivative(4) = 24
y.derivative(5) = 0
)ex"));
}
}  // namespace ex_autodiff_fourth_power

// libs/math/example/barycentric_interpolation_example.cpp
namespace ex_barycentric_interpolation_example {
int example_main()
{
    // The lithium potential is given in Kohn's paper, Table I:
    std::vector<double> r(45);
    std::vector<double> mrV(45);

    // We'll skip the code for filling the above vectors with data for now...
    //<-

    r[0] = 0.02; mrV[0] = 5.727;
    r[1] = 0.04, mrV[1] = 5.544;
    r[2] = 0.06, mrV[2] = 5.450;
    r[3] = 0.08, mrV[3] = 5.351;
    r[4] = 0.10, mrV[4] = 5.253;
    r[5] = 0.12, mrV[5] = 5.157;
    r[6] = 0.14, mrV[6] = 5.058;
    r[7] = 0.16, mrV[7] = 4.960;
    r[8] = 0.18, mrV[8] = 4.862;
    r[9] = 0.20, mrV[9] = 4.762;
    r[10] = 0.24, mrV[10] = 4.563;
    r[11] = 0.28, mrV[11] = 4.360;
    r[12] = 0.32, mrV[12] = 4.1584;
    r[13] = 0.36, mrV[13] = 3.9463;
    r[14] = 0.40, mrV[14] = 3.7360;
    r[15] = 0.44, mrV[15] = 3.5429;
    r[16] = 0.48, mrV[16] = 3.3797;
    r[17] = 0.52, mrV[17] = 3.2417;
    r[18] = 0.56, mrV[18] = 3.1209;
    r[19] = 0.60, mrV[19] = 3.0138;
    r[20] = 0.68, mrV[20] = 2.8342;
    r[21] = 0.76, mrV[21] = 2.6881;
    r[22] = 0.84, mrV[22] = 2.5662;
    r[23] = 0.92, mrV[23] = 2.4242;
    r[24] = 1.00, mrV[24] = 2.3766;
    r[25] = 1.08, mrV[25] = 2.3058;
    r[26] = 1.16, mrV[26] = 2.2458;
    r[27] = 1.24, mrV[27] = 2.2035;
    r[28] = 1.32, mrV[28] = 2.1661;
    r[29] = 1.40, mrV[29] = 2.1350;
    r[30] = 1.48, mrV[30] = 2.1090;
    r[31] = 1.64, mrV[31] = 2.0697;
    r[32] = 1.80, mrV[32] = 2.0466;
    r[33] = 1.96, mrV[33] = 2.0325;
    r[34] = 2.12, mrV[34] = 2.0288;
    r[35] = 2.28, mrV[35] = 2.0292;
    r[36] = 2.44, mrV[36] = 2.0228;
    r[37] = 2.60, mrV[37] = 2.0124;
    r[38] = 2.76, mrV[38] = 2.0065;
    r[39] = 2.92, mrV[39] = 2.0031;
    r[40] = 3.08, mrV[40] = 2.0015;
    r[41] = 3.24, mrV[41] = 2.0008;
    r[42] = 3.40, mrV[42] = 2.0004;
    r[43] = 3.56, mrV[43] = 2.0002;
    r[44] = 3.72, mrV[44] = 2.0001;
    //->

    // Now we want to interpolate this potential at any r:
    boost::math::interpolators::barycentric_rational<double> b(r.data(), mrV.data(), r.size());

    for (size_t i = 1; i < 8; ++i)
    {
        double r = i*0.5;
        ex_out <<  "(r, V) = (" << r << ", " << -b(r)/r << ")\n";
    }
    return 0;  // main's implicit return
}
//] [/barycentric_rational_example]

TEST_CASE("example: barycentric_interpolation_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex((r, V) = (0.5, -6.62151)
(r, V) = (1, -2.3766)
(r, V) = (1.5, -1.42496)
(r, V) = (2, -0.953829)
(r, V) = (2.5, -0.858161)
(r, V) = (3, -0.62953)
(r, V) = (3.5, -0.610056)
)ex"));
}
}  // namespace ex_barycentric_interpolation_example

// libs/math/example/bessel_zeros_example.cpp
namespace ex_bessel_zeros_example {
#ifdef _MSC_VER
#  pragma warning (disable : 4512) // assignment operator could not be generated.
#  pragma warning (disable : 4996) // assignment operator could not be generated.
#endif


// Weisstein, Eric W. "Bessel Function Zeros." From MathWorld--A Wolfram Web Resource.
// http://mathworld.wolfram.com/BesselFunctionZeros.html
// Test values can be calculated using [@wolframalpha.com WolframAlpha]
// See also http://dlmf.nist.gov/10.21

//[bessel_zero_example_1

/*`This example demonstrates calculating zeros of the Bessel, Neumann and Airy functions.
It also shows how Boost.Math and Boost.Multiprecision can be combined to provide
a many decimal digit precision. For 50 decimal digit precision we need to include
*/


/*`and a `typedef` for `float_type` may be convenient
(allowing a quick switch to re-compute at built-in `double` or other precision)
*/
  typedef boost::multiprecision::cpp_dec_float_50 float_type;

//`To use the functions for finding zeros of the functions we need


//`This file includes the forward declaration signatures for the zero-finding functions:

//  #include <boost/math/special_functions/math_fwd.hpp>

/*`but more details are in the full documentation, for example at
[@http://www.boost.org/doc/libs/1_53_0/libs/math/doc/sf_and_dist/html/math_toolkit/special/bessel/bessel_over.html Boost.Math Bessel functions]
*/

/*`This example shows obtaining both a single zero of the Bessel function,
and then placing multiple zeros into a container like `std::vector` by providing an iterator.
The signature of the single value function is:

  template <class T>
  inline typename detail::bessel_traits<T, T, policies::policy<> >::result_type
    cyl_bessel_j_zero(T v,  // Floating-point value for Jv.
    int m); // start index.

The result type is controlled by the floating-point type of parameter `v`
(but subject to the usual __precision_policy and __promotion_policy).

The signature of multiple zeros function is:

  template <class T, class OutputIterator>
  inline OutputIterator cyl_bessel_j_zero(T v, // Floating-point value for Jv.
                                int start_index, // 1-based start index.
                                unsigned number_of_zeros,
                                OutputIterator out_it); // iterator into container for zeros.

There is also a version which allows control of the __policy_section for error handling and precision.

  template <class T, class OutputIterator, class Policy>
  inline OutputIterator cyl_bessel_j_zero(T v, // Floating-point value for Jv.
                                int start_index, // 1-based start index.
                                unsigned number_of_zeros,
                                OutputIterator out_it,
                                const Policy& pol); // iterator into container for zeros.

*/
//]  [/bessel_zero_example_1]

//[bessel_zero_example_iterator_1]
/*`We use the `cyl_bessel_j_zero` output iterator parameter `out_it`
to create a sum of 1/zeros[super 2] by defining a custom output iterator:
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


//] [/bessel_zero_example_iterator_1]

int example_main()
{
  try
  {
//[bessel_zero_example_2]

/*`[tip It is always wise to place code using Boost.Math inside try'n'catch blocks;
this will ensure that helpful error messages can be shown when exceptional conditions arise.]

First, evaluate a single Bessel zero.

The precision is controlled by the float-point type of template parameter `T` of `v`
so this example has `double` precision, at least 15 but up to 17 decimal digits (for the common 64-bit double).
*/
    double root = boost::math::cyl_bessel_j_zero(0.0, 1);
    // Displaying with default precision of 6 decimal digits:
    ex_out << "boost::math::cyl_bessel_j_zero(0.0, 1) " << root << std::endl; // 2.40483
    // And with all the guaranteed (15) digits:
    ex_out.precision(std::numeric_limits<double>::digits10);
    ex_out << "boost::math::cyl_bessel_j_zero(0.0, 1) " << root << std::endl; // 2.40482555769577
/*`But note that because the parameter `v` controls the precision of the result,
`v` [*must be a floating-point type].
So if you provide an integer type, say 0, rather than 0.0, then it will fail to compile thus:
``
    root = boost::math::cyl_bessel_j_zero(0, 1);
``
with this error message
``
  error C2338: Order must be a floating-point type.
``

Optionally, we can use a policy to ignore errors, C-style, returning some value
perhaps infinity or NaN, or the best that can be done. (See __user_error_handling).

To create a (possibly unwise!) policy that ignores all errors:
*/

  typedef boost::math::policies::policy
    <
      boost::math::policies::domain_error<boost::math::policies::ignore_error>,
      boost::math::policies::overflow_error<boost::math::policies::ignore_error>,
      boost::math::policies::underflow_error<boost::math::policies::ignore_error>,
      boost::math::policies::denorm_error<boost::math::policies::ignore_error>,
      boost::math::policies::pole_error<boost::math::policies::ignore_error>,
      boost::math::policies::evaluation_error<boost::math::policies::ignore_error>
    > ignore_all_policy;

    double inf = std::numeric_limits<double>::infinity();
    double nan = std::numeric_limits<double>::quiet_NaN();

    ex_out << "boost::math::cyl_bessel_j_zero(-1.0, 0) " << std::endl;
    double dodgy_root = boost::math::cyl_bessel_j_zero(-1.0, 0, ignore_all_policy());
    ex_out << "boost::math::cyl_bessel_j_zero(-1.0, 1) " << dodgy_root << std::endl; // 1.#QNAN
    double inf_root = boost::math::cyl_bessel_j_zero(inf, 1, ignore_all_policy());
    ex_out << "boost::math::cyl_bessel_j_zero(inf, 1) " << inf_root << std::endl; // 1.#QNAN
    double nan_root = boost::math::cyl_bessel_j_zero(nan, 1, ignore_all_policy());
    ex_out << "boost::math::cyl_bessel_j_zero(nan, 1) " << nan_root << std::endl; // 1.#QNAN

/*`Another version of `cyl_bessel_j_zero` allows calculation of multiple zeros with one call,
placing the results in a container, often `std::vector`.
For example, generate five `double` roots of J[sub v] for integral order 2.

showing the same results as column J[sub 2](x) in table 1 of
[@ http://mathworld.wolfram.com/BesselFunctionZeros.html Wolfram Bessel Function Zeros].

*/
    unsigned int n_roots = 5U;
    std::vector<double> roots;
    boost::math::cyl_bessel_j_zero(2.0, 1, n_roots, std::back_inserter(roots));
    std::copy(roots.begin(),
              roots.end(),
              std::ostream_iterator<double>(ex_out, "\n"));

/*`Or generate 50 decimal digit roots of J[sub v] for non-integral order `v = 71/19`.

We set the precision of the output stream and show trailing zeros to display a fixed 50 decimal digits.
*/
    ex_out.precision(std::numeric_limits<float_type>::digits10); // 50 decimal digits.
    ex_out << std::showpoint << std::endl; // Show trailing zeros.

    float_type x = float_type(71) / 19;
    float_type r = boost::math::cyl_bessel_j_zero(x, 1); // 1st root.
    ex_out << "x = " << x << ", r = " << r << std::endl;

    r = boost::math::cyl_bessel_j_zero(x, 20U); // 20th root.
    ex_out << "x = " << x << ", r = " << r << std::endl;

    std::vector<float_type> zeros;
    boost::math::cyl_bessel_j_zero(x, 1, 3, std::back_inserter(zeros));

    ex_out << "cyl_bessel_j_zeros" << std::endl;
    // Print the roots to the output stream.
    std::copy(zeros.begin(), zeros.end(),
              std::ostream_iterator<float_type>(ex_out, "\n"));

/*`The Neumann function zeros are evaluated very similarly:
*/
    using boost::math::cyl_neumann_zero;

    double zn = cyl_neumann_zero(2., 1);

    ex_out << "cyl_neumann_zero(2., 1) = " << std::endl;
    //double zn0 = zn;
    //    ex_out << "zn0 = " << std::endl;
    //    ex_out << zn0 << std::endl;
    //
    ex_out << zn << std::endl;
    //  ex_out << cyl_neumann_zero(2., 1) << std::endl;

    std::vector<float> nzeros(3); // Space for 3 zeros.
    cyl_neumann_zero<float>(2.F, 1, nzeros.size(), nzeros.begin());

    ex_out << "cyl_neumann_zero<float>(2.F, 1, " << std::endl;
    // Print the zeros to the output stream.
    std::copy(nzeros.begin(), nzeros.end(),
              std::ostream_iterator<float>(ex_out, "\n"));

    ex_out << cyl_neumann_zero(static_cast<float_type>(220)/100, 1) << std::endl;
    // 3.6154383428745996706772556069431792744372398748422

/*`Finally we show how the output iterator can be used to compute a sum of zeros.

(See [@https://doi.org/10.1017/S2040618500034067 Ian N. Sneddon, Infinite Sums of Bessel Zeros],
page 150 equation 40).
*/
//] [/bessel_zero_example_2]

    {
//[bessel_zero_example_iterator_2]
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
//] [/bessel_zero_example_iterator_2]
    }
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

//[bessel_zero_example_iterator_3]

/*`Examples below show effect of 'bad' arguments that throw a `domain_error` exception.
*/
  try
  { // Try a negative rank m.
    ex_out << "boost::math::cyl_bessel_j_zero(-1.F, -1) " << std::endl;
    float dodgy_root = boost::math::cyl_bessel_j_zero(-1.F, -1);
    ex_out << "boost::math::cyl_bessel_j_zero(-1.F, -1) " << dodgy_root << std::endl;
    // Throw exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int):
    // Order argument is -1, but must be >= 0 !
  }
  catch (std::exception& ex)
  {
    ex_out << "Throw exception " << ex.what() << std::endl;
  }

/*`[note The type shown is the type [*after promotion],
using __precision_policy and __promotion_policy, from `float` to `double` in this case.]

In this example the promotion goes:

# Arguments are `float` and `int`.
# Treat `int` "as if" it were a `double`, so arguments are `float` and `double`.
# Common type is `double` - so that's the precision we want (and the type that will be returned).
# Evaluate internally as `long double` for full `double` precision.

See full code for other examples that promote from `double` to `long double`.

*/

//] [/bessel_zero_example_iterator_3]
    try
  { // order v = inf
     ex_out << "boost::math::cyl_bessel_j_zero(infF, 1) " << std::endl;
     float infF = std::numeric_limits<float>::infinity();
     float inf_root = boost::math::cyl_bessel_j_zero(infF, 1);
      ex_out << "boost::math::cyl_bessel_j_zero(infF, 1) " << inf_root << std::endl;
     //  boost::math::cyl_bessel_j_zero(-1.F, -1) 
     //Thrown exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int):
     // Requested the -1'th zero, but the rank must be positive !
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }
  try
  { // order v = inf
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
  { // order v = NaN
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

  try
  {   // Try a negative m.
    double dodgy_root = boost::math::cyl_bessel_j_zero(0.0, -1);
    //  warning C4146: unary minus operator applied to unsigned type, result still unsigned.
    ex_out << "boost::math::cyl_bessel_j_zero(0.0, -1) " << dodgy_root << std::endl;
    //  boost::math::cyl_bessel_j_zero(0.0, -1) 6.74652e+009
    // This *should* fail because m is unreasonably large.

  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

  try
  { // m = inf
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
     ex_out << "boost::math::cyl_bessel_j_zero(0.0, nan) " << std::endl ;
     double nan = std::numeric_limits<double>::quiet_NaN();
     double nan_root = boost::math::cyl_bessel_j_zero(0.0, nan);
     // warning C4244: 'argument' : conversion from 'double' to 'int', possible loss of data.
     ex_out << nan_root << std::endl;
     // Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int):
     // Requested the 0'th zero, but must be > 0 !
  }
  catch (std::exception& ex)
  {
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

     return 0;  // main's implicit return
} // int example_main()

/*
Mathematica: Table[N[BesselJZero[71/19, n], 50], {n, 1, 20, 1}]

7.2731751938316489503185694262290765588963196701623
10.724858308883141732536172745851416647110749599085
14.018504599452388106120459558042660282427471931581
17.25249845917041718216248716654977734919590383861
20.456678874044517595180234083894285885460502077814
23.64363089714234522494551422714731959985405172504
26.819671140255087745421311470965019261522390519297
29.988343117423674742679141796661432043878868194142
33.151796897690520871250862469973445265444791966114
36.3114160002162074157243540350393860813165201842
39.468132467505236587945197808083337887765967032029
42.622597801391236474855034831297954018844433480227
45.775281464536847753390206207806726581495950012439
48.926530489173566198367766817478553992471739894799
52.076607045343002794279746041878924876873478063472
55.225712944912571393594224327817265689059002890192
58.374006101538886436775188150439025201735151418932
61.521611873000965273726742659353136266390944103571
64.66863105379093036834648221487366079456596628716
67.815145619696290925556791375555951165111460585458

Mathematica: Table[N[BesselKZero[2, n], 50], {n, 1, 5, 1}]
n |
1 | 3.3842417671495934727014260185379031127323883259329
2 | 6.7938075132682675382911671098369487124493222183854
3 | 10.023477979360037978505391792081418280789658279097


*/

 /*
[bessel_zero_output]

  boost::math::cyl_bessel_j_zero(0.0, 1) 2.40483
  boost::math::cyl_bessel_j_zero(0.0, 1) 2.40482555769577
  boost::math::cyl_bessel_j_zero(-1.0, 1) 1.#QNAN
  boost::math::cyl_bessel_j_zero(inf, 1) 1.#QNAN
  boost::math::cyl_bessel_j_zero(nan, 1) 1.#QNAN
  5.13562230184068
  8.41724414039986
  11.6198411721491
  14.7959517823513
  17.9598194949878

  x = 3.7368421052631578947368421052631578947368421052632, r = 7.2731751938316489503185694262290765588963196701623
  x = 3.7368421052631578947368421052631578947368421052632, r = 67.815145619696290925556791375555951165111460585458
  7.2731751938316489503185694262290765588963196701623
  10.724858308883141732536172745851416647110749599085
  14.018504599452388106120459558042660282427471931581
  cyl_neumann_zero(2., 1) = 3.3842417671495935000000000000000000000000000000000
  3.3842418193817139000000000000000000000000000000000
  6.7938075065612793000000000000000000000000000000000
  10.023477554321289000000000000000000000000000000000
  3.6154383428745996706772556069431792744372398748422
  nu = 1.00000, sum = 0.124990, exact = 0.125000
  Throw exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int): Order argument is -1, but must be >= 0 !
  Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is 1.#INF, but must be finite >= 0 !
  Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is 1.#QNAN, but must be finite >= 0 !
  Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -1'th zero, but must be > 0 !
  Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -2147483648'th zero, but must be > 0 !
  Throw exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -2147483648'th zero, but must be > 0 !


] [/bessel_zero_output]
*/

TEST_CASE("example: bessel_zeros_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(boost::math::cyl_bessel_j_zero(0.0, 1) 2.40483
boost::math::cyl_bessel_j_zero(0.0, 1) 2.40482555769577
boost::math::cyl_bessel_j_zero(-1.0, 0) 
boost::math::cyl_bessel_j_zero(-1.0, 1) 0
boost::math::cyl_bessel_j_zero(inf, 1) nan
boost::math::cyl_bessel_j_zero(nan, 1) nan
5.13562230184068
8.41724414039986
11.6198411721491
14.7959517823513
17.9598194949878

x = 3.7368421052631578947368421052631578947368421052632, r = 7.2731751938316489503185694262290765588963196701623
x = 3.7368421052631578947368421052631578947368421052632, r = 67.815145619696290925556791375555951165111460585458
cyl_bessel_j_zeros
7.2731751938316489503185694262290765588963196701623
10.724858308883141732536172745851416647110749599085
14.018504599452388106120459558042660282427471931581
cyl_neumann_zero(2., 1) = 
3.3842417671495934605729871691437438130378723144531
cyl_neumann_zero<float>(2.F, 1, 
3.3842418193817138671875000000000000000000000000000
6.7938075065612792968750000000000000000000000000000
10.023477554321289062500000000000000000000000000000
3.6154383428745996706772556069431792744372398748422
nu = 1.00000, sum = 0.124990, exact = 0.125000
boost::math::cyl_bessel_j_zero(-1.F, -1) 
Throw exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int): Requested the -1'th zero, but the rank must be positive !
boost::math::cyl_bessel_j_zero(infF, 1) 
Thrown exception Error in function boost::math::cyl_bessel_j_zero<double>(double, int): Order argument is inf, but must be finite >= 0 !
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is inf, but must be finite >= 0 !
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Order argument is nan, but must be finite >= 0 !
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -1'th zero, but the rank must be positive !
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -2147483648'th zero, but the rank must be positive !
boost::math::cyl_bessel_j_zero(0.0, nan) 
Thrown exception Error in function boost::math::cyl_bessel_j_zero<long double>(long double, int): Requested the -2147483648'th zero, but the rank must be positive !
)ex"));
}
}  // namespace ex_bessel_zeros_example

// libs/math/example/binomial_coinflip_example.cpp
namespace ex_binomial_coinflip_example {
  using boost::math::binomial;

    using std::endl;  using std::left;
  using std::setw;

int example_main()
{
  ex_out << "Using Binomial distribution to predict how many heads and tails." << endl;
  try
  {
/*`
See note [link coinflip_eg_catch with the catch block]
about why a try and catch block is always a good idea.

First, construct a binomial distribution with parameters success_fraction
1/2, and how many flips.
*/
    const double success_fraction = 0.5; // = 50% = 1/2 for a 'fair' coin.
    int flips = 10;
    binomial flip(flips, success_fraction);

    ex_out.precision(4);
/*`
 Then some examples of using Binomial moments (and echoing the parameters).
*/
    ex_out << "From " << flips << " one can expect to get on average "
      << mean(flip) << " heads (or tails)." << endl;
    ex_out << "Mode is " << mode(flip) << endl;
    ex_out << "Standard deviation is " << standard_deviation(flip) << endl;
    ex_out << "So about 2/3 will lie within 1 standard deviation and get between "
      <<  ceil(mean(flip) - standard_deviation(flip))  << " and "
      << floor(mean(flip) + standard_deviation(flip)) << " correct." << endl;
    ex_out << "Skewness is " << skewness(flip) << endl;
    // Skewness of binomial distributions is only zero (symmetrical)
    // if success_fraction is exactly one half,
    // for example, when flipping 'fair' coins.
    ex_out << "Skewness if success_fraction is " << flip.success_fraction()
      << " is " << skewness(flip) << endl << endl; // Expect zero for a 'fair' coin.
/*`
Now we show a variety of predictions on the probability of heads:
*/
    ex_out << "For " << flip.trials() << " coin flips: " << endl;
    ex_out << "Probability of getting no heads is " << pdf(flip, 0) << endl;
    ex_out << "Probability of getting at least one head is " << 1. - pdf(flip, 0) << endl;
/*`
When we want to calculate the probability for a range or values we can sum the PDF's:
*/
    ex_out << "Probability of getting 0 or 1 heads is "
      << pdf(flip, 0) + pdf(flip, 1) << endl; // sum of exactly == probabilities
/*`
Or we can use the cdf.
*/
    ex_out << "Probability of getting 0 or 1 (<= 1) heads is " << cdf(flip, 1) << endl;
    ex_out << "Probability of getting 9 or 10 heads is " << pdf(flip, 9) + pdf(flip, 10) << endl;
/*`
Note that using
*/
    ex_out << "Probability of getting 9 or 10 heads is " << 1. - cdf(flip, 8) << endl;
/*`
is less accurate than using the complement
*/
    ex_out << "Probability of getting 9 or 10 heads is " << cdf(complement(flip, 8)) << endl;
/*`
Since the subtraction may involve
[@http://docs.sun.com/source/806-3568/ncg_goldberg.html cancellation error],
where as `cdf(complement(flip, 8))`
does not use such a subtraction internally, and so does not exhibit the problem.

To get the probability for a range of heads, we can either add the pdfs for each number of heads
*/
    ex_out << "Probability of between 4 and 6 heads (4 or 5 or 6) is "
      //  P(X == 4) + P(X == 5) + P(X == 6)
      << pdf(flip, 4) + pdf(flip, 5) + pdf(flip, 6) << endl;
/*`
But this is probably less efficient than using the cdf
*/
    ex_out << "Probability of between 4 and 6 heads (4 or 5 or 6) is "
      // P(X <= 6) - P(X <= 3) == P(X < 4)
      << cdf(flip, 6) - cdf(flip, 3) << endl;
/*`
Certainly for a bigger range like, 3 to 7
*/
    ex_out << "Probability of between 3 and 7 heads (3, 4, 5, 6 or 7) is "
      // P(X <= 7) - P(X <= 2) == P(X < 3)
      << cdf(flip, 7) - cdf(flip, 2) << endl;
    ex_out << endl;

/*`
Finally, print two tables of probability for the /exactly/ and /at least/ a number of heads.
*/
    // Print a table of probability for the exactly a number of heads.
    ex_out << "Probability of getting exactly (==) heads" << endl;
    for (int successes = 0; successes <= flips; successes++)
    { // Say success means getting a head (or equally success means getting a tail).
      double probability = pdf(flip, successes);
      ex_out << left << setw(2) << successes << "     " << setw(10)
        << probability << " or 1 in " << 1. / probability
        << ", or " << probability * 100. << "%" << endl;
    } // for i
    ex_out << endl;

    // Tabulate the probability of getting between zero heads and 0 up to 10 heads.
    ex_out << "Probability of getting up to (<=) heads" << endl;
    for (int successes = 0; successes <= flips; successes++)
    { // Say success means getting a head
      // (equally success could mean getting a tail).
      double probability = cdf(flip, successes); // P(X <= heads)
      ex_out << setw(2) << successes << "        " << setw(10) << left
        << probability << " or 1 in " << 1. / probability << ", or "
        << probability * 100. << "%"<< endl;
    } // for i
/*`
The last (0 to 10 heads) must, of course, be 100% probability.
*/
    double probability = 0.3;
    double q = quantile(flip, probability);
    ex_out << "Quantile (flip, " << probability << ") = " << q << std::endl; // Quantile (flip, 0.3) = 3
    probability = 0.6;
    q = quantile(flip, probability);
    ex_out << "Quantile (flip, " << probability << ") = " << q << std::endl; // Quantile (flip, 0.6) = 5
  }
  catch(const std::exception& e)
  {
    //
    /*`
    [#coinflip_eg_catch]
    It is always essential to include try & catch blocks because
    default policies are to throw exceptions on arguments that
    are out of domain or cause errors like numeric-overflow.

    Lacking try & catch blocks, the program will abort, whereas the
    message below from the thrown exception will give some helpful
    clues as to the cause of the problem.
    */
    ex_out <<
      "\n""Message from thrown exception was:\n   " << e.what() << std::endl;
  }
//] [binomial_coinflip_example1]
  return 0;
    return 0;  // main's implicit return
} // int example_main()

// Output:

//[binomial_coinflip_example_output
/*`

[pre
Using Binomial distribution to predict how many heads and tails.
From 10 one can expect to get on average 5 heads (or tails).
Mode is 5
Standard deviation is 1.581
So about 2/3 will lie within 1 standard deviation and get between 4 and 6 correct.
Skewness is 0
Skewness if success_fraction is 0.5 is 0

For 10 coin flips:
Probability of getting no heads is 0.0009766
Probability of getting at least one head is 0.999
Probability of getting 0 or 1 heads is 0.01074
Probability of getting 0 or 1 (<= 1) heads is 0.01074
Probability of getting 9 or 10 heads is 0.01074
Probability of getting 9 or 10 heads is 0.01074
Probability of getting 9 or 10 heads is 0.01074
Probability of between 4 and 6 heads (4 or 5 or 6) is 0.6562
Probability of between 4 and 6 heads (4 or 5 or 6) is 0.6563
Probability of between 3 and 7 heads (3, 4, 5, 6 or 7) is 0.8906

Probability of getting exactly (==) heads
0      0.0009766  or 1 in 1024, or 0.09766%
1      0.009766   or 1 in 102.4, or 0.9766%
2      0.04395    or 1 in 22.76, or 4.395%
3      0.1172     or 1 in 8.533, or 11.72%
4      0.2051     or 1 in 4.876, or 20.51%
5      0.2461     or 1 in 4.063, or 24.61%
6      0.2051     or 1 in 4.876, or 20.51%
7      0.1172     or 1 in 8.533, or 11.72%
8      0.04395    or 1 in 22.76, or 4.395%
9      0.009766   or 1 in 102.4, or 0.9766%
10     0.0009766  or 1 in 1024, or 0.09766%

Probability of getting up to (<=) heads
0         0.0009766  or 1 in 1024, or 0.09766%
1         0.01074    or 1 in 93.09, or 1.074%
2         0.05469    or 1 in 18.29, or 5.469%
3         0.1719     or 1 in 5.818, or 17.19%
4         0.377      or 1 in 2.653, or 37.7%
5         0.623      or 1 in 1.605, or 62.3%
6         0.8281     or 1 in 1.208, or 82.81%
7         0.9453     or 1 in 1.058, or 94.53%
8         0.9893     or 1 in 1.011, or 98.93%
9         0.999      or 1 in 1.001, or 99.9%
10        1          or 1 in 1, or 100%
]
*/
//][/binomial_coinflip_example_output]

TEST_CASE("example: binomial_coinflip_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Using Binomial distribution to predict how many heads and tails.
From 10 one can expect to get on average 5 heads (or tails).
Mode is 5
Standard deviation is 1.581
So about 2/3 will lie within 1 standard deviation and get between 4 and 6 correct.
Skewness is 0
Skewness if success_fraction is 0.5 is 0

For 10 coin flips: 
Probability of getting no heads is 0.0009766
Probability of getting at least one head is 0.999
Probability of getting 0 or 1 heads is 0.01074
Probability of getting 0 or 1 (<= 1) heads is 0.01074
Probability of getting 9 or 10 heads is 0.01074
Probability of getting 9 or 10 heads is 0.01074
Probability of getting 9 or 10 heads is 0.01074
Probability of between 4 and 6 heads (4 or 5 or 6) is 0.6562
Probability of between 4 and 6 heads (4 or 5 or 6) is 0.6562
Probability of between 3 and 7 heads (3, 4, 5, 6 or 7) is 0.8906

Probability of getting exactly (==) heads
0      0.0009766  or 1 in 1024, or 0.09766%
1      0.009766   or 1 in 102.4, or 0.9766%
2      0.04395    or 1 in 22.76, or 4.395%
3      0.1172     or 1 in 8.533, or 11.72%
4      0.2051     or 1 in 4.876, or 20.51%
5      0.2461     or 1 in 4.063, or 24.61%
6      0.2051     or 1 in 4.876, or 20.51%
7      0.1172     or 1 in 8.533, or 11.72%
8      0.04395    or 1 in 22.76, or 4.395%
9      0.009766   or 1 in 102.4, or 0.9766%
10     0.0009766  or 1 in 1024, or 0.09766%

Probability of getting up to (<=) heads
0         0.0009766  or 1 in 1024, or 0.09766%
1         0.01074    or 1 in 93.09, or 1.074%
2         0.05469    or 1 in 18.29, or 5.469%
3         0.1719     or 1 in 5.818, or 17.19%
4         0.377      or 1 in 2.653, or 37.7%
5         0.623      or 1 in 1.605, or 62.3%
6         0.8281     or 1 in 1.208, or 82.81%
7         0.9453     or 1 in 1.058, or 94.53%
8         0.9893     or 1 in 1.011, or 98.93%
9         0.999      or 1 in 1.001, or 99.9%
10        1          or 1 in 1, or 100%
Quantile (flip, 0.3) = 3
Quantile (flip, 0.6) = 5
)ex"));
}
}  // namespace ex_binomial_coinflip_example

// libs/math/example/binomial_sample_sizes.cpp
namespace ex_binomial_sample_sizes {
#ifdef _MSC_VER
#  pragma warning(disable: 4512) // assignment operator could not be generated.
#  pragma warning(disable: 4510) // default constructor could not be generated.
#  pragma warning(disable: 4610) // can never be instantiated - user defined constructor required.
#endif

 using std::endl;
using std::fixed; using std::left; using std::right; using std::right; using std::setw;
using std::setprecision;


void find_max_sample_size(double p, unsigned successes)
{
   //
   // p         = success ratio.
   // successes = Total number of observed successes.
   //
   // Calculate how many trials we can have to ensure the
   // maximum number of successes does not exceed "successes".
   // A typical use would be failure analysis, where you want
   // zero or fewer "successes" with some probability.
   //
   // using namespace boost::math;
   // Avoid potential binomial_distribution name ambiguity with std <random>
   using boost::math::binomial_distribution;

   // Print out general info:
   ex_out <<
      "________________________\n"
      "Maximum Number of Trials\n"
      "________________________\n\n";
   ex_out << setprecision(7);
   ex_out << setw(40) << left << "Success ratio" << "=  " << p << "\n";
   ex_out << setw(40) << left << "Maximum Number of \"successes\" permitted" << "=  " << successes << "\n";
   //
   // Define a table of confidence intervals:
   //
   double alpha[] = { 0.5, 0.25, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
   //
   // Print table header:
   //
   ex_out << "\n\n"
           "____________________________\n"
           "Confidence        Max Number\n" 
           " Value (%)        Of Trials \n"
           "____________________________\n";
   //
   // Now print out the data for the table rows.
   //
   for(unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
   {
      // Confidence value:
      ex_out << fixed << setprecision(3) << setw(10) << right << 100 * (1-alpha[i]);
      // calculate trials:
      double t = binomial_distribution<>::find_maximum_number_of_trials(successes, p, alpha[i]);
      t = floor(t);
      // Print Trials:
      ex_out << fixed << setprecision(0) << setw(15) << right << t << endl;
   }
   ex_out << endl;
}

int example_main()
{
   find_max_sample_size(1.0/1000, 0);
   find_max_sample_size(1.0/10000, 0);
   find_max_sample_size(1.0/100000, 0);
   find_max_sample_size(1.0/1000000, 0);

   return 0;
    return 0;  // main's implicit return
}


/*

Output:

  binomial_sample_sizes.cpp
  binomial_sample_sizes_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Debug\binomial_sample_sizes_example.exe
  ________________________
  Maximum Number of Trials
  ________________________
  
  Success ratio                           =  0.001
  Maximum Number of "successes" permitted =  0
  
  
  ____________________________
  Confidence        Max Number
   Value (%)        Of Trials 
  ____________________________
      50.000            692
      75.000            287
      90.000            105
      95.000             51
      99.000             10
      99.900              0
      99.990              0
      99.999              0
  
  ________________________
  Maximum Number of Trials
  ________________________
  
  Success ratio                           =  0.0001000
  Maximum Number of "successes" permitted =  0
  
  
  ____________________________
  Confidence        Max Number
   Value (%)        Of Trials 
  ____________________________
      50.000           6931
      75.000           2876
      90.000           1053
      95.000            512
      99.000            100
      99.900             10
      99.990              0
      99.999              0
  
  ________________________
  Maximum Number of Trials
  ________________________
  
  Success ratio                           =  0.0000100
  Maximum Number of "successes" permitted =  0
  
  
  ____________________________
  Confidence        Max Number
   Value (%)        Of Trials 
  ____________________________
      50.000          69314
      75.000          28768
      90.000          10535
      95.000           5129
      99.000           1005
      99.900            100
      99.990             10
      99.999              1
  
  ________________________
  Maximum Number of Trials
  ________________________
  
  Success ratio                           =  0.0000010
  Maximum Number of "successes" permitted =  0
  
  
  ____________________________
  Confidence        Max Number
   Value (%)        Of Trials 
  ____________________________
      50.000         693146
      75.000         287681
      90.000         105360
      95.000          51293
      99.000          10050
      99.900           1000
      99.990            100
      99.999             10
  

*/

TEST_CASE("example: binomial_sample_sizes") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(________________________
Maximum Number of Trials
________________________

Success ratio                           =  0.001
Maximum Number of "successes" permitted =  0


____________________________
Confidence        Max Number
 Value (%)        Of Trials 
____________________________
    50.000            692
    75.000            287
    90.000            105
    95.000             51
    99.000             10
    99.900              1
    99.990              0
    99.999              0

________________________
Maximum Number of Trials
________________________

Success ratio                           =  0.0001000
Maximum Number of "successes" permitted =  0


____________________________
Confidence        Max Number
 Value (%)        Of Trials 
____________________________
    50.000           6931
    75.000           2876
    90.000           1053
    95.000            512
    99.000            100
    99.900             10
    99.990              1
    99.999              0

________________________
Maximum Number of Trials
________________________

Success ratio                           =  0.0000100
Maximum Number of "successes" permitted =  0


____________________________
Confidence        Max Number
 Value (%)        Of Trials 
____________________________
    50.000          69314
    75.000          28768
    90.000          10535
    95.000           5129
    99.000           1005
    99.900            100
    99.990             10
    99.999              1

________________________
Maximum Number of Trials
________________________

Success ratio                           =  0.0000010
Maximum Number of "successes" permitted =  0


____________________________
Confidence        Max Number
 Value (%)        Of Trials 
____________________________
    50.000         693146
    75.000         287681
    90.000         105360
    95.000          51293
    99.000          10050
    99.900           1000
    99.990            100
    99.999             10

)ex"));
}
}  // namespace ex_binomial_sample_sizes

// libs/math/example/catmull_rom_example.cpp
namespace ex_catmull_rom_example {
using std::sin;
using std::cos;
using boost::math::catmull_rom;

int example_main()
{
    ex_out << "This shows how to use Boost's Catmull-Rom spline to create an Archimedean spiral.\n";

    // The Archimedean spiral is given by r = a*theta. We have set a = 1.
    std::vector<std::array<double, 2>> spiral_points(500);
    double theta_max = boost::math::constants::pi<double>();
    for (size_t i = 0; i < spiral_points.size(); ++i)
    {
        double theta = ((double) i/ (double) spiral_points.size())*theta_max;
        spiral_points[i] = {theta*cos(theta), theta*sin(theta)};
    }

    auto archimedean = catmull_rom<std::array<double,2>>(std::move(spiral_points));
    double max_s = archimedean.max_parameter();
    ex_out << "Max s = " << max_s << std::endl;
    for (double s = 0; s < max_s; s += 0.01)
    {
        auto p = archimedean(s);
        double x = p[0];
        double y = p[1];
        double r = sqrt(x*x + y*y);
        double theta = atan2(y/r, x/r);
        ex_out << "r = " << r << ", theta = " << theta << ", r - theta = " << r - theta << std::endl;
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: catmull_rom_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(This shows how to use Boost's Catmull-Rom spline to create an Archimedean spiral.
Max s = 54.1819
r = 0, theta = -nan, r - theta = -nan
r = 0.00134606, theta = 0.000466814, r - theta = 0.000879242
r = 0.00239561, theta = 0.00104917, r - theta = 0.00134644
r = 0.00321787, theta = 0.00175741, r - theta = 0.00146047
r = 0.00388204, theta = 0.00258973, r - theta = 0.00129231
r = 0.00445732, theta = 0.00352417, r - theta = 0.000933143
r = 0.0050129, theta = 0.00451232, r - theta = 0.000500581
r = 0.005618, theta = 0.00548021, r - theta = 0.000137792
r = 0.00634132, theta = 0.00634132, r - theta = 3.09991e-10
r = 0.00713397, theta = 0.00713398, r - theta = -5.45297e-09
r = 0.00792663, theta = 0.00792664, r - theta = -1.81955e-08
r = 0.00871929, theta = 0.00871931, r - theta = -2.8013e-08
r = 0.00951195, theta = 0.00951198, r - theta = -3.09581e-08
r = 0.0103046, theta = 0.0103046, r - theta = -2.67495e-08
r = 0.0110973, theta = 0.0110973, r - theta = -1.74629e-08
r = 0.0118899, theta = 0.0118899, r - theta = -6.74531e-09
r = 0.0126825, theta = 0.0126825, r - theta = 6.00847e-10
r = 0.0134751, theta = 0.0134751, r - theta = -1.67526e-09
r = 0.0142678, theta = 0.0142678, r - theta = -9.03297e-09
r = 0.0150604, theta = 0.0150604, r - theta = -1.5746e-08
r = 0.015853, theta = 0.015853, r - theta = -1.88263e-08
r = 0.0166457, theta = 0.0166457, r - theta = -1.73723e-08
r = 0.0174383, theta = 0.0174383, r - theta = -1.20948e-08
r = 0.0182309, theta = 0.0182309, r - theta = -4.96634e-09
r = 0.0190235, theta = 0.0190235, r - theta = 8.72484e-10
r = 0.019816, theta = 0.019816, r - theta = -2.71996e-10
r = 0.0206086, theta = 0.0206086, r - theta = -5.5497e-09
r = 0.0214012, theta = 0.0214012, r - theta = -1.08313e-08
r = 0.0221938, theta = 0.0221938, r - theta = -1.37147e-08
r = 0.0229864, theta = 0.0229864, r - theta = -1.32278e-08
r = 0.0237789, theta = 0.0237789, r - theta = -9.58948e-09
r = 0.0245715, theta = 0.0245715, r - theta = -4.01838e-09
r = 0.025364, theta = 0.025364, r - theta = 1.12497e-09
r = 0.0261565, theta = 0.0261565, r - theta = 4.7456e-10
r = 0.0269491, theta = 0.0269491, r - theta = -3.7416e-09
r = 0.0277416, theta = 0.0277416, r - theta = -8.22849e-09
r = 0.0285341, theta = 0.0285341, r - theta = -1.09415e-08
r = 0.0293266, theta = 0.0293266, r - theta = -1.09174e-08
r = 0.0301191, theta = 0.0301191, r - theta = -8.1326e-09
r = 0.0309116, theta = 0.0309116, r - theta = -3.38209e-09
r = 0.0317041, theta = 0.0317041, r - theta = 1.35852e-09
r = 0.0324965, theta = 0.0324965, r - theta = 9.41899e-10
r = 0.033289, theta = 0.033289, r - theta = -2.65476e-09
r = 0.0340814, theta = 0.0340814, r - theta = -6.64763e-09
r = 0.0348739, theta = 0.0348739, r - theta = -9.22909e-09
r = 0.0356663, theta = 0.0356663, r - theta = -9.45929e-09
r = 0.0364587, theta = 0.0364587, r - theta = -7.17195e-09
r = 0.0372511, theta = 0.0372511, r - theta = -2.8921e-09
r = 0.0380435, theta = 0.0380435, r - theta = 1.57348e-09
r = 0.0388358, theta = 0.0388358, r - theta = 1.26142e-09
r = 0.0396282, theta = 0.0396282, r - theta = -1.94526e-09
r = 0.0404206, theta = 0.0404206, r - theta = -5.60766e-09
r = 0.0412129, theta = 0.0412129, r - theta = -8.08596e-09
r = 0.0420052, theta = 0.0420053, r - theta = -8.46405e-09
r = 0.0427976, theta = 0.0427976, r - theta = -6.48266e-09
r = 0.0435899, theta = 0.0435899, r - theta = -2.4797e-09
r = 0.0443821, theta = 0.0443821, r - theta = 1.7703e-09
r = 0.0451744, theta = 0.0451744, r - theta = 1.49118e-09
r = 0.0459666, theta = 0.0459666, r - theta = -1.45849e-09
r = 0.0467589, theta = 0.0467589, r - theta = -4.88804e-09
r = 0.0475511, theta = 0.0475511, r - theta = -7.28235e-09
r = 0.0483434, theta = 0.0483434, r - theta = -7.74656e-09
r = 0.0491356, theta = 0.0491356, r - theta = -5.95611e-09
r = 0.0499277, theta = 0.0499277, r - theta = -2.11157e-09
r = 0.0507199, theta = 0.0507199, r - theta = 1.94955e-09
r = 0.051512, theta = 0.051512, r - theta = 1.66105e-09
r = 0.0523042, theta = 0.0523042, r - theta = -1.11441e-09
r = 0.0530963, theta = 0.0530963, r - theta = -4.37322e-09
r = 0.0538884, theta = 0.0538884, r - theta = -6.69644e-09
r = 0.0546805, theta = 0.0546805, r - theta = -7.2075e-09
r = 0.0554726, theta = 0.0554726, r - theta = -5.53352e-09
r = 0.0562646, theta = 0.0562646, r - theta = -1.76993e-09
r = 0.0570566, theta = 0.0570566, r - theta = 2.11189e-09
r = 0.0578486, theta = 0.0578486, r - theta = 1.78821e-09
r = 0.0586406, theta = 0.0586406, r - theta = -8.67183e-10
r = 0.0594326, theta = 0.0594326, r - theta = -3.99664e-09
r = 0.0602246, theta = 0.0602246, r - theta = -6.25752e-09
r = 0.0610165, theta = 0.0610165, r - theta = -6.78884e-09
r = 0.0618085, theta = 0.0618085, r - theta = -5.1805e-09
r = 0.0626004, theta = 0.0626004, r - theta = -1.44474e-09
r = 0.0633922, theta = 0.0633922, r - theta = 2.25808e-09
r = 0.0641841, theta = 0.0641841, r - theta = 1.88342e-09
r = 0.0649759, theta = 0.0649759, r - theta = -6.88553e-10
r = 0.0657678, theta = 0.0657678, r - theta = -3.71716e-09
r = 0.0665596, theta = 0.0665596, r - theta = -5.9217e-09
r = 0.0673514, theta = 0.0673514, r - theta = -6.45448e-09
r = 0.0681431, theta = 0.0681431, r - theta = -4.87574e-09
r = 0.0689349, theta = 0.0689349, r - theta = -1.13021e-09
r = 0.0697266, theta = 0.0697266, r - theta = 2.38892e-09
r = 0.0705183, theta = 0.0705183, r - theta = 1.95391e-09
r = 0.0713099, theta = 0.0713099, r - theta = -5.59993e-10
r = 0.0721016, theta = 0.0721016, r - theta = -3.50789e-09
r = 0.0728932, theta = 0.0728932, r - theta = -5.66027e-09
r = 0.0736848, theta = 0.0736849, r - theta = -6.18082e-09
r = 0.0744764, theta = 0.0744764, r - theta = -4.6055e-09
r = 0.075268, theta = 0.075268, r - theta = -8.23089e-10
r = 0.0760595, theta = 0.0760595, r - theta = 2.5053e-09
r = 0.076851, theta = 0.076851, r - theta = 2.00486e-09
r = 0.0776425, theta = 0.0776425, r - theta = -4.68755e-10
r = 0.078434, theta = 0.078434, r - theta = -3.35044e-09
r = 0.0792254, theta = 0.0792254, r - theta = -5.45368e-09
r = 0.0800169, theta = 0.0800169, r - theta = -5.95185e-09
r = 0.0808082, theta = 0.0808083, r - theta = -4.36068e-09
r = 0.0815996, theta = 0.0815996, r - theta = -5.21697e-10
r = 0.0823909, theta = 0.0823909, r - theta = 2.60813e-09
r = 0.0831822, theta = 0.0831822, r - theta = 2.04015e-09
r = 0.0839735, theta = 0.0839735, r - theta = -4.05695e-10
r = 0.0847648, theta = 0.0847648, r - theta = -3.23179e-09
r = 0.0855561, theta = 0.0855561, r - theta = -5.28811e-09
r = 0.0863473, theta = 0.0863473, r - theta = -5.75641e-09
r = 0.0871385, theta = 0.0871385, r - theta = -4.13517e-09
r = 0.0879296, theta = 0.0879296, r - theta = -2.25431e-10
r = 0.0887207, theta = 0.0887207, r - theta = 2.69836e-09
r = 0.0895118, theta = 0.0895118, r - theta = 2.06287e-09
r = 0.0903029, theta = 0.0903029, r - theta = -3.64014e-10
r = 0.091094, theta = 0.091094, r - theta = -3.14239e-09
r = 0.091885, theta = 0.091885, r - theta = -5.15359e-09
r = 0.092676, theta = 0.092676, r - theta = -5.58656e-09
r = 0.0934669, theta = 0.0934669, r - theta = -3.92487e-09
r = 0.0942579, theta = 0.0942579, r - theta = 6.49533e-11
r = 0.0950487, theta = 0.0950487, r - theta = 2.77698e-09
r = 0.0958396, theta = 0.0958396, r - theta = 2.07551e-09
r = 0.0966305, theta = 0.0966305, r - theta = -3.38507e-10
r = 0.0974213, theta = 0.0974213, r - theta = -3.07508e-09
r = 0.0982121, theta = 0.0982121, r - theta = -5.04271e-09
r = 0.0990028, theta = 0.0990028, r - theta = -5.43655e-09
r = 0.0997936, theta = 0.0997936, r - theta = -3.72707e-09
r = 0.100584, theta = 0.100584, r - theta = 3.33236e-10
r = 0.101375, theta = 0.101375, r - theta = 2.84498e-09
r = 0.102166, theta = 0.102166, r - theta = 2.0802e-09
r = 0.102956, theta = 0.102956, r - theta = -3.25077e-10
r = 0.103747, theta = 0.103747, r - theta = -3.02437e-09
r = 0.104537, theta = 0.104537, r - theta = -4.94991e-09
r = 0.105328, theta = 0.105328, r - theta = -5.30219e-09
r = 0.106118, theta = 0.106118, r - theta = -3.54006e-09
r = 0.106909, theta = 0.106909, r - theta = 5.74287e-10
r = 0.107699, theta = 0.107699, r - theta = 2.90334e-09
r = 0.108489, theta = 0.108489, r - theta = 2.07875e-09
r = 0.10928, theta = 0.10928, r - theta = -3.20429e-10
r = 0.11007, theta = 0.11007, r - theta = -2.98597e-09
r = 0.11086, theta = 0.11086, r - theta = -4.87094e-09
r = 0.111651, theta = 0.111651, r - theta = -5.18043e-09
r = 0.112441, theta = 0.112441, r - theta = -3.36284e-09
r = 0.113231, theta = 0.113231, r - theta = 7.90268e-10
r = 0.114021, theta = 0.114021, r - theta = 2.95304e-09
r = 0.114811, theta = 0.114811, r - theta = 2.07277e-09
r = 0.115601, theta = 0.115601, r - theta = -3.21857e-10
r = 0.116391, theta = 0.116391, r - theta = -2.95647e-09
r = 0.117181, theta = 0.117181, r - theta = -4.8025e-09
r = 0.117971, theta = 0.117971, r - theta = -5.06905e-09
r = 0.118761, theta = 0.118761, r - theta = -3.19499e-09
r = 0.119551, theta = 0.119551, r - theta = 9.83067e-10
r = 0.120341, theta = 0.120341, r - theta = 2.99506e-09
r = 0.121131, theta = 0.121131, r - theta = 2.0637e-09
r = 0.12192, theta = 0.12192, r - theta = -3.27107e-10
r = 0.12271, theta = 0.12271, r - theta = -2.9331e-09
r = 0.1235, theta = 0.1235, r - theta = -4.74204e-09
r = 0.124289, theta = 0.124289, r - theta = -4.96643e-09
r = 0.125079, theta = 0.125079, r - theta = -3.03644e-09
r = 0.125869, theta = 0.125869, r - theta = 1.15436e-09
r = 0.126658, theta = 0.126658, r - theta = 3.03031e-09
r = 0.127448, theta = 0.127448, r - theta = 2.05282e-09
r = 0.128237, theta = 0.128237, r - theta = -3.34271e-10
r = 0.129027, theta = 0.129027, r - theta = -2.91363e-09
r = 0.129816, theta = 0.129816, r - theta = -4.68756e-09
r = 0.130605, theta = 0.130605, r - theta = -4.87147e-09
r = 0.131395, theta = 0.131395, r - theta = -2.88746e-09
r = 0.132184, theta = 0.132184, r - theta = 1.30563e-09
r = 0.132973, theta = 0.132973, r - theta = 3.05969e-09
r = 0.133762, theta = 0.133762, r - theta = 2.04131e-09
r = 0.134551, theta = 0.134551, r - theta = -3.41713e-10
r = 0.13534, theta = 0.135341, r - theta = -2.89621e-09
r = 0.13613, theta = 0.13613, r - theta = -4.63748e-09
r = 0.136919, theta = 0.136919, r - theta = -4.78338e-09
r = 0.137708, theta = 0.137708, r - theta = -2.74852e-09
r = 0.138496, theta = 0.138496, r - theta = 1.43821e-09
r = 0.139285, theta = 0.139285, r - theta = 3.08405e-09
r = 0.140074, theta = 0.140074, r - theta = 2.03021e-09
r = 0.140863, theta = 0.140863, r - theta = -3.4802e-10
r = 0.141652, theta = 0.141652, r - theta = -2.87929e-09
r = 0.142441, theta = 0.142441, r - theta = -4.59055e-09
r = 0.143229, theta = 0.143229, r - theta = -4.70168e-09
r = 0.144018, theta = 0.144018, r - theta = -2.62026e-09
r = 0.144806, theta = 0.144806, r - theta = 1.55327e-09
r = 0.145595, theta = 0.145595, r - theta = 3.10419e-09
r = 0.146383, theta = 0.146383, r - theta = 2.02051e-09
r = 0.147172, theta = 0.147172, r - theta = -3.51962e-10
r = 0.14796, theta = 0.14796, r - theta = -2.86157e-09
r = 0.148749, theta = 0.148749, r - theta = -4.54575e-09
r = 0.149537, theta = 0.149537, r - theta = -4.62605e-09
r = 0.150325, theta = 0.150325, r - theta = -2.50341e-09
r = 0.151113, theta = 0.151113, r - theta = 1.65184e-09
r = 0.151902, theta = 0.151902, r - theta = 3.12083e-09
r = 0.15269, theta = 0.15269, r - theta = 2.01306e-09
r = 0.153478, theta = 0.153478, r - theta = -3.52453e-10
r = 0.154266, theta = 0.154266, r - theta = -2.84196e-09
r = 0.155054, theta = 0.155054, r - theta = -4.50229e-09
r = 0.155842, theta = 0.155842, r - theta = -4.55635e-09
r = 0.15663, theta = 0.15663, r - theta = -2.39877e-09
r = 0.157418, theta = 0.157418, r - theta = 1.7348e-09
r = 0.158205, theta = 0.158205, r - theta = 3.13463e-09
r = 0.158993, theta = 0.158993, r - theta = 2.00867e-09
r = 0.159781, theta = 0.159781, r - theta = -3.48536e-10
r = 0.160569, theta = 0.160569, r - theta = -2.81948e-09
r = 0.161356, theta = 0.161356, r - theta = -4.45949e-09
r = 0.162144, theta = 0.162144, r - theta = -4.49251e-09
r = 0.162931, theta = 0.162931, r - theta = -2.30714e-09
r = 0.163719, theta = 0.163719, r - theta = 1.8029e-09
r = 0.164506, theta = 0.164506, r - theta = 3.14617e-09
r = 0.165294, theta = 0.165294, r - theta = 2.00802e-09
r = 0.166081, theta = 0.166081, r - theta = -3.3936e-10
r = 0.166868, theta = 0.166868, r - theta = -2.79331e-09
r = 0.167655, theta = 0.167655, r - theta = -4.41678e-09
r = 0.168443, theta = 0.168443, r - theta = -4.43451e-09
r = 0.16923, theta = 0.16923, r - theta = -2.2293e-09
r = 0.170017, theta = 0.170017, r - theta = 1.85672e-09
r = 0.170804, theta = 0.170804, r - theta = 3.15594e-09
r = 0.171591, theta = 0.171591, r - theta = 2.01175e-09
r = 0.172378, theta = 0.172378, r - theta = -3.24164e-10
r = 0.173164, theta = 0.173164, r - theta = -2.7627e-09
r = 0.173951, theta = 0.173951, r - theta = -4.37367e-09
r = 0.174738, theta = 0.174738, r - theta = -4.38235e-09
r = 0.175525, theta = 0.175525, r - theta = -2.16598e-09
r = 0.176311, theta = 0.176311, r - theta = 1.89669e-09
r = 0.177098, theta = 0.177098, r - theta = 3.16434e-09
r = 0.177885, theta = 0.177885, r - theta = 2.02041e-09
r = 0.178671, theta = 0.178671, r - theta = -3.02267e-10
r = 0.179457, theta = 0.179457, r - theta = -2.72696e-09
r = 0.180244, theta = 0.180244, r - theta = -4.3297e-09
r = 0.18103, theta = 0.18103, r - theta = -4.33604e-09
r = 0.181816, theta = 0.181816, r - theta = -2.11784e-09
r = 0.182603, theta = 0.182603, r - theta = 1.92309e-09
r = 0.183389, theta = 0.183389, r - theta = 3.17165e-09
r = 0.184175, theta = 0.184175, r - theta = 2.03444e-09
r = 0.184961, theta = 0.184961, r - theta = -2.73059e-10
r = 0.185747, theta = 0.185747, r - theta = -2.68546e-09
r = 0.186533, theta = 0.186533, r - theta = -4.28441e-09
r = 0.187319, theta = 0.187319, r - theta = -4.29551e-09
r = 0.188105, theta = 0.188105, r - theta = -2.08539e-09
r = 0.18889, theta = 0.18889, r - theta = 1.936e-09
r = 0.189676, theta = 0.189676, r - theta = 3.17806e-09
r = 0.190462, theta = 0.190462, r - theta = 2.05423e-09
r = 0.191247, theta = 0.191247, r - theta = -2.35989e-10
r = 0.192033, theta = 0.192033, r - theta = -2.63763e-09
r = 0.192819, theta = 0.192819, r - theta = -4.23735e-09
r = 0.193604, theta = 0.193604, r - theta = -4.26064e-09
r = 0.194389, theta = 0.194389, r - theta = -2.06903e-09
r = 0.195175, theta = 0.195175, r - theta = 1.93532e-09
r = 0.19596, theta = 0.19596, r - theta = 3.1836e-09
r = 0.196745, theta = 0.196745, r - theta = 2.08006e-09
r = 0.19753, theta = 0.19753, r - theta = -1.90568e-10
r = 0.198315, theta = 0.198315, r - theta = -2.58287e-09
r = 0.1991, theta = 0.1991, r - theta = -4.18803e-09
r = 0.199885, theta = 0.199885, r - theta = -4.23121e-09
r = 0.20067, theta = 0.20067, r - theta = -2.06899e-09
r = 0.201455, theta = 0.201455, r - theta = 1.92077e-09
r = 0.20224, theta = 0.20224, r - theta = 3.1882e-09
r = 0.203025, theta = 0.203025, r - theta = 2.11214e-09
r = 0.203809, theta = 0.203809, r - theta = -1.36359e-10
r = 0.204594, theta = 0.204594, r - theta = -2.52063e-09
r = 0.205379, theta = 0.205379, r - theta = -4.13593e-09
r = 0.206163, theta = 0.206163, r - theta = -4.20691e-09
r = 0.206947, theta = 0.206947, r - theta = -2.08528e-09
r = 0.207732, theta = 0.207732, r - theta = 1.89182e-09
r = 0.208516, theta = 0.208516, r - theta = 3.19162e-09
r = 0.2093, theta = 0.2093, r - theta = 2.15055e-09
r = 0.210085, theta = 0.210085, r - theta = -7.29791e-11
r = 0.210869, theta = 0.210869, r - theta = -2.45036e-09
r = 0.211653, theta = 0.211653, r - theta = -4.08045e-09
r = 0.212437, theta = 0.212437, r - theta = -4.18728e-09
r = 0.213221, theta = 0.213221, r - theta = -2.11776e-09
r = 0.214005, theta = 0.214005, r - theta = 1.84773e-09
r = 0.214788, theta = 0.214788, r - theta = 3.19345e-09
r = 0.215572, theta = 0.215572, r - theta = 2.19528e-09
r = 0.216356, theta = 0.216356, r - theta = -1.01946e-13
r = 0.217139, theta = 0.217139, r - theta = -2.37149e-09
r = 0.217923, theta = 0.217923, r - theta = -4.02095e-09
r = 0.218706, theta = 0.218706, r - theta = -4.17172e-09
r = 0.21949, theta = 0.21949, r - theta = -2.166e-09
r = 0.220273, theta = 0.220273, r - theta = 1.7875e-09
r = 0.221057, theta = 0.221057, r - theta = 3.19313e-09
r = 0.22184, theta = 0.22184, r - theta = 2.2462e-09
r = 0.222623, theta = 0.222623, r - theta = 8.254e-11
r = 0.223406, theta = 0.223406, r - theta = -2.28345e-09
r = 0.224189, theta = 0.224189, r - theta = -3.9567e-09
r = 0.224972, theta = 0.224972, r - theta = -4.15948e-09
r = 0.225755, theta = 0.225755, r - theta = -2.22938e-09
r = 0.226538, theta = 0.226538, r - theta = 1.70985e-09
r = 0.227321, theta = 0.227321, r - theta = 3.18986e-09
r = 0.228103, theta = 0.228103, r - theta = 2.30305e-09
r = 0.228886, theta = 0.228886, r - theta = 1.75149e-10
r = 0.229669, theta = 0.229669, r - theta = -2.18568e-09
r = 0.230451, theta = 0.230451, r - theta = -3.88689e-09
r = 0.231234, theta = 0.231234, r - theta = -4.14966e-09
r = 0.232016, theta = 0.232016, r - theta = -2.307e-09
r = 0.232798, theta = 0.232798, r - theta = 1.61322e-09
r = 0.23358, theta = 0.23358, r - theta = 3.18267e-09
r = 0.234363, theta = 0.234363, r - theta = 2.36543e-09
r = 0.235145, theta = 0.235145, r - theta = 2.77854e-10
r = 0.235927, theta = 0.235927, r - theta = -2.07762e-09
r = 0.236709, theta = 0.236709, r - theta = -3.81063e-09
r = 0.237491, theta = 0.237491, r - theta = -4.14116e-09
r = 0.238273, theta = 0.238273, r - theta = -2.3977e-09
r = 0.239054, theta = 0.239054, r - theta = 1.4957e-09
r = 0.239836, theta = 0.239836, r - theta = 3.17032e-09
r = 0.240618, theta = 0.240618, r - theta = 2.43276e-09
r = 0.241399, theta = 0.241399, r - theta = 3.90695e-10
r = 0.242181, theta = 0.242181, r - theta = -1.95869e-09
r = 0.242962, theta = 0.242962, r - theta = -3.72696e-09
r = 0.243743, theta = 0.243743, r - theta = -4.13274e-09
r = 0.244525, theta = 0.244525, r - theta = -2.50005e-09
r = 0.245306, theta = 0.245306, r - theta = 1.35506e-09
r = 0.246087, theta = 0.246087, r - theta = 3.15131e-09
r = 0.246868, theta = 0.246868, r - theta = 2.5043e-09
r = 0.247649, theta = 0.247649, r - theta = 5.13615e-10
r = 0.24843, theta = 0.24843, r - theta = -1.82836e-09
r = 0.249211, theta = 0.249211, r - theta = -3.63482e-09
r = 0.249992, theta = 0.249992, r - theta = -4.12296e-09
r = 0.250772, theta = 0.250772, r - theta = -2.61236e-09
r = 0.251553, theta = 0.251553, r - theta = 1.18866e-09
r = 0.252334, theta = 0.252334, r - theta = 3.12385e-09
r = 0.253114, theta = 0.253114, r - theta = 2.57908e-09
r = 0.253894, theta = 0.253894, r - theta = 6.46433e-10
r = 0.254675, theta = 0.254675, r - theta = -1.68611e-09
r = 0.255455, theta = 0.255455, r - theta = -3.53312e-09
r = 0.256235, theta = 0.256235, r - theta = -4.11023e-09
r = 0.257015, theta = 0.257015, r - theta = -2.73264e-09
r = 0.257795, theta = 0.257795, r - theta = 9.93445e-10
r = 0.258575, theta = 0.258575, r - theta = 3.08583e-09
r = 0.259355, theta = 0.259355, r - theta = 2.65592e-09
r = 0.260135, theta = 0.260135, r - theta = 7.88831e-10
r = 0.260915, theta = 0.260915, r - theta = -1.53147e-09
r = 0.261695, theta = 0.261695, r - theta = -3.42069e-09
r = 0.262474, theta = 0.262474, r - theta = -4.09281e-09
r = 0.263254, theta = 0.263254, r - theta = -2.85866e-09
r = 0.264033, theta = 0.264033, r - theta = 7.65918e-10
r = 0.264813, theta = 0.264813, r - theta = 3.03478e-09
r = 0.265592, theta = 0.265592, r - theta = 2.73337e-09
r = 0.266371, theta = 0.266371, r - theta = 9.40321e-10
r = 0.26715, theta = 0.26715, r - theta = -1.36403e-09
r = 0.267929, theta = 0.267929, r - theta = -3.29633e-09
r = 0.268708, theta = 0.268708, r - theta = -4.0688e-09
r = 0.269487, theta = 0.269487, r - theta = -2.98793e-09
r = 0.270266, theta = 0.270266, r - theta = 5.0207e-10
r = 0.271045, theta = 0.271045, r - theta = 2.96782e-09
r = 0.271824, theta = 0.271824, r - theta = 2.80966e-09
r = 0.272602, theta = 0.272602, r - theta = 1.10022e-09
r = 0.273381, theta = 0.273381, r - theta = -1.1835e-09
r = 0.274159, theta = 0.274159, r - theta = -3.15884e-09
r = 0.274938, theta = 0.274938, r - theta = -4.03618e-09
r = 0.275716, theta = 0.275716, r - theta = -3.1177e-09
r = 0.276494, theta = 0.276494, r - theta = 1.97349e-10
r = 0.277273, theta = 0.277273, r - theta = 2.88166e-09
r = 0.278051, theta = 0.278051, r - theta = 2.88272e-09
r = 0.278829, theta = 0.278829, r - theta = 1.26761e-09
r = 0.279607, theta = 0.279607, r - theta = -9.89684e-10
r = 0.280384, theta = 0.280384, r - theta = -3.00701e-09
r = 0.281162, theta = 0.281162, r - theta = -3.99282e-09
r = 0.28194, theta = 0.28194, r - theta = -3.24501e-09
r = 0.282717, theta = 0.282717, r - theta = -1.49653e-10
r = 0.283495, theta = 0.283495, r - theta = 2.77248e-09
r = 0.284272, theta = 0.284272, r - theta = 2.95008e-09
r = 0.28505, theta = 0.28505, r - theta = 1.44131e-09
r = 0.285827, theta = 0.285827, r - theta = -7.82555e-10
r = 0.286604, theta = 0.286604, r - theta = -2.83969e-09
r = 0.287382, theta = 0.287382, r - theta = -3.93653e-09
r = 0.288159, theta = 0.288159, r - theta = -3.36669e-09
r = 0.288936, theta = 0.288936, r - theta = -5.09773e-10
r = 0.289712, theta = 0.289712, r - theta = 2.63597e-09
r = 0.290489, theta = 0.290489, r - theta = 3.00885e-09
r = 0.291266, theta = 0.291266, r - theta = 1.61982e-09
r = 0.292043, theta = 0.292043, r - theta = -5.6229e-10
r = 0.292819, theta = 0.292819, r - theta = -2.65579e-09
r = 0.293596, theta = 0.293596, r - theta = -3.86506e-09
r = 0.294372, theta = 0.294372, r - theta = -3.47942e-09
r = 0.295148, theta = 0.295148, r - theta = -8.72106e-10
r = 0.295925, theta = 0.295925, r - theta = 2.46719e-09
r = 0.296701, theta = 0.296701, r - theta = 3.05566e-09
r = 0.297477, theta = 0.297477, r - theta = 1.80128e-09
r = 0.298253, theta = 0.298253, r - theta = -3.29303e-10
r = 0.299029, theta = 0.299029, r - theta = -2.45438e-09
r = 0.299805, theta = 0.299805, r - theta = -3.77616e-09
r = 0.30058, theta = 0.30058, r - theta = -3.57973e-09
r = 0.301356, theta = 0.301356, r - theta = -1.23204e-09
r = 0.302132, theta = 0.302132, r - theta = 2.26057e-09
r = 0.302907, theta = 0.302907, r - theta = 3.08661e-09
r = 0.303683, theta = 0.303683, r - theta = 1.9834e-09
r = 0.304458, theta = 0.304458, r - theta = -8.43067e-11
r = 0.305233, theta = 0.305233, r - theta = -2.23465e-09
r = 0.306008, theta = 0.306008, r - theta = -3.66762e-09
r = 0.306783, theta = 0.306783, r - theta = -3.66406e-09
r = 0.307558, theta = 0.307558, r - theta = -1.58473e-09
r = 0.308333, theta = 0.308333, r - theta = 2.00981e-09
r = 0.309108, theta = 0.309108, r - theta = 3.09718e-09
r = 0.309883, theta = 0.309883, r - theta = 2.16345e-09
r = 0.310658, theta = 0.310658, r - theta = 1.71638e-10
r = 0.311432, theta = 0.311432, r - theta = -1.99606e-09
r = 0.312207, theta = 0.312207, r - theta = -3.53734e-09
r = 0.312981, theta = 0.312981, r - theta = -3.72882e-09
r = 0.313755, theta = 0.313755, r - theta = -1.92516e-09
r = 0.31453, theta = 0.31453, r - theta = 1.70781e-09
r = 0.315304, theta = 0.315304, r - theta = 3.08221e-09
r = 0.316078, theta = 0.316078, r - theta = 2.33811e-09
r = 0.316852, theta = 0.316852, r - theta = 4.37055e-10
r = 0.317626, theta = 0.317626, r - theta = -1.73831e-09
r = 0.3184, theta = 0.3184, r - theta = -3.38333e-09
r = 0.319173, theta = 0.319173, r - theta = -3.77041e-09
r = 0.319947, theta = 0.319947, r - theta = -2.24818e-09
r = 0.32072, theta = 0.32072, r - theta = 1.34662e-09
r = 0.321494, theta = 0.321494, r - theta = 3.03579e-09
r = 0.322267, theta = 0.322267, r - theta = 2.50349e-09
r = 0.323041, theta = 0.323041, r - theta = 7.09988e-10
r = 0.323814, theta = 0.323814, r - theta = -1.46148e-09
r = 0.324587, theta = 0.324587, r - theta = -3.20384e-09
r = 0.32536, theta = 0.32536, r - theta = -3.78531e-09
r = 0.326133, theta = 0.326133, r - theta = -2.54856e-09
r = 0.326906, theta = 0.326906, r - theta = 9.17328e-10
r = 0.327678, theta = 0.327678, r - theta = 2.95117e-09
r = 0.328451, theta = 0.328451, r - theta = 2.65498e-09
r = 0.329224, theta = 0.329224, r - theta = 9.87922e-10
r = 0.329996, theta = 0.329996, r - theta = -1.16606e-09
r = 0.330769, theta = 0.330769, r - theta = -2.9974e-09
r = 0.331541, theta = 0.331541, r - theta = -3.77014e-09
r = 0.332313, theta = 0.332313, r - theta = -2.82107e-09
r = 0.333085, theta = 0.333085, r - theta = 4.09983e-10
r = 0.333857, theta = 0.333857, r - theta = 2.82073e-09
r = 0.334629, theta = 0.334629, r - theta = 2.78721e-09
r = 0.335401, theta = 0.335401, r - theta = 1.2677e-09
r = 0.336173, theta = 0.336173, r - theta = -8.53022e-10
r = 0.336945, theta = 0.336945, r - theta = -2.76289e-09
r = 0.337716, theta = 0.337716, r - theta = -3.72175e-09
r = 0.338488, theta = 0.338488, r - theta = -3.06057e-09
r = 0.339259, theta = 0.339259, r - theta = -1.80725e-10
r = 0.34003, theta = 0.34003, r - theta = 2.63581e-09
r = 0.340802, theta = 0.340802, r - theta = 2.89395e-09
r = 0.341573, theta = 0.341573, r - theta = 1.54546e-09
r = 0.342344, theta = 0.342344, r - theta = -5.23953e-10
r = 0.343115, theta = 0.343115, r - theta = -2.49963e-09
r = 0.343886, theta = 0.343886, r - theta = -3.63725e-09
r = 0.344656, theta = 0.344656, r - theta = -3.26203e-09
r = 0.345427, theta = 0.345427, r - theta = -7.68077e-10
r = 0.346198, theta = 0.346198, r - theta = 2.38669e-09
r = 0.346968, theta = 0.346968, r - theta = 2.968e-09
r = 0.347739, theta = 0.347739, r - theta = 1.81649e-09
r = 0.348509, theta = 0.348509, r - theta = -1.811e-10
r = 0.349279, theta = 0.349279, r - theta = -2.20748e-09
r = 0.350049, theta = 0.350049, r - theta = -3.51416e-09
r = 0.350819, theta = 0.350819, r - theta = -3.42069e-09
r = 0.351589, theta = 0.351589, r - theta = -1.31398e-09
r = 0.352359, theta = 0.352359, r - theta = 2.06241e-09
r = 0.353129, theta = 0.353129, r - theta = 3.0011e-09
r = 0.353899, theta = 0.353899, r - theta = 2.07516e-09
r = 0.354668, theta = 0.354668, r - theta = 1.72507e-10
r = 0.355438, theta = 0.355438, r - theta = -1.88693e-09
r = 0.356207, theta = 0.356207, r - theta = -3.35049e-09
r = 0.356976, theta = 0.356976, r - theta = -3.5321e-09
r = 0.357746, theta = 0.357746, r - theta = -1.8116e-09
r = 0.358515, theta = 0.358515, r - theta = 1.65071e-09
r = 0.359284, theta = 0.359284, r - theta = 2.98381e-09
r = 0.360053, theta = 0.360053, r - theta = 2.31484e-09
r = 0.360821, theta = 0.360821, r - theta = 5.32953e-10
r = 0.36159, theta = 0.36159, r - theta = -1.53921e-09
r = 0.362359, theta = 0.362359, r - theta = -3.14482e-09
r = 0.363127, theta = 0.363127, r - theta = -3.59224e-09
r = 0.363896, theta = 0.363896, r - theta = -2.25436e-09
r = 0.364664, theta = 0.364664, r - theta = 1.13791e-09
r = 0.365432, theta = 0.365432, r - theta = 2.90541e-09
r = 0.366201, theta = 0.366201, r - theta = 2.52772e-09
r = 0.366969, theta = 0.366969, r - theta = 8.95325e-10
r = 0.367737, theta = 0.367737, r - theta = -1.16638e-09
r = 0.368505, theta = 0.368505, r - theta = -2.89643e-09
r = 0.369272, theta = 0.369272, r - theta = -3.59765e-09
r = 0.37004, theta = 0.37004, r - theta = -2.6361e-09
r = 0.370808, theta = 0.370808, r - theta = 5.08752e-10
r = 0.371575, theta = 0.371575, r - theta = 2.75376e-09
r = 0.372342, theta = 0.372342, r - theta = 2.70474e-09
r = 0.37311, theta = 0.37311, r - theta = 1.25359e-09
r = 0.373877, theta = 0.373877, r - theta = -7.715e-10
r = 0.374644, theta = 0.374644, r - theta = -2.60542e-09
r = 0.375411, theta = 0.375411, r - theta = -3.54554e-09
r = 0.376178, theta = 0.376178, r - theta = -2.95113e-09
r = 0.376945, theta = 0.376945, r - theta = -2.42757e-10
r = 0.377712, theta = 0.377712, r - theta = 2.51516e-09
r = 0.378478, theta = 0.378478, r - theta = 2.83542e-09
r = 0.379245, theta = 0.379245, r - theta = 1.60046e-09
r = 0.380011, theta = 0.380011, r - theta = -3.5871e-10
r = 0.380778, theta = 0.380778, r - theta = -2.27284e-09
r = 0.381544, theta = 0.381544, r - theta = -3.43389e-09
r = 0.38231, theta = 0.38231, r - theta = -3.19443e-09
r = 0.383076, theta = 0.383076, r - theta = -9.67128e-10
r = 0.383842, theta = 0.383842, r - theta = 2.17425e-09
r = 0.384608, theta = 0.384608, r - theta = 2.90777e-09
r = 0.385373, theta = 0.385373, r - theta = 1.92727e-09
r = 0.386139, theta = 0.386139, r - theta = 6.66127e-11
r = 0.386905, theta = 0.386905, r - theta = -1.90081e-09
r = 0.38767, theta = 0.38767, r - theta = -3.2616e-09
r = 0.388435, theta = 0.388435, r - theta = -3.36173e-09
r = 0.389201, theta = 0.389201, r - theta = -1.60606e-09
r = 0.389966, theta = 0.389966, r - theta = 1.71381e-09
r = 0.390731, theta = 0.390731, r - theta = 2.9081e-09
r = 0.391496, theta = 0.391496, r - theta = 2.22381e-09
r = 0.392261, theta = 0.392261, r - theta = 4.9771e-10
r = 0.393025, theta = 0.393025, r - theta = -1.49269e-09
r = 0.39379, theta = 0.39379, r - theta = -3.02865e-09
r = 0.394555, theta = 0.394555, r - theta = -3.44965e-09
r = 0.395319, theta = 0.395319, r - theta = -2.15287e-09
r = 0.396084, theta = 0.396084, r - theta = 1.11467e-09
r = 0.396848, theta = 0.396848, r - theta = 2.82089e-09
r = 0.397612, theta = 0.397612, r - theta = 2.4782e-09
r = 0.398376, theta = 0.398376, r - theta = 9.263e-10
r = 0.39914, theta = 0.39914, r - theta = -1.05319e-09
r = 0.399904, theta = 0.399904, r - theta = -2.73625e-09
r = 0.400668, theta = 0.400668, r - theta = -3.45591e-09
r = 0.401431, theta = 0.401431, r - theta = -2.60179e-09
r = 0.402195, theta = 0.402195, r - theta = 3.55491e-10
r = 0.402958, theta = 0.402958, r - theta = 2.62863e-09
r = 0.403722, theta = 0.403722, r - theta = 2.67671e-09
r = 0.404485, theta = 0.404485, r - theta = 1.34241e-09
r = 0.405248, theta = 0.405248, r - theta = -5.88568e-10
r = 0.406011, theta = 0.406011, r - theta = -2.38694e-09
r = 0.406774, theta = 0.406774, r - theta = -3.3794e-09
r = 0.407537, theta = 0.407537, r - theta = -2.9481e-09
r = 0.4083, theta = 0.4083, r - theta = -5.30184e-10
r = 0.409062, theta = 0.409062, r - theta = 2.31164e-09
r = 0.409825, theta = 0.409825, r - theta = 2.80363e-09
r = 0.410587, theta = 0.410587, r - theta = 1.73422e-09
r = 0.41135, theta = 0.41135, r - theta = -1.06769e-10
r = 0.412112, theta = 0.412112, r - theta = -1.98485e-09
r = 0.412874, theta = 0.412874, r - theta = -3.22043e-09
r = 0.413636, theta = 0.413636, r - theta = -3.1883e-09
r = 0.414398, theta = 0.414398, r - theta = -1.31722e-09
r = 0.41516, theta = 0.41516, r - theta = 1.84793e-09
r = 0.415921, theta = 0.415921, r - theta = 2.84102e-09
r = 0.416683, theta = 0.416683, r - theta = 2.08787e-09
r = 0.417445, theta = 0.417445, r - theta = 3.82386e-10
r = 0.418206, theta = 0.418206, r - theta = -1.53579e-09
r = 0.418967, theta = 0.418967, r - theta = -2.98083e-09
r = 0.419728, theta = 0.419729, r - theta = -3.32029e-09
r = 0.42049, theta = 0.42049, r - theta = -1.97465e-09
r = 0.421251, theta = 0.421251, r - theta = 1.21297e-09
r = 0.422011, theta = 0.422011, r - theta = 2.76863e-09
r = 0.422772, theta = 0.422772, r - theta = 2.38731e-09
r = 0.423533, theta = 0.423533, r - theta = 8.67024e-10
r = 0.424294, theta = 0.424294, r - theta = -1.04747e-09
r = 0.425054, theta = 0.425054, r - theta = -2.6642e-09
r = 0.425814, theta = 0.425814, r - theta = -3.34355e-09
r = 0.426575, theta = 0.426575, r - theta = -2.49786e-09
r = 0.427335, theta = 0.427335, r - theta = 3.79537e-10
r = 0.428095, theta = 0.428095, r - theta = 2.56362e-09
r = 0.428855, theta = 0.428855, r - theta = 2.61409e-09
r = 0.429615, theta = 0.429615, r - theta = 1.33303e-09
r = 0.430374, theta = 0.430374, r - theta = -5.29692e-10
r = 0.431134, theta = 0.431134, r - theta = -2.27603e-09
r = 0.431894, theta = 0.431894, r - theta = -3.25931e-09
r = 0.432653, theta = 0.432653, r - theta = -2.88384e-09
r = 0.433412, theta = 0.433412, r - theta = -6.04487e-10
r = 0.434172, theta = 0.434172, r - theta = 2.20042e-09
r = 0.434931, theta = 0.434931, r - theta = 2.74715e-09
r = 0.43569, theta = 0.43569, r - theta = 1.76387e-09
r = 0.436449, theta = 0.436449, r - theta = 5.46635e-12
r = 0.437208, theta = 0.437208, r - theta = -1.82396e-09
r = 0.437966, theta = 0.437966, r - theta = -3.07078e-09
r = 0.438725, theta = 0.438725, r - theta = -3.13138e-09
r = 0.439483, theta = 0.439483, r - theta = -1.4518e-09
r = 0.440242, theta = 0.440242, r - theta = 1.6505e-09
r = 0.441, theta = 0.441, r - theta = 2.76266e-09
r = 0.441758, theta = 0.441758, r - theta = 2.14033e-09
r = 0.442516, theta = 0.442516, r - theta = 5.43444e-10
r = 0.443274, theta = 0.443274, r - theta = -1.31795e-09
r = 0.444032, theta = 0.444032, r - theta = -2.78333e-09
r = 0.44479, theta = 0.44479, r - theta = -3.24129e-09
r = 0.445547, theta = 0.445547, r - theta = -2.12916e-09
r = 0.446305, theta = 0.446305, r - theta = 8.82193e-10
r = 0.447062, theta = 0.447062, r - theta = 2.63376e-09
r = 0.44782, theta = 0.44782, r - theta = 2.44038e-09
r = 0.448577, theta = 0.448577, r - theta = 1.06698e-09
r = 0.449334, theta = 0.449334, r - theta = -7.70498e-10
r = 0.450091, theta = 0.450091, r - theta = -2.40472e-09
r = 0.450848, theta = 0.450848, r - theta = -3.21659e-09
r = 0.451605, theta = 0.451605, r - theta = -2.63489e-09
r = 0.452361, theta = 0.452361, r - theta = -1.35892e-10
r = 0.453118, theta = 0.453118, r - theta = 2.33038e-09
r = 0.453874, theta = 0.453874, r - theta = 2.63891e-09
r = 0.454631, theta = 0.454631, r - theta = 1.55591e-09
r = 0.455387, theta = 0.455387, r - theta = -1.96851e-10
r = 0.456143, theta = 0.456143, r - theta = -1.94531e-09
r = 0.456899, theta = 0.456899, r - theta = -3.06277e-09
r = 0.457655, theta = 0.457655, r - theta = -2.96958e-09
r = 0.458411, theta = 0.458411, r - theta = -1.13275e-09
r = 0.459166, theta = 0.459166, r - theta = 1.81898e-09
r = 0.459922, theta = 0.459922, r - theta = 2.70751e-09
r = 0.460678, theta = 0.460678, r - theta = 1.98691e-09
r = 0.461433, theta = 0.461433, r - theta = 3.84748e-10
r = 0.462188, theta = 0.462188, r - theta = -1.41827e-09
r = 0.462943, theta = 0.462943, r - theta = -2.78797e-09
r = 0.463698, theta = 0.463698, r - theta = -3.13636e-09
r = 0.464453, theta = 0.464453, r - theta = -1.92133e-09
r = 0.465208, theta = 0.465208, r - theta = 1.06238e-09
r = 0.465963, theta = 0.465963, r - theta = 2.61425e-09
r = 0.466718, theta = 0.466718, r - theta = 2.3333e-09
r = 0.467472, theta = 0.467472, r - theta = 9.5284e-10
r = 0.468226, theta = 0.468226, r - theta = -8.39855e-10
r = 0.468981, theta = 0.468981, r - theta = -2.40323e-09
r = 0.469735, theta = 0.469735, r - theta = -3.14109e-09
r = 0.470489, theta = 0.470489, r - theta = -2.50231e-09
r = 0.471243, theta = 0.471243, r - theta = 1.94248e-11
r = 0.471997, theta = 0.471997, r - theta = 2.32341e-09
r = 0.472751, theta = 0.472751, r - theta = 2.56477e-09
r = 0.473504, theta = 0.473504, r - theta = 1.48251e-09
r = 0.474258, theta = 0.474258, r - theta = -2.2962e-10
r = 0.475011, theta = 0.475011, r - theta = -1.92275e-09
r = 0.475764, theta = 0.475764, r - theta = -2.99262e-09
r = 0.476518, theta = 0.476518, r - theta = -2.87923e-09
r = 0.477271, theta = 0.477271, r - theta = -1.06656e-09
r = 0.478024, theta = 0.478024, r - theta = 1.79529e-09
r = 0.478777, theta = 0.478777, r - theta = 2.64717e-09
r = 0.479529, theta = 0.479529, r - theta = 1.94513e-09
r = 0.480282, theta = 0.480282, r - theta = 3.89333e-10
r = 0.481034, theta = 0.481034, r - theta = -1.36415e-09
r = 0.481787, theta = 0.481787, r - theta = -2.70307e-09
r = 0.482539, theta = 0.482539, r - theta = -3.05871e-09
r = 0.483291, theta = 0.483291, r - theta = -1.90555e-09
r = 0.484043, theta = 0.484043, r - theta = 9.85886e-10
r = 0.484795, theta = 0.484795, r - theta = 2.54222e-09
r = 0.485547, theta = 0.485547, r - theta = 2.30811e-09
r = 0.486299, theta = 0.486299, r - theta = 9.90092e-10
r = 0.487051, theta = 0.487051, r - theta = -7.48658e-10
r = 0.487802, theta = 0.487802, r - theta = -2.28803e-09
r = 0.488554, theta = 0.488554, r - theta = -3.05069e-09
r = 0.489305, theta = 0.489305, r - theta = -2.50178e-09
r = 0.490056, theta = 0.490056, r - theta = -1.48649e-10
r = 0.490807, theta = 0.490807, r - theta = 2.20727e-09
r = 0.491558, theta = 0.491558, r - theta = 2.53465e-09
r = 0.492309, theta = 0.492309, r - theta = 1.54167e-09
r = 0.49306, theta = 0.49306, r - theta = -1.01459e-10
r = 0.49381, theta = 0.49381, r - theta = -1.76687e-09
r = 0.494561, theta = 0.494561, r - theta = -2.86875e-09
r = 0.495311, theta = 0.495311, r - theta = -2.86305e-09
r = 0.496062, theta = 0.496062, r - theta = -1.24719e-09
r = 0.496812, theta = 0.496812, r - theta = 1.59502e-09
r = 0.497562, theta = 0.497562, r - theta = 2.58345e-09
r = 0.498312, theta = 0.498312, r - theta = 2.00876e-09
r = 0.499062, theta = 0.499062, r - theta = 5.48095e-10
r = 0.499812, theta = 0.499812, r - theta = -1.16301e-09
r = 0.500561, theta = 0.500561, r - theta = -2.53036e-09
r = 0.501311, theta = 0.501311, r - theta = -3.00085e-09
r = 0.50206, theta = 0.50206, r - theta = -2.06217e-09
r = 0.502809, theta = 0.502809, r - theta = 6.53272e-10
r = 0.503559, theta = 0.503559, r - theta = 2.40845e-09
r = 0.504308, theta = 0.504308, r - theta = 2.35142e-09
r = 0.505057, theta = 0.505057, r - theta = 1.16619e-09
r = 0.505805, theta = 0.505805, r - theta = -5.0414e-10
r = 0.506554, theta = 0.506554, r - theta = -2.05713e-09
r = 0.507303, theta = 0.507303, r - theta = -2.93073e-09
r = 0.508051, theta = 0.508051, r - theta = -2.60302e-09
r = 0.5088, theta = 0.5088, r - theta = -5.9196e-10
r = 0.509548, theta = 0.509548, r - theta = 1.95853e-09
r = 0.510296, theta = 0.510296, r - theta = 2.52481e-09
r = 0.511044, theta = 0.511044, r - theta = 1.71426e-09
r = 0.511792, theta = 0.511792, r - theta = 1.77423e-10
r = 0.51254, theta = 0.51254, r - theta = -1.47511e-09
r = 0.513288, theta = 0.513288, r - theta = -2.67248e-09
r = 0.514035, theta = 0.514035, r - theta = -2.88331e-09
r = 0.514783, theta = 0.514783, r - theta = -1.61545e-09
r = 0.51553, theta = 0.51553, r - theta = 1.17727e-09
r = 0.516277, theta = 0.516277, r - theta = 2.47895e-09
r = 0.517024, theta = 0.517024, r - theta = 2.14871e-09
r = 0.517772, theta = 0.517772, r - theta = 8.44495e-10
r = 0.518518, theta = 0.518518, r - theta = -8.15088e-10
r = 0.519265, theta = 0.519265, r - theta = -2.2505e-09
r = 0.520012, theta = 0.520012, r - theta = -2.92105e-09
r = 0.520759, theta = 0.520759, r - theta = -2.3246e-09
r = 0.521505, theta = 0.521505, r - theta = 2.59381e-12
r = 0.522251, theta = 0.522251, r - theta = 2.15834e-09
r = 0.522998, theta = 0.522998, r - theta = 2.42064e-09
r = 0.523744, theta = 0.523744, r - theta = 1.45471e-09
r = 0.52449, theta = 0.52449, r - theta = -1.12891e-10
r = 0.525236, theta = 0.525236, r - theta = -1.69408e-09
r = 0.525981, theta = 0.525981, r - theta = -2.73896e-09
r = 0.526727, theta = 0.526727, r - theta = -2.73561e-09
r = 0.527473, theta = 0.527473, r - theta = -1.20985e-09
r = 0.528218, theta = 0.528218, r - theta = 1.50173e-09
r = 0.528963, theta = 0.528963, r - theta = 2.47549e-09
r = 0.529708, theta = 0.529708, r - theta = 1.96024e-09
r = 0.530454, theta = 0.530454, r - theta = 5.90341e-10
r = 0.531199, theta = 0.531199, r - theta = -1.03767e-09
r = 0.531943, theta = 0.531943, r - theta = -2.36483e-09
r = 0.532688, theta = 0.532688, r - theta = -2.86957e-09
r = 0.533433, theta = 0.533433, r - theta = -2.06743e-09
r = 0.534177, theta = 0.534177, r - theta = 4.41779e-10
r = 0.534922, theta = 0.534922, r - theta = 2.25281e-09
r = 0.535666, theta = 0.535666, r - theta = 2.30746e-09
r = 0.53641, theta = 0.53641, r - theta = 1.24783e-09
r = 0.537154, theta = 0.537154, r - theta = -3.21212e-10
r = 0.537898, theta = 0.537898, r - theta = -1.83177e-09
r = 0.538642, theta = 0.538642, r - theta = -2.75275e-09
r = 0.539386, theta = 0.539386, r - theta = -2.58958e-09
r = 0.540129, theta = 0.540129, r - theta = -8.84043e-10
r = 0.540873, theta = 0.540873, r - theta = 1.68586e-09
r = 0.541616, theta = 0.541616, r - theta = 2.43664e-09
r = 0.542359, theta = 0.542359, r - theta = 1.80683e-09
r = 0.543102, theta = 0.543102, r - theta = 4.09539e-10
r = 0.543845, theta = 0.543845, r - theta = -1.17855e-09
r = 0.544588, theta = 0.544588, r - theta = -2.41694e-09
r = 0.545331, theta = 0.545331, r - theta = -2.80112e-09
r = 0.546074, theta = 0.546074, r - theta = -1.86232e-09
r = 0.546816, theta = 0.546816, r - theta = 7.01386e-10
r = 0.547559, theta = 0.547559, r - theta = 2.28167e-09
r = 0.548301, theta = 0.548301, r - theta = 2.20837e-09
r = 0.549043, theta = 0.549043, r - theta = 1.10272e-09
r = 0.549785, theta = 0.549785, r - theta = -4.49917e-10
r = 0.550527, theta = 0.550527, r - theta = -1.89978e-09
r = 0.551269, theta = 0.551269, r - theta = -2.73254e-09
r = 0.552011, theta = 0.552011, r - theta = -2.46908e-09
r = 0.552752, theta = 0.552752, r - theta = -6.65289e-10
r = 0.553494, theta = 0.553494, r - theta = 1.76967e-09
r = 0.554235, theta = 0.554235, r - theta = 2.38686e-09
r = 0.554977, theta = 0.554977, r - theta = 1.70002e-09
r = 0.555718, theta = 0.555718, r - theta = 3.03087e-10
r = 0.556459, theta = 0.556459, r - theta = -1.24504e-09
r = 0.5572, theta = 0.5572, r - theta = -2.42036e-09
r = 0.55794, theta = 0.55794, r - theta = -2.73355e-09
r = 0.558681, theta = 0.558681, r - theta = -1.72973e-09
r = 0.559422, theta = 0.559422, r - theta = 8.20727e-10
r = 0.560162, theta = 0.560162, r - theta = 2.26979e-09
r = 0.560902, theta = 0.560902, r - theta = 2.13634e-09
r = 0.561643, theta = 0.561643, r - theta = 1.02279e-09
r = 0.562383, theta = 0.562383, r - theta = -5.02991e-10
r = 0.563123, theta = 0.563123, r - theta = -1.90744e-09
r = 0.563863, theta = 0.563863, r - theta = -2.69117e-09
r = 0.564602, theta = 0.564602, r - theta = -2.38873e-09
r = 0.565342, theta = 0.565342, r - theta = -5.68417e-10
r = 0.566081, theta = 0.566081, r - theta = 1.77741e-09
r = 0.566821, theta = 0.566821, r - theta = 2.3395e-09
r = 0.56756, theta = 0.56756, r - theta = 1.64454e-09
r = 0.568299, theta = 0.568299, r - theta = 2.69283e-10
r = 0.569038, theta = 0.569038, r - theta = -1.24332e-09
r = 0.569777, theta = 0.569777, r - theta = -2.38394e-09
r = 0.570516, theta = 0.570516, r - theta = -2.67669e-09
r = 0.571255, theta = 0.571255, r - theta = -1.67893e-09
r = 0.571993, theta = 0.571993, r - theta = 8.22369e-10
r = 0.572732, theta = 0.572732, r - theta = 2.22986e-09
r = 0.57347, theta = 0.57347, r - theta = 2.09641e-09
r = 0.574208, theta = 0.574208, r - theta = 1.00753e-09
r = 0.574946, theta = 0.574946, r - theta = -4.84542e-10
r = 0.575684, theta = 0.575684, r - theta = -1.8607e-09
r = 0.576422, theta = 0.576422, r - theta = -2.63477e-09
r = 0.57716, theta = 0.57716, r - theta = -2.35334e-09
r = 0.577898, theta = 0.577898, r - theta = -5.95564e-10
r = 0.578635, theta = 0.578635, r - theta = 1.71998e-09
r = 0.579373, theta = 0.579373, r - theta = 2.29879e-09
r = 0.58011, theta = 0.58011, r - theta = 1.64e-09
r = 0.580847, theta = 0.580847, r - theta = 3.04962e-10
r = 0.581584, theta = 0.581584, r - theta = -1.17755e-09
r = 0.582321, theta = 0.582321, r - theta = -2.31123e-09
r = 0.583058, theta = 0.583058, r - theta = -2.63203e-09
r = 0.583795, theta = 0.583795, r - theta = -1.70801e-09
r = 0.584531, theta = 0.584531, r - theta = 7.14264e-10
r = 0.585268, theta = 0.585268, r - theta = 2.1641e-09
r = 0.586004, theta = 0.586004, r - theta = 2.08707e-09
r = 0.58674, theta = 0.58674, r - theta = 1.05356e-09
r = 0.587476, theta = 0.587476, r - theta = -3.98147e-10
r = 0.588212, theta = 0.588212, r - theta = -1.76174e-09
r = 0.588948, theta = 0.588948, r - theta = -2.5627e-09
r = 0.589684, theta = 0.589684, r - theta = -2.35814e-09
r = 0.590419, theta = 0.590419, r - theta = -7.36627e-10
r = 0.591155, theta = 0.591155, r - theta = 1.59639e-09
r = 0.59189, theta = 0.59189, r - theta = 2.26091e-09
r = 0.592626, theta = 0.592626, r - theta = 1.68154e-09
r = 0.593361, theta = 0.593361, r - theta = 4.05919e-10
r = 0.594096, theta = 0.594096, r - theta = -1.04978e-09
r = 0.594831, theta = 0.594831, r - theta = -2.2007e-09
r = 0.595565, theta = 0.595565, r - theta = -2.59318e-09
r = 0.5963, theta = 0.5963, r - theta = -1.80453e-09
r = 0.597035, theta = 0.597035, r - theta = 4.90976e-10
r = 0.597769, theta = 0.597769, r - theta = 2.06514e-09
r = 0.598503, theta = 0.598503, r - theta = 2.10076e-09
r = 0.599238, theta = 0.599238, r - theta = 1.15477e-09
r = 0.599972, theta = 0.599972, r - theta = -2.4696e-10
r = 0.600706, theta = 0.600706, r - theta = -1.60939e-09
r = 0.60144, theta = 0.60144, r - theta = -2.46821e-09
r = 0.602173, theta = 0.602173, r - theta = -2.38966e-09
r = 0.602907, theta = 0.602907, r - theta = -9.70356e-10
r = 0.60364, theta = 0.60364, r - theta = 1.3944e-09
r = 0.604374, theta = 0.604374, r - theta = 2.2143e-09
r = 0.605107, theta = 0.605107, r - theta = 1.75986e-09
r = 0.60584, theta = 0.60584, r - theta = 5.66573e-10
r = 0.606573, theta = 0.606573, r - theta = -8.60501e-10
r = 0.607306, theta = 0.607306, r - theta = -2.04656e-09
r = 0.608039, theta = 0.608039, r - theta = -2.54691e-09
r = 0.608771, theta = 0.608771, r - theta = -1.94684e-09
r = 0.609504, theta = 0.609504, r - theta = 1.34045e-10
r = 0.610236, theta = 0.610236, r - theta = 1.91604e-09
r = 0.610969, theta = 0.610969, r - theta = 2.12364e-09
r = 0.611701, theta = 0.611701, r - theta = 1.3018e-09
r = 0.612433, theta = 0.612433, r - theta = -3.45137e-11
r = 0.613165, theta = 0.613165, r - theta = -1.40016e-09
r = 0.613897, theta = 0.613897, r - theta = -2.33969e-09
r = 0.614628, theta = 0.614628, r - theta = -2.42716e-09
r = 0.61536, theta = 0.61536, r - theta = -1.26605e-09
r = 0.616091, theta = 0.616091, r - theta = 1.09033e-09
r = 0.616823, theta = 0.616823, r - theta = 2.13914e-09
r = 0.617554, theta = 0.617554, r - theta = 1.86044e-09
r = 0.618285, theta = 0.618285, r - theta = 7.78971e-10
r = 0.619016, theta = 0.619016, r - theta = -6.09937e-10
r = 0.619747, theta = 0.619747, r - theta = -1.8402e-09
r = 0.620478, theta = 0.620478, r - theta = -2.47486e-09
r = 0.621208, theta = 0.621208, r - theta = -2.10592e-09
r = 0.621939, theta = 0.621939, r - theta = -3.54214e-10
r = 0.622669, theta = 0.622669, r - theta = 1.68962e-09
r = 0.623399, theta = 0.623399, r - theta = 2.13458e-09
r = 0.62413, theta = 0.62413, r - theta = 1.48086e-09
r = 0.62486, theta = 0.62486, r - theta = 2.33845e-10
r = 0.625589, theta = 0.625589, r - theta = -1.1299e-09
r = 0.626319, theta = 0.626319, r - theta = -2.16254e-09
r = 0.627049, theta = 0.627049, r - theta = -2.44478e-09
r = 0.627778, theta = 0.627778, r - theta = -1.58577e-09
r = 0.628508, theta = 0.628508, r - theta = 6.48157e-10
r = 0.629237, theta = 0.629237, r - theta = 2.00626e-09
r = 0.629966, theta = 0.629966, r - theta = 1.96216e-09
r = 0.630695, theta = 0.630695, r - theta = 1.03117e-09
r = 0.631424, theta = 0.631424, r - theta = -2.99837e-10
r = 0.632153, theta = 0.632153, r - theta = -1.57227e-09
r = 0.632882, theta = 0.632882, r - theta = -2.35572e-09
r = 0.633611, theta = 0.633611, r - theta = -2.2478e-09
r = 0.634339, theta = 0.634339, r - theta = -8.74021e-10
r = 0.635067, theta = 0.635067, r - theta = 1.34714e-09
r = 0.635796, theta = 0.635796, r - theta = 2.10363e-09
r = 0.636524, theta = 0.636524, r - theta = 1.67188e-09
r = 0.637252, theta = 0.637252, r - theta = 5.48953e-10
r = 0.63798, theta = 0.63798, r - theta = -7.95989e-10
r = 0.638707, theta = 0.638707, r - theta = -1.92155e-09
r = 0.639435, theta = 0.639435, r - theta = -2.41398e-09
r = 0.640162, theta = 0.640162, r - theta = -1.88705e-09
r = 0.64089, theta = 0.64089, r - theta = 1.80171e-11
r = 0.641617, theta = 0.641617, r - theta = 1.77536e-09
r = 0.642344, theta = 0.642344, r - theta = 2.03533e-09
r = 0.643071, theta = 0.643071, r - theta = 1.30505e-09
r = 0.643798, theta = 0.643798, r - theta = 6.41419e-11
r = 0.644525, theta = 0.644525, r - theta = -1.23518e-09
r = 0.645252, theta = 0.645252, r - theta = -2.16796e-09
r = 0.645978, theta = 0.645978, r - theta = -2.33638e-09
r = 0.646705, theta = 0.646705, r - theta = -1.36963e-09
r = 0.647431, theta = 0.647431, r - theta = 8.36334e-10
r = 0.648157, theta = 0.648157, r - theta = 1.9899e-09
r = 0.648883, theta = 0.648883, r - theta = 1.84625e-09
r = 0.649609, theta = 0.649609, r - theta = 8.95312e-10
r = 0.650335, theta = 0.650335, r - theta = -4.00024e-10
r = 0.651061, theta = 0.651061, r - theta = -1.60375e-09
r = 0.651786, theta = 0.651786, r - theta = -2.30661e-09
r = 0.652512, theta = 0.652512, r - theta = -2.12603e-09
r = 0.653237, theta = 0.653237, r - theta = -7.05913e-10
r = 0.653962, theta = 0.653962, r - theta = 1.39267e-09
r = 0.654687, theta = 0.654687, r - theta = 2.03917e-09
r = 0.655412, theta = 0.655412, r - theta = 1.57367e-09
r = 0.656137, theta = 0.656137, r - theta = 4.6961e-10
r = 0.656862, theta = 0.656862, r - theta = -8.26102e-10
r = 0.657586, theta = 0.657586, r - theta = -1.89296e-09
r = 0.658311, theta = 0.658311, r - theta = -2.33673e-09
r = 0.659035, theta = 0.659035, r - theta = -1.78936e-09
r = 0.65976, theta = 0.65976, r - theta = 8.89858e-11
r = 0.660484, theta = 0.660484, r - theta = 1.73883e-09
r = 0.661208, theta = 0.661208, r - theta = 1.96392e-09
r = 0.661932, theta = 0.661932, r - theta = 1.2481e-09
r = 0.662655, theta = 0.662655, r - theta = 4.9038e-11
r = 0.663379, theta = 0.663379, r - theta = -1.20165e-09
r = 0.664103, theta = 0.664103, r - theta = -2.09828e-09
r = 0.664826, theta = 0.664826, r - theta = -2.26096e-09
r = 0.665549, theta = 0.665549, r - theta = -1.33554e-09
r = 0.666272, theta = 0.666272, r - theta = 7.88229e-10
r = 0.666996, theta = 0.666996, r - theta = 1.91891e-09
r = 0.667718, theta = 0.667718, r - theta = 1.79812e-09
r = 0.668441, theta = 0.668441, r - theta = 8.9417e-10
r = 0.669164, theta = 0.669164, r - theta = -3.50372e-10
r = 0.669887, theta = 0.669887, r - theta = -1.5185e-09
r = 0.670609, theta = 0.670609, r - theta = -2.21869e-09
r = 0.671331, theta = 0.671331, r - theta = -2.08477e-09
r = 0.672054, theta = 0.672054, r - theta = -7.75838e-10
r = 0.672776, theta = 0.672776, r - theta = 1.27916e-09
r = 0.673498, theta = 0.673498, r - theta = 1.9702e-09
r = 0.674219, theta = 0.674219, r - theta = 1.5698e-09
r = 0.674941, theta = 0.674941, r - theta = 5.31365e-10
r = 0.675663, theta = 0.675663, r - theta = -7.16939e-10
r = 0.676384, theta = 0.676384, r - theta = -1.77209e-09
r = 0.677106, theta = 0.677106, r - theta = -2.25609e-09
r = 0.677827, theta = 0.677827, r - theta = -1.81587e-09
r = 0.678548, theta = 0.678548, r - theta = -1.23152e-10
r = 0.679269, theta = 0.679269, r - theta = 1.60229e-09
r = 0.67999, theta = 0.67999, r - theta = 1.92401e-09
r = 0.680711, theta = 0.680711, r - theta = 1.30177e-09
r = 0.681431, theta = 0.681431, r - theta = 1.74753e-10
r = 0.682152, theta = 0.682152, r - theta = -1.04268e-09
r = 0.682872, theta = 0.682872, r - theta = -1.96087e-09
r = 0.683592, theta = 0.683592, r - theta = -2.21474e-09
r = 0.684313, theta = 0.684313, r - theta = -1.46369e-09
r = 0.685033, theta = 0.685033, r - theta = 5.19364e-10
r = 0.685752, theta = 0.685752, r - theta = 1.79205e-09
r = 0.686472, theta = 0.686472, r - theta = 1.80637e-09
r = 0.687192, theta = 0.687192, r - theta = 1.01237e-09
r = 0.687911, theta = 0.687911, r - theta = -1.64401e-10
r = 0.688631, theta = 0.688631, r - theta = -1.32275e-09
r = 0.68935, theta = 0.68935, r - theta = -2.08578e-09
r = 0.690069, theta = 0.690069, r - theta = -2.10075e-09
r = 0.690788, theta = 0.690788, r - theta = -1.03898e-09
r = 0.691507, theta = 0.691507, r - theta = 9.95874e-10
r = 0.692226, theta = 0.692226, r - theta = 1.87755e-09
r = 0.692945, theta = 0.692945, r - theta = 1.63872e-09
r = 0.693664, theta = 0.693664, r - theta = 7.16005e-10
r = 0.694382, theta = 0.694382, r - theta = -4.78059e-10
r = 0.6951, theta = 0.6951, r - theta = -1.55488e-09
r = 0.695819, theta = 0.695819, r - theta = -2.14975e-09
r = 0.696537, theta = 0.696537, r - theta = -1.92168e-09
r = 0.697255, theta = 0.697255, r - theta = -5.53373e-10
r = 0.697972, theta = 0.697972, r - theta = 1.33411e-09
r = 0.69869, theta = 0.69869, r - theta = 1.88321e-09
r = 0.699408, theta = 0.699408, r - theta = 1.43851e-09
r = 0.700125, theta = 0.700125, r - theta = 4.23766e-10
r = 0.700843, theta = 0.700843, r - theta = -7.60901e-10
r = 0.70156, theta = 0.70156, r - theta = -1.73894e-09
r = 0.702277, theta = 0.702277, r - theta = -2.15726e-09
r = 0.702994, theta = 0.702994, r - theta = -1.68612e-09
r = 0.703711, theta = 0.703711, r - theta = -1.90458e-11
r = 0.704428, theta = 0.704428, r - theta = 1.56135e-09
r = 0.705144, theta = 0.705144, r - theta = 1.82936e-09
r = 0.705861, theta = 0.705861, r - theta = 1.21972e-09
r = 0.706577, theta = 0.706577, r - theta = 1.43905e-10
r = 0.707294, theta = 0.707294, r - theta = -1.00986e-09
r = 0.70801, theta = 0.70801, r - theta = -1.8765e-09
r = 0.708726, theta = 0.708726, r - theta = -2.11401e-09
r = 0.709442, theta = 0.709442, r - theta = -1.40337e-09
r = 0.710158, theta = 0.710158, r - theta = 4.74342e-10
r = 0.710873, theta = 0.710873, r - theta = 1.70067e-09
r = 0.711589, theta = 0.711589, r - theta = 1.73279e-09
r = 0.712304, theta = 0.712304, r - theta = 9.93416e-10
r = 0.71302, theta = 0.71302, r - theta = -1.17708e-10
r = 0.713735, theta = 0.713735, r - theta = -1.2237e-09
r = 0.71445, theta = 0.71445, r - theta = -1.97044e-09
r = 0.715165, theta = 0.715165, r - theta = -2.02652e-09
r = 0.71588, theta = 0.71588, r - theta = -1.0831e-09
r = 0.716594, theta = 0.716594, r - theta = 8.49887e-10
r = 0.717309, theta = 0.717309, r - theta = 1.77149e-09
r = 0.718023, theta = 0.718023, r - theta = 1.6072e-09
r = 0.718738, theta = 0.718738, r - theta = 7.68128e-10
r = 0.719452, theta = 0.719452, r - theta = -3.5718e-10
r = 0.720166, theta = 0.720166, r - theta = -1.40267e-09
r = 0.72088, theta = 0.72088, r - theta = -2.02468e-09
r = 0.721594, theta = 0.721594, r - theta = -1.90187e-09
r = 0.722308, theta = 0.722308, r - theta = -7.35119e-10
r = 0.723021, theta = 0.723021, r - theta = 1.12941e-09
r = 0.723735, theta = 0.723735, r - theta = 1.79005e-09
r = 0.724448, theta = 0.724448, r - theta = 1.46366e-09
r = 0.725161, theta = 0.725161, r - theta = 5.50281e-10
r = 0.725875, theta = 0.725875, r - theta = -5.72233e-10
r = 0.726588, theta = 0.726588, r - theta = -1.54816e-09
r = 0.727301, theta = 0.727301, r - theta = -2.0438e-09
r = 0.728013, theta = 0.728013, r - theta = -1.74742e-09
r = 0.728726, theta = 0.728726, r - theta = -3.69145e-10
r = 0.729438, theta = 0.729438, r - theta = 1.33152e-09
r = 0.730151, theta = 0.730151, r - theta = 1.76981e-09
r = 0.730863, theta = 0.730863, r - theta = 1.31099e-09
r = 0.731575, theta = 0.731575, r - theta = 3.44539e-10
r = 0.732287, theta = 0.732287, r - theta = -7.61891e-10
r = 0.732999, theta = 0.732999, r - theta = -1.66243e-09
r = 0.733711, theta = 0.733711, r - theta = -2.03287e-09
r = 0.734423, theta = 0.734423, r - theta = -1.57062e-09
r = 0.735134, theta = 0.735134, r - theta = 5.4029e-12
r = 0.735846, theta = 0.735846, r - theta = 1.47193e-09
r = 0.736557, theta = 0.736557, r - theta = 1.72184e-09
r = 0.737268, theta = 0.737268, r - theta = 1.1561e-09
r = 0.737979, theta = 0.737979, r - theta = 1.5412e-10
r = 0.73869, theta = 0.73869, r - theta = -9.26191e-10
r = 0.739401, theta = 0.739401, r - theta = -1.74834e-09
r = 0.740112, theta = 0.740112, r - theta = -1.99717e-09
r = 0.740823, theta = 0.740823, r - theta = -1.37877e-09
r = 0.741533, theta = 0.741533, r - theta = 3.39327e-10
r = 0.742243, theta = 0.742243, r - theta = 1.56385e-09
r = 0.742954, theta = 0.742954, r - theta = 1.65517e-09
r = 0.743664, theta = 0.743664, r - theta = 1.00432e-09
r = 0.744374, theta = 0.744374, r - theta = -1.893e-11
r = 0.745084, theta = 0.745084, r - theta = -1.06594e-09
r = 0.745793, theta = 0.745793, r - theta = -1.80916e-09
r = 0.746503, theta = 0.746503, r - theta = -1.94202e-09
r = 0.747213, theta = 0.747213, r - theta = -1.17889e-09
r = 0.747922, theta = 0.747922, r - theta = 6.01646e-10
r = 0.748631, theta = 0.748631, r - theta = 1.61828e-09
r = 0.74934, theta = 0.74934, r - theta = 1.57711e-09
r = 0.750049, theta = 0.750049, r - theta = 8.59646e-10
r = 0.750758, theta = 0.750758, r - theta = -1.73491e-10
r = 0.751467, theta = 0.751467, r - theta = -1.18251e-09
r = 0.752176, theta = 0.752176, r - theta = -1.84835e-09
r = 0.752884, theta = 0.752884, r - theta = -1.87264e-09
r = 0.753593, theta = 0.753593, r - theta = -9.77577e-10
r = 0.754301, theta = 0.754301, r - theta = 8.05213e-10
r = 0.755009, theta = 0.755009, r - theta = 1.64435e-09
r = 0.755717, theta = 0.755717, r - theta = 1.49346e-09
r = 0.756425, theta = 0.756425, r - theta = 7.24996e-10
r = 0.757133, theta = 0.757133, r - theta = -3.09162e-10
r = 0.757841, theta = 0.757841, r - theta = -1.27763e-09
r = 0.758549, theta = 0.758549, r - theta = -1.86945e-09
r = 0.759256, theta = 0.759256, r - theta = -1.79399e-09
r = 0.759963, theta = 0.759963, r - theta = -7.80906e-10
r = 0.760671, theta = 0.760671, r - theta = 9.60846e-10
r = 0.761378, theta = 0.761378, r - theta = 1.64956e-09
r = 0.762085, theta = 0.762085, r - theta = 1.40881e-09
r = 0.762792, theta = 0.762792, r - theta = 6.02407e-10
r = 0.763498, theta = 0.763498, r - theta = -4.2608e-10
r = 0.764205, theta = 0.764205, r - theta = -1.35327e-09
r = 0.764911, theta = 0.764911, r - theta = -1.87588e-09
r = 0.765618, theta = 0.765618, r - theta = -1.71066e-09
r = 0.766324, theta = 0.766324, r - theta = -5.94323e-10
r = 0.76703, theta = 0.76703, r - theta = 1.07759e-09
r = 0.767736, theta = 0.767736, r - theta = 1.64001e-09
r = 0.768442, theta = 0.768442, r - theta = 1.32669e-09
r = 0.769148, theta = 0.769148, r - theta = 4.93216e-10
r = 0.769854, theta = 0.769854, r - theta = -5.24763e-10
r = 0.770559, theta = 0.770559, r - theta = -1.41146e-09
r = 0.771265, theta = 0.771265, r - theta = -1.8709e-09
r = 0.77197, theta = 0.77197, r - theta = -1.62681e-09
r = 0.772675, theta = 0.772675, r - theta = -4.2259e-10
r = 0.77338, theta = 0.77338, r - theta = 1.16293e-09
r = 0.774085, theta = 0.774085, r - theta = 1.62062e-09
r = 0.77479, theta = 0.77479, r - theta = 1.24979e-09
r = 0.775494, theta = 0.775494, r - theta = 3.98218e-10
r = 0.776199, theta = 0.776199, r - theta = -6.0598e-10
r = 0.776904, theta = 0.776904, r - theta = -1.45424e-09
r = 0.777608, theta = 0.777608, r - theta = -1.85748e-09
r = 0.778312, theta = 0.778312, r - theta = -1.54608e-09
r = 0.779016, theta = 0.779016, r - theta = -2.69739e-10
r = 0.77972, theta = 0.77972, r - theta = 1.22303e-09
r = 0.780424, theta = 0.780424, r - theta = 1.59528e-09
r = 0.781128, theta = 0.781128, r - theta = 1.18009e-09
r = 0.781831, theta = 0.781831, r - theta = 3.1779e-10
r = 0.782535, theta = 0.782535, r - theta = -6.70646e-10
r = 0.783238, theta = 0.783238, r - theta = -1.4835e-09
r = 0.783942, theta = 0.783942, r - theta = -1.83825e-09
r = 0.784645, theta = 0.784645, r - theta = -1.47154e-09
r = 0.785348, theta = 0.785348, r - theta = -1.3904e-10
r = 0.786051, theta = 0.786051, r - theta = 1.26284e-09
r = 0.786753, theta = 0.786753, r - theta = 1.56702e-09
r = 0.787456, theta = 0.787456, r - theta = 1.11899e-09
r = 0.788159, theta = 0.788159, r - theta = 2.51999e-10
r = 0.788861, theta = 0.788861, r - theta = -7.19735e-10
r = 0.789563, theta = 0.789563, r - theta = -1.50099e-09
r = 0.790265, theta = 0.790265, r - theta = -1.81546e-09
r = 0.790967, theta = 0.790967, r - theta = -1.40568e-09
r = 0.791669, theta = 0.791669, r - theta = -3.29877e-11
r = 0.792371, theta = 0.792371, r - theta = 1.28633e-09
r = 0.793073, theta = 0.793073, r - theta = 1.53812e-09
r = 0.793774, theta = 0.793774, r - theta = 1.0674e-09
r = 0.794476, theta = 0.794476, r - theta = 2.00691e-10
r = 0.795177, theta = 0.795177, r - theta = -7.5421e-10
r = 0.795879, theta = 0.795879, r - theta = -1.50822e-09
r = 0.79658, theta = 0.79658, r - theta = -1.7909e-09
r = 0.797281, theta = 0.797281, r - theta = -1.35038e-09
r = 0.797981, theta = 0.797981, r - theta = 4.59505e-11
r = 0.798682, theta = 0.798682, r - theta = 1.29652e-09
r = 0.799383, theta = 0.799383, r - theta = 1.51024e-09
r = 0.800083, theta = 0.800083, r - theta = 1.02586e-09
r = 0.800784, theta = 0.800784, r - theta = 1.63555e-10
r = 0.801484, theta = 0.801484, r - theta = -7.74972e-10
r = 0.802184, theta = 0.802184, r - theta = -1.50645e-09
r = 0.802884, theta = 0.802884, r - theta = -1.76596e-09
r = 0.803584, theta = 0.803584, r - theta = -1.30691e-09
r = 0.804284, theta = 0.804284, r - theta = 9.57074e-11
r = 0.804983, theta = 0.804983, r - theta = 1.29566e-09
r = 0.805683, theta = 0.805683, r - theta = 1.48448e-09
r = 0.806382, theta = 0.806382, r - theta = 9.94598e-10
r = 0.807082, theta = 0.807082, r - theta = 1.4018e-10
r = 0.807781, theta = 0.807781, r - theta = -7.82828e-10
r = 0.80848, theta = 0.80848, r - theta = -1.49664e-09
r = 0.809179, theta = 0.809179, r - theta = -1.74156e-09
r = 0.809878, theta = 0.809878, r - theta = -1.27594e-09
r = 0.810576, theta = 0.810576, r - theta = 1.18612e-10
r = 0.811275, theta = 0.811275, r - theta = 1.2853e-09
r = 0.811974, theta = 0.811974, r - theta = 1.46146e-09
r = 0.812672, theta = 0.812672, r - theta = 9.73567e-10
r = 0.81337, theta = 0.81337, r - theta = 1.30088e-10
r = 0.814068, theta = 0.814068, r - theta = -7.78467e-10
r = 0.814766, theta = 0.814766, r - theta = -1.47948e-09
r = 0.815464, theta = 0.815464, r - theta = -1.71818e-09
r = 0.816162, theta = 0.816162, r - theta = -1.25755e-09
r = 0.816859, theta = 0.816859, r - theta = 1.16502e-10
r = 0.817557, theta = 0.817557, r - theta = 1.26633e-09
r = 0.818254, theta = 0.818254, r - theta = 1.44138e-09
r = 0.818952, theta = 0.818952, r - theta = 9.62495e-10
r = 0.819649, theta = 0.819649, r - theta = 1.32756e-10
r = 0.820346, theta = 0.820346, r - theta = -7.62454e-10
r = 0.821043, theta = 0.821043, r - theta = -1.45539e-09
r = 0.82174, theta = 0.82174, r - theta = -1.69588e-09
r = 0.822436, theta = 0.822436, r - theta = -1.25129e-09
r = 0.823133, theta = 0.823133, r - theta = 9.0416e-11
r = 0.823829, theta = 0.823829, r - theta = 1.23906e-09
r = 0.824526, theta = 0.824526, r - theta = 1.42405e-09
r = 0.825222, theta = 0.825222, r - theta = 9.6091e-10
r = 0.825918, theta = 0.825918, r - theta = 1.47629e-10
r = 0.826614, theta = 0.826614, r - theta = -7.35231e-10
r = 0.82731, theta = 0.82731, r - theta = -1.4245e-09
r = 0.828005, theta = 0.828005, r - theta = -1.67434e-09
r = 0.828701, theta = 0.828701, r - theta = -1.25619e-09
r = 0.829396, theta = 0.829396, r - theta = 4.06448e-11
r = 0.830092, theta = 0.830092, r - theta = 1.20325e-09
r = 0.830787, theta = 0.830787, r - theta = 1.40893e-09
r = 0.831482, theta = 0.831482, r - theta = 9.68145e-10
r = 0.832177, theta = 0.832177, r - theta = 1.74118e-10
r = 0.832872, theta = 0.832872, r - theta = -6.9713e-10
r = 0.833567, theta = 0.833567, r - theta = -1.38672e-09
r = 0.834262, theta = 0.834262, r - theta = -1.65287e-09
r = 0.834956, theta = 0.834956, r - theta = -1.27082e-09
r = 0.835651, theta = 0.835651, r - theta = -3.28118e-11
r = 0.836345, theta = 0.836345, r - theta = 1.15816e-09
r = 0.837039, theta = 0.837039, r - theta = 1.39511e-09
r = 0.837733, theta = 0.837733, r - theta = 9.83344e-10
r = 0.838427, theta = 0.838427, r - theta = 2.11587e-10
r = 0.839121, theta = 0.839121, r - theta = -6.48398e-10
r = 0.839815, theta = 0.839815, r - theta = -1.34175e-09
r = 0.840508, theta = 0.840508, r - theta = -1.63046e-09
r = 0.841202, theta = 0.841202, r - theta = -1.29333e-09
r = 0.841895, theta = 0.841895, r - theta = -1.25903e-10
r = 0.842588, theta = 0.842588, r - theta = 1.10253e-09
r = 0.843281, theta = 0.843281, r - theta = 1.38135e-09
r = 0.843974, theta = 0.843974, r - theta = 1.00545e-09
r = 0.844667, theta = 0.844667, r - theta = 2.59333e-10
r = 0.84536, theta = 0.84536, r - theta = -5.89227e-10
r = 0.846053, theta = 0.846053, r - theta = -1.28912e-09
r = 0.846745, theta = 0.846745, r - theta = -1.60587e-09
r = 0.847438, theta = 0.847438, r - theta = -1.32155e-09
r = 0.84813, theta = 0.84813, r - theta = -2.34768e-10
r = 0.848822, theta = 0.848822, r - theta = 1.03458e-09
r = 0.849514, theta = 0.849514, r - theta = 1.36605e-09
r = 0.850206, theta = 0.850206, r - theta = 1.03319e-09
r = 0.850898, theta = 0.850898, r - theta = 3.16561e-10
r = 0.85159, theta = 0.85159, r - theta = -5.19793e-10
r = 0.852281, theta = 0.852281, r - theta = -1.22826e-09
r = 0.852973, theta = 0.852973, r - theta = -1.57762e-09
r = 0.853664, theta = 0.853664, r - theta = -1.353e-09
r = 0.854355, theta = 0.854355, r - theta = -3.55815e-10
r = 0.855046, theta = 0.855046, r - theta = 9.52057e-10
r = 0.855737, theta = 0.855737, r - theta = 1.34723e-09
r = 0.856428, theta = 0.856428, r - theta = 1.06505e-09
r = 0.857119, theta = 0.857119, r - theta = 3.82344e-10
r = 0.85781, theta = 0.85781, r - theta = -4.40294e-10
r = 0.8585, theta = 0.8585, r - theta = -1.1585e-09
r = 0.85919, theta = 0.85919, r - theta = -1.54409e-09
r = 0.859881, theta = 0.859881, r - theta = -1.38498e-09
r = 0.860571, theta = 0.860571, r - theta = -4.85178e-10
r = 0.861261, theta = 0.861261, r - theta = 8.5216e-10
r = 0.861951, theta = 0.861951, r - theta = 1.32255e-09
r = 0.862641, theta = 0.862641, r - theta = 1.09922e-09
r = 0.86333, theta = 0.86333, r - theta = 4.55583e-10
r = 0.86402, theta = 0.86402, r - theta = -3.51011e-10
r = 0.864709, theta = 0.864709, r - theta = -1.0792e-09
r = 0.865399, theta = 0.865399, r - theta = -1.50359e-09
r = 0.866088, theta = 0.866088, r - theta = -1.41466e-09
r = 0.866777, theta = 0.866777, r - theta = -6.18788e-10
r = 0.867466, theta = 0.867466, r - theta = 7.31552e-10
r = 0.868155, theta = 0.868155, r - theta = 1.2892e-09
r = 0.868844, theta = 0.868844, r - theta = 1.1336e-09
r = 0.869532, theta = 0.869532, r - theta = 5.34961e-10
r = 0.870221, theta = 0.870221, r - theta = -2.52354e-10
r = 0.870909, theta = 0.870909, r - theta = -9.89759e-10
r = 0.871598, theta = 0.871598, r - theta = -1.45441e-09
r = 0.872286, theta = 0.872286, r - theta = -1.43914e-09
r = 0.872974, theta = 0.872974, r - theta = -7.52461e-10
r = 0.873662, theta = 0.873662, r - theta = 5.86318e-10
r = 0.87435, theta = 0.87435, r - theta = 1.24393e-09
r = 0.875037, theta = 0.875037, r - theta = 1.16571e-09
r = 0.875725, theta = 0.875725, r - theta = 6.18886e-10
r = 0.876412, theta = 0.876412, r - theta = -1.44927e-10
r = 0.8771, theta = 0.8771, r - theta = -8.89688e-10
r = 0.877787, theta = 0.877787, r - theta = -1.39489e-09
r = 0.878474, theta = 0.878474, r - theta = -1.45551e-09
r = 0.879161, theta = 0.879161, r - theta = -8.81968e-10
r = 0.879848, theta = 0.879848, r - theta = 4.11928e-10
r = 0.880535, theta = 0.880535, r - theta = 1.18298e-09
r = 0.881221, theta = 0.881221, r - theta = 1.19265e-09
r = 0.881908, theta = 0.881908, r - theta = 7.05438e-10
r = 0.882594, theta = 0.882594, r - theta = -2.9584e-11
r = 0.883281, theta = 0.883281, r - theta = -7.78701e-10
r = 0.883967, theta = 0.883967, r - theta = -1.32353e-09
r = 0.884653, theta = 0.884653, r - theta = -1.46096e-09
r = 0.885339, theta = 0.885339, r - theta = -1.00313e-09
r = 0.886025, theta = 0.886025, r - theta = 2.032e-10
r = 0.88671, theta = 0.88671, r - theta = 1.10203e-09
r = 0.887396, theta = 0.887396, r - theta = 1.21107e-09
r = 0.888081, theta = 0.888081, r - theta = 7.92305e-10
r = 0.888767, theta = 0.888767, r - theta = 9.24942e-11
r = 0.889452, theta = 0.889452, r - theta = -6.56764e-10
r = 0.890137, theta = 0.890137, r - theta = -1.239e-09
r = 0.890822, theta = 0.890822, r - theta = -1.45282e-09
r = 0.891507, theta = 0.891507, r - theta = -1.11189e-09
r = 0.892192, theta = 0.892192, r - theta = -4.48661e-11
r = 0.892876, theta = 0.892876, r - theta = 9.96139e-10
r = 0.893561, theta = 0.893561, r - theta = 1.21707e-09
r = 0.894245, theta = 0.894245, r - theta = 8.76721e-10
r = 0.89493, theta = 0.89493, r - theta = 2.19742e-10
r = 0.895614, theta = 0.895614, r - theta = -5.24179e-10
r = 0.896298, theta = 0.896298, r - theta = -1.14028e-09
r = 0.896982, theta = 0.896982, r - theta = -1.4287e-09
r = 0.897666, theta = 0.897666, r - theta = -1.20442e-09
r = 0.89835, theta = 0.89835, r - theta = -2.97223e-10
r = 0.899033, theta = 0.899033, r - theta = 8.59712e-10
r = 0.899717, theta = 0.899717, r - theta = 1.20618e-09
r = 0.9004, theta = 0.9004, r - theta = 9.554e-10
r = 0.901083, theta = 0.901083, r - theta = 3.50129e-10
r = 0.901766, theta = 0.901766, r - theta = -3.81656e-10
r = 0.90245, theta = 0.90245, r - theta = -1.02672e-09
r = 0.903132, theta = 0.903132, r - theta = -1.38652e-09
r = 0.903815, theta = 0.903815, r - theta = -1.27717e-09
r = 0.904498, theta = 0.904498, r - theta = -5.29412e-10
r = 0.905181, theta = 0.905181, r - theta = 6.86416e-10
r = 0.905863, theta = 0.905863, r - theta = 1.17325e-09
r = 0.906545, theta = 0.906545, r - theta = 1.02446e-09
r = 0.907228, theta = 0.907228, r - theta = 4.81093e-10
r = 0.90791, theta = 0.90791, r - theta = -2.30392e-10
r = 0.908592, theta = 0.908592, r - theta = -8.98084e-10
r = 0.909274, theta = 0.909274, r - theta = -1.32459e-09
r = 0.909955, theta = 0.909955, r - theta = -1.32698e-09
r = 0.910637, theta = 0.910637, r - theta = -7.36758e-10
r = 0.911318, theta = 0.911318, r - theta = 4.6912e-10
r = 0.912, theta = 0.912, r - theta = 1.11242e-09
r = 0.912681, theta = 0.912681, r - theta = 1.07936e-09
r = 0.913362, theta = 0.913362, r - theta = 6.09459e-10
r = 0.914043, theta = 0.914043, r - theta = -7.21433e-11
r = 0.914724, theta = 0.914724, r - theta = -7.54687e-10
r = 0.915405, theta = 0.915405, r - theta = -1.24174e-09
r = 0.916086, theta = 0.916086, r - theta = -1.35115e-09
r = 0.916767, theta = 0.916767, r - theta = -9.15029e-10
r = 0.917447, theta = 0.917447, r - theta = 1.9983e-10
r = 0.918127, theta = 0.918127, r - theta = 1.01703e-09
r = 0.918808, theta = 0.918808, r - theta = 1.11481e-09
r = 0.919488, theta = 0.919488, r - theta = 7.31369e-10
r = 0.920168, theta = 0.920168, r - theta = 9.06896e-11
r = 0.920848, theta = 0.920848, r - theta = -5.97434e-10
r = 0.921528, theta = 0.921528, r - theta = -1.13736e-09
r = 0.922207, theta = 0.922207, r - theta = -1.34754e-09
r = 0.922887, theta = 0.922887, r - theta = -1.06053e-09
r = 0.923566, theta = 0.923566, r - theta = -1.22897e-10
r = 0.924246, theta = 0.924246, r - theta = 8.79545e-10
r = 0.924925, theta = 0.924925, r - theta = 1.12472e-09
r = 0.925604, theta = 0.925604, r - theta = 8.42197e-10
r = 0.926283, theta = 0.926283, r - theta = 2.54984e-10
r = 0.926962, theta = 0.926962, r - theta = -4.27921e-10
r = 0.927641, theta = 0.927641, r - theta = -1.01149e-09
r = 0.928319, theta = 0.928319, r - theta = -1.31464e-09
r = 0.928998, theta = 0.928998, r - theta = -1.17017e-09
r = 0.929676, theta = 0.929676, r - theta = -4.24769e-10
r = 0.930354, theta = 0.930354, r - theta = 6.91478e-10
r = 0.931033, theta = 0.931033, r - theta = 1.1021e-09
r = 0.931711, theta = 0.931711, r - theta = 9.36474e-10
r = 0.932389, theta = 0.932389, r - theta = 4.16814e-10
r = 0.933067, theta = 0.933067, r - theta = -2.48511e-10
r = 0.933744, theta = 0.933744, r - theta = -8.64933e-10
r = 0.934422, theta = 0.934422, r - theta = -1.25165e-09
r = 0.935099, theta = 0.935099, r - theta = -1.24158e-09
r = 0.935777, theta = 0.935777, r - theta = -6.8135e-10
r = 0.936454, theta = 0.936454, r - theta = 4.43323e-10
r = 0.937131, theta = 0.937131, r - theta = 1.03898e-09
r = 0.937808, theta = 0.937808, r - theta = 1.0078e-09
r = 0.938485, theta = 0.938485, r - theta = 5.71373e-10
r = 0.939162, theta = 0.939162, r - theta = -6.2415e-11
r = 0.939839, theta = 0.939839, r - theta = -6.99287e-10
r = 0.940515, theta = 0.940515, r - theta = -1.15857e-09
r = 0.941192, theta = 0.941192, r - theta = -1.27315e-09
r = 0.941868, theta = 0.941868, r - theta = -8.8944e-10
r = 0.942544, theta = 0.942544, r - theta = 1.24466e-10
r = 0.943221, theta = 0.943221, r - theta = 9.26347e-10
r = 0.943897, theta = 0.943897, r - theta = 1.04879e-09
r = 0.944572, theta = 0.944572, r - theta = 7.12883e-10
r = 0.945248, theta = 0.945248, r - theta = 1.26223e-10
r = 0.945924, theta = 0.945924, r - theta = -5.17064e-10
r = 0.946599, theta = 0.946599, r - theta = -1.03628e-09
r = 0.947275, theta = 0.947275, r - theta = -1.26414e-09
r = 0.94795, theta = 0.94795, r - theta = -1.04669e-09
r = 0.948625, theta = 0.948625, r - theta = -2.43349e-10
r = 0.949301, theta = 0.949301, r - theta = 7.54052e-10
r = 0.949976, theta = 0.949976, r - theta = 1.05094e-09
r = 0.95065, theta = 0.95065, r - theta = 8.34521e-10
r = 0.951325, theta = 0.951325, r - theta = 3.12246e-10
r = 0.952, theta = 0.952, r - theta = -3.21759e-10
r = 0.952674, theta = 0.952674, r - theta = -8.86631e-10
r = 0.953349, theta = 0.953349, r - theta = -1.21475e-09
r = 0.954023, theta = 0.954023, r - theta = -1.15168e-09
r = 0.954697, theta = 0.954697, r - theta = -5.56198e-10
r = 0.955371, theta = 0.955371, r - theta = 5.10722e-10
r = 0.956045, theta = 0.956045, r - theta = 1.00458e-09
r = 0.956719, theta = 0.956719, r - theta = 9.28331e-10
r = 0.957393, theta = 0.957393, r - theta = 4.89398e-10
r = 0.958067, theta = 0.958067, r - theta = -1.17935e-10
r = 0.95874, theta = 0.95874, r - theta = -7.12498e-10
r = 0.959414, theta = 0.959414, r - theta = -1.1262e-09
r = 0.960087, theta = 0.960087, r - theta = -1.20401e-09
r = 0.96076, theta = 0.96076, r - theta = -8.03878e-10
r = 0.961433, theta = 0.961433, r - theta = 1.83677e-10
r = 0.962106, theta = 0.962106, r - theta = 8.988e-10
r = 0.962779, theta = 0.962779, r - theta = 9.85136e-10
r = 0.963452, theta = 0.963452, r - theta = 6.50244e-10
r = 0.964124, theta = 0.964124, r - theta = 8.86958e-11
r = 0.964797, theta = 0.964797, r - theta = -5.17894e-10
r = 0.965469, theta = 0.965469, r - theta = -1.00083e-09
r = 0.966141, theta = 0.966141, r - theta = -1.20433e-09
r = 0.966813, theta = 0.966813, r - theta = -9.85443e-10
r = 0.967486, theta = 0.967486, r - theta = -2.14073e-10
r = 0.968157, theta = 0.968157, r - theta = 7.21324e-10
r = 0.968829, theta = 0.968829, r - theta = 9.94459e-10
r = 0.969501, theta = 0.969501, r - theta = 7.86082e-10
r = 0.970173, theta = 0.970173, r - theta = 2.91188e-10
r = 0.970844, theta = 0.970844, r - theta = -3.08029e-10
r = 0.971515, theta = 0.971515, r - theta = -8.42148e-10
r = 0.972187, theta = 0.972187, r - theta = -1.15449e-09
r = 0.972858, theta = 0.972858, r - theta = -1.10109e-09
r = 0.973529, theta = 0.973529, r - theta = -5.50661e-10
r = 0.9742, theta = 0.9742, r - theta = 4.58447e-10
r = 0.974871, theta = 0.974871, r - theta = 9.44433e-10
r = 0.975541, theta = 0.975541, r - theta = 8.86858e-10
r = 0.976212, theta = 0.976212, r - theta = 4.8128e-10
r = 0.976882, theta = 0.976882, r - theta = -8.94023e-11
r = 0.977553, theta = 0.977553, r - theta = -6.5491e-10
r = 0.978223, theta = 0.978223, r - theta = -1.05756e-09
r = 0.978893, theta = 0.978893, r - theta = -1.15224e-09
r = 0.979563, theta = 0.979563, r - theta = -8.06353e-10
r = 0.980233, theta = 0.980233, r - theta = 9.49416e-11
r = 0.980903, theta = 0.980903, r - theta = 8.21709e-10
r = 0.981572, theta = 0.981572, r - theta = 9.41085e-10
r = 0.982242, theta = 0.982242, r - theta = 6.49311e-10
r = 0.982912, theta = 0.982912, r - theta = 1.3012e-10
r = 0.983581, theta = 0.983581, r - theta = -4.4523e-10
r = 0.98425, theta = 0.98425, r - theta = -9.17934e-10
r = 0.984919, theta = 0.984919, r - theta = -1.1416e-09
r = 0.985588, theta = 0.985588, r - theta = -9.82236e-10
r = 0.986257, theta = 0.986257, r - theta = -3.18201e-10
r = 0.986926, theta = 0.986926, r - theta = 6.11373e-10
r = 0.987595, theta = 0.987595, r - theta = 9.35747e-10
r = 0.988263, theta = 0.988263, r - theta = 7.84135e-10
r = 0.988932, theta = 0.988932, r - theta = 3.41223e-10
r = 0.9896, theta = 0.9896, r - theta = -2.20633e-10
r = 0.990268, theta = 0.990268, r - theta = -7.41387e-10
r = 0.990936, theta = 0.990936, r - theta = -1.07327e-09
r = 0.991604, theta = 0.991604, r - theta = -1.08075e-09
r = 0.992272, theta = 0.992272, r - theta = -6.40536e-10
r = 0.99294, theta = 0.99294, r - theta = 2.96848e-10
r = 0.993608, theta = 0.993608, r - theta = 8.56216e-10
r = 0.994275, theta = 0.994275, r - theta = 8.73031e-10
r = 0.994943, theta = 0.994943, r - theta = 5.33055e-10
r = 0.99561, theta = 0.99561, r - theta = 9.85345e-12
r = 0.996277, theta = 0.996277, r - theta = -5.35173e-10
r = 0.996944, theta = 0.996944, r - theta = -9.5276e-10
r = 0.997611, theta = 0.997611, r - theta = -1.10575e-09
r = 0.998278, theta = 0.998278, r - theta = -8.69082e-10
r = 0.998945, theta = 0.998945, r - theta = -1.29727e-10
r = 0.999612, theta = 0.999612, r - theta = 6.86157e-10
r = 1.00028, theta = 1.00028, r - theta = 9.01623e-10
r = 1.00094, theta = 1.00094, r - theta = 6.93147e-10
r = 1.00161, theta = 1.00161, r - theta = 2.35622e-10
r = 1.00228, theta = 1.00228, r - theta = -3.0809e-10
r = 1.00294, theta = 1.00294, r - theta = -7.87124e-10
r = 1.00361, theta = 1.00361, r - theta = -1.06259e-09
r = 1.00428, theta = 1.00428, r - theta = -1.00755e-09
r = 1.00494, theta = 1.00494, r - theta = -5.06977e-10
r = 1.00561, theta = 1.00561, r - theta = 4.07435e-10
r = 1.00627, theta = 1.00627, r - theta = 8.53776e-10
r = 1.00694, theta = 1.00694, r - theta = 8.07322e-10
r = 1.0076, theta = 1.0076, r - theta = 4.44399e-10
r = 1.00827, theta = 1.00827, r - theta = -7.05644e-11
r = 1.00893, theta = 1.00893, r - theta = -5.85e-10
r = 1.0096, theta = 1.0096, r - theta = -9.58182e-10
r = 1.01026, theta = 1.01026, r - theta = -1.0612e-09
r = 1.01093, theta = 1.01093, r - theta = -7.76919e-10
r = 1.01159, theta = 1.01159, r - theta = 2.13163e-14
r = 1.01226, theta = 1.01226, r - theta = 7.11518e-10
r = 1.01292, theta = 1.01292, r - theta = 8.5961e-10
r = 1.01359, theta = 1.01359, r - theta = 6.22158e-10
r = 1.01425, theta = 1.01425, r - theta = 1.65267e-10
r = 1.01491, theta = 1.01491, r - theta = -3.56696e-10
r = 1.01558, theta = 1.01558, r - theta = -8.01069e-10
r = 1.01624, theta = 1.01624, r - theta = -1.03687e-09
r = 1.01691, theta = 1.01691, r - theta = -9.44785e-10
r = 1.01757, theta = 1.01757, r - theta = -4.17112e-10
r = 1.01823, theta = 1.01823, r - theta = 4.54933e-10
r = 1.0189, theta = 1.0189, r - theta = 8.32149e-10
r = 1.01956, theta = 1.01956, r - theta = 7.53038e-10
r = 1.02022, theta = 1.02022, r - theta = 3.85475e-10
r = 1.02089, theta = 1.02089, r - theta = -1.14272e-10
r = 1.02155, theta = 1.02155, r - theta = -6.01512e-10
r = 1.02221, theta = 1.02221, r - theta = -9.43107e-10
r = 1.02288, theta = 1.02288, r - theta = -1.01745e-09
r = 1.02354, theta = 1.02354, r - theta = -7.14428e-10
r = 1.0242, theta = 1.0242, r - theta = 6.20681e-11
r = 1.02486, theta = 1.02486, r - theta = 7.05098e-10
r = 1.02553, theta = 1.02553, r - theta = 8.19245e-10
r = 1.02619, theta = 1.02619, r - theta = 5.74251e-10
r = 1.02685, theta = 1.02685, r - theta = 1.28379e-10
r = 1.02751, theta = 1.02751, r - theta = -3.71558e-10
r = 1.02817, theta = 1.02817, r - theta = -7.90174e-10
r = 1.02884, theta = 1.02884, r - theta = -1.00349e-09
r = 1.0295, theta = 1.0295, r - theta = -8.98889e-10
r = 1.03016, theta = 1.03016, r - theta = -3.75129e-10
r = 1.03082, theta = 1.03082, r - theta = 4.56534e-10
r = 1.03148, theta = 1.03148, r - theta = 8.00963e-10
r = 1.03214, theta = 1.03214, r - theta = 7.13818e-10
r = 1.0328, theta = 1.0328, r - theta = 3.55448e-10
r = 1.03347, theta = 1.03347, r - theta = -1.25122e-10
r = 1.03413, theta = 1.03413, r - theta = -5.90173e-10
r = 1.03479, theta = 1.03479, r - theta = -9.13261e-10
r = 1.03545, theta = 1.03545, r - theta = -9.79199e-10
r = 1.03611, theta = 1.03611, r - theta = -6.8403e-10
r = 1.03677, theta = 1.03677, r - theta = 6.2351e-11
r = 1.03743, theta = 1.03743, r - theta = 6.76256e-10
r = 1.03809, theta = 1.03809, r - theta = 7.84337e-10
r = 1.03875, theta = 1.03875, r - theta = 5.49121e-10
r = 1.03941, theta = 1.03941, r - theta = 1.21929e-10
r = 1.04007, theta = 1.04007, r - theta = -3.57099e-10
r = 1.04073, theta = 1.04073, r - theta = -7.5898e-10
r = 1.04139, theta = 1.04139, r - theta = -9.65863e-10
r = 1.04205, theta = 1.04205, r - theta = -8.71009e-10
r = 1.04271, theta = 1.04271, r - theta = -3.78764e-10
r = 1.04337, theta = 1.04337, r - theta = 4.20963e-10
r = 1.04403, theta = 1.04403, r - theta = 7.63813e-10
r = 1.04469, theta = 1.04469, r - theta = 6.89488e-10
r = 1.04535, theta = 1.04535, r - theta = 3.51692e-10
r = 1.046, theta = 1.046, r - theta = -1.06932e-10
r = 1.04666, theta = 1.04666, r - theta = -5.54779e-10
r = 1.04732, theta = 1.04732, r - theta = -8.71262e-10
r = 1.04798, theta = 1.04798, r - theta = -9.46781e-10
r = 1.04864, theta = 1.04864, r - theta = -6.82706e-10
r = 1.0493, theta = 1.0493, r - theta = 8.59579e-12
r = 1.04996, theta = 1.04996, r - theta = 6.27985e-10
r = 1.05061, theta = 1.05061, r - theta = 7.54442e-10
r = 1.05127, theta = 1.05127, r - theta = 5.44134e-10
r = 1.05193, theta = 1.05193, r - theta = 1.42284e-10
r = 1.05259, theta = 1.05259, r - theta = -3.16804e-10
r = 1.05325, theta = 1.05325, r - theta = -7.09727e-10
r = 1.0539, theta = 1.0539, r - theta = -9.23954e-10
r = 1.05456, theta = 1.05456, r - theta = -8.57809e-10
r = 1.05522, theta = 1.05522, r - theta = -4.20445e-10
r = 1.05588, theta = 1.05588, r - theta = 3.5023e-10
r = 1.05653, theta = 1.05653, r - theta = 7.19585e-10
r = 1.05719, theta = 1.05719, r - theta = 6.76996e-10
r = 1.05785, theta = 1.05785, r - theta = 3.70346e-10
r = 1.0585, theta = 1.0585, r - theta = -6.32898e-11
r = 1.05916, theta = 1.05916, r - theta = -4.97618e-10
r = 1.05982, theta = 1.05982, r - theta = -8.17106e-10
r = 1.06048, theta = 1.06048, r - theta = -9.16962e-10
r = 1.06113, theta = 1.06113, r - theta = -7.03108e-10
r = 1.06179, theta = 1.06179, r - theta = -9.21634e-11
r = 1.06244, theta = 1.06244, r - theta = 5.58116e-10
r = 1.0631, theta = 1.0631, r - theta = 7.25695e-10
r = 1.06376, theta = 1.06376, r - theta = 5.54798e-10
r = 1.06441, theta = 1.06441, r - theta = 1.85346e-10
r = 1.06507, theta = 1.06507, r - theta = -2.53408e-10
r = 1.06572, theta = 1.06572, r - theta = -6.4286e-10
r = 1.06638, theta = 1.06638, r - theta = -8.75035e-10
r = 1.06704, theta = 1.06704, r - theta = -8.5256e-10
r = 1.06769, theta = 1.06769, r - theta = -4.88648e-10
r = 1.06835, theta = 1.06835, r - theta = 2.40717e-10
r = 1.069, theta = 1.069, r - theta = 6.63211e-10
r = 1.06966, theta = 1.06966, r - theta = 6.70828e-10
r = 1.07031, theta = 1.07031, r - theta = 4.06406e-10
r = 1.07097, theta = 1.07097, r - theta = 2.22711e-12
r = 1.07162, theta = 1.07162, r - theta = -4.19969e-10
r = 1.07228, theta = 1.07228, r - theta = -7.48955e-10
r = 1.07293, theta = 1.07293, r - theta = -8.84004e-10
r = 1.07358, theta = 1.07358, r - theta = -7.3486e-10
r = 1.07424, theta = 1.07424, r - theta = -2.21723e-10
r = 1.07489, theta = 1.07489, r - theta = 4.59979e-10
r = 1.07555, theta = 1.07555, r - theta = 6.91164e-10
r = 1.0762, theta = 1.0762, r - theta = 5.74817e-10
r = 1.07686, theta = 1.07686, r - theta = 2.46314e-10
r = 1.07751, theta = 1.07751, r - theta = -1.69401e-10
r = 1.07816, theta = 1.07816, r - theta = -5.57791e-10
r = 1.07882, theta = 1.07882, r - theta = -8.14708e-10
r = 1.07947, theta = 1.07947, r - theta = -8.46372e-10
r = 1.08012, theta = 1.08012, r - theta = -5.69349e-10
r = 1.08078, theta = 1.08078, r - theta = 8.37634e-11
r = 1.08143, theta = 1.08143, r - theta = 5.85954e-10
r = 1.08208, theta = 1.08208, r - theta = 6.63013e-10
r = 1.08274, theta = 1.08274, r - theta = 4.53477e-10
r = 1.08339, theta = 1.08339, r - theta = 8.5556e-11
r = 1.08404, theta = 1.08404, r - theta = -3.22842e-10
r = 1.08469, theta = 1.08469, r - theta = -6.64091e-10
r = 1.08535, theta = 1.08535, r - theta = -8.40827e-10
r = 1.086, theta = 1.086, r - theta = -7.65926e-10
r = 1.08665, theta = 1.08665, r - theta = -3.62489e-10
r = 1.0873, theta = 1.0873, r - theta = 3.22645e-10
r = 1.08796, theta = 1.08796, r - theta = 6.40827e-10
r = 1.08861, theta = 1.08861, r - theta = 5.95831e-10
r = 1.08926, theta = 1.08926, r - theta = 3.19197e-10
r = 1.08991, theta = 1.08991, r - theta = -6.7738e-11
r = 1.09056, theta = 1.09056, r - theta = -4.5381e-10
r = 1.09122, theta = 1.09122, r - theta = -7.38017e-10
r = 1.09187, theta = 1.09187, r - theta = -8.29493e-10
r = 1.09252, theta = 1.09252, r - theta = -6.47492e-10
r = 1.09317, theta = 1.09317, r - theta = -1.21366e-10
r = 1.09382, theta = 1.09382, r - theta = 4.75359e-10
r = 1.09447, theta = 1.09447, r - theta = 6.4286e-10
r = 1.09512, theta = 1.09512, r - theta = 5.03284e-10
r = 1.09577, theta = 1.09577, r - theta = 1.8146e-10
r = 1.09642, theta = 1.09642, r - theta = -2.07861e-10
r = 1.09707, theta = 1.09707, r - theta = -5.5998e-10
r = 1.09773, theta = 1.09773, r - theta = -7.80239e-10
r = 1.09838, theta = 1.09838, r - theta = -7.83996e-10
r = 1.09903, theta = 1.09903, r - theta = -4.96605e-10
r = 1.09968, theta = 1.09968, r - theta = 1.30846e-10
r = 1.10033, theta = 1.10033, r - theta = 5.61291e-10
r = 1.10098, theta = 1.10098, r - theta = 6.06932e-10
r = 1.10163, theta = 1.10163, r - theta = 3.96141e-10
r = 1.10228, theta = 1.10228, r - theta = 4.73179e-11
r = 1.10293, theta = 1.10293, r - theta = -3.31097e-10
r = 1.10358, theta = 1.10358, r - theta = -6.40598e-10
r = 1.10422, theta = 1.10422, r - theta = -7.92599e-10
r = 1.10487, theta = 1.10487, r - theta = -7.0841e-10
r = 1.10552, theta = 1.10552, r - theta = -3.19224e-10
r = 1.10617, theta = 1.10617, r - theta = 3.14958e-10
r = 1.10682, theta = 1.10682, r - theta = 5.9647e-10
r = 1.10747, theta = 1.10747, r - theta = 5.45026e-10
r = 1.10812, theta = 1.10812, r - theta = 2.82723e-10
r = 1.10877, theta = 1.10877, r - theta = -7.82026e-11
r = 1.10942, theta = 1.10942, r - theta = -4.35351e-10
r = 1.11007, theta = 1.11007, r - theta = -6.96143e-10
r = 1.11071, theta = 1.11071, r - theta = -7.778e-10
r = 1.11136, theta = 1.11136, r - theta = -6.07328e-10
r = 1.11201, theta = 1.11201, r - theta = -1.21493e-10
r = 1.11266, theta = 1.11266, r - theta = 4.3531e-10
r = 1.11331, theta = 1.11331, r - theta = 5.94032e-10
r = 1.11395, theta = 1.11395, r - theta = 4.66667e-10
r = 1.1146, theta = 1.1146, r - theta = 1.69298e-10
r = 1.11525, theta = 1.11525, r - theta = -1.91735e-10
r = 1.1159, theta = 1.1159, r - theta = -5.19814e-10
r = 1.11655, theta = 1.11655, r - theta = -7.28026e-10
r = 1.11719, theta = 1.11719, r - theta = -7.39147e-10
r = 1.11784, theta = 1.11784, r - theta = -4.85621e-10
r = 1.11849, theta = 1.11849, r - theta = 8.37912e-11
r = 1.11913, theta = 1.11913, r - theta = 5.06127e-10
r = 1.11978, theta = 1.11978, r - theta = 5.64636e-10
r = 1.12043, theta = 1.12043, r - theta = 3.79293e-10
r = 1.12107, theta = 1.12107, r - theta = 6.04277e-11
r = 1.12172, theta = 1.12172, r - theta = -2.91261e-10
r = 1.12237, theta = 1.12237, r - theta = -5.84684e-10
r = 1.12301, theta = 1.12301, r - theta = -7.38345e-10
r = 1.12366, theta = 1.12366, r - theta = -6.80323e-10
r = 1.12431, theta = 1.12431, r - theta = -3.48255e-10
r = 1.12495, theta = 1.12495, r - theta = 2.41178e-10
r = 1.1256, theta = 1.1256, r - theta = 5.39145e-10
r = 1.12625, theta = 1.12625, r - theta = 5.16833e-10
r = 1.12689, theta = 1.12689, r - theta = 2.88603e-10
r = 1.12754, theta = 1.12754, r - theta = -4.07201e-11
r = 1.12818, theta = 1.12818, r - theta = -3.75829e-10
r = 1.12883, theta = 1.12883, r - theta = -6.30917e-10
r = 1.12947, theta = 1.12947, r - theta = -7.2966e-10
r = 1.13012, theta = 1.13012, r - theta = -6.05199e-10
r = 1.13076, theta = 1.13076, r - theta = -2.00123e-10
r = 1.13141, theta = 1.13141, r - theta = 3.49579e-10
r = 1.13205, theta = 1.13205, r - theta = 5.4397e-10
r = 1.1327, theta = 1.1327, r - theta = 4.57396e-10
r = 1.13334, theta = 1.13334, r - theta = 1.98846e-10
r = 1.13399, theta = 1.13399, r - theta = -1.32111e-10
r = 1.13463, theta = 1.13463, r - theta = -4.45321e-10
r = 1.13528, theta = 1.13528, r - theta = -6.60019e-10
r = 1.13592, theta = 1.13592, r - theta = -7.04815e-10
r = 1.13657, theta = 1.13657, r - theta = -5.17678e-10
r = 1.13721, theta = 1.13721, r - theta = -4.59175e-11
r = 1.13785, theta = 1.13785, r - theta = 4.19597e-10
r = 1.1385, theta = 1.1385, r - theta = 5.28392e-10
r = 1.13914, theta = 1.13914, r - theta = 3.91599e-10
r = 1.13979, theta = 1.13979, r - theta = 1.13084e-10
r = 1.14043, theta = 1.14043, r - theta = -2.12612e-10
r = 1.14107, theta = 1.14107, r - theta = -5.00248e-10
r = 1.14172, theta = 1.14172, r - theta = -6.73868e-10
r = 1.14236, theta = 1.14236, r - theta = -6.66789e-10
r = 1.143, theta = 1.143, r - theta = -4.21576e-10
r = 1.14365, theta = 1.14365, r - theta = 9.92673e-11
r = 1.14429, theta = 1.14429, r - theta = 4.59988e-10
r = 1.14493, theta = 1.14493, r - theta = 4.98658e-10
r = 1.14558, theta = 1.14558, r - theta = 3.23478e-10
r = 1.14622, theta = 1.14622, r - theta = 3.34102e-11
r = 1.14686, theta = 1.14686, r - theta = -2.81792e-10
r = 1.1475, theta = 1.1475, r - theta = -5.41574e-10
r = 1.14815, theta = 1.14815, r - theta = -6.74568e-10
r = 1.14879, theta = 1.14879, r - theta = -6.18565e-10
r = 1.14943, theta = 1.14943, r - theta = -3.20509e-10
r = 1.15007, theta = 1.15007, r - theta = 2.07207e-10
r = 1.15071, theta = 1.15071, r - theta = 4.77926e-10
r = 1.15136, theta = 1.15136, r - theta = 4.59728e-10
r = 1.152, theta = 1.152, r - theta = 2.56037e-10
r = 1.15264, theta = 1.15264, r - theta = -3.88511e-11
r = 1.15328, theta = 1.15328, r - theta = -3.39752e-10
r = 1.15392, theta = 1.15392, r - theta = -5.70578e-10
r = 1.15456, theta = 1.15456, r - theta = -6.64321e-10
r = 1.15521, theta = 1.15521, r - theta = -5.63036e-10
r = 1.15585, theta = 1.15585, r - theta = -2.17826e-10
r = 1.15649, theta = 1.15649, r - theta = 2.83893e-10
r = 1.15713, theta = 1.15713, r - theta = 4.79246e-10
r = 1.15777, theta = 1.15777, r - theta = 4.15473e-10
r = 1.15841, theta = 1.15841, r - theta = 1.91448e-10
r = 1.15905, theta = 1.15905, r - theta = -1.02984e-10
r = 1.15969, theta = 1.15969, r - theta = -3.86988e-10
r = 1.16033, theta = 1.16033, r - theta = -5.88725e-10
r = 1.16097, theta = 1.16097, r - theta = -6.45336e-10
r = 1.16161, theta = 1.16161, r - theta = -5.02925e-10
r = 1.16225, theta = 1.16225, r - theta = -1.16544e-10
r = 1.16289, theta = 1.16289, r - theta = 3.35996e-10
r = 1.16353, theta = 1.16353, r - theta = 4.68648e-10
r = 1.16417, theta = 1.16417, r - theta = 3.68873e-10
r = 1.16481, theta = 1.16481, r - theta = 1.31209e-10
r = 1.16545, theta = 1.16545, r - theta = -1.58735e-10
r = 1.16609, theta = 1.16609, r - theta = -4.24262e-10
r = 1.16673, theta = 1.16673, r - theta = -5.9757e-10
r = 1.16737, theta = 1.16737, r - theta = -6.19742e-10
r = 1.16801, theta = 1.16801, r - theta = -4.40724e-10
r = 1.16865, theta = 1.16865, r - theta = -1.93126e-11
r = 1.16929, theta = 1.16929, r - theta = 3.69001e-10
r = 1.16993, theta = 1.16993, r - theta = 4.49879e-10
r = 1.17057, theta = 1.17057, r - theta = 3.22168e-10
r = 1.17121, theta = 1.17121, r - theta = 7.62803e-11
r = 1.17184, theta = 1.17184, r - theta = -2.06202e-10
r = 1.17248, theta = 1.17248, r - theta = -4.52512e-10
r = 1.17312, theta = 1.17312, r - theta = -5.98683e-10
r = 1.17376, theta = 1.17376, r - theta = -5.89534e-10
r = 1.1744, theta = 1.1744, r - theta = -3.78654e-10
r = 1.17504, theta = 1.17504, r - theta = 6.62643e-11
r = 1.17567, theta = 1.17567, r - theta = 3.87395e-10
r = 1.17631, theta = 1.17631, r - theta = 4.25893e-10
r = 1.17695, theta = 1.17695, r - theta = 2.76999e-10
r = 1.17759, theta = 1.17759, r - theta = 2.72067e-11
r = 1.17823, theta = 1.17823, r - theta = -2.45727e-10
r = 1.17886, theta = 1.17886, r - theta = -4.72766e-10
r = 1.1795, theta = 1.1795, r - theta = -5.93579e-10
r = 1.18014, theta = 1.18014, r - theta = -5.56524e-10
r = 1.18078, theta = 1.18078, r - theta = -3.18637e-10
r = 1.18141, theta = 1.18141, r - theta = 1.30587e-10
r = 1.18205, theta = 1.18205, r - theta = 3.94811e-10
r = 1.18269, theta = 1.18269, r - theta = 3.98981e-10
r = 1.18332, theta = 1.18332, r - theta = 2.34525e-10
r = 1.18396, theta = 1.18396, r - theta = -1.57863e-11
r = 1.1846, theta = 1.1846, r - theta = -2.77824e-10
r = 1.18523, theta = 1.18523, r - theta = -4.86085e-10
r = 1.18587, theta = 1.18587, r - theta = -5.83679e-10
r = 1.18651, theta = 1.18651, r - theta = -5.22313e-10
r = 1.18714, theta = 1.18714, r - theta = -2.62279e-10
r = 1.18778, theta = 1.18778, r - theta = 1.77534e-10
r = 1.18842, theta = 1.18842, r - theta = 3.94168e-10
r = 1.18905, theta = 1.18905, r - theta = 3.70894e-10
r = 1.18969, theta = 1.18969, r - theta = 1.9552e-10
r = 1.19032, theta = 1.19032, r - theta = -5.27112e-11
r = 1.19096, theta = 1.19096, r - theta = -3.03105e-10
r = 1.19159, theta = 1.19159, r - theta = -4.93504e-10
r = 1.19223, theta = 1.19223, r - theta = -5.7027e-10
r = 1.19287, theta = 1.19287, r - theta = -4.88272e-10
r = 1.1935, theta = 1.1935, r - theta = -2.10872e-10
r = 1.19414, theta = 1.19414, r - theta = 2.10628e-10
r = 1.19477, theta = 1.19477, r - theta = 3.87787e-10
r = 1.19541, theta = 1.19541, r - theta = 3.42937e-10
r = 1.19604, theta = 1.19604, r - theta = 1.60458e-10
r = 1.19668, theta = 1.19668, r - theta = -8.37481e-11
r = 1.19731, theta = 1.19731, r - theta = -3.22235e-10
r = 1.19794, theta = 1.19794, r - theta = -4.96004e-10
r = 1.19858, theta = 1.19858, r - theta = -5.54486e-10
r = 1.19921, theta = 1.19921, r - theta = -4.55531e-10
r = 1.19985, theta = 1.19985, r - theta = -1.6539e-10
r = 1.20048, theta = 1.20048, r - theta = 2.32741e-10
r = 1.20112, theta = 1.20112, r - theta = 3.7749e-10
r = 1.20175, theta = 1.20175, r - theta = 3.16058e-10
r = 1.20238, theta = 1.20238, r - theta = 1.2958e-10
r = 1.20302, theta = 1.20302, r - theta = -1.09195e-10
r = 1.20365, theta = 1.20365, r - theta = -3.35889e-10
r = 1.20429, theta = 1.20429, r - theta = -4.94482e-10
r = 1.20492, theta = 1.20492, r - theta = -5.37295e-10
r = 1.20555, theta = 1.20555, r - theta = -4.24981e-10
r = 1.20619, theta = 1.20619, r - theta = -1.26507e-10
r = 1.20682, theta = 1.20682, r - theta = 2.46196e-10
r = 1.20745, theta = 1.20745, r - theta = 3.64683e-10
r = 1.20808, theta = 1.20808, r - theta = 2.90912e-10
r = 1.20872, theta = 1.20872, r - theta = 1.02953e-10
r = 1.20935, theta = 1.20935, r - theta = -1.29421e-10
r = 1.20998, theta = 1.20998, r - theta = -3.44722e-10
r = 1.21062, theta = 1.21062, r - theta = -4.89734e-10
r = 1.21125, theta = 1.21125, r - theta = -5.19493e-10
r = 1.21188, theta = 1.21188, r - theta = -3.97284e-10
r = 1.21251, theta = 1.21251, r - theta = -9.46176e-11
r = 1.21315, theta = 1.21315, r - theta = 2.52848e-10
r = 1.21378, theta = 1.21378, r - theta = 3.5043e-10
r = 1.21441, theta = 1.21441, r - theta = 2.67924e-10
r = 1.21504, theta = 1.21504, r - theta = 8.05118e-11
r = 1.21567, theta = 1.21567, r - theta = -1.44837e-10
r = 1.21631, theta = 1.21631, r - theta = -3.49351e-10
r = 1.21694, theta = 1.21694, r - theta = -4.82445e-10
r = 1.21757, theta = 1.21757, r - theta = -5.01706e-10
r = 1.2182, theta = 1.2182, r - theta = -3.72879e-10
r = 1.21883, theta = 1.21883, r - theta = -6.98539e-11
r = 1.21946, theta = 1.21946, r - theta = 2.54164e-10
r = 1.22009, theta = 1.22009, r - theta = 3.35515e-10
r = 1.22072, theta = 1.22072, r - theta = 2.47338e-10
r = 1.22136, theta = 1.22136, r - theta = 6.20994e-11
r = 1.22199, theta = 1.22199, r - theta = -1.55866e-10
r = 1.22262, theta = 1.22262, r - theta = -3.50336e-10
r = 1.22325, theta = 1.22325, r - theta = -4.73191e-10
r = 1.22388, theta = 1.22388, r - theta = -4.84399e-10
r = 1.22451, theta = 1.22451, r - theta = -3.52006e-10
r = 1.22514, theta = 1.22514, r - theta = -5.21185e-11
r = 1.22577, theta = 1.22577, r - theta = 2.51281e-10
r = 1.2264, theta = 1.2264, r - theta = 3.20488e-10
r = 1.22703, theta = 1.22703, r - theta = 2.29254e-10
r = 1.22766, theta = 1.22766, r - theta = 4.74916e-11
r = 1.22829, theta = 1.22829, r - theta = -1.62931e-10
r = 1.22892, theta = 1.22892, r - theta = -3.48177e-10
r = 1.22955, theta = 1.22955, r - theta = -4.62432e-10
r = 1.23018, theta = 1.23018, r - theta = -4.67886e-10
r = 1.23081, theta = 1.23081, r - theta = -3.34724e-10
r = 1.23144, theta = 1.23144, r - theta = -4.11124e-11
r = 1.23207, theta = 1.23207, r - theta = 2.45059e-10
r = 1.2327, theta = 1.2327, r - theta = 3.05713e-10
r = 1.23333, theta = 1.23333, r - theta = 2.13659e-10
r = 1.23395, theta = 1.23395, r - theta = 3.64215e-11
r = 1.23458, theta = 1.23458, r - theta = -1.66438e-10
r = 1.23521, theta = 1.23521, r - theta = -3.43312e-10
r = 1.23584, theta = 1.23584, r - theta = -4.5053e-10
r = 1.23647, theta = 1.23647, r - theta = -4.52347e-10
r = 1.2371, theta = 1.2371, r - theta = -3.20935e-10
r = 1.23773, theta = 1.23773, r - theta = -3.63649e-11
r = 1.23835, theta = 1.23835, r - theta = 2.36129e-10
r = 1.23898, theta = 1.23898, r - theta = 2.91399e-10
r = 1.23961, theta = 1.23961, r - theta = 2.00457e-10
r = 1.24024, theta = 1.24024, r - theta = 2.85958e-11
r = 1.24087, theta = 1.24087, r - theta = -1.66774e-10
r = 1.24149, theta = 1.24149, r - theta = -3.36114e-10
r = 1.24212, theta = 1.24212, r - theta = -4.37748e-10
r = 1.24275, theta = 1.24275, r - theta = -4.37844e-10
r = 1.24338, theta = 1.24338, r - theta = -3.10408e-10
r = 1.24401, theta = 1.24401, r - theta = -3.72671e-11
r = 1.24463, theta = 1.24463, r - theta = 2.24929e-10
r = 1.24526, theta = 1.24526, r - theta = 2.7763e-10
r = 1.24589, theta = 1.24589, r - theta = 1.89482e-10
r = 1.24651, theta = 1.24651, r - theta = 2.37028e-11
r = 1.24714, theta = 1.24714, r - theta = -1.64297e-10
r = 1.24777, theta = 1.24777, r - theta = -3.26899e-10
r = 1.2484, theta = 1.2484, r - theta = -4.2427e-10
r = 1.24902, theta = 1.24902, r - theta = -4.24341e-10
r = 1.24965, theta = 1.24965, r - theta = -3.02806e-10
r = 1.25027, theta = 1.25027, r - theta = -4.31049e-11
r = 1.2509, theta = 1.2509, r - theta = 2.11735e-10
r = 1.25153, theta = 1.25153, r - theta = 2.64387e-10
r = 1.25215, theta = 1.25215, r - theta = 1.8052e-10
r = 1.25278, theta = 1.25278, r - theta = 2.1422e-11
r = 1.25341, theta = 1.25341, r - theta = -1.59344e-10
r = 1.25403, theta = 1.25403, r - theta = -3.15934e-10
r = 1.25466, theta = 1.25466, r - theta = -4.10208e-10
r = 1.25528, theta = 1.25528, r - theta = -4.11723e-10
r = 1.25591, theta = 1.25591, r - theta = -2.97714e-10
r = 1.25653, theta = 1.25653, r - theta = -5.30891e-11
r = 1.25716, theta = 1.25716, r - theta = 1.96685e-10
r = 1.25779, theta = 1.25779, r - theta = 2.51569e-10
r = 1.25841, theta = 1.25841, r - theta = 1.73313e-10
r = 1.25904, theta = 1.25904, r - theta = 2.143e-11
r = 1.25966, theta = 1.25966, r - theta = -1.52226e-10
r = 1.26029, theta = 1.26029, r - theta = -3.03439e-10
r = 1.26091, theta = 1.26091, r - theta = -3.95624e-10
r = 1.26154, theta = 1.26154, r - theta = -3.99816e-10
r = 1.26216, theta = 1.26216, r - theta = -2.94657e-10
r = 1.26279, theta = 1.26279, r - theta = -6.63851e-11
r = 1.26341, theta = 1.26341, r - theta = 1.79809e-10
r = 1.26403, theta = 1.26403, r - theta = 2.39005e-10
r = 1.26466, theta = 1.26466, r - theta = 1.67578e-10
r = 1.26528, theta = 1.26528, r - theta = 2.34004e-11
r = 1.26591, theta = 1.26591, r - theta = -1.43235e-10
r = 1.26653, theta = 1.26653, r - theta = -2.89603e-10
r = 1.26715, theta = 1.26715, r - theta = -3.80535e-10
r = 1.26778, theta = 1.26778, r - theta = -3.88409e-10
r = 1.2684, theta = 1.2684, r - theta = -2.93134e-10
r = 1.26903, theta = 1.26903, r - theta = -8.21441e-11
r = 1.26965, theta = 1.26965, r - theta = 1.61035e-10
r = 1.27027, theta = 1.27027, r - theta = 2.26467e-10
r = 1.2709, theta = 1.2709, r - theta = 1.63e-10
r = 1.27152, theta = 1.27152, r - theta = 2.70055e-11
r = 1.27214, theta = 1.27214, r - theta = -1.32649e-10
r = 1.27277, theta = 1.27277, r - theta = -2.74592e-10
r = 1.27339, theta = 1.27339, r - theta = -3.64936e-10
r = 1.27401, theta = 1.27401, r - theta = -3.77267e-10
r = 1.27463, theta = 1.27463, r - theta = -2.92631e-10
r = 1.27526, theta = 1.27526, r - theta = -9.95273e-11
r = 1.27588, theta = 1.27588, r - theta = 1.40217e-10
r = 1.2765, theta = 1.2765, r - theta = 2.13684e-10
r = 1.27712, theta = 1.27712, r - theta = 1.59247e-10
r = 1.27775, theta = 1.27775, r - theta = 3.19167e-11
r = 1.27837, theta = 1.27837, r - theta = -1.20734e-10
r = 1.27899, theta = 1.27899, r - theta = -2.58554e-10
r = 1.27961, theta = 1.27961, r - theta = -3.48806e-10
r = 1.28024, theta = 1.28024, r - theta = -3.66152e-10
r = 1.28086, theta = 1.28086, r - theta = -2.92649e-10
r = 1.28148, theta = 1.28148, r - theta = -1.17729e-10
r = 1.2821, theta = 1.2821, r - theta = 1.17138e-10
r = 1.28272, theta = 1.28272, r - theta = 2.00342e-10
r = 1.28334, theta = 1.28334, r - theta = 1.55972e-10
r = 1.28397, theta = 1.28397, r - theta = 3.78033e-11
r = 1.28459, theta = 1.28459, r - theta = -1.07751e-10
r = 1.28521, theta = 1.28521, r - theta = -2.41632e-10
r = 1.28583, theta = 1.28583, r - theta = -3.32124e-10
r = 1.28645, theta = 1.28645, r - theta = -3.5484e-10
r = 1.28707, theta = 1.28707, r - theta = -2.92717e-10
r = 1.28769, theta = 1.28769, r - theta = -1.36001e-10
r = 1.28831, theta = 1.28831, r - theta = 9.15261e-11
r = 1.28893, theta = 1.28893, r - theta = 1.86099e-10
r = 1.28955, theta = 1.28955, r - theta = 1.52809e-10
r = 1.29017, theta = 1.29017, r - theta = 4.43294e-11
r = 1.29079, theta = 1.29079, r - theta = -9.39619e-11
r = 1.29141, theta = 1.29141, r - theta = -2.23972e-10
r = 1.29204, theta = 1.29204, r - theta = -3.1488e-10
r = 1.29266, theta = 1.29266, r - theta = -3.43128e-10
r = 1.29328, theta = 1.29328, r - theta = -2.92411e-10
r = 1.2939, theta = 1.2939, r - theta = -1.53666e-10
r = 1.29451, theta = 1.29451, r - theta = 6.30616e-11
r = 1.29513, theta = 1.29513, r - theta = 1.70584e-10
r = 1.29575, theta = 1.29575, r - theta = 1.49384e-10
r = 1.29637, theta = 1.29637, r - theta = 5.11573e-11
r = 1.29699, theta = 1.29699, r - theta = -7.96292e-11
r = 1.29761, theta = 1.29761, r - theta = -2.05724e-10
r = 1.29823, theta = 1.29823, r - theta = -2.97081e-10
r = 1.29885, theta = 1.29885, r - theta = -3.3085e-10
r = 1.29947, theta = 1.29947, r - theta = -2.91365e-10
r = 1.30009, theta = 1.30009, r - theta = -1.70134e-10
r = 1.30071, theta = 1.30071, r - theta = 3.13842e-11
r = 1.30133, theta = 1.30133, r - theta = 1.53403e-10
r = 1.30195, theta = 1.30195, r - theta = 1.45309e-10
r = 1.30256, theta = 1.30256, r - theta = 5.79417e-11
r = 1.30318, theta = 1.30318, r - theta = -6.50227e-11
r = 1.3038, theta = 1.3038, r - theta = -1.87054e-10
r = 1.30442, theta = 1.30442, r - theta = -2.78763e-10
r = 1.30504, theta = 1.30504, r - theta = -3.17886e-10
r = 1.30566, theta = 1.30566, r - theta = -2.89283e-10
r = 1.30627, theta = 1.30627, r - theta = -1.84917e-10
r = 1.30689, theta = 1.30689, r - theta = -3.85492e-12
r = 1.30751, theta = 1.30751, r - theta = 1.3415e-10
r = 1.30813, theta = 1.30813, r - theta = 1.40193e-10
r = 1.30874, theta = 1.30874, r - theta = 6.4335e-11
r = 1.30936, theta = 1.30936, r - theta = -5.04181e-11
r = 1.30998, theta = 1.30998, r - theta = -1.68142e-10
r = 1.3106, theta = 1.3106, r - theta = -2.59986e-10
r = 1.31121, theta = 1.31121, r - theta = -3.04164e-10
r = 1.31183, theta = 1.31183, r - theta = -2.85943e-10
r = 1.31245, theta = 1.31245, r - theta = -1.97631e-10
r = 1.31307, theta = 1.31307, r - theta = -3.85725e-11
r = 1.31368, theta = 1.31368, r - theta = 1.12404e-10
r = 1.3143, theta = 1.3143, r - theta = 1.33636e-10
r = 1.31492, theta = 1.31492, r - theta = 6.99849e-11
r = 1.31553, theta = 1.31553, r - theta = -3.60996e-11
r = 1.31615, theta = 1.31615, r - theta = -1.49185e-10
r = 1.31677, theta = 1.31677, r - theta = -2.40849e-10
r = 1.31738, theta = 1.31738, r - theta = -2.89666e-10
r = 1.318, theta = 1.318, r - theta = -2.81202e-10
r = 1.31862, theta = 1.31862, r - theta = -2.08001e-10
r = 1.31923, theta = 1.31923, r - theta = -6.9577e-11
r = 1.31985, theta = 1.31985, r - theta = 8.77352e-11
r = 1.32046, theta = 1.32046, r - theta = 1.25238e-10
r = 1.32108, theta = 1.32108, r - theta = 7.45382e-11
r = 1.3217, theta = 1.3217, r - theta = -2.23588e-11
r = 1.32231, theta = 1.32231, r - theta = -1.304e-10
r = 1.32293, theta = 1.32293, r - theta = -2.21481e-10
r = 1.32354, theta = 1.32354, r - theta = -2.74429e-10
r = 1.32416, theta = 1.32416, r - theta = -2.75001e-10
r = 1.32477, theta = 1.32477, r - theta = -2.15867e-10
r = 1.32539, theta = 1.32539, r - theta = -9.66074e-11
r = 1.326, theta = 1.326, r - theta = 5.97149e-11
r = 1.32662, theta = 1.32662, r - theta = 1.14601e-10
r = 1.32723, theta = 1.32723, r - theta = 7.76426e-11
r = 1.32785, theta = 1.32785, r - theta = -9.49019e-12
r = 1.32846, theta = 1.32846, r - theta = -1.12017e-10
r = 1.32908, theta = 1.32908, r - theta = -2.02041e-10
r = 1.32969, theta = 1.32969, r - theta = -2.5854e-10
r = 1.33031, theta = 1.33031, r - theta = -2.67353e-10
r = 1.33092, theta = 1.33092, r - theta = -2.21175e-10
r = 1.33154, theta = 1.33154, r - theta = -1.19548e-10
r = 1.33215, theta = 1.33215, r - theta = 2.79126e-11
r = 1.33276, theta = 1.33276, r - theta = 1.01333e-10
r = 1.33338, theta = 1.33338, r - theta = 7.89513e-11
r = 1.33399, theta = 1.33399, r - theta = 2.20979e-12
r = 1.33461, theta = 1.33461, r - theta = -9.42781e-11
r = 1.33522, theta = 1.33522, r - theta = -1.8272e-10
r = 1.33583, theta = 1.33583, r - theta = -2.42135e-10
r = 1.33645, theta = 1.33645, r - theta = -2.58349e-10
r = 1.33706, theta = 1.33706, r - theta = -2.23976e-10
r = 1.33767, theta = 1.33767, r - theta = -1.38417e-10
r = 1.33829, theta = 1.33829, r - theta = -7.84772e-12
r = 1.3389, theta = 1.3389, r - theta = 8.5056e-11
r = 1.33951, theta = 1.33951, r - theta = 7.81297e-11
r = 1.34013, theta = 1.34013, r - theta = 1.24516e-11
r = 1.34074, theta = 1.34074, r - theta = -7.74287e-11
r = 1.34135, theta = 1.34135, r - theta = -1.63723e-10
r = 1.34196, theta = 1.34196, r - theta = -2.25392e-10
r = 1.34258, theta = 1.34258, r - theta = -2.48141e-10
r = 1.34319, theta = 1.34319, r - theta = -2.24411e-10
r = 1.3438, theta = 1.3438, r - theta = -1.53362e-10
r = 1.34441, theta = 1.34441, r - theta = -4.08746e-11
r = 1.34503, theta = 1.34503, r - theta = 6.54041e-11
r = 1.34564, theta = 1.34564, r - theta = 7.48583e-11
r = 1.34625, theta = 1.34625, r - theta = 2.09557e-11
r = 1.34686, theta = 1.34686, r - theta = -6.17162e-11
r = 1.34748, theta = 1.34748, r - theta = -1.45271e-10
r = 1.34809, theta = 1.34809, r - theta = -2.08515e-10
r = 1.3487, theta = 1.3487, r - theta = -2.36939e-10
r = 1.34931, theta = 1.34931, r - theta = -2.22705e-10
r = 1.34992, theta = 1.34992, r - theta = -1.64644e-10
r = 1.35053, theta = 1.35053, r - theta = -6.82414e-11
r = 1.35115, theta = 1.35115, r - theta = 4.2039e-11
r = 1.35176, theta = 1.35176, r - theta = 6.88409e-11
r = 1.35237, theta = 1.35237, r - theta = 2.74618e-11
r = 1.35298, theta = 1.35298, r - theta = -4.73743e-11
r = 1.35359, theta = 1.35359, r - theta = -1.27589e-10
r = 1.3542, theta = 1.3542, r - theta = -1.91734e-10
r = 1.35481, theta = 1.35481, r - theta = -2.24989e-10
r = 1.35542, theta = 1.35542, r - theta = -2.19149e-10
r = 1.35603, theta = 1.35603, r - theta = -1.72617e-10
r = 1.35664, theta = 1.35664, r - theta = -9.03941e-11
r = 1.35725, theta = 1.35725, r - theta = 1.46509e-11
r = 1.35787, theta = 1.35787, r - theta = 5.98135e-11
r = 1.35848, theta = 1.35848, r - theta = 3.17395e-11
r = 1.35909, theta = 1.35909, r - theta = -3.46176e-11
r = 1.3597, theta = 1.3597, r - theta = -1.10888e-10
r = 1.36031, theta = 1.36031, r - theta = -1.75279e-10
r = 1.36092, theta = 1.36092, r - theta = -2.12566e-10
r = 1.36153, theta = 1.36153, r - theta = -2.14083e-10
r = 1.36214, theta = 1.36214, r - theta = -1.77715e-10
r = 1.36275, theta = 1.36275, r - theta = -1.07887e-10
r = 1.36336, theta = 1.36336, r - theta = -1.55602e-11
r = 1.36396, theta = 1.36396, r - theta = 4.75506e-11
r = 1.36457, theta = 1.36457, r - theta = 3.35929e-11
r = 1.36518, theta = 1.36518, r - theta = -2.36278e-11
r = 1.36579, theta = 1.36579, r - theta = -9.53611e-11
r = 1.3664, theta = 1.3664, r - theta = -1.59376e-10
r = 1.36701, theta = 1.36701, r - theta = -1.99954e-10
r = 1.36762, theta = 1.36762, r - theta = -2.07877e-10
r = 1.36823, theta = 1.36823, r - theta = -1.80422e-10
r = 1.36884, theta = 1.36884, r - theta = -1.21355e-10
r = 1.36945, theta = 1.36945, r - theta = -4.09155e-11
r = 1.37006, theta = 1.37006, r - theta = 3.18745e-11
r = 1.37066, theta = 1.37066, r - theta = 3.28779e-11
r = 1.37127, theta = 1.37127, r - theta = -1.45417e-11
r = 1.37188, theta = 1.37188, r - theta = -8.11604e-11
r = 1.37249, theta = 1.37249, r - theta = -1.44221e-10
r = 1.3731, theta = 1.3731, r - theta = -1.87421e-10
r = 1.37371, theta = 1.37371, r - theta = -2.00905e-10
r = 1.37431, theta = 1.37431, r - theta = -1.81257e-10
r = 1.37492, theta = 1.37492, r - theta = -1.31491e-10
r = 1.37553, theta = 1.37553, r - theta = -6.10427e-11
r = 1.37614, theta = 1.37614, r - theta = 1.26639e-11
r = 1.37674, theta = 1.37674, r - theta = 2.95064e-11
r = 1.37735, theta = 1.37735, r - theta = -7.43849e-12
r = 1.37796, theta = 1.37796, r - theta = -6.8388e-11
r = 1.37857, theta = 1.37857, r - theta = -1.29969e-10
r = 1.37917, theta = 1.37917, r - theta = -1.75209e-10
r = 1.37978, theta = 1.37978, r - theta = -1.9353e-10
r = 1.38039, theta = 1.38039, r - theta = -1.80736e-10
r = 1.381, theta = 1.381, r - theta = -1.3901e-10
r = 1.3816, theta = 1.3816, r - theta = -7.68998e-11
r = 1.38221, theta = 1.38221, r - theta = -9.31699e-12
r = 1.38282, theta = 1.38282, r - theta = 2.34595e-11
r = 1.38342, theta = 1.38342, r - theta = -2.32903e-12
r = 1.38403, theta = 1.38403, r - theta = -5.70768e-11
r = 1.38464, theta = 1.38464, r - theta = -1.1671e-10
r = 1.38524, theta = 1.38524, r - theta = -1.63504e-10
r = 1.38585, theta = 1.38585, r - theta = -1.86071e-10
r = 1.38646, theta = 1.38646, r - theta = -1.79356e-10
r = 1.38706, theta = 1.38706, r - theta = -1.44625e-10
r = 1.38767, theta = 1.38767, r - theta = -8.94609e-11
r = 1.38827, theta = 1.38827, r - theta = -2.77514e-11
r = 1.38888, theta = 1.38888, r - theta = 1.47997e-11
r = 1.38949, theta = 1.38949, r - theta = 8.60645e-13
r = 1.39009, theta = 1.39009, r - theta = -4.71765e-11
r = 1.3907, theta = 1.3907, r - theta = -1.04456e-10
r = 1.3913, theta = 1.3913, r - theta = -1.52417e-10
r = 1.39191, theta = 1.39191, r - theta = -1.78783e-10
r = 1.39251, theta = 1.39251, r - theta = -1.77557e-10
r = 1.39312, theta = 1.39312, r - theta = -1.49011e-10
r = 1.39373, theta = 1.39373, r - theta = -9.96805e-11
r = 1.39433, theta = 1.39433, r - theta = -4.23541e-11
r = 1.39494, theta = 1.39494, r - theta = 3.67839e-12
r = 1.39554, theta = 1.39554, r - theta = 2.30371e-12
r = 1.39615, theta = 1.39615, r - theta = -3.85341e-11
r = 1.39675, theta = 1.39675, r - theta = -9.31144e-11
r = 1.39735, theta = 1.39735, r - theta = -1.4196e-10
r = 1.39796, theta = 1.39796, r - theta = -1.71827e-10
r = 1.39856, theta = 1.39856, r - theta = -1.75696e-10
r = 1.39917, theta = 1.39917, r - theta = -1.52769e-10
r = 1.39977, theta = 1.39977, r - theta = -1.08456e-10
r = 1.40038, theta = 1.40038, r - theta = -5.43698e-11
r = 1.40098, theta = 1.40098, r - theta = -8.31912e-12
r = 1.40159, theta = 1.40159, r - theta = 2.27951e-12
r = 1.40219, theta = 1.40219, r - theta = -3.08822e-11
r = 1.40279, theta = 1.40279, r - theta = -8.24747e-11
r = 1.4034, theta = 1.4034, r - theta = -1.32026e-10
r = 1.404, theta = 1.404, r - theta = -1.65245e-10
r = 1.40461, theta = 1.40461, r - theta = -1.74017e-10
r = 1.40521, theta = 1.40521, r - theta = -1.56395e-10
r = 1.40581, theta = 1.40581, r - theta = -1.16589e-10
r = 1.40642, theta = 1.40642, r - theta = -6.4964e-11
r = 1.40702, theta = 1.40702, r - theta = -1.80256e-11
r = 1.40762, theta = 1.40762, r - theta = 1.19016e-12
r = 1.40823, theta = 1.40823, r - theta = -2.38212e-11
r = 1.40883, theta = 1.40883, r - theta = -7.21865e-11
r = 1.40943, theta = 1.40943, r - theta = -1.22362e-10
r = 1.41003, theta = 1.41003, r - theta = -1.58936e-10
r = 1.41064, theta = 1.41064, r - theta = -1.72621e-10
r = 1.41124, theta = 1.41124, r - theta = -1.60247e-10
r = 1.41184, theta = 1.41184, r - theta = -1.24752e-10
r = 1.41245, theta = 1.41245, r - theta = -7.51774e-11
r = 1.41305, theta = 1.41305, r - theta = -2.66571e-11
r = 1.41365, theta = 1.41365, r - theta = -4.12559e-13
r = 1.41425, theta = 1.41425, r - theta = -1.6807e-11
r = 1.41485, theta = 1.41485, r - theta = -6.17431e-11
r = 1.41546, theta = 1.41546, r - theta = -1.12552e-10
r = 1.41606, theta = 1.41606, r - theta = -1.52625e-10
r = 1.41666, theta = 1.41666, r - theta = -1.71431e-10
r = 1.41726, theta = 1.41726, r - theta = -1.64505e-10
r = 1.41786, theta = 1.41786, r - theta = -1.3344e-10
r = 1.41847, theta = 1.41847, r - theta = -8.58831e-11
r = 1.41907, theta = 1.41907, r - theta = -3.55258e-11
r = 1.41967, theta = 1.41967, r - theta = -2.09788e-12
r = 1.42027, theta = 1.42027, r - theta = -9.13736e-12
r = 1.42087, theta = 1.42087, r - theta = -5.04659e-11
r = 1.42147, theta = 1.42147, r - theta = -1.01994e-10
r = 1.42208, theta = 1.42208, r - theta = -1.45846e-10
r = 1.42268, theta = 1.42268, r - theta = -1.70174e-10
r = 1.42328, theta = 1.42328, r - theta = -1.69145e-10
r = 1.42388, theta = 1.42388, r - theta = -1.42941e-10
r = 1.42448, theta = 1.42448, r - theta = -9.77447e-11
r = 1.42508, theta = 1.42508, r - theta = -4.57367e-11
r = 1.42568, theta = 1.42568, r - theta = -5.08549e-12
r = 1.42628, theta = 1.42628, r - theta = 5.66214e-14
r = 1.42688, theta = 1.42688, r - theta = -3.74882e-11
r = 1.42748, theta = 1.42748, r - theta = -8.98821e-11
r = 1.42808, theta = 1.42808, r - theta = -1.37911e-10
r = 1.42868, theta = 1.42868, r - theta = -1.68339e-10
r = 1.42928, theta = 1.42928, r - theta = -1.73901e-10
r = 1.42988, theta = 1.42988, r - theta = -1.53295e-10
r = 1.43048, theta = 1.43048, r - theta = -1.11174e-10
r = 1.43108, theta = 1.43108, r - theta = -5.81393e-11
r = 1.43168, theta = 1.43168, r - theta = -1.07343e-11
r = 1.43228, theta = 1.43228, r - theta = 8.56337e-12
r = 1.43288, theta = 1.43288, r - theta = -2.17459e-11
r = 1.43348, theta = 1.43348, r - theta = -7.51912e-11
r = 1.43408, theta = 1.43408, r - theta = -1.27895e-10
r = 1.43468, theta = 1.43468, r - theta = -1.65168e-10
r = 1.43528, theta = 1.43528, r - theta = -1.78241e-10
r = 1.43588, theta = 1.43588, r - theta = -1.64265e-10
r = 1.43648, theta = 1.43648, r - theta = -1.26294e-10
r = 1.43708, theta = 1.43708, r - theta = -7.32887e-11
r = 1.43768, theta = 1.43768, r - theta = -2.01004e-11
r = 1.43828, theta = 1.43828, r - theta = 1.25309e-11
r = 1.43888, theta = 1.43888, r - theta = -1.9662e-12
r = 1.43947, theta = 1.43947, r - theta = -5.6662e-11
r = 1.44007, theta = 1.44007, r - theta = -1.14616e-10
r = 1.44067, theta = 1.44067, r - theta = -1.59618e-10
r = 1.44127, theta = 1.44127, r - theta = -1.81333e-10
r = 1.44187, theta = 1.44187, r - theta = -1.75295e-10
r = 1.44247, theta = 1.44247, r - theta = -1.42899e-10
r = 1.44306, theta = 1.44306, r - theta = -9.13967e-11
r = 1.44366, theta = 1.44366, r - theta = -3.3888e-11
r = 1.44426, theta = 1.44426, r - theta = 1.06886e-11
r = 1.44486, theta = 1.44486, r - theta = 1.7558e-11
r = 1.44546, theta = 1.44546, r - theta = -3.27904e-11
r = 1.44605, theta = 1.44605, r - theta = -9.66198e-11
r = 1.44665, theta = 1.44665, r - theta = -1.50352e-10
r = 1.44725, theta = 1.44725, r - theta = -1.82023e-10
r = 1.44785, theta = 1.44785, r - theta = -1.85487e-10
r = 1.44845, theta = 1.44845, r - theta = -1.60417e-10
r = 1.44904, theta = 1.44904, r - theta = -1.12294e-10
r = 1.44964, theta = 1.44964, r - theta = -5.24027e-11
r = 1.45024, theta = 1.45024, r - theta = 2.17781e-12
r = 1.45083, theta = 1.45083, r - theta = 2.85798e-11
r = 1.45143, theta = 1.45143, r - theta = -1.81988e-12
r = 1.45203, theta = 1.45203, r - theta = -7.21636e-11
r = 1.45263, theta = 1.45263, r - theta = -1.35714e-10
r = 1.45322, theta = 1.45322, r - theta = -1.78811e-10
r = 1.45382, theta = 1.45382, r - theta = -1.93571e-10
r = 1.45442, theta = 1.45442, r - theta = -1.77881e-10
r = 1.45501, theta = 1.45501, r - theta = -1.35391e-10
r = 1.45561, theta = 1.45561, r - theta = -7.5512e-11
r = 1.45621, theta = 1.45621, r - theta = -1.34022e-11
r = 1.4568, theta = 1.4568, r - theta = 3.00358e-11
r = 1.4574, theta = 1.4574, r - theta = 2.81599e-11
r = 1.45799, theta = 1.45799, r - theta = -3.92166e-11
r = 1.45859, theta = 1.45859, r - theta = -1.13721e-10
r = 1.45919, theta = 1.45919, r - theta = -1.69837e-10
r = 1.45978, theta = 1.45978, r - theta = -1.97882e-10
r = 1.46038, theta = 1.46038, r - theta = -1.93897e-10
r = 1.46097, theta = 1.46097, r - theta = -1.59645e-10
r = 1.46157, theta = 1.46157, r - theta = -1.026e-10
r = 1.46217, theta = 1.46217, r - theta = -3.59444e-11
r = 1.46276, theta = 1.46276, r - theta = 2.14417e-11
r = 1.46336, theta = 1.46336, r - theta = 4.49834e-11
r = 1.46395, theta = 1.46395, r - theta = 4.42024e-12
r = 1.46455, theta = 1.46455, r - theta = -8.20517e-11
r = 1.46514, theta = 1.46514, r - theta = -1.52863e-10
r = 1.46574, theta = 1.46574, r - theta = -1.96342e-10
r = 1.46633, theta = 1.46633, r - theta = -2.06626e-10
r = 1.46693, theta = 1.46693, r - theta = -1.83531e-10
r = 1.46752, theta = 1.46752, r - theta = -1.32539e-10
r = 1.46812, theta = 1.46812, r - theta = -6.47993e-11
r = 1.46871, theta = 1.46871, r - theta = 2.88725e-12
r = 1.46931, theta = 1.46931, r - theta = 4.80669e-11
r = 1.4699, theta = 1.4699, r - theta = 4.26443e-11
r = 1.4705, theta = 1.4705, r - theta = -3.80427e-11
r = 1.47109, theta = 1.47109, r - theta = -1.25265e-10
r = 1.47168, theta = 1.47168, r - theta = -1.86442e-10
r = 1.47228, theta = 1.47228, r - theta = -2.13754e-10
r = 1.47287, theta = 1.47287, r - theta = -2.05012e-10
r = 1.47347, theta = 1.47347, r - theta = -1.63652e-10
r = 1.47406, theta = 1.47406, r - theta = -9.87308e-11
r = 1.47465, theta = 1.47465, r - theta = -2.49194e-11
r = 1.47525, theta = 1.47525, r - theta = 3.75056e-11
r = 1.47584, theta = 1.47584, r - theta = 6.26663e-11
r = 1.47644, theta = 1.47644, r - theta = 1.90923e-11
r = 1.47703, theta = 1.47703, r - theta = -8.40259e-11
r = 1.47762, theta = 1.47762, r - theta = -1.65236e-10
r = 1.47822, theta = 1.47822, r - theta = -2.12484e-10
r = 1.47881, theta = 1.47881, r - theta = -2.21522e-10
r = 1.4794, theta = 1.4794, r - theta = -1.93686e-10
r = 1.48, theta = 1.48, r - theta = -1.3589e-10
r = 1.48059, theta = 1.48059, r - theta = -6.06155e-11
r = 1.48118, theta = 1.48118, r - theta = 1.40872e-11
r = 1.48177, theta = 1.48177, r - theta = 6.46116e-11
r = 1.48237, theta = 1.48237, r - theta = 6.17986e-11
r = 1.48296, theta = 1.48296, r - theta = -2.57476e-11
r = 1.48355, theta = 1.48355, r - theta = -1.29337e-10
r = 1.48415, theta = 1.48415, r - theta = -1.99525e-10
r = 1.48474, theta = 1.48474, r - theta = -2.29952e-10
r = 1.48533, theta = 1.48533, r - theta = -2.198e-10
r = 1.48592, theta = 1.48592, r - theta = -1.73783e-10
r = 1.48651, theta = 1.48651, r - theta = -1.02149e-10
r = 1.48711, theta = 1.48711, r - theta = -2.06648e-11
r = 1.4877, theta = 1.4877, r - theta = 4.93827e-11
r = 1.48829, theta = 1.48829, r - theta = 8.11984e-11
r = 1.48888, theta = 1.48888, r - theta = 4.24818e-11
r = 1.48947, theta = 1.48947, r - theta = -7.49156e-11
r = 1.49007, theta = 1.49007, r - theta = -1.71088e-10
r = 1.49066, theta = 1.49066, r - theta = -2.26638e-10
r = 1.49125, theta = 1.49125, r - theta = -2.38538e-10
r = 1.49184, theta = 1.49184, r - theta = -2.09257e-10
r = 1.49243, theta = 1.49243, r - theta = -1.46753e-10
r = 1.49302, theta = 1.49302, r - theta = -6.44635e-11
r = 1.49362, theta = 1.49362, r - theta = 1.86982e-11
r = 1.49421, theta = 1.49421, r - theta = 7.83473e-11
r = 1.4948, theta = 1.4948, r - theta = 8.46356e-11
r = 1.49539, theta = 1.49539, r - theta = 2.25975e-12
r = 1.49598, theta = 1.49598, r - theta = -1.22894e-10
r = 1.49657, theta = 1.49657, r - theta = -2.07359e-10
r = 1.49716, theta = 1.49716, r - theta = -2.45832e-10
r = 1.49775, theta = 1.49775, r - theta = -2.38483e-10
r = 1.49834, theta = 1.49834, r - theta = -1.90928e-10
r = 1.49893, theta = 1.49893, r - theta = -1.14229e-10
r = 1.49952, theta = 1.49952, r - theta = -2.48783e-11
r = 1.50011, theta = 1.50011, r - theta = 5.51998e-11
r = 1.5007, theta = 1.5007, r - theta = 9.86577e-11
r = 1.50129, theta = 1.50129, r - theta = 7.2732e-11
r = 1.50188, theta = 1.50188, r - theta = -5.01783e-11
r = 1.50247, theta = 1.50247, r - theta = -1.67347e-10
r = 1.50306, theta = 1.50306, r - theta = -2.36995e-10
r = 1.50365, theta = 1.50365, r - theta = -2.56945e-10
r = 1.50424, theta = 1.50424, r - theta = -2.30427e-10
r = 1.50483, theta = 1.50483, r - theta = -1.66072e-10
r = 1.50542, theta = 1.50542, r - theta = -7.79088e-11
r = 1.50601, theta = 1.50601, r - theta = 1.46465e-11
r = 1.5066, theta = 1.5066, r - theta = 8.67939e-11
r = 1.50719, theta = 1.50719, r - theta = 1.08354e-10
r = 1.50778, theta = 1.50778, r - theta = 4.37792e-11
r = 1.50837, theta = 1.50837, r - theta = -1.01297e-10
r = 1.50896, theta = 1.50896, r - theta = -2.06732e-10
r = 1.50955, theta = 1.50955, r - theta = -2.59451e-10
r = 1.51014, theta = 1.51014, r - theta = -2.60249e-10
r = 1.51073, theta = 1.51073, r - theta = -2.15283e-10
r = 1.51132, theta = 1.51132, r - theta = -1.36068e-10
r = 1.5119, theta = 1.5119, r - theta = -3.94667e-11
r = 1.51249, theta = 1.51249, r - theta = 5.23115e-11
r = 1.51308, theta = 1.51308, r - theta = 1.1172e-10
r = 1.51367, theta = 1.51367, r - theta = 1.05877e-10
r = 1.51426, theta = 1.51426, r - theta = -3.38507e-12
r = 1.51485, theta = 1.51485, r - theta = -1.49153e-10
r = 1.51544, theta = 1.51544, r - theta = -2.40135e-10
r = 1.51602, theta = 1.51602, r - theta = -2.74648e-10
r = 1.51661, theta = 1.51661, r - theta = -2.56328e-10
r = 1.5172, theta = 1.5172, r - theta = -1.94131e-10
r = 1.51779, theta = 1.51779, r - theta = -1.02323e-10
r = 1.51838, theta = 1.51838, r - theta = -4.78062e-13
r = 1.51896, theta = 1.51896, r - theta = 8.65323e-11
r = 1.51955, theta = 1.51955, r - theta = 1.2854e-10
r = 1.52014, theta = 1.52014, r - theta = 9.00908e-11
r = 1.52073, theta = 1.52073, r - theta = -5.77967e-11
r = 1.52131, theta = 1.52131, r - theta = -1.92478e-10
r = 1.5219, theta = 1.5219, r - theta = -2.67139e-10
r = 1.52249, theta = 1.52249, r - theta = -2.82854e-10
r = 1.52308, theta = 1.52308, r - theta = -2.45974e-10
r = 1.52366, theta = 1.52366, r - theta = -1.68127e-10
r = 1.52425, theta = 1.52425, r - theta = -6.62057e-11
r = 1.52484, theta = 1.52484, r - theta = 3.76346e-11
r = 1.52543, theta = 1.52543, r - theta = 1.15982e-10
r = 1.52601, theta = 1.52601, r - theta = 1.36174e-10
r = 1.5266, theta = 1.5266, r - theta = 6.03024e-11
r = 1.52719, theta = 1.52719, r - theta = -1.09333e-10
r = 1.52777, theta = 1.52777, r - theta = -2.30524e-10
r = 1.52836, theta = 1.52836, r - theta = -2.87706e-10
r = 1.52895, theta = 1.52895, r - theta = -2.84587e-10
r = 1.52953, theta = 1.52953, r - theta = -2.30109e-10
r = 1.53012, theta = 1.53012, r - theta = -1.38443e-10
r = 1.5307, theta = 1.5307, r - theta = -2.89875e-11
r = 1.53129, theta = 1.53129, r - theta = 7.36424e-11
r = 1.53188, theta = 1.53188, r - theta = 1.39617e-10
r = 1.53246, theta = 1.53246, r - theta = 1.33899e-10
r = 1.53305, theta = 1.53305, r - theta = 1.62503e-11
r = 1.53363, theta = 1.53363, r - theta = -1.56922e-10
r = 1.53422, theta = 1.53422, r - theta = -2.62955e-10
r = 1.53481, theta = 1.53481, r - theta = -3.02086e-10
r = 1.53539, theta = 1.53539, r - theta = -2.80533e-10
r = 1.53598, theta = 1.53598, r - theta = -2.09706e-10
r = 1.53656, theta = 1.53656, r - theta = -1.06204e-10
r = 1.53715, theta = 1.53715, r - theta = 8.19544e-12
r = 1.53773, theta = 1.53773, r - theta = 1.06535e-10
r = 1.53832, theta = 1.53832, r - theta = 1.56688e-10
r = 1.5389, theta = 1.5389, r - theta = 1.2136e-10
r = 1.53949, theta = 1.53949, r - theta = -3.7814e-11
r = 1.54007, theta = 1.54007, r - theta = -1.99927e-10
r = 1.54066, theta = 1.54066, r - theta = -2.89749e-10
r = 1.54124, theta = 1.54124, r - theta = -3.1073e-10
r = 1.54183, theta = 1.54183, r - theta = -2.71477e-10
r = 1.54241, theta = 1.54241, r - theta = -1.85751e-10
r = 1.543, theta = 1.543, r - theta = -7.24547e-11
r = 1.54358, theta = 1.54358, r - theta = 4.43672e-11
r = 1.54417, theta = 1.54417, r - theta = 1.35535e-10
r = 1.54475, theta = 1.54475, r - theta = 1.6674e-10
r = 1.54533, theta = 1.54533, r - theta = 9.85509e-11
r = 1.54592, theta = 1.54592, r - theta = -8.92586e-11
r = 1.5465, theta = 1.5465, r - theta = -2.38053e-10
r = 1.54709, theta = 1.54709, r - theta = -3.11121e-10
r = 1.54767, theta = 1.54767, r - theta = -3.14227e-10
r = 1.54826, theta = 1.54826, r - theta = -2.58252e-10
r = 1.54884, theta = 1.54884, r - theta = -1.59187e-10
r = 1.54942, theta = 1.54942, r - theta = -3.81279e-11
r = 1.55001, theta = 1.55001, r - theta = 7.87324e-11
r = 1.55059, theta = 1.55059, r - theta = 1.60105e-10
r = 1.55117, theta = 1.55117, r - theta = 1.69616e-10
r = 1.55176, theta = 1.55176, r - theta = 6.58051e-11
r = 1.55234, theta = 1.55234, r - theta = -1.36729e-10
r = 1.55292, theta = 1.55292, r - theta = -2.71269e-10
r = 1.55351, theta = 1.55351, r - theta = -3.27454e-10
r = 1.55409, theta = 1.55409, r - theta = -3.13248e-10
r = 1.55467, theta = 1.55467, r - theta = -2.41692e-10
r = 1.55526, theta = 1.55526, r - theta = -1.30894e-10
r = 1.55584, theta = 1.55584, r - theta = -4.02611e-12
r = 1.55642, theta = 1.55642, r - theta = 1.10681e-10
r = 1.557, theta = 1.557, r - theta = 1.79943e-10
r = 1.55759, theta = 1.55759, r - theta = 1.65429e-10
r = 1.55817, theta = 1.55817, r - theta = 2.37654e-11
r = 1.55875, theta = 1.55875, r - theta = -1.7994e-10
r = 1.55933, theta = 1.55933, r - theta = -2.99744e-10
r = 1.55992, theta = 1.55992, r - theta = -3.3925e-10
r = 1.5605, theta = 1.5605, r - theta = -3.0851e-10
r = 1.56108, theta = 1.56108, r - theta = -2.2261e-10
r = 1.56166, theta = 1.56166, r - theta = -1.01665e-10
r = 1.56225, theta = 1.56225, r - theta = 2.91871e-11
r = 1.56283, theta = 1.56283, r - theta = 1.39788e-10
r = 1.56341, theta = 1.56341, r - theta = 1.94968e-10
r = 1.56399, theta = 1.56399, r - theta = 1.5455e-10
r = 1.56457, theta = 1.56457, r - theta = -2.51654e-11
r = 1.56515, theta = 1.56515, r - theta = -2.18827e-10
r = 1.56574, theta = 1.56574, r - theta = -3.2379e-10
r = 1.56632, theta = 1.56632, r - theta = -3.47082e-10
r = 1.5669, theta = 1.5669, r - theta = -3.00734e-10
r = 1.56748, theta = 1.56748, r - theta = -2.01767e-10
r = 1.56806, theta = 1.56806, r - theta = -7.2194e-11
r = 1.56864, theta = 1.56864, r - theta = 6.09881e-11
r = 1.56922, theta = 1.56922, r - theta = 1.65805e-10
r = 1.56981, theta = 1.56981, r - theta = 2.05307e-10
r = 1.57039, theta = 1.57039, r - theta = 1.37578e-10
r = 1.57097, theta = 1.57097, r - theta = -7.16573e-11
r = 1.57155, theta = 1.57155, r - theta = -2.53494e-10
r = 1.57213, theta = 1.57213, r - theta = -3.43816e-10
r = 1.57271, theta = 1.57271, r - theta = -3.51561e-10
r = 1.57329, theta = 1.57329, r - theta = -2.90626e-10
r = 1.57387, theta = 1.57387, r - theta = -1.79862e-10
r = 1.57445, theta = 1.57445, r - theta = -4.30733e-11
r = 1.57503, theta = 1.57503, r - theta = 9.0993e-11
r = 1.57561, theta = 1.57561, r - theta = 1.88649e-10
r = 1.57619, theta = 1.57619, r - theta = 2.11272e-10
r = 1.57677, theta = 1.57677, r - theta = 1.15309e-10
r = 1.57735, theta = 1.57735, r - theta = -1.14171e-10
r = 1.57793, theta = 1.57793, r - theta = -2.84165e-10
r = 1.57851, theta = 1.57851, r - theta = -3.60299e-10
r = 1.57909, theta = 1.57909, r - theta = -3.5331e-10
r = 1.57967, theta = 1.57967, r - theta = -2.78859e-10
r = 1.58025, theta = 1.58025, r - theta = -1.57524e-10
r = 1.58083, theta = 1.58083, r - theta = -1.47908e-11
r = 1.58141, theta = 1.58141, r - theta = 1.18944e-10
r = 1.58199, theta = 1.58199, r - theta = 2.08383e-10
r = 1.58257, theta = 1.58257, r - theta = 2.13332e-10
r = 1.58315, theta = 1.58315, r - theta = 8.8705e-11
r = 1.58373, theta = 1.58373, r - theta = -1.52717e-10
r = 1.58431, theta = 1.58431, r - theta = -3.11148e-10
r = 1.58489, theta = 1.58489, r - theta = -3.73744e-10
r = 1.58547, theta = 1.58547, r - theta = -3.52943e-10
r = 1.58604, theta = 1.58604, r - theta = -2.66063e-10
r = 1.58662, theta = 1.58662, r - theta = -1.35302e-10
r = 1.5872, theta = 1.5872, r - theta = 1.22657e-11
r = 1.58778, theta = 1.58778, r - theta = 1.447e-10
r = 1.58836, theta = 1.58836, r - theta = 2.25196e-10
r = 1.58894, theta = 1.58894, r - theta = 2.12088e-10
r = 1.58952, theta = 1.58952, r - theta = 5.88574e-11
r = 1.59009, theta = 1.59009, r - theta = -1.87408e-10
r = 1.59067, theta = 1.59067, r - theta = -3.34803e-10
r = 1.59125, theta = 1.59125, r - theta = -3.84673e-10
r = 1.59183, theta = 1.59183, r - theta = -3.5105e-10
r = 1.59241, theta = 1.59241, r - theta = -2.52812e-10
r = 1.59299, theta = 1.59299, r - theta = -1.13679e-10
r = 1.59356, theta = 1.59356, r - theta = 3.77944e-11
r = 1.59414, theta = 1.59414, r - theta = 1.68219e-10
r = 1.59472, theta = 1.59472, r - theta = 2.39381e-10
r = 1.5953, theta = 1.5953, r - theta = 2.08242e-10
r = 1.59587, theta = 1.59587, r - theta = 2.69489e-11
r = 1.59645, theta = 1.59645, r - theta = -2.18421e-10
r = 1.59703, theta = 1.59703, r - theta = -3.55517e-10
r = 1.59761, theta = 1.59761, r - theta = -3.93597e-10
r = 1.59818, theta = 1.59818, r - theta = -3.48189e-10
r = 1.59876, theta = 1.59876, r - theta = -2.39629e-10
r = 1.59934, theta = 1.59934, r - theta = -9.30613e-11
r = 1.59992, theta = 1.59992, r - theta = 6.15741e-11
r = 1.60049, theta = 1.60049, r - theta = 1.8954e-10
r = 1.60107, theta = 1.60107, r - theta = 2.51308e-10
r = 1.60165, theta = 1.60165, r - theta = 2.02566e-10
r = 1.60222, theta = 1.60222, r - theta = -5.72009e-12
r = 1.6028, theta = 1.6028, r - theta = -2.45969e-10
r = 1.60338, theta = 1.60338, r - theta = -3.73682e-10
r = 1.60395, theta = 1.60395, r - theta = -4.01009e-10
r = 1.60453, theta = 1.60453, r - theta = -3.44876e-10
r = 1.60511, theta = 1.60511, r - theta = -2.26982e-10
r = 1.60568, theta = 1.60568, r - theta = -7.37963e-11
r = 1.60626, theta = 1.60626, r - theta = 8.34492e-11
r = 1.60684, theta = 1.60684, r - theta = 2.08765e-10
r = 1.60741, theta = 1.60741, r - theta = 2.61403e-10
r = 1.60799, theta = 1.60799, r - theta = 1.95872e-10
r = 1.60856, theta = 1.60856, r - theta = -3.56875e-11
r = 1.60914, theta = 1.60914, r - theta = -2.70283e-10
r = 1.60972, theta = 1.60972, r - theta = -3.89682e-10
r = 1.61029, theta = 1.61029, r - theta = -4.07373e-10
r = 1.61087, theta = 1.61087, r - theta = -3.41585e-10
r = 1.61144, theta = 1.61144, r - theta = -2.15287e-10
r = 1.61202, theta = 1.61202, r - theta = -5.61777e-11
r = 1.61259, theta = 1.61259, r - theta = 1.03314e-10
r = 1.61317, theta = 1.61317, r - theta = 2.26036e-10
r = 1.61374, theta = 1.61374, r - theta = 2.70119e-10
r = 1.61432, theta = 1.61432, r - theta = 1.88978e-10
r = 1.61489, theta = 1.61489, r - theta = -6.15212e-11
r = 1.61547, theta = 1.61547, r - theta = -2.9159e-10
r = 1.61604, theta = 1.61604, r - theta = -4.03876e-10
r = 1.61662, theta = 1.61662, r - theta = -4.13115e-10
r = 1.61719, theta = 1.61719, r - theta = -3.38747e-10
r = 1.61777, theta = 1.61777, r - theta = -2.04913e-10
r = 1.61834, theta = 1.61834, r - theta = -4.04541e-11
r = 1.61892, theta = 1.61892, r - theta = 1.21098e-10
r = 1.61949, theta = 1.61949, r - theta = 2.41521e-10
r = 1.62007, theta = 1.62007, r - theta = 2.7791e-10
r = 1.62064, theta = 1.62064, r - theta = 1.82683e-10
r = 1.62122, theta = 1.62122, r - theta = -8.32405e-11
r = 1.62179, theta = 1.62179, r - theta = -3.10091e-10
r = 1.62237, theta = 1.62237, r - theta = -4.16588e-10
r = 1.62294, theta = 1.62294, r - theta = -4.18618e-10
r = 1.62351, theta = 1.62351, r - theta = -3.36742e-10
r = 1.62409, theta = 1.62409, r - theta = -1.96185e-10
r = 1.62466, theta = 1.62466, r - theta = -2.68379e-11
r = 1.62524, theta = 1.62524, r - theta = 1.36754e-10
r = 1.62581, theta = 1.62581, r - theta = 2.55387e-10
r = 1.62638, theta = 1.62638, r - theta = 2.85214e-10
r = 1.62696, theta = 1.62696, r - theta = 1.77737e-10
r = 1.62753, theta = 1.62753, r - theta = -1.00845e-10
r = 1.6281, theta = 1.6281, r - theta = -3.25957e-10
r = 1.62868, theta = 1.62868, r - theta = -4.28098e-10
r = 1.62925, theta = 1.62925, r - theta = -4.2422e-10
r = 1.62982, theta = 1.62982, r - theta = -3.35912e-10
r = 1.6304, theta = 1.6304, r - theta = -1.89396e-10
r = 1.63097, theta = 1.63097, r - theta = -1.55203e-11
r = 1.63154, theta = 1.63154, r - theta = 1.50239e-10
r = 1.63212, theta = 1.63212, r - theta = 2.6779e-10
r = 1.63269, theta = 1.63269, r - theta = 2.92421e-10
r = 1.63326, theta = 1.63326, r - theta = 1.74815e-10
r = 1.63383, theta = 1.63383, r - theta = -1.14292e-10
r = 1.63441, theta = 1.63441, r - theta = -3.39306e-10
r = 1.63498, theta = 1.63498, r - theta = -4.38635e-10
r = 1.63555, theta = 1.63555, r - theta = -4.30207e-10
r = 1.63612, theta = 1.63612, r - theta = -3.36551e-10
r = 1.6367, theta = 1.6367, r - theta = -1.84799e-10
r = 1.63727, theta = 1.63727, r - theta = -6.67311e-12
r = 1.63784, theta = 1.63784, r - theta = 1.6151e-10
r = 1.63841, theta = 1.63841, r - theta = 2.7885e-10
r = 1.63899, theta = 1.63899, r - theta = 2.99865e-10
r = 1.63956, theta = 1.63956, r - theta = 1.74495e-10
r = 1.64013, theta = 1.64013, r - theta = -1.23485e-10
r = 1.6407, theta = 1.6407, r - theta = -3.50199e-10
r = 1.64127, theta = 1.64127, r - theta = -4.48369e-10
r = 1.64184, theta = 1.64184, r - theta = -4.36811e-10
r = 1.64242, theta = 1.64242, r - theta = -3.38911e-10
r = 1.64299, theta = 1.64299, r - theta = -1.82625e-10
r = 1.64356, theta = 1.64356, r - theta = -4.62297e-13
r = 1.64413, theta = 1.64413, r - theta = 1.70502e-10
r = 1.6447, theta = 1.6447, r - theta = 2.88645e-10
r = 1.64527, theta = 1.64527, r - theta = 3.07795e-10
r = 1.64585, theta = 1.64585, r - theta = 1.77235e-10
r = 1.64642, theta = 1.64642, r - theta = -1.28257e-10
r = 1.64699, theta = 1.64699, r - theta = -3.58626e-10
r = 1.64756, theta = 1.64756, r - theta = -4.57402e-10
r = 1.64813, theta = 1.64813, r - theta = -4.44208e-10
r = 1.6487, theta = 1.6487, r - theta = -3.43202e-10
r = 1.64927, theta = 1.64927, r - theta = -1.83076e-10
r = 1.64984, theta = 1.64984, r - theta = 2.94875e-12
r = 1.65041, theta = 1.65041, r - theta = 1.77127e-10
r = 1.65098, theta = 1.65098, r - theta = 2.97194e-10
r = 1.65155, theta = 1.65155, r - theta = 3.16369e-10
r = 1.65212, theta = 1.65212, r - theta = 1.8336e-10
r = 1.65269, theta = 1.65269, r - theta = -1.28354e-10
r = 1.65326, theta = 1.65326, r - theta = -3.64493e-10
r = 1.65383, theta = 1.65383, r - theta = -4.65764e-10
r = 1.6544, theta = 1.6544, r - theta = -4.52512e-10
r = 1.65497, theta = 1.65497, r - theta = -3.49586e-10
r = 1.65554, theta = 1.65554, r - theta = -1.86336e-10
r = 1.65611, theta = 1.65611, r - theta = 3.3924e-12
r = 1.65668, theta = 1.65668, r - theta = 1.81262e-10
r = 1.65725, theta = 1.65725, r - theta = 3.04446e-10
r = 1.65782, theta = 1.65782, r - theta = 3.25636e-10
r = 1.65839, theta = 1.65839, r - theta = 1.93041e-10
r = 1.65896, theta = 1.65896, r - theta = -1.23427e-10
r = 1.65953, theta = 1.65953, r - theta = -3.67617e-10
r = 1.6601, theta = 1.6601, r - theta = -4.73405e-10
r = 1.66067, theta = 1.66067, r - theta = -4.61775e-10
r = 1.66124, theta = 1.66124, r - theta = -3.58185e-10
r = 1.66181, theta = 1.66181, r - theta = -1.92564e-10
r = 1.66238, theta = 1.66238, r - theta = 6.97886e-13
r = 1.66295, theta = 1.66295, r - theta = 1.82748e-10
r = 1.66352, theta = 1.66352, r - theta = 3.10278e-10
r = 1.66409, theta = 1.66409, r - theta = 3.3553e-10
r = 1.66466, theta = 1.66466, r - theta = 2.06295e-10
r = 1.66522, theta = 1.66522, r - theta = -1.13011e-10
r = 1.66579, theta = 1.66579, r - theta = -3.67711e-10
r = 1.66636, theta = 1.66636, r - theta = -4.80181e-10
r = 1.66693, theta = 1.66693, r - theta = -4.71968e-10
r = 1.6675, theta = 1.6675, r - theta = -3.69062e-10
r = 1.66807, theta = 1.66807, r - theta = -2.0189e-10
r = 1.66864, theta = 1.66864, r - theta = -5.31131e-12
r = 1.6692, theta = 1.6692, r - theta = 1.81387e-10
r = 1.66977, theta = 1.66977, r - theta = 3.14491e-10
r = 1.67034, theta = 1.67034, r - theta = 3.45869e-10
r = 1.67091, theta = 1.67091, r - theta = 2.2297e-10
r = 1.67148, theta = 1.67148, r - theta = -9.65164e-11
r = 1.67204, theta = 1.67204, r - theta = -3.6437e-10
r = 1.67261, theta = 1.67261, r - theta = -4.85849e-10
r = 1.67318, theta = 1.67318, r - theta = -4.82987e-10
r = 1.67375, theta = 1.67375, r - theta = -3.82228e-10
r = 1.67431, theta = 1.67431, r - theta = -2.1442e-10
r = 1.67488, theta = 1.67488, r - theta = -1.48141e-11
r = 1.67545, theta = 1.67545, r - theta = 1.76942e-10
r = 1.67602, theta = 1.67602, r - theta = 3.16807e-10
r = 1.67658, theta = 1.67658, r - theta = 3.56347e-10
r = 1.67715, theta = 1.67715, r - theta = 2.42746e-10
r = 1.67772, theta = 1.67772, r - theta = -7.32101e-11
r = 1.67829, theta = 1.67829, r - theta = -3.57059e-10
r = 1.67885, theta = 1.67885, r - theta = -4.90055e-10
r = 1.67942, theta = 1.67942, r - theta = -4.94637e-10
r = 1.67999, theta = 1.67999, r - theta = -3.97627e-10
r = 1.68055, theta = 1.68055, r - theta = -2.30222e-10
r = 1.68112, theta = 1.68112, r - theta = -2.79858e-11
r = 1.68169, theta = 1.68169, r - theta = 1.69144e-10
r = 1.68225, theta = 1.68225, r - theta = 3.16872e-10
r = 1.68282, theta = 1.68282, r - theta = 3.66541e-10
r = 1.68339, theta = 1.68339, r - theta = 2.65139e-10
r = 1.68395, theta = 1.68395, r - theta = -4.22009e-11
r = 1.68452, theta = 1.68452, r - theta = -3.45106e-10
r = 1.68509, theta = 1.68509, r - theta = -4.92321e-10
r = 1.68565, theta = 1.68565, r - theta = -5.0662e-10
r = 1.68622, theta = 1.68622, r - theta = -4.15128e-10
r = 1.68679, theta = 1.68679, r - theta = -2.49315e-10
r = 1.68735, theta = 1.68735, r - theta = -4.49911e-11
r = 1.68792, theta = 1.68792, r - theta = 1.57697e-10
r = 1.68848, theta = 1.68848, r - theta = 3.14269e-10
r = 1.68905, theta = 1.68905, r - theta = 3.75915e-10
r = 1.68961, theta = 1.68961, r - theta = 2.895e-10
r = 1.69018, theta = 1.69018, r - theta = -2.42495e-12
r = 1.69075, theta = 1.69075, r - theta = -3.27675e-10
r = 1.69131, theta = 1.69131, r - theta = -4.9203e-10
r = 1.69188, theta = 1.69188, r - theta = -5.18524e-10
r = 1.69244, theta = 1.69244, r - theta = -4.34514e-10
r = 1.69301, theta = 1.69301, r - theta = -2.71669e-10
r = 1.69357, theta = 1.69357, r - theta = -6.59728e-11
r = 1.69414, theta = 1.69414, r - theta = 1.42287e-10
r = 1.6947, theta = 1.6947, r - theta = 3.08518e-10
r = 1.69527, theta = 1.69527, r - theta = 3.83828e-10
r = 1.69583, theta = 1.69583, r - theta = 3.1503e-10
r = 1.6964, theta = 1.6964, r - theta = 4.46441e-11
r = 1.69696, theta = 1.69696, r - theta = -3.03756e-10
r = 1.69753, theta = 1.69753, r - theta = -4.88407e-10
r = 1.69809, theta = 1.69809, r - theta = -5.29803e-10
r = 1.69866, theta = 1.69866, r - theta = -4.55459e-10
r = 1.69922, theta = 1.69922, r - theta = -2.97175e-10
r = 1.69979, theta = 1.69979, r - theta = -9.10338e-11
r = 1.70035, theta = 1.70035, r - theta = 1.22603e-10
r = 1.70091, theta = 1.70091, r - theta = 2.99102e-10
r = 1.70148, theta = 1.70148, r - theta = 3.89557e-10
r = 1.70204, theta = 1.70204, r - theta = 3.40797e-10
r = 1.70261, theta = 1.70261, r - theta = 9.53881e-11
r = 1.70317, theta = 1.70317, r - theta = -2.7214e-10
r = 1.70374, theta = 1.70374, r - theta = -4.80507e-10
r = 1.7043, theta = 1.7043, r - theta = -5.39766e-10
r = 1.70486, theta = 1.70486, r - theta = -4.77521e-10
r = 1.70543, theta = 1.70543, r - theta = -3.25636e-10
r = 1.70599, theta = 1.70599, r - theta = -1.20225e-10
r = 1.70655, theta = 1.70655, r - theta = 9.83478e-11
r = 1.70712, theta = 1.70712, r - theta = 2.85474e-10
r = 1.70768, theta = 1.70768, r - theta = 3.92302e-10
r = 1.70824, theta = 1.70824, r - theta = 3.65746e-10
r = 1.70881, theta = 1.70881, r - theta = 1.48482e-10
r = 1.70937, theta = 1.70937, r - theta = -2.31409e-10
r = 1.70993, theta = 1.70993, r - theta = -4.67189e-10
r = 1.7105, theta = 1.7105, r - theta = -5.47549e-10
r = 1.71106, theta = 1.71106, r - theta = -5.00118e-10
r = 1.71162, theta = 1.71162, r - theta = -3.56749e-10
r = 1.71219, theta = 1.71219, r - theta = -1.53525e-10
r = 1.71275, theta = 1.71275, r - theta = 6.92579e-11
r = 1.71331, theta = 1.71331, r - theta = 2.67082e-10
r = 1.71388, theta = 1.71388, r - theta = 3.91216e-10
r = 1.71444, theta = 1.71444, r - theta = 3.88725e-10
r = 1.715, theta = 1.715, r - theta = 2.02463e-10
r = 1.71556, theta = 1.71556, r - theta = -1.79903e-10
r = 1.71613, theta = 1.71613, r - theta = -4.47096e-10
r = 1.71669, theta = 1.71669, r - theta = -5.52102e-10
r = 1.71725, theta = 1.71725, r - theta = -5.22502e-10
r = 1.71781, theta = 1.71781, r - theta = -3.90079e-10
r = 1.71837, theta = 1.71837, r - theta = -1.90811e-10
r = 1.71894, theta = 1.71894, r - theta = 3.51335e-11
r = 1.7195, theta = 1.7195, r - theta = 2.43394e-10
r = 1.72006, theta = 1.72006, r - theta = 3.85429e-10
r = 1.72062, theta = 1.72062, r - theta = 4.08513e-10
r = 1.72118, theta = 1.72118, r - theta = 2.55746e-10
r = 1.72175, theta = 1.72175, r - theta = -1.15704e-10
r = 1.72231, theta = 1.72231, r - theta = -4.18634e-10
r = 1.72287, theta = 1.72287, r - theta = -5.52156e-10
r = 1.72343, theta = 1.72343, r - theta = -5.43741e-10
r = 1.72399, theta = 1.72399, r - theta = -4.25031e-10
r = 1.72455, theta = 1.72455, r - theta = -2.3184e-10
r = 1.72512, theta = 1.72512, r - theta = -4.1418e-12
r = 1.72568, theta = 1.72568, r - theta = 2.13927e-10
r = 1.72624, theta = 1.72624, r - theta = 3.74073e-10
r = 1.7268, theta = 1.7268, r - theta = 4.23849e-10
r = 1.72736, theta = 1.72736, r - theta = 3.06658e-10
r = 1.72792, theta = 1.72792, r - theta = -3.66056e-11
r = 1.72848, theta = 1.72848, r - theta = -3.79941e-10
r = 1.72904, theta = 1.72904, r - theta = -5.46202e-10
r = 1.7296, theta = 1.7296, r - theta = -5.62683e-10
r = 1.73017, theta = 1.73017, r - theta = -4.60828e-10
r = 1.73073, theta = 1.73073, r - theta = -2.76216e-10
r = 1.73129, theta = 1.73129, r - theta = -4.85654e-11
r = 1.73185, theta = 1.73185, r - theta = 1.78272e-10
r = 1.73241, theta = 1.73241, r - theta = 3.56316e-10
r = 1.73297, theta = 1.73297, r - theta = 4.33457e-10
r = 1.73353, theta = 1.73353, r - theta = 3.5347e-10
r = 1.73409, theta = 1.73409, r - theta = 5.60023e-11
r = 1.73465, theta = 1.73465, r - theta = -3.28866e-10
r = 1.73521, theta = 1.73521, r - theta = -5.32463e-10
r = 1.73577, theta = 1.73577, r - theta = -5.77941e-10
r = 1.73633, theta = 1.73633, r - theta = -4.96479e-10
r = 1.73689, theta = 1.73689, r - theta = -3.23362e-10
r = 1.73745, theta = 1.73745, r - theta = -9.79901e-11
r = 1.73801, theta = 1.73801, r - theta = 1.36132e-10
r = 1.73857, theta = 1.73857, r - theta = 3.31397e-10
r = 1.73913, theta = 1.73913, r - theta = 4.36098e-10
r = 1.73969, theta = 1.73969, r - theta = 3.94433e-10
r = 1.74025, theta = 1.74025, r - theta = 1.46509e-10
r = 1.74081, theta = 1.74081, r - theta = -2.62932e-10
r = 1.74137, theta = 1.74137, r - theta = -5.0886e-10
r = 1.74193, theta = 1.74193, r - theta = -5.87848e-10
r = 1.74248, theta = 1.74248, r - theta = -5.30742e-10
r = 1.74304, theta = 1.74304, r - theta = -3.72481e-10
r = 1.7436, theta = 1.7436, r - theta = -1.5208e-10
r = 1.74416, theta = 1.74416, r - theta = 8.73601e-11
r = 1.74472, theta = 1.74472, r - theta = 2.98667e-10
r = 1.74528, theta = 1.74528, r - theta = 4.30595e-10
r = 1.74584, theta = 1.74584, r - theta = 4.2783e-10
r = 1.7464, theta = 1.7464, r - theta = 2.30988e-10
r = 1.74696, theta = 1.74696, r - theta = -1.79313e-10
r = 1.74752, theta = 1.74752, r - theta = -4.72985e-10
r = 1.74807, theta = 1.74807, r - theta = -5.90425e-10
r = 1.74863, theta = 1.74863, r - theta = -5.62098e-10
r = 1.74919, theta = 1.74919, r - theta = -4.22524e-10
r = 1.74975, theta = 1.74975, r - theta = -2.10282e-10
r = 1.75031, theta = 1.75031, r - theta = 3.19949e-11
r = 1.75087, theta = 1.75087, r - theta = 2.57628e-10
r = 1.75142, theta = 1.75142, r - theta = 4.15889e-10
r = 1.75198, theta = 1.75198, r - theta = 4.52007e-10
r = 1.75254, theta = 1.75254, r - theta = 3.07174e-10
r = 1.7531, theta = 1.7531, r - theta = -7.47979e-11
r = 1.75366, theta = 1.75366, r - theta = -4.22062e-10
r = 1.75421, theta = 1.75421, r - theta = -5.83359e-10
r = 1.75477, theta = 1.75477, r - theta = -5.88706e-10
r = 1.75533, theta = 1.75533, r - theta = -4.72153e-10
r = 1.75589, theta = 1.75589, r - theta = -2.71779e-10
r = 1.75644, theta = 1.75644, r - theta = -2.96931e-11
r = 1.757, theta = 1.757, r - theta = 2.07974e-10
r = 1.75756, theta = 1.75756, r - theta = 3.91073e-10
r = 1.75812, theta = 1.75812, r - theta = 4.65433e-10
r = 1.75867, theta = 1.75867, r - theta = 3.72874e-10
r = 1.75923, theta = 1.75923, r - theta = 5.12059e-11
r = 1.75979, theta = 1.75979, r - theta = -3.52919e-10
r = 1.76035, theta = 1.76035, r - theta = -5.63953e-10
r = 1.7609, theta = 1.7609, r - theta = -6.08371e-10
r = 1.76146, theta = 1.76146, r - theta = -5.19695e-10
r = 1.76202, theta = 1.76202, r - theta = -3.3545e-10
r = 1.76257, theta = 1.76257, r - theta = -9.71632e-11
r = 1.76313, theta = 1.76313, r - theta = 1.49643e-10
r = 1.76369, theta = 1.76369, r - theta = 3.55449e-10
r = 1.76424, theta = 1.76424, r - theta = 4.66745e-10
r = 1.7648, theta = 1.7648, r - theta = 4.26033e-10
r = 1.76536, theta = 1.76536, r - theta = 1.7183e-10
r = 1.76591, theta = 1.76591, r - theta = -2.61949e-10
r = 1.76647, theta = 1.76647, r - theta = -5.29092e-10
r = 1.76703, theta = 1.76703, r - theta = -6.18501e-10
r = 1.76758, theta = 1.76758, r - theta = -5.63109e-10
r = 1.76814, theta = 1.76814, r - theta = -3.9983e-10
r = 1.76869, theta = 1.76869, r - theta = -1.69553e-10
r = 1.76925, theta = 1.76925, r - theta = 8.28602e-11
r = 1.76981, theta = 1.76981, r - theta = 3.08576e-10
r = 1.77036, theta = 1.77036, r - theta = 4.54801e-10
r = 1.77092, theta = 1.77092, r - theta = 4.64772e-10
r = 1.77147, theta = 1.77147, r - theta = 2.77771e-10
r = 1.77203, theta = 1.77203, r - theta = -1.4507e-10
r = 1.77259, theta = 1.77259, r - theta = -4.75209e-10
r = 1.77314, theta = 1.77314, r - theta = -6.16065e-10
r = 1.7737, theta = 1.7737, r - theta = -5.9993e-10
r = 1.77425, theta = 1.77425, r - theta = -4.63053e-10
r = 1.77481, theta = 1.77481, r - theta = -2.4563e-10
r = 1.77536, theta = 1.77536, r - theta = 8.19322e-12
r = 1.77592, theta = 1.77592, r - theta = 2.50326e-10
r = 1.77647, theta = 1.77647, r - theta = 4.2874e-10
r = 1.77703, theta = 1.77703, r - theta = 4.87464e-10
r = 1.77758, theta = 1.77758, r - theta = 3.66598e-10
r = 1.77814, theta = 1.77814, r - theta = 2.30527e-12
r = 1.77869, theta = 1.77869, r - theta = -3.98236e-10
r = 1.77925, theta = 1.77925, r - theta = -5.97554e-10
r = 1.7798, theta = 1.7798, r - theta = -6.2724e-10
r = 1.78036, theta = 1.78036, r - theta = -5.2282e-10
r = 1.78091, theta = 1.78091, r - theta = -3.23749e-10
r = 1.78147, theta = 1.78147, r - theta = -7.33997e-11
r = 1.78202, theta = 1.78202, r - theta = 1.80931e-10
r = 1.78258, theta = 1.78258, r - theta = 3.88033e-10
r = 1.78313, theta = 1.78313, r - theta = 4.92781e-10
r = 1.78369, theta = 1.78369, r - theta = 4.36141e-10
r = 1.78424, theta = 1.78424, r - theta = 1.55172e-10
r = 1.78479, theta = 1.78479, r - theta = -2.93575e-10
r = 1.78535, theta = 1.78535, r - theta = -5.58938e-10
r = 1.7859, theta = 1.7859, r - theta = -6.41609e-10
r = 1.78646, theta = 1.78646, r - theta = -5.76343e-10
r = 1.78701, theta = 1.78701, r - theta = -4.01793e-10
r = 1.78756, theta = 1.78756, r - theta = -1.60513e-10
r = 1.78812, theta = 1.78812, r - theta = 1.01048e-10
r = 1.78867, theta = 1.78867, r - theta = 3.3255e-10
r = 1.78923, theta = 1.78923, r - theta = 4.79765e-10
r = 1.78978, theta = 1.78978, r - theta = 4.84576e-10
r = 1.79033, theta = 1.79033, r - theta = 2.84987e-10
r = 1.79089, theta = 1.79089, r - theta = -1.56045e-10
r = 1.79144, theta = 1.79144, r - theta = -4.95618e-10
r = 1.79199, theta = 1.79199, r - theta = -6.39058e-10
r = 1.79255, theta = 1.79255, r - theta = -6.20292e-10
r = 1.7931, theta = 1.7931, r - theta = -4.77127e-10
r = 1.79365, theta = 1.79365, r - theta = -2.51243e-10
r = 1.79421, theta = 1.79421, r - theta = 1.18101e-11
r = 1.79476, theta = 1.79476, r - theta = 2.62614e-10
r = 1.79531, theta = 1.79531, r - theta = 4.47884e-10
r = 1.79586, theta = 1.79586, r - theta = 5.10476e-10
r = 1.79642, theta = 1.79642, r - theta = 3.89384e-10
r = 1.79697, theta = 1.79697, r - theta = 1.97513e-11
r = 1.79752, theta = 1.79752, r - theta = -4.02387e-10
r = 1.79808, theta = 1.79808, r - theta = -6.15005e-10
r = 1.79863, theta = 1.79863, r - theta = -6.50755e-10
r = 1.79918, theta = 1.79918, r - theta = -5.46544e-10
r = 1.79973, theta = 1.79973, r - theta = -3.43128e-10
r = 1.80029, theta = 1.80029, r - theta = -8.51086e-11
r = 1.80084, theta = 1.80084, r - theta = 1.79066e-10
r = 1.80139, theta = 1.80139, r - theta = 3.97106e-10
r = 1.80194, theta = 1.80194, r - theta = 5.12887e-10
r = 1.80249, theta = 1.80249, r - theta = 4.66445e-10
r = 1.80305, theta = 1.80305, r - theta = 1.93989e-10
r = 1.8036, theta = 1.8036, r - theta = -2.73385e-10
r = 1.80415, theta = 1.80415, r - theta = -5.64225e-10
r = 1.8047, theta = 1.8047, r - theta = -6.63182e-10
r = 1.80525, theta = 1.80525, r - theta = -6.0621e-10
r = 1.80581, theta = 1.80581, r - theta = -4.33095e-10
r = 1.80636, theta = 1.80636, r - theta = -1.8744e-10
r = 1.80691, theta = 1.80691, r - theta = 8.33249e-11
r = 1.80746, theta = 1.80746, r - theta = 3.27956e-10
r = 1.80801, theta = 1.80801, r - theta = 4.91394e-10
r = 1.80856, theta = 1.80856, r - theta = 5.14767e-10
r = 1.80911, theta = 1.80911, r - theta = 3.35394e-10
r = 1.80967, theta = 1.80967, r - theta = -1.02052e-10
r = 1.81022, theta = 1.81022, r - theta = -4.808e-10
r = 1.81077, theta = 1.81077, r - theta = -6.52336e-10
r = 1.81132, theta = 1.81132, r - theta = -6.51617e-10
r = 1.81187, theta = 1.81187, r - theta = -5.17405e-10
r = 1.81242, theta = 1.81242, r - theta = -2.92261e-10
r = 1.81297, theta = 1.81297, r - theta = -2.25453e-11
r = 1.81352, theta = 1.81352, r - theta = 2.41588e-10
r = 1.81407, theta = 1.81407, r - theta = 4.46193e-10
r = 1.81463, theta = 1.81463, r - theta = 5.33536e-10
r = 1.81518, theta = 1.81518, r - theta = 4.42098e-10
r = 1.81573, theta = 1.81573, r - theta = 1.06576e-10
r = 1.81628, theta = 1.81628, r - theta = -3.58075e-10
r = 1.81683, theta = 1.81683, r - theta = -6.12247e-10
r = 1.81738, theta = 1.81738, r - theta = -6.77523e-10
r = 1.81793, theta = 1.81793, r - theta = -5.91595e-10
r = 1.81848, theta = 1.81848, r - theta = -3.95933e-10
r = 1.81903, theta = 1.81903, r - theta = -1.35782e-10
r = 1.81958, theta = 1.81958, r - theta = 1.39843e-10
r = 1.82013, theta = 1.82013, r - theta = 3.78156e-10
r = 1.82068, theta = 1.82068, r - theta = 5.22605e-10
r = 1.82123, theta = 1.82123, r - theta = 5.12877e-10
r = 1.82178, theta = 1.82178, r - theta = 2.84899e-10
r = 1.82233, theta = 1.82233, r - theta = -1.88608e-10
r = 1.82288, theta = 1.82288, r - theta = -5.36161e-10
r = 1.82343, theta = 1.82343, r - theta = -6.77908e-10
r = 1.82398, theta = 1.82398, r - theta = -6.50425e-10
r = 1.82453, theta = 1.82453, r - theta = -4.94044e-10
r = 1.82508, theta = 1.82508, r - theta = -2.52849e-10
r = 1.82563, theta = 1.82563, r - theta = 2.53271e-11
r = 1.82618, theta = 1.82618, r - theta = 2.88905e-10
r = 1.82672, theta = 1.82672, r - theta = 4.82562e-10
r = 1.82727, theta = 1.82727, r - theta = 5.47235e-10
r = 1.82782, theta = 1.82782, r - theta = 4.20124e-10
r = 1.82837, theta = 1.82837, r - theta = 3.46951e-11
r = 1.82892, theta = 1.82892, r - theta = -4.16492e-10
r = 1.82947, theta = 1.82947, r - theta = -6.45915e-10
r = 1.83002, theta = 1.83002, r - theta = -6.87818e-10
r = 1.83057, theta = 1.83057, r - theta = -5.81348e-10
r = 1.83112, theta = 1.83112, r - theta = -3.69385e-10
r = 1.83167, theta = 1.83167, r - theta = -9.85316e-11
r = 1.83221, theta = 1.83221, r - theta = 1.80881e-10
r = 1.83276, theta = 1.83276, r - theta = 4.14806e-10
r = 1.83331, theta = 1.83331, r - theta = 5.45473e-10
r = 1.83386, theta = 1.83386, r - theta = 5.11401e-10
r = 1.83441, theta = 1.83441, r - theta = 2.47392e-10
r = 1.83496, theta = 1.83496, r - theta = -2.44779e-10
r = 1.8355, theta = 1.8355, r - theta = -5.73811e-10
r = 1.83605, theta = 1.83605, r - theta = -6.96817e-10
r = 1.8366, theta = 1.8366, r - theta = -6.51717e-10
r = 1.83715, theta = 1.83715, r - theta = -4.80141e-10
r = 1.8377, theta = 1.8377, r - theta = -2.27421e-10
r = 1.83825, theta = 1.83825, r - theta = 5.74087e-11
r = 1.83879, theta = 1.83879, r - theta = 3.21616e-10
r = 1.83934, theta = 1.83934, r - theta = 5.0877e-10
r = 1.83989, theta = 1.83989, r - theta = 5.58748e-10
r = 1.84044, theta = 1.84044, r - theta = 4.07736e-10
r = 1.84098, theta = 1.84098, r - theta = -1.16414e-11
r = 1.84153, theta = 1.84153, r - theta = -4.52928e-10
r = 1.84208, theta = 1.84208, r - theta = -6.69524e-10
r = 1.84263, theta = 1.84263, r - theta = -6.98077e-10
r = 1.84317, theta = 1.84317, r - theta = -5.78922e-10
r = 1.84372, theta = 1.84372, r - theta = -3.56076e-10
r = 1.84427, theta = 1.84427, r - theta = -7.72353e-11
r = 1.84482, theta = 1.84482, r - theta = 2.06225e-10
r = 1.84536, theta = 1.84536, r - theta = 4.39255e-10
r = 1.84591, theta = 1.84591, r - theta = 5.63134e-10
r = 1.84646, theta = 1.84646, r - theta = 5.15474e-10
r = 1.847, theta = 1.847, r - theta = 2.30219e-10
r = 1.84755, theta = 1.84755, r - theta = -2.73626e-10
r = 1.8481, theta = 1.8481, r - theta = -5.97056e-10
r = 1.84865, theta = 1.84865, r - theta = -7.12356e-10
r = 1.84919, theta = 1.84919, r - theta = -6.58523e-10
r = 1.84974, theta = 1.84974, r - theta = -4.78218e-10
r = 1.85029, theta = 1.85029, r - theta = -2.17758e-10
r = 1.85083, theta = 1.85083, r - theta = 7.28839e-11
r = 1.85138, theta = 1.85138, r - theta = 3.4008e-10
r = 1.85192, theta = 1.85192, r - theta = 5.26554e-10
r = 1.85247, theta = 1.85247, r - theta = 5.7138e-10
r = 1.85302, theta = 1.85302, r - theta = 4.09989e-10
r = 1.85356, theta = 1.85356, r - theta = -2.52454e-11
r = 1.85411, theta = 1.85411, r - theta = -4.69497e-10
r = 1.85466, theta = 1.85466, r - theta = -6.85437e-10
r = 1.8552, theta = 1.8552, r - theta = -7.10686e-10
r = 1.85575, theta = 1.85575, r - theta = -5.86503e-10
r = 1.85629, theta = 1.85629, r - theta = -3.57784e-10
r = 1.85684, theta = 1.85684, r - theta = -7.30638e-11
r = 1.85739, theta = 1.85739, r - theta = 2.15496e-10
r = 1.85793, theta = 1.85793, r - theta = 4.52101e-10
r = 1.85848, theta = 1.85848, r - theta = 5.77333e-10
r = 1.85902, theta = 1.85902, r - theta = 5.28148e-10
r = 1.85957, theta = 1.85957, r - theta = 2.37882e-10
r = 1.86011, theta = 1.86011, r - theta = -2.75852e-10
r = 1.86066, theta = 1.86066, r - theta = -6.07108e-10
r = 1.8612, theta = 1.8612, r - theta = -7.26035e-10
r = 1.86175, theta = 1.86175, r - theta = -6.72454e-10
r = 1.86229, theta = 1.86229, r - theta = -4.898e-10
r = 1.86284, theta = 1.86284, r - theta = -2.25124e-10
r = 1.86339, theta = 1.86339, r - theta = 7.09144e-11
r = 1.86393, theta = 1.86393, r - theta = 3.44045e-10
r = 1.86448, theta = 1.86448, r - theta = 5.36394e-10
r = 1.86502, theta = 1.86502, r - theta = 5.86483e-10
r = 1.86556, theta = 1.86556, r - theta = 4.29237e-10
r = 1.86611, theta = 1.86611, r - theta = -4.00657e-12
r = 1.86665, theta = 1.86665, r - theta = -4.66013e-10
r = 1.8672, theta = 1.8672, r - theta = -6.94036e-10
r = 1.86774, theta = 1.86774, r - theta = -7.26418e-10
r = 1.86829, theta = 1.86829, r - theta = -6.05092e-10
r = 1.86883, theta = 1.86883, r - theta = -3.75587e-10
r = 1.86938, theta = 1.86938, r - theta = -8.70213e-11
r = 1.86992, theta = 1.86992, r - theta = 2.07899e-10
r = 1.87047, theta = 1.87047, r - theta = 4.52883e-10
r = 1.87101, theta = 1.87101, r - theta = 5.88057e-10
r = 1.87155, theta = 1.87155, r - theta = 5.49971e-10
r = 1.8721, theta = 1.8721, r - theta = 2.71592e-10
r = 1.87264, theta = 1.87264, r - theta = -2.49615e-10
r = 1.87319, theta = 1.87319, r - theta = -6.02949e-10
r = 1.87373, theta = 1.87373, r - theta = -7.3751e-10
r = 1.87427, theta = 1.87427, r - theta = -6.93695e-10
r = 1.87482, theta = 1.87482, r - theta = -5.15471e-10
r = 1.87536, theta = 1.87536, r - theta = -2.50375e-10
r = 1.87591, theta = 1.87591, r - theta = 5.04872e-11
r = 1.87645, theta = 1.87645, r - theta = 3.32447e-10
r = 1.87699, theta = 1.87699, r - theta = 5.37274e-10
r = 1.87754, theta = 1.87754, r - theta = 6.03177e-10
r = 1.87808, theta = 1.87808, r - theta = 4.64811e-10
r = 1.87862, theta = 1.87862, r - theta = 5.32741e-11
r = 1.87917, theta = 1.87917, r - theta = -4.3978e-10
r = 1.87971, theta = 1.87971, r - theta = -6.93569e-10
r = 1.88025, theta = 1.88025, r - theta = -7.4434e-10
r = 1.8808, theta = 1.8808, r - theta = -6.34464e-10
r = 1.88134, theta = 1.88134, r - theta = -4.09859e-10
r = 1.88188, theta = 1.88188, r - theta = -1.1999e-10
r = 1.88243, theta = 1.88243, r - theta = 1.82132e-10
r = 1.88297, theta = 1.88297, r - theta = 4.39957e-10
r = 1.88351, theta = 1.88351, r - theta = 5.93392e-10
r = 1.88405, theta = 1.88405, r - theta = 5.7881e-10
r = 1.8846, theta = 1.8846, r - theta = 3.29052e-10
r = 1.88514, theta = 1.88514, r - theta = -1.90253e-10
r = 1.88568, theta = 1.88568, r - theta = -5.81113e-10
r = 1.88622, theta = 1.88622, r - theta = -7.4441e-10
r = 1.88677, theta = 1.88677, r - theta = -7.20876e-10
r = 1.88731, theta = 1.88731, r - theta = -5.54775e-10
r = 1.88785, theta = 1.88785, r - theta = -2.93897e-10
r = 1.88839, theta = 1.88839, r - theta = 1.0445e-11
r = 1.88894, theta = 1.88894, r - theta = 3.03415e-10
r = 1.88948, theta = 1.88948, r - theta = 5.26657e-10
r = 1.89002, theta = 1.89002, r - theta = 6.18299e-10
r = 1.89056, theta = 1.89056, r - theta = 5.12957e-10
r = 1.8911, theta = 1.8911, r - theta = 1.41732e-10
r = 1.89165, theta = 1.89165, r - theta = -3.85304e-10
r = 1.89219, theta = 1.89219, r - theta = -6.79892e-10
r = 1.89273, theta = 1.89273, r - theta = -7.61591e-10
r = 1.89327, theta = 1.89327, r - theta = -6.7297e-10
r = 1.89381, theta = 1.89381, r - theta = -4.60108e-10
r = 1.89435, theta = 1.89435, r - theta = -1.72586e-10
r = 1.8949, theta = 1.8949, r - theta = 1.3651e-10
r = 1.89544, theta = 1.89544, r - theta = 4.10598e-10
r = 1.89598, theta = 1.89598, r - theta = 5.89594e-10
r = 1.89652, theta = 1.89652, r - theta = 6.09926e-10
r = 1.89706, theta = 1.89706, r - theta = 4.04525e-10
r = 1.8976, theta = 1.8976, r - theta = -8.99154e-11
r = 1.89814, theta = 1.89814, r - theta = -5.35349e-10
r = 1.89869, theta = 1.89869, r - theta = -7.42024e-10
r = 1.89923, theta = 1.89923, r - theta = -7.50782e-10
r = 1.89977, theta = 1.89977, r - theta = -6.05955e-10
r = 1.90031, theta = 1.90031, r - theta = -3.55356e-10
r = 1.90085, theta = 1.90085, r - theta = -5.02842e-11
r = 1.90139, theta = 1.90139, r - theta = 2.54484e-10
r = 1.90193, theta = 1.90193, r - theta = 5.00691e-10
r = 1.90247, theta = 1.90247, r - theta = 6.26607e-10
r = 1.90301, theta = 1.90301, r - theta = 5.6703e-10
r = 1.90355, theta = 1.90355, r - theta = 2.53286e-10
r = 1.90409, theta = 1.90409, r - theta = -2.93875e-10
r = 1.90463, theta = 1.90463, r - theta = -6.46084e-10
r = 1.90517, theta = 1.90517, r - theta = -7.73004e-10
r = 1.90571, theta = 1.90571, r - theta = -7.17182e-10
r = 1.90626, theta = 1.90626, r - theta = -5.24631e-10
r = 1.9068, theta = 1.9068, r - theta = -2.44828e-10
r = 1.90734, theta = 1.90734, r - theta = 6.9289e-11
r = 1.90788, theta = 1.90788, r - theta = 3.61325e-10
r = 1.90842, theta = 1.90842, r - theta = 5.71427e-10
r = 1.90896, theta = 1.90896, r - theta = 6.36289e-10
r = 1.9095, theta = 1.9095, r - theta = 4.89156e-10
r = 1.91004, theta = 1.91004, r - theta = 5.9823e-11
r = 1.91058, theta = 1.91058, r - theta = -4.56149e-10
r = 1.91112, theta = 1.91112, r - theta = -7.22847e-10
r = 1.91165, theta = 1.91165, r - theta = -7.77907e-10
r = 1.91219, theta = 1.91219, r - theta = -6.65508e-10
r = 1.91273, theta = 1.91273, r - theta = -4.33269e-10
r = 1.91327, theta = 1.91327, r - theta = -1.32255e-10
r = 1.91381, theta = 1.91381, r - theta = 1.83031e-10
r = 1.91435, theta = 1.91435, r - theta = 4.54649e-10
r = 1.91489, theta = 1.91489, r - theta = 6.21223e-10
r = 1.91543, theta = 1.91543, r - theta = 6.17948e-10
r = 1.91597, theta = 1.91597, r - theta = 3.76586e-10
r = 1.91651, theta = 1.91651, r - theta = -1.53032e-10
r = 1.91705, theta = 1.91705, r - theta = -5.81911e-10
r = 1.91759, theta = 1.91759, r - theta = -7.70587e-10
r = 1.91813, theta = 1.91813, r - theta = -7.61369e-10
r = 1.91867, theta = 1.91867, r - theta = -5.99991e-10
r = 1.9192, theta = 1.9192, r - theta = -3.35608e-10
r = 1.91974, theta = 1.91974, r - theta = -2.07991e-11
r = 1.92028, theta = 1.92028, r - theta = 2.88441e-10
r = 1.92082, theta = 1.92082, r - theta = 5.327e-10
r = 1.92136, theta = 1.92136, r - theta = 6.49153e-10
r = 1.9219, theta = 1.9219, r - theta = 5.71563e-10
r = 1.92244, theta = 1.92244, r - theta = 2.30288e-10
r = 1.92297, theta = 1.92297, r - theta = -3.30159e-10
r = 1.92351, theta = 1.92351, r - theta = -6.75987e-10
r = 1.92405, theta = 1.92405, r - theta = -7.93858e-10
r = 1.92459, theta = 1.92459, r - theta = -7.27585e-10
r = 1.92513, theta = 1.92513, r - theta = -5.24385e-10
r = 1.92567, theta = 1.92567, r - theta = -2.34877e-10
r = 1.9262, theta = 1.9262, r - theta = 8.69171e-11
r = 1.92674, theta = 1.92674, r - theta = 3.83583e-10
r = 1.92728, theta = 1.92728, r - theta = 5.94309e-10
r = 1.92782, theta = 1.92782, r - theta = 6.54889e-10
r = 1.92836, theta = 1.92836, r - theta = 4.97731e-10
r = 1.92889, theta = 1.92889, r - theta = 5.18507e-11
r = 1.92943, theta = 1.92943, r - theta = -4.73164e-10
r = 1.92997, theta = 1.92997, r - theta = -7.42846e-10
r = 1.93051, theta = 1.93051, r - theta = -7.96815e-10
r = 1.93104, theta = 1.93104, r - theta = -6.80314e-10
r = 1.93158, theta = 1.93158, r - theta = -4.41975e-10
r = 1.93212, theta = 1.93212, r - theta = -1.33809e-10
r = 1.93266, theta = 1.93266, r - theta = 1.88792e-10
r = 1.93319, theta = 1.93319, r - theta = 4.67063e-10
r = 1.93373, theta = 1.93373, r - theta = 6.38861e-10
r = 1.93427, theta = 1.93427, r - theta = 6.38671e-10
r = 1.93481, theta = 1.93481, r - theta = 3.97613e-10
r = 1.93534, theta = 1.93534, r - theta = -1.3945e-10
r = 1.93588, theta = 1.93588, r - theta = -5.86419e-10
r = 1.93642, theta = 1.93642, r - theta = -7.86592e-10
r = 1.93695, theta = 1.93695, r - theta = -7.83212e-10
r = 1.93749, theta = 1.93749, r - theta = -6.22889e-10
r = 1.93803, theta = 1.93803, r - theta = -3.55595e-10
r = 1.93856, theta = 1.93856, r - theta = -3.46649e-11
r = 1.9391, theta = 1.9391, r - theta = 2.83209e-10
r = 1.93964, theta = 1.93964, r - theta = 5.37979e-10
r = 1.94017, theta = 1.94017, r - theta = 6.66241e-10
r = 1.94071, theta = 1.94071, r - theta = 6.01241e-10
r = 1.94125, theta = 1.94125, r - theta = 2.72875e-10
r = 1.94178, theta = 1.94178, r - theta = -3.00863e-10
r = 1.94232, theta = 1.94232, r - theta = -6.73966e-10
r = 1.94286, theta = 1.94286, r - theta = -8.10966e-10
r = 1.94339, theta = 1.94339, r - theta = -7.5642e-10
r = 1.94393, theta = 1.94393, r - theta = -5.58233e-10
r = 1.94446, theta = 1.94446, r - theta = -2.67653e-10
r = 1.945, theta = 1.945, r - theta = 6.07356e-11
r = 1.94554, theta = 1.94554, r - theta = 3.69003e-10
r = 1.94607, theta = 1.94607, r - theta = 5.95889e-10
r = 1.94661, theta = 1.94661, r - theta = 6.76795e-10
r = 1.94714, theta = 1.94714, r - theta = 5.43795e-10
r = 1.94768, theta = 1.94768, r - theta = 1.25632e-10
r = 1.94821, theta = 1.94821, r - theta = -4.34111e-10
r = 1.94875, theta = 1.94875, r - theta = -7.39516e-10
r = 1.94929, theta = 1.94929, r - theta = -8.19355e-10
r = 1.94982, theta = 1.94982, r - theta = -7.19433e-10
r = 1.95036, theta = 1.95036, r - theta = -4.8888e-10
r = 1.95089, theta = 1.95089, r - theta = -1.8015e-10
r = 1.95143, theta = 1.95143, r - theta = 1.50981e-10
r = 1.95196, theta = 1.95196, r - theta = 4.45421e-10
r = 1.9525, theta = 1.9525, r - theta = 6.40759e-10
r = 1.95303, theta = 1.95303, r - theta = 6.71273e-10
r = 1.95357, theta = 1.95357, r - theta = 4.67929e-10
r = 1.9541, theta = 1.9541, r - theta = -4.03313e-11
r = 1.95464, theta = 1.95464, r - theta = -5.42856e-10
r = 1.95517, theta = 1.95517, r - theta = -7.86454e-10
r = 1.95571, theta = 1.95571, r - theta = -8.14802e-10
r = 1.95624, theta = 1.95624, r - theta = -6.74884e-10
r = 1.95678, theta = 1.95678, r - theta = -4.16992e-10
r = 1.95731, theta = 1.95731, r - theta = -9.47209e-11
r = 1.95785, theta = 1.95785, r - theta = 2.35035e-10
r = 1.95838, theta = 1.95838, r - theta = 5.12081e-10
r = 1.95892, theta = 1.95892, r - theta = 6.72925e-10
r = 1.95945, theta = 1.95945, r - theta = 6.50783e-10
r = 1.95999, theta = 1.95999, r - theta = 3.75578e-10
r = 1.96052, theta = 1.96052, r - theta = -1.93034e-10
r = 1.96105, theta = 1.96105, r - theta = -6.30455e-10
r = 1.96159, theta = 1.96159, r - theta = -8.17846e-10
r = 1.96212, theta = 1.96212, r - theta = -8.00018e-10
r = 1.96266, theta = 1.96266, r - theta = -6.25072e-10
r = 1.96319, theta = 1.96319, r - theta = -3.44393e-10
r = 1.96372, theta = 1.96372, r - theta = -1.26508e-11
r = 1.96426, theta = 1.96426, r - theta = 3.12202e-10
r = 1.96479, theta = 1.96479, r - theta = 5.68936e-10
r = 1.96533, theta = 1.96533, r - theta = 6.93042e-10
r = 1.96586, theta = 1.96586, r - theta = 6.16738e-10
r = 1.96639, theta = 1.96639, r - theta = 2.68967e-10
r = 1.96693, theta = 1.96693, r - theta = -3.22078e-10
r = 1.96746, theta = 1.96746, r - theta = -6.9997e-10
r = 1.96799, theta = 1.96799, r - theta = -8.36451e-10
r = 1.96853, theta = 1.96853, r - theta = -7.77404e-10
r = 1.96906, theta = 1.96906, r - theta = -5.71978e-10
r = 1.96959, theta = 1.96959, r - theta = -2.7259e-10
r = 1.97013, theta = 1.97013, r - theta = 6.50802e-11
r = 1.97066, theta = 1.97066, r - theta = 3.82086e-10
r = 1.97119, theta = 1.97119, r - theta = 6.16226e-10
r = 1.97173, theta = 1.97173, r - theta = 7.02035e-10
r = 1.97226, theta = 1.97226, r - theta = 5.70799e-10
r = 1.97279, theta = 1.97279, r - theta = 1.50544e-10
r = 1.97333, theta = 1.97333, r - theta = -4.30492e-10
r = 1.97386, theta = 1.97386, r - theta = -7.54178e-10
r = 1.97439, theta = 1.97439, r - theta = -8.44741e-10
r = 1.97493, theta = 1.97493, r - theta = -7.49068e-10
r = 1.97546, theta = 1.97546, r - theta = -5.17295e-10
r = 1.97599, theta = 1.97599, r - theta = -2.02806e-10
r = 1.97652, theta = 1.97652, r - theta = 1.37767e-10
r = 1.97706, theta = 1.97706, r - theta = 4.44551e-10
r = 1.97759, theta = 1.97759, r - theta = 6.54431e-10
r = 1.97812, theta = 1.97812, r - theta = 7.01055e-10
r = 1.97865, theta = 1.97865, r - theta = 5.1483e-10
r = 1.97919, theta = 1.97919, r - theta = 2.29332e-11
r = 1.97972, theta = 1.97972, r - theta = -5.21036e-10
r = 1.98025, theta = 1.98025, r - theta = -7.95584e-10
r = 1.98078, theta = 1.98078, r - theta = -8.44911e-10
r = 1.98131, theta = 1.98131, r - theta = -7.16845e-10
r = 1.98185, theta = 1.98185, r - theta = -4.62449e-10
r = 1.98238, theta = 1.98238, r - theta = -1.36013e-10
r = 1.98291, theta = 1.98291, r - theta = 2.04946e-10
r = 1.98344, theta = 1.98344, r - theta = 4.99685e-10
r = 1.98397, theta = 1.98397, r - theta = 6.8424e-10
r = 1.98451, theta = 1.98451, r - theta = 6.91427e-10
r = 1.98504, theta = 1.98504, r - theta = 4.50844e-10
r = 1.98557, theta = 1.98557, r - theta = -1.02734e-10
r = 1.9861, theta = 1.9861, r - theta = -5.96209e-10
r = 1.98663, theta = 1.98663, r - theta = -8.26431e-10
r = 1.98716, theta = 1.98716, r - theta = -8.38898e-10
r = 1.98769, theta = 1.98769, r - theta = -6.82322e-10
r = 1.98823, theta = 1.98823, r - theta = -4.08628e-10
r = 1.98876, theta = 1.98876, r - theta = -7.29534e-11
r = 1.98929, theta = 1.98929, r - theta = 2.6636e-10
r = 1.98982, theta = 1.98982, r - theta = 5.47761e-10
r = 1.99035, theta = 1.99035, r - theta = 7.06497e-10
r = 1.99088, theta = 1.99088, r - theta = 6.74613e-10
r = 1.99141, theta = 1.99141, r - theta = 3.80952e-10
r = 1.99194, theta = 1.99194, r - theta = -2.10699e-10
r = 1.99248, theta = 1.99248, r - theta = -6.5826e-10
r = 1.99301, theta = 1.99301, r - theta = -8.48716e-10
r = 1.99354, theta = 1.99354, r - theta = -8.28404e-10
r = 1.99407, theta = 1.99407, r - theta = -6.4686e-10
r = 1.9946, theta = 1.9946, r - theta = -3.56812e-10
r = 1.99513, theta = 1.99513, r - theta = -1.41822e-11
r = 1.99566, theta = 1.99566, r - theta = 3.2192e-10
r = 1.99619, theta = 1.99619, r - theta = 5.89197e-10
r = 1.99672, theta = 1.99672, r - theta = 7.22163e-10
r = 1.99725, theta = 1.99725, r - theta = 6.52153e-10
r = 1.99778, theta = 1.99778, r - theta = 3.07318e-10
r = 1.99831, theta = 1.99831, r - theta = -3.02778e-10
r = 1.99884, theta = 1.99884, r - theta = -7.09201e-10
r = 1.99937, theta = 1.99937, r - theta = -8.64206e-10
r = 1.9999, theta = 1.9999, r - theta = -8.14912e-10
r = 2.00043, theta = 2.00043, r - theta = -6.11617e-10
r = 2.00096, theta = 2.00096, r - theta = -3.07793e-10
r = 2.00149, theta = 2.00149, r - theta = 3.99121e-11
r = 2.00202, theta = 2.00202, r - theta = 3.7168e-10
r = 2.00255, theta = 2.00255, r - theta = 6.24521e-10
r = 2.00308, theta = 2.00308, r - theta = 7.32281e-10
r = 2.00361, theta = 2.00361, r - theta = 6.25638e-10
r = 2.00414, theta = 2.00414, r - theta = 2.32109e-10
r = 2.00467, theta = 2.00467, r - theta = -3.80945e-10
r = 2.0052, theta = 2.0052, r - theta = -7.50817e-10
r = 2.00573, theta = 2.00573, r - theta = -8.74457e-10
r = 2.00626, theta = 2.00626, r - theta = -7.99708e-10
r = 2.00679, theta = 2.00679, r - theta = -5.7757e-10
r = 2.00732, theta = 2.00732, r - theta = -2.62203e-10
r = 2.00785, theta = 2.00785, r - theta = 8.90754e-11
r = 2.00838, theta = 2.00838, r - theta = 4.15796e-10
r = 2.00891, theta = 2.00891, r - theta = 6.54339e-10
r = 2.00943, theta = 2.00943, r - theta = 7.37933e-10
r = 2.00996, theta = 2.00996, r - theta = 5.9666e-10
r = 2.01049, theta = 2.01049, r - theta = 1.57457e-10
r = 2.01102, theta = 2.01102, r - theta = -4.46953e-10
r = 2.01155, theta = 2.01155, r - theta = -7.84679e-10
r = 2.01208, theta = 2.01208, r - theta = -8.80823e-10
r = 2.01261, theta = 2.01261, r - theta = -7.83891e-10
r = 2.01314, theta = 2.01314, r - theta = -5.45534e-10
r = 2.01367, theta = 2.01367, r - theta = -2.20542e-10
r = 2.01419, theta = 2.01419, r - theta = 1.33158e-10
r = 2.01472, theta = 2.01472, r - theta = 4.54503e-10
r = 2.01525, theta = 2.01525, r - theta = 6.79295e-10
r = 2.01578, theta = 2.01578, r - theta = 7.40204e-10
r = 2.01631, theta = 2.01631, r - theta = 5.66774e-10
r = 2.01684, theta = 2.01684, r - theta = 8.54148e-11
r = 2.01736, theta = 2.01736, r - theta = -5.02339e-10
r = 2.01789, theta = 2.01789, r - theta = -8.12156e-10
r = 2.01842, theta = 2.01842, r - theta = -8.84472e-10
r = 2.01895, theta = 2.01895, r - theta = -7.68404e-10
r = 2.01948, theta = 2.01948, r - theta = -5.16192e-10
r = 2.02001, theta = 2.02001, r - theta = -1.83197e-10
r = 2.02053, theta = 2.02053, r - theta = 1.72094e-10
r = 2.02106, theta = 2.02106, r - theta = 4.88082e-10
r = 2.02159, theta = 2.02159, r - theta = 7.00048e-10
r = 2.02212, theta = 2.02212, r - theta = 7.40157e-10
r = 2.02264, theta = 2.02264, r - theta = 5.37467e-10
r = 2.02317, theta = 2.02317, r - theta = 1.79217e-11
r = 2.0237, theta = 2.0237, r - theta = -5.48428e-10
r = 2.02423, theta = 2.02423, r - theta = -8.34429e-10
r = 2.02476, theta = 2.02476, r - theta = -8.86411e-10
r = 2.02528, theta = 2.02528, r - theta = -7.54042e-10
r = 2.02581, theta = 2.02581, r - theta = -4.901e-10
r = 2.02634, theta = 2.02634, r - theta = -1.50466e-10
r = 2.02686, theta = 2.02686, r - theta = 2.05875e-10
r = 2.02739, theta = 2.02739, r - theta = 5.16838e-10
r = 2.02792, theta = 2.02792, r - theta = 7.17237e-10
r = 2.02845, theta = 2.02845, r - theta = 7.38792e-10
r = 2.02897, theta = 2.02897, r - theta = 5.10123e-10
r = 2.0295, theta = 2.0295, r - theta = -4.1966e-11
r = 2.03003, theta = 2.03003, r - theta = -5.86351e-10
r = 2.03055, theta = 2.03055, r - theta = -8.52498e-10
r = 2.03108, theta = 2.03108, r - theta = -8.8748e-10
r = 2.03161, theta = 2.03161, r - theta = -7.41469e-10
r = 2.03213, theta = 2.03213, r - theta = -4.67718e-10
r = 2.03266, theta = 2.03266, r - theta = -1.22577e-10
r = 2.03319, theta = 2.03319, r - theta = 2.34525e-10
r = 2.03371, theta = 2.03371, r - theta = 5.4107e-10
r = 2.03424, theta = 2.03424, r - theta = 7.31462e-10
r = 2.03477, theta = 2.03477, r - theta = 7.37022e-10
r = 2.03529, theta = 2.03529, r - theta = 4.85996e-10
r = 2.03582, theta = 2.03582, r - theta = -9.03992e-11
r = 2.03635, theta = 2.03635, r - theta = -6.17043e-10
r = 2.03687, theta = 2.03687, r - theta = -8.67192e-10
r = 2.0374, theta = 2.0374, r - theta = -8.88383e-10
r = 2.03792, theta = 2.03792, r - theta = -7.31229e-10
r = 2.03845, theta = 2.03845, r - theta = -4.49418e-10
r = 2.03898, theta = 2.03898, r - theta = -9.97034e-11
r = 2.0395, theta = 2.0395, r - theta = 2.58086e-10
r = 2.04003, theta = 2.04003, r - theta = 5.61059e-10
r = 2.04055, theta = 2.04055, r - theta = 7.43256e-10
r = 2.04108, theta = 2.04108, r - theta = 7.35657e-10
r = 2.04161, theta = 2.04161, r - theta = 4.6618e-10
r = 2.04213, theta = 2.04213, r - theta = -1.27959e-10
r = 2.04266, theta = 2.04266, r - theta = -6.41255e-10
r = 2.04318, theta = 2.04318, r - theta = -8.7918e-10
r = 2.04371, theta = 2.04371, r - theta = -8.89683e-10
r = 2.04423, theta = 2.04423, r - theta = -7.23769e-10
r = 2.04476, theta = 2.04476, r - theta = -4.35497e-10
r = 2.04528, theta = 2.04528, r - theta = -8.19846e-11
r = 2.04581, theta = 2.04581, r - theta = 2.766e-10
r = 2.04634, theta = 2.04634, r - theta = 5.77039e-10
r = 2.04686, theta = 2.04686, r - theta = 7.53067e-10
r = 2.04739, theta = 2.04739, r - theta = 7.35371e-10
r = 2.04791, theta = 2.04791, r - theta = 4.51592e-10
r = 2.04844, theta = 2.04844, r - theta = -1.55248e-10
r = 2.04896, theta = 2.04896, r - theta = -6.59553e-10
r = 2.04949, theta = 2.04949, r - theta = -8.88974e-10
r = 2.05001, theta = 2.05001, r - theta = -8.91818e-10
r = 2.05054, theta = 2.05054, r - theta = -7.19429e-10
r = 2.05106, theta = 2.05106, r - theta = -4.26193e-10
r = 2.05159, theta = 2.05159, r - theta = -6.95319e-11
r = 2.05211, theta = 2.05211, r - theta = 2.90095e-10
r = 2.05263, theta = 2.05263, r - theta = 5.89195e-10
r = 2.05316, theta = 2.05316, r - theta = 7.61246e-10
r = 2.05368, theta = 2.05368, r - theta = 7.36693e-10
r = 2.05421, theta = 2.05421, r - theta = 4.42954e-10
r = 2.05473, theta = 2.05473, r - theta = -1.72671e-10
r = 2.05526, theta = 2.05526, r - theta = -6.72329e-10
r = 2.05578, theta = 2.05578, r - theta = -8.96937e-10
r = 2.05631, theta = 2.05631, r - theta = -8.95106e-10
r = 2.05683, theta = 2.05683, r - theta = -7.18472e-10
r = 2.05735, theta = 2.05735, r - theta = -4.21691e-10
r = 2.05788, theta = 2.05788, r - theta = -6.24434e-11
r = 2.0584, theta = 2.0584, r - theta = 2.98572e-10
r = 2.05893, theta = 2.05893, r - theta = 5.97639e-10
r = 2.05945, theta = 2.05945, r - theta = 7.68026e-10
r = 2.05997, theta = 2.05997, r - theta = 7.39989e-10
r = 2.0605, theta = 2.0605, r - theta = 4.40771e-10
r = 2.06102, theta = 2.06102, r - theta = -1.80426e-10
r = 2.06154, theta = 2.06154, r - theta = -6.79796e-10
r = 2.06207, theta = 2.06207, r - theta = -9.03281e-10
r = 2.06259, theta = 2.06259, r - theta = -8.99747e-10
r = 2.06312, theta = 2.06312, r - theta = -7.21069e-10
r = 2.06364, theta = 2.06364, r - theta = -4.22126e-10
r = 2.06416, theta = 2.06416, r - theta = -6.0806e-11
r = 2.06469, theta = 2.06469, r - theta = 3.02002e-10
r = 2.06521, theta = 2.06521, r - theta = 6.02408e-10
r = 2.06573, theta = 2.06573, r - theta = 7.73522e-10
r = 2.06626, theta = 2.06626, r - theta = 7.45456e-10
r = 2.06678, theta = 2.06678, r - theta = 4.45329e-10
r = 2.0673, theta = 2.0673, r - theta = -1.78508e-10
r = 2.06782, theta = 2.06782, r - theta = -6.8199e-10
r = 2.06835, theta = 2.06835, r - theta = -9.08072e-10
r = 2.06887, theta = 2.06887, r - theta = -9.05825e-10
r = 2.06939, theta = 2.06939, r - theta = -7.27312e-10
r = 2.06992, theta = 2.06992, r - theta = -4.27591e-10
r = 2.07044, theta = 2.07044, r - theta = -6.47042e-11
r = 2.07096, theta = 2.07096, r - theta = 3.00317e-10
r = 2.07148, theta = 2.07148, r - theta = 6.03457e-10
r = 2.07201, theta = 2.07201, r - theta = 7.77716e-10
r = 2.07253, theta = 2.07253, r - theta = 7.53112e-10
r = 2.07305, theta = 2.07305, r - theta = 4.56685e-10
r = 2.07357, theta = 2.07357, r - theta = -1.66704e-10
r = 2.0741, theta = 2.0741, r - theta = -6.78768e-10
r = 2.07462, theta = 2.07462, r - theta = -9.11226e-10
r = 2.07514, theta = 2.07514, r - theta = -9.13307e-10
r = 2.07566, theta = 2.07566, r - theta = -7.37217e-10
r = 2.07619, theta = 2.07619, r - theta = -4.38137e-10
r = 2.07671, theta = 2.07671, r - theta = -7.4218e-11
r = 2.07723, theta = 2.07723, r - theta = 2.93412e-10
r = 2.07775, theta = 2.07775, r - theta = 6.0066e-10
r = 2.07827, theta = 2.07827, r - theta = 7.80464e-10
r = 2.0788, theta = 2.0788, r - theta = 7.62796e-10
r = 2.07932, theta = 2.07932, r - theta = 4.74666e-10
r = 2.07984, theta = 2.07984, r - theta = -1.44588e-10
r = 2.08036, theta = 2.08036, r - theta = -6.69801e-10
r = 2.08088, theta = 2.08088, r - theta = -9.12506e-10
r = 2.0814, theta = 2.0814, r - theta = -9.22042e-10
r = 2.08193, theta = 2.08193, r - theta = -7.50708e-10
r = 2.08245, theta = 2.08245, r - theta = -4.53763e-10
r = 2.08297, theta = 2.08297, r - theta = -8.94227e-11
r = 2.08349, theta = 2.08349, r - theta = 2.81142e-10
r = 2.08401, theta = 2.08401, r - theta = 5.93805e-10
r = 2.08453, theta = 2.08453, r - theta = 7.81488e-10
r = 2.08505, theta = 2.08505, r - theta = 7.74166e-10
r = 2.08558, theta = 2.08558, r - theta = 4.98863e-10
r = 2.0861, theta = 2.0861, r - theta = -1.11512e-10
r = 2.08662, theta = 2.08662, r - theta = -6.54569e-10
r = 2.08714, theta = 2.08714, r - theta = -9.11514e-10
r = 2.08766, theta = 2.08766, r - theta = -9.31752e-10
r = 2.08818, theta = 2.08818, r - theta = -7.67625e-10
r = 2.0887, theta = 2.0887, r - theta = -4.74424e-10
r = 2.08922, theta = 2.08922, r - theta = -1.10381e-10
r = 2.08974, theta = 2.08974, r - theta = 2.63332e-10
r = 2.09026, theta = 2.09026, r - theta = 5.82605e-10
r = 2.09079, theta = 2.09079, r - theta = 7.80391e-10
r = 2.09131, theta = 2.09131, r - theta = 7.8671e-10
r = 2.09183, theta = 2.09183, r - theta = 5.28649e-10
r = 2.09235, theta = 2.09235, r - theta = -6.66014e-11
r = 2.09287, theta = 2.09287, r - theta = -6.32352e-10
r = 2.09339, theta = 2.09339, r - theta = -9.07686e-10
r = 2.09391, theta = 2.09391, r - theta = -9.42022e-10
r = 2.09443, theta = 2.09443, r - theta = -7.87704e-10
r = 2.09495, theta = 2.09495, r - theta = -5.00008e-10
r = 2.09547, theta = 2.09547, r - theta = -1.37137e-10
r = 2.09599, theta = 2.09599, r - theta = 2.39785e-10
r = 2.09651, theta = 2.09651, r - theta = 5.66707e-10
r = 2.09703, theta = 2.09703, r - theta = 7.76659e-10
r = 2.09755, theta = 2.09755, r - theta = 7.99751e-10
r = 2.09807, theta = 2.09807, r - theta = 5.63177e-10
r = 2.09859, theta = 2.09859, r - theta = -8.73746e-12
r = 2.09911, theta = 2.09911, r - theta = -6.0222e-10
r = 2.09963, theta = 2.09963, r - theta = -9.00276e-10
r = 2.10015, theta = 2.10015, r - theta = -9.52296e-10
r = 2.10067, theta = 2.10067, r - theta = -8.10578e-10
r = 2.10119, theta = 2.10119, r - theta = -5.30339e-10
r = 2.10171, theta = 2.10171, r - theta = -1.69705e-10
r = 2.10223, theta = 2.10223, r - theta = 2.10289e-10
r = 2.10275, theta = 2.10275, r - theta = 5.45698e-10
r = 2.10327, theta = 2.10327, r - theta = 7.69671e-10
r = 2.10379, theta = 2.10379, r - theta = 8.12457e-10
r = 2.10431, theta = 2.10431, r - theta = 6.01398e-10
r = 2.10482, theta = 2.10482, r - theta = 6.09357e-11
r = 2.10534, theta = 2.10534, r - theta = -5.63018e-10
r = 2.10586, theta = 2.10586, r - theta = -8.88349e-10
r = 2.10638, theta = 2.10638, r - theta = -9.61857e-10
r = 2.1069, theta = 2.1069, r - theta = -8.35754e-10
r = 2.10742, theta = 2.10742, r - theta = -5.65151e-10
r = 2.10794, theta = 2.10794, r - theta = -2.08054e-10
r = 2.10846, theta = 2.10846, r - theta = 1.74636e-10
r = 2.10898, theta = 2.10898, r - theta = 5.19123e-10
r = 2.1095, theta = 2.1095, r - theta = 7.58724e-10
r = 2.11001, theta = 2.11001, r - theta = 8.23861e-10
r = 2.11053, theta = 2.11053, r - theta = 6.42076e-10
r = 2.11105, theta = 2.11105, r - theta = 1.38018e-10
r = 2.11157, theta = 2.11157, r - theta = -5.13348e-10
r = 2.11209, theta = 2.11209, r - theta = -8.70755e-10
r = 2.11261, theta = 2.11261, r - theta = -9.69813e-10
r = 2.11313, theta = 2.11313, r - theta = -8.62599e-10
r = 2.11364, theta = 2.11364, r - theta = -6.04077e-10
r = 2.11416, theta = 2.11416, r - theta = -2.52089e-10
r = 2.11468, theta = 2.11468, r - theta = 1.32641e-10
r = 2.1152, theta = 2.1152, r - theta = 4.8651e-10
r = 2.11572, theta = 2.11572, r - theta = 7.43043e-10
r = 2.11624, theta = 2.11624, r - theta = 8.32886e-10
r = 2.11675, theta = 2.11675, r - theta = 6.83814e-10
r = 2.11727, theta = 2.11727, r - theta = 2.20731e-10
r = 2.11779, theta = 2.11779, r - theta = -4.51559e-10
r = 2.11831, theta = 2.11831, r - theta = -8.46124e-10
r = 2.11883, theta = 2.11883, r - theta = -9.75078e-10
r = 2.11934, theta = 2.11934, r - theta = -8.90323e-10
r = 2.11986, theta = 2.11986, r - theta = -6.46631e-10
r = 2.12038, theta = 2.12038, r - theta = -3.01637e-10
r = 2.1209, theta = 2.1209, r - theta = 8.41549e-11
r = 2.12141, theta = 2.12141, r - theta = 4.47379e-10
r = 2.12193, theta = 2.12193, r - theta = 7.21807e-10
r = 2.12245, theta = 2.12245, r - theta = 8.38352e-10
r = 2.12297, theta = 2.12297, r - theta = 7.25069e-10
r = 2.12348, theta = 2.12348, r - theta = 3.07154e-10
r = 2.124, theta = 2.124, r - theta = -3.7572e-10
r = 2.12452, theta = 2.12452, r - theta = -8.12836e-10
r = 2.12504, theta = 2.12504, r - theta = -9.76354e-10
r = 2.12555, theta = 2.12555, r - theta = -9.17956e-10
r = 2.12607, theta = 2.12607, r - theta = -6.9218e-10
r = 2.12659, theta = 2.12659, r - theta = -3.56414e-10
r = 2.1271, theta = 2.1271, r - theta = 2.91012e-11
r = 2.12762, theta = 2.12762, r - theta = 4.01277e-10
r = 2.12814, theta = 2.12814, r - theta = 6.94179e-10
r = 2.12866, theta = 2.12866, r - theta = 8.39024e-10
r = 2.12917, theta = 2.12917, r - theta = 7.64189e-10
r = 2.12969, theta = 2.12969, r - theta = 3.95206e-10
r = 2.13021, theta = 2.13021, r - theta = -2.83598e-10
r = 2.13072, theta = 2.13072, r - theta = -7.68998e-10
r = 2.13124, theta = 2.13124, r - theta = -9.72098e-10
r = 2.13176, theta = 2.13176, r - theta = -9.44318e-10
r = 2.13227, theta = 2.13227, r - theta = -7.39921e-10
r = 2.13279, theta = 2.13279, r - theta = -4.16005e-10
r = 2.13331, theta = 2.13331, r - theta = -3.25069e-11
r = 2.13382, theta = 2.13382, r - theta = 3.47804e-10
r = 2.13434, theta = 2.13434, r - theta = 6.59328e-10
r = 2.13485, theta = 2.13485, r - theta = 8.33627e-10
r = 2.13537, theta = 2.13537, r - theta = 7.99439e-10
r = 2.13589, theta = 2.13589, r - theta = 4.82673e-10
r = 2.1364, theta = 2.1364, r - theta = -1.72638e-10
r = 2.13692, theta = 2.13692, r - theta = -7.12426e-10
r = 2.13744, theta = 2.13744, r - theta = -9.60509e-10
r = 2.13795, theta = 2.13795, r - theta = -9.68004e-10
r = 2.13847, theta = 2.13847, r - theta = -7.88858e-10
r = 2.13898, theta = 2.13898, r - theta = -4.79838e-10
r = 2.1395, theta = 2.1395, r - theta = -1.00533e-10
r = 2.14001, theta = 2.14001, r - theta = 2.86643e-10
r = 2.14053, theta = 2.14053, r - theta = 6.16465e-10
r = 2.14105, theta = 2.14105, r - theta = 8.20883e-10
r = 2.14156, theta = 2.14156, r - theta = 8.29037e-10
r = 2.14208, theta = 2.14208, r - theta = 5.67252e-10
r = 2.14259, theta = 2.14259, r - theta = -3.99383e-11
r = 2.14311, theta = 2.14311, r - theta = -6.40615e-10
r = 2.14362, theta = 2.14362, r - theta = -9.39493e-10
r = 2.14414, theta = 2.14414, r - theta = -9.87348e-10
r = 2.14465, theta = 2.14465, r - theta = -8.37766e-10
r = 2.14517, theta = 2.14517, r - theta = -5.47145e-10
r = 2.14568, theta = 2.14568, r - theta = -1.74688e-10
r = 2.1462, theta = 2.1462, r - theta = 2.17591e-10
r = 2.14672, theta = 2.14672, r - theta = 5.64877e-10
r = 2.14723, theta = 2.14723, r - theta = 7.9955e-10
r = 2.14775, theta = 2.14775, r - theta = 8.51192e-10
r = 2.14826, theta = 2.14826, r - theta = 6.4658e-10
r = 2.14877, theta = 2.14877, r - theta = 1.09698e-10
r = 2.14929, theta = 2.14929, r - theta = -5.50707e-10
r = 2.1498, theta = 2.1498, r - theta = -9.06634e-10
r = 2.15032, theta = 2.15032, r - theta = -1.00039e-09
r = 2.15083, theta = 2.15083, r - theta = -8.85159e-10
r = 2.15135, theta = 2.15135, r - theta = -6.16932e-10
r = 2.15186, theta = 2.15186, r - theta = -2.54489e-10
r = 2.15238, theta = 2.15238, r - theta = 1.40598e-10
r = 2.15289, theta = 2.15289, r - theta = 5.03966e-10
r = 2.15341, theta = 2.15341, r - theta = 7.68462e-10
r = 2.15392, theta = 2.15392, r - theta = 8.64144e-10
r = 2.15444, theta = 2.15444, r - theta = 7.18289e-10
r = 2.15495, theta = 2.15495, r - theta = 2.55383e-10
r = 2.15546, theta = 2.15546, r - theta = -4.39464e-10
r = 2.15598, theta = 2.15598, r - theta = -8.59163e-10
r = 2.15649, theta = 2.15649, r - theta = -1.00484e-09
r = 2.15701, theta = 2.15701, r - theta = -9.29261e-10
r = 2.15752, theta = 2.15752, r - theta = -6.8795e-10
r = 2.15803, theta = 2.15803, r - theta = -3.39228e-10
r = 2.15855, theta = 2.15855, r - theta = 5.58091e-11
r = 2.15906, theta = 2.15906, r - theta = 4.33289e-10
r = 2.15958, theta = 2.15958, r - theta = 7.26564e-10
r = 2.16009, theta = 2.16009, r - theta = 8.66212e-10
r = 2.1606, theta = 2.1606, r - theta = 7.80038e-10
r = 2.16112, theta = 2.16112, r - theta = 3.93079e-10
r = 2.16163, theta = 2.16163, r - theta = -3.03233e-10
r = 2.16214, theta = 2.16214, r - theta = -7.9392e-10
r = 2.16266, theta = 2.16266, r - theta = -9.9807e-10
r = 2.16317, theta = 2.16317, r - theta = -9.67961e-10
r = 2.16369, theta = 2.16369, r - theta = -7.58643e-10
r = 2.1642, theta = 2.1642, r - theta = -4.2793e-10
r = 2.16471, theta = 2.16471, r - theta = -3.64015e-11
r = 2.16523, theta = 2.16523, r - theta = 3.52598e-10
r = 2.16574, theta = 2.16574, r - theta = 6.72964e-10
r = 2.16625, theta = 2.16625, r - theta = 8.5583e-10
r = 2.16676, theta = 2.16676, r - theta = 8.29571e-10
r = 2.16728, theta = 2.16728, r - theta = 5.19807e-10
r = 2.16779, theta = 2.16779, r - theta = -1.37919e-10
r = 2.1683, theta = 2.1683, r - theta = -7.07329e-10
r = 2.16882, theta = 2.16882, r - theta = -9.77029e-10
r = 2.16933, theta = 2.16933, r - theta = -9.98786e-10
r = 2.16984, theta = 2.16984, r - theta = -8.27121e-10
r = 2.17036, theta = 2.17036, r - theta = -5.19311e-10
r = 2.17087, theta = 2.17087, r - theta = -1.35381e-10
r = 2.17138, theta = 2.17138, r - theta = 2.61889e-10
r = 2.17189, theta = 2.17189, r - theta = 6.06976e-10
r = 2.17241, theta = 2.17241, r - theta = 8.31607e-10
r = 2.17292, theta = 2.17292, r - theta = 8.64763e-10
r = 2.17343, theta = 2.17343, r - theta = 6.32683e-10
r = 2.17394, theta = 2.17394, r - theta = 5.8864e-11
r = 2.17446, theta = 2.17446, r - theta = -5.9535e-10
r = 2.17497, theta = 2.17497, r - theta = -9.38247e-10
r = 2.17548, theta = 2.17548, r - theta = -1.01885e-09
r = 2.17599, theta = 2.17599, r - theta = -8.91111e-10
r = 2.17651, theta = 2.17651, r - theta = -6.11734e-10
r = 2.17702, theta = 2.17702, r - theta = -2.40156e-10
r = 2.17753, theta = 2.17753, r - theta = 1.61449e-10
r = 2.17804, theta = 2.17804, r - theta = 5.28174e-10
r = 2.17855, theta = 2.17855, r - theta = 7.92374e-10
r = 2.17907, theta = 2.17907, r - theta = 8.83674e-10
r = 2.17958, theta = 2.17958, r - theta = 7.28971e-10
r = 2.18009, theta = 2.18009, r - theta = 2.52428e-10
r = 2.1806, theta = 2.1806, r - theta = -4.53446e-10
r = 2.18111, theta = 2.18111, r - theta = -8.77775e-10
r = 2.18162, theta = 2.18162, r - theta = -1.02481e-09
r = 2.18214, theta = 2.18214, r - theta = -9.47913e-10
r = 2.18265, theta = 2.18265, r - theta = -7.03167e-10
r = 2.18316, theta = 2.18316, r - theta = -3.49385e-10
r = 2.18367, theta = 2.18367, r - theta = 5.19007e-11
r = 2.18418, theta = 2.18418, r - theta = 4.36432e-10
r = 2.18469, theta = 2.18469, r - theta = 7.37232e-10
r = 2.18521, theta = 2.18521, r - theta = 8.84604e-10
r = 2.18572, theta = 2.18572, r - theta = 8.06135e-10
r = 2.18623, theta = 2.18623, r - theta = 4.26696e-10
r = 2.18674, theta = 2.18674, r - theta = -2.76545e-10
r = 2.18725, theta = 2.18725, r - theta = -7.91148e-10
r = 2.18776, theta = 2.18776, r - theta = -1.01285e-09
r = 2.18827, theta = 2.18827, r - theta = -9.94361e-10
r = 2.18878, theta = 2.18878, r - theta = -7.91131e-10
r = 2.18929, theta = 2.18929, r - theta = -4.61305e-10
r = 2.1898, theta = 2.1898, r - theta = -6.57407e-11
r = 2.19032, theta = 2.19032, r - theta = 3.31991e-10
r = 2.19083, theta = 2.19083, r - theta = 6.65616e-10
r = 2.19134, theta = 2.19134, r - theta = 8.6615e-10
r = 2.19185, theta = 2.19185, r - theta = 8.61909e-10
r = 2.19236, theta = 2.19236, r - theta = 5.78502e-10
r = 2.19287, theta = 2.19287, r - theta = -5.89977e-11
r = 2.19338, theta = 2.19338, r - theta = -6.73346e-10
r = 2.19389, theta = 2.19389, r - theta = -9.78587e-10
r = 2.1944, theta = 2.1944, r - theta = -1.02677e-09
r = 2.19491, theta = 2.19491, r - theta = -8.72658e-10
r = 2.19542, theta = 2.19542, r - theta = -5.7369e-10
r = 2.19593, theta = 2.19593, r - theta = -1.90022e-10
r = 2.19644, theta = 2.19644, r - theta = 2.15502e-10
r = 2.19695, theta = 2.19695, r - theta = 5.77343e-10
r = 2.19746, theta = 2.19746, r - theta = 8.27266e-10
r = 2.19797, theta = 2.19797, r - theta = 8.94346e-10
r = 2.19848, theta = 2.19848, r - theta = 7.0497e-10
r = 2.19899, theta = 2.19899, r - theta = 1.82833e-10
r = 2.1995, theta = 2.1995, r - theta = -5.18744e-10
r = 2.20001, theta = 2.20001, r - theta = -9.17085e-10
r = 2.20052, theta = 2.20052, r - theta = -1.0409e-09
r = 2.20103, theta = 2.20103, r - theta = -9.44234e-10
r = 2.20154, theta = 2.20154, r - theta = -6.83795e-10
r = 2.20205, theta = 2.20205, r - theta = -3.18991e-10
r = 2.20256, theta = 2.20256, r - theta = 8.809e-11
r = 2.20307, theta = 2.20307, r - theta = 4.72678e-10
r = 2.20358, theta = 2.20358, r - theta = 7.67321e-10
r = 2.20409, theta = 2.20409, r - theta = 9.01891e-10
r = 2.2046, theta = 2.2046, r - theta = 8.03583e-10
r = 2.20511, theta = 2.20511, r - theta = 3.96912e-10
r = 2.20562, theta = 2.20562, r - theta = -3.21077e-10
r = 2.20613, theta = 2.20613, r - theta = -8.22764e-10
r = 2.20664, theta = 2.20664, r - theta = -1.0319e-09
r = 2.20715, theta = 2.20715, r - theta = -1.00176e-09
r = 2.20766, theta = 2.20766, r - theta = -7.88299e-10
r = 2.20816, theta = 2.20816, r - theta = -4.50146e-10
r = 2.20867, theta = 2.20867, r - theta = -4.85962e-11
r = 2.20918, theta = 2.20918, r - theta = 3.52385e-10
r = 2.20969, theta = 2.20969, r - theta = 6.86164e-10
r = 2.2102, theta = 2.2102, r - theta = 8.83439e-10
r = 2.21071, theta = 2.21071, r - theta = 8.72248e-10
r = 2.21122, theta = 2.21122, r - theta = 5.7796e-10
r = 2.21173, theta = 2.21173, r - theta = -7.33884e-11
r = 2.21224, theta = 2.21224, r - theta = -6.8937e-10
r = 2.21275, theta = 2.21275, r - theta = -9.94237e-10
r = 2.21325, theta = 2.21325, r - theta = -1.04048e-09
r = 2.21376, theta = 2.21376, r - theta = -8.83259e-10
r = 2.21427, theta = 2.21427, r - theta = -5.80381e-10
r = 2.21478, theta = 2.21478, r - theta = -1.92319e-10
r = 2.21529, theta = 2.21529, r - theta = 2.17797e-10
r = 2.2158, theta = 2.2158, r - theta = 5.84183e-10
r = 2.2163, theta = 2.2163, r - theta = 8.38403e-10
r = 2.21681, theta = 2.21681, r - theta = 9.09364e-10
r = 2.21732, theta = 2.21732, r - theta = 7.23326e-10
r = 2.21783, theta = 2.21783, r - theta = 2.039e-10
r = 2.21834, theta = 2.21834, r - theta = -5.09933e-10
r = 2.21885, theta = 2.21885, r - theta = -9.21703e-10
r = 2.21935, theta = 2.21935, r - theta = -1.05497e-09
r = 2.21986, theta = 2.21986, r - theta = -9.64056e-10
r = 2.22037, theta = 2.22037, r - theta = -7.05929e-10
r = 2.22088, theta = 2.22088, r - theta = -3.40201e-10
r = 2.22139, theta = 2.22139, r - theta = 7.0866e-11
r = 2.22189, theta = 2.22189, r - theta = 4.62373e-10
r = 2.2224, theta = 2.2224, r - theta = 7.66774e-10
r = 2.22291, theta = 2.22291, r - theta = 9.13889e-10
r = 2.22342, theta = 2.22342, r - theta = 8.30894e-10
r = 2.22392, theta = 2.22392, r - theta = 4.42331e-10
r = 2.22443, theta = 2.22443, r - theta = -2.76713e-10
r = 2.22494, theta = 2.22494, r - theta = -8.07313e-10
r = 2.22545, theta = 2.22545, r - theta = -1.03904e-09
r = 2.22595, theta = 2.22595, r - theta = -1.02534e-09
r = 2.22646, theta = 2.22646, r - theta = -8.22303e-10
r = 2.22697, theta = 2.22697, r - theta = -4.88662e-10
r = 2.22748, theta = 2.22748, r - theta = -8.57669e-11
r = 2.22798, theta = 2.22798, r - theta = 3.22392e-10
r = 2.22849, theta = 2.22849, r - theta = 6.69199e-10
r = 2.229, theta = 2.229, r - theta = 8.8541e-10
r = 2.22951, theta = 2.22951, r - theta = 8.99156e-10
r = 2.23001, theta = 2.23001, r - theta = 6.35938e-10
r = 2.23052, theta = 2.23052, r - theta = 1.86398e-11
r = 2.23103, theta = 2.23103, r - theta = -6.43284e-10
r = 2.23153, theta = 2.23153, r - theta = -9.857e-10
r = 2.23204, theta = 2.23204, r - theta = -1.06096e-09
r = 2.23255, theta = 2.23255, r - theta = -9.24244e-10
r = 2.23305, theta = 2.23305, r - theta = -6.33355e-10
r = 2.23356, theta = 2.23356, r - theta = -2.48715e-10
r = 2.23407, theta = 2.23407, r - theta = 1.66635e-10
r = 2.23457, theta = 2.23457, r - theta = 5.47037e-10
r = 2.23508, theta = 2.23508, r - theta = 8.24217e-10
r = 2.23559, theta = 2.23559, r - theta = 9.27285e-10
r = 2.23609, theta = 2.23609, r - theta = 7.82743e-10
r = 2.2366, theta = 2.2366, r - theta = 3.14475e-10
r = 2.23711, theta = 2.23711, r - theta = -4.20974e-10
r = 2.23761, theta = 2.23761, r - theta = -8.87133e-10
r = 2.23812, theta = 2.23812, r - theta = -1.06395e-09
r = 2.23863, theta = 2.23863, r - theta = -1.00566e-09
r = 2.23913, theta = 2.23913, r - theta = -7.6912e-10
r = 2.23964, theta = 2.23964, r - theta = -4.1378e-10
r = 2.24014, theta = 2.24014, r - theta = -1.70575e-12
r = 2.24065, theta = 2.24065, r - theta = 4.02433e-10
r = 2.24116, theta = 2.24116, r - theta = 7.31365e-10
r = 2.24166, theta = 2.24166, r - theta = 9.15214e-10
r = 2.24217, theta = 2.24217, r - theta = 8.81502e-10
r = 2.24267, theta = 2.24267, r - theta = 5.55157e-10
r = 2.24318, theta = 2.24318, r - theta = -1.3084e-10
r = 2.24368, theta = 2.24368, r - theta = -7.34608e-10
r = 2.24419, theta = 2.24419, r - theta = -1.02643e-09
r = 2.2447, theta = 2.2447, r - theta = -1.05958e-09
r = 2.2452, theta = 2.2452, r - theta = -8.89911e-10
r = 2.24571, theta = 2.24571, r - theta = -5.75894e-10
r = 2.24621, theta = 2.24621, r - theta = -1.7858e-10
r = 2.24672, theta = 2.24672, r - theta = 2.38382e-10
r = 2.24722, theta = 2.24722, r - theta = 6.08749e-10
r = 2.24773, theta = 2.24773, r - theta = 8.63691e-10
r = 2.24823, theta = 2.24823, r - theta = 9.31787e-10
r = 2.24874, theta = 2.24874, r - theta = 7.39027e-10
r = 2.24925, theta = 2.24925, r - theta = 2.08817e-10
r = 2.24975, theta = 2.24975, r - theta = -5.1845e-10
r = 2.25026, theta = 2.25026, r - theta = -9.3959e-10
r = 2.25076, theta = 2.25076, r - theta = -1.07807e-09
r = 2.25127, theta = 2.25127, r - theta = -9.88752e-10
r = 2.25177, theta = 2.25177, r - theta = -7.29058e-10
r = 2.25228, theta = 2.25228, r - theta = -3.5901e-10
r = 2.25278, theta = 2.25278, r - theta = 5.87947e-11
r = 2.25329, theta = 2.25329, r - theta = 4.59174e-10
r = 2.25379, theta = 2.25379, r - theta = 7.74373e-10
r = 2.2543, theta = 2.2543, r - theta = 9.34052e-10
r = 2.2548, theta = 2.2548, r - theta = 8.65303e-10
r = 2.2553, theta = 2.2553, r - theta = 4.92637e-10
r = 2.25581, theta = 2.25581, r - theta = -2.27986e-10
r = 2.25631, theta = 2.25631, r - theta = -7.93608e-10
r = 2.25682, theta = 2.25682, r - theta = -1.05223e-09
r = 2.25732, theta = 2.25732, r - theta = -1.05767e-09
r = 2.25783, theta = 2.25783, r - theta = -8.66282e-10
r = 2.25833, theta = 2.25833, r - theta = -5.37032e-10
r = 2.25884, theta = 2.25884, r - theta = -1.31435e-10
r = 2.25934, theta = 2.25934, r - theta = 2.86421e-10
r = 2.25984, theta = 2.25984, r - theta = 6.4988e-10
r = 2.26035, theta = 2.26035, r - theta = 8.8972e-10
r = 2.26085, theta = 2.26085, r - theta = 9.34158e-10
r = 2.26136, theta = 2.26136, r - theta = 7.08841e-10
r = 2.26186, theta = 2.26186, r - theta = 1.36862e-10
r = 2.26237, theta = 2.26237, r - theta = -5.77624e-10
r = 2.26287, theta = 2.26287, r - theta = -9.7209e-10
r = 2.26337, theta = 2.26337, r - theta = -1.08763e-09
r = 2.26388, theta = 2.26388, r - theta = -9.79522e-10
r = 2.26438, theta = 2.26438, r - theta = -7.05631e-10
r = 2.26488, theta = 2.26488, r - theta = -3.26363e-10
r = 2.26539, theta = 2.26539, r - theta = 9.53135e-11
r = 2.26589, theta = 2.26589, r - theta = 4.93876e-10
r = 2.2664, theta = 2.2664, r - theta = 8.01246e-10
r = 2.2669, theta = 2.2669, r - theta = 9.46792e-10
r = 2.2674, theta = 2.2674, r - theta = 8.57332e-10
r = 2.26791, theta = 2.26791, r - theta = 4.57133e-10
r = 2.26841, theta = 2.26841, r - theta = -2.79677e-10
r = 2.26891, theta = 2.26891, r - theta = -8.26577e-10
r = 2.26942, theta = 2.26942, r - theta = -1.0685e-09
r = 2.26992, theta = 2.26992, r - theta = -1.05963e-09
r = 2.27042, theta = 2.27042, r - theta = -8.56683e-10
r = 2.27093, theta = 2.27093, r - theta = -5.18937e-10
r = 2.27143, theta = 2.27143, r - theta = -1.08209e-10
r = 2.27193, theta = 2.27193, r - theta = 3.11137e-10
r = 2.27244, theta = 2.27244, r - theta = 6.72195e-10
r = 2.27294, theta = 2.27294, r - theta = 9.05518e-10
r = 2.27344, theta = 2.27344, r - theta = 9.39116e-10
r = 2.27395, theta = 2.27395, r - theta = 6.98464e-10
r = 2.27445, theta = 2.27445, r - theta = 1.06495e-10
r = 2.27495, theta = 2.27495, r - theta = -6.03475e-10
r = 2.27545, theta = 2.27545, r - theta = -9.89005e-10
r = 2.27596, theta = 2.27596, r - theta = -1.09629e-09
r = 2.27646, theta = 2.27646, r - theta = -9.80894e-10
r = 2.27696, theta = 2.27696, r - theta = -7.00929e-10
r = 2.27746, theta = 2.27746, r - theta = -3.17034e-10
r = 2.27797, theta = 2.27797, r - theta = 1.07615e-10
r = 2.27847, theta = 2.27847, r - theta = 5.07313e-10
r = 2.27897, theta = 2.27897, r - theta = 8.13823e-10
r = 2.27947, theta = 2.27947, r - theta = 9.56379e-10
r = 2.27998, theta = 2.27998, r - theta = 8.61686e-10
r = 2.28048, theta = 2.28048, r - theta = 4.53926e-10
r = 2.28098, theta = 2.28098, r - theta = -2.89362e-10
r = 2.28148, theta = 2.28148, r - theta = -8.36634e-10
r = 2.28199, theta = 2.28199, r - theta = -1.07797e-09
r = 2.28249, theta = 2.28249, r - theta = -1.06774e-09
r = 2.28299, theta = 2.28299, r - theta = -8.62877e-10
r = 2.28349, theta = 2.28349, r - theta = -5.22807e-10
r = 2.28399, theta = 2.28399, r - theta = -1.0949e-10
r = 2.2845, theta = 2.2845, r - theta = 3.12597e-10
r = 2.285, theta = 2.285, r - theta = 6.76453e-10
r = 2.2855, theta = 2.2855, r - theta = 9.12564e-10
r = 2.286, theta = 2.286, r - theta = 9.48895e-10
r = 2.2865, theta = 2.2865, r - theta = 7.109e-10
r = 2.28701, theta = 2.28701, r - theta = 1.21514e-10
r = 2.28751, theta = 2.28751, r - theta = -5.97631e-10
r = 2.28801, theta = 2.28801, r - theta = -9.91856e-10
r = 2.28851, theta = 2.28851, r - theta = -1.10542e-09
r = 2.28901, theta = 2.28901, r - theta = -9.9403e-10
r = 2.28951, theta = 2.28951, r - theta = -7.15873e-10
r = 2.29001, theta = 2.29001, r - theta = -3.31669e-10
r = 2.29052, theta = 2.29052, r - theta = 9.53584e-11
r = 2.29102, theta = 2.29102, r - theta = 4.99475e-10
r = 2.29152, theta = 2.29152, r - theta = 8.12443e-10
r = 2.29202, theta = 2.29202, r - theta = 9.63517e-10
r = 2.29252, theta = 2.29252, r - theta = 8.79447e-10
r = 2.29302, theta = 2.29302, r - theta = 4.84478e-10
r = 2.29352, theta = 2.29352, r - theta = -2.56931e-10
r = 2.29402, theta = 2.29402, r - theta = -8.23835e-10
r = 2.29453, theta = 2.29453, r - theta = -1.08081e-09
r = 2.29503, theta = 2.29503, r - theta = -1.08228e-09
r = 2.29553, theta = 2.29553, r - theta = -8.85215e-10
r = 2.29603, theta = 2.29603, r - theta = -5.49041e-10
r = 2.29653, theta = 2.29653, r - theta = -1.35706e-10
r = 2.29703, theta = 2.29703, r - theta = 2.90351e-10
r = 2.29753, theta = 2.29753, r - theta = 6.62194e-10
r = 2.29803, theta = 2.29803, r - theta = 9.10392e-10
r = 2.29853, theta = 2.29853, r - theta = 9.63019e-10
r = 2.29903, theta = 2.29903, r - theta = 7.45659e-10
r = 2.29953, theta = 2.29953, r - theta = 1.81406e-10
r = 2.30003, theta = 2.30003, r - theta = -5.58408e-10
r = 2.30054, theta = 2.30054, r - theta = -9.79358e-10
r = 2.30104, theta = 2.30104, r - theta = -1.11414e-09
r = 2.30154, theta = 2.30154, r - theta = -1.0184e-09
r = 2.30204, theta = 2.30204, r - theta = -7.50292e-10
r = 2.30254, theta = 2.30254, r - theta = -3.70451e-10
r = 2.30304, theta = 2.30304, r - theta = 5.80043e-11
r = 2.30354, theta = 2.30354, r - theta = 4.69467e-10
r = 2.30404, theta = 2.30404, r - theta = 7.95844e-10
r = 2.30454, theta = 2.30454, r - theta = 9.66565e-10
r = 2.30504, theta = 2.30504, r - theta = 9.08575e-10
r = 2.30554, theta = 2.30554, r - theta = 5.46335e-10
r = 2.30604, theta = 2.30604, r - theta = -1.7867e-10
r = 2.30654, theta = 2.30654, r - theta = -7.8513e-10
r = 2.30704, theta = 2.30704, r - theta = -1.07463e-09
r = 2.30754, theta = 2.30754, r - theta = -1.10152e-09
r = 2.30804, theta = 2.30804, r - theta = -9.22619e-10
r = 2.30854, theta = 2.30854, r - theta = -5.97227e-10
r = 2.30904, theta = 2.30904, r - theta = -1.87121e-10
r = 2.30954, theta = 2.30954, r - theta = 2.43447e-10
r = 2.31004, theta = 2.31004, r - theta = 6.27753e-10
r = 2.31054, theta = 2.31054, r - theta = 8.96601e-10
r = 2.31104, theta = 2.31104, r - theta = 9.78322e-10
r = 2.31153, theta = 2.31153, r - theta = 7.9878e-10
r = 2.31203, theta = 2.31203, r - theta = 2.81369e-10
r = 2.31253, theta = 2.31253, r - theta = -4.80686e-10
r = 2.31303, theta = 2.31303, r - theta = -9.47314e-10
r = 2.31353, theta = 2.31353, r - theta = -1.11915e-09
r = 2.31403, theta = 2.31403, r - theta = -1.05167e-09
r = 2.31453, theta = 2.31453, r - theta = -8.0282e-10
r = 2.31503, theta = 2.31503, r - theta = -4.32996e-10
r = 2.31553, theta = 2.31553, r - theta = -5.07194e-12
r = 2.31603, theta = 2.31603, r - theta = 4.15618e-10
r = 2.31653, theta = 2.31653, r - theta = 7.61281e-10
r = 2.31703, theta = 2.31703, r - theta = 9.6166e-10
r = 2.31752, theta = 2.31752, r - theta = 9.44043e-10
r = 2.31802, theta = 2.31802, r - theta = 6.33257e-10
r = 2.31852, theta = 2.31852, r - theta = -4.70797e-11
r = 2.31902, theta = 2.31902, r - theta = -7.14187e-10
r = 2.31952, theta = 2.31952, r - theta = -1.0543e-09
r = 2.32002, theta = 2.32002, r - theta = -1.12152e-09
r = 2.32052, theta = 2.32052, r - theta = -9.724e-10
r = 2.32102, theta = 2.32102, r - theta = -6.65952e-10
r = 2.32151, theta = 2.32151, r - theta = -2.63638e-10
r = 2.32201, theta = 2.32201, r - theta = 1.70623e-10
r = 2.32251, theta = 2.32251, r - theta = 5.70466e-10
r = 2.32301, theta = 2.32301, r - theta = 8.67072e-10
r = 2.32351, theta = 2.32351, r - theta = 9.89175e-10
r = 2.32401, theta = 2.32401, r - theta = 8.63061e-10
r = 2.32451, theta = 2.32451, r - theta = 4.12572e-10
r = 2.325, theta = 2.325, r - theta = -3.55668e-10
r = 2.3255, theta = 2.3255, r - theta = -8.88359e-10
r = 2.326, theta = 2.326, r - theta = -1.11458e-09
r = 2.3265, theta = 2.3265, r - theta = -1.08947e-09
r = 2.327, theta = 2.327, r - theta = -8.70624e-10
r = 2.32749, theta = 2.32749, r - theta = -5.18076e-10
r = 2.32799, theta = 2.32799, r - theta = -9.42988e-11
r = 2.32849, theta = 2.32849, r - theta = 3.3579e-10
r = 2.32899, theta = 2.32899, r - theta = 7.04836e-10
r = 2.32949, theta = 2.32949, r - theta = 9.43044e-10
r = 2.32998, theta = 2.32998, r - theta = 9.78183e-10
r = 2.33048, theta = 2.33048, r - theta = 7.35589e-10
r = 2.33098, theta = 2.33098, r - theta = 1.38159e-10
r = 2.33148, theta = 2.33148, r - theta = -6.01066e-10
r = 2.33197, theta = 2.33197, r - theta = -1.01158e-09
r = 2.33247, theta = 2.33247, r - theta = -1.13581e-09
r = 2.33297, theta = 2.33297, r - theta = -1.02991e-09
r = 2.33347, theta = 2.33347, r - theta = -7.52438e-10
r = 2.33397, theta = 2.33397, r - theta = -3.64418e-10
r = 2.33446, theta = 2.33446, r - theta = 7.07132e-11
r = 2.33496, theta = 2.33496, r - theta = 4.87086e-10
r = 2.33546, theta = 2.33546, r - theta = 8.16402e-10
r = 2.33595, theta = 2.33595, r - theta = 9.87938e-10
r = 2.33645, theta = 2.33645, r - theta = 9.28543e-10
r = 2.33695, theta = 2.33695, r - theta = 5.62644e-10
r = 2.33745, theta = 2.33745, r - theta = -1.70479e-10
r = 2.33794, theta = 2.33794, r - theta = -7.91551e-10
r = 2.33844, theta = 2.33844, r - theta = -1.09147e-09
r = 2.33894, theta = 2.33894, r - theta = -1.1249e-09
r = 2.33943, theta = 2.33943, r - theta = -9.48954e-10
r = 2.33993, theta = 2.33993, r - theta = -6.23146e-10
r = 2.34043, theta = 2.34043, r - theta = -2.09416e-10
r = 2.34092, theta = 2.34092, r - theta = 2.27876e-10
r = 2.34142, theta = 2.34142, r - theta = 6.2195e-10
r = 2.34192, theta = 2.34192, r - theta = 9.03614e-10
r = 2.34242, theta = 2.34242, r - theta = 1.00126e-09
r = 2.34291, theta = 2.34291, r - theta = 8.40857e-10
r = 2.34341, theta = 2.34341, r - theta = 3.45975e-10
r = 2.34391, theta = 2.34391, r - theta = -4.31744e-10
r = 2.3444, theta = 2.3444, r - theta = -9.34666e-10
r = 2.3449, theta = 2.3449, r - theta = -1.13488e-09
r = 2.34539, theta = 2.34539, r - theta = -1.08799e-09
r = 2.34589, theta = 2.34589, r - theta = -8.51994e-10
r = 2.34639, theta = 2.34639, r - theta = -4.87313e-10
r = 2.34688, theta = 2.34688, r - theta = -5.67728e-11
r = 2.34738, theta = 2.34738, r - theta = 3.74392e-10
r = 2.34788, theta = 2.34788, r - theta = 7.38539e-10
r = 2.34837, theta = 2.34837, r - theta = 9.65624e-10
r = 2.34887, theta = 2.34887, r - theta = 9.83195e-10
r = 2.34936, theta = 2.34936, r - theta = 7.16399e-10
r = 2.34986, theta = 2.34986, r - theta = 8.79821e-11
r = 2.35036, theta = 2.35036, r - theta = -6.41828e-10
r = 2.35085, theta = 2.35085, r - theta = -1.03726e-09
r = 2.35135, theta = 2.35135, r - theta = -1.148e-09
r = 2.35184, theta = 2.35184, r - theta = -1.03053e-09
r = 2.35234, theta = 2.35234, r - theta = -7.43711e-10
r = 2.35284, theta = 2.35284, r - theta = -3.48815e-10
r = 2.35333, theta = 2.35333, r - theta = 9.04889e-11
r = 2.35383, theta = 2.35383, r - theta = 5.08136e-10
r = 2.35432, theta = 2.35432, r - theta = 8.35667e-10
r = 2.35482, theta = 2.35482, r - theta = 1.00223e-09
r = 2.35531, theta = 2.35531, r - theta = 9.34567e-10
r = 2.35581, theta = 2.35581, r - theta = 5.5705e-10
r = 2.3563, theta = 2.3563, r - theta = -1.87512e-10
r = 2.3568, theta = 2.3568, r - theta = -8.07545e-10
r = 2.3573, theta = 2.3573, r - theta = -1.10552e-09
r = 2.35779, theta = 2.35779, r - theta = -1.13634e-09
r = 2.35829, theta = 2.35829, r - theta = -9.57319e-10
r = 2.35878, theta = 2.35878, r - theta = -6.28141e-10
r = 2.35928, theta = 2.35928, r - theta = -2.10887e-10
r = 2.35977, theta = 2.35977, r - theta = 2.29978e-10
r = 2.36027, theta = 2.36027, r - theta = 6.27601e-10
r = 2.36076, theta = 2.36076, r - theta = 9.12746e-10
r = 2.36126, theta = 2.36126, r - theta = 1.0138e-09
r = 2.36175, theta = 2.36175, r - theta = 8.56749e-10
r = 2.36225, theta = 2.36225, r - theta = 3.65224e-10
r = 2.36274, theta = 2.36274, r - theta = -4.19325e-10
r = 2.36324, theta = 2.36324, r - theta = -9.35078e-10
r = 2.36373, theta = 2.36373, r - theta = -1.14499e-09
r = 2.36423, theta = 2.36423, r - theta = -1.10478e-09
r = 2.36472, theta = 2.36472, r - theta = -8.72534e-10
r = 2.36522, theta = 2.36522, r - theta = -5.08717e-10
r = 2.36571, theta = 2.36571, r - theta = -7.61737e-11
r = 2.36621, theta = 2.36621, r - theta = 3.59874e-10
r = 2.3667, theta = 2.3667, r - theta = 7.3183e-10
r = 2.36719, theta = 2.36719, r - theta = 9.69725e-10
r = 2.36769, theta = 2.36769, r - theta = 1.00122e-09
r = 2.36818, theta = 2.36818, r - theta = 7.51602e-10
r = 2.36868, theta = 2.36868, r - theta = 1.43791e-10
r = 2.36917, theta = 2.36917, r - theta = -6.09246e-10
r = 2.36967, theta = 2.36967, r - theta = -1.03002e-09
r = 2.37016, theta = 2.37016, r - theta = -1.16066e-09
r = 2.37065, theta = 2.37065, r - theta = -1.05764e-09
r = 2.37115, theta = 2.37115, r - theta = -7.7979e-10
r = 2.37164, theta = 2.37164, r - theta = -3.88315e-10
r = 2.37214, theta = 2.37214, r - theta = 5.32192e-11
r = 2.37263, theta = 2.37263, r - theta = 4.7888e-10
r = 2.37312, theta = 2.37312, r - theta = 8.20369e-10
r = 2.37362, theta = 2.37362, r - theta = 1.00703e-09
r = 2.37411, theta = 2.37411, r - theta = 9.65836e-10
r = 2.37461, theta = 2.37461, r - theta = 6.21414e-10
r = 2.3751, theta = 2.3751, r - theta = -9.85665e-11
r = 2.37559, theta = 2.37559, r - theta = -7.62874e-10
r = 2.37609, theta = 2.37609, r - theta = -1.09739e-09
r = 2.37658, theta = 2.37658, r - theta = -1.15693e-09
r = 2.37707, theta = 2.37707, r - theta = -9.98683e-10
r = 2.37757, theta = 2.37757, r - theta = -6.82177e-10
r = 2.37806, theta = 2.37806, r - theta = -2.69309e-10
r = 2.37856, theta = 2.37856, r - theta = 1.75671e-10
r = 2.37905, theta = 2.37905, r - theta = 5.86162e-10
r = 2.37954, theta = 2.37954, r - theta = 8.93206e-10
r = 2.38004, theta = 2.38004, r - theta = 1.02549e-09
r = 2.38053, theta = 2.38053, r - theta = 9.0937e-10
r = 2.38102, theta = 2.38102, r - theta = 4.68824e-10
r = 2.38152, theta = 2.38152, r - theta = -3.12954e-10
r = 2.38201, theta = 2.38201, r - theta = -8.8528e-10
r = 2.3825, theta = 2.3825, r - theta = -1.1417e-09
r = 2.38299, theta = 2.38299, r - theta = -1.13772e-09
r = 2.38349, theta = 2.38349, r - theta = -9.3119e-10
r = 2.38398, theta = 2.38398, r - theta = -5.82305e-10
r = 2.38447, theta = 2.38447, r - theta = -1.53612e-10
r = 2.38497, theta = 2.38497, r - theta = 2.90001e-10
r = 2.38546, theta = 2.38546, r - theta = 6.813e-10
r = 2.38595, theta = 2.38595, r - theta = 9.50713e-10
r = 2.38645, theta = 2.38645, r - theta = 1.02632e-09
r = 2.38694, theta = 2.38694, r - theta = 8.33873e-10
r = 2.38743, theta = 2.38743, r - theta = 2.96768e-10
r = 2.38792, theta = 2.38792, r - theta = -4.92491e-10
r = 2.38842, theta = 2.38842, r - theta = -9.81027e-10
r = 2.38891, theta = 2.38891, r - theta = -1.16697e-09
r = 2.3894, theta = 2.3894, r - theta = -1.10646e-09
r = 2.38989, theta = 2.38989, r - theta = -8.57979e-10
r = 2.39039, theta = 2.39039, r - theta = -4.8235e-10
r = 2.39088, theta = 2.39088, r - theta = -4.27232e-11
r = 2.39137, theta = 2.39137, r - theta = 3.95413e-10
r = 2.39186, theta = 2.39186, r - theta = 7.64238e-10
r = 2.39236, theta = 2.39236, r - theta = 9.93599e-10
r = 2.39285, theta = 2.39285, r - theta = 1.01101e-09
r = 2.39334, theta = 2.39334, r - theta = 7.41666e-10
r = 2.39383, theta = 2.39383, r - theta = 1.08418e-10
r = 2.39432, theta = 2.39432, r - theta = -6.41776e-10
r = 2.39482, theta = 2.39482, r - theta = -1.05421e-09
r = 2.39531, theta = 2.39531, r - theta = -1.17675e-09
r = 2.3958, theta = 2.3958, r - theta = -1.06614e-09
r = 2.39629, theta = 2.39629, r - theta = -7.81459e-10
r = 2.39678, theta = 2.39678, r - theta = -3.84096e-10
r = 2.39728, theta = 2.39728, r - theta = 6.2224e-11
r = 2.39777, theta = 2.39777, r - theta = 4.91454e-10
r = 2.39826, theta = 2.39826, r - theta = 8.35226e-10
r = 2.39875, theta = 2.39875, r - theta = 1.02285e-09
r = 2.39924, theta = 2.39924, r - theta = 9.81309e-10
r = 2.39973, theta = 2.39973, r - theta = 6.35279e-10
r = 2.40023, theta = 2.40023, r - theta = -8.86011e-11
r = 2.40072, theta = 2.40072, r - theta = -7.64954e-10
r = 2.40121, theta = 2.40121, r - theta = -1.10849e-09
r = 2.4017, theta = 2.4017, r - theta = -1.1742e-09
r = 2.40219, theta = 2.40219, r - theta = -1.01938e-09
r = 2.40268, theta = 2.40268, r - theta = -7.03663e-10
r = 2.40317, theta = 2.40317, r - theta = -2.8898e-10
r = 2.40367, theta = 2.40367, r - theta = 1.60417e-10
r = 2.40416, theta = 2.40416, r - theta = 5.77962e-10
r = 2.40465, theta = 2.40465, r - theta = 8.94775e-10
r = 2.40514, theta = 2.40514, r - theta = 1.03967e-09
r = 2.40563, theta = 2.40563, r - theta = 9.39147e-10
r = 2.40612, theta = 2.40612, r - theta = 5.17399e-10
r = 2.40661, theta = 2.40661, r - theta = -2.62425e-10
r = 2.4071, theta = 2.4071, r - theta = -8.65749e-10
r = 2.4076, theta = 2.4076, r - theta = -1.14713e-09
r = 2.40809, theta = 2.40809, r - theta = -1.16207e-09
r = 2.40858, theta = 2.40858, r - theta = -9.68417e-10
r = 2.40907, theta = 2.40907, r - theta = -6.2629e-10
r = 2.40956, theta = 2.40956, r - theta = -1.9813e-10
r = 2.41005, theta = 2.41005, r - theta = 2.51322e-10
r = 2.41054, theta = 2.41054, r - theta = 6.55019e-10
r = 2.41103, theta = 2.41103, r - theta = 9.43615e-10
r = 2.41152, theta = 2.41152, r - theta = 1.04546e-09
r = 2.41201, theta = 2.41201, r - theta = 8.86608e-10
r = 2.4125, theta = 2.4125, r - theta = 3.90812e-10
r = 2.41299, theta = 2.41299, r - theta = -4.10254e-10
r = 2.41348, theta = 2.41348, r - theta = -9.47491e-10
r = 2.41397, theta = 2.41397, r - theta = -1.173e-09
r = 2.41446, theta = 2.41446, r - theta = -1.14279e-09
r = 2.41495, theta = 2.41495, r - theta = -9.15167e-10
r = 2.41544, theta = 2.41544, r - theta = -5.5074e-10
r = 2.41593, theta = 2.41593, r - theta = -1.12406e-10
r = 2.41642, theta = 2.41642, r - theta = 3.34643e-10
r = 2.41691, theta = 2.41691, r - theta = 7.22916e-10
r = 2.41741, theta = 2.41741, r - theta = 9.82639e-10
r = 2.4179, theta = 2.4179, r - theta = 1.04174e-09
r = 2.41839, theta = 2.41839, r - theta = 8.25863e-10
r = 2.41888, theta = 2.41888, r - theta = 2.58357e-10
r = 2.41937, theta = 2.41937, r - theta = -5.35455e-10
r = 2.41985, theta = 2.41985, r - theta = -1.01315e-09
r = 2.42034, theta = 2.42034, r - theta = -1.18865e-09
r = 2.42083, theta = 2.42083, r - theta = -1.11844e-09
r = 2.42132, theta = 2.42132, r - theta = -8.61263e-10
r = 2.42181, theta = 2.42181, r - theta = -4.78154e-10
r = 2.4223, theta = 2.4223, r - theta = -3.24381e-11
r = 2.42279, theta = 2.42279, r - theta = 4.10278e-10
r = 2.42328, theta = 2.42328, r - theta = 7.82105e-10
r = 2.42377, theta = 2.42377, r - theta = 1.01287e-09
r = 2.42426, theta = 2.42426, r - theta = 1.03012e-09
r = 2.42475, theta = 2.42475, r - theta = 7.59129e-10
r = 2.42524, theta = 2.42524, r - theta = 1.22879e-10
r = 2.42573, theta = 2.42573, r - theta = -6.41031e-10
r = 2.42622, theta = 2.42622, r - theta = -1.06534e-09
r = 2.42671, theta = 2.42671, r - theta = -1.19632e-09
r = 2.4272, theta = 2.4272, r - theta = -1.09084e-09
r = 2.42769, theta = 2.42769, r - theta = -8.08074e-10
r = 2.42818, theta = 2.42818, r - theta = -4.09445e-10
r = 2.42867, theta = 2.42867, r - theta = 4.1338e-11
r = 2.42915, theta = 2.42915, r - theta = 4.78292e-10
r = 2.42964, theta = 2.42964, r - theta = 8.3316e-10
r = 2.43013, theta = 2.43013, r - theta = 1.03542e-09
r = 2.43062, theta = 2.43062, r - theta = 1.01226e-09
r = 2.43111, theta = 2.43111, r - theta = 6.88629e-10
r = 2.4316, theta = 2.4316, r - theta = -1.27351e-11
r = 2.43209, theta = 2.43209, r - theta = -7.29654e-10
r = 2.43258, theta = 2.43258, r - theta = -1.10639e-09
r = 2.43307, theta = 2.43307, r - theta = -1.19794e-09
r = 2.43355, theta = 2.43355, r - theta = -1.06155e-09
r = 2.43404, theta = 2.43404, r - theta = -7.5675e-10
r = 2.43453, theta = 2.43453, r - theta = -3.45332e-10
r = 2.43502, theta = 2.43502, r - theta = 1.08647e-10
r = 2.43551, theta = 2.43551, r - theta = 5.38868e-10
r = 2.436, theta = 2.436, r - theta = 8.76744e-10
r = 2.43649, theta = 2.43649, r - theta = 1.05143e-09
r = 2.43697, theta = 2.43697, r - theta = 9.89821e-10
r = 2.43746, theta = 2.43746, r - theta = 6.16548e-10
r = 2.43795, theta = 2.43795, r - theta = -1.35902e-10
r = 2.43844, theta = 2.43844, r - theta = -8.03679e-10
r = 2.43893, theta = 2.43893, r - theta = -1.13833e-09
r = 2.43942, theta = 2.43942, r - theta = -1.1952e-09
r = 2.4399, theta = 2.4399, r - theta = -1.03188e-09
r = 2.44039, theta = 2.44039, r - theta = -7.08237e-10
r = 2.44088, theta = 2.44088, r - theta = -2.86371e-10
r = 2.44137, theta = 2.44137, r - theta = 1.69342e-10
r = 2.44186, theta = 2.44186, r - theta = 5.92282e-10
r = 2.44234, theta = 2.44234, r - theta = 9.13571e-10
r = 2.44283, theta = 2.44283, r - theta = 1.06208e-09
r = 2.44332, theta = 2.44332, r - theta = 9.64433e-10
r = 2.44381, theta = 2.44381, r - theta = 5.44995e-10
r = 2.4443, theta = 2.4443, r - theta = -2.40642e-10
r = 2.44478, theta = 2.44478, r - theta = -8.65175e-10
r = 2.44527, theta = 2.44527, r - theta = -1.16293e-09
r = 2.44576, theta = 2.44576, r - theta = -1.18956e-09
r = 2.44625, theta = 2.44625, r - theta = -1.00297e-09
r = 2.44673, theta = 2.44673, r - theta = -6.63313e-10
r = 2.44722, theta = 2.44722, r - theta = -2.32982e-10
r = 2.44771, theta = 2.44771, r - theta = 2.23377e-10
r = 2.4482, theta = 2.4482, r - theta = 6.38875e-10
r = 2.44868, theta = 2.44868, r - theta = 9.44379e-10
r = 2.44917, theta = 2.44917, r - theta = 1.06851e-09
r = 2.44966, theta = 2.44966, r - theta = 9.37666e-10
r = 2.45015, theta = 2.45015, r - theta = 4.75973e-10
r = 2.45063, theta = 2.45063, r - theta = -3.29008e-10
r = 2.45112, theta = 2.45112, r - theta = -9.15935e-10
r = 2.45161, theta = 2.45161, r - theta = -1.18171e-09
r = 2.45209, theta = 2.45209, r - theta = -1.18226e-09
r = 2.45258, theta = 2.45258, r - theta = -9.75753e-10
r = 2.45307, theta = 2.45307, r - theta = -6.2261e-10
r = 2.45356, theta = 2.45356, r - theta = -1.85471e-10
r = 2.45404, theta = 2.45404, r - theta = 2.70779e-10
r = 2.45453, theta = 2.45453, r - theta = 6.79019e-10
r = 2.45502, theta = 2.45502, r - theta = 9.69898e-10
r = 2.4555, theta = 2.4555, r - theta = 1.07183e-09
r = 2.45599, theta = 2.45599, r - theta = 9.10994e-10
r = 2.45648, theta = 2.45648, r - theta = 4.11343e-10
r = 2.45696, theta = 2.45696, r - theta = -4.02773e-10
r = 2.45745, theta = 2.45745, r - theta = -9.57506e-10
r = 2.45794, theta = 2.45794, r - theta = -1.19597e-09
r = 2.45842, theta = 2.45842, r - theta = -1.17434e-09
r = 2.45891, theta = 2.45891, r - theta = -9.51014e-10
r = 2.4594, theta = 2.4594, r - theta = -5.86635e-10
r = 2.45988, theta = 2.45988, r - theta = -1.44061e-10
r = 2.46037, theta = 2.46037, r - theta = 3.11621e-10
r = 2.46086, theta = 2.46086, r - theta = 7.13096e-10
r = 2.46134, theta = 2.46134, r - theta = 9.90825e-10
r = 2.46183, theta = 2.46183, r - theta = 1.07305e-09
r = 2.46231, theta = 2.46231, r - theta = 8.85774e-10
r = 2.4628, theta = 2.4628, r - theta = 3.52802e-10
r = 2.46329, theta = 2.46329, r - theta = -4.63442e-10
r = 2.46377, theta = 2.46377, r - theta = -9.91192e-10
r = 2.46426, theta = 2.46426, r - theta = -1.20681e-09
r = 2.46475, theta = 2.46475, r - theta = -1.16668e-09
r = 2.46523, theta = 2.46523, r - theta = -9.29398e-10
r = 2.46572, theta = 2.46572, r - theta = -5.55794e-10
r = 2.4662, theta = 2.4662, r - theta = -1.08905e-10
r = 2.46669, theta = 2.46669, r - theta = 3.46009e-10
r = 2.46717, theta = 2.46717, r - theta = 7.41476e-10
r = 2.46766, theta = 2.46766, r - theta = 1.0078e-09
r = 2.46815, theta = 2.46815, r - theta = 1.07309e-09
r = 2.46863, theta = 2.46863, r - theta = 8.63216e-10
r = 2.46912, theta = 2.46912, r - theta = 3.01854e-10
r = 2.4696, theta = 2.4696, r - theta = -5.12274e-10
r = 2.47009, theta = 2.47009, r - theta = -1.01808e-09
r = 2.47057, theta = 2.47057, r - theta = -1.21514e-09
r = 2.47106, theta = 2.47106, r - theta = -1.16e-09
r = 2.47155, theta = 2.47155, r - theta = -9.11425e-10
r = 2.47203, theta = 2.47203, r - theta = -5.30401e-10
r = 2.47252, theta = 2.47252, r - theta = -8.01097e-11
r = 2.473, theta = 2.473, r - theta = 3.74056e-10
r = 2.47349, theta = 2.47349, r - theta = 7.64496e-10
r = 2.47397, theta = 2.47397, r - theta = 1.0214e-09
r = 2.47446, theta = 2.47446, r - theta = 1.07276e-09
r = 2.47494, theta = 2.47494, r - theta = 8.44366e-10
r = 2.47543, theta = 2.47543, r - theta = 2.59789e-10
r = 2.47591, theta = 2.47591, r - theta = -5.50283e-10
r = 2.4764, theta = 2.4764, r - theta = -1.03905e-09
r = 2.47688, theta = 2.47688, r - theta = -1.22168e-09
r = 2.47737, theta = 2.47737, r - theta = -1.15487e-09
r = 2.47785, theta = 2.47785, r - theta = -8.97509e-10
r = 2.47834, theta = 2.47834, r - theta = -5.10704e-10
r = 2.47882, theta = 2.47882, r - theta = -5.77463e-11
r = 2.47931, theta = 2.47931, r - theta = 3.95868e-10
r = 2.47979, theta = 2.47979, r - theta = 7.82447e-10
r = 2.48028, theta = 2.48028, r - theta = 1.0321e-09
r = 2.48076, theta = 2.48076, r - theta = 1.07274e-09
r = 2.48125, theta = 2.48125, r - theta = 8.30088e-10
r = 2.48173, theta = 2.48173, r - theta = 2.2767e-10
r = 2.48222, theta = 2.48222, r - theta = -5.78255e-10
r = 2.4827, theta = 2.4827, r - theta = -1.05477e-09
r = 2.48318, theta = 2.48318, r - theta = -1.22699e-09
r = 2.48367, theta = 2.48367, r - theta = -1.15173e-09
r = 2.48415, theta = 2.48415, r - theta = -8.87965e-10
r = 2.48464, theta = 2.48464, r - theta = -4.96886e-10
r = 2.48512, theta = 2.48512, r - theta = -4.18665e-11
r = 2.48561, theta = 2.48561, r - theta = 4.11532e-10
r = 2.48609, theta = 2.48609, r - theta = 7.95556e-10
r = 2.48657, theta = 2.48657, r - theta = 1.04027e-09
r = 2.48706, theta = 2.48706, r - theta = 1.07353e-09
r = 2.48754, theta = 2.48754, r - theta = 8.21049e-10
r = 2.48803, theta = 2.48803, r - theta = 2.06315e-10
r = 2.48851, theta = 2.48851, r - theta = -5.96755e-10
r = 2.489, theta = 2.489, r - theta = -1.06573e-09
r = 2.48948, theta = 2.48948, r - theta = -1.23149e-09
r = 2.48996, theta = 2.48996, r - theta = -1.1509e-09
r = 2.49045, theta = 2.49045, r - theta = -8.83022e-10
r = 2.49093, theta = 2.49093, r - theta = -4.89084e-10
r = 2.49141, theta = 2.49141, r - theta = -3.25078e-11
r = 2.4919, theta = 2.4919, r - theta = 4.21107e-10
r = 2.49238, theta = 2.49238, r - theta = 8.03983e-10
r = 2.49287, theta = 2.49287, r - theta = 1.04616e-09
r = 2.49335, theta = 2.49335, r - theta = 1.07551e-09
r = 2.49383, theta = 2.49383, r - theta = 8.17714e-10
r = 2.49432, theta = 2.49432, r - theta = 1.96288e-10
r = 2.4948, theta = 2.4948, r - theta = -6.06129e-10
r = 2.49528, theta = 2.49528, r - theta = -1.07224e-09
r = 2.49577, theta = 2.49577, r - theta = -1.23542e-09
r = 2.49625, theta = 2.49625, r - theta = -1.15259e-09
r = 2.49673, theta = 2.49673, r - theta = -8.82824e-10
r = 2.49722, theta = 2.49722, r - theta = -4.87387e-10
r = 2.4977, theta = 2.4977, r - theta = -2.97069e-11
r = 2.49818, theta = 2.49818, r - theta = 4.24615e-10
r = 2.49867, theta = 2.49867, r - theta = 8.07807e-10
r = 2.49915, theta = 2.49915, r - theta = 1.04992e-09
r = 2.49963, theta = 2.49963, r - theta = 1.07886e-09
r = 2.50012, theta = 2.50012, r - theta = 8.2033e-10
r = 2.5006, theta = 2.5006, r - theta = 1.97894e-10
r = 2.50108, theta = 2.50108, r - theta = -6.0651e-10
r = 2.50157, theta = 2.50157, r - theta = -1.0744e-09
r = 2.50205, theta = 2.50205, r - theta = -1.23888e-09
r = 2.50253, theta = 2.50253, r - theta = -1.15687e-09
r = 2.50301, theta = 2.50301, r - theta = -8.8744e-10
r = 2.5035, theta = 2.5035, r - theta = -4.91847e-10
r = 2.50398, theta = 2.50398, r - theta = -3.3495e-11
r = 2.50446, theta = 2.50446, r - theta = 4.2204e-10
r = 2.50494, theta = 2.50494, r - theta = 8.07026e-10
r = 2.50543, theta = 2.50543, r - theta = 1.05157e-09
r = 2.50591, theta = 2.50591, r - theta = 1.08361e-09
r = 2.50639, theta = 2.50639, r - theta = 8.28932e-10
r = 2.50688, theta = 2.50688, r - theta = 2.11172e-10
r = 2.50736, theta = 2.50736, r - theta = -5.9782e-10
r = 2.50784, theta = 2.50784, r - theta = -1.07216e-09
r = 2.50832, theta = 2.50832, r - theta = -1.24183e-09
r = 2.5088, theta = 2.5088, r - theta = -1.16372e-09
r = 2.50929, theta = 2.50929, r - theta = -8.96863e-10
r = 2.50977, theta = 2.50977, r - theta = -5.02474e-10
r = 2.51025, theta = 2.51025, r - theta = -4.3908e-11
r = 2.51073, theta = 2.51073, r - theta = 4.13325e-10
r = 2.51122, theta = 2.51122, r - theta = 8.01557e-10
r = 2.5117, theta = 2.5117, r - theta = 1.05097e-09
r = 2.51218, theta = 2.51218, r - theta = 1.0896e-09
r = 2.51266, theta = 2.51266, r - theta = 8.43335e-10
r = 2.51314, theta = 2.51314, r - theta = 2.35896e-10
r = 2.51363, theta = 2.51363, r - theta = -5.79766e-10
r = 2.51411, theta = 2.51411, r - theta = -1.06528e-09
r = 2.51459, theta = 2.51459, r - theta = -1.24408e-09
r = 2.51507, theta = 2.51507, r - theta = -1.173e-09
r = 2.51555, theta = 2.51555, r - theta = -9.11008e-10
r = 2.51604, theta = 2.51604, r - theta = -5.19243e-10
r = 2.51652, theta = 2.51652, r - theta = -6.09788e-11
r = 2.517, theta = 2.517, r - theta = 3.98368e-10
r = 2.51748, theta = 2.51748, r - theta = 7.91231e-10
r = 2.51796, theta = 2.51796, r - theta = 1.0479e-09
r = 2.51844, theta = 2.51844, r - theta = 1.09654e-09
r = 2.51892, theta = 2.51892, r - theta = 8.6314e-10
r = 2.51941, theta = 2.51941, r - theta = 2.71581e-10
r = 2.51989, theta = 2.51989, r - theta = -5.51839e-10
r = 2.52037, theta = 2.52037, r - theta = -1.05333e-09
r = 2.52085, theta = 2.52085, r - theta = -1.24529e-09
r = 2.52133, theta = 2.52133, r - theta = -1.18445e-09
r = 2.52181, theta = 2.52181, r - theta = -9.29708e-10
r = 2.52229, theta = 2.52229, r - theta = -5.42083e-10
r = 2.52278, theta = 2.52278, r - theta = -8.47344e-11
r = 2.52326, theta = 2.52326, r - theta = 3.77041e-10
r = 2.52374, theta = 2.52374, r - theta = 7.7581e-10
r = 2.52422, theta = 2.52422, r - theta = 1.04201e-09
r = 2.5247, theta = 2.5247, r - theta = 1.10393e-09
r = 2.52518, theta = 2.52518, r - theta = 8.87743e-10
r = 2.52566, theta = 2.52566, r - theta = 3.17485e-10
r = 2.52614, theta = 2.52614, r - theta = -5.13308e-10
r = 2.52662, theta = 2.52662, r - theta = -1.03568e-09
r = 2.5271, theta = 2.5271, r - theta = -1.24494e-09
r = 2.52759, theta = 2.52759, r - theta = -1.19769e-09
r = 2.52807, theta = 2.52807, r - theta = -9.52709e-10
r = 2.52855, theta = 2.52855, r - theta = -5.7087e-10
r = 2.52903, theta = 2.52903, r - theta = -1.1519e-10
r = 2.52951, theta = 2.52951, r - theta = 3.49184e-10
r = 2.52999, theta = 2.52999, r - theta = 7.54984e-10
r = 2.53047, theta = 2.53047, r - theta = 1.03281e-09
r = 2.53095, theta = 2.53095, r - theta = 1.11114e-09
r = 2.53143, theta = 2.53143, r - theta = 9.16339e-10
r = 2.53191, theta = 2.53191, r - theta = 3.72625e-10
r = 2.53239, theta = 2.53239, r - theta = -4.63212e-10
r = 2.53287, theta = 2.53287, r - theta = -1.01153e-09
r = 2.53335, theta = 2.53335, r - theta = -1.24238e-09
r = 2.53383, theta = 2.53383, r - theta = -1.21222e-09
r = 2.53431, theta = 2.53431, r - theta = -9.79661e-10
r = 2.53479, theta = 2.53479, r - theta = -6.05421e-10
r = 2.53527, theta = 2.53527, r - theta = -1.5234e-10
r = 2.53575, theta = 2.53575, r - theta = 3.14621e-10
r = 2.53623, theta = 2.53623, r - theta = 7.28384e-10
r = 2.53671, theta = 2.53671, r - theta = 1.01975e-09
r = 2.53719, theta = 2.53719, r - theta = 1.11742e-09
r = 2.53767, theta = 2.53767, r - theta = 9.47942e-10
r = 2.53815, theta = 2.53815, r - theta = 4.35787e-10
r = 2.53863, theta = 2.53863, r - theta = -4.00353e-10
r = 2.53911, theta = 2.53911, r - theta = -9.79848e-10
r = 2.53959, theta = 2.53959, r - theta = -1.23676e-09
r = 2.54007, theta = 2.54007, r - theta = -1.22737e-09
r = 2.54055, theta = 2.54055, r - theta = -1.0101e-09
r = 2.54103, theta = 2.54103, r - theta = -6.45482e-10
r = 2.54151, theta = 2.54151, r - theta = -1.9614e-10
r = 2.54199, theta = 2.54199, r - theta = 2.73175e-10
r = 2.54247, theta = 2.54247, r - theta = 6.95607e-10
r = 2.54295, theta = 2.54295, r - theta = 1.00219e-09
r = 2.54343, theta = 2.54343, r - theta = 1.12184e-09
r = 2.54391, theta = 2.54391, r - theta = 9.8139e-10
r = 2.54439, theta = 2.54439, r - theta = 5.05541e-10
r = 2.54487, theta = 2.54487, r - theta = -3.23281e-10
r = 2.54535, theta = 2.54535, r - theta = -9.39387e-10
r = 2.54583, theta = 2.54583, r - theta = -1.22704e-09
r = 2.54631, theta = 2.54631, r - theta = -1.24234e-09
r = 2.54679, theta = 2.54679, r - theta = -1.04346e-09
r = 2.54726, theta = 2.54726, r - theta = -6.90713e-10
r = 2.54774, theta = 2.54774, r - theta = -2.46499e-10
r = 2.54822, theta = 2.54822, r - theta = 2.24678e-10
r = 2.5487, theta = 2.5487, r - theta = 6.56212e-10
r = 2.54918, theta = 2.54918, r - theta = 9.79393e-10
r = 2.54966, theta = 2.54966, r - theta = 1.12342e-09
r = 2.55014, theta = 2.55014, r - theta = 1.01537e-09
r = 2.55062, theta = 2.55062, r - theta = 5.80263e-10
r = 2.5511, theta = 2.5511, r - theta = -2.30284e-10
r = 2.55158, theta = 2.55158, r - theta = -8.88671e-10
r = 2.55205, theta = 2.55205, r - theta = -1.212e-09
r = 2.55253, theta = 2.55253, r - theta = -1.25613e-09
r = 2.55301, theta = 2.55301, r - theta = -1.07901e-09
r = 2.55349, theta = 2.55349, r - theta = -7.40669e-10
r = 2.55397, theta = 2.55397, r - theta = -3.03257e-10
r = 2.55445, theta = 2.55445, r - theta = 1.68993e-10
r = 2.55493, theta = 2.55493, r - theta = 6.09755e-10
r = 2.5554, theta = 2.5554, r - theta = 9.5061e-10
r = 2.55588, theta = 2.55588, r - theta = 1.12104e-09
r = 2.55636, theta = 2.55636, r - theta = 1.04846e-09
r = 2.55684, theta = 2.55684, r - theta = 6.58155e-10
r = 2.55732, theta = 2.55732, r - theta = -1.19369e-10
r = 2.5578, theta = 2.5578, r - theta = -8.25972e-10
r = 2.55828, theta = 2.55828, r - theta = -1.19019e-09
r = 2.55875, theta = 2.55875, r - theta = -1.2676e-09
r = 2.55923, theta = 2.55923, r - theta = -1.11588e-09
r = 2.55971, theta = 2.55971, r - theta = -7.94784e-10
r = 2.56019, theta = 2.56019, r - theta = -3.66169e-10
r = 2.56067, theta = 2.56067, r - theta = 1.06036e-10
r = 2.56114, theta = 2.56114, r - theta = 5.5581e-10
r = 2.56162, theta = 2.56162, r - theta = 9.1505e-10
r = 2.5621, theta = 2.5621, r - theta = 1.11357e-09
r = 2.56258, theta = 2.56258, r - theta = 1.07909e-09
r = 2.56306, theta = 2.56306, r - theta = 7.37278e-10
r = 2.56353, theta = 2.56353, r - theta = 1.16858e-11
r = 2.56401, theta = 2.56401, r - theta = -7.49291e-10
r = 2.56449, theta = 2.56449, r - theta = -1.15989e-09
r = 2.56497, theta = 2.56497, r - theta = -1.27535e-09
r = 2.56544, theta = 2.56544, r - theta = -1.15301e-09
r = 2.56592, theta = 2.56592, r - theta = -8.52347e-10
r = 2.5664, theta = 2.5664, r - theta = -4.34876e-10
r = 2.56688, theta = 2.56688, r - theta = 3.57949e-11
r = 2.56736, theta = 2.56736, r - theta = 4.93983e-10
r = 2.56783, theta = 2.56783, r - theta = 8.71926e-10
r = 2.56831, theta = 2.56831, r - theta = 1.09979e-09
r = 2.56879, theta = 2.56879, r - theta = 1.10566e-09
r = 2.56927, theta = 2.56927, r - theta = 8.15567e-10
r = 2.56974, theta = 2.56974, r - theta = 1.53443e-10
r = 2.57022, theta = 2.57022, r - theta = -6.56344e-10
r = 2.5707, theta = 2.5707, r - theta = -1.11917e-09
r = 2.57117, theta = 2.57117, r - theta = -1.27777e-09
r = 2.57165, theta = 2.57165, r - theta = -1.18917e-09
r = 2.57213, theta = 2.57213, r - theta = -9.12481e-10
r = 2.57261, theta = 2.57261, r - theta = -5.08888e-10
r = 2.57308, theta = 2.57308, r - theta = -4.16369e-11
r = 2.57356, theta = 2.57356, r - theta = 4.23948e-10
r = 2.57404, theta = 2.57404, r - theta = 8.20481e-10
r = 2.57451, theta = 2.57451, r - theta = 1.07851e-09
r = 2.57499, theta = 2.57499, r - theta = 1.1265e-09
r = 2.57547, theta = 2.57547, r - theta = 8.90878e-10
r = 2.57594, theta = 2.57594, r - theta = 2.95986e-10
r = 2.57642, theta = 2.57642, r - theta = -5.44533e-10
r = 2.5769, theta = 2.5769, r - theta = -1.06579e-09
r = 2.57738, theta = 2.57738, r - theta = -1.27301e-09
r = 2.57785, theta = 2.57785, r - theta = -1.22287e-09
r = 2.57833, theta = 2.57833, r - theta = -9.74114e-10
r = 2.57881, theta = 2.57881, r - theta = -5.87551e-10
r = 2.57928, theta = 2.57928, r - theta = -1.26045e-10
r = 2.57976, theta = 2.57976, r - theta = 3.45472e-10
r = 2.58023, theta = 2.58023, r - theta = 7.60014e-10
r = 2.58071, theta = 2.58071, r - theta = 1.04853e-09
r = 2.58119, theta = 2.58119, r - theta = 1.13992e-09
r = 2.58166, theta = 2.58166, r - theta = 9.61006e-10
r = 2.58214, theta = 2.58214, r - theta = 4.36577e-10
r = 2.58262, theta = 2.58262, r - theta = -4.10931e-10
r = 2.58309, theta = 2.58309, r - theta = -9.97194e-10
r = 2.58357, theta = 2.58357, r - theta = -1.25892e-09
r = 2.58405, theta = 2.58405, r - theta = -1.25239e-09
r = 2.58452, theta = 2.58452, r - theta = -1.03595e-09
r = 2.585, theta = 2.585, r - theta = -6.70025e-10
r = 2.58547, theta = 2.58547, r - theta = -2.17054e-10
r = 2.58595, theta = 2.58595, r - theta = 2.58448e-10
r = 2.58643, theta = 2.58643, r - theta = 6.89917e-10
r = 2.5869, theta = 2.5869, r - theta = 1.00874e-09
r = 2.58738, theta = 2.58738, r - theta = 1.14425e-09
r = 2.58785, theta = 2.58785, r - theta = 1.02373e-09
r = 2.58833, theta = 2.58833, r - theta = 5.72423e-10
r = 2.58881, theta = 2.58881, r - theta = -2.52248e-10
r = 2.58928, theta = 2.58928, r - theta = -9.10516e-10
r = 2.58976, theta = 2.58976, r - theta = -1.23305e-09
r = 2.59023, theta = 2.59023, r - theta = -1.27572e-09
r = 2.59071, theta = 2.59071, r - theta = -1.09646e-09
r = 2.59118, theta = 2.59118, r - theta = -7.55248e-10
r = 2.59166, theta = 2.59166, r - theta = -3.14096e-10
r = 2.59214, theta = 2.59214, r - theta = 1.62925e-10
r = 2.59261, theta = 2.59261, r - theta = 6.09707e-10
r = 2.59309, theta = 2.59309, r - theta = 9.58094e-10
r = 2.59356, theta = 2.59356, r - theta = 1.13789e-09
r = 2.59404, theta = 2.59404, r - theta = 1.07686e-09
r = 2.59451, theta = 2.59451, r - theta = 7.00709e-10
r = 2.59499, theta = 2.59499, r - theta = -6.48126e-11
r = 2.59546, theta = 2.59546, r - theta = -8.02515e-10
r = 2.59594, theta = 2.59594, r - theta = -1.19262e-09
r = 2.59641, theta = 2.59641, r - theta = -1.29056e-09
r = 2.59689, theta = 2.59689, r - theta = -1.15381e-09
r = 2.59736, theta = 2.59736, r - theta = -8.41902e-10
r = 2.59784, theta = 2.59784, r - theta = -4.16384e-10
r = 2.59831, theta = 2.59831, r - theta = 5.91469e-11
r = 2.59879, theta = 2.59879, r - theta = 5.1906e-10
r = 2.59926, theta = 2.59926, r - theta = 8.95687e-10
r = 2.59974, theta = 2.59974, r - theta = 1.11933e-09
r = 2.60021, theta = 2.60021, r - theta = 1.11824e-09
r = 2.60069, theta = 2.60069, r - theta = 8.18656e-10
r = 2.60116, theta = 2.60116, r - theta = 1.44776e-10
r = 2.60164, theta = 2.60164, r - theta = -6.69566e-10
r = 2.60211, theta = 2.60211, r - theta = -1.13448e-09
r = 2.60259, theta = 2.60259, r - theta = -1.29424e-09
r = 2.60306, theta = 2.60306, r - theta = -1.20587e-09
r = 2.60354, theta = 2.60354, r - theta = -9.28388e-10
r = 2.60401, theta = 2.60401, r - theta = -5.2287e-10
r = 2.60449, theta = 2.60449, r - theta = -5.24141e-11
r = 2.60496, theta = 2.60496, r - theta = 4.17855e-10
r = 2.60544, theta = 2.60544, r - theta = 8.2078e-10
r = 2.60591, theta = 2.60591, r - theta = 1.08718e-09
r = 2.60639, theta = 2.60639, r - theta = 1.14585e-09
r = 2.60686, theta = 2.60686, r - theta = 9.23545e-10
r = 2.60733, theta = 2.60733, r - theta = 3.45015e-10
r = 2.60781, theta = 2.60781, r - theta = -5.07621e-10
r = 2.60828, theta = 2.60828, r - theta = -1.05508e-09
r = 2.60876, theta = 2.60876, r - theta = -1.28375e-09
r = 2.60923, theta = 2.60923, r - theta = -1.25014e-09
r = 2.60971, theta = 2.60971, r - theta = -1.01278e-09
r = 2.61018, theta = 2.61018, r - theta = -6.32208e-10
r = 2.61065, theta = 2.61065, r - theta = -1.71015e-10
r = 2.61113, theta = 2.61113, r - theta = 3.06209e-10
r = 2.6116, theta = 2.6116, r - theta = 7.32846e-10
r = 2.61208, theta = 2.61208, r - theta = 1.04026e-09
r = 2.61255, theta = 2.61255, r - theta = 1.15779e-09
r = 2.61302, theta = 2.61302, r - theta = 1.01278e-09
r = 2.6135, theta = 2.6135, r - theta = 5.30517e-10
r = 2.61397, theta = 2.61397, r - theta = -3.12178e-10
r = 2.61444, theta = 2.61444, r - theta = -9.50447e-10
r = 2.61492, theta = 2.61492, r - theta = -1.25565e-09
r = 2.61539, theta = 2.61539, r - theta = -1.28376e-09
r = 2.61587, theta = 2.61587, r - theta = -1.09278e-09
r = 2.61634, theta = 2.61634, r - theta = -7.4272e-10
r = 2.61681, theta = 2.61681, r - theta = -2.95605e-10
r = 2.61729, theta = 2.61729, r - theta = 1.84522e-10
r = 2.61776, theta = 2.61776, r - theta = 6.31609e-10
r = 2.61823, theta = 2.61823, r - theta = 9.77591e-10
r = 2.61871, theta = 2.61871, r - theta = 1.15239e-09
r = 2.61918, theta = 2.61918, r - theta = 1.08392e-09
r = 2.61965, theta = 2.61965, r - theta = 6.98078e-10
r = 2.62013, theta = 2.62013, r - theta = -7.82552e-11
r = 2.6206, theta = 2.6206, r - theta = -8.16136e-10
r = 2.62107, theta = 2.62107, r - theta = -1.20605e-09
r = 2.62155, theta = 2.62155, r - theta = -1.30343e-09
r = 2.62202, theta = 2.62202, r - theta = -1.16572e-09
r = 2.62249, theta = 2.62249, r - theta = -8.52351e-10
r = 2.62297, theta = 2.62297, r - theta = -4.24786e-10
r = 2.62344, theta = 2.62344, r - theta = 5.35172e-11
r = 2.62391, theta = 2.62391, r - theta = 5.17093e-10
r = 2.62439, theta = 2.62439, r - theta = 8.98472e-10
r = 2.62486, theta = 2.62486, r - theta = 1.12818e-09
r = 2.62533, theta = 2.62533, r - theta = 1.13474e-09
r = 2.6258, theta = 2.6258, r - theta = 8.44677e-10
r = 2.62628, theta = 2.62628, r - theta = 1.82505e-10
r = 2.62675, theta = 2.62675, r - theta = -6.47206e-10
r = 2.62722, theta = 2.62722, r - theta = -1.13061e-09
r = 2.6277, theta = 2.6277, r - theta = -1.30541e-09
r = 2.62817, theta = 2.62817, r - theta = -1.22847e-09
r = 2.62864, theta = 2.62864, r - theta = -9.58631e-10
r = 2.62911, theta = 2.62911, r - theta = -5.56763e-10
r = 2.62959, theta = 2.62959, r - theta = -8.57145e-11
r = 2.63006, theta = 2.63006, r - theta = 3.89663e-10
r = 2.63053, theta = 2.63053, r - theta = 8.0252e-10
r = 2.631, theta = 2.631, r - theta = 1.08401e-09
r = 2.63148, theta = 2.63148, r - theta = 1.16329e-09
r = 2.63195, theta = 2.63195, r - theta = 9.67527e-10
r = 2.63242, theta = 2.63242, r - theta = 4.21887e-10
r = 2.63289, theta = 2.63289, r - theta = -4.38181e-10
r = 2.63337, theta = 2.63337, r - theta = -1.02444e-09
r = 2.63384, theta = 2.63384, r - theta = -1.28544e-09
r = 2.63431, theta = 2.63431, r - theta = -1.27742e-09
r = 2.63478, theta = 2.63478, r - theta = -1.05863e-09
r = 2.63525, theta = 2.63525, r - theta = -6.89306e-10
r = 2.63573, theta = 2.63573, r - theta = -2.31664e-10
r = 2.6362, theta = 2.6362, r - theta = 2.50081e-10
r = 2.63667, theta = 2.63667, r - theta = 6.8972e-10
r = 2.63714, theta = 2.63714, r - theta = 1.01906e-09
r = 2.63762, theta = 2.63762, r - theta = 1.16792e-09
r = 2.63809, theta = 2.63809, r - theta = 1.06413e-09
r = 2.63856, theta = 2.63856, r - theta = 6.33538e-10
r = 2.63903, theta = 2.63903, r - theta = -1.8301e-10
r = 2.6395, theta = 2.6395, r - theta = -8.82109e-10
r = 2.63997, theta = 2.63997, r - theta = -1.23872e-09
r = 2.64045, theta = 2.64045, r - theta = -1.30846e-09
r = 2.64092, theta = 2.64092, r - theta = -1.14892e-09
r = 2.64139, theta = 2.64139, r - theta = -8.19701e-10
r = 2.64186, theta = 2.64186, r - theta = -3.82361e-10
r = 2.64233, theta = 2.64233, r - theta = 9.95422e-11
r = 2.6428, theta = 2.6428, r - theta = 5.60474e-10
r = 2.64328, theta = 2.64328, r - theta = 9.32913e-10
r = 2.64375, theta = 2.64375, r - theta = 1.14736e-09
r = 2.64422, theta = 2.64422, r - theta = 1.13235e-09
r = 2.64469, theta = 2.64469, r - theta = 8.144e-10
r = 2.64516, theta = 2.64516, r - theta = 1.18089e-10
r = 2.64563, theta = 2.64563, r - theta = -6.97598e-10
r = 2.6461, theta = 2.6461, r - theta = -1.1599e-09
r = 2.64658, theta = 2.64658, r - theta = -1.3169e-09
r = 2.64705, theta = 2.64705, r - theta = -1.22553e-09
r = 2.64752, theta = 2.64752, r - theta = -9.44711e-10
r = 2.64799, theta = 2.64799, r - theta = -5.35333e-10
r = 2.64846, theta = 2.64846, r - theta = -6.02656e-11
r = 2.64893, theta = 2.64893, r - theta = 4.15648e-10
r = 2.6494, theta = 2.6494, r - theta = 8.25591e-10
r = 2.64987, theta = 2.64987, r - theta = 1.10077e-09
r = 2.65034, theta = 2.65034, r - theta = 1.17041e-09
r = 2.65082, theta = 2.65082, r - theta = 9.61795e-10
r = 2.65129, theta = 2.65129, r - theta = 4.00198e-10
r = 2.65176, theta = 2.65176, r - theta = -4.64243e-10
r = 2.65223, theta = 2.65223, r - theta = -1.04298e-09
r = 2.6527, theta = 2.6527, r - theta = -1.29745e-09
r = 2.65317, theta = 2.65317, r - theta = -1.28389e-09
r = 2.65364, theta = 2.65364, r - theta = -1.06052e-09
r = 2.65411, theta = 2.65411, r - theta = -6.87546e-10
r = 2.65458, theta = 2.65458, r - theta = -2.27118e-10
r = 2.65505, theta = 2.65505, r - theta = 2.56634e-10
r = 2.65552, theta = 2.65552, r - theta = 6.97616e-10
r = 2.65599, theta = 2.65599, r - theta = 1.02776e-09
r = 2.65646, theta = 2.65646, r - theta = 1.17705e-09
r = 2.65693, theta = 2.65693, r - theta = 1.07348e-09
r = 2.6574, theta = 2.6574, r - theta = 6.43097e-10
r = 2.65788, theta = 2.65788, r - theta = -1.74714e-10
r = 2.65835, theta = 2.65835, r - theta = -8.81319e-10
r = 2.65882, theta = 2.65882, r - theta = -1.24416e-09
r = 2.65929, theta = 2.65929, r - theta = -1.31879e-09
r = 2.65976, theta = 2.65976, r - theta = -1.1627e-09
r = 2.66023, theta = 2.66023, r - theta = -8.35362e-10
r = 2.6607, theta = 2.6607, r - theta = -3.98201e-10
r = 2.66117, theta = 2.66117, r - theta = 8.53939e-11
r = 2.66164, theta = 2.66164, r - theta = 5.50069e-10
r = 2.66211, theta = 2.66211, r - theta = 9.28519e-10
r = 2.66258, theta = 2.66258, r - theta = 1.15147e-09
r = 2.66305, theta = 2.66305, r - theta = 1.14771e-09
r = 2.66352, theta = 2.66352, r - theta = 8.4404e-10
r = 2.66399, theta = 2.66399, r - theta = 1.65329e-10
r = 2.66446, theta = 2.66446, r - theta = -6.67558e-10
r = 2.66493, theta = 2.66493, r - theta = -1.15042e-09
r = 2.6654, theta = 2.6654, r - theta = -1.32437e-09
r = 2.66587, theta = 2.66587, r - theta = -1.24615e-09
r = 2.66634, theta = 2.66634, r - theta = -9.74488e-10
r = 2.66681, theta = 2.66681, r - theta = -5.7006e-10
r = 2.66728, theta = 2.66728, r - theta = -9.54894e-11
r = 2.66775, theta = 2.66775, r - theta = 3.84641e-10
r = 2.66821, theta = 2.66821, r - theta = 8.03797e-10
r = 2.66868, theta = 2.66868, r - theta = 1.09349e-09
r = 2.66915, theta = 2.66915, r - theta = 1.1833e-09
r = 2.66962, theta = 2.66962, r - theta = 1.00082e-09
r = 2.67009, theta = 2.67009, r - theta = 4.71725e-10
r = 2.67056, theta = 2.67056, r - theta = -3.93601e-10
r = 2.67103, theta = 2.67103, r - theta = -1.00886e-09
r = 2.6715, theta = 2.6715, r - theta = -1.29401e-09
r = 2.67197, theta = 2.67197, r - theta = -1.30505e-09
r = 2.67244, theta = 2.67244, r - theta = -1.09993e-09
r = 2.67291, theta = 2.67291, r - theta = -7.38551e-10
r = 2.67338, theta = 2.67338, r - theta = -2.8275e-10
r = 2.67385, theta = 2.67385, r - theta = 2.0368e-10
r = 2.67432, theta = 2.67432, r - theta = 6.55005e-10
r = 2.67479, theta = 2.67479, r - theta = 1.00354e-09
r = 2.67525, theta = 2.67525, r - theta = 1.17967e-09
r = 2.67572, theta = 2.67572, r - theta = 1.11183e-09
r = 2.67619, theta = 2.67619, r - theta = 7.26497e-10
r = 2.67666, theta = 2.67666, r - theta = -5.05698e-11
r = 2.67713, theta = 2.67713, r - theta = -8.11324e-10
r = 2.6776, theta = 2.6776, r - theta = -1.22034e-09
r = 2.67807, theta = 2.67807, r - theta = -1.33283e-09
r = 2.67854, theta = 2.67854, r - theta = -1.20594e-09
r = 2.67901, theta = 2.67901, r - theta = -8.98789e-10
r = 2.67947, theta = 2.67947, r - theta = -4.72395e-10
r = 2.67994, theta = 2.67994, r - theta = 1.02593e-11
r = 2.68041, theta = 2.68041, r - theta = 4.8426e-10
r = 2.68088, theta = 2.68088, r - theta = 8.82753e-10
r = 2.68135, theta = 2.68135, r - theta = 1.13695e-09
r = 2.68182, theta = 2.68182, r - theta = 1.17613e-09
r = 2.68229, theta = 2.68229, r - theta = 9.27617e-10
r = 2.68275, theta = 2.68275, r - theta = 3.16829e-10
r = 2.68322, theta = 2.68322, r - theta = -5.48859e-10
r = 2.68369, theta = 2.68369, r - theta = -1.09517e-09
r = 2.68416, theta = 2.68416, r - theta = -1.3221e-09
r = 2.68463, theta = 2.68463, r - theta = -1.28598e-09
r = 2.6851, theta = 2.6851, r - theta = -1.04509e-09
r = 2.68556, theta = 2.68556, r - theta = -6.59646e-10
r = 2.68603, theta = 2.68603, r - theta = -1.91777e-10
r = 2.6865, theta = 2.6865, r - theta = 2.94441e-10
r = 2.68697, theta = 2.68697, r - theta = 7.33003e-10
r = 2.68744, theta = 2.68744, r - theta = 1.05597e-09
r = 2.68791, theta = 2.68791, r - theta = 1.1935e-09
r = 2.68837, theta = 2.68837, r - theta = 1.07377e-09
r = 2.68884, theta = 2.68884, r - theta = 6.23089e-10
r = 2.68931, theta = 2.68931, r - theta = -2.11636e-10
r = 2.68978, theta = 2.68978, r - theta = -9.09458e-10
r = 2.69025, theta = 2.69025, r - theta = -1.26463e-09
r = 2.69071, theta = 2.69071, r - theta = -1.33265e-09
r = 2.69118, theta = 2.69118, r - theta = -1.17095e-09
r = 2.69165, theta = 2.69165, r - theta = -8.38891e-10
r = 2.69212, theta = 2.69212, r - theta = -3.97755e-10
r = 2.69258, theta = 2.69258, r - theta = 8.92464e-11
r = 2.69305, theta = 2.69305, r - theta = 5.56985e-10
r = 2.69352, theta = 2.69352, r - theta = 9.38403e-10
r = 2.69399, theta = 2.69399, r - theta = 1.16452e-09
r = 2.69445, theta = 2.69445, r - theta = 1.16444e-09
r = 2.69492, theta = 2.69492, r - theta = 8.65336e-10
r = 2.69539, theta = 2.69539, r - theta = 1.9247e-10
r = 2.69586, theta = 2.69586, r - theta = -6.53246e-10
r = 2.69632, theta = 2.69632, r - theta = -1.15127e-09
r = 2.69679, theta = 2.69679, r - theta = -1.33765e-09
r = 2.69726, theta = 2.69726, r - theta = -1.26894e-09
r = 2.69773, theta = 2.69773, r - theta = -1.00363e-09
r = 2.69819, theta = 2.69819, r - theta = -6.02122e-10
r = 2.69866, theta = 2.69866, r - theta = -1.26745e-10
r = 2.69913, theta = 2.69913, r - theta = 3.58263e-10
r = 2.6996, theta = 2.6996, r - theta = 7.86745e-10
r = 2.70006, theta = 2.70006, r - theta = 1.09063e-09
r = 2.70053, theta = 2.70053, r - theta = 1.19992e-09
r = 2.701, theta = 2.701, r - theta = 1.04273e-09
r = 2.70146, theta = 2.70146, r - theta = 5.45232e-10
r = 2.70193, theta = 2.70193, r - theta = -3.15637e-10
r = 2.7024, theta = 2.7024, r - theta = -9.71952e-10
r = 2.70287, theta = 2.70287, r - theta = -1.29175e-09
r = 2.70333, theta = 2.70333, r - theta = -1.3307e-09
r = 2.7038, theta = 2.7038, r - theta = -1.14641e-09
r = 2.70427, theta = 2.70427, r - theta = -7.98382e-10
r = 2.70473, theta = 2.70473, r - theta = -3.48026e-10
r = 2.7052, theta = 2.7052, r - theta = 1.41328e-10
r = 2.70567, theta = 2.70567, r - theta = 6.04442e-10
r = 2.70613, theta = 2.70613, r - theta = 9.7417e-10
r = 2.7066, theta = 2.7066, r - theta = 1.18146e-09
r = 2.70707, theta = 2.70707, r - theta = 1.15534e-09
r = 2.70753, theta = 2.70753, r - theta = 8.22951e-10
r = 2.708, theta = 2.708, r - theta = 1.09515e-10
r = 2.70847, theta = 2.70847, r - theta = -7.1559e-10
r = 2.70893, theta = 2.70893, r - theta = -1.18472e-09
r = 2.7094, theta = 2.7094, r - theta = -1.3469e-09
r = 2.70987, theta = 2.70987, r - theta = -1.25882e-09
r = 2.71033, theta = 2.71033, r - theta = -9.79056e-10
r = 2.7108, theta = 2.7108, r - theta = -5.68101e-10
r = 2.71126, theta = 2.71126, r - theta = -8.83467e-11
r = 2.71173, theta = 2.71173, r - theta = 3.9591e-10
r = 2.7122, theta = 2.7122, r - theta = 8.18471e-10
r = 2.71266, theta = 2.71266, r - theta = 1.11123e-09
r = 2.71313, theta = 2.71313, r - theta = 1.2042e-09
r = 2.7136, theta = 2.7136, r - theta = 1.02546e-09
r = 2.71406, theta = 2.71406, r - theta = 5.01216e-10
r = 2.71453, theta = 2.71453, r - theta = -3.70083e-10
r = 2.71499, theta = 2.71499, r - theta = -1.0053e-09
r = 2.71546, theta = 2.71546, r - theta = -1.30715e-09
r = 2.71593, theta = 2.71593, r - theta = -1.33137e-09
r = 2.71639, theta = 2.71639, r - theta = -1.13562e-09
r = 2.71686, theta = 2.71686, r - theta = -7.79433e-10
r = 2.71732, theta = 2.71732, r - theta = -3.24244e-10
r = 2.71779, theta = 2.71779, r - theta = 1.66609e-10
r = 2.71825, theta = 2.71825, r - theta = 6.27896e-10
r = 2.71872, theta = 2.71872, r - theta = 9.92488e-10
r = 2.71919, theta = 2.71919, r - theta = 1.19137e-09
r = 2.71965, theta = 2.71965, r - theta = 1.15362e-09
r = 2.72012, theta = 2.72012, r - theta = 8.06429e-10
r = 2.72058, theta = 2.72058, r - theta = 7.5107e-11
r = 2.72105, theta = 2.72105, r - theta = -7.41146e-10
r = 2.72151, theta = 2.72151, r - theta = -1.19997e-09
r = 2.72198, theta = 2.72198, r - theta = -1.35352e-09
r = 2.72245, theta = 2.72245, r - theta = -1.25847e-09
r = 2.72291, theta = 2.72291, r - theta = -9.73392e-10
r = 2.72338, theta = 2.72338, r - theta = -5.58753e-10
r = 2.72384, theta = 2.72384, r - theta = -7.69047e-11
r = 2.72431, theta = 2.72431, r - theta = 4.07909e-10
r = 2.72477, theta = 2.72477, r - theta = 8.29556e-10
r = 2.72524, theta = 2.72524, r - theta = 1.12002e-09
r = 2.7257, theta = 2.7257, r - theta = 1.20939e-09
r = 2.72617, theta = 2.72617, r - theta = 1.02587e-09
r = 2.72663, theta = 2.72663, r - theta = 4.95787e-10
r = 2.7271, theta = 2.7271, r - theta = -3.78798e-10
r = 2.72756, theta = 2.72756, r - theta = -1.01278e-09
r = 2.72803, theta = 2.72803, r - theta = -1.31356e-09
r = 2.72849, theta = 2.72849, r - theta = -1.33684e-09
r = 2.72896, theta = 2.72896, r - theta = -1.1402e-09
r = 2.72942, theta = 2.72942, r - theta = -7.83112e-10
r = 2.72989, theta = 2.72989, r - theta = -3.26922e-10
r = 2.73035, theta = 2.73035, r - theta = 1.65131e-10
r = 2.73082, theta = 2.73082, r - theta = 6.27931e-10
r = 2.73128, theta = 2.73128, r - theta = 9.94478e-10
r = 2.73175, theta = 2.73175, r - theta = 1.19589e-09
r = 2.73221, theta = 2.73221, r - theta = 1.16142e-09
r = 2.73268, theta = 2.73268, r - theta = 8.18413e-10
r = 2.73314, theta = 2.73314, r - theta = 9.23626e-11
r = 2.73361, theta = 2.73361, r - theta = -7.31807e-10
r = 2.73407, theta = 2.73407, r - theta = -1.19863e-09
r = 2.73453, theta = 2.73453, r - theta = -1.3588e-09
r = 2.735, theta = 2.735, r - theta = -1.26889e-09
r = 2.73546, theta = 2.73546, r - theta = -9.87359e-10
r = 2.73593, theta = 2.73593, r - theta = -5.74526e-10
r = 2.73639, theta = 2.73639, r - theta = -9.2605e-11
r = 2.73686, theta = 2.73686, r - theta = 3.94321e-10
r = 2.73732, theta = 2.73732, r - theta = 8.20295e-10
r = 2.73779, theta = 2.73779, r - theta = 1.11749e-09
r = 2.73825, theta = 2.73825, r - theta = 1.21619e-09
r = 2.73871, theta = 2.73871, r - theta = 1.04482e-09
r = 2.73918, theta = 2.73918, r - theta = 5.29928e-10
r = 2.73964, theta = 2.73964, r - theta = -3.42067e-10
r = 2.74011, theta = 2.74011, r - theta = -9.9463e-10
r = 2.74057, theta = 2.74057, r - theta = -1.31118e-09
r = 2.74104, theta = 2.74104, r - theta = -1.34725e-09
r = 2.7415, theta = 2.7415, r - theta = -1.16028e-09
r = 2.74196, theta = 2.74196, r - theta = -8.09539e-10
r = 2.74243, theta = 2.74243, r - theta = -3.56199e-10
r = 2.74289, theta = 2.74289, r - theta = 1.36715e-10
r = 2.74335, theta = 2.74335, r - theta = 6.04304e-10
r = 2.74382, theta = 2.74382, r - theta = 9.79802e-10
r = 2.74428, theta = 2.74428, r - theta = 1.19458e-09
r = 2.74475, theta = 2.74475, r - theta = 1.17813e-09
r = 2.74521, theta = 2.74521, r - theta = 8.58092e-10
r = 2.74567, theta = 2.74567, r - theta = 1.60239e-10
r = 2.74614, theta = 2.74614, r - theta = -6.86196e-10
r = 2.7466, theta = 2.7466, r - theta = -1.17951e-09
r = 2.74706, theta = 2.74706, r - theta = -1.36177e-09
r = 2.74753, theta = 2.74753, r - theta = -1.28935e-09
r = 2.74799, theta = 2.74799, r - theta = -1.02047e-09
r = 2.74846, theta = 2.74846, r - theta = -6.15225e-10
r = 2.74892, theta = 2.74892, r - theta = -1.3557e-10
r = 2.74938, theta = 2.74938, r - theta = 3.54674e-10
r = 2.74985, theta = 2.74985, r - theta = 7.89829e-10
r = 2.75031, theta = 2.75031, r - theta = 1.10235e-09
r = 2.75077, theta = 2.75077, r - theta = 1.22285e-09
r = 2.75124, theta = 2.75124, r - theta = 1.08005e-09
r = 2.7517, theta = 2.7517, r - theta = 6.00833e-10
r = 2.75216, theta = 2.75216, r - theta = -2.56666e-10
r = 2.75263, theta = 2.75263, r - theta = -9.48038e-10
r = 2.75309, theta = 2.75309, r - theta = -1.29762e-09
r = 2.75355, theta = 2.75355, r - theta = -1.36072e-09
r = 2.75401, theta = 2.75401, r - theta = -1.19447e-09
r = 2.75448, theta = 2.75448, r - theta = -8.57888e-10
r = 2.75494, theta = 2.75494, r - theta = -4.1183e-10
r = 2.7554, theta = 2.7554, r - theta = 8.09788e-11
r = 2.75587, theta = 2.75587, r - theta = 5.55966e-10
r = 2.75633, theta = 2.75633, r - theta = 9.46699e-10
r = 2.75679, theta = 2.75679, r - theta = 1.18489e-09
r = 2.75726, theta = 2.75726, r - theta = 1.20042e-09
r = 2.75772, theta = 2.75772, r - theta = 9.21273e-10
r = 2.75818, theta = 2.75818, r - theta = 2.73625e-10
r = 2.75864, theta = 2.75864, r - theta = -5.99629e-10
r = 2.75911, theta = 2.75911, r - theta = -1.13859e-09
r = 2.75957, theta = 2.75957, r - theta = -1.35913e-09
r = 2.76003, theta = 2.76003, r - theta = -1.31729e-09
r = 2.76049, theta = 2.76049, r - theta = -1.07097e-09
r = 2.76096, theta = 2.76096, r - theta = -6.79928e-10
r = 2.76142, theta = 2.76142, r - theta = -2.05764e-10
r = 2.76188, theta = 2.76188, r - theta = 2.88067e-10
r = 2.76234, theta = 2.76234, r - theta = 7.36268e-10
r = 2.76281, theta = 2.76281, r - theta = 1.07169e-09
r = 2.76327, theta = 2.76327, r - theta = 1.22533e-09
r = 2.76373, theta = 2.76373, r - theta = 1.12636e-09
r = 2.76419, theta = 2.76419, r - theta = 7.02081e-10
r = 2.76466, theta = 2.76466, r - theta = -1.15763e-10
r = 2.76512, theta = 2.76512, r - theta = -8.67058e-10
r = 2.76558, theta = 2.76558, r - theta = -1.2679e-09
r = 2.76604, theta = 2.76604, r - theta = -1.37321e-09
r = 2.76651, theta = 2.76651, r - theta = -1.23978e-09
r = 2.76697, theta = 2.76697, r - theta = -9.26235e-10
r = 2.76743, theta = 2.76743, r - theta = -4.93032e-10
r = 2.76789, theta = 2.76789, r - theta = -2.48468e-12
r = 2.76835, theta = 2.76835, r - theta = 4.81253e-10
r = 2.76882, theta = 2.76882, r - theta = 8.92188e-10
r = 2.76928, theta = 2.76928, r - theta = 1.16249e-09
r = 2.76974, theta = 2.76974, r - theta = 1.22247e-09
r = 2.7702, theta = 2.7702, r - theta = 1.00063e-09
r = 2.77066, theta = 2.77066, r - theta = 4.23603e-10
r = 2.77113, theta = 2.77113, r - theta = -4.63943e-10
r = 2.77159, theta = 2.77159, r - theta = -1.06886e-09
r = 2.77205, theta = 2.77205, r - theta = -1.34505e-09
r = 2.77251, theta = 2.77251, r - theta = -1.34813e-09
r = 2.77297, theta = 2.77297, r - theta = -1.1356e-09
r = 2.77343, theta = 2.77343, r - theta = -7.66753e-10
r = 2.7739, theta = 2.7739, r - theta = -3.02748e-10
r = 2.77436, theta = 2.77436, r - theta = 1.93432e-10
r = 2.77482, theta = 2.77482, r - theta = 6.56963e-10
r = 2.77528, theta = 2.77528, r - theta = 1.02119e-09
r = 2.77574, theta = 2.77574, r - theta = 1.21762e-09
r = 2.7762, theta = 2.7762, r - theta = 1.17594e-09
r = 2.77666, theta = 2.77666, r - theta = 8.23978e-10
r = 2.77713, theta = 2.77713, r - theta = 8.77547e-11
r = 2.77759, theta = 2.77759, r - theta = -7.42366e-10
r = 2.77805, theta = 2.77805, r - theta = -1.21407e-09
r = 2.77851, theta = 2.77851, r - theta = -1.37827e-09
r = 2.77897, theta = 2.77897, r - theta = -1.29129e-09
r = 2.77943, theta = 2.77943, r - theta = -1.01126e-09
r = 2.77989, theta = 2.77989, r - theta = -5.98158e-10
r = 2.78036, theta = 2.78036, r - theta = -1.13781e-10
r = 2.78082, theta = 2.78082, r - theta = 3.78238e-10
r = 2.78128, theta = 2.78128, r - theta = 8.12439e-10
r = 2.78174, theta = 2.78174, r - theta = 1.12154e-09
r = 2.7822, theta = 2.7822, r - theta = 1.23641e-09
r = 2.78266, theta = 2.78266, r - theta = 1.08612e-09
r = 2.78312, theta = 2.78312, r - theta = 5.97904e-10
r = 2.78358, theta = 2.78358, r - theta = -2.67204e-10
r = 2.78404, theta = 2.78404, r - theta = -9.59978e-10
r = 2.7845, theta = 2.7845, r - theta = -1.31086e-09
r = 2.78496, theta = 2.78496, r - theta = -1.37497e-09
r = 2.78543, theta = 2.78543, r - theta = -1.20925e-09
r = 2.78589, theta = 2.78589, r - theta = -8.72492e-10
r = 2.78635, theta = 2.78635, r - theta = -4.25286e-10
r = 2.78681, theta = 2.78681, r - theta = 6.99432e-11
r = 2.78727, theta = 2.78727, r - theta = 5.48952e-10
r = 2.78773, theta = 2.78773, r - theta = 9.45668e-10
r = 2.78819, theta = 2.78819, r - theta = 1.19221e-09
r = 2.78865, theta = 2.78865, r - theta = 1.21886e-09
r = 2.78911, theta = 2.78911, r - theta = 9.54091e-10
r = 2.78957, theta = 2.78957, r - theta = 3.24555e-10
r = 2.79003, theta = 2.79003, r - theta = -5.60896e-10
r = 2.79049, theta = 2.79049, r - theta = -1.12493e-09
r = 2.79095, theta = 2.79095, r - theta = -1.36663e-09
r = 2.79141, theta = 2.79141, r - theta = -1.34174e-09
r = 2.79187, theta = 2.79187, r - theta = -1.10782e-09
r = 2.79233, theta = 2.79233, r - theta = -7.24251e-10
r = 2.79279, theta = 2.79279, r - theta = -2.52231e-10
r = 2.79325, theta = 2.79325, r - theta = 2.45228e-10
r = 2.79371, theta = 2.79371, r - theta = 7.03296e-10
r = 2.79417, theta = 2.79417, r - theta = 1.05533e-09
r = 2.79463, theta = 2.79463, r - theta = 1.23286e-09
r = 2.79509, theta = 2.79509, r - theta = 1.16563e-09
r = 2.79555, theta = 2.79555, r - theta = 7.81531e-10
r = 2.79601, theta = 2.79601, r - theta = 6.67777e-12
r = 2.79647, theta = 2.79647, r - theta = -7.97864e-10
r = 2.79693, theta = 2.79693, r - theta = -1.2446e-09
r = 2.79739, theta = 2.79739, r - theta = -1.38802e-09
r = 2.79785, theta = 2.79785, r - theta = -1.28447e-09
r = 2.79831, theta = 2.79831, r - theta = -9.92077e-10
r = 2.79877, theta = 2.79877, r - theta = -5.708e-10
r = 2.79923, theta = 2.79923, r - theta = -8.24034e-11
r = 2.79969, theta = 2.79969, r - theta = 4.09539e-10
r = 2.80015, theta = 2.80015, r - theta = 8.39642e-10
r = 2.80061, theta = 2.80061, r - theta = 1.14071e-09
r = 2.80107, theta = 2.80107, r - theta = 1.24375e-09
r = 2.80153, theta = 2.80153, r - theta = 1.07794e-09
r = 2.80199, theta = 2.80199, r - theta = 5.70677e-10
r = 2.80245, theta = 2.80245, r - theta = -3.05367e-10
r = 2.80291, theta = 2.80291, r - theta = -9.85485e-10
r = 2.80337, theta = 2.80337, r - theta = -1.32567e-09
r = 2.80383, theta = 2.80383, r - theta = -1.38099e-09
r = 2.80429, theta = 2.80429, r - theta = -1.20835e-09
r = 2.80475, theta = 2.80475, r - theta = -8.66438e-10
r = 2.80521, theta = 2.80521, r - theta = -4.15747e-10
r = 2.80567, theta = 2.80567, r - theta = 8.14189e-11
r = 2.80613, theta = 2.80613, r - theta = 5.60952e-10
r = 2.80659, theta = 2.80659, r - theta = 9.56945e-10
r = 2.80704, theta = 2.80704, r - theta = 1.20168e-09
r = 2.8075, theta = 2.8075, r - theta = 1.22566e-09
r = 2.80796, theta = 2.80796, r - theta = 9.57548e-10
r = 2.80842, theta = 2.80842, r - theta = 3.24246e-10
r = 2.80888, theta = 2.80888, r - theta = -5.64153e-10
r = 2.80934, theta = 2.80934, r - theta = -1.13045e-09
r = 2.8098, theta = 2.8098, r - theta = -1.37414e-09
r = 2.81026, theta = 2.81026, r - theta = -1.35083e-09
r = 2.81072, theta = 2.81072, r - theta = -1.11795e-09
r = 2.81118, theta = 2.81118, r - theta = -7.34701e-10
r = 2.81163, theta = 2.81163, r - theta = -2.62102e-10
r = 2.81209, theta = 2.81209, r - theta = 2.37034e-10
r = 2.81255, theta = 2.81255, r - theta = 6.98101e-10
r = 2.81301, theta = 2.81301, r - theta = 1.05469e-09
r = 2.81347, theta = 2.81347, r - theta = 1.23861e-09
r = 2.81393, theta = 2.81393, r - theta = 1.17985e-09
r = 2.81439, theta = 2.81439, r - theta = 8.06637e-10
r = 2.81485, theta = 2.81485, r - theta = 4.53744e-11
r = 2.8153, theta = 2.8153, r - theta = -7.76355e-10
r = 2.81576, theta = 2.81576, r - theta = -1.23881e-09
r = 2.81622, theta = 2.81622, r - theta = -1.39539e-09
r = 2.81668, theta = 2.81668, r - theta = -1.30222e-09
r = 2.81714, theta = 2.81714, r - theta = -1.01722e-09
r = 2.8176, theta = 2.8176, r - theta = -6.00089e-10
r = 2.81806, theta = 2.81806, r - theta = -1.12331e-10
r = 2.81851, theta = 2.81851, r - theta = 3.82762e-10
r = 2.81897, theta = 2.81897, r - theta = 8.20112e-10
r = 2.81943, theta = 2.81943, r - theta = 1.13284e-09
r = 2.81989, theta = 2.81989, r - theta = 1.2523e-09
r = 2.82035, theta = 2.82035, r - theta = 1.10802e-09
r = 2.82081, theta = 2.82081, r - theta = 6.27782e-10
r = 2.82126, theta = 2.82126, r - theta = -2.35552e-10
r = 2.82172, theta = 2.82172, r - theta = -9.48056e-10
r = 2.82218, theta = 2.82218, r - theta = -1.31601e-09
r = 2.82264, theta = 2.82264, r - theta = -1.39422e-09
r = 2.8231, theta = 2.8231, r - theta = -1.23929e-09
r = 2.82355, theta = 2.82355, r - theta = -9.09593e-10
r = 2.82401, theta = 2.82401, r - theta = -4.65312e-10
r = 2.82447, theta = 2.82447, r - theta = 3.16001e-11
r = 2.82493, theta = 2.82493, r - theta = 5.174e-10
r = 2.82539, theta = 2.82539, r - theta = 9.26563e-10
r = 2.82584, theta = 2.82584, r - theta = 1.19178e-09
r = 2.8263, theta = 2.8263, r - theta = 1.24395e-09
r = 2.82676, theta = 2.82676, r - theta = 1.01221e-09
r = 2.82722, theta = 2.82722, r - theta = 4.23888e-10
r = 2.82767, theta = 2.82767, r - theta = -4.73058e-10
r = 2.82813, theta = 2.82813, r - theta = -1.08476e-09
r = 2.82859, theta = 2.82859, r - theta = -1.36694e-09
r = 2.82905, theta = 2.82905, r - theta = -1.37488e-09
r = 2.82951, theta = 2.82951, r - theta = -1.16563e-09
r = 2.82996, theta = 2.82996, r - theta = -7.97998e-10
r = 2.83042, theta = 2.83042, r - theta = -3.32602e-10
r = 2.83088, theta = 2.83088, r - theta = 1.68172e-10
r = 2.83134, theta = 2.83134, r - theta = 6.40162e-10
r = 2.83179, theta = 2.83179, r - theta = 1.01742e-09
r = 2.83225, theta = 2.83225, r - theta = 1.23224e-09
r = 2.83271, theta = 2.83271, r - theta = 1.2151e-09
r = 2.83316, theta = 2.83316, r - theta = 8.94747e-10
r = 2.83362, theta = 2.83362, r - theta = 1.98124e-10
r = 2.83408, theta = 2.83408, r - theta = -6.71763e-10
r = 2.83454, theta = 2.83454, r - theta = -1.19142e-09
r = 2.83499, theta = 2.83499, r - theta = -1.39598e-09
r = 2.83545, theta = 2.83545, r - theta = -1.34115e-09
r = 2.83591, theta = 2.83591, r - theta = -1.08437e-09
r = 2.83636, theta = 2.83636, r - theta = -6.84912e-10
r = 2.83682, theta = 2.83682, r - theta = -2.03767e-10
r = 2.83728, theta = 2.83728, r - theta = 2.96269e-10
r = 2.83774, theta = 2.83774, r - theta = 7.50637e-10
r = 2.83819, theta = 2.83819, r - theta = 1.09301e-09
r = 2.83865, theta = 2.83865, r - theta = 1.25527e-09
r = 2.83911, theta = 2.83911, r - theta = 1.16756e-09
r = 2.83956, theta = 2.83956, r - theta = 7.58234e-10
r = 2.84002, theta = 2.84002, r - theta = -4.52198e-11
r = 2.84048, theta = 2.84048, r - theta = -8.3668e-10
r = 2.84093, theta = 2.84093, r - theta = -1.2725e-09
r = 2.84139, theta = 2.84139, r - theta = -1.40701e-09
r = 2.84185, theta = 2.84185, r - theta = -1.2963e-09
r = 2.8423, theta = 2.8423, r - theta = -9.98221e-10
r = 2.84276, theta = 2.84276, r - theta = -5.72399e-10
r = 2.84322, theta = 2.84322, r - theta = -8.023e-11
r = 2.84367, theta = 2.84367, r - theta = 4.15125e-10
r = 2.84413, theta = 2.84413, r - theta = 8.48738e-10
r = 2.84459, theta = 2.84459, r - theta = 1.15392e-09
r = 2.84504, theta = 2.84504, r - theta = 1.26221e-09
r = 2.8455, theta = 2.8455, r - theta = 1.10338e-09
r = 2.84596, theta = 2.84596, r - theta = 6.05456e-10
r = 2.84641, theta = 2.84641, r - theta = -2.69763e-10
r = 2.84687, theta = 2.84687, r - theta = -9.72335e-10
r = 2.84733, theta = 2.84733, r - theta = -1.33201e-09
r = 2.84778, theta = 2.84778, r - theta = -1.40349e-09
r = 2.84824, theta = 2.84824, r - theta = -1.24324e-09
r = 2.84869, theta = 2.84869, r - theta = -9.09464e-10
r = 2.84915, theta = 2.84915, r - theta = -4.62162e-10
r = 2.84961, theta = 2.84961, r - theta = 3.69229e-11
r = 2.85006, theta = 2.85006, r - theta = 5.24286e-10
r = 2.85052, theta = 2.85052, r - theta = 9.34659e-10
r = 2.85097, theta = 2.85097, r - theta = 1.20101e-09
r = 2.85143, theta = 2.85143, r - theta = 1.25457e-09
r = 2.85189, theta = 2.85189, r - theta = 1.02478e-09
r = 2.85234, theta = 2.85234, r - theta = 4.39342e-10
r = 2.8528, theta = 2.8528, r - theta = -4.613e-10
r = 2.85325, theta = 2.85325, r - theta = -1.08281e-09
r = 2.85371, theta = 2.85371, r - theta = -1.37351e-09
r = 2.85417, theta = 2.85417, r - theta = -1.38846e-09
r = 2.85462, theta = 2.85462, r - theta = -1.18445e-09
r = 2.85508, theta = 2.85508, r - theta = -8.20051e-10
r = 2.85553, theta = 2.85553, r - theta = -3.55575e-10
r = 2.85599, theta = 2.85599, r - theta = 1.46906e-10
r = 2.85644, theta = 2.85644, r - theta = 6.23566e-10
r = 2.8569, theta = 2.8569, r - theta = 1.00883e-09
r = 2.85736, theta = 2.85736, r - theta = 1.23535e-09
r = 2.85781, theta = 2.85781, r - theta = 1.23406e-09
r = 2.85827, theta = 2.85827, r - theta = 9.34114e-10
r = 2.85872, theta = 2.85872, r - theta = 2.6292e-10
r = 2.85918, theta = 2.85918, r - theta = -6.23957e-10
r = 2.85963, theta = 2.85963, r - theta = -1.17175e-09
r = 2.86009, theta = 2.86009, r - theta = -1.40017e-09
r = 2.86054, theta = 2.86054, r - theta = -1.36458e-09
r = 2.861, theta = 2.861, r - theta = -1.1221e-09
r = 2.86145, theta = 2.86145, r - theta = -7.31609e-10
r = 2.86191, theta = 2.86191, r - theta = -2.53724e-10
r = 2.86237, theta = 2.86237, r - theta = 2.49189e-10
r = 2.86282, theta = 2.86282, r - theta = 7.13008e-10
r = 2.86328, theta = 2.86328, r - theta = 1.07187e-09
r = 2.86373, theta = 2.86373, r - theta = 1.25816e-09
r = 2.86419, theta = 2.86419, r - theta = 1.20252e-09
r = 2.86464, theta = 2.86464, r - theta = 8.33837e-10
r = 2.8651, theta = 2.8651, r - theta = 7.92708e-11
r = 2.86555, theta = 2.86555, r - theta = -7.61449e-10
r = 2.86601, theta = 2.86601, r - theta = -1.24243e-09
r = 2.86646, theta = 2.86646, r - theta = -1.41479e-09
r = 2.86692, theta = 2.86692, r - theta = -1.33419e-09
r = 2.86737, theta = 2.86737, r - theta = -1.05803e-09
r = 2.86783, theta = 2.86783, r - theta = -6.45486e-10
r = 2.86828, theta = 2.86828, r - theta = -1.5744e-10
r = 2.86874, theta = 2.86874, r - theta = 3.43463e-10
r = 2.86919, theta = 2.86919, r - theta = 7.92838e-10
r = 2.86964, theta = 2.86964, r - theta = 1.12456e-09
r = 2.8701, theta = 2.8701, r - theta = 1.27076e-09
r = 2.87055, theta = 2.87055, r - theta = 1.16184e-09
r = 2.87101, theta = 2.87101, r - theta = 7.26439e-10
r = 2.87146, theta = 2.87146, r - theta = -1.03692e-10
r = 2.87192, theta = 2.87192, r - theta = -8.77108e-10
r = 2.87237, theta = 2.87237, r - theta = -1.29776e-09
r = 2.87283, theta = 2.87283, r - theta = -1.41986e-09
r = 2.87328, theta = 2.87328, r - theta = -1.29932e-09
r = 2.87374, theta = 2.87374, r - theta = -9.93822e-10
r = 2.87419, theta = 2.87419, r - theta = -5.6278e-10
r = 2.87464, theta = 2.87464, r - theta = -6.73426e-11
r = 2.8751, theta = 2.8751, r - theta = 4.29604e-10
r = 2.87555, theta = 2.87555, r - theta = 8.63437e-10
r = 2.87601, theta = 2.87601, r - theta = 1.1678e-09
r = 2.87646, theta = 2.87646, r - theta = 1.27459e-09
r = 2.87692, theta = 2.87692, r - theta = 1.11399e-09
r = 2.87737, theta = 2.87737, r - theta = 6.14423e-10
r = 2.87782, theta = 2.87782, r - theta = -2.63843e-10
r = 2.87828, theta = 2.87828, r - theta = -9.73911e-10
r = 2.87873, theta = 2.87873, r - theta = -1.34033e-09
r = 2.87919, theta = 2.87919, r - theta = -1.41754e-09
r = 2.87964, theta = 2.87964, r - theta = -1.26172e-09
r = 2.88009, theta = 2.88009, r - theta = -9.30787e-10
r = 2.88055, theta = 2.88055, r - theta = -4.84376e-10
r = 2.881, theta = 2.881, r - theta = 1.61351e-11
r = 2.88146, theta = 2.88146, r - theta = 5.07642e-10
r = 2.88191, theta = 2.88191, r - theta = 9.25305e-10
r = 2.88236, theta = 2.88236, r - theta = 1.20256e-09
r = 2.88282, theta = 2.88282, r - theta = 1.27111e-09
r = 2.88327, theta = 2.88327, r - theta = 1.06092e-09
r = 2.88373, theta = 2.88373, r - theta = 5.00259e-10
r = 2.88418, theta = 2.88418, r - theta = -4.01386e-10
r = 2.88463, theta = 2.88463, r - theta = -1.05451e-09
r = 2.88509, theta = 2.88509, r - theta = -1.37241e-09
r = 2.88554, theta = 2.88554, r - theta = -1.40974e-09
r = 2.88599, theta = 2.88599, r - theta = -1.22291e-09
r = 2.88645, theta = 2.88645, r - theta = -8.7004e-10
r = 2.8869, theta = 2.8869, r - theta = -4.10973e-10
r = 2.88735, theta = 2.88735, r - theta = 9.27143e-11
r = 2.88781, theta = 2.88781, r - theta = 5.77721e-10
r = 2.88826, theta = 2.88826, r - theta = 9.79024e-10
r = 2.88871, theta = 2.88871, r - theta = 1.22987e-09
r = 2.88917, theta = 2.88917, r - theta = 1.26179e-09
r = 2.88962, theta = 2.88962, r - theta = 1.00459e-09
r = 2.89007, theta = 2.89007, r - theta = 3.86355e-10
r = 2.89053, theta = 2.89053, r - theta = -5.18997e-10
r = 2.89098, theta = 2.89098, r - theta = -1.12125e-09
r = 2.89143, theta = 2.89143, r - theta = -1.396e-09
r = 2.89189, theta = 2.89189, r - theta = -1.39811e-09
r = 2.89234, theta = 2.89234, r - theta = -1.18417e-09
r = 2.89279, theta = 2.89279, r - theta = -8.12498e-10
r = 2.89325, theta = 2.89325, r - theta = -3.4311e-10
r = 2.8937, theta = 2.8937, r - theta = 1.6224e-10
r = 2.89415, theta = 2.89415, r - theta = 6.40083e-10
r = 2.89461, theta = 2.89461, r - theta = 1.02523e-09
r = 2.89506, theta = 2.89506, r - theta = 1.25078e-09
r = 2.89551, theta = 2.89551, r - theta = 1.24811e-09
r = 2.89596, theta = 2.89596, r - theta = 9.46878e-10
r = 2.89642, theta = 2.89642, r - theta = 2.75024e-10
r = 2.89687, theta = 2.89687, r - theta = -6.19047e-10
r = 2.89732, theta = 2.89732, r - theta = -1.17618e-09
r = 2.89778, theta = 2.89778, r - theta = -1.41284e-09
r = 2.89823, theta = 2.89823, r - theta = -1.38406e-09
r = 2.89868, theta = 2.89868, r - theta = -1.1466e-09
r = 2.89913, theta = 2.89913, r - theta = -7.58915e-10
r = 2.89959, theta = 2.89959, r - theta = -2.81197e-10
r = 2.90004, theta = 2.90004, r - theta = 2.24655e-10
r = 2.90049, theta = 2.90049, r - theta = 6.9503e-10
r = 2.90094, theta = 2.90094, r - theta = 1.0646e-09
r = 2.9014, theta = 2.9014, r - theta = 1.26634e-09
r = 2.90185, theta = 2.90185, r - theta = 1.23148e-09
r = 2.9023, theta = 2.9023, r - theta = 8.89577e-10
r = 2.90275, theta = 2.90275, r - theta = 1.68452e-10
r = 2.90321, theta = 2.90321, r - theta = -7.03622e-10
r = 2.90366, theta = 2.90366, r - theta = -1.22113e-09
r = 2.90411, theta = 2.90411, r - theta = -1.42447e-09
r = 2.90456, theta = 2.90456, r - theta = -1.36883e-09
r = 2.90501, theta = 2.90501, r - theta = -1.11111e-09
r = 2.90547, theta = 2.90547, r - theta = -7.09905e-10
r = 2.90592, theta = 2.90592, r - theta = -2.25536e-10
r = 2.90637, theta = 2.90637, r - theta = 2.79978e-10
r = 2.90682, theta = 2.90682, r - theta = 7.42903e-10
r = 2.90728, theta = 2.90728, r - theta = 1.0978e-09
r = 2.90773, theta = 2.90773, r - theta = 1.27753e-09
r = 2.90818, theta = 2.90818, r - theta = 1.21324e-09
r = 2.90863, theta = 2.90863, r - theta = 8.34377e-10
r = 2.90908, theta = 2.90908, r - theta = 6.86748e-11
r = 2.90954, theta = 2.90954, r - theta = -7.74532e-10
r = 2.90999, theta = 2.90999, r - theta = -1.25764e-09
r = 2.91044, theta = 2.91044, r - theta = -1.43217e-09
r = 2.91089, theta = 2.91089, r - theta = -1.35345e-09
r = 2.91134, theta = 2.91134, r - theta = -1.07847e-09
r = 2.91179, theta = 2.91179, r - theta = -6.6596e-10
r = 2.91225, theta = 2.91225, r - theta = -1.76336e-10
r = 2.9127, theta = 2.9127, r - theta = 3.28281e-10
r = 2.91315, theta = 2.91315, r - theta = 7.84064e-10
r = 2.9136, theta = 2.9136, r - theta = 1.12549e-09
r = 2.91405, theta = 2.91405, r - theta = 1.28533e-09
r = 2.9145, theta = 2.9145, r - theta = 1.19465e-09
r = 2.91496, theta = 2.91496, r - theta = 7.82836e-10
r = 2.91541, theta = 2.91541, r - theta = -2.2236e-11
r = 2.91586, theta = 2.91586, r - theta = -8.33342e-10
r = 2.91631, theta = 2.91631, r - theta = -1.28705e-09
r = 2.91676, theta = 2.91676, r - theta = -1.43706e-09
r = 2.91721, theta = 2.91721, r - theta = -1.33878e-09
r = 2.91767, theta = 2.91767, r - theta = -1.04932e-09
r = 2.91812, theta = 2.91812, r - theta = -6.27474e-10
r = 2.91857, theta = 2.91857, r - theta = -1.33746e-10
r = 2.91902, theta = 2.91902, r - theta = 3.69667e-10
r = 2.91947, theta = 2.91947, r - theta = 8.18869e-10
r = 2.91992, theta = 2.91992, r - theta = 1.14827e-09
r = 2.92037, theta = 2.92037, r - theta = 1.29059e-09
r = 2.92082, theta = 2.92082, r - theta = 1.17684e-09
r = 2.92127, theta = 2.92127, r - theta = 7.36349e-10
r = 2.92173, theta = 2.92173, r - theta = -9.89298e-11
r = 2.92218, theta = 2.92218, r - theta = -8.81377e-10
r = 2.92263, theta = 2.92263, r - theta = -1.31049e-09
r = 2.92308, theta = 2.92308, r - theta = -1.44006e-09
r = 2.92353, theta = 2.92353, r - theta = -1.32557e-09
r = 2.92398, theta = 2.92398, r - theta = -1.02418e-09
r = 2.92443, theta = 2.92443, r - theta = -5.94754e-10
r = 2.92488, theta = 2.92488, r - theta = -9.78595e-11
r = 2.92533, theta = 2.92533, r - theta = 4.04258e-10
r = 2.92578, theta = 2.92578, r - theta = 8.47659e-10
r = 2.92624, theta = 2.92624, r - theta = 1.16671e-09
r = 2.92669, theta = 2.92669, r - theta = 1.2941e-09
r = 2.92714, theta = 2.92714, r - theta = 1.16081e-09
r = 2.92759, theta = 2.92759, r - theta = 6.96147e-10
r = 2.92804, theta = 2.92804, r - theta = -1.60604e-10
r = 2.92849, theta = 2.92849, r - theta = -9.19739e-10
r = 2.92894, theta = 2.92894, r - theta = -1.32891e-09
r = 2.92939, theta = 2.92939, r - theta = -1.44195e-09
r = 2.92984, theta = 2.92984, r - theta = -1.31439e-09
r = 2.93029, theta = 2.93029, r - theta = -1.00345e-09
r = 2.93074, theta = 2.93074, r - theta = -5.68036e-10
r = 2.93119, theta = 2.93119, r - theta = -6.87352e-11
r = 2.93164, theta = 2.93164, r - theta = 4.32177e-10
r = 2.93209, theta = 2.93209, r - theta = 8.70738e-10
r = 2.93254, theta = 2.93254, r - theta = 1.1813e-09
r = 2.93299, theta = 2.93299, r - theta = 1.29653e-09
r = 2.93344, theta = 2.93344, r - theta = 1.14742e-09
r = 2.93389, theta = 2.93389, r - theta = 6.63269e-10
r = 2.93434, theta = 2.93434, r - theta = -2.08288e-10
r = 2.93479, theta = 2.93479, r - theta = -9.49315e-10
r = 2.93524, theta = 2.93524, r - theta = -1.34303e-09
r = 2.93569, theta = 2.93569, r - theta = -1.44332e-09
r = 2.93614, theta = 2.93614, r - theta = -1.30572e-09
r = 2.93659, theta = 2.93659, r - theta = -9.87471e-10
r = 2.93704, theta = 2.93704, r - theta = -5.475e-10
r = 2.93749, theta = 2.93749, r - theta = -4.64033e-11
r = 2.93794, theta = 2.93794, r - theta = 4.53539e-10
r = 2.93839, theta = 2.93839, r - theta = 8.88364e-10
r = 2.93884, theta = 2.93884, r - theta = 1.19244e-09
r = 2.93929, theta = 2.93929, r - theta = 1.29844e-09
r = 2.93974, theta = 2.93974, r - theta = 1.13737e-09
r = 2.94019, theta = 2.94019, r - theta = 6.38556e-10
r = 2.94064, theta = 2.94064, r - theta = -2.42776e-10
r = 2.94109, theta = 2.94109, r - theta = -9.70794e-10
r = 2.94154, theta = 2.94154, r - theta = -1.35345e-09
r = 2.94199, theta = 2.94199, r - theta = -1.44464e-09
r = 2.94244, theta = 2.94244, r - theta = -1.2999e-09
r = 2.94289, theta = 2.94289, r - theta = -9.76474e-10
r = 2.94334, theta = 2.94334, r - theta = -5.33275e-10
r = 2.94379, theta = 2.94379, r - theta = -3.08851e-11
r = 2.94424, theta = 2.94424, r - theta = 4.68433e-10
r = 2.94469, theta = 2.94469, r - theta = 9.00744e-10
r = 2.94514, theta = 2.94514, r - theta = 1.20044e-09
r = 2.94559, theta = 2.94559, r - theta = 1.30024e-09
r = 2.94604, theta = 2.94604, r - theta = 1.13118e-09
r = 2.94649, theta = 2.94649, r - theta = 6.22643e-10
r = 2.94694, theta = 2.94694, r - theta = -2.6464e-10
r = 2.94739, theta = 2.94739, r - theta = -9.84667e-10
r = 2.94784, theta = 2.94784, r - theta = -1.36058e-09
r = 2.94829, theta = 2.94829, r - theta = -1.44624e-09
r = 2.94873, theta = 2.94873, r - theta = -1.29719e-09
r = 2.94918, theta = 2.94918, r - theta = -9.70632e-10
r = 2.94963, theta = 2.94963, r - theta = -5.25448e-10
r = 2.95008, theta = 2.95008, r - theta = -2.21898e-11
r = 2.95053, theta = 2.95053, r - theta = 4.76928e-10
r = 2.95098, theta = 2.95098, r - theta = 9.08015e-10
r = 2.95143, theta = 2.95143, r - theta = 1.20552e-09
r = 2.95188, theta = 2.95188, r - theta = 1.30221e-09
r = 2.95233, theta = 2.95233, r - theta = 1.12921e-09
r = 2.95278, theta = 2.95278, r - theta = 6.15943e-10
r = 2.95322, theta = 2.95322, r - theta = -2.74238e-10
r = 2.95367, theta = 2.95367, r - theta = -9.91235e-10
r = 2.95412, theta = 2.95412, r - theta = -1.36465e-09
r = 2.95457, theta = 2.95457, r - theta = -1.44832e-09
r = 2.95502, theta = 2.95502, r - theta = -1.29773e-09
r = 2.95547, theta = 2.95547, r - theta = -9.7004e-10
r = 2.95592, theta = 2.95592, r - theta = -5.24076e-10
r = 2.95637, theta = 2.95637, r - theta = -2.03264e-11
r = 2.95682, theta = 2.95682, r - theta = 4.79055e-10
r = 2.95726, theta = 2.95726, r - theta = 9.10251e-10
r = 2.95771, theta = 2.95771, r - theta = 1.20778e-09
r = 2.95816, theta = 2.95816, r - theta = 1.30451e-09
r = 2.95861, theta = 2.95861, r - theta = 1.13162e-09
r = 2.95906, theta = 2.95906, r - theta = 6.18657e-10
r = 2.95951, theta = 2.95951, r - theta = -2.71707e-10
r = 2.95996, theta = 2.95996, r - theta = -9.90612e-10
r = 2.9604, theta = 2.9604, r - theta = -1.36577e-09
r = 2.96085, theta = 2.96085, r - theta = -1.45095e-09
r = 2.9613, theta = 2.9613, r - theta = -1.30158e-09
r = 2.96175, theta = 2.96175, r - theta = -9.74736e-10
r = 2.9622, theta = 2.9622, r - theta = -5.29176e-10
r = 2.96265, theta = 2.96265, r - theta = -2.53064e-11
r = 2.96309, theta = 2.96309, r - theta = 4.74812e-10
r = 2.96354, theta = 2.96354, r - theta = 9.07451e-10
r = 2.96399, theta = 2.96399, r - theta = 1.20723e-09
r = 2.96444, theta = 2.96444, r - theta = 1.30712e-09
r = 2.96489, theta = 2.96489, r - theta = 1.13841e-09
r = 2.96533, theta = 2.96533, r - theta = 6.30758e-10
r = 2.96578, theta = 2.96578, r - theta = -2.56981e-10
r = 2.96623, theta = 2.96623, r - theta = -9.82734e-10
r = 2.96668, theta = 2.96668, r - theta = -1.36387e-09
r = 2.96713, theta = 2.96713, r - theta = -1.45408e-09
r = 2.96757, theta = 2.96757, r - theta = -1.30868e-09
r = 2.96802, theta = 2.96802, r - theta = -9.84688e-10
r = 2.96847, theta = 2.96847, r - theta = -5.40739e-10
r = 2.96892, theta = 2.96892, r - theta = -3.71378e-11
r = 2.96937, theta = 2.96937, r - theta = 4.64161e-10
r = 2.96981, theta = 2.96981, r - theta = 8.99548e-10
r = 2.97026, theta = 2.97026, r - theta = 1.20376e-09
r = 2.97071, theta = 2.97071, r - theta = 1.3099e-09
r = 2.97116, theta = 2.97116, r - theta = 1.14938e-09
r = 2.9716, theta = 2.9716, r - theta = 6.52004e-10
r = 2.97205, theta = 2.97205, r - theta = -2.29775e-10
r = 2.9725, theta = 2.9725, r - theta = -9.67346e-10
r = 2.97295, theta = 2.97295, r - theta = -1.35873e-09
r = 2.97339, theta = 2.97339, r - theta = -1.45751e-09
r = 2.97384, theta = 2.97384, r - theta = -1.31891e-09
r = 2.97429, theta = 2.97429, r - theta = -9.99802e-10
r = 2.97474, theta = 2.97474, r - theta = -5.58719e-10
r = 2.97518, theta = 2.97518, r - theta = -5.58336e-11
r = 2.97563, theta = 2.97563, r - theta = 4.47031e-10
r = 2.97608, theta = 2.97608, r - theta = 8.86405e-10
r = 2.97653, theta = 2.97653, r - theta = 1.19717e-09
r = 2.97697, theta = 2.97697, r - theta = 1.31256e-09
r = 2.97742, theta = 2.97742, r - theta = 1.16417e-09
r = 2.97787, theta = 2.97787, r - theta = 6.81935e-10
r = 2.97832, theta = 2.97832, r - theta = -1.89599e-10
r = 2.97876, theta = 2.97876, r - theta = -9.44012e-10
r = 2.97921, theta = 2.97921, r - theta = -1.34998e-09
r = 2.97966, theta = 2.97966, r - theta = -1.46095e-09
r = 2.9801, theta = 2.9801, r - theta = -1.33201e-09
r = 2.98055, theta = 2.98055, r - theta = -1.01992e-09
r = 2.981, theta = 2.981, r - theta = -5.83037e-10
r = 2.98145, theta = 2.98145, r - theta = -8.14016e-11
r = 2.98189, theta = 2.98189, r - theta = 4.23321e-10
r = 2.98234, theta = 2.98234, r - theta = 8.67819e-10
r = 2.98279, theta = 2.98279, r - theta = 1.18714e-09
r = 2.98323, theta = 2.98323, r - theta = 1.31469e-09
r = 2.98368, theta = 2.98368, r - theta = 1.18223e-09
r = 2.98413, theta = 2.98413, r - theta = 7.19884e-10
r = 2.98457, theta = 2.98457, r - theta = -1.3574e-10
r = 2.98502, theta = 2.98502, r - theta = -9.12102e-10
r = 2.98547, theta = 2.98547, r - theta = -1.33707e-09
r = 2.98591, theta = 2.98591, r - theta = -1.46394e-09
r = 2.98636, theta = 2.98636, r - theta = -1.34766e-09
r = 2.98681, theta = 2.98681, r - theta = -1.04479e-09
r = 2.98725, theta = 2.98725, r - theta = -6.13568e-10
r = 2.9877, theta = 2.9877, r - theta = -1.13836e-10
r = 2.98815, theta = 2.98815, r - theta = 3.92907e-10
r = 2.98859, theta = 2.98859, r - theta = 8.43533e-10
r = 2.98904, theta = 2.98904, r - theta = 1.17327e-09
r = 2.98949, theta = 2.98949, r - theta = 1.31573e-09
r = 2.98993, theta = 2.98993, r - theta = 1.20285e-09
r = 2.99038, theta = 2.99038, r - theta = 7.64981e-10
r = 2.99083, theta = 2.99083, r - theta = -6.72657e-11
r = 2.99127, theta = 2.99127, r - theta = -8.70798e-10
r = 2.99172, theta = 2.99172, r - theta = -1.3193e-09
r = 2.99216, theta = 2.99216, r - theta = -1.46591e-09
r = 2.99261, theta = 2.99261, r - theta = -1.36538e-09
r = 2.99306, theta = 2.99306, r - theta = -1.07412e-09
r = 2.9935, theta = 2.9935, r - theta = -6.50139e-10
r = 2.99395, theta = 2.99395, r - theta = -1.53118e-10
r = 2.9944, theta = 2.9944, r - theta = 3.55652e-10
r = 2.99484, theta = 2.99484, r - theta = 8.13244e-10
r = 2.99529, theta = 2.99529, r - theta = 1.1551e-09
r = 2.99573, theta = 2.99573, r - theta = 1.31502e-09
r = 2.99618, theta = 2.99618, r - theta = 1.2252e-09
r = 2.99663, theta = 2.99663, r - theta = 8.1617e-10
r = 2.99707, theta = 2.99707, r - theta = 1.68656e-11
r = 2.99752, theta = 2.99752, r - theta = -8.19071e-10
r = 2.99796, theta = 2.99796, r - theta = -1.2958e-09
r = 2.99841, theta = 2.99841, r - theta = -1.46613e-09
r = 2.99886, theta = 2.99886, r - theta = -1.38462e-09
r = 2.9993, theta = 2.9993, r - theta = -1.10748e-09
r = 2.99975, theta = 2.99975, r - theta = -6.9252e-10
r = 3.00019, theta = 3.00019, r - theta = -1.99196e-10
r = 3.00064, theta = 3.00064, r - theta = 3.11415e-10
r = 3.00108, theta = 3.00108, r - theta = 7.7661e-10
r = 3.00153, theta = 3.00153, r - theta = 1.13206e-09
r = 3.00198, theta = 3.00198, r - theta = 1.3118e-09
r = 3.00242, theta = 3.00242, r - theta = 1.24825e-09
r = 3.00287, theta = 3.00287, r - theta = 8.72217e-10
r = 3.00331, theta = 3.00331, r - theta = 1.12856e-10
r = 3.00376, theta = 3.00376, r - theta = -7.5569e-10
r = 3.0042, theta = 3.0042, r - theta = -1.26548e-09
r = 3.00465, theta = 3.00465, r - theta = -1.46369e-09
r = 3.00509, theta = 3.00509, r - theta = -1.40467e-09
r = 3.00554, theta = 3.00554, r - theta = -1.14437e-09
r = 3.00598, theta = 3.00598, r - theta = -7.40406e-10
r = 3.00643, theta = 3.00643, r - theta = -2.51981e-10
r = 3.00688, theta = 3.00688, r - theta = 2.60063e-10
r = 3.00732, theta = 3.00732, r - theta = 7.33266e-10
r = 3.00777, theta = 3.00777, r - theta = 1.10355e-09
r = 3.00821, theta = 3.00821, r - theta = 1.3052e-09
r = 3.00866, theta = 3.00866, r - theta = 1.27091e-09
r = 3.0091, theta = 3.0091, r - theta = 9.31728e-10
r = 3.00955, theta = 3.00955, r - theta = 2.17099e-10
r = 3.00999, theta = 3.00999, r - theta = -6.79198e-10
r = 3.01044, theta = 3.01044, r - theta = -1.2271e-09
r = 3.01088, theta = 3.01088, r - theta = -1.45756e-09
r = 3.01133, theta = 3.01133, r - theta = -1.42468e-09
r = 3.01177, theta = 3.01177, r - theta = -1.18418e-09
r = 3.01222, theta = 3.01222, r - theta = -7.9341e-10
r = 3.01266, theta = 3.01266, r - theta = -3.11327e-10
r = 3.01311, theta = 3.01311, r - theta = 2.01489e-10
r = 3.01355, theta = 3.01355, r - theta = 6.82842e-10
r = 3.014, theta = 3.014, r - theta = 1.06892e-09
r = 3.01444, theta = 3.01444, r - theta = 1.2943e-09
r = 3.01489, theta = 3.01489, r - theta = 1.29194e-09
r = 3.01533, theta = 3.01533, r - theta = 9.93175e-10
r = 3.01578, theta = 3.01578, r - theta = 3.27744e-10
r = 3.01622, theta = 3.01622, r - theta = -5.87902e-10
r = 3.01666, theta = 3.01666, r - theta = -1.17918e-09
r = 3.01711, theta = 3.01711, r - theta = -1.44649e-09
r = 3.01755, theta = 3.01755, r - theta = -1.44366e-09
r = 3.018, theta = 3.018, r - theta = -1.22615e-09
r = 3.01844, theta = 3.01844, r - theta = -8.51041e-10
r = 3.01889, theta = 3.01889, r - theta = -3.77018e-10
r = 3.01933, theta = 3.01933, r - theta = 1.35624e-10
r = 3.01978, theta = 3.01978, r - theta = 6.24976e-10
r = 3.02022, theta = 3.02022, r - theta = 1.02752e-09
r = 3.02067, theta = 3.02067, r - theta = 1.27811e-09
r = 3.02111, theta = 3.02111, r - theta = 1.31002e-09
r = 3.02155, theta = 3.02155, r - theta = 1.0549e-09
r = 3.022, theta = 3.022, r - theta = 4.42785e-10
r = 3.02244, theta = 3.02244, r - theta = -4.79866e-10
r = 3.02289, theta = 3.02289, r - theta = -1.12004e-09
r = 3.02333, theta = 3.02333, r - theta = -1.42903e-09
r = 3.02378, theta = 3.02378, r - theta = -1.46043e-09
r = 3.02422, theta = 3.02422, r - theta = -1.26939e-09
r = 3.02466, theta = 3.02466, r - theta = -9.12699e-10
r = 3.02511, theta = 3.02511, r - theta = -4.48749e-10
r = 3.02555, theta = 3.02555, r - theta = 6.24643e-11
r = 3.026, theta = 3.026, r - theta = 5.59339e-10
r = 3.02644, theta = 3.02644, r - theta = 9.78665e-10
r = 3.02688, theta = 3.02688, r - theta = 1.25562e-09
r = 3.02733, theta = 3.02733, r - theta = 1.3238e-09
r = 3.02777, theta = 3.02777, r - theta = 1.11516e-09
r = 3.02822, theta = 3.02822, r - theta = 5.60087e-10
r = 3.02866, theta = 3.02866, r - theta = -3.52891e-10
r = 3.0291, theta = 3.0291, r - theta = -1.04772e-09
r = 3.02955, theta = 3.02955, r - theta = -1.40355e-09
r = 3.02999, theta = 3.02999, r - theta = -1.47363e-09
r = 3.03043, theta = 3.03043, r - theta = -1.31284e-09
r = 3.03088, theta = 3.03088, r - theta = -9.7764e-10
r = 3.03132, theta = 3.03132, r - theta = -5.26107e-10
r = 3.03177, theta = 3.03177, r - theta = -1.79177e-11
r = 3.03221, theta = 3.03221, r - theta = 4.85652e-10
r = 3.03265, theta = 3.03265, r - theta = 9.21721e-10
r = 3.0331, theta = 3.0331, r - theta = 1.22581e-09
r = 3.03354, theta = 3.03354, r - theta = 1.33184e-09
r = 3.03398, theta = 3.03398, r - theta = 1.17213e-09
r = 3.03443, theta = 3.03443, r - theta = 6.77407e-10
r = 3.03487, theta = 3.03487, r - theta = -2.04498e-10
r = 3.03531, theta = 3.03531, r - theta = -9.60065e-10
r = 3.03576, theta = 3.03576, r - theta = -1.36815e-09
r = 3.0362, theta = 3.0362, r - theta = -1.48171e-09
r = 3.03664, theta = 3.03664, r - theta = -1.35526e-09
r = 3.03709, theta = 3.03709, r - theta = -1.04497e-09
r = 3.03753, theta = 3.03753, r - theta = -6.08547e-10
r = 3.03797, theta = 3.03797, r - theta = -1.05346e-10
r = 3.03842, theta = 3.03842, r - theta = 4.03708e-10
r = 3.03886, theta = 3.03886, r - theta = 8.56085e-10
r = 3.0393, theta = 3.0393, r - theta = 1.18766e-09
r = 3.03975, theta = 3.03975, r - theta = 1.33272e-09
r = 3.04019, theta = 3.04019, r - theta = 1.22395e-09
r = 3.04063, theta = 3.04063, r - theta = 7.92428e-10
r = 3.04108, theta = 3.04108, r - theta = -3.191e-11
r = 3.04152, theta = 3.04152, r - theta = -8.54594e-10
r = 3.04196, theta = 3.04196, r - theta = -1.32071e-09
r = 3.0424, theta = 3.0424, r - theta = -1.48285e-09
r = 3.04285, theta = 3.04285, r - theta = -1.39522e-09
r = 3.04329, theta = 3.04329, r - theta = -1.1136e-09
r = 3.04373, theta = 3.04373, r - theta = -6.95375e-10
r = 3.04418, theta = 3.04418, r - theta = -1.9952e-10
r = 3.04462, theta = 3.04462, r - theta = 3.134e-10
r = 3.04506, theta = 3.04506, r - theta = 7.81229e-10
r = 3.0455, theta = 3.0455, r - theta = 1.14022e-09
r = 3.04595, theta = 3.04595, r - theta = 1.32502e-09
r = 3.04639, theta = 3.04639, r - theta = 1.26872e-09
r = 3.04683, theta = 3.04683, r - theta = 9.02782e-10
r = 3.04728, theta = 3.04728, r - theta = 1.57112e-10
r = 3.04772, theta = 3.04772, r - theta = -7.28555e-10
r = 3.04816, theta = 3.04816, r - theta = -1.2588e-09
r = 3.0486, theta = 3.0486, r - theta = -1.47502e-09
r = 3.04905, theta = 3.04905, r - theta = -1.43103e-09
r = 3.04949, theta = 3.04949, r - theta = -1.18226e-09
r = 3.04993, theta = 3.04993, r - theta = -7.85718e-10
r = 3.05037, theta = 3.05037, r - theta = -2.99984e-10
r = 3.05082, theta = 3.05082, r - theta = 2.1475e-10
r = 3.05126, theta = 3.05126, r - theta = 6.9672e-10
r = 3.0517, theta = 3.0517, r - theta = 1.08257e-09
r = 3.05214, theta = 3.05214, r - theta = 1.30736e-09
r = 3.05258, theta = 3.05258, r - theta = 1.30457e-09
r = 3.05303, theta = 3.05303, r - theta = 1.00609e-09
r = 3.05347, theta = 3.05347, r - theta = 3.42214e-10
r = 3.05391, theta = 3.05391, r - theta = -5.78877e-10
r = 3.05435, theta = 3.05435, r - theta = -1.17974e-09
r = 3.0548, theta = 3.0548, r - theta = -1.45589e-09
r = 3.05524, theta = 3.05524, r - theta = -1.46079e-09
r = 3.05568, theta = 3.05568, r - theta = -1.24945e-09
r = 3.05612, theta = 3.05612, r - theta = -8.78495e-10
r = 3.05656, theta = 3.05656, r - theta = -4.06108e-10
r = 3.05701, theta = 3.05701, r - theta = 1.07929e-10
r = 3.05745, theta = 3.05745, r - theta = 6.02257e-10
r = 3.05789, theta = 3.05789, r - theta = 1.01394e-09
r = 3.05833, theta = 3.05833, r - theta = 1.27845e-09
r = 3.05877, theta = 3.05877, r - theta = 1.32969e-09
r = 3.05922, theta = 3.05922, r - theta = 1.09997e-09
r = 3.05966, theta = 3.05966, r - theta = 5.20046e-10
r = 3.0601, theta = 3.0601, r - theta = -4.02148e-10
r = 3.06054, theta = 3.06054, r - theta = -1.08049e-09
r = 3.06098, theta = 3.06098, r - theta = -1.42286e-09
r = 3.06142, theta = 3.06142, r - theta = -1.4823e-09
r = 3.06187, theta = 3.06187, r - theta = -1.31341e-09
r = 3.06231, theta = 3.06231, r - theta = -9.72403e-10
r = 3.06275, theta = 3.06275, r - theta = -5.17051e-10
r = 3.06319, theta = 3.06319, r - theta = -6.70974e-12
r = 3.06363, theta = 3.06363, r - theta = 4.97694e-10
r = 3.06407, theta = 3.06407, r - theta = 9.3365e-10
r = 3.06452, theta = 3.06452, r - theta = 1.23707e-09
r = 3.06496, theta = 3.06496, r - theta = 1.34231e-09
r = 3.0654, theta = 3.0654, r - theta = 1.18212e-09
r = 3.06584, theta = 3.06584, r - theta = 6.87704e-10
r = 3.06628, theta = 3.06628, r - theta = -1.94595e-10
r = 3.06672, theta = 3.06672, r - theta = -9.57703e-10
r = 3.06716, theta = 3.06716, r - theta = -1.37298e-09
r = 3.0676, theta = 3.0676, r - theta = -1.49305e-09
r = 3.06805, theta = 3.06805, r - theta = -1.37209e-09
r = 3.06849, theta = 3.06849, r - theta = -1.06587e-09
r = 3.06893, theta = 3.06893, r - theta = -6.31739e-10
r = 3.06937, theta = 3.06937, r - theta = -1.28594e-10
r = 3.06981, theta = 3.06981, r - theta = 3.83074e-10
r = 3.07025, theta = 3.07025, r - theta = 8.41211e-10
r = 3.07069, theta = 3.07069, r - theta = 1.18219e-09
r = 3.07113, theta = 3.07113, r - theta = 1.34081e-09
r = 3.07158, theta = 3.07158, r - theta = 1.25031e-09
r = 3.07202, theta = 3.07202, r - theta = 8.42351e-10
r = 3.07246, theta = 3.07246, r - theta = 4.70171e-11
r = 3.0729, theta = 3.0729, r - theta = -8.07639e-10
r = 3.07334, theta = 3.07334, r - theta = -1.30297e-09
r = 3.07378, theta = 3.07378, r - theta = -1.49022e-09
r = 3.07422, theta = 3.07422, r - theta = -1.42315e-09
r = 3.07466, theta = 3.07466, r - theta = -1.15705e-09
r = 3.0751, theta = 3.0751, r - theta = -7.48828e-10
r = 3.07554, theta = 3.07554, r - theta = -2.56916e-10
r = 3.07598, theta = 3.07598, r - theta = 2.58662e-10
r = 3.07642, theta = 3.07642, r - theta = 7.36321e-10
r = 3.07687, theta = 3.07687, r - theta = 1.11291e-09
r = 3.07731, theta = 3.07731, r - theta = 1.32371e-09
r = 3.07775, theta = 3.07775, r - theta = 1.30244e-09
r = 3.07819, theta = 3.07819, r - theta = 9.81239e-10
r = 3.07863, theta = 3.07863, r - theta = 2.90707e-10
r = 3.07907, theta = 3.07907, r - theta = -6.26175e-10
r = 3.07951, theta = 3.07951, r - theta = -1.20915e-09
r = 3.07995, theta = 3.07995, r - theta = -1.47063e-09
r = 3.08039, theta = 3.08039, r - theta = -1.46389e-09
r = 3.08083, theta = 3.08083, r - theta = -1.24377e-09
r = 3.08127, theta = 3.08127, r - theta = -8.66677e-10
r = 3.08171, theta = 3.08171, r - theta = -3.90581e-10
r = 3.08215, theta = 3.08215, r - theta = 1.24982e-10
r = 3.08259, theta = 3.08259, r - theta = 6.18915e-10
r = 3.08303, theta = 3.08303, r - theta = 1.02856e-09
r = 3.08347, theta = 3.08347, r - theta = 1.2897e-09
r = 3.08391, theta = 3.08391, r - theta = 1.33655e-09
r = 3.08435, theta = 3.08435, r - theta = 1.10177e-09
r = 3.08479, theta = 3.08479, r - theta = 5.16454e-10
r = 3.08523, theta = 3.08523, r - theta = -4.08766e-10
r = 3.08567, theta = 3.08567, r - theta = -1.08748e-09
r = 3.08611, theta = 3.08611, r - theta = -1.43071e-09
r = 3.08655, theta = 3.08655, r - theta = -1.49127e-09
r = 3.08699, theta = 3.08699, r - theta = -1.3235e-09
r = 3.08743, theta = 3.08743, r - theta = -9.83312e-10
r = 3.08787, theta = 3.08787, r - theta = -5.28187e-10
r = 3.08831, theta = 3.08831, r - theta = -1.71543e-11
r = 3.08875, theta = 3.08875, r - theta = 4.89198e-10
r = 3.08919, theta = 3.08919, r - theta = 9.28723e-10
r = 3.08963, theta = 3.08963, r - theta = 1.23772e-09
r = 3.09007, theta = 3.09007, r - theta = 1.35092e-09
r = 3.09051, theta = 3.09051, r - theta = 1.20151e-09
r = 3.09095, theta = 3.09095, r - theta = 7.21125e-10
r = 3.09139, theta = 3.09139, r - theta = -1.50419e-10
r = 3.09183, theta = 3.09183, r - theta = -9.33433e-10
r = 3.09227, theta = 3.09227, r - theta = -1.36647e-09
r = 3.09271, theta = 3.09271, r - theta = -1.50182e-09
r = 3.09315, theta = 3.09315, r - theta = -1.39333e-09
r = 3.09359, theta = 3.09359, r - theta = -1.09639e-09
r = 3.09403, theta = 3.09403, r - theta = -6.67983e-10
r = 3.09447, theta = 3.09447, r - theta = -1.66603e-10
r = 3.09491, theta = 3.09491, r - theta = 3.47685e-10
r = 3.09535, theta = 3.09535, r - theta = 8.13262e-10
r = 3.09579, theta = 3.09579, r - theta = 1.16696e-09
r = 3.09623, theta = 3.09623, r - theta = 1.34405e-09
r = 3.09667, theta = 3.09667, r - theta = 1.27827e-09
r = 3.09711, theta = 3.09711, r - theta = 9.0179e-10
r = 3.09755, theta = 3.09755, r - theta = 1.45238e-10
r = 3.09799, theta = 3.09799, r - theta = -7.42058e-10
r = 3.09843, theta = 3.09843, r - theta = -1.27346e-09
r = 3.09886, theta = 3.09886, r - theta = -1.49165e-09
r = 3.0993, theta = 3.0993, r - theta = -1.44993e-09
r = 3.09974, theta = 3.09974, r - theta = -1.20319e-09
r = 3.10018, theta = 3.10018, r - theta = -8.07837e-10
r = 3.10062, theta = 3.10062, r - theta = -3.21857e-10
r = 3.10106, theta = 3.10106, r - theta = 1.95235e-10
r = 3.1015, theta = 3.1015, r - theta = 6.82369e-10
r = 3.10194, theta = 3.10194, r - theta = 1.07693e-09
r = 3.10238, theta = 3.10238, r - theta = 1.31474e-09
r = 3.10282, theta = 3.10282, r - theta = 1.33011e-09
r = 3.10326, theta = 3.10326, r - theta = 1.05576e-09
r = 3.10369, theta = 3.10369, r - theta = 4.22905e-10
r = 3.10413, theta = 3.10413, r - theta = -5.07895e-10
r = 3.10457, theta = 3.10457, r - theta = -1.14678e-09
r = 3.10501, theta = 3.10501, r - theta = -1.4564e-09
r = 3.10545, theta = 3.10545, r - theta = -1.48954e-09
r = 3.10589, theta = 3.10589, r - theta = -1.30051e-09
r = 3.10633, theta = 3.10633, r - theta = -9.45199e-10
r = 3.10677, theta = 3.10677, r - theta = -4.81009e-10
r = 3.10721, theta = 3.10721, r - theta = 3.30926e-11
r = 3.10764, theta = 3.10764, r - theta = 5.36603e-10
r = 3.10808, theta = 3.10808, r - theta = 9.67476e-10
r = 3.10852, theta = 3.10852, r - theta = 1.26212e-09
r = 3.10896, theta = 3.10896, r - theta = 1.3554e-09
r = 3.1094, theta = 3.1094, r - theta = 1.18065e-09
r = 3.10984, theta = 3.10984, r - theta = 6.69643e-10
r = 3.11028, theta = 3.11028, r - theta = -2.2496e-10
r = 3.11071, theta = 3.11071, r - theta = -9.80992e-10
r = 3.11115, theta = 3.11115, r - theta = -1.39123e-09
r = 3.11159, theta = 3.11159, r - theta = -1.50789e-09
r = 3.11203, theta = 3.11203, r - theta = -1.38473e-09
r = 3.11247, theta = 3.11247, r - theta = -1.07706e-09
r = 3.11291, theta = 3.11291, r - theta = -6.41709e-10
r = 3.11335, theta = 3.11335, r - theta = -1.37073e-10
r = 3.11378, theta = 3.11378, r - theta = 3.76934e-10
r = 3.11422, theta = 3.11422, r - theta = 8.38852e-10
r = 3.11466, theta = 3.11466, r - theta = 1.18568e-09
r = 3.1151, theta = 3.1151, r - theta = 1.35288e-09
r = 3.11554, theta = 3.11554, r - theta = 1.27439e-09
r = 3.11597, theta = 3.11597, r - theta = 8.82581e-10
r = 3.11641, theta = 3.11641, r - theta = 1.08318e-10
r = 3.11685, theta = 3.11685, r - theta = -7.70128e-10
r = 3.11729, theta = 3.11729, r - theta = -1.29074e-09
r = 3.11773, theta = 3.11773, r - theta = -1.5002e-09
r = 3.11817, theta = 3.11817, r - theta = -1.45168e-09
r = 3.1186, theta = 3.1186, r - theta = -1.19991e-09
r = 3.11904, theta = 3.11904, r - theta = -8.0113e-10
r = 3.11948, theta = 3.11948, r - theta = -3.13125e-10
r = 3.11992, theta = 3.11992, r - theta = 2.0478e-10
r = 3.12036, theta = 3.12036, r - theta = 6.91733e-10
r = 3.12079, theta = 3.12079, r - theta = 1.08534e-09
r = 3.12123, theta = 3.12123, r - theta = 1.32168e-09
r = 3.12167, theta = 3.12167, r - theta = 1.3353e-09
r = 3.12211, theta = 3.12211, r - theta = 1.05921e-09
r = 3.12254, theta = 3.12254, r - theta = 4.24892e-10
r = 3.12298, theta = 3.12298, r - theta = -5.0764e-10
r = 3.12342, theta = 3.12342, r - theta = -1.14898e-09
r = 3.12386, theta = 3.12386, r - theta = -1.46114e-09
r = 3.1243, theta = 3.1243, r - theta = -1.49669e-09
r = 3.12473, theta = 3.12473, r - theta = -1.30973e-09
r = 3.12517, theta = 3.12517, r - theta = -9.55921e-10
r = 3.12561, theta = 3.12561, r - theta = -4.92426e-10
r = 3.12605, theta = 3.12605, r - theta = 2.20517e-11
r = 3.12648, theta = 3.12648, r - theta = 5.27276e-10
r = 3.12692, theta = 3.12692, r - theta = 9.61484e-10
r = 3.12736, theta = 3.12736, r - theta = 1.26139e-09
r = 3.1278, theta = 3.1278, r - theta = 1.36216e-09
r = 3.12823, theta = 3.12823, r - theta = 1.19745e-09
r = 3.12867, theta = 3.12867, r - theta = 6.99389e-10
r = 3.12911, theta = 3.12911, r - theta = 3.14395e-06
r = 3.12967, theta = 3.12955, r - theta = 0.000118677
r = 3.13037, theta = 3.12999, r - theta = 0.00037263
r = 3.13117, theta = 3.13044, r - theta = 0.000726688
r = 3.13203, theta = 3.13089, r - theta = 0.00114251
r = 3.13292, theta = 3.13134, r - theta = 0.00158172
r = 3.13379, theta = 3.13178, r - theta = 0.00200591
r = 3.13461, theta = 3.13223, r - theta = 0.00237666
r = 3.13533, theta = 3.13267, r - theta = 0.0026555
r = 3.13592, theta = 3.13312, r - theta = 0.00280394
r = 3.13634, theta = 3.13355, r - theta = 0.00278347
r = 3.13654, theta = 3.13398, r - theta = 0.00255555
r = 3.13649, theta = 3.13441, r - theta = 0.0020816
r = 3.13615, theta = 3.13482, r - theta = 0.00132303
r = 3.13548, theta = 3.13523, r - theta = 0.000241233
)ex"));
}
}  // namespace ex_catmull_rom_example

// libs/math/example/cstdfloat_example.cpp: its additions to namespace boost/std, outside the case namespace
namespace boost
{
template <typename FPT>
const int max_digits10()
{
// Since max_digits10 is not defined (or wrong) on older systems, define a local max_digits10.
  // Usage:   int m = max_digits10<boost::float64_t>();
  const int m =
#if (defined BOOST_NO_CXX11_NUMERIC_LIMITS) || (_MSC_VER == 1600) // is wrongly 8 not 9 for VS2010.
  2 + std::numeric_limits<FPT>::digits * 3010/10000;
#else
  std::numeric_limits<FPT>::max_digits10;
#endif
  return m;
}
}
// libs/math/example/cstdfloat_example.cpp
namespace ex_cstdfloat_example {
//[cstdfloat_example_1
//] [/cstdfloat_example_1]


/*!
Function max_digits10
Returns maximum number of possibly significant decimal digits for a floating-point type FPT,
even for older compilers/standard libraries that
lack support for std::numeric_limits<FPT>::max_digits10,
when the Kahan formula 2 + binary_digits * 0.3010 is used instead.
Also provides the correct result for Visual Studio 2010
(where the value 8 provided for float is wrong).
*/
 // namespace boost

//`Define the Jahnke-Emden_lambda function.
//[cstdfloat_example_2
boost::float64_t jahnke_emden_lambda(boost::float64_t v, boost::float64_t x)
{
  const boost::float64_t gamma_v_plus_one = boost::math::tgamma(v + 1);
  const boost::float64_t x_half_pow_v     = std::pow(x /2, v);

  return gamma_v_plus_one * boost::math::cyl_bessel_j(x, v) / x_half_pow_v;
}
//] [/cstdfloat_example_2]

int example_main()
{
  ex_out.setf(std::ios::showpoint); // show all significant trailing zeros.

    long double p = 1.L;
  //ex_out.precision(std::numeric_limits<long double>::digits10);

  ex_out << "pi = "  << p << std::endl;

//[cstdfloat_example_3
//`Ensure that all possibly significant digits (17) including trailing zeros are shown.

  ex_out.precision(std::numeric_limits<boost::float64_t>::max_digits10);
  ex_out.setf(std::ios::showpoint); // Show trailing zeros.

  try
  { // Always use try'n'catch blocks to ensure any error messages are displayed.

  // Evaluate and display an evaluation of the Jahnke-Emden lambda function:
  boost::float64_t v = 1.;
  boost::float64_t x = 1.;
  ex_out << jahnke_emden_lambda(v, x) << std::endl; // 0.88010117148986700
//] [/cstdfloat_example_3]

  // We can show some evaluations with various precisions:
  { // float64_t
    for (int i = 0; i < 10; i++)
    {
      ex_out << std::setprecision(2) << boost::float64_t(i) << ' '
        << std::setprecision(std::numeric_limits<boost::float64_t>::max_digits10)
        << jahnke_emden_lambda(boost::float64_t(i), v) << std::endl; //
    }
  }
  { // floatmax_t = the maximum available on this platform.
    for (int i = 0; i < 10; i++)
    {
      ex_out << std::setprecision(2) << boost::floatmax_t(i) << ' '
        << std::setprecision(std::numeric_limits<boost::floatmax_t>::max_digits10)
        << jahnke_emden_lambda(boost::floatmax_t(i), v) << std::endl; //
    }
  }
  // Show the precision of long double (this might be 64, 80 or 128 bits).
  ex_out << "Floating-point type long double is available with:" << std::endl;
  ex_out << "  std::numeric_limits<long double>::digits10 == "
    << std::numeric_limits<long double>::digits10 << std::endl; // 18
  ex_out << "  std::numeric_limits<long double>::max_digits10 == "
    << std::numeric_limits<long double>::max_digits10 << std::endl; // 21
  long double p = boost::math::constants::pi<double>();
  ex_out.precision(std::numeric_limits<long double>::digits10);
  ex_out << "pi = "  << p << std::endl;

//[cstdfloat_constant_2
//`These allow floating-point [*constants of at least the specified width] to be declared:

  // Declare Archimedes' constant using float32_t with approximately 7 decimal digits of precision.
  static const boost::float32_t pi = BOOST_FLOAT32_C(3.1415926536);

  // Declare the Euler-gamma constant with approximately 15 decimal digits of precision.
  static const boost::float64_t euler =
     BOOST_FLOAT64_C(0.57721566490153286060651209008240243104216);

  // Declare the Golden Ratio constant with the maximum decimal digits of precision that the platform supports.
  static const boost::floatmax_t golden_ratio =
     BOOST_FLOATMAX_C(1.61803398874989484820458683436563811772);
//] [/cstdfloat_constant_2]

// http://www.boost.org/doc/libs/1_55_0/libs/multiprecision/doc/html/boost_multiprecision/tut/floats/float128.html
//[cstdfloat_constant_1
// Display the constant pi to the maximum available precision.
  boost::floatmax_t pi_max = boost::math::constants::pi<boost::floatmax_t>();
  ex_out.precision(std::numeric_limits<boost::floatmax_t>::digits10);
  ex_out << "Most precise pi = "  << pi_max << std::endl;
// If floatmax_t is float_128_t, then
// Most precise pi = 3.141592653589793238462643383279503
//] [/cstdfloat_constant_1]

// Test all the floating-point precisions in turn, and if they are available
// then display how many decimal digits of precision.
#ifdef BOOST_FLOAT16_C
  ex_out << "Floating-point type boost::float16_t is available." << std::endl;
#else
  ex_out << "Floating-point type boost::float16_t is NOT available." << std::endl;
#endif

#ifdef BOOST_FLOAT32_C
  ex_out << "Floating-point type boost::float32_t is available." << std::endl;
  ex_out << "  std::numeric_limits<boost::float32_t>::digits10 == "
    << std::numeric_limits<boost::float32_t>::digits10 << std::endl;
  ex_out << "  std::numeric_limits<boost::float32_t>::max_digits10 == "
    << std::numeric_limits<boost::float32_t>::max_digits10 << std::endl;
#else
  ex_out << "Floating-point type boost::float32_t is NOT available." << std::endl;
#endif

#ifdef BOOST_FLOAT64_C
  ex_out << "Floating-point type boost::float64_t is available." << std::endl;
    ex_out << "  std::numeric_limits<boost::float64_t>::digits10 == "
    << std::numeric_limits<boost::float64_t>::digits10 << std::endl;
  ex_out << "  std::numeric_limits<boost::float64_t>::max_digits10 == "
    << std::numeric_limits<boost::float64_t>::max_digits10 << std::endl;
#else
  ex_out << "Floating-point type boost::float64_t is NOT available." << std::endl;
#endif

#ifdef BOOST_FLOAT80_C
  ex_out << "Floating-point type boost::float80_t is available." << std::endl;
  ex_out << "  std::numeric_limits<boost::float80_t>::digits10 == "
    << std::numeric_limits<boost::float80_t>::digits10 << std::endl;
  ex_out << "  std::numeric_limits<boost::float80_t>::max_digits10 == "
    << std::numeric_limits<boost::float80_t>::max_digits10 << std::endl;
#else
  ex_out << "Floating-point type boost::float80_t is NOT available." << std::endl;
#endif

#ifdef BOOST_FLOAT128_C
  ex_out << "Floating-point type boost::float128_t is available." << std::endl;
    ex_out << "  std::numeric_limits<boost::float128_t>::digits10 == "
    << std::numeric_limits<boost::float128_t>::digits10 << std::endl;
  ex_out << "  std::numeric_limits<boost::float128_t>::max_digits10 == "
    << std::numeric_limits<boost::float128_t>::max_digits10 << std::endl;
#else
  ex_out << "Floating-point type boost::float128_t is NOT available." << std::endl;
#endif

// Show some constants at a precision depending on the available type(s).
#ifdef BOOST_FLOAT16_C
  ex_out.precision(boost::max_digits10<boost::float16_t>()); // Show all significant decimal digits,
  ex_out.setf(std::ios::showpoint); // including all significant trailing zeros.

  ex_out << "BOOST_FLOAT16_C(123.456789012345678901234567890) = "
    << BOOST_FLOAT16_C(123.456789012345678901234567890) << std::endl;
  // BOOST_FLOAT16_C(123.456789012345678901234567890) = 123.45678901234568
#endif

//[floatmax_widths_1
#ifdef BOOST_FLOAT32_C
  ex_out.precision(boost::max_digits10<boost::float32_t>()); // Show all significant decimal digits,
  ex_out.setf(std::ios::showpoint); // including all significant trailing zeros.
  ex_out << "BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = "
    << BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) << std::endl;
  //   BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = 123.456787
#endif
//] [/floatmax_widths_1]

#ifdef BOOST_FLOAT64_C
  ex_out.precision(boost::max_digits10<boost::float64_t>()); // Show all significant decimal digits,
  ex_out.setf(std::ios::showpoint); // including all significant trailing zeros.
  ex_out << "BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = "
    << BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) << std::endl;
  // BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = 123.45678901234568
#endif

#ifdef BOOST_FLOAT80_C
  ex_out.precision(boost::max_digits10<boost::float80_t>()); // Show all significant decimal digits,
  ex_out.setf(std::ios::showpoint); // including all significant trailing zeros.
  ex_out << "BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) = "
    << BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) << std::endl;
  // BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) = 123.456789012345678903
#endif

#ifdef BOOST_FLOAT128_C
  ex_out.precision(boost::max_digits10<boost::float128_t>()); // Show all significant decimal digits,
  ex_out.setf(std::ios::showpoint); // including all significant trailing zeros.
  ex_out << "BOOST_FLOAT128_C(123.4567890123456789012345678901234567890) = "
    << BOOST_FLOAT128_C(123.4567890123456789012345678901234567890) << std::endl;
  // BOOST_FLOAT128_C(123.4567890123456789012345678901234567890) = 123.456789012345678901234567890123453
#endif

/*
//[floatmax_widths_2
BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = 123.456787
BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = 123.45678901234568
BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) = 123.456789012345678903
BOOST_FLOAT128_C(123.4567890123456789012345678901234567890) = 123.456789012345678901234567890123453
//] [/floatmax_widths_2]
*/

// Display the precisions available for floatmax_t

#ifdef BOOST_FLOATMAX_C
  BOOST_MATH_ASSERT(std::numeric_limits<boost::floatmax_t>::is_specialized == true);
  BOOST_MATH_ASSERT(std::numeric_limits<boost::floatmax_t>::is_iec559 == true);
  BOOST_MATH_ASSERT(BOOST_FLOATMAX_C(0.) == 0);

  ex_out << "floatmax_t " << std::numeric_limits<boost::floatmax_t>::digits << " bits\n" // 113
    << std::numeric_limits<boost::floatmax_t>::digits10 << " decimal digits\n" // 34
    << std::numeric_limits<boost::floatmax_t>::max_digits10 << " max_digits\n" // 36
    << std::numeric_limits<boost::floatmax_t>::radix << " radix\n"
    << std::endl;

  int significand_bits = std::numeric_limits<boost::floatmax_t>::digits;
  int exponent_max = std::numeric_limits<boost::floatmax_t>::max_exponent;
  int exponent_min = std::numeric_limits<boost::floatmax_t>::min_exponent;
  int exponent_bits = 1 + static_cast<int>(std::log2(std::numeric_limits<boost::floatmax_t>::max_exponent));
  int sign_bits = std::numeric_limits<boost::floatmax_t>::is_signed;

  ex_out << "significand_bits (including one implicit bit)" << significand_bits
    << ", exponent_bits " << exponent_bits
    << ", sign_bits " << sign_bits << std::endl;

  // One can compute the total number of bits in the floatmax_t,
  // but probably not at compile time.

  ex_out << "bits = " << significand_bits + exponent_bits + sign_bits -1 << std::endl;
  // -1 to take account of the implicit bit that is not part of the physical layout.

  // One can compare typedefs (but, of course, only those that are defined for the platform in use.)
  ex_out.setf(std::ios::boolalpha);
  ex_out << "double, double: " << std::is_same<double, double>::value << std::endl;
  bool b = boost::is_same<boost::floatmax_t, boost::float64_t>::value;
  ex_out << "boost::is_same<boost::floatmax_t, boost::float64_t>::value; " << b << std::endl;
  ex_out << "floatmax_t, float64_t: "
    << std::is_same<boost::floatmax_t, boost::float64_t>::value << std::endl;

/*`So the simplest way of obtaining the total number of bits in the floatmax_t
is to infer it from the std::numeric_limits<>::digits value.
This is possible because the type, must be a IEEE754 layout. */
//[floatmax_1
  const int fpbits =
    (std::numeric_limits<boost::floatmax_t>::digits == 113) ? 128 :
    (std::numeric_limits<boost::floatmax_t>::digits == 64) ? 80 :
    (std::numeric_limits<boost::floatmax_t>::digits == 53) ? 64 :
    (std::numeric_limits<boost::floatmax_t>::digits == 24) ? 32 :
    (std::numeric_limits<boost::floatmax_t>::digits == 11) ? 16 :
    0; // Unknown - not IEEE754 format.
   ex_out << fpbits << " bits." << std::endl;
//] [/floatmax_1]
#endif

  }
  catch (std::exception ex)
  { // Display details about why any exceptions are thrown.
    ex_out << "Thrown exception " << ex.what() << std::endl;
  }

    return 0;  // main's implicit return
} // int example_main()

/*
[cstdfloat_output

GCC 4.8.1 with quadmath

 pi = 1.00000
 0.88010117148986700
 0.0 0.0000000000000000
 1.0 0.88010117148986700
 2.0 4.6137984620549872
 3.0 16.274830009244951
 4.0 -25.360637961042869
 5.0 -1257.9038883512264
 6.0 -12749.592182518225
 7.0 -3020.9830849309437
 8.0 2421897.6013183584
 9.0 45577595.449204877
 0.0 0.00000000000000000000000000000000000
 1.0 0.880101171489866995756301548681221902
 2.0 4.61379846205498722611082484945654869
 3.0 16.2748300092449511566883302293717861
 4.0 -25.3606379610428689375112298876047134
 5.0 -1257.90388835122644195507746189832687
 6.0 -12749.5921825182249449426308274269104
 7.0 -3020.98308493094373261556029319763184
 8.0 2421897.60131835844367742538452148438
 9.0 45577595.4492048770189285278320312500
 Floating-point type long double is available with:
 std::numeric_limits<long double>::digits10 == 18
 std::numeric_limits<long double>::max_digits10 == 21
 pi = 3.14159265358979312
 Most precise pi = 3.141592653589793238462643383279503
 Floating-point type boost::float16_t is NOT available.
 Floating-point type boost::float32_t is available.
 std::numeric_limits<boost::float32_t>::digits10 == 6
 std::numeric_limits<boost::float32_t>::max_digits10 == 9
 Floating-point type boost::float64_t is available.
 std::numeric_limits<boost::float64_t>::digits10 == 15
 std::numeric_limits<boost::float64_t>::max_digits10 == 17
 Floating-point type boost::float80_t is available.
 std::numeric_limits<boost::float80_t>::digits10 == 18
 std::numeric_limits<boost::float80_t>::max_digits10 == 21
 Floating-point type boost::float128_t is available.
 std::numeric_limits<boost::float128_t>::digits10 == 34
 std::numeric_limits<boost::float128_t>::max_digits10 == 36
 BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = 123.456787
 BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = 123.45678901234568
 BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) = 123.456789012345678903
 BOOST_FLOAT128_C(123.4567890123456789012345678901234567890) = 123.456789012345678901234567890123453
 floatmax_t 113 bits
 34 decimal digits
 36 max_digits
 2 radix

 significand_bits (including one implicit bit)113, exponent_bits 15, sign_bits 1
 bits = 128
 double, double: true
 boost::is_same<boost::floatmax_t, boost::float64_t>::value; false
 floatmax_t, float64_t: false
 128 bits.

 RUN SUCCESSFUL (total time: 53ms)

GCC 6.1.1

pi = 1.00000
0.88010117148986700
0.0 0.0000000000000000
1.0 0.88010117148986700
2.0 4.6137984620549872
3.0 16.274830009244951
4.0 -25.360637961042869
5.0 -1257.9038883512264
6.0 -12749.592182518225
7.0 -3020.9830849309437
8.0 2421897.6013183584
9.0 45577595.449204877
0.0 0.00000000000000000000000000000000000
1.0 0.880101171489866995756301548681221902
2.0 4.61379846205498722611082484945654869
3.0 16.2748300092449511566883302293717861
4.0 -25.3606379610428689375112298876047134
5.0 -1257.90388835122644195507746189832687
6.0 -12749.5921825182249449426308274269104
7.0 -3020.98308493094373261556029319763184
8.0 2421897.60131835844367742538452148438
9.0 45577595.4492048770189285278320312500
Floating-point type long double is available with:
  std::numeric_limits<long double>::digits10 == 18
  std::numeric_limits<long double>::max_digits10 == 21
pi = 3.14159265358979312
Most precise pi = 3.14159265358979323846264338327950
Floating-point type boost::float16_t is NOT available.
Floating-point type boost::float32_t is available.
  std::numeric_limits<boost::float32_t>::digits10 == 6
  std::numeric_limits<boost::float32_t>::max_digits10 == 9
Floating-point type boost::float64_t is available.
  std::numeric_limits<boost::float64_t>::digits10 == 15
  std::numeric_limits<boost::float64_t>::max_digits10 == 17
Floating-point type boost::float80_t is available.
  std::numeric_limits<boost::float80_t>::digits10 == 18
  std::numeric_limits<boost::float80_t>::max_digits10 == 21
Floating-point type boost::float128_t is available.
  std::numeric_limits<boost::float128_t>::digits10 == 33
  std::numeric_limits<boost::float128_t>::max_digits10 == 36
BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = 123.456787
BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = 123.45678901234568
BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) = 123.456789012345678903
BOOST_FLOAT128_C(123.4567890123456789012345678901234567890) = 123.456789012345678901234567890123453
floatmax_t 113 bits
33 decimal digits
36 max_digits
2 radix

significand_bits (including one implicit bit)113, exponent_bits 15, sign_bits 1
bits = 128
double, double: true
boost::is_same<boost::floatmax_t, boost::float64_t>::value; false
floatmax_t, float64_t: false
128 bits.


 MSVC 2013  64-bit

 1>  pi = 1.00000
 1>  0.88010117148986700
 1>  0.00 0.00000000000000000
 1>  1.0 0.88010117148986700
 1>  2.0 4.6137984620549854
 1>  3.0 16.274830009244948
 1>  4.0 -25.360637961042869
 1>  5.0 -1257.9038883512258
 1>  6.0 -12749.592182518225
 1>  7.0 -3020.9830849309396
 1>  8.0 2421897.6013183575
 1>  9.0 45577595.449204892
 1>  0.00 0.00000000000000000
 1>  1.0 0.88010117148986700
 1>  2.0 4.6137984620549854
 1>  3.0 16.274830009244948
 1>  4.0 -25.360637961042869
 1>  5.0 -1257.9038883512258
 1>  6.0 -12749.592182518225
 1>  7.0 -3020.9830849309396
 1>  8.0 2421897.6013183575
 1>  9.0 45577595.449204892
 1>  Floating-point type long double is available with:
 1>    std::numeric_limits<long double>::digits10 == 15
 1>    std::numeric_limits<long double>::max_digits10 == 17
 1>  pi = 3.14159265358979
 1>  Most precise pi = 3.14159265358979
 1>  Floating-point type boost::float16_t is NOT available.
 1>  Floating-point type boost::float32_t is available.
 1>    std::numeric_limits<boost::float32_t>::digits10 == 6
 1>    std::numeric_limits<boost::float32_t>::max_digits10 == 9
 1>  Floating-point type boost::float64_t is available.
 1>    std::numeric_limits<boost::float64_t>::digits10 == 15
 1>    std::numeric_limits<boost::float64_t>::max_digits10 == 17
 1>  Floating-point type boost::float80_t is NOT available.
 1>  Floating-point type boost::float128_t is NOT available.
 1>  BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = 123.456787
 1>  BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = 123.45678901234568
 1>  floatmax_t 53 bits
 1>  15 decimal digits
 1>  17 max_digits
 1>  2 radix
 1>
 1>  significand_bits (including one implicit bit)53, exponent_bits 11, sign_bits 1
 1>  bits = 64
 1>  double, double: true
 1>  boost::is_same<boost::floatmax_t, boost::float64_t>::value; true
 1>  floatmax_t, float64_t: true
 1>  64 bits.
] [/cstdfloat_output]


*/

TEST_CASE("example: cstdfloat_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(pi = 1.00000
0.88010117148986700
0.0 0.0000000000000000
1.0 0.88010117148986700
2.0 4.6137984620549872
3.0 16.274830009244951
4.0 -25.360637961042869
5.0 -1257.9038883512264
6.0 -12749.592182518225
7.0 -3020.9830849309437
8.0 2421897.6013183584
9.0 45577595.449204877
0.0 0.00000000000000000000
1.0 0.880101171489866995756
2.0 4.61379846205498722611
3.0 16.2748300092449511567
4.0 -25.3606379610428689375
5.0 -1257.90388835122644196
6.0 -12749.5921825182249449
7.0 -3020.98308493094373262
8.0 2421897.60131835844368
9.0 45577595.4492048770189
Floating-point type long double is available with:
  std::numeric_limits<long double>::digits10 == 18
  std::numeric_limits<long double>::max_digits10 == 21
pi = 3.14159265358979312
Most precise pi = 3.14159265358979324
Floating-point type boost::float16_t is NOT available.
Floating-point type boost::float32_t is available.
  std::numeric_limits<boost::float32_t>::digits10 == 6
  std::numeric_limits<boost::float32_t>::max_digits10 == 9
Floating-point type boost::float64_t is available.
  std::numeric_limits<boost::float64_t>::digits10 == 15
  std::numeric_limits<boost::float64_t>::max_digits10 == 17
Floating-point type boost::float80_t is available.
  std::numeric_limits<boost::float80_t>::digits10 == 18
  std::numeric_limits<boost::float80_t>::max_digits10 == 21
Floating-point type boost::float128_t is NOT available.
BOOST_FLOAT32_C(123.4567890123456789012345678901234567890) = 123.456787
BOOST_FLOAT64_C(123.4567890123456789012345678901234567890) = 123.45678901234568
BOOST_FLOAT80_C(123.4567890123456789012345678901234567890) = 123.456789012345678903
floatmax_t 64 bits
18 decimal digits
21 max_digits
2 radix

significand_bits (including one implicit bit)64, exponent_bits 15, sign_bits 1
bits = 79
double, double: true
boost::is_same<boost::floatmax_t, boost::float64_t>::value; false
floatmax_t, float64_t: false
80 bits.
)ex"));
}
}  // namespace ex_cstdfloat_example

// libs/math/example/error_policies_example.cpp
namespace ex_error_policies_example {
  using boost::math::normal_distribution;

   using boost::math::students_t;  // Probability of students_t(df, t).
   using boost::math::students_t_distribution;

//  using namespace boost::math; causes:
//.\error_policy_normal.cpp(30) : error C2872: 'policy' : ambiguous symbol
//        could be '\boost/math/policies/policy.hpp(392) : boost::math::policies::policy'
//        or 'boost::math::policies'

// So should not use this 'using namespace boost::math;' command.

// Suppose we want a statistical distribution to return infinities,
// rather than throw exceptions (the default policy), then we can use:

// std
   
   using std::endl;

// using namespace boost::math::policies; or

using boost::math::policies::policy;
// Possible errors
using boost::math::policies::overflow_error;
using boost::math::policies::underflow_error;
using boost::math::policies::domain_error;
using boost::math::policies::pole_error;
using boost::math::policies::denorm_error;
using boost::math::policies::evaluation_error;
using boost::math::policies::ignore_error;

// Define a custom policy to ignore just overflow:
typedef policy<
overflow_error<ignore_error>
      > my_policy;

// Define another custom policy (perhaps ill-advised?)
// to ignore all errors: domain, pole, overflow, underflow, denorm & evaluation:
typedef policy<
domain_error<ignore_error>,
pole_error<ignore_error>,
overflow_error<ignore_error>,
underflow_error<ignore_error>,
denorm_error<ignore_error>,
evaluation_error<ignore_error>
      > my_ignoreall_policy;

// Define a new distribution with a custom policy to ignore_error
// (& thus perhaps return infinity for some arguments):
typedef boost::math::normal_distribution<double, my_policy> my_normal;
// Note: uses default parameters zero mean and unit standard deviation.

// We could also do the same for another distribution, for example:
using boost::math::students_t_distribution;
typedef students_t_distribution<double, my_ignoreall_policy> my_students_t;

int example_main()
{
  ex_out << "quantile(my_normal(), 0.05); = " << quantile(my_normal(), 0.05) << endl; // 0.05 is argument within normal range.
  ex_out << "quantile(my_normal(), 0.); = " << quantile(my_normal(), 0.) << endl; // argument zero, so expect infinity.
  ex_out << "quantile(my_normal(), 0.); = " << quantile(my_normal(), 0.F) << endl; // argument zero, so expect infinity.

  ex_out << "quantile(my_students_t(), 0.); = " << quantile(my_students_t(-1), 0.F) << endl; // 'bad' argument negative, so expect NaN.

#ifndef BOOST_MATH_NO_LONG_DOUBLE_MATH_FUNCTIONS
  // Construct a (0, 1) normal distribution that ignores all errors,
  // returning NaN, infinity, zero, or best guess,
  // and NOT setting errno.
  normal_distribution<long double, my_ignoreall_policy> my_normal2(0.L, 1.L); // explicit parameters for distribution.
  ex_out << "quantile(my_normal2(), 0.); = " << quantile(my_normal2, 0.01) << endl; // argument 0.01, so result finite.
  ex_out << "quantile(my_normal2(), 0.); = " << quantile(my_normal2, 0.) << endl; // argument zero, so expect infinity.
#endif

  return 0;
    return 0;  // main's implicit return
}

/*

Output:

error_policies_example.cpp
  Generating code
  Finished generating code
  error_policy_normal_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\error_policies_example.exe
  quantile(my_normal(), 0.05); = -1.64485
  quantile(my_normal(), 0.); = -1.#INF
  quantile(my_normal(), 0.); = -1.#INF
  quantile(my_students_t(), 0.); = 1.#QNAN
  quantile(my_normal2(), 0.); = -2.32635
  quantile(my_normal2(), 0.); = -1.#INF

*/

TEST_CASE("example: error_policies_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(quantile(my_normal(), 0.05); = -1.64485
quantile(my_normal(), 0.); = -inf
quantile(my_normal(), 0.); = -inf
quantile(my_students_t(), 0.); = nan
quantile(my_normal2(), 0.); = -2.32635
quantile(my_normal2(), 0.); = -inf
)ex"));
}
}  // namespace ex_error_policies_example

// libs/math/example/factorial_example.cpp
namespace ex_factorial_example {
using namespace std;

int example_main()
{
  using namespace boost::math;

  // Factorials
  unsigned int n = 3;
 
  try
  {
     ex_out << "Factorial: " << factorial<double>(n) << endl;

     // Caution: You must provide a return type template value, so this will not compile
     // unsigned int nfac = factorial(n); // could not deduce template argument for 'T'
     // You must provide an explicit floating-point (not integer) return type.
     // If you do provide an integer type, like this:
     // unsigned int uintfac = factorial<unsigned int>(n);
     // you will also get a compile error, for MSVC C2338.
     // If you really want an integer type, you can convert from double:
     unsigned int intfac = static_cast<unsigned int>(factorial<double>(n));
     // this will be exact, until the result of the factorial overflows the integer type.

     ex_out << "Unchecked factorial: " << boost::math::unchecked_factorial<float>(n) << endl;
     // Note:
     // unsigned int unfac = boost::math::unchecked_factorial<unsigned int>(n);
     // also fails to compile for the same reasons.
  } 
  catch(exception& e)
  {
    ex_out << e.what() << endl;
  }

  // Double factorial n!!
  try
  {
    //ex_out << "Double factorial: " << boost::math::double_factorial<unsigned>(n);
  }
  catch(exception& e)
  {
    ex_out << e.what() << endl;
  }

  // Rising and falling factorials
  try
  {
    int i = 2; double x = 8;
    ex_out << "Rising factorial: " << rising_factorial(x,i) << endl;
    ex_out << "Falling factorial: " << falling_factorial(x,i) << endl;
  }
  catch(exception& e)
  {
    ex_out << e.what() << endl;
  }

  // Binomial coefficients
  try
  {
    unsigned n = 10; unsigned k = 2;
    // ex_out << "Binomial coefficient: " << boost::math::binomial_coefficient<unsigned>(n,k) << endl;
  }
  catch(exception& e)
  {
    ex_out << e.what() << endl;
  }
  return 0;
    return 0;  // main's implicit return
}

/*

Output:

  factorial_example.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\factorial_example.exe
  Factorial: 6
  Unchecked factorial: 6
  Rising factorial: 72
  Falling factorial: 56

*/

TEST_CASE("example: factorial_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Factorial: 6
Unchecked factorial: 6
Rising factorial: 72
Falling factorial: 56
)ex"));
}
}  // namespace ex_factorial_example

// libs/math/example/find_root_example.cpp
namespace ex_find_root_example {
  using boost::math::normal; // typedef provides default type is double.

   using std::endl; using std::left; using std::showpoint; using std::noshowpoint;
  using std::setw; using std::setprecision;
  using std::numeric_limits;
  

//] //[/root_find1]

int example_main()
{
  ex_out << "Example: Normal distribution, root finding.";
  try
  {

//[root_find2

/*`A machine is set to pack 3 kg of ground beef per pack.
Over a long period of time it is found that the average packed was 3 kg
with a standard deviation of 0.1 kg.
Assuming the packing is normally distributed,
we can find the fraction (or %) of packages that weigh more than 3.1 kg.
*/

double mean = 3.; // kg
double standard_deviation = 0.1; // kg
normal packs(mean, standard_deviation);

double max_weight = 3.1; // kg
ex_out << "Percentage of packs > " << max_weight << " is "
<< cdf(complement(packs, max_weight)) << endl; // P(X > 3.1)

double under_weight = 2.9;
ex_out <<"fraction of packs <= " << under_weight << " with a mean of " << mean
  << " is " << cdf(complement(packs, under_weight)) << endl;
// fraction of packs <= 2.9 with a mean of 3 is 0.841345
// This is 0.84 - more than the target 0.95
// Want 95% to be over this weight, so what should we set the mean weight to be?
// KK StatCalc says:
double over_mean = 3.0664;
normal xpacks(over_mean, standard_deviation);
ex_out << "fraction of packs >= " << under_weight
<< " with a mean of " << xpacks.mean()
  << " is " << cdf(complement(xpacks, under_weight)) << endl;
// fraction of packs >= 2.9 with a mean of 3.06449 is 0.950005
double under_fraction = 0.05;  // so 95% are above the minimum weight mean - sd = 2.9
double low_limit = standard_deviation;
double offset = mean - low_limit - quantile(packs, under_fraction);
double nominal_mean = mean + offset;

normal nominal_packs(nominal_mean, standard_deviation);
ex_out << "Setting the packer to " << nominal_mean << " will mean that "
  << "fraction of packs >= " << under_weight
  << " is " << cdf(complement(nominal_packs, under_weight)) << endl;

/*`
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95.

Setting the packer to 3.13263 will mean that fraction of packs >= 2.9 is 0.99,
but will more than double the mean loss from 0.0644 to 0.133.

Alternatively, we could invest in a better (more precise) packer with a lower standard deviation.

To estimate how much better (how much smaller standard deviation) it would have to be,
we need to get the 5% quantile to be located at the under_weight limit, 2.9
*/
double p = 0.05; // wanted p th quantile.
ex_out << "Quantile of " << p << " = " << quantile(packs, p)
  << ", mean = " << packs.mean() << ", sd = " << packs.standard_deviation() << endl; //
/*`
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1

With the current packer (mean = 3, sd = 0.1), the 5% quantile is at 2.8551 kg,
a little below our target of 2.9 kg.
So we know that the standard deviation is going to have to be smaller.

Let's start by guessing that it (now 0.1) needs to be halved, to a standard deviation of 0.05
*/
normal pack05(mean, 0.05);
ex_out << "Quantile of " << p << " = " << quantile(pack05, p)
  << ", mean = " << pack05.mean() << ", sd = " << pack05.standard_deviation() << endl;

ex_out <<"Fraction of packs >= " << under_weight << " with a mean of " << mean
  << " and standard deviation of " << pack05.standard_deviation()
  << " is " << cdf(complement(pack05, under_weight)) << endl;
//
/*`
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.9772

So 0.05 was quite a good guess, but we are a little over the 2.9 target,
so the standard deviation could be a tiny bit more. So we could do some
more guessing to get closer, say by increasing to 0.06
*/

normal pack06(mean, 0.06);
ex_out << "Quantile of " << p << " = " << quantile(pack06, p)
  << ", mean = " << pack06.mean() << ", sd = " << pack06.standard_deviation() << endl;

ex_out <<"Fraction of packs >= " << under_weight << " with a mean of " << mean
  << " and standard deviation of " << pack06.standard_deviation()
  << " is " << cdf(complement(pack06, under_weight)) << endl;
/*`
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.9522

Now we are getting really close, but to do the job properly,
we could use root finding method, for example the tools provided, and used elsewhere,
in the Math Toolkit, see __root_finding_without_derivatives.

But in this normal distribution case, we could be even smarter and make a direct calculation.
*/
//] [/root_find2]

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

//[root_find_output

Autorun "i:\boost-06-05-03-1300\libs\math\test\Math_test\debug\find_root_example.exe"
Example: Normal distribution, root finding.Percentage of packs > 3.1 is 0.158655
fraction of packs <= 2.9 with a mean of 3 is 0.841345
fraction of packs >= 2.9 with a mean of 3.0664 is 0.951944
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1
Quantile of 0.05 = 2.91776, mean = 3, sd = 0.05
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.97725
Quantile of 0.05 = 2.90131, mean = 3, sd = 0.06
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.95221

//] [/root_find_output]
*/

TEST_CASE("example: find_root_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example: Normal distribution, root finding.Percentage of packs > 3.1 is 0.158655
fraction of packs <= 2.9 with a mean of 3 is 0.841345
fraction of packs >= 2.9 with a mean of 3.0664 is 0.951944
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1
Quantile of 0.05 = 2.91776, mean = 3, sd = 0.05
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.97725
Quantile of 0.05 = 2.90131, mean = 3, sd = 0.06
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.95221
)ex"));
}
}  // namespace ex_find_root_example

// libs/math/example/gauss_example.cpp
namespace ex_gauss_example {
void gauss_examples()
{
   //[gauss_example

   /*`
   We'll begin by integrating t[super 2] atan(t) over (0,1) using a 7 term Gauss-Legendre rule,
   and begin by defining the function to integrate as a C++ lambda expression:
   */
   using namespace boost::math::quadrature;

   auto f = [](const double& t) { return t * t * std::atan(t); };

   /*`
   Integration is simply a matter of calling the `gauss<double, 7>::integrate` method:
   */

   double Q = gauss<double, 7>::integrate(f, 0, 1);

   /*`
   Which yields a value 0.2106572512 accurate to 1e-10.

   For more accurate evaluations, we'll move to a multiprecision type and use a 20-point integration scheme:
   */

   using boost::multiprecision::cpp_bin_float_quad;

   auto f2 = [](const cpp_bin_float_quad& t) { return t * t * atan(t); };

   cpp_bin_float_quad Q2 = gauss<cpp_bin_float_quad, 20>::integrate(f2, 0, 1);

   /*`
   Which yields 0.2106572512258069881080923020669, which is accurate to 5e-28.
   */

   //]

   ex_out << std::setprecision(18) << Q << std::endl;
   ex_out << boost::math::relative_difference(Q, (boost::math::constants::pi<double>() - 2 + 2 * boost::math::constants::ln_two<double>()) / 12) << std::endl;

   ex_out << std::setprecision(34) << Q2 << std::endl;
   ex_out << boost::math::relative_difference(Q2, (boost::math::constants::pi<cpp_bin_float_quad>() - 2 + 2 * boost::math::constants::ln_two<cpp_bin_float_quad>()) / 12) << std::endl;
}

void gauss_kronrod_examples()
{
   //[gauss_kronrod_example

   /*`
   We'll begin by integrating exp(-t[super 2]/2) over (0,+[infin]) using a 7 term Gauss rule
   and 15 term Kronrod rule,
   and begin by defining the function to integrate as a C++ lambda expression:
   */
   using namespace boost::math::quadrature;

   auto f1 = [](double t) { return std::exp(-t*t / 2); };

   //<-
   double Q_expected = sqrt(boost::math::constants::half_pi<double>());
   //->

   /*`
   We'll start off with a one shot (ie non-adaptive)
   integration, and keep track of the estimated error:
   */
   double error;
   double Q = gauss_kronrod<double, 15>::integrate(f1, 0, std::numeric_limits<double>::infinity(), 0, 0, &error);

   /*`
   This yields Q = 1.25348207361, which has an absolute error of 1e-4 compared to the estimated error
   of 5e-3: this is fairly typical, with the difference between Gauss and Gauss-Kronrod schemes being
   much higher than the actual error.  Before moving on to adaptive quadrature, lets try again
   with more points, in fact with the largest Gauss-Kronrod scheme we have cached (30/61):
   */
   //<-
   ex_out << std::setprecision(16) << Q << std::endl;
   ex_out << boost::math::relative_difference(Q, Q_expected) << std::endl;
   ex_out << fabs(Q - Q_expected) << std::endl;
   ex_out << error << std::endl;
   //->
   Q = gauss_kronrod<double, 61>::integrate(f1, 0, std::numeric_limits<double>::infinity(), 0, 0, &error);
   //<-
   ex_out << std::setprecision(16) << Q << std::endl;
   ex_out << boost::math::relative_difference(Q, Q_expected) << std::endl;
   ex_out << fabs(Q - Q_expected) << std::endl;
   ex_out << error << std::endl;
   //->
   /*`
   This yields an absolute error of 3e-15 against an estimate of 1e-8, which is about as good as we're going to get
   at double precision

   However, instead of continuing with ever more points, lets switch to adaptive integration, and set the desired relative
   error to 1e-14 against a maximum depth of 5:
   */
   Q = gauss_kronrod<double, 15>::integrate(f1, 0, std::numeric_limits<double>::infinity(), 5, 1e-14, &error);
   //<-
   ex_out << std::setprecision(16) << Q << std::endl;
   ex_out << boost::math::relative_difference(Q, Q_expected) << std::endl;
   ex_out << fabs(Q - Q_expected) << std::endl;
   ex_out << error << std::endl;
   //->
   /*`
   This yields an actual error of zero, against an estimate of 4e-15.  In fact in this case the requested tolerance was almost
   certainly set too low: as we've seen above, for smooth functions, the precision achieved is often double
   that of the estimate, so if we integrate with a tolerance of 1e-9:
   */
   Q = gauss_kronrod<double, 15>::integrate(f1, 0, std::numeric_limits<double>::infinity(), 5, 1e-9, &error);
   //<-
   ex_out << std::setprecision(16) << Q << std::endl;
   ex_out << boost::math::relative_difference(Q, Q_expected) << std::endl;
   ex_out << fabs(Q - Q_expected) << std::endl;
   ex_out << error << std::endl;
   //->
   /*`
   We still achieve 1e-15 precision, with an error estimate of 1e-10.
   */
   //]
}

int example_main()
{
   gauss_examples();
   gauss_kronrod_examples();
   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: gauss_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0.210657251248371341
1.07113992775630998e-10
0.2106572512258069881080923020669069
5.51042479573307102209438033710404e-28
1.253482073612048
0.0001339937782136083
0.0001679362965474329
0.005441989038053929
1.253314137315504
3.011821355347324e-15
3.774758283725532e-15
1.982572761516366e-08
1.2533141373155
0
0
4.156566901595602e-15
1.2533141373155
1.771659620792544e-16
2.220446049250313e-16
2.628606807130007e-10
)ex"));
}
}  // namespace ex_gauss_example

// libs/math/example/inspect_fp.cpp
namespace ex_inspect_fp {
#ifndef BOOST_MATH_STANDALONE
#else
#endif


//------------------------------------------------------------------------------

bool is_big_endian()
{
    float x = 1.0f;
    unsigned char first_byte;
  memcpy(&first_byte, &x, 1);
    return first_byte != 0;
}

//------------------------------------------------------------------------------

void print_processor();
void print_endianness();
template<class T> void print_table();
template<class T> void print_row(const char* name, T val, bool ok = true);

//------------------------------------------------------------------------------

int example_main()
{
    ex_out << '\n';

  print_processor();

  print_endianness();

    ex_out << "---------- float --------------------\n\n";
    print_table<float>();

    ex_out << "---------- double -------------------\n\n";
    print_table<double>();

    ex_out << "---------- long double --------------\n\n";
    print_table<long double>();

    return 0;
    return 0;  // main's implicit return
}

//------------------------------------------------------------------------------

void print_processor()
{
#if defined(__i386) || defined(__i386__) || defined(_M_IX86) \
    || defined(__amd64) || defined(__amd64__)  || defined(_M_AMD64) \
    || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64)

  ex_out << "Processor: x86 or x64\n\n";

#elif defined(__ia64) || defined(__ia64__) || defined(_M_IA64)

  ex_out << "Processor: ia64\n\n";

#elif defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__) \
    || defined(__ppc) || defined(__ppc__) || defined(__PPC__)

  ex_out << "Processor: PowerPC\n\n";

#elif defined(__m68k) || defined(__m68k__) \
    || defined(__mc68000) || defined(__mc68000__) \

  ex_out << "Processor: Motorola 68K\n\n";

#else

  ex_out << "Processor: Unknown\n\n";

#endif
}

void print_endianness()
{
    if(is_big_endian())
        ex_out << "This platform is big-endian.\n";
    else
        ex_out << "This platform is little-endian.\n";

#ifdef BOOST_BIG_ENDIAN
    ex_out << "BOOST_BIG_ENDIAN is defined.\n\n";
#endif
#if defined BOOST_LITTLE_ENDIAN
    ex_out << "BOOST_LITTTLE_ENDIAN is defined.\n\n";
#endif
}

//..............................................................................

template<class T> void print_table()
{
    print_row("0", (T)0);
    print_row("sn.min", std::numeric_limits<T>::denorm_min(),
          boost::math::detail::has_denorm_now<T>());
    print_row("-sn.min", -std::numeric_limits<T>::denorm_min(),
          boost::math::detail::has_denorm_now<T>());
    print_row("n.min/256", (std::numeric_limits<T>::min)()/256);
    print_row("n.min/2", (std::numeric_limits<T>::min)()/2);
    print_row("-n.min/2", -(std::numeric_limits<T>::min)()/2);
    print_row("n.min", (std::numeric_limits<T>::min)());
    print_row("1", (T)1);
    print_row("3/4", (T)3/(T)4);
    print_row("4/3", (T)4/(T)3);
    print_row("max", (std::numeric_limits<T>::max)());
    print_row("inf", std::numeric_limits<T>::infinity(),
          std::numeric_limits<T>::has_infinity);
    print_row("q.nan", std::numeric_limits<T>::quiet_NaN(),
          std::numeric_limits<T>::has_quiet_NaN);
    print_row("s.nan", std::numeric_limits<T>::signaling_NaN(),
          std::numeric_limits<T>::has_signaling_NaN);

    ex_out << "\n\n";
}

template<class T>
void print_row(const char* name, T val, bool ok)
{
    ex_out << std::left << std::setw(10) << name << std::right;

    ex_out << std::hex << std::setfill('0');

    if(ok) {
        if(is_big_endian()) {
      for(size_t i = 0; i < sizeof(T); ++i) {
        unsigned char c = *(reinterpret_cast<unsigned char*>(&val) + i);
                ex_out << std::setw(2)
                    << static_cast<unsigned int>(c) << ' ';
      }
        }
        else {
      for(size_t i = sizeof(T) - 1; i < sizeof(T); --i) {
        unsigned char c = *(reinterpret_cast<unsigned char*>(&val) + i);
                ex_out << std::setw(2)
                    << static_cast<unsigned int>(c) << ' ';
      }
        }
    }
    else {
        for(size_t i = 0; i < sizeof(T); ++i)
            ex_out << "-- ";
    }

    ex_out << '\n';
    ex_out << std::dec << std::setfill(' ');
}

/*

Sample output on an AMD Quadcore running MSVC 10

  Processor: x86 or x64

  This platform is little-endian.
  BOOST_LITTTLE_ENDIAN is defined.

  ---------- float --------------------

  0         00 00 00 00
  sn.min    00 00 00 01
  -sn.min   80 00 00 01
  n.min/256 00 00 80 00
  n.min/2   00 40 00 00
  -n.min/2  80 40 00 00
  n.min     00 80 00 00
  1         3f 80 00 00
  3/4       3f 40 00 00
  4/3       3f aa aa ab
  max       7f 7f ff ff
  inf       7f 80 00 00
  q.nan     7f c0 00 00
  s.nan     7f c0 00 01


  ---------- double -------------------

  0         00 00 00 00 00 00 00 00
  sn.min    00 00 00 00 00 00 00 01
  -sn.min   80 00 00 00 00 00 00 01
  n.min/256 00 00 10 00 00 00 00 00
  n.min/2   00 08 00 00 00 00 00 00
  -n.min/2  80 08 00 00 00 00 00 00
  n.min     00 10 00 00 00 00 00 00
  1         3f f0 00 00 00 00 00 00
  3/4       3f e8 00 00 00 00 00 00
  4/3       3f f5 55 55 55 55 55 55
  max       7f ef ff ff ff ff ff ff
  inf       7f f0 00 00 00 00 00 00
  q.nan     7f f8 00 00 00 00 00 00
  s.nan     7f f8 00 00 00 00 00 01


  ---------- long double --------------

  0         00 00 00 00 00 00 00 00
  sn.min    00 00 00 00 00 00 00 01
  -sn.min   80 00 00 00 00 00 00 01
  n.min/256 00 00 10 00 00 00 00 00
  n.min/2   00 08 00 00 00 00 00 00
  -n.min/2  80 08 00 00 00 00 00 00
  n.min     00 10 00 00 00 00 00 00
  1         3f f0 00 00 00 00 00 00
  3/4       3f e8 00 00 00 00 00 00
  4/3       3f f5 55 55 55 55 55 55
  max       7f ef ff ff ff ff ff ff
  inf       7f f0 00 00 00 00 00 00
  q.nan     7f f8 00 00 00 00 00 00
  s.nan     7f f8 00 00 00 00 00 01

  */

TEST_CASE("example: inspect_fp") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(
Processor: x86 or x64

This platform is little-endian.
---------- float --------------------

0         00 00 00 00 
sn.min    00 00 00 01 
-sn.min   80 00 00 01 
n.min/256 00 00 80 00 
n.min/2   00 40 00 00 
-n.min/2  80 40 00 00 
n.min     00 80 00 00 
1         3f 80 00 00 
3/4       3f 40 00 00 
4/3       3f aa aa ab 
max       7f 7f ff ff 
inf       7f 80 00 00 
q.nan     7f c0 00 00 
s.nan     7f a0 00 00 


---------- double -------------------

0         00 00 00 00 00 00 00 00 
sn.min    00 00 00 00 00 00 00 01 
-sn.min   80 00 00 00 00 00 00 01 
n.min/256 00 00 10 00 00 00 00 00 
n.min/2   00 08 00 00 00 00 00 00 
-n.min/2  80 08 00 00 00 00 00 00 
n.min     00 10 00 00 00 00 00 00 
1         3f f0 00 00 00 00 00 00 
3/4       3f e8 00 00 00 00 00 00 
4/3       3f f5 55 55 55 55 55 55 
max       7f ef ff ff ff ff ff ff 
inf       7f f0 00 00 00 00 00 00 
q.nan     7f f8 00 00 00 00 00 00 
s.nan     7f f4 00 00 00 00 00 00 


---------- long double --------------

0         00 00 55 d0 56 2f 00 00 00 00 00 00 00 00 00 00 
sn.min    00 00 55 d0 56 30 00 00 00 00 00 00 00 00 00 01 
-sn.min   00 00 55 d0 56 30 80 00 00 00 00 00 00 00 00 01 
n.min/256 00 00 55 d0 56 30 00 00 00 80 00 00 00 00 00 00 
n.min/2   00 00 55 d0 56 30 00 00 40 00 00 00 00 00 00 00 
-n.min/2  00 00 55 d0 56 30 80 00 40 00 00 00 00 00 00 00 
n.min     00 00 55 d0 56 30 00 01 80 00 00 00 00 00 00 00 
1         00 00 55 d0 56 30 3f ff 80 00 00 00 00 00 00 00 
3/4       00 00 55 d0 56 30 3f fe c0 00 00 00 00 00 00 00 
4/3       00 00 55 d0 56 30 3f ff aa aa aa aa aa aa aa ab 
max       00 00 55 d0 56 30 7f fe ff ff ff ff ff ff ff ff 
inf       00 00 55 d0 56 30 7f ff 80 00 00 00 00 00 00 00 
q.nan     00 00 55 d0 56 30 7f ff c0 00 00 00 00 00 00 00 
s.nan     00 00 55 d0 56 30 7f ff a0 00 00 00 00 00 00 00 


)ex"));
#endif
}
}  // namespace ex_inspect_fp

// libs/math/example/inverse_gamma_distribution_example.cpp
namespace ex_inverse_gamma_distribution_example {
using boost::math::inverse_gamma_distribution;  //  inverse_gamma_distribution.
using boost::math::inverse_gamma;

using boost::math::tgamma; // Used for naive pdf as a comparison.

using boost::math::inverse_gamma_distribution;

    using std::endl;
using std::setprecision;
using std::sqrt;

int example_main()
{

  ex_out << "Example using Inverse Gamma distribution. " << endl;
  // TODO - awaiting a real example using Bayesian statistics.

#ifdef BOOST_NO_CXX11_NUMERIC_LIMITS
  int max_digits10 = 2 + (boost::math::policies::digits<double, boost::math::policies::policy<> >() * 30103UL) / 100000UL;
  ex_out << "BOOST_NO_CXX11_NUMERIC_LIMITS is defined" << endl;
#else
  int max_digits10 = std::numeric_limits<double>::max_digits10;
#endif
  ex_out << "Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = "
    << max_digits10 << endl;
  ex_out.precision(max_digits10); //

  double shape = 1.;
  double scale = 1.;
  double x = 0.5;
  // Construction using default RealType double, and default shape and scale..
  inverse_gamma_distribution<> my_inverse_gamma(shape, scale); // (alpha, beta)

  ex_out << "my_inverse_gamma.shape() = " << my_inverse_gamma.shape()
    << ", scale = "<< my_inverse_gamma.scale() << endl;
  ex_out << "x = " << x << ", pdf = " << pdf(my_inverse_gamma, x)
    << ", cdf = " << cdf(my_inverse_gamma, x) << endl;

  // Construct using  typedef and default shape and scale parameters.
  inverse_gamma my_ig;

  inverse_gamma my_ig23(2, 3);
  ex_out << "my_inverse_gamma.shape() = " << my_ig23.shape()
    << ", scale = "<< my_ig23.scale() << endl;
  ex_out << "x = " << x << ", pdf = " << pdf(my_ig23, x)
    << ", cdf = " << cdf(my_ig23, x) << endl;

  // Example of providing an 'out of domain' or 'bad' parameter,
  // here a shape < 1, for which mean is not defined.
  // Try block is essential to catch the exception message.
  // (Uses the default policy which is to throw on all errors).
  try
  {
    inverse_gamma if051(0.5, 1);
    //inverse_gamma if051(0.5, 1);
    ex_out << "mean(if051) = " << mean(if051) << endl;
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
  Example using Inverse Gamma distribution.
  std::numeric_limits<double>::max_digits10 = 17
  my_inverse_gamma.shape() = 1, scale = 1
  x = 0.5, pdf = 0.54134113294645081, cdf = 0.1353352832366127
  my_inverse_gamma.shape() = 2, scale = 3
  x = 0.5, pdf = 0.17847015671997774, cdf = 0.017351265236664509

  Message from thrown exception was:
     Error in function boost::math::mean(const inverse_gamma_distribution<double>&): Shape parameter is 0.5, but for a defined mean it must be > 1


*/

TEST_CASE("example: inverse_gamma_distribution_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example using Inverse Gamma distribution. 
Show all potentially significant decimal digits std::numeric_limits<double>::max_digits10 = 17
my_inverse_gamma.shape() = 1, scale = 1
x = 0.5, pdf = 0.54134113294645081, cdf = 0.1353352832366127
my_inverse_gamma.shape() = 2, scale = 3
x = 0.5, pdf = 0.17847015671997782, cdf = 0.017351265236664509
mean(if051) = 
Message from thrown exception was:
   Error in function boost::math::mean(const inverse_gamma_distribution<double>&): Shape parameter is 0.5, but for a defined mean it must be > 1
)ex"));
}
}  // namespace ex_inverse_gamma_distribution_example

// libs/math/example/lambert_w_basic_example.cpp
namespace ex_lambert_w_basic_example {
int example_main()
{
  double z = 2.0;
  double w0 = boost::math::lambert_w0(z);
  ex_out.setf(std::ios_base::showpoint); // Include any trailing zeros.
  ex_out.precision(std::numeric_limits<double>::max_digits10); // Show all possibly significant digits.
  // Avoid using max_digfigs10 so as many old compilers can run the most basic lambert_w0 test?
  // Require to get max_digits10
  //   [ run lambert_w_basic_example.cpp  : : : [ requires cxx11_numeric_limits ] ]
  ex_out << " lambert_w0(" << z << ") = " << w0 << std::endl; // lambert_w0(2.00000) = 0.852606
  z = -0.2;
  double wm1 = boost::math::lambert_wm1(z);
  ex_out << " lambert_wm1(" << z << ") = " << wm1 << std::endl; // lambert_wm1(-0.200000) = -2.54264
  return 0;
    return 0;  // main's implicit return
} // int example_main()

TEST_CASE("example: lambert_w_basic_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( lambert_w0(2.0000000000000000) = 0.85260550201372554
 lambert_wm1(-0.20000000000000001) = -2.5426413577735261
)ex"));
}
}  // namespace ex_lambert_w_basic_example

// libs/math/example/lbfgs_example.cpp
namespace ex_lbfgs_example {
namespace rdiff = boost::math::differentiation::reverse_mode;
namespace bopt  = boost::math::optimization;
double random_double(double min = 0.0, double max = 1.0)
{
    static thread_local std::mt19937       rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

template<typename S>
struct vec3
{
    /**
     * @brief R^3 coordinates of particle on Thomson Sphere
     */
    S x, y, z;
};

template<class S>
static inline vec3<S> sph_to_xyz(const S& theta, const S& phi)
{
    /**
     * convenience overload to convert from [theta,phi] -> x, y, z
     */
    return {sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)};
}

template<typename T>
T thomson_energy(std::vector<T>& r)
{
    /* inverse square law
     */
    const size_t N    = r.size() / 2;
    const T      tiny = T(1e-12);

    T E = 0;
    for (size_t i = 0; i < N; ++i) {
        const T& theta_i = r[2 * i + 0];
        const T& phi_i   = r[2 * i + 1];
        auto     ri      = sph_to_xyz(theta_i, phi_i);

        for (size_t j = i + 1; j < N; ++j) {
            const T& theta_j = r[2 * j + 0];
            const T& phi_j   = r[2 * j + 1];
            auto     rj      = sph_to_xyz(theta_j, phi_j);

            T dx = ri.x - rj.x;
            T dy = ri.y - rj.y;
            T dz = ri.z - rj.z;

            T d2 = dx * dx + dy * dy + dz * dz + tiny;
            E += 1.0 / sqrt(d2);
        }
    }
    return E;
}

template<class T>
std::vector<rdiff::rvar<T, 1>> init_theta_phi_uniform(size_t N, unsigned seed = 12345)
{
    const T pi = T(3.1415926535897932384626433832795);

    std::mt19937                      rng(seed);
    std::uniform_real_distribution<T> unif01(T(0), T(1));
    std::uniform_real_distribution<T> unifm11(T(-1), T(1));

    std::vector<rdiff::rvar<T, 1>> u;
    u.reserve(2 * N);

    for (size_t i = 0; i < N; ++i) {
        T z     = unifm11(rng);
        T phi   = (T(2) * pi) * unif01(rng) - pi;
        T theta = std::acos(z);

        u.emplace_back(theta);
        u.emplace_back(phi);
    }
    return u;
}

int example_main(int argc, char* argv[])
{

    if (argc != 2) {
        ex_out << "Usage: " << argv[0] << " <N>\n";
        return 1;
    }

    const int    N      = std::stoi(argv[1]);
    auto u_ad = init_theta_phi_uniform<double>(N);

    auto lbfgs_opt = bopt::make_lbfgs(&thomson_energy<rdiff::rvar<double, 1>>, u_ad);

    // filenames
    std::string pos_filename    = "thomson_" + std::to_string(N) + ".csv";
    std::string energy_filename = "lbfgs_energy_" + std::to_string(N) + ".csv";

    std::ofstream pos_out(pos_filename);
    std::ofstream energy_out(energy_filename);

    energy_out << "step,energy\n";

    auto result = minimize(lbfgs_opt);
    for (int pi = 0; pi < N; ++pi) {
        double theta = u_ad[2 * pi + 0].item();
        double phi   = u_ad[2 * pi + 1].item();
        auto   r     = sph_to_xyz(theta, phi);
        pos_out << pi << "," << r.x << "," << r.y << "," << r.z << "\n";
    }
    auto E = lbfgs_opt.objective_value();
    int i = 0;
    for(auto& obj_hist : result.objective_history)
    {
        energy_out << i << "," << obj_hist << "\n";
        ++i;
    }
    energy_out << "," << E << "\n";

    pos_out.close();
    energy_out.close();

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: lbfgs_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    ex_args = {"example"};
    ex_argv_store.clear();
    for (auto& a : ex_args) ex_argv_store.push_back(a.data());
    ex_argv_store.push_back(nullptr);
    ex_argv = ex_argv_store.data();
    ex_argc = int(ex_args.size());
    int rc = example_main(ex_argc, ex_argv_t{ex_argv});
    CHECK(rc == 1);
    CHECK(ex_out.str() == std::string_view(R"ex(Usage: example <N>
)ex"));
}
}  // namespace ex_lbfgs_example

// libs/math/example/naive_monte_carlo_example.cpp
namespace ex_naive_monte_carlo_example {
using std::vector;
using std::pair;
using boost::math::quadrature::naive_monte_carlo;

void display_progress(double progress,
                      double error_estimate,
                      double current_estimate,
                      std::chrono::duration<double> estimated_time_to_completion)
{
    int barWidth = 70;

    ex_out << "[";
    int pos = barWidth * progress;
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) ex_out << "=";
        else if (i == pos) ex_out << ">";
        else ex_out << " ";
    }
    ex_out << "] "
              << int(progress * 100.0)
              << "%, E = "
              << std::setprecision(3)
              << error_estimate
              << ", time to completion: "
              << estimated_time_to_completion.count()
              << " seconds, estimate: "
              << std::setprecision(5)
              << current_estimate
              << "     \r";

    ex_out.flush();
}

int example_main()
{
    using std::abs;
    double exact = 1.3932039296856768591842462603255;
    double A = 1.0 / boost::math::pow<3>(boost::math::constants::pi<double>());
    auto g = [&](std::vector<double> const & x)
    {
      return A / (1.0 - cos(x[0])*cos(x[1])*cos(x[2]));
    };
    vector<pair<double, double>> bounds{{0, boost::math::constants::pi<double>() }, {0, boost::math::constants::pi<double>() }, {0, boost::math::constants::pi<double>() }};
    naive_monte_carlo<double, decltype(g)> mc(g, bounds, 0.001);

    auto task = mc.integrate();

    int s = 0;
    ex_out << "Hit ctrl-c to cancel.\n";
    while (task.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
    {
        display_progress(mc.progress(),
                         mc.current_error_estimate(),
                         mc.current_estimate(),
                         mc.estimated_time_to_completion());
        // TODO: The following shows that cancellation works,
        // but it would be nice to show how it works with a ctrl-c signal handler.
        if (s++ > 25){
          mc.cancel();
          ex_out << "\nCancelling because this is too slow!\n";
        }
    }
    double y = task.get();
    display_progress(mc.progress(),
                     mc.current_error_estimate(),
                     mc.current_estimate(),
                     mc.estimated_time_to_completion());
    ex_out << std::setprecision(std::numeric_limits<double>::digits10) << std::fixed;
    ex_out << "\nFinal value: " << y << std::endl;
    ex_out << "Exact      : " << exact << std::endl;
    ex_out << "Final error estimate: " << mc.current_error_estimate() << std::endl;
    ex_out << "Actual error        : " << abs(y - exact) << std::endl;
    ex_out << "Function calls: " << mc.calls() << std::endl;
    ex_out << "Is this good enough? [y/N] ";
    bool goodenough = true;
    std::string line;
    std::getline(ex_in, line);
    if (line[0] != 'y')
    {
         goodenough = false;
    }
    double new_error = -1;
    if (!goodenough)
    {
        ex_out << "What is the new target error? ";
        std::getline(ex_in, line);
        new_error = atof(line.c_str());
        if (new_error >= mc.current_error_estimate())
        {
           ex_out << "That error bound is already satisfied.\n";
           return 0;
        }
    }
    if (new_error > 0)
    {
        mc.update_target_error(new_error);
        auto task = mc.integrate();
        ex_out << "Hit ctrl-c to cancel.\n";
        while (task.wait_for(std::chrono::seconds(1)) != std::future_status::ready)
        {
            display_progress(mc.progress(),
                             mc.current_error_estimate(),
                             mc.current_estimate(),
                             mc.estimated_time_to_completion());
        }
        double y = task.get();
        display_progress(mc.progress(),
                         mc.current_error_estimate(),
                         mc.current_estimate(),
                         mc.estimated_time_to_completion());
        ex_out << std::setprecision(std::numeric_limits<double>::digits10) << std::fixed;
        ex_out << "\nFinal value: " << y << std::endl;
        ex_out << "Exact      : " << exact << std::endl;
        ex_out << "Final error estimate: " << mc.current_error_estimate() << std::endl;
        ex_out << "Actual error        : " << abs(y - exact) << std::endl;
        ex_out << "Function calls: " << mc.calls() << std::endl;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: naive_monte_carlo_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(std::string_view("Hit ctrl-c to cancel.\n"
        "[=============>                                                        ] 19%, E = 0.00226, time to completion: 4.09 seconds, estimate: 1.3948     \r"
        "[=============================>                                        ] 42%, E = 0.00154, time to completion: 2.72 seconds, estimate: 1.3942     \r"
        "[===========================================>                          ] 61%, E = 0.00128, time to completion: 1.88 seconds, estimate: 1.3941     \r"
        "[===============================================================>      ] 90%, E = 0.00105, time to completion: 0.434 seconds, estimate: 1.3935     \r"
        "[======================================================================] 100%, E = 0.000992, time to completion: 0 seconds, estimate: 1.3937     \r\n"
        "Final value: 1.393668020931441\n"
        "Exact      : 1.393203929685677\n"
        "Final error estimate: 0.000991958098367\n"
        "Actual error        : 0.000464091245764\n"
        "Function calls: 212170768\n"
        "Is this good enough? [y/N] What is the new target error? ", 983)));
#endif
}
}  // namespace ex_naive_monte_carlo_example

// libs/math/example/negative_binomial_example2.cpp
namespace ex_negative_binomial_example2 {
  using boost::math::negative_binomial_distribution;
  using boost::math::negative_binomial; // typedef

// In a sequence of trials or events
// (Bernoulli, independent, yes or no, succeed or fail)
// with success_fraction probability p,
// negative_binomial is the probability that k or fewer failures
// precede the r th trial's success.


using std::endl;
using std::setprecision;
using std::showpoint;
using std::setw;
using std::left;
using std::right;
using std::numeric_limits;

int example_main()
{
  ex_out << "Negative_binomial distribution - simple example 2" << endl;
  // Construct a negative binomial distribution with:
  // 8 successes (r), success fraction (p) 0.25 = 25% or 1 in 4 successes.
  negative_binomial mynbdist(8, 0.25); // Shorter method using typedef.

  // Display (to check) properties of the distribution just constructed.
  ex_out << "mean(mynbdist) = " << mean(mynbdist) << endl; // 24
  ex_out << "mynbdist.successes() = " << mynbdist.successes()  << endl; // 8
  // r th successful trial, after k failures, is r + k th trial.
  ex_out << "mynbdist.success_fraction() = " << mynbdist.success_fraction() << endl; 
  // success_fraction = failures/successes or k/r = 0.25 or 25%. 
  ex_out << "mynbdist.percent success  = " << mynbdist.success_fraction() * 100 << "%"  << endl;
  // Show as % too.
  // Show some cumulative distribution function values for failures k = 2 and 8
  ex_out << "cdf(mynbdist, 2.) = " << cdf(mynbdist, 2.) << endl; // 0.000415802001953125
  ex_out << "cdf(mynbdist, 8.) = " << cdf(mynbdist, 8.) << endl; // 0.027129956288263202
  ex_out << "cdf(complement(mynbdist, 8.)) = " << cdf(complement(mynbdist, 8.)) << endl; // 0.9728700437117368
  // Check that cdf plus its complement is unity.
  ex_out << "cdf + complement = " << cdf(mynbdist, 8.) + cdf(complement(mynbdist, 8.))  << endl; // 1
  // Note: No complement for pdf! 

  // Compare cdf with sum of pdfs.
  double sum = 0.; // Calculate the sum of all the pdfs,
  int k = 20; // for 20 failures
  for(signed i = 0; i <= k; ++i)
  {
    sum += pdf(mynbdist, double(i));
  }
  // Compare with the cdf
  double cdf8 = cdf(mynbdist, static_cast<double>(k));
  double diff = sum - cdf8; // Expect the difference to be very small.
  ex_out << setprecision(17) << "Sum pdfs = " << sum << ' ' // sum = 0.40025683281803698
  << ", cdf = " << cdf(mynbdist, static_cast<double>(k)) //  cdf = 0.40025683281803687
  << ", difference = "  // difference = 0.50000000000000000
  << setprecision(1) << diff/ (std::numeric_limits<double>::epsilon() * sum)
  << " in epsilon units." << endl;

  // Note: Use boost::math::tools::epsilon rather than std::numeric_limits
  //  to cover RealTypes that do not specialize numeric_limits.

//[neg_binomial_example2

  // Print a table of values that can be used to plot
  // using Excel, or some other superior graphical display tool.

  ex_out.precision(17); // Use max_digits10 precision, the maximum available for a reference table.
  ex_out << showpoint << endl; // include trailing zeros.
  // This is a maximum possible precision for the type (here double) to suit a reference table.
  int maxk = static_cast<int>(2. * mynbdist.successes() /  mynbdist.success_fraction());
  // This maxk shows most of the range of interest, probability about 0.0001 to 0.999.
  ex_out << "\n"" k            pdf                      cdf""\n" << endl;
  for (int k = 0; k < maxk; k++)
  {
    ex_out << right << setprecision(17) << showpoint
      << right << setw(3) << k  << ", "
      << left << setw(25) << pdf(mynbdist, static_cast<double>(k))
      << left << setw(25) << cdf(mynbdist, static_cast<double>(k))
      << endl;
  }
  ex_out << endl;
//] [/ neg_binomial_example2]
  return 0;
    return 0;  // main's implicit return
} // int example_main()

/*

Output is:

negative_binomial distribution - simple example 2
mean(mynbdist) = 24
mynbdist.successes() = 8
mynbdist.success_fraction() = 0.25
mynbdist.percent success  = 25%
cdf(mynbdist, 2.) = 0.000415802001953125
cdf(mynbdist, 8.) = 0.027129956288263202
cdf(complement(mynbdist, 8.)) = 0.9728700437117368
cdf + complement = 1
Sum pdfs = 0.40025683281803692 , cdf = 0.40025683281803687, difference = 0.25 in epsilon units.

//[neg_binomial_example2_1
 k            pdf                      cdf
  0, 1.5258789062500000e-005  1.5258789062500003e-005  
  1, 9.1552734375000000e-005  0.00010681152343750000   
  2, 0.00030899047851562522   0.00041580200195312500   
  3, 0.00077247619628906272   0.0011882781982421875    
  4, 0.0015932321548461918    0.0027815103530883789    
  5, 0.0028678178787231476    0.0056493282318115234    
  6, 0.0046602040529251142    0.010309532284736633     
  7, 0.0069903060793876605    0.017299838364124298     
  8, 0.0098301179241389001    0.027129956288263202     
  9, 0.013106823898851871     0.040236780187115073     
 10, 0.016711200471036140     0.056947980658151209     
 11, 0.020509200578089786     0.077457181236241013     
 12, 0.024354675686481652     0.10181185692272265      
 13, 0.028101548869017230     0.12991340579173993      
 14, 0.031614242477644432     0.16152764826938440      
 15, 0.034775666725408917     0.19630331499479325      
 16, 0.037492515688331451     0.23379583068312471      
 17, 0.039697957787645101     0.27349378847076977      
 18, 0.041352039362130305     0.31484582783290005      
 19, 0.042440250924291580     0.35728607875719176      
 20, 0.042970754060845245     0.40025683281803687      
 21, 0.042970754060845225     0.44322758687888220      
 22, 0.042482450037426581     0.48571003691630876      
 23, 0.041558918514873783     0.52726895543118257      
 24, 0.040260202311284021     0.56752915774246648      
 25, 0.038649794218832620     0.60617895196129912      
 26, 0.036791631035234917     0.64297058299653398      
 27, 0.034747651533277427     0.67771823452981139      
 28, 0.032575923312447595     0.71029415784225891      
 29, 0.030329307911589130     0.74062346575384819      
 30, 0.028054609818219924     0.76867807557206813      
 31, 0.025792141284492545     0.79447021685656061      
 32, 0.023575629142856460     0.81804584599941710      
 33, 0.021432390129869489     0.83947823612928651      
 34, 0.019383705779220189     0.85886194190850684      
 35, 0.017445335201298231     0.87630727710980494      
 36, 0.015628112784496322     0.89193538989430121      
 37, 0.013938587078064250     0.90587397697236549      
 38, 0.012379666154859701     0.91825364312722524      
 39, 0.010951243136991251     0.92920488626421649      
 40, 0.0096507830144735539    0.93885566927869002      
 41, 0.0084738582566109364    0.94732952753530097      
 42, 0.0074146259745345548    0.95474415350983555      
 43, 0.0064662435824429246    0.96121039709227851      
 44, 0.0056212231142827853    0.96683162020656122      
 45, 0.0048717266990450708    0.97170334690560634      
 46, 0.0042098073105878630    0.97591315421619418      
 47, 0.0036275999165703964    0.97954075413276465      
 48, 0.0031174686783026818    0.98265822281106729      
 49, 0.0026721160099737302    0.98533033882104104      
 50, 0.0022846591885275322    0.98761499800956853      
 51, 0.0019486798960970148    0.98956367790566557      
 52, 0.0016582516423517923    0.99122192954801736      
 53, 0.0014079495076571762    0.99262987905567457      
 54, 0.0011928461106539983    0.99382272516632852      
 55, 0.0010084971662802015    0.99483122233260868      
 56, 0.00085091948404891532   0.99568214181665760      
 57, 0.00071656377604119542   0.99639870559269883      
 58, 0.00060228420831048650   0.99700098980100937      
 59, 0.00050530624256557675   0.99750629604357488      
 60, 0.00042319397814867202   0.99792949002172360      
 61, 0.00035381791615708398   0.99828330793788067      
 62, 0.00029532382517950324   0.99857863176306016      
 63, 0.00024610318764958566   0.99882473495070978      
//] [neg_binomial_example2_1 end of Quickbook]

*/

TEST_CASE("example: negative_binomial_example2") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Negative_binomial distribution - simple example 2
mean(mynbdist) = 24
mynbdist.successes() = 8
mynbdist.success_fraction() = 0.25
mynbdist.percent success  = 25%
cdf(mynbdist, 2.) = 0.000415802
cdf(mynbdist, 8.) = 0.02713
cdf(complement(mynbdist, 8.)) = 0.97287
cdf + complement = 1
Sum pdfs = 0.40025683281803692 , cdf = 0.40025683281803692, difference = 0 in epsilon units.


 k            pdf                      cdf

  0, 1.5258789062500000e-05   1.5258789062500000e-05   
  1, 9.1552734375000000e-05   0.00010681152343750000   
  2, 0.00030899047851562500   0.00041580200195312500   
  3, 0.00077247619628906250   0.0011882781982421875    
  4, 0.0015932321548461914    0.0027815103530883789    
  5, 0.0028678178787231445    0.0056493282318115234    
  6, 0.0046602040529251099    0.010309532284736633     
  7, 0.0069903060793876648    0.017299838364124298     
  8, 0.0098301179241389036    0.027129956288263202     
  9, 0.013106823898851871     0.040236780187115073     
 10, 0.016711200471036136     0.056947980658151209     
 11, 0.020509200578089803     0.077457181236241013     
 12, 0.024354675686481642     0.10181185692272265      
 13, 0.028101548869017279     0.12991340579173993      
 14, 0.031614242477644439     0.16152764826938437      
 15, 0.034775666725408882     0.19630331499479325      
 16, 0.037492515688331451     0.23379583068312471      
 17, 0.039697957787645066     0.27349378847076977      
 18, 0.041352039362130277     0.31484582783290005      
 19, 0.042440250924291600     0.35728607875719165      
 20, 0.042970754060845245     0.40025683281803692      
 21, 0.042970754060845245     0.44322758687888214      
 22, 0.042482450037426553     0.48571003691630871      
 23, 0.041558918514873797     0.52726895543118246      
 24, 0.040260202311283994     0.56752915774246648      
 25, 0.038649794218832634     0.60617895196129912      
 26, 0.036791631035234910     0.64297058299653398      
 27, 0.034747651533277413     0.67771823452981139      
 28, 0.032575923312447574     0.71029415784225902      
 29, 0.030329307911589123     0.74062346575384819      
 30, 0.028054609818219937     0.76867807557206802      
 31, 0.025792141284492524     0.79447021685656061      
 32, 0.023575629142856450     0.81804584599941710      
 33, 0.021432390129869500     0.83947823612928651      
 34, 0.019383705779220207     0.85886194190850673      
 35, 0.017445335201298186     0.87630727710980494      
 36, 0.015628112784496294     0.89193538989430121      
 37, 0.013938587078064260     0.90587397697236549      
 38, 0.012379666154859706     0.91825364312722524      
 39, 0.010951243136991277     0.92920488626421649      
 40, 0.0096507830144735626    0.93885566927869002      
 41, 0.0084738582566109329    0.94732952753530097      
 42, 0.0074146259745345678    0.95474415350983555      
 43, 0.0064662435824429359    0.96121039709227851      
 44, 0.0056212231142827801    0.96683162020656122      
 45, 0.0048717266990450760    0.97170334690560634      
 46, 0.0042098073105878648    0.97591315421619418      
 47, 0.0036275999165703938    0.97954075413276465      
 48, 0.0031174686783026818    0.98265822281106729      
 49, 0.0026721160099737276    0.98533033882104104      
 50, 0.0022846591885275370    0.98761499800956853      
 51, 0.0019486798960970169    0.98956367790566557      
 52, 0.0016582516423517886    0.99122192954801736      
 53, 0.0014079495076571790    0.99262987905567457      
 54, 0.0011928461106539988    0.99382272516632852      
 55, 0.0010084971662801989    0.99483122233260868      
 56, 0.00085091948404891792   0.99568214181665760      
 57, 0.00071656377604119401   0.99639870559269883      
 58, 0.00060228420831048640   0.99700098980100937      
 59, 0.00050530624256557750   0.99750629604357488      
 60, 0.00042319397814867120   0.99792949002172360      
 61, 0.00035381791615708577   0.99828330793788067      
 62, 0.00029532382517950302   0.99857863176306016      
 63, 0.00024610318764958588   0.99882473495070978      

)ex"));
}
}  // namespace ex_negative_binomial_example2

// libs/math/example/nonfinite_facet_sstream.cpp
namespace ex_nonfinite_facet_sstream {
using boost::math::nonfinite_num_put;
using boost::math::nonfinite_num_get;

using boost::math::legacy;


using std::endl;
using std::locale;

using std::stringstream;
using std::istringstream;
using std::ostringstream;

using std::numeric_limits;


int example_main()
{
  //[nonfinite_facets_sstream_1
  locale old_locale;
  locale tmp_locale(old_locale, new nonfinite_num_put<char>);
  locale new_locale(tmp_locale, new nonfinite_num_get<char>);
  //] [/nonfinite_facets_sstream_1]

  // Note that to add two facets,  nonfinite_num_put and nonfinite_num_get,
  // you have to add one at a time, using a temporary locale.

  {
    ostringstream oss;
    oss.imbue(new_locale);
    double inf = numeric_limits<double>::infinity();
    oss << inf; // Write out.
    ex_out << "infinity output was " << oss.str() << endl;
    BOOST_MATH_ASSERT(oss.str() == "inf");
  }
  {
    istringstream iss;
    iss.str("inf");
    iss.imbue(new_locale);
    double inf;
    iss >> inf; // Read from "inf"
    ex_out << "Infinity input was " << iss.str() << endl;
    BOOST_MATH_ASSERT(inf == numeric_limits<double>::infinity());
  }

  {
    //[nonfinite_facets_sstream_2
    stringstream ss;
    ss.imbue(new_locale);
    double inf = numeric_limits<double>::infinity();
    ss << inf; // Write out.
    BOOST_MATH_ASSERT(ss.str() == "inf");
    double r;
    ss >> r; // Read back in.
    BOOST_MATH_ASSERT(inf == r); // Confirms that the double values really are identical.

    ex_out << "infinity output was " << ss.str() << endl;
    ex_out << "infinity input was " << r << endl;
    // But the string representation of r displayed will be the native type
    // because, when it was constructed, ex_out had NOT been imbued
    // with the new locale containing the nonfinite_numput facet.
    // So the ex_out output will be "1.#INF on MS platforms
    // and may be "inf" or other string representation on other platforms.

    //] [/nonfinite_facets_sstream_2]
  }

  {
    stringstream ss;
    ss.imbue(new_locale);

    double nan = numeric_limits<double>::quiet_NaN();
    ss << nan; // Write out.
    BOOST_MATH_ASSERT(ss.str() == "nan");

    double v;
    ss >> v; // Read back in.

    ex_out << "NaN output was " << ss.str() << endl;
    ex_out << "NaN input was " << v << endl;

    // assert(nan == v); // Always fails because NaN == NaN fails!
    // assert(nan == numeric_limits<double>::quiet_NaN()); asserts!

    // And the string representation will be the native type
    // because ex_out has NOT been imbued with a locale containing
    // the nonfinite_numput facet.
    // So the output will be "1.#QNAN on MS platforms
    // and may be "nan" or other string representation on other platforms.
  }

    return 0;  // main's implicit return
} // int example_main()


/*
//[nonfinite_facet_sstream_output

infinity output was inf
Infinity input was inf
infinity output was inf
infinity input was 1.#INF
NaN output was nan
NaN input was 1.#QNAN

//] [nonfinite_facet_sstream_output]
*/

TEST_CASE("example: nonfinite_facet_sstream") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(infinity output was inf
Infinity input was inf
infinity output was inf
infinity input was inf
NaN output was nan
NaN input was nan
)ex"));
}
}  // namespace ex_nonfinite_facet_sstream

// libs/math/example/normal_misc_examples.cpp
namespace ex_normal_misc_examples {
  using boost::math::normal; // typedef provides default type is double.

   using std::endl; using std::left; using std::showpoint; using std::noshowpoint;
  using std::setw; using std::setprecision;
  using std::numeric_limits;

int example_main()
{
  ex_out << "Example: Normal distribution, Miscellaneous Applications.";

  try
  {
    { // Traditional tables and values.
/*`Let's start by printing some traditional tables.
*/
      double step = 1.; // in z
      double range = 4; // min and max z = -range to +range.
      int precision = 17; // traditional tables are only computed to much lower precision.
      // but std::numeric_limits<double>::max_digits10; on new Standard Libraries gives
      // 17, the maximum number of digits that can possibly be significant.
      // std::numeric_limits<double>::digits10; == 15 is number of guaranteed digits,
      // the other two digits being 'noisy'.

      // Construct a standard normal distribution s
        normal s; // (default mean = zero, and standard deviation = unity)
        ex_out << "Standard normal distribution, mean = "<< s.mean()
          << ", standard deviation = " << s.standard_deviation() << endl;

/*` First the probability distribution function (pdf).
*/
      ex_out << "Probability distribution function values" << endl;
      ex_out << "  z " "      pdf " << endl;
      ex_out.precision(5);
      for (double z = -range; z < range + step; z += step)
      {
        ex_out << left << setprecision(3) << setw(6) << z << " "
          << setprecision(precision) << setw(12) << pdf(s, z) << endl;
      }
      ex_out.precision(6); // default
      /*`And the area under the normal curve from -[infin] up to z,
      the cumulative distribution function (cdf).
*/
      // For a standard normal distribution
      ex_out << "Standard normal mean = "<< s.mean()
        << ", standard deviation = " << s.standard_deviation() << endl;
      ex_out << "Integral (area under the curve) from - infinity up to z " << endl;
      ex_out << "  z " "      cdf " << endl;
      for (double z = -range; z < range + step; z += step)
      {
        ex_out << left << setprecision(3) << setw(6) << z << " "
          << setprecision(precision) << setw(12) << cdf(s, z) << endl;
      }
      ex_out.precision(6); // default

/*`And all this you can do with a nanoscopic amount of work compared to
the team of *human computers* toiling with Milton Abramovitz and Irene Stegen
at the US National Bureau of Standards (now [@http://www.nist.gov NIST]).
Starting in 1938, their "Handbook of Mathematical Functions with Formulas, Graphs and Mathematical Tables",
was eventually published in 1964, and has been reprinted numerous times since.
(A major replacement is planned at [@http://dlmf.nist.gov Digital Library of Mathematical Functions]).

Pretty-printing a traditional 2-dimensional table is left as an exercise for the student,
but why bother now that the Math Toolkit lets you write
*/
    double z = 2.;
    ex_out << "Area for z = " << z << " is " << cdf(s, z) << endl; // to get the area for z.
/*`
Correspondingly, we can obtain the traditional 'critical' values for significance levels.
For the 95% confidence level, the significance level usually called alpha,
is 0.05 = 1 - 0.95 (for a one-sided test), so we can write
*/
     ex_out << "95% of area has a z below " << quantile(s, 0.95) << endl;
   // 95% of area has a z below 1.64485
/*`and a two-sided test (a comparison between two levels, rather than a one-sided test)

*/
     ex_out << "95% of area has a z between " << quantile(s, 0.975)
       << " and " << -quantile(s, 0.975) << endl;
   // 95% of area has a z between 1.95996 and -1.95996
/*`

First, define a table of significance levels: these are the probabilities
that the true occurrence frequency lies outside the calculated interval.

It is convenient to have an alpha level for the probability that z lies outside just one standard deviation.
This will not be some nice neat number like 0.05, but we can easily calculate it,
*/
    double alpha1 = cdf(s, -1) * 2; // 0.3173105078629142
    ex_out << setprecision(17) << "Significance level for z == 1 is " << alpha1 << endl;
/*`
    and place in our array of favorite alpha values.
*/
    double alpha[] = {0.3173105078629142, // z for 1 standard deviation.
      0.20, 0.1, 0.05, 0.01, 0.001, 0.0001, 0.00001 };
/*`

Confidence value as % is (1 - alpha) * 100 (so alpha 0.05 == 95% confidence)
that the true occurrence frequency lies *inside* the calculated interval.

*/
    ex_out << "level of significance (alpha)" << setprecision(4) << endl;
    ex_out << "2-sided       1 -sided          z(alpha) " << endl;
    for (unsigned i = 0; i < sizeof(alpha)/sizeof(alpha[0]); ++i)
    {
      ex_out << setw(15) << alpha[i] << setw(15) << alpha[i] /2 << setw(10) << quantile(complement(s,  alpha[i]/2)) << endl;
      // Use quantile(complement(s, alpha[i]/2)) to avoid potential loss of accuracy from quantile(s,  1 - alpha[i]/2)
    }
    ex_out << endl;

/*`Notice the distinction between one-sided (also called one-tailed)
where we are using a > *or* < test (and not both)
and considering the area of the tail (integral) from z up to +[infin],
and a two-sided test where we are using two > *and* < tests, and thus considering two tails,
from -[infin] up to z low and z high up to +[infin].

So the 2-sided values alpha[i] are calculated using alpha[i]/2.

If we consider a simple example of alpha = 0.05, then for a two-sided test,
the lower tail area from -[infin] up to -1.96 is 0.025 (alpha/2)
and the upper tail area from +z up to +1.96 is also  0.025 (alpha/2),
and the area between -1.96 up to 12.96 is alpha = 0.95.
and the sum of the two tails is 0.025 + 0.025 = 0.05,

*/
//] [/[normal_basic1]

//[normal_basic2

/*`Armed with the cumulative distribution function, we can easily calculate the
easy to remember proportion of values that lie within 1, 2 and 3 standard deviations from the mean.

*/
    ex_out.precision(3);
    ex_out << showpoint << "cdf(s, s.standard_deviation()) = "
      << cdf(s, s.standard_deviation()) << endl;  // from -infinity to 1 sd
    ex_out << "cdf(complement(s, s.standard_deviation())) = "
      << cdf(complement(s, s.standard_deviation())) << endl;
    ex_out << "Fraction 1 standard deviation within either side of mean is "
      << 1 -  cdf(complement(s, s.standard_deviation())) * 2 << endl;
    ex_out << "Fraction 2 standard deviations within either side of mean is "
      << 1 -  cdf(complement(s, 2 * s.standard_deviation())) * 2 << endl;
    ex_out << "Fraction 3 standard deviations within either side of mean is "
      << 1 -  cdf(complement(s, 3 * s.standard_deviation())) * 2 << endl;

/*`
To a useful precision, the 1, 2 & 3 percentages are 68, 95 and 99.7,
and these are worth memorising as useful 'rules of thumb', as, for example, in
[@http://en.wikipedia.org/wiki/Standard_deviation standard deviation]:

[pre
Fraction 1 standard deviation within either side of mean is 0.683
Fraction 2 standard deviations within either side of mean is 0.954
Fraction 3 standard deviations within either side of mean is 0.997
]

We could of course get some really accurate values for these
[@http://en.wikipedia.org/wiki/Confidence_interval confidence intervals]
by using ex_out.precision(15);

[pre
Fraction 1 standard deviation within either side of mean is 0.682689492137086
Fraction 2 standard deviations within either side of mean is 0.954499736103642
Fraction 3 standard deviations within either side of mean is 0.997300203936740
]

But before you get too excited about this impressive precision,
don't forget that the *confidence intervals of the standard deviation* are surprisingly wide,
especially if you have estimated the standard deviation from only a few measurements.
*/
//] [/[normal_basic2]


//[normal_bulbs_example1
/*`
Examples from K. Krishnamoorthy, Handbook of Statistical Distributions with Applications,
ISBN 1 58488 635 8, page 125... implemented using the Math Toolkit library.

A few very simple examples are shown here:
*/
// K. Krishnamoorthy, Handbook of Statistical Distributions with Applications,
 // ISBN 1 58488 635 8, page 125, example 10.3.5
/*`Mean lifespan of 100 W bulbs is 1100 h with standard deviation of 100 h.
Assuming, perhaps with little evidence and much faith, that the distribution is normal,
we construct a normal distribution called /bulbs/ with these values:
*/
    double mean_life = 1100.;
    double life_standard_deviation = 100.;
    normal bulbs(mean_life, life_standard_deviation);
    double expected_life = 1000.;

/*`The we can use the Cumulative distribution function to predict fractions
(or percentages, if * 100) that will last various lifetimes.
*/
    ex_out << "Fraction of bulbs that will last at best (<=) " // P(X <= 1000)
      << expected_life << " is "<< cdf(bulbs, expected_life) << endl;
    ex_out << "Fraction of bulbs that will last at least (>) " // P(X > 1000)
      << expected_life << " is "<< cdf(complement(bulbs, expected_life)) << endl;
    double min_life = 900;
    double max_life = 1200;
    ex_out << "Fraction of bulbs that will last between "
      << min_life << " and " << max_life << " is "
      << cdf(bulbs, max_life)  // P(X <= 1200)
       - cdf(bulbs, min_life) << endl; // P(X <= 900)
/*`
[note Real-life failures are often very ab-normal,
with a significant number that 'dead-on-arrival' or suffer failure very early in their life:
the lifetime of the survivors of 'early mortality' may be well described by the normal distribution.]
*/
//] [/normal_bulbs_example1 Quickbook end]
  }
  {
    // K. Krishnamoorthy, Handbook of Statistical Distributions with Applications,
    // ISBN 1 58488 635 8, page 125, Example 10.3.6

//[normal_bulbs_example3
/*`Weekly demand for 5 lb sacks of onions at a store is normally distributed with mean 140 sacks and standard deviation 10.
*/
  double mean = 140.; // sacks per week.
  double standard_deviation = 10;
  normal sacks(mean, standard_deviation);

  double stock = 160.; // per week.
  ex_out << "Percentage of weeks overstocked "
    << cdf(sacks, stock) * 100. << endl; // P(X <=160)
  // Percentage of weeks overstocked 97.7

/*`So there will be lots of mouldy onions!
So we should be able to say what stock level will meet demand 95% of the weeks.
*/
  double stock_95 = quantile(sacks, 0.95);
  ex_out << "Store should stock " << int(stock_95) << " sacks to meet 95% of demands." << endl;
/*`And it is easy to estimate how to meet 80% of demand, and waste even less.
*/
  double stock_80 = quantile(sacks, 0.80);
  ex_out << "Store should stock " << int(stock_80) << " sacks to meet 8 out of 10 demands." << endl;
//] [/normal_bulbs_example3 Quickbook end]
  }
  { // K. Krishnamoorthy, Handbook of Statistical Distributions with Applications,
    // ISBN 1 58488 635 8, page 125, Example 10.3.7

//[normal_bulbs_example4

/*`A machine is set to pack 3 kg of ground beef per pack.
Over a long period of time it is found that the average packed was 3 kg
with a standard deviation of 0.1 kg.
Assuming the packing is normally distributed,
we can find the fraction (or %) of packages that weigh more than 3.1 kg.
*/

double mean = 3.; // kg
double standard_deviation = 0.1; // kg
normal packs(mean, standard_deviation);

double max_weight = 3.1; // kg
ex_out << "Percentage of packs > " << max_weight << " is "
<< cdf(complement(packs, max_weight)) << endl; // P(X > 3.1)

double under_weight = 2.9;
ex_out <<"fraction of packs <= " << under_weight << " with a mean of " << mean
  << " is " << cdf(complement(packs, under_weight)) << endl;
// fraction of packs <= 2.9 with a mean of 3 is 0.841345
// This is 0.84 - more than the target 0.95
// Want 95% to be over this weight, so what should we set the mean weight to be?
// KK StatCalc says:
double over_mean = 3.0664;
normal xpacks(over_mean, standard_deviation);
ex_out << "fraction of packs >= " << under_weight
<< " with a mean of " << xpacks.mean()
  << " is " << cdf(complement(xpacks, under_weight)) << endl;
// fraction of packs >= 2.9 with a mean of 3.06449 is 0.950005
double under_fraction = 0.05;  // so 95% are above the minimum weight mean - sd = 2.9
double low_limit = standard_deviation;
double offset = mean - low_limit - quantile(packs, under_fraction);
double nominal_mean = mean + offset;

normal nominal_packs(nominal_mean, standard_deviation);
ex_out << "Setting the packer to " << nominal_mean << " will mean that "
  << "fraction of packs >= " << under_weight
  << " is " << cdf(complement(nominal_packs, under_weight)) << endl;

/*`
Setting the packer to 3.06449 will mean that fraction of packs >= 2.9 is 0.95.

Setting the packer to 3.13263 will mean that fraction of packs >= 2.9 is 0.99,
but will more than double the mean loss from 0.0644 to 0.133.

Alternatively, we could invest in a better (more precise) packer with a lower standard deviation.

To estimate how much better (how much smaller standard deviation) it would have to be,
we need to get the 5% quantile to be located at the under_weight limit, 2.9
*/
double p = 0.05; // wanted p th quantile.
ex_out << "Quantile of " << p << " = " << quantile(packs, p)
  << ", mean = " << packs.mean() << ", sd = " << packs.standard_deviation() << endl; //
/*`
Quantile of 0.05 = 2.83551, mean = 3, sd = 0.1

With the current packer (mean = 3, sd = 0.1), the 5% quantile is at 2.8551 kg,
a little below our target of 2.9 kg.
So we know that the standard deviation is going to have to be smaller.

Let's start by guessing that it (now 0.1) needs to be halved, to a standard deviation of 0.05
*/
normal pack05(mean, 0.05);
ex_out << "Quantile of " << p << " = " << quantile(pack05, p)
  << ", mean = " << pack05.mean() << ", sd = " << pack05.standard_deviation() << endl;

ex_out <<"Fraction of packs >= " << under_weight << " with a mean of " << mean
  << " and standard deviation of " << pack05.standard_deviation()
  << " is " << cdf(complement(pack05, under_weight)) << endl;
//
/*`
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.05 is 0.9772

So 0.05 was quite a good guess, but we are a little over the 2.9 target,
so the standard deviation could be a tiny bit more. So we could do some
more guessing to get closer, say by increasing to 0.06
*/

normal pack06(mean, 0.06);
ex_out << "Quantile of " << p << " = " << quantile(pack06, p)
  << ", mean = " << pack06.mean() << ", sd = " << pack06.standard_deviation() << endl;

ex_out <<"Fraction of packs >= " << under_weight << " with a mean of " << mean
  << " and standard deviation of " << pack06.standard_deviation()
  << " is " << cdf(complement(pack06, under_weight)) << endl;
/*`
Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.06 is 0.9522

Now we are getting really close, but to do the job properly,
we could use root finding method, for example the tools provided, and used elsewhere,
in the Math Toolkit, see __root_finding_without_derivatives.

But in this normal distribution case, we could be even smarter and make a direct calculation.
*/

normal s; // For standard normal distribution,
double sd = 0.1;
double x = 2.9; // Our required limit.
// then probability p = N((x - mean) / sd)
// So if we want to find the standard deviation that would be required to meet this limit,
// so that the p th quantile is located at x,
// in this case the 0.95 (95%) quantile at 2.9 kg pack weight, when the mean is 3 kg.

double prob =  pdf(s, (x - mean) / sd);
double qp = quantile(s, 0.95);
ex_out << "prob = " << prob << ", quantile(p) " << qp << endl; // p = 0.241971, quantile(p) 1.64485
// Rearranging, we can directly calculate the required standard deviation:
double sd95 = std::abs((x - mean)) / qp;

ex_out << "If we want the "<< p << " th quantile to be located at "
  << x << ", would need a standard deviation of " << sd95 << endl;

normal pack95(mean, sd95);  // Distribution of the 'ideal better' packer.
ex_out <<"Fraction of packs >= " << under_weight << " with a mean of " << mean
  << " and standard deviation of " << pack95.standard_deviation()
  << " is " << cdf(complement(pack95, under_weight)) << endl;

// Fraction of packs >= 2.9 with a mean of 3 and standard deviation of 0.0608 is 0.95

/*`Notice that these two deceptively simple questions
(do we over-fill or measure better) are actually very common.
The weight of beef might be replaced by a measurement of more or less anything.
But the calculations rely on the accuracy of the standard deviation - something
that is almost always less good than we might wish,
especially if based on a few measurements.
*/

//] [/normal_bulbs_example4 Quickbook end]
  }

  { // K. Krishnamoorthy, Handbook of Statistical Distributions with Applications,
    // ISBN 1 58488 635 8, page 125, example 10.3.8
//[normal_bulbs_example5
/*`A bolt is usable if between 3.9 and 4.1 long.
From a large batch of bolts, a sample of 50 show a
mean length of 3.95 with standard deviation 0.1.
Assuming a normal distribution, what proportion is usable?
The true sample mean is unknown,
but we can use the sample mean and standard deviation to find approximate solutions.
*/

    normal bolts(3.95, 0.1);
    double top = 4.1;
    double bottom = 3.9;

ex_out << "Fraction long enough [ P(X <= " << top << ") ] is " << cdf(bolts, top) << endl;
ex_out << "Fraction too short [ P(X <= " << bottom << ") ] is " << cdf(bolts, bottom) << endl;
ex_out << "Fraction OK  -between " << bottom << " and " << top
  << "[ P(X <= " << top  << ") - P(X<= " << bottom << " ) ] is "
  << cdf(bolts, top) - cdf(bolts, bottom) << endl;

ex_out << "Fraction too long [ P(X > " << top << ") ] is "
  << cdf(complement(bolts, top)) << endl;

ex_out << "95% of bolts are shorter than " << quantile(bolts, 0.95) << endl;

//] [/normal_bulbs_example5 Quickbook end]
  }
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

Autorun "i:\boost-06-05-03-1300\libs\math\test\Math_test\debug\normal_misc_examples.exe"
Example: Normal distribution, Miscellaneous Applications.Standard normal distribution, mean = 0, standard deviation = 1
Probability distribution function values
  z       pdf
-4     0.00013383022576488537
-3     0.0044318484119380075
-2     0.053990966513188063
-1     0.24197072451914337
0      0.3989422804014327
1      0.24197072451914337
2      0.053990966513188063
3      0.0044318484119380075
4      0.00013383022576488537
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z
  z       cdf
-4     3.1671241833119979e-005
-3     0.0013498980316300959
-2     0.022750131948179219
-1     0.1586552539314571
0      0.5
1      0.84134474606854293
2      0.97724986805182079
3      0.9986501019683699
4      0.99996832875816688
Area for z = 2 is 0.97725
95% of area has a z below 1.64485
95% of area has a z between 1.95996 and -1.95996
Significance level for z == 1 is 0.3173105078629142
level of significance (alpha)
2-sided       1 -sided          z(alpha)
0.3173         0.1587         1
0.2            0.1            1.282
0.1            0.05           1.645
0.05           0.025          1.96
0.01           0.005          2.576
0.001          0.0005         3.291
0.0001         5e-005         3.891
1e-005         5e-006         4.417
cdf(s, s.standard_deviation()) = 0.841
cdf(complement(s, s.standard_deviation())) = 0.159
Fraction 1 standard deviation within either side of mean is 0.683
Fraction 2 standard deviations within either side of mean is 0.954
Fraction 3 standard deviations within either side of mean is 0.997
Fraction of bulbs that will last at best (<=) 1.00e+003 is 0.159
Fraction of bulbs that will last at least (>) 1.00e+003 is 0.841
Fraction of bulbs that will last between 900. and 1.20e+003 is 0.819
Percentage of weeks overstocked 97.7
Store should stock 156 sacks to meet 95% of demands.
Store should stock 148 sacks to meet 8 out of 10 demands.
Percentage of packs > 3.10 is 0.159
fraction of packs <= 2.90 with a mean of 3.00 is 0.841
fraction of packs >= 2.90 with a mean of 3.07 is 0.952
Setting the packer to 3.06 will mean that fraction of packs >= 2.90 is 0.950
Quantile of 0.0500 = 2.84, mean = 3.00, sd = 0.100
Quantile of 0.0500 = 2.92, mean = 3.00, sd = 0.0500
Fraction of packs >= 2.90 with a mean of 3.00 and standard deviation of 0.0500 is 0.977
Quantile of 0.0500 = 2.90, mean = 3.00, sd = 0.0600
Fraction of packs >= 2.90 with a mean of 3.00 and standard deviation of 0.0600 is 0.952
prob = 0.242, quantile(p) 1.64
If we want the 0.0500 th quantile to be located at 2.90, would need a standard deviation of 0.0608
Fraction of packs >= 2.90 with a mean of 3.00 and standard deviation of 0.0608 is 0.950
Fraction long enough [ P(X <= 4.10) ] is 0.933
Fraction too short [ P(X <= 3.90) ] is 0.309
Fraction OK  -between 3.90 and 4.10[ P(X <= 4.10) - P(X<= 3.90 ) ] is 0.625
Fraction too long [ P(X > 4.10) ] is 0.0668
95% of bolts are shorter than 4.11

*/

TEST_CASE("example: normal_misc_examples") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example: Normal distribution, Miscellaneous Applications.Standard normal distribution, mean = 0, standard deviation = 1
Probability distribution function values
  z       pdf 
-4     0.00013383022576488537
-3     0.0044318484119380075
-2     0.053990966513188063
-1     0.24197072451914337
0      0.3989422804014327
1      0.24197072451914337
2      0.053990966513188063
3      0.0044318484119380075
4      0.00013383022576488537
Standard normal mean = 0, standard deviation = 1
Integral (area under the curve) from - infinity up to z 
  z       cdf 
-4     3.1671241833119972e-05
-3     0.0013498980316300957
-2     0.022750131948179216
-1     0.15865525393145705
0      0.5         
1      0.84134474606854293
2      0.97724986805182079
3      0.9986501019683699
4      0.99996832875816688
Area for z = 2 is 0.97725
95% of area has a z below 1.64485
95% of area has a z between 1.95996 and -1.95996
Significance level for z == 1 is 0.31731050786291409
level of significance (alpha)
2-sided       1 -sided          z(alpha) 
0.3173         0.1587         1         
0.2            0.1            1.282     
0.1            0.05           1.645     
0.05           0.025          1.96      
0.01           0.005          2.576     
0.001          0.0005         3.291     
0.0001         5e-05          3.891     
1e-05          5e-06          4.417     

cdf(s, s.standard_deviation()) = 0.841
cdf(complement(s, s.standard_deviation())) = 0.159
Fraction 1 standard deviation within either side of mean is 0.683
Fraction 2 standard deviations within either side of mean is 0.954
Fraction 3 standard deviations within either side of mean is 0.997
Fraction of bulbs that will last at best (<=) 1.00e+03 is 0.159
Fraction of bulbs that will last at least (>) 1.00e+03 is 0.841
Fraction of bulbs that will last between 900. and 1.20e+03 is 0.819
Percentage of weeks overstocked 97.7
Store should stock 156 sacks to meet 95% of demands.
Store should stock 148 sacks to meet 8 out of 10 demands.
Percentage of packs > 3.10 is 0.159
fraction of packs <= 2.90 with a mean of 3.00 is 0.841
fraction of packs >= 2.90 with a mean of 3.07 is 0.952
Setting the packer to 3.06 will mean that fraction of packs >= 2.90 is 0.950
Quantile of 0.0500 = 2.84, mean = 3.00, sd = 0.100
Quantile of 0.0500 = 2.92, mean = 3.00, sd = 0.0500
Fraction of packs >= 2.90 with a mean of 3.00 and standard deviation of 0.0500 is 0.977
Quantile of 0.0500 = 2.90, mean = 3.00, sd = 0.0600
Fraction of packs >= 2.90 with a mean of 3.00 and standard deviation of 0.0600 is 0.952
prob = 0.242, quantile(p) 1.64
If we want the 0.0500 th quantile to be located at 2.90, would need a standard deviation of 0.0608
Fraction of packs >= 2.90 with a mean of 3.00 and standard deviation of 0.0608 is 0.950
Fraction long enough [ P(X <= 4.10) ] is 0.933
Fraction too short [ P(X <= 3.90) ] is 0.309
Fraction OK  -between 3.90 and 4.10[ P(X <= 4.10) - P(X<= 3.90 ) ] is 0.625
Fraction too long [ P(X > 4.10) ] is 0.0668
95% of bolts are shorter than 4.11
)ex"));
}
}  // namespace ex_normal_misc_examples

// libs/math/example/ooura_fourier_integrals_example.cpp
namespace ex_ooura_fourier_integrals_example {
#ifdef BOOST_NO_CXX11_LAMBDAS
#  error "This example requires a C++11 compiler that supports lambdas.  Try C++11 or later."
#endif

//#define BOOST_MATH_INSTRUMENT_OOURA // or -DBOOST_MATH_INSTRUMENT_OOURA etc for diagnostics.



int example_main()
{
  try
  {
  ex_out.precision(std::numeric_limits<double>::max_digits10); // Show all potentially significant digits.

  using boost::math::quadrature::ooura_fourier_sin;
  using boost::math::constants::half_pi;

//[ooura_fourier_integrals_example_1
  ooura_fourier_sin<double>integrator = ooura_fourier_sin<double>();
  // Use the default tolerance root_epsilon and eight levels for type double.

  auto f = [](double x)
  { // Simple reciprocal function for sinc.
    return 1 / x;
  };

  double omega = 1;
  std::pair<double, double> result = integrator.integrate(f, omega);
  ex_out << "Integral = " << result.first << ", relative error estimate " << result.second << std::endl;

//] [/ooura_fourier_integrals_example_1]

//[ooura_fourier_integrals_example_2

  constexpr double expected = half_pi<double>();
  ex_out << "pi/2 =     " << expected << ", difference " << result.first - expected << std::endl;
//] [/ooura_fourier_integrals_example_2]
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

//[ooura_fourier_integrals_example_output_1

integral = 1.5707963267948966, relative error estimate 1.2655356398390254e-11
pi/2 =     1.5707963267948966, difference 0

//] [/ooura_fourier_integrals_example_output_1]


//[ooura_fourier_integrals_example_diagnostic_output_1

ooura_fourier_sin with relative error goal 1.4901161193847656e-08 & 8 levels.
h = 1.000000000000000, I_h = 1.571890732004545 = 0x1.92676e56d853500p+0, absolute error estimate = nan
h = 0.500000000000000, I_h = 1.570793292491940 = 0x1.921f825c076f600p+0, absolute error estimate = 1.097439512605325e-03
h = 0.250000000000000, I_h = 1.570796326814776 = 0x1.921fb54458acf00p+0, absolute error estimate = 3.034322835882008e-06
h = 0.125000000000000, I_h = 1.570796326794897 = 0x1.921fb54442d1800p+0, absolute error estimate = 1.987898734512328e-11
Integral = 1.570796326794897e+00, relative error estimate 1.265535639839025e-11
pi/2 =     1.570796326794897e+00, difference 0.000000000000000e+00

//] [/ooura_fourier_integrals_example_diagnostic_output_1]

*/

TEST_CASE("example: ooura_fourier_integrals_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Integral = 1.5707963267948966, relative error estimate 1.2655215040404414e-11
pi/2 =     1.5707963267948966, difference 0
)ex"));
}
}  // namespace ex_ooura_fourier_integrals_example

// libs/math/example/policy_eg_10.cpp
namespace ex_policy_eg_10 {
 using std::endl;
using std::left; using std::fixed; using std::right; using std::scientific;
using std::setw;
using std::setprecision;

/*`

Next we'll bring the needed declarations into scope, and
define distribution types for all the available rounding policies:

*/
// Avoid 
// using namespace std; // and 
// using namespace boost::math;
// to avoid potential ambiguity of names, like binomial.
// using namespace boost::math::policies; is small risk, but
// the necessary items are brought into scope thus:

using boost::math::binomial_distribution;
using boost::math::policies::policy;
using boost::math::policies::discrete_quantile;

using boost::math::policies::integer_round_outwards;
using boost::math::policies::integer_round_down;
using boost::math::policies::integer_round_up;
using boost::math::policies::integer_round_nearest;
using boost::math::policies::integer_round_inwards;
using boost::math::policies::real;

using boost::math::binomial_distribution; // Not std::binomial_distribution.

typedef binomial_distribution<
            double, 
            policy<discrete_quantile<integer_round_outwards> > > 
        binom_round_outwards;

typedef binomial_distribution<
            double, 
            policy<discrete_quantile<integer_round_inwards> > > 
        binom_round_inwards;

typedef binomial_distribution<
            double, 
            policy<discrete_quantile<integer_round_down> > > 
        binom_round_down;

typedef binomial_distribution<
            double, 
            policy<discrete_quantile<integer_round_up> > > 
        binom_round_up;

typedef binomial_distribution<
            double, 
            policy<discrete_quantile<integer_round_nearest> > > 
        binom_round_nearest;

typedef binomial_distribution<
            double, 
            policy<discrete_quantile<real> > > 
        binom_real_quantile;

/*`
Now let's set to work calling those quantiles:
*/

int example_main()
{
   ex_out << 
      "Testing rounding policies for a 50 sample binomial distribution,\n"
      "with a success fraction of 0.5.\n\n"
      "Lower quantiles are calculated at p = 0.05\n\n"
      "Upper quantiles at p = 0.95.\n\n";

   ex_out << setw(25) << right
      << "Policy"<< setw(18) << right 
      << "Lower Quantile" << setw(18) << right 
      << "Upper Quantile" << endl;
   
   // Test integer_round_outwards:
   ex_out << setw(25) << right
      << "integer_round_outwards"
      << setw(18) << right
      << quantile(binom_round_outwards(50, 0.5), 0.05)
      << setw(18) << right
      << quantile(binom_round_outwards(50, 0.5), 0.95) 
      << endl;
   
   // Test integer_round_inwards:
   ex_out << setw(25) << right
      << "integer_round_inwards"
      << setw(18) << right
      << quantile(binom_round_inwards(50, 0.5), 0.05)
      << setw(18) << right
      << quantile(binom_round_inwards(50, 0.5), 0.95) 
      << endl;
   
   // Test integer_round_down:
   ex_out << setw(25) << right
      << "integer_round_down"
      << setw(18) << right
      << quantile(binom_round_down(50, 0.5), 0.05)
      << setw(18) << right
      << quantile(binom_round_down(50, 0.5), 0.95) 
      << endl;
   
   // Test integer_round_up:
   ex_out << setw(25) << right
      << "integer_round_up"
      << setw(18) << right
      << quantile(binom_round_up(50, 0.5), 0.05)
      << setw(18) << right
      << quantile(binom_round_up(50, 0.5), 0.95) 
      << endl;
   
   // Test integer_round_nearest:
   ex_out << setw(25) << right
      << "integer_round_nearest"
      << setw(18) << right
      << quantile(binom_round_nearest(50, 0.5), 0.05)
      << setw(18) << right
      << quantile(binom_round_nearest(50, 0.5), 0.95) 
      << endl;
   
   // Test real:
   ex_out << setw(25) << right
      << "real"
      << setw(18) << right
      << quantile(binom_real_quantile(50, 0.5), 0.05)
      << setw(18) << right
      << quantile(binom_real_quantile(50, 0.5), 0.95) 
      << endl;
    return 0;  // main's implicit return
} // int example_main()

/*`

Which produces the program output:

[pre
  policy_eg_10.vcxproj -> J:\Cpp\MathToolkit\test\Math_test\Release\policy_eg_10.exe
  Testing rounding policies for a 50 sample binomial distribution,
  with a success fraction of 0.5.
  
  Lower quantiles are calculated at p = 0.05
  
  Upper quantiles at p = 0.95.
  
                     Policy    Lower Quantile    Upper Quantile
     integer_round_outwards                18                31
      integer_round_inwards                19                30
         integer_round_down                18                30
           integer_round_up                19                31
      integer_round_nearest                19                30
                       real            18.701            30.299
]

*/

//] //[policy_eg_10] ends quickbook import.

TEST_CASE("example: policy_eg_10") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Testing rounding policies for a 50 sample binomial distribution,
with a success fraction of 0.5.

Lower quantiles are calculated at p = 0.05

Upper quantiles at p = 0.95.

                   Policy    Lower Quantile    Upper Quantile
   integer_round_outwards                18                31
    integer_round_inwards                19                30
       integer_round_down                18                30
         integer_round_up                19                31
    integer_round_nearest                19                30
                     real            18.701            30.299
)ex"));
}
}  // namespace ex_policy_eg_10

// libs/math/example/policy_eg_5.cpp
namespace ex_policy_eg_5 {
  using std::endl;

//[policy_eg_5

// using boost::math::tgamma; // Would create an ambiguity between
// 'double boost::math::tgamma<int>(T)' and
// 'double 'anonymous-namespace'::tgamma<int>(RT)'.

namespace mymath
{ // unnamed

using namespace boost::math::policies;

typedef policy<
   domain_error<errno_on_error>,
   pole_error<errno_on_error>,
   overflow_error<errno_on_error>,
   evaluation_error<errno_on_error>
> c_policy;

BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS(c_policy)

/*`
So that when we call `mymath::tgamma(z)`, we really end up calling
 `boost::math::tgamma(z, anonymous-namespace::c_policy())`.
*/

} // close unnamed namespace

int example_main()
{
   errno = 0;
   ex_out << "Result of tgamma(30000) is: "
      << mymath::tgamma(30000) << endl;
      // tgamma in unnamed namespace in this translation unit (file) only.
   ex_out << "errno = " << errno << endl;
   ex_out << "Result of tgamma(-10) is: "
      << mymath::tgamma(-10) << endl;
   ex_out << "errno = " << errno << endl;
   // Default tgamma policy would throw an exception, and abort.
    return 0;  // main's implicit return
}

//] //[/policy_eg_5]

/*
Output:

  Result of tgamma(30000) is: 1.#INF
  errno = 34
  Result of tgamma(-10) is: 1.#QNAN
  errno = 33


*/

TEST_CASE("example: policy_eg_5") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Result of tgamma(30000) is: inf
errno = 34
Result of tgamma(-10) is: nan
errno = 33
)ex"));
}
}  // namespace ex_policy_eg_5

// libs/math/example/policy_eg_9.cpp: its additions to namespace boost/std, outside the case namespace
namespace boost{ namespace math{
namespace policies
{

template <class T>
T user_domain_error(const char* function, const char* message, const T& val)
{
   /*`
   We'll begin with a bit of defensive programming in case function or message are empty:
   */
   if(function == 0)
       function = "Unknown function with arguments of type %1%";
   if(message == 0)
       message = "Cause unknown with bad argument %1%";
   /*`
   Next we'll format the name of the function with the name of type T, perhaps double:
   */
   std::string msg("Error in function ");
   msg += (boost::format(function) % typeid(T).name()).str();
   /*`
   Then likewise format the error message with the value of parameter /val/,
   making sure we output all the potentially significant digits of /val/:
   */
   msg += ": \n";
   int prec = 2 + (std::numeric_limits<T>::digits * 30103UL) / 100000UL;
   // int prec = std::numeric_limits<T>::max_digits10; //  For C++0X Standard Library
   msg += (boost::format(message) % boost::io::group(std::setprecision(prec), val)).str();
   /*`
   Now we just have to do something with the message, we could throw an
   exception, but for the purposes of this example we'll just dump the message
   to ex_out:
   */
   ex_out << msg << std::endl;
   /*`
   Finally the only sensible value we can return from a domain error is a NaN:
   */
   return std::numeric_limits<T>::quiet_NaN();
}

/*`
Pole errors are essentially a special case of domain errors,
so in this example we'll just return the result of a domain error:
*/

template <class T>
T user_pole_error(const char* function, const char* message, const T& val)
{
   return user_domain_error(function, message, val);
}

/*`
Overflow errors are very similar to domain errors, except that there's
no %1% format specifier in the /message/ parameter:
*/
template <class T>
T user_overflow_error(const char* function, const char* message, const T& val)
{
   if(function == 0)
       function = "Unknown function with arguments of type %1%";
   if(message == 0)
       message = "Result of function is too large to represent";

   std::string msg("Error in function ");
   msg += (boost::format(function) % typeid(T).name()).str();

   msg += ": \n";
   msg += message;

   ex_out << msg << std::endl;

   // Value passed to the function is an infinity, just return it:
   return val;
}

/*`
Underflow errors are much the same as overflow:
*/

template <class T>
T user_underflow_error(const char* function, const char* message, const T& val)
{
   if(function == 0)
       function = "Unknown function with arguments of type %1%";
   if(message == 0)
       message = "Result of function is too small to represent";

   std::string msg("Error in function ");
   msg += (boost::format(function) % typeid(T).name()).str();

   msg += ": \n";
   msg += message;

   ex_out << msg << std::endl;

   // Value passed to the function is zero, just return it:
   return val;
}

/*`
Denormalised results are much the same as underflow:
*/

template <class T>
T user_denorm_error(const char* function, const char* message, const T& val)
{
   if(function == 0)
       function = "Unknown function with arguments of type %1%";
   if(message == 0)
       message = "Result of function is denormalised";

   std::string msg("Error in function ");
   msg += (boost::format(function) % typeid(T).name()).str();

   msg += ": \n";
   msg += message;

   ex_out << msg << std::endl;

   // Value passed to the function is denormalised, just return it:
   return val;
}

/*`
Which leaves us with evaluation errors: these occur when an internal
error occurs that prevents the function being fully evaluated.
The parameter /val/ contains the closest approximation to the result
found so far:
*/

template <class T>
T user_evaluation_error(const char* function, const char* message, const T& val)
{
   if(function == 0)
       function = "Unknown function with arguments of type %1%";
   if(message == 0)
       message = "An internal evaluation error occurred with "
                  "the best value calculated so far of %1%";

   std::string msg("Error in function ");
   msg += (boost::format(function) % typeid(T).name()).str();

   msg += ": \n";
   int prec = 2 + (std::numeric_limits<T>::digits * 30103UL) / 100000UL;
   // int prec = std::numeric_limits<T>::max_digits10; // For C++0X Standard Library
   msg += (boost::format(message) % boost::io::group(std::setprecision(prec), val)).str();

   ex_out << msg << std::endl;

   // What do we return here?  This is generally a fatal error, that should never occur,
   // so we just return a NaN for the purposes of the example:
   return std::numeric_limits<T>::quiet_NaN();
}

} // policies
}}
// libs/math/example/policy_eg_9.cpp
namespace ex_policy_eg_9 {
 using std::endl; 

//[policy_eg_9

/*`
The previous example was all well and good, but the custom error handlers
didn't really do much of any use.  In this example we'll implement all
the custom handlers and show how the information provided to them can be
used to generate nice formatted error messages.

Each error handler has the general form:

   template <class T>
   T user_``['error_type]``(
      const char* function,
      const char* message,
      const T& val);

and accepts three arguments:

[variablelist
[[const char* function]
   [The name of the function that raised the error, this string
   contains one or more %1% format specifiers that should be
   replaced by the name of real type T, like float or double.]]
[[const char* message]
   [A message associated with the error, normally this
   contains a %1% format specifier that should be replaced with
   the value of ['value]: however note that overflow and underflow messages
   do not contain this %1% specifier (since the value of ['value] is
   immaterial in these cases).]]
[[const T& value]
   [The value that caused the error: either an argument to the function
   if this is a domain or pole error, the tentative result
   if this is a denorm or evaluation error, or zero or infinity for
   underflow or overflow errors.]]
]

As before we'll include the headers we need first:

*/


/*`
Next we'll implement our own error handlers for each type of error,
starting with domain errors:
*/

 // boost::math


/*`
Now we'll need to define a suitable policy that will call these handlers,
and define some forwarding functions that make use of the policy:
*/

namespace mymath
{ // unnamed.

using namespace boost::math::policies;

typedef policy<
   domain_error<user_error>,
   pole_error<user_error>,
   overflow_error<user_error>,
   underflow_error<user_error>,
   denorm_error<user_error>,
   evaluation_error<user_error>
> user_error_policy;

BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS(user_error_policy)

} // unnamed namespace

/*`
We now have a set of forwarding functions, defined in namespace mymath,
that all look something like this:

``
template <class RealType>
inline typename boost::math::tools::promote_args<RT>::type
   tgamma(RT z)
{
   return boost::math::tgamma(z, user_error_policy());
}
``

So that when we call `mymath::tgamma(z)` we really end up calling
`boost::math::tgamma(z, user_error_policy())`, and any
errors will get directed to our own error handlers:
*/

int example_main()
{
   // Raise a domain error:
   ex_out << "Result of erf_inv(-10) is: "
      << mymath::erf_inv(-10) << std::endl << endl;
   // Raise a pole error:
   ex_out << "Result of tgamma(-10) is: "
      << mymath::tgamma(-10) << std::endl << endl;
   // Raise an overflow error:
   ex_out << "Result of tgamma(3000) is: "
      << mymath::tgamma(3000) << std::endl << endl;
   // Raise an underflow error:
   ex_out << "Result of tgamma(-190.5) is: "
      << mymath::tgamma(-190.5) << std::endl << endl;
   // Unfortunately we can't predictably raise a denormalised
   // result, nor can we raise an evaluation error in this example
   // since these should never really occur!
    return 0;  // main's implicit return
} // int example_main()

/*`

Which outputs:

[pre
Error in function boost::math::erf_inv<double>(double, double):
Argument outside range \[-1, 1\] in inverse erf function (got p=-10).
Result of erf_inv(-10) is: 1.#QNAN

Error in function boost::math::tgamma<long double>(long double):
Evaluation of tgamma at a negative integer -10.
Result of tgamma(-10) is: 1.#QNAN

Error in function boost::math::tgamma<long double>(long double):
Result of tgamma is too large to represent.
Error in function boost::math::tgamma<double>(double):
Result of function is too large to represent
Result of tgamma(3000) is: 1.#INF

Error in function boost::math::tgamma<long double>(long double):
Result of tgamma is too large to represent.
Error in function boost::math::tgamma<long double>(long double):
Result of tgamma is too small to represent.
Result of tgamma(-190.5) is: 0
]

Notice how some of the calls result in an error handler being called more
than once, or for more than one handler to be called: this is an artefact
of the fact that many functions are implemented in terms of one or more
sub-routines each of which may have it's own error handling.  For example
`tgamma(-190.5)` is implemented in terms of `tgamma(190.5)` - which overflows -
the reflection formula for `tgamma` then notices that it is dividing by
infinity and so underflows.
*/

//] //[/policy_eg_9]

TEST_CASE("example: policy_eg_9") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Result of erf_inv(-10) is: Error in function boost::math::erf_inv<d>(d, d): 
Argument outside range [-1, 1] in inverse erf function (got p=-10).
nan

Result of tgamma(-10) is: Error in function boost::math::tgamma<e>(e): 
Evaluation of tgamma at a negative integer -10.
nan

Result of tgamma(3000) is: Error in function boost::math::tgamma<e>(e): 
Result of tgamma is too large to represent.
Error in function boost::math::tgamma<d>(d): 
Result of function is too large to represent
inf

Result of tgamma(-190.5) is: Error in function boost::math::tgamma<d>(d): 
Result of function is too small to represent
0

)ex"));
}
}  // namespace ex_policy_eg_9

// libs/math/example/policy_ref_snip12.cpp
namespace ex_policy_ref_snip12 {
 using std::endl;

//[policy_ref_snip12

//using boost::math::tgamma;
// Need not declare using boost::math::tgamma here,
// because will define tgamma in myspace using macro below.

namespace myspace
{
  using namespace boost::math::policies;

  // Define a policy that does not throw on overflow:
  typedef policy<overflow_error<errno_on_error> > my_policy;

  // Define the special functions in this scope to use the policy:   
  BOOST_MATH_DECLARE_SPECIAL_FUNCTIONS(my_policy)
}

// Now we can use myspace::tgamma etc.
// They will automatically use "my_policy":
//
double t = myspace::tgamma(30.0); // Will *not* throw on overflow,
// despite the large value of factorial 30 = 265252859812191058636308480000000
// unlike default policy boost::math::tgamma;

//]

int example_main()
{
   ex_out << "myspace::tgamma(30.0) = " << t << endl;
    return 0;  // main's implicit return
}

/*

Output:

myspace::tgamma(30.0) = 8.84176e+030

*/

TEST_CASE("example: policy_ref_snip12") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(myspace::tgamma(30.0) = 8.84176e+30
)ex"));
}
}  // namespace ex_policy_ref_snip12

// libs/math/example/policy_ref_snip4.cpp
namespace ex_policy_ref_snip4 {
#ifdef _MSC_VER
# pragma warning (disable : 4305) // 'initializing' : truncation from 'long double' to 'const eval_type'
# pragma warning (disable : 4244) // 'conversion' : truncation from 'long double' to 'const eval_type'
#endif

//[policy_ref_snip4

using boost::math::normal_distribution;

using namespace boost::math::policies;

// Define a policy:
typedef policy<
      promote_float<false>
      > my_policy;

// Define the new normal distribution using my_policy:
typedef normal_distribution<float, my_policy> my_norm;

// Get a quantile:
float q = quantile(my_norm(), 0.05f);

//] [policy_ref_snip4]

 using std::endl;

int example_main()
{
   ex_out << " quantile(my_norm(), 0.05f) = " << q << endl; //   -1.64485
    return 0;  // main's implicit return
}

TEST_CASE("example: policy_ref_snip4") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex( quantile(my_norm(), 0.05f) = -1.64485
)ex"));
}
}  // namespace ex_policy_ref_snip4

// libs/math/example/policy_ref_snip8.cpp
namespace ex_policy_ref_snip8 {
using boost::math::negative_binomial_distribution;

using namespace boost::math::policies;

typedef negative_binomial_distribution<
      double, 
      policy<discrete_quantile<integer_round_nearest> > 
   > dist_type;
   
// Lower quantile rounded (down) to nearest:
double x = quantile(dist_type(20, 0.3), 0.05); // 27
// Upper quantile rounded (down) to nearest:
double y = quantile(complement(dist_type(20, 0.3), 0.05)); // 68

//] //[/policy_ref_snip8]

 using std::endl;

int example_main()
{
   ex_out << "using policy<discrete_quantile<integer_round_nearest> " << endl
     << "quantile(dist_type(20, 0.3), 0.05) = " << x << endl
     << "quantile(complement(dist_type(20, 0.3), 0.05)) " << y << endl;
    return 0;  // main's implicit return
}

/*

Output:

   using policy<discrete_quantile<integer_round_nearest> 
  quantile(dist_type(20, 0.3), 0.05) = 27
  quantile(complement(dist_type(20, 0.3), 0.05)) 68

*/

TEST_CASE("example: policy_ref_snip8") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(using policy<discrete_quantile<integer_round_nearest> 
quantile(dist_type(20, 0.3), 0.05) = 27
quantile(complement(dist_type(20, 0.3), 0.05)) 68
)ex"));
}
}  // namespace ex_policy_ref_snip8

// libs/math/example/reverse_mode_linear_regression_example.cpp
namespace ex_reverse_mode_linear_regression_example {
using namespace boost::math::differentiation::reverse_mode;
double random_double(double min_val, double max_val)
{
    static std::random_device              rd;
    static std::mt19937                    gen(rd());
    std::uniform_real_distribution<double> dist(min_val, max_val);
    return dist(gen);
}

template<size_t N>
rvar<double, 1> loss(std::array<double, N>& y_target, std::array<rvar<double, 1>, N>& y_fit)
{
    rvar<double, 1> loss_v = make_rvar<double, 1>(0.0);
    for (size_t i = 0; i < N; ++i) {
        loss_v += pow(abs(y_target[i] - y_fit[i]), 2) / N;
    }
    return loss_v;
}
double noisy_linear_function(double intercept, double slope, double x)
{
    return intercept + slope * x + random_double(-0.1, 0.1);
}

template<size_t N>
std::array<rvar<double, 1>, N> model(rvar<double, 1>&       a,
                                     rvar<double, 1>&       b,
                                     std::array<double, N>& x)
{
    std::array<rvar<double, 1>, N> ret;
    for (size_t i = 0; i < N; ++i) {
        ret[i] = a * x[i] + b;
    }
    return ret;
}
int example_main()
{
    double                               slope            = random_double(-5, 5);
    double                               intercept        = random_double(-5, 5);

    const size_t                         num_data_samples = 100;
    /**/
    std::array<double, num_data_samples> noisy_data_x;
    std::array<double, num_data_samples> noisy_data_y;
    for (size_t i = 0; i < num_data_samples; i++) {
        double x        = random_double(-1, 1);
        double y        = noisy_linear_function(intercept, slope, x);
        noisy_data_x[i] = x;
        noisy_data_y[i] = y;
    }

    double                    slope_guess     = random_double(-5, 5);
    double                    intercept_guess = random_double(-5, 5);

    rvar<double, 1>           a               = make_rvar<double, 1>(slope_guess);
    rvar<double, 1>           b               = make_rvar<double, 1>(intercept_guess);

    gradient_tape<double, 1>& tape            = get_active_tape<double, 1>();
    tape.add_checkpoint();

    auto            y_fit         = model(a, b, noisy_data_x);
    rvar<double, 1> loss_v        = loss(noisy_data_y, y_fit);

    double          learning_rate = 1e-3;
    while (loss_v > 0.005) {
        tape.rewind_to_last_checkpoint();
        y_fit    = model(a, b, noisy_data_x);
        loss_v   = loss(noisy_data_y, y_fit);
        auto gv  = grad(loss_v, &a, &b);
        a       -= gv[0] * learning_rate;
        b       -= gv[1] * learning_rate;
    }

    double slope_error              = std::abs(slope - a.item());
    double intercept_error          = std::abs(intercept - b.item());
    double relative_slope_error     = slope_error / std::abs(slope);
    double relative_intercept_error = intercept_error / std::abs(intercept);

    ex_out << "Autodiff Linear Regression Summary \n";
    ex_out << "learning rate : " << learning_rate << "\n";
    ex_out << "true slope: " << slope;
    ex_out << " regression: " << a.item() << "\n";

    ex_out << "relative error (slope): " << relative_slope_error << "\n";
    ex_out << "absolute error (slope): " << slope_error << "\n";
    ex_out << "true intercept: " << intercept;
    ex_out << " regression: " << b.item() << "\n";
    ex_out << "absolute error (intercept): " << intercept_error << "\n";
    ex_out << "aelative error (intercept): " << relative_intercept_error << "\n";
    ex_out << "-------------------------------" << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: reverse_mode_linear_regression_example") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex(Autodiff Linear Regression Summary 
learning rate : 0.001
true slope: -1.82286 regression: -1.89003
relative error (slope): 0.0368471
absolute error (slope): 0.0671671
true intercept: 1.70498 regression: 1.71336
absolute error (intercept): 0.00837366
aelative error (intercept): 0.00491128
-------------------------------
)ex"));
#endif
}
}  // namespace ex_reverse_mode_linear_regression_example

// libs/math/example/series.cpp
namespace ex_series {
template <class T>
struct log1p_series
{
   // we must define a result_type typedef:
   typedef T result_type;

   log1p_series(T x)
      : k(0), m_mult(-x), m_prod(-1) {}

   T operator()()
   {
      // This is the function operator invoked by the summation
      // algorithm, the first call to this operator should return
      // the first term of the series, the second call the second
      // term and so on.
      m_prod *= m_mult;
      return m_prod / ++k;
   }

private:
   int k;
   const T m_mult;
   T m_prod;
};
//]

//[series_log1p_func
template <class T>
T log1p(T x)
{
   // We really should add some error checking on x here!
   BOOST_MATH_ASSERT(std::fabs(x) < 1);

   // Construct the series functor:
   log1p_series<T> s(x);
   // Set a limit on how many iterations we permit:
   std::uintmax_t max_iter = 1000;
   // Add it up, with enough precision for full machine precision:
   return boost::math::tools::sum_series(s, std::numeric_limits<T>::epsilon(), max_iter);
}
//]

//[series_clog1p_func
template <class T>
struct log1p_series<std::complex<T> >
{
   // we must define a result_type typedef:
   typedef std::complex<T> result_type;

   log1p_series(std::complex<T> x)
      : k(0), m_mult(-x), m_prod(-1) {}

   std::complex<T> operator()()
   {
      // This is the function operator invoked by the summation
      // algorithm, the first call to this operator should return
      // the first term of the series, the second call the second
      // term and so on.
      m_prod *= m_mult;
      return m_prod / T(++k);
   }

private:
   int k;
   const std::complex<T> m_mult;
   std::complex<T> m_prod;
};


template <class T>
std::complex<T> log1p(std::complex<T> x)
{
   // We really should add some error checking on x here!
   BOOST_MATH_ASSERT(abs(x) < 1);

   // Construct the series functor:
   log1p_series<std::complex<T> > s(x);
   // Set a limit on how many iterations we permit:
   std::uintmax_t max_iter = 1000;
   // Add it up, with enough precision for full machine precision:
   return boost::math::tools::sum_series(s, std::complex<T>(std::numeric_limits<T>::epsilon()), max_iter);
}
//]

int example_main()
{
   using namespace boost::math::tools;

   ex_out << log1p(0.25) << std::endl;

   ex_out << log1p(std::complex<double>(0.25, 0.25)) << std::endl;

   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: series") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(0.223144
(0.242754,0.197396)
)ex"));
}
}  // namespace ex_series

// libs/math/example/students_t_example3.cpp
namespace ex_students_t_example3 {
   using boost::math::students_t;  // Probability of students_t(df, t).

       using std::endl;
   using std::setprecision;    using std::setw;
   using std::sqrt;

// This example of a two-sided test is from:
// B. M. Smith & M. B. Griffiths, Analyst, 1982, 107, 253,
// from Statistics for Analytical Chemistry, 3rd ed. (1994), pp 58-59
// J. C. Miller and J. N. Miller, Ellis Horwood ISBN 0 13 0309907

// Concentrations of lead (ug/l) determined by two different methods
// for each of four test portions,
// the concentration of each portion is significantly different,
// the values may NOT be pooled.
// (Called a 'paired test' by Miller and Miller
// because each portion analysed has a different concentration.)

// Portion  Wet oxidation Direct Extraction
//   1           71            76
//   2           61            68
//   3           50            48
//   4           60            57

const int portions = 4;
const int methods = 2;
float data [portions][methods] = {{71, 76}, {61,68}, {50, 48}, {60, 57}};
float diffs[portions];

int example_main()
{
   ex_out << "Example3 using Student's t function. " << endl;
   float mean_diff = 0.f;
   ex_out << "\n""Portion  wet_oxidation  Direct_extraction  difference" << endl;
   for (int portion = 0; portion < portions; portion++)
   { // Echo data and differences.
      diffs[portion] = data[portion][0] - data[portion][1];
      mean_diff += diffs[portion];
      ex_out << setw(4) << portion << ' ' << setw(14) << data[portion][0] << ' ' << setw(18)<< data[portion][1] << ' ' << setw(9) << diffs[portion] << endl;
   }
   mean_diff /= portions;
   ex_out << "Mean difference = " << mean_diff << endl; // -1.75

   float sd_diffs = 0.f;
   for (int portion = 0; portion < portions; portion++)
   { // Calculate standard deviation of differences.
      sd_diffs +=(diffs[portion] - mean_diff) * (diffs[portion] - mean_diff);
   }
   int degrees_of_freedom = portions-1; // Use the n-1 formula.
   sd_diffs /= degrees_of_freedom;
   sd_diffs = sqrt(sd_diffs);
   ex_out << "Standard deviation of differences = " << sd_diffs << endl; // 4.99166
   // Standard deviation of differences = 4.99166
   double t = mean_diff * sqrt(static_cast<double>(portions))/ sd_diffs; // -0.70117
   ex_out << "Student's t = " << t << ", if " << degrees_of_freedom << " degrees of freedom." << endl;
   // Student's t = -0.70117, if 3 degrees of freedom.
   ex_out << "Probability of the means being different is "
    << 2.F * cdf(students_t(degrees_of_freedom), t) << "."<< endl; // 0.266846 * 2 = 0.533692
   // Double the probability because using a 'two-sided test' because
   // mean for 'Wet oxidation' could be either
   // greater OR LESS THAN for 'Direct extraction'.

   return 0;
    return 0;  // main's implicit return
}  // int example_main()

/*

Output is:

Example3 using Student's t function. 
Portion  wet_oxidation  Direct_extraction  difference
   0             71                 76        -5
   1             61                 68        -7
   2             50                 48         2
   3             60                 57         3
Mean difference = -1.75
Standard deviation of differences = 4.99166
Student's t = -0.70117, if 3 degrees of freedom.
Probability of the means being different is 0.533692.

*/

TEST_CASE("example: students_t_example3") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Example3 using Student's t function. 

Portion  wet_oxidation  Direct_extraction  difference
   0             71                 76        -5
   1             61                 68        -7
   2             50                 48         2
   3             60                 57         3
Mean difference = -1.75
Standard deviation of differences = 4.99166
Student's t = -0.70117, if 3 degrees of freedom.
Probability of the means being different is 0.533692.
)ex"));
}
}  // namespace ex_students_t_example3

// libs/math/example/to_continued_fraction.cpp
namespace ex_to_continued_fraction {
using boost::math::constants::root_two;
using boost::math::constants::phi;
using boost::math::constants::pi;
using boost::math::constants::e;
using boost::math::constants::zeta_three;
using boost::math::tools::simple_continued_fraction;

int example_main()
{
    using Real = boost::multiprecision::cpp_bin_float_100;
    auto phi_cfrac = simple_continued_fraction(phi<Real>());
    ex_out << "φ ≈ " << phi_cfrac << "\n\n";

    auto pi_cfrac = simple_continued_fraction(pi<Real>());
    ex_out << "π ≈ " << pi_cfrac << "\n";
    ex_out << "Known: [3; 7, 15, 1, 292, 1, 1, 1, 2, 1, 3, 1, 14, 2, 1, 1, 2, 2, 2, 2, 1, 84, 2, 1, 1, 15, 3, 13, 1, 4, 2, 6, 6, 99, 1, 2, 2, 6, 3, 5, 1, 1, 6, 8, 1, 7, 1, 2, 3, 7, 1, 2, 1, 1, 12, 1, 1, 1, 3, 1, 1, 8, 1, 1, 2, 1, 6, 1, 1, 5, 2, 2, 3, 1, 2, 4, 4, 16, 1, 161, 45, 1, 22, 1, 2, 2, 1, 4, 1, 2, 24, 1, 2, 1, 3, 1, 2, 1, ...]\n\n";

    auto rt_cfrac = simple_continued_fraction(root_two<Real>());
    ex_out << "√2 ≈ " << rt_cfrac << "\n\n";

    auto e_cfrac = simple_continued_fraction(e<Real>());
    ex_out << "e ≈ " << e_cfrac << "\n";

    // Correctness can be checked in Mathematica via: ContinuedFraction[Zeta[3], 500]
    auto z_cfrac = simple_continued_fraction(zeta_three<Real>());
    ex_out << "ζ(3) ≈ " << z_cfrac << "\n";
    return 0;  // main's implicit return
}

TEST_CASE("example: to_continued_fraction") {
    std::locale::global(std::locale::classic());  // no global locale left by another example
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(φ ≈ [1; 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2]

π ≈ [3; 7, 15, 1, 292, 1, 1, 1, 2, 1, 3, 1, 14, 2, 1, 1, 2, 2, 2, 2, 1, 84, 2, 1, 1, 15, 3, 13, 1, 4, 2, 6, 6, 99, 1, 2, 2, 6, 3, 5, 1, 1, 6, 8, 1, 7, 1, 2, 3, 7, 1, 2, 1, 1, 12, 1, 1, 1, 3, 1, 1, 8, 1, 1, 2, 1, 6, 1, 1, 5, 2, 2, 3, 1, 2, 4, 4, 16, 1, 161, 45, 1, 22, 1, 2, 2, 1, 4, 1, 2, 24, 1, 3]
Known: [3; 7, 15, 1, 292, 1, 1, 1, 2, 1, 3, 1, 14, 2, 1, 1, 2, 2, 2, 2, 1, 84, 2, 1, 1, 15, 3, 13, 1, 4, 2, 6, 6, 99, 1, 2, 2, 6, 3, 5, 1, 1, 6, 8, 1, 7, 1, 2, 3, 7, 1, 2, 1, 1, 12, 1, 1, 1, 3, 1, 1, 8, 1, 1, 2, 1, 6, 1, 1, 5, 2, 2, 3, 1, 2, 4, 4, 16, 1, 161, 45, 1, 22, 1, 2, 2, 1, 4, 1, 2, 24, 1, 2, 1, 3, 1, 2, 1, ...]

√2 ≈ [1; 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]

e ≈ [2; 1, 2, 1, 1, 4, 1, 1, 6, 1, 1, 8, 1, 1, 10, 1, 1, 12, 1, 1, 14, 1, 1, 16, 1, 1, 18, 1, 1, 20, 1, 1, 22, 1, 1, 24, 1, 1, 26, 1, 1, 28, 1, 1, 30, 1, 1, 32, 1, 1, 34, 1, 1, 36, 1, 1, 38, 1, 1, 40, 1, 1, 42, 1, 1, 44, 1, 1, 46, 1, 1, 48, 1, 1, 50, 1, 1, 52, 1, 1, 54, 1, 1, 56, 1, 1, 58]
ζ(3) ≈ [1; 4, 1, 18, 1, 1, 1, 4, 1, 9, 9, 2, 1, 1, 1, 2, 7, 1, 1, 7, 11, 1, 1, 1, 3, 1, 6, 1, 30, 1, 4, 1, 1, 4, 1, 3, 1, 2, 7, 1, 3, 1, 2, 2, 1, 16, 1, 1, 3, 3, 1, 2, 2, 1, 6, 1, 1, 1, 6, 1, 1, 4, 428, 5, 1, 1, 3, 1, 1, 11, 2, 4, 4, 5, 4, 1, 5, 14, 1, 3, 1, 2, 19, 1, 2, 5, 1, 7, 1, 1, 1, 1, 1, 57, 3, 2, 14, 2, 7, 1, 4, 2, 3, 9]
)ex"));
}
}  // namespace ex_to_continued_fraction
