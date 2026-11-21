// mcpplibs.mwrap.boost.decimal — the official examples (libs/decimal/example/
// *.cpp) ported mechanically: each program runs as a case and its output must
// equal what the same code printed when built from upstream/boost headers
// alone. The examples branch on the library's configuration macros
// (BOOST_DECIMAL_HAS_FORMAT_SUPPORT, ...); like any consumer of those macros
// the TU includes <version>, boost/decimal/detail/config.hpp and the headers
// that define them (decimal/format.hpp) before the import. statistics.cpp and
// rounding_mode_compile_time.cpp define configuration macros before the
// headers (the module is built with the defaults) and are excluded, as are
// the fmtlib example's fmt parts (fmt is not vendored). print.cpp's
// std::print goes to stdout, so the port formats into the captured stream;
// to_from_file draws its values from std::uniform_int_distribution, whose
// sequence is the standard library's own, so only its round-trip result is
// checked (the program verifies the values it reads back).
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <cstdio>
#include <version>
#include <boost/decimal/detail/config.hpp>
#include <boost/decimal/format.hpp>
#include <cassert>
#include <cmath>
#include <boost/config.hpp>
import std;
import mcpplibs.mwrap.boost.decimal;

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

// libs/decimal/example/addition.cpp
namespace ex_addition {
int example_main()
{
    using boost::decimal::decimal32_t;

    constexpr decimal32_t decimal_one_tenth {"0.1"}; // Construct constant 0.1 from string for lossless conversion
    constexpr float float_one_tenth {0.1f};              // Construct floating point constant from literal

    decimal32_t decimal_value {};   // Construct decimal 0 to start from
    float float_value {};           // Construct float 0 to start from

    // We now add 0.1 1000 times which should result exactly in 100
    // What we actually find is that the decimal32_t value does result in exactly 100
    // With type float the result is not 100 due to inexact representation
    for (int i {}; i < 1000; ++i)
    {
        decimal_value += decimal_one_tenth; // Decimal types support compound arithmetic as expected
        float_value += float_one_tenth;
    }

    // Each of the decimal types has complete support for std::numeric_limits,
    // which we leverage here with set precision to show any fractional part of the number (if applicable)
    ex_out << std::setprecision(std::numeric_limits<decimal32_t>::digits10)
              << "Decimal Result: " << decimal_value << "\n"
              << std::setprecision(std::numeric_limits<float>::digits10)
              << "  Float Result: " << float_value << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: addition") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Decimal Result: 100
  Float Result: 99.999
)ex"));
}
}  // namespace ex_addition

// libs/decimal/example/adl.cpp
namespace ex_adl {
template <typename T>
void sin_identity(T val)
{
    // ADL allows builtin and decimal types to both be used
    // Boost.Decimal is not allowed to overload std::sin so it must be provided in its own namespace
    // You must also include using std::sin to ensure that it is found for the float, double, and long double cases.
    // It is preferred to have using statements for the functions you intend to use instead of using namespace XXX.
    using std::sin;
    using boost::decimal::sin;

    // sin(x) = -sin(-x)
    // The call here MUST be unqualified, or you will get compiler errors
    // For example calling std::sin here would not allow any of the decimal types to be used
    ex_out << "sin(" << val << ") = " << sin(val) << '\n'
              << "-sin(" << -val << ") = " << -sin(-val) << "\n\n";
}

int example_main()
{
    // Because of the two using statements in the above function we can now call it with built-in floating point,
    // or our decimal types as show below

    ex_out << "Float:\n";
    sin_identity(-0.5F);

    ex_out << "Double:\n";
    sin_identity(-0.5);

    ex_out << "Long Double:\n";
    sin_identity(-0.5L);

    ex_out << "decimal32_t:\n";
    sin_identity(boost::decimal::decimal32_t{"-0.5"});

    ex_out << "decimal64_t:\n";
    sin_identity(boost::decimal::decimal64_t{"-0.5"});

    ex_out << "decimal128_t:\n";
    sin_identity(boost::decimal::decimal128_t{"-0.5"});
    return 0;  // main's implicit return
}

TEST_CASE("example: adl") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Float:
sin(-0.5) = -0.479426
-sin(0.5) = -0.479426

Double:
sin(-0.5) = -0.479426
-sin(0.5) = -0.479426

Long Double:
sin(-0.5) = -0.479426
-sin(0.5) = -0.479426

decimal32_t:
sin(-0.5) = -0.479426
-sin(0.5) = -0.479426

decimal64_t:
sin(-0.5) = -0.479426
-sin(0.5) = -0.479426

decimal128_t:
sin(-0.5) = -0.479426
-sin(0.5) = -0.479426

)ex"));
}
}  // namespace ex_adl

// libs/decimal/example/basic_arithmetic.cpp
namespace ex_basic_arithmetic {
int example_main()
{
    using boost::decimal::decimal64_t;      // Type decimal64_t

    constexpr decimal64_t a {"-5.123456891234567"};  // Constructs -5.123456 from string
    constexpr decimal64_t b {"3.123456891234567"};   // Constructs 3.123456 from string
    constexpr decimal64_t c {a + b};

    // Here we can see that the result is exact
    constexpr decimal64_t neg_two {-2};
    static_assert(c == neg_two, "Result should be exact");

    // We can use std::setprecision and std::numeric_limits in their usual ways
    ex_out << std::setprecision(std::numeric_limits<decimal64_t>::digits10)
              << "A: " << a << '\n'
              << "B: " << b << '\n'
              << "A + B: " << c << '\n';

    // The decimal library provides comprehensive implementations of the <cmath> functions
    // that you would expect to have with the builtin floating point types
    //
    // They are all located in namespace boost::decimal::,
    // as overloading namespace std is not allowed

    constexpr decimal64_t abs_c {boost::decimal::abs(c)};
    ex_out << "abs(A + B): " << abs_c << '\n';

    // All cmath functions are constexpr even if their std:: counterparts are not
    constexpr decimal64_t sqrt_two {boost::decimal::sqrt(abs_c)};

    // Value compute by N[Sqrt[2], 50] using Wolfram Alpha or Mathematica
    constexpr decimal64_t wa_sqrt_two {"1.4142135623730950488016887242096980785696718753769"};
    ex_out << "sqrt(abs(A + B)): " << sqrt_two << '\n'
              << "Wolfram Alpha sqrt(2): " << wa_sqrt_two << '\n';
    return 0;  // main's implicit return
}

TEST_CASE("example: basic_arithmetic") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(A: -5.123456891234567
B: 3.123456891234567
A + B: -2
abs(A + B): 2
sqrt(abs(A + B)): 1.414213562373095
Wolfram Alpha sqrt(2): 1.414213562373095
)ex"));
}
}  // namespace ex_basic_arithmetic

// libs/decimal/example/basic_construction.cpp
namespace ex_basic_construction {
int example_main()
{
    using boost::decimal::decimal32_t;          // The decimal32_t type
    using boost::decimal::construction_sign;    // An enum class for specifying sign during certain instances of construction
    using boost::decimal::isinf;                // Analogous to std::isinf but for decimal types
    using boost::decimal::isnan;                // Analogous to std::isnan but for decimal types

    // Construction from an integer
    constexpr decimal32_t val_1 {100};

    // Construction from a signed integer and exponent
    constexpr decimal32_t val_2 {10, 1};

    // Construction from an unsigned integer, exponent, and sign
    // The sign enum is named construction_sign, and has two members: positive and negative
    constexpr decimal32_t val_3 {1U, 2, construction_sign::negative};

    ex_out << "Val_1: " << val_1 << '\n'
              << "Val_2: " << val_2 << '\n'
              << "Val_3: " << val_3 << '\n';

    if (val_1 == val_2 && val_2 == val_3 && val_1 == val_3)
    {
        ex_out << "All equal values" << std::endl;
    }

    // Demonstration of the overflow and underflow handling
    // A value that overflows constructs an infinity (which can be queried using isinf)
    // A value that underflows constructs a zero
    constexpr decimal32_t overflow_value {100, 10000};
    if (isinf(overflow_value))
    {
        ex_out << "Overflow constructs infinity" << std::endl;
    }

    constexpr decimal32_t underflow_value {100, -10000};
    constexpr decimal32_t zero {0};
    if (underflow_value == zero)
    {
        ex_out << "Underflow constructs zero" << std::endl;
    }

    // Construction of NANs can be done using numeric limits,
    // and checked using the normal isnan function
    constexpr decimal32_t non_finite_from_float {std::numeric_limits<decimal32_t>::quiet_NaN()};
    if (isnan(non_finite_from_float))
    {
        ex_out << "NaN constructs NaN" << std::endl;
    }

    // We can also construct both from a C-string (const char*) and from a std::string

    const char* c_string_value {"4.3e-02"};
    const decimal32_t from_c_string {c_string_value};
    const decimal32_t from_std_string {std::string(c_string_value)};

    if (from_c_string == from_std_string)
    {
        ex_out << "Values constructed from const char* and std::string are the same" << '\n';
    }

    // If we attempt construction for a string that cannot be converted into a decimal value,
    // the constructor will do 1 of 2 things:
    // 1) In a noexcept environment the constructor will return a quiet NaN
    // 2) Otherwise it will throw
    //
    // The exception environment is detected automatically,
    // or can be set by defining BOOST_DECIMAL_DISABLE_EXCEPTIONS

    const char* bad_string {"Junk_String"};

    #ifndef BOOST_DECIMAL_DISABLE_EXCEPTIONS

    try
    {
        const decimal32_t throwing_value {bad_string};
        ex_out << throwing_value << '\n';
    }
    catch (const std::runtime_error& e)
    {
        ex_out << e.what() << std::endl;
    }

    #else

    const decimal32_t nan_value {bad_string};
    if (isnan(nan_value))
    {
        ex_out << "Bad string construction has formed a NAN" << std::endl;
    }

    #endif
    return 0;  // main's implicit return
}

TEST_CASE("example: basic_construction") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Val_1: 100
Val_2: 100
Val_3: -100
Overflow constructs infinity
Underflow constructs zero
NaN constructs NaN
Values constructed from const char* and std::string are the same
Can not construct from invalid string
)ex"));
}
}  // namespace ex_basic_construction

// libs/decimal/example/binary_float_conversions.cpp
namespace ex_binary_float_conversions {
int example_main()
{
    using boost::decimal::decimal32_t;      // Type decimal32_t
    using boost::decimal::decimal64_t;      // Type decimal64_t

    // Non-finite values construct the equivalent non-finite value in binary floating point
    constexpr decimal64_t decimal_qnan {std::numeric_limits<decimal64_t>::quiet_NaN()};
    const double double_from_qnan {static_cast<double>(decimal_qnan)};

    // Note here that we must use boost::decimal::isnan for decimal types,
    // as it is illegal to overload std::isnan
    if (boost::decimal::isnan(decimal_qnan) && std::isnan(double_from_qnan))
    {
        ex_out << "Decimal QNAN converts to double QNAN\n";
    }

    constexpr decimal64_t decimal_inf {std::numeric_limits<decimal64_t>::infinity()};
    const double double_from_inf {static_cast<double>(decimal_inf)};

    // Same as the above but with INF instead of NAN
    if (boost::decimal::isinf(decimal_inf) && std::isinf(double_from_inf))
    {
        ex_out << "Decimal INFINITY converts to double INFINITY\n";
    }

    // For finite values we make a best effort approach to covert to double
    // We are able to decompose the decimal floating point value into a sign, significand, and exponent.
    // From there we use the methods outline in Daniel Lemire's "Number Parsing at a Gigabyte a Second",
    // to construct the binary floating point value.
    // See: https://arxiv.org/pdf/2101.11408

    // Construct the decimal64_t version of pi using our pre-computed constants from <boost/decimal/numbers.hpp>
    constexpr decimal64_t decimal_pi {boost::decimal::numbers::pi_v<decimal64_t>};
    const double double_from_pi {static_cast<double>(decimal_pi)};

    ex_out << std::setprecision(std::numeric_limits<decimal64_t>::digits10)
              << "decimal64_t pi: " << decimal_pi << '\n'
              << "     double pi: " << double_from_pi << '\n';

    // To construct a decimal64_t from double we use the methods described in "Ryu: fast float-to-string conversion"
    // See: https://dl.acm.org/doi/10.1145/3192366.3192369
    // This paper shows how to decompose a double into it's sign, significand, and exponent
    // Once we have those components we can use the normal constructors of the decimal types to construct
    // Since we are using the normal constructors here,
    // any construction from this conversion is subject to the current rounding mode
    // Such as with a lossy conversion like shown (double -> decimal32_t)

    const decimal64_t decimal_from_double {static_cast<decimal64_t>(double_from_pi)};
    const decimal32_t lossy_decimal_from_double {static_cast<decimal32_t>(double_from_pi)};

    ex_out << "  Converted pi: " << decimal_from_double << '\n'
              << "decimal32_t pi: " << lossy_decimal_from_double << '\n';


    // Other than what has already been shown,
    // there are no other ways in the library to convert between decimal types and binary floating point types
    // The reason for this is to discourage their use.
    //
    // You can use intermediate representations like strings if you want to make these conversions,
    // and want to be sure about what the resulting value will be

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: binary_float_conversions") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Decimal QNAN converts to double QNAN
Decimal INFINITY converts to double INFINITY
decimal64_t pi: 3.141592653589793
     double pi: 3.141592653589793
  Converted pi: 3.141592653589793
decimal32_t pi: 3.141593
)ex"));
}
}  // namespace ex_binary_float_conversions

// libs/decimal/example/bit_conversions.cpp
namespace ex_bit_conversions {
int example_main()
{
    using boost::decimal::decimal32_t;
    using boost::decimal::from_bid;
    using boost::decimal::from_dpd;
    using boost::decimal::to_bid;
    using boost::decimal::to_dpd;

    // First we construct a decimal value and then convert it into both BID and DPD encoded bits
    const decimal32_t decimal_value {5};
    const std::uint32_t BID_bits {to_bid(decimal_value)};
    const std::uint32_t DPD_bits {to_dpd(decimal_value)};

    // Display the difference between the hex values of both bit encodings
    ex_out << std::hex
              << "BID format: " << BID_bits << '\n'
              << "DPD format: " << DPD_bits << std::endl;

    // Recover the original value by encoding two new decimals using the from_bid and from_dpd functions
    const decimal32_t bid_decimal {from_bid<decimal32_t>(BID_bits)};
    const decimal32_t dpd_decimal {from_dpd<decimal32_t>(DPD_bits)};

    if (bid_decimal == dpd_decimal)
    {
        // These should both have recovered the original value of 5
        return 0;
    }
    else
    {
        // Something has gone wrong recovering the decimal values
        return 1;
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: bit_conversions") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(BID format: 32800005
DPD format: 22500005
)ex"));
}
}  // namespace ex_bit_conversions

// libs/decimal/example/charconv.cpp
namespace ex_charconv {
int example_main()
{
    using boost::decimal::decimal64_t;      // The type decimal64_t
    using boost::decimal::to_chars;         // The to_chars functions
    using boost::decimal::to_chars_result;  // The return type of to_chars
    using boost::decimal::from_chars;       // The from_chars functions
    using boost::decimal::from_chars_result;// The return type of from_chars
    using boost::decimal::chars_format;     // The enum class of the different formatting options
    using boost::decimal::formatting_limits;// Allows the user to correctly size buffers

    const char* initial_value {"-7.12345e+06"};

    decimal64_t initial_decimal;
    const from_chars_result r_initial {from_chars(initial_value, initial_value + std::strlen(initial_value), initial_decimal)};

    // from_chars_result contains a value of std::errc, but also has a bool operator for better checks like this
    // Regular <charconv> should be getting this bool operator in C++26
    if (!r_initial)
    {
        // LCOV_EXCL_START
        // Here you can handle an error condition in any way you see fit
        // For the purposes of our example we log and abort
        ex_out << "Unexpected failure" << std::endl;
        return 1;
        // LCOV_EXCL_STOP
    }
    else
    {
        ex_out << "Initial decimal: " << initial_decimal << '\n';
    }

    // boost::decimal::from_chars deviates from the C++ standard by allowing a std::string,
    // or a std::string_view (when available)
    //
    // It is also perfectly acceptable to use an auto return type for even more brevity when using from_chars
    const std::string string_value {"3.1415"};
    decimal64_t string_decimal;
    const auto r_string {from_chars(string_value, string_decimal)};
    if (r_string)
    {
        ex_out << "Value from string: " << string_decimal << '\n';
    }

    // We can now compare the various ways to print the value
    // First we will review the formatting_limits struct
    // This struct contains a number of members that allow the to_chars buffer to be correctly sized
    //
    // First formatting_limits takes a type and optionally a precision as template parameters
    // It then has members each corresponding to the maximum number of characters needed to print the type
    //
    // 1) scientific_format_max_chars
    // 2) fixed_format_max_chars
    // 3) hex_format_max_chars
    // 4) cohort_preserving_scientific_max_chars
    // 5) general_format_max_chars
    // 6) max_chars - Equal to the maximum value of 1 to 5 to allow to_chars of any format
    //
    // Each of these will give you one additional character so you can write a null terminator to the end
    // NOTE: to_chars IS NOT default null terminated

    char scientific_buffer[formatting_limits<decimal64_t>::scientific_format_max_chars];
    const to_chars_result r_sci {to_chars(scientific_buffer,
        scientific_buffer + sizeof(scientific_buffer), initial_decimal, chars_format::scientific)};
    if (r_sci)
    {
        *r_sci.ptr = '\0'; // to_chars does not null terminate per the C++ standard
        ex_out << "Value in scientific format: " << scientific_buffer << '\n';
    }
    // else handle the error how you would like

    // If we went to print the value to some specified precision our buffer will need more space
    // Formatting limits takes a precision in this case
    //
    // Also as with from_chars it's perfectly fine to use an auto return type with to_chars
    constexpr int required_precision {20};
    char precision_20_scientific_buffer[formatting_limits<decimal64_t, required_precision>::scientific_format_max_chars];
    const auto r_sci20 {to_chars(precision_20_scientific_buffer,
        precision_20_scientific_buffer + sizeof(precision_20_scientific_buffer),
        initial_decimal, chars_format::scientific, required_precision)};

    if (r_sci20)
    {
        *r_sci20.ptr = '\0';
        ex_out << "Value in scientific format with precision 20: " << precision_20_scientific_buffer << '\n';
    }
    // else handle the error how you would like
    return 0;  // main's implicit return
}

TEST_CASE("example: charconv") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Initial decimal: -7123450
Value from string: 3.1415
Value in scientific format: -7.12345e+06
Value in scientific format with precision 20: -7.12345000000000000000e+06
)ex"));
}
}  // namespace ex_charconv

// libs/decimal/example/charconv_cohort_preservation.cpp
namespace ex_charconv_cohort_preservation {
static constexpr std::size_t N {7};

// All the following decimal values will compare equal,
// but since they have different numbers of 0s in the significand they will not be bitwise equal
constexpr std::array<boost::decimal::decimal32_t, N> decimals = {
    boost::decimal::decimal32_t{3, 2},
    boost::decimal::decimal32_t{30, 1},
    boost::decimal::decimal32_t{300, 0},
    boost::decimal::decimal32_t{3000, -1},
    boost::decimal::decimal32_t{30000, -2},
    boost::decimal::decimal32_t{300000, -3},
    boost::decimal::decimal32_t{3000000, -4},
};

// These strings represent the same values as the constructed ones shown above
constexpr std::array<const char*, N> strings = {
    "3e+02",
    "3.0e+02",
    "3.00e+02",
    "3.000e+02",
    "3.0000e+02",
    "3.00000e+02",
    "3.000000e+02",
};

int example_main()
{
    using boost::decimal::decimal32_t;      // For type decimal32_t
    using boost::decimal::from_chars;       // decimal specific from_chars
    using boost::decimal::chars_format;     // chars_format enum with decimal specific option shown here

    // In some instances we want to preserve the cohort of our values
    // In the above strings array all of these values compare equal,
    // but will NOT be bitwise equal once constructed.

    for (std::size_t i = 0; i < N; ++i)
    {
        decimal32_t string_val;
        const auto r_from = from_chars(strings[i], string_val, chars_format::cohort_preserving_scientific);

        if (!r_from)
        {
            // Unexpected failure
            return 1;
        }

        for (std::size_t j = 0; j < N; ++j)
        {
            // Now that we have constructed a value from string
            // we can compare it bitwise to all the members of the decimal array
            // to show the difference between operator== and bitwise equality
            //
            // All members of a cohort are supposed to compare equal with operator==,
            // and likewise will hash equal to
            std::uint32_t string_val_bits;
            std::uint32_t constructed_val_bits;

            std::memcpy(&string_val_bits, &string_val, sizeof(string_val_bits));
            std::memcpy(&constructed_val_bits, &decimals[j], sizeof(constructed_val_bits));

            if (string_val == decimals[j])
            {
                ex_out << "Values are equal and ";
                if (string_val_bits == constructed_val_bits)
                {
                    ex_out << "bitwise equal.\n";
                }
                else
                {
                    ex_out << "NOT bitwise equal.\n";
                }
            }
        }

        // The same chars_format option applies to to_chars which allows us to roundtrip the values
        char buffer[boost::decimal::formatting_limits<decimal32_t>::cohort_preserving_scientific_max_chars] {};
        const auto r_to = to_chars(buffer, buffer + sizeof(buffer), string_val, chars_format::cohort_preserving_scientific);

        if (!r_to)
        {
            // Unexpected failure
            return 1;
        }

        *r_to.ptr = '\0'; // charconv does not null terminate per the C++ specification

        if (std::strcmp(strings[i], buffer) == 0)
        {
            ex_out << "Successful Roundtrip of value: " << buffer << "\n\n";
        }
        else
        {
            ex_out << "Failed\n\n";
            return 1;
        }
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: charconv_cohort_preservation") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Values are equal and bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Successful Roundtrip of value: 3e+02

Values are equal and NOT bitwise equal.
Values are equal and bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Successful Roundtrip of value: 3.0e+02

Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Successful Roundtrip of value: 3.00e+02

Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Successful Roundtrip of value: 3.000e+02

Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Successful Roundtrip of value: 3.0000e+02

Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and bitwise equal.
Values are equal and NOT bitwise equal.
Successful Roundtrip of value: 3.00000e+02

Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and NOT bitwise equal.
Values are equal and bitwise equal.
Successful Roundtrip of value: 3.000000e+02

)ex"));
}
}  // namespace ex_charconv_cohort_preservation

// libs/decimal/example/debugger.cpp
namespace ex_debugger {
template <typename T>
void debug_values()
{
    // Displays the maximum and minimum values that the type can hold
    // from numeric_limits
    const T max {std::numeric_limits<T>::max()};
    const T min {std::numeric_limits<T>::min()};

    // A number whose representation will change based on IEEE vs fast type
    // In the IEEE case 3.140e+00 will be displayed as the pretty printer is cohort preserving
    const T short_num {"3.140"};

    // Shows how infinities will be displayed
    const T pos_inf {std::numeric_limits<T>::infinity()};
    const T neg_inf {-std::numeric_limits<T>::infinity()};

    // Shows how the different kinds of NANs will be displayed
    const T qnan {std::numeric_limits<T>::quiet_NaN()};
    const T snan {std::numeric_limits<T>::signaling_NaN()};

    // Shows how a payload added to a QNAN will be displayed
    const T payload_nan {boost::decimal::nan<T>("7")};

    // Break Here:
    static_cast<void>(max);
    static_cast<void>(min);
    static_cast<void>(short_num);
    static_cast<void>(pos_inf);
    static_cast<void>(neg_inf);
    static_cast<void>(qnan);
    static_cast<void>(snan);
    static_cast<void>(payload_nan);
}

int example_main()
{
    debug_values<boost::decimal::decimal32_t>();
    debug_values<boost::decimal::decimal64_t>();
    debug_values<boost::decimal::decimal128_t>();

    debug_values<boost::decimal::decimal_fast32_t>();
    debug_values<boost::decimal::decimal_fast64_t>();
    debug_values<boost::decimal::decimal_fast128_t>();

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: debugger") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex()ex"));
}
}  // namespace ex_debugger

// libs/decimal/example/decompose_frexp10_normalize.cpp
namespace ex_decompose_frexp10_normalize {
template <typename Decimal>
void show(const Decimal value)
{
    // decompose returns the encoded significand, biased exponent and sign.
    // For IEEE types this preserves the cohort that was used at construction.
    // For fast types the value is normalized in the constructor, so the
    // components always reflect the normalized form.
    const auto components {boost::decimal::decompose(value)};

    // frexp10 returns the cohort-agnostic significand together with the
    // matching exponent written through the pointer argument. The significand
    // is normalized to the full precision of the type (for decimal32_t the
    // range is [1'000'000, 9'999'999]).
    int frexp_exp {};
    const auto frexp_sig {boost::decimal::frexp10(value, &frexp_exp)};

    // normalize returns a decimal of the same type whose representation has
    // had the cohort effects removed. For fast types this is a no-op since
    // they are already normalized.
    const auto normalized {boost::decimal::normalize(value)};

    ex_out << "  value      = " << value << '\n'
              << "  decompose  : sig = " << components.sig
              << ", exp = " << components.exp
              << ", sign = " << components.sign << '\n'
              << "  frexp10    : sig = " << frexp_sig
              << ", exp = " << frexp_exp << '\n'
              << "  normalize  = " << normalized << "\n\n";
}

int example_main()
{
    // All three values compare equal but use different cohorts.
    constexpr std::array<boost::decimal::decimal32_t, 3> ieee_values {
        boost::decimal::decimal32_t{3, 2},
        boost::decimal::decimal32_t{300, 0},
        boost::decimal::decimal32_t{3000000, -4}
    };

    ex_out << "IEEE decimal32_t (cohort is preserved by the encoding):\n\n";
    for (const auto& v : ieee_values)
    {
        show(v);
    }

    // The same set of inputs given to a fast type all encode identically.
    constexpr std::array<boost::decimal::decimal_fast32_t, 3> fast_values {
        boost::decimal::decimal_fast32_t{3, 2},
        boost::decimal::decimal_fast32_t{300, 0},
        boost::decimal::decimal_fast32_t{3000000, -4}
    };

    ex_out << "decimal_fast32_t (always normalized internally):\n\n";
    for (const auto& v : fast_values)
    {
        show(v);
    }

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: decompose_frexp10_normalize") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(IEEE decimal32_t (cohort is preserved by the encoding):

  value      = 300
  decompose  : sig = 3, exp = 2, sign = 0
  frexp10    : sig = 3000000, exp = -4
  normalize  = 300

  value      = 300
  decompose  : sig = 300, exp = 0, sign = 0
  frexp10    : sig = 3000000, exp = -4
  normalize  = 300

  value      = 300
  decompose  : sig = 3000000, exp = -4, sign = 0
  frexp10    : sig = 3000000, exp = -4
  normalize  = 300

decimal_fast32_t (always normalized internally):

  value      = 300
  decompose  : sig = 3000000, exp = -4, sign = 0
  frexp10    : sig = 3000000, exp = -4
  normalize  = 300

  value      = 300
  decompose  : sig = 3000000, exp = -4, sign = 0
  frexp10    : sig = 3000000, exp = -4
  normalize  = 300

  value      = 300
  decompose  : sig = 3000000, exp = -4, sign = 0
  frexp10    : sig = 3000000, exp = -4
  normalize  = 300

)ex"));
}
}  // namespace ex_decompose_frexp10_normalize

// libs/decimal/example/first_example.cpp
namespace ex_first_example {
int example_main()
{
    using namespace boost::decimal::literals; // The literals are in their own namespace like std::literals

    // First we show the result of 0.1 + 0.2 using regular doubles
    ex_out << std::fixed << std::setprecision(17)
              << "Using doubles:\n"
              << "0.1 + 0.2 = " << 0.1 + 0.2 << "\n\n";

    // Construct the two decimal values
    // We construct using the literals defined by the library
    constexpr boost::decimal::decimal64_t a {0.1_DD};
    constexpr boost::decimal::decimal64_t b {0.2_DD};

    // Now we display the result of the same calculation using decimal64_t
    ex_out << "Using decimal64_t:\n"
              << "0.1_DD + 0.2_DD = " << a + b << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: first_example") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Using doubles:
0.1 + 0.2 = 0.30000000000000004

Using decimal64_t:
0.1_DD + 0.2_DD = 0.3000000000000000
)ex"));
}
}  // namespace ex_first_example

// libs/decimal/example/fmt_format.cpp
namespace ex_fmt_format {
#if defined(BOOST_DECIMAL_HAS_FMTLIB_SUPPORT) && defined(BOOST_DECIMAL_TEST_FMT)

// tag::example[]
int example_main()
{
    constexpr boost::decimal::decimal64_t val1 {"3.14"};
    constexpr boost::decimal::decimal32_t val2 {"3.141"};

    // The easiest is no specification which is general format
    // Given these values they will print in fixed format
    ex_out << "Default Format:\n";
    ex_out << fmt::format("{}", val1) << '\n';
    ex_out << fmt::format("{}", val2) << "\n\n";

    // Next we can add a type modifier to get scientific formatting
    ex_out << "Scientific Format:\n";
    ex_out << fmt::format("{:e}", val1) << '\n';
    ex_out << fmt::format("{:e}", val2) << "\n\n";

    // Next we can add a type modifier to get scientific formatting
    // Here this gives one digit of precision rounded according to current rounding mode
    ex_out << "Scientific Format with Specified Precision:\n";
    ex_out << fmt::format("{:.1e}", val1) << '\n';
    ex_out << fmt::format("{:.1e}", val2) << "\n\n";

    // Width specifies the minimum field width
    // By default, values are right-aligned with space fill
    ex_out << "Width with Default Alignment (right-align, space fill):\n";
    ex_out << fmt::format("[{:12}]", val1) << '\n';
    ex_out << fmt::format("[{:12.3e}]", val2) << "\n\n";

    // Alignment specifiers: < (left), > (right), ^ (center)
    ex_out << "Alignment Specifiers:\n";
    ex_out << fmt::format("[{:<12}]", val1) << " (left)\n";
    ex_out << fmt::format("[{:>12}]", val1) << " (right)\n";
    ex_out << fmt::format("[{:^12}]", val1) << " (center)\n\n";

    // Fill character can be any character placed before the alignment specifier
    ex_out << "Custom Fill Characters:\n";
    ex_out << fmt::format("[{:*<12}]", val1) << " (left, * fill)\n";
    ex_out << fmt::format("[{:->12}]", val1) << " (right, - fill)\n";
    ex_out << fmt::format("[{:=^12}]", val1) << " (center, = fill)\n\n";

    // Combining fill, alignment, sign, width, precision, and type
    ex_out << "Combined Format Specifiers:\n";
    ex_out << fmt::format("[{:*>+12.2e}]", val1) << " (right, * fill, + sign, precision 2, scientific)\n";
    ex_out << fmt::format("[{:-<+12.2f}]", val2) << " (left, - fill, + sign, precision 2, fixed)\n";

    return 0;
    return 0;  // main's implicit return
}
// end::example[]

#else

int example_main()
{
    ex_out << "<fmt/format> is unsupported" << std::endl;
    return 0;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: fmt_format") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(<fmt/format> is unsupported
)ex"));
}
}  // namespace ex_fmt_format

// libs/decimal/example/format.cpp
namespace ex_format {
#ifdef BOOST_DECIMAL_HAS_FORMAT_SUPPORT


int example_main()
{
    constexpr boost::decimal::decimal64_t val1 {"3.14"};
    constexpr boost::decimal::decimal32_t val2 {"3.141"};

    // The easiest is no specification which is general format
    // Given these values they will print in fixed format
    ex_out << "Default Format:\n";
    ex_out << std::format("{}", val1) << '\n';
    ex_out << std::format("{}", val2) << "\n\n";

    // Next we can add a type modifier to get scientific formatting
    ex_out << "Scientific Format:\n";
    ex_out << std::format("{:e}", val1) << '\n';
    ex_out << std::format("{:e}", val2) << "\n\n";

    // Next we can add a type modifier to get scientific formatting
    // Here this gives one digit of precision rounded according to current rounding mode
    ex_out << "Scientific Format with Specified Precision:\n";
    ex_out << std::format("{:.1e}", val1) << '\n';
    ex_out << std::format("{:.1e}", val2) << "\n\n";

    // Width specifies the minimum field width
    // By default, values are right-aligned with space fill
    ex_out << "Width with Default Alignment (right-align, space fill):\n";
    ex_out << std::format("[{:12}]", val1) << '\n';
    ex_out << std::format("[{:12.3e}]", val2) << "\n\n";

    // Alignment specifiers: < (left), > (right), ^ (center)
    ex_out << "Alignment Specifiers:\n";
    ex_out << std::format("[{:<12}]", val1) << " (left)\n";
    ex_out << std::format("[{:>12}]", val1) << " (right)\n";
    ex_out << std::format("[{:^12}]", val1) << " (center)\n\n";

    // Fill character can be any character placed before the alignment specifier
    ex_out << "Custom Fill Characters:\n";
    ex_out << std::format("[{:*<12}]", val1) << " (left, * fill)\n";
    ex_out << std::format("[{:->12}]", val1) << " (right, - fill)\n";
    ex_out << std::format("[{:=^12}]", val1) << " (center, = fill)\n\n";

    // Combining fill, alignment, sign, width, precision, and type
    ex_out << "Combined Format Specifiers:\n";
    ex_out << std::format("[{:*>+12.2e}]", val1) << " (right, * fill, + sign, precision 2, scientific)\n";
    ex_out << std::format("[{:-<+12.2f}]", val2) << " (left, - fill, + sign, precision 2, fixed)\n";

    return 0;
    return 0;  // main's implicit return
}

#else

int example_main()
{
    ex_out << "<format> is unsupported" << std::endl;
    return 0;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: format") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Default Format:
3.14
3.141

Scientific Format:
3.14e+00
3.141e+00

Scientific Format with Specified Precision:
3.1e+00
3.1e+00

Width with Default Alignment (right-align, space fill):
[        3.14]
[   3.141e+00]

Alignment Specifiers:
[3.14        ] (left)
[        3.14] (right)
[    3.14    ] (center)

Custom Fill Characters:
[3.14********] (left, * fill)
[--------3.14] (right, - fill)
[====3.14====] (center, = fill)

Combined Format Specifiers:
[***+3.14e+00] (right, * fill, + sign, precision 2, scientific)
[+3.14-------] (left, - fill, + sign, precision 2, fixed)
)ex"));
}
}  // namespace ex_format

// libs/decimal/example/integral_conversions.cpp
namespace ex_integral_conversions {
int example_main()
{
    using boost::decimal::decimal32_t;      // Type decimal32_t
    using boost::decimal::decimal64_t;      // Type decimal64_t

    // Non-finite values construct std::numeric_limits<TargetIntegerType>::max()
    constexpr decimal64_t decimal_qnan {std::numeric_limits<decimal64_t>::quiet_NaN()};
    const std::uint32_t int_from_nan {static_cast<std::uint32_t>(decimal_qnan)};

    // Note here that we must use boost::decimal::isnan for decimal types,
    // as it is illegal to overload std::isnan
    if (boost::decimal::isnan(decimal_qnan) && int_from_nan == std::numeric_limits<std::uint32_t>::max())
    {
        ex_out << "Decimal QNAN converts to Integer Max\n";
    }

    // Same thing happens with decimal infinities since integers don't have an infinity
    constexpr decimal32_t decimal_inf {std::numeric_limits<decimal32_t>::infinity()};
    const std::uint64_t int_from_inf {static_cast<std::uint64_t>(decimal_inf)};

    // Same as the above but with INF instead of NAN
    if (boost::decimal::isinf(decimal_inf) && int_from_inf == std::numeric_limits<std::uint64_t>::max())
    {
        ex_out << "Decimal INF converts to Integer Max\n";
    }

    // For finite values the construction of the resulting integer matches the behavior
    // you are familiar with from binary floating point to integer conversions.
    // Namely, the result will only have the integer component of the decimal

    // Construct the decimal64_t version of pi using our pre-computed constants from <boost/decimal/numbers.hpp>
    constexpr decimal64_t decimal_pi {boost::decimal::numbers::pi_v<decimal64_t>};
    const std::uint32_t int_from_pi {static_cast<std::uint32_t>(decimal_pi)};

    ex_out << std::setprecision(std::numeric_limits<decimal64_t>::digits10)
              << "  decimal64_t pi: " << decimal_pi << '\n'
              << "std::uint32_t pi: " << int_from_pi << "\n\n";

    // Constructing a decimal value from an integer is lossless until
    // the number of digits in the integer exceeds the precision of the decimal type

    ex_out << "Conversions will be lossless\n"
              << "  decimal64_t digits10: " << std::numeric_limits<decimal64_t>::digits10 << "\n"
              << "std::uint32_t digits10: " << std::numeric_limits<std::uint32_t>::digits10 << "\n";

    constexpr decimal64_t decimal_from_u32_max {std::numeric_limits<std::uint32_t>::max()};
    ex_out << "   std::uint32_t max: " << std::numeric_limits<std::uint32_t>::max() << "\n"
              << "decimal64_t from max: " << decimal_from_u32_max << "\n\n";

    // In the construction of lossy values the rounding will be handled according to
    // the current global rounding mode.

    ex_out << "Conversions will be lossy\n"
          << "  decimal32_t digits10: " << std::numeric_limits<decimal32_t>::digits10 << "\n"
          << "std::uint64_t digits10: " << std::numeric_limits<std::uint64_t>::digits10 << "\n";

    constexpr decimal32_t decimal_from_u64_max {std::numeric_limits<std::uint64_t>::max()};
    ex_out << "   std::uint64_t max: " << std::numeric_limits<std::uint64_t>::max() << "\n"
              << "decimal32_t from max: " << decimal_from_u64_max << '\n';

    return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: integral_conversions") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Decimal QNAN converts to Integer Max
Decimal INF converts to Integer Max
  decimal64_t pi: 3.141592653589793
std::uint32_t pi: 3

Conversions will be lossless
  decimal64_t digits10: 16
std::uint32_t digits10: 9
   std::uint32_t max: 4294967295
decimal64_t from max: 4294967295

Conversions will be lossy
  decimal32_t digits10: 7
std::uint64_t digits10: 19
   std::uint64_t max: 18446744073709551615
decimal32_t from max: 1.844674e+19
)ex"));
}
}  // namespace ex_integral_conversions

// libs/decimal/example/literals.cpp
namespace ex_literals {
int example_main()
{
    using namespace boost::decimal::literals;   // Much like the std namespace, literals are separate form the lib
    using boost::decimal::decimal32_t;  // Type decimal32_t
    using boost::decimal::decimal64_t;  // Type decimal64_t

    // Defaulted numeric constants are available with type decimal64_t,
    // much like std::numbers::pi defaults to double
    constexpr auto default_pi {boost::decimal::numbers::pi};
    using default_type = std::remove_cv_t<decltype(default_pi)>;
    static_assert(std::is_same<default_type, decimal64_t>::value, "Defaulted value has type decimal64_t");

    // You can also specify the type explicitly as these are template constants
    constexpr decimal32_t decimal32_pi {boost::decimal::numbers::pi_v<decimal32_t>};

    // We can use std::setprecision from <iomanip> to see the real difference between the two values
    // numeric_limits is also specialized for each type
    ex_out << std::setprecision(std::numeric_limits<decimal32_t>::digits10)
              << "32-bit Pi: " << decimal32_pi << '\n';

    ex_out << std::setprecision(std::numeric_limits<decimal64_t>::digits10)
              << "64-bit Pi: " << default_pi << '\n';

    // All of our types also offer user defined literals:
    // _df or _DF for decimal32_t
    // _dd or _DD for decimal64_t
    // _dl or _DL for decimal128_t
    // For fast types add an f to the end of each (e.g. _dff = decimal_fast32_t or _DLF decimal_fast128_t)
    //
    // Since we have specified the type using the literal it is safe to use auto for the type
    //
    // We construct both from the first 40 digits of pi
    // The constructor will parse this and then round to the proper precision automatically

    constexpr auto literal32_pi {"3.141592653589793238462643383279502884197"_DF};
    constexpr auto literal64_pi {"3.141592653589793238462643383279502884197"_DD};

    ex_out << std::setprecision(std::numeric_limits<decimal32_t>::digits10)
              << "32-bit UDL Pi: " << literal32_pi << '\n';

    // Unlike built-in binary floating point, floating equal is acceptable with decimal floating point
    // Float equal will automatically address cohorts as per IEEE 754 if required (not shown in this example)
    if (literal32_pi == decimal32_pi)
    {
        ex_out << "Rounded UDL has the same value as the 32-bit constant" << '\n';
    }

    ex_out << std::setprecision(std::numeric_limits<decimal64_t>::digits10)
              << "64-bit UDL Pi: " << literal64_pi << '\n';

    if (literal64_pi == default_pi)
    {
        ex_out << "Rounded UDL has the same value as the 64-bit constant" << '\n';
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: literals") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(32-bit Pi: 3.141593
64-bit Pi: 3.141592653589793
32-bit UDL Pi: 3.141593
Rounded UDL has the same value as the 32-bit constant
64-bit UDL Pi: 3.141592653589793
Rounded UDL has the same value as the 64-bit constant
)ex"));
}
}  // namespace ex_literals

// libs/decimal/example/numerical_parsing.cpp: its additions to namespace boost/std, outside the case namespace
namespace boost {
namespace decimal {

inline auto where_file(const std::string& test_vectors_filename) -> std::string
{
    // Try to open the file in each of the known relative paths
    // in order to find out where it is located.

    // Boost-root
    std::string test_vectors_filename_relative = "libs/decimal/example/" + test_vectors_filename;

    std::ifstream in_01(test_vectors_filename_relative.c_str());

    const bool file_01_is_open { in_01.is_open() };


    if(file_01_is_open)
    {
        in_01.close();
    }
    else
    {
        // Local test directory or IDE
        test_vectors_filename_relative = "../example/" + test_vectors_filename;

        std::ifstream in_02(test_vectors_filename_relative.c_str());

        const bool file_02_is_open { in_02.is_open() };

        if(file_02_is_open)
        {
            in_02.close();
        }
        else
        {
            // test/cover
            test_vectors_filename_relative = "../../example/" + test_vectors_filename;

            std::ifstream in_03(test_vectors_filename_relative.c_str());

            const bool file_03_is_open { in_03.is_open() };

            if(file_03_is_open)
            {
                in_03.close();
            }
            else
            {
                // CMake builds
                test_vectors_filename_relative = "../../../../libs/decimal/example/" + test_vectors_filename;

                std::ifstream in_04(test_vectors_filename_relative.c_str());

                const bool file_04_is_open { in_04.is_open() };

                if(file_04_is_open)
                {
                    in_04.close();
                }
                else
                {
                    // Try to open the file from the absolute path.
                    test_vectors_filename_relative = test_vectors_filename;

                    std::ifstream in_05(test_vectors_filename_relative.c_str());

                    const bool file_05_is_open { in_05.is_open() };

                    if(file_05_is_open)
                    {
                        in_05.close();
                    }
                    else
                    {
                        // Clion Cmake builds
                        test_vectors_filename_relative = "../../../libs/decimal/example/" + test_vectors_filename;

                        std::ifstream in_06(test_vectors_filename_relative.c_str());

                        const bool file_06_is_open { in_06.is_open() };
                        if (file_06_is_open)
                        {
                            in_06.close();
                        }
                        else
                        {
                            test_vectors_filename_relative = "";
                        }
                    }
                }
            }
        }
    }

    return test_vectors_filename_relative;
}

} // namespace decimal
}
// libs/decimal/example/numerical_parsing.cpp
namespace ex_numerical_parsing {
#undef BOOST_DECIMAL_EXAMPLE_WHERE_FILE_HPP
// Copyright 2025 Matt Borland
// Distributed under the Boost Software License, Version 1.0.
// https://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_DECIMAL_EXAMPLE_WHERE_FILE_HPP
#define BOOST_DECIMAL_EXAMPLE_WHERE_FILE_HPP


 // namespace boost

#endif //BOOST_DECIMAL_EXAMPLE_WHERE_FILE_HPP
// ---- end where_file.hpp ----

using boost::decimal::decimal32_t;

template <typename T>
T parse_opening_price(const std::string& line);

template <>
float parse_opening_price<float>(const std::string& line)
{
    const auto result {std::stof(line)};
    return result;
}

template <>
decimal32_t parse_opening_price<decimal32_t>(const std::string& line)
{
    decimal32_t result;
    const auto r = from_chars(line, result);

    // If we have a parse failure throw std::invalid_argument if the environment supports it,
    // using std::invalid_argument which is the same thing thrown by std::stof in the float case
    //
    // If we are in a no throw environment returning a qNaN will poison our results as well
    if (!r)
    {
        // LCOV_EXCL_START
        result = std::numeric_limits<decimal32_t>::quiet_NaN();
        BOOST_DECIMAL_THROW_EXCEPTION(std::invalid_argument("Parsing has failed"));
        // LCOV_EXCL_STOP
    }

    return result;
}

template <typename T>
T parse_csv_line(const std::string& line)
{
    std::stringstream ss(line);
    std::string token;
    std::string date;

    std::getline(ss, date, ',');
    std::getline(ss, token, ',');

    return parse_opening_price<T>(token);
}

int example_main()
{
    // We have a CSV file containing one years worth of daily stock data for AAPL
    // Here we will show the differences that arise (however small)
    // between parsing with float and decimal32_t

    // Open and read the CSV file
    std::ifstream file(boost::decimal::where_file("AAPL.csv"));
    std::string line;

    // Skip header line
    std::getline(file, line);

    std::vector<decimal32_t> decimal_opening_prices;
    std::vector<float> float_opening_prices;

    // Parse each line once into decimal32_t and once into a float
    while (std::getline(file, line))
    {
        decimal_opening_prices.emplace_back(parse_csv_line<decimal32_t>(line));
        float_opening_prices.emplace_back(parse_csv_line<float>(line));
    }

    // Use std::accumulate to get the sum of all the pricing information in the array
    // This will be used to compare the total value parsed
    const auto decimal_sum {std::accumulate(decimal_opening_prices.begin(),
        decimal_opening_prices.end(), decimal32_t{0})};
    
    const auto float_sum {std::accumulate(float_opening_prices.begin(),
        float_opening_prices.end(), float{0})};

    // This is the reference value that was found using the sum command of the CSV
    // inside Microsoft Excel
    const std::string ms_excel_result {"52151.99"};

    ex_out << std::setprecision(std::numeric_limits<float>::digits10 + 1)
              << "Number of data points: " << decimal_opening_prices.size() << '\n'
              << "    Sum from MS Excel: " << ms_excel_result << '\n'
              << "Sum using decimal32_t: " << decimal_sum << '\n'
              << "      Sum using float: " << float_sum << std::endl;
    return 0;  // main's implicit return
}

TEST_CASE("example: numerical_parsing") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(Number of data points: 0
    Sum from MS Excel: 52151.99
Sum using decimal32_t: 0
      Sum using float: 0
)ex"));
}
}  // namespace ex_numerical_parsing

// libs/decimal/example/print.cpp
namespace ex_print {
#if __cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#  if __has_include(<print>)
#    if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
#      define BOOST_DECIMAL_HAS_PRINT_SUPPORT
#    endif
#  endif
#endif

#if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT) && defined(BOOST_DECIMAL_HAS_PRINT_SUPPORT)

// tag::example[]
int example_main()
{
    constexpr boost::decimal::decimal64_t val1 {314, -2};
    constexpr boost::decimal::decimal32_t val2 {3141, -3};

    ex_out << std::format("[{:>10.3e}]\n", val1);
    ex_out << std::format("[{:*<10.3e}]\n", val2);

    return 0;
    return 0;  // main's implicit return
}
// end::example[]

#else

int example_main()
{
    ex_out << "<print> is unsupported" << std::endl;
    return 0;
    return 0;  // main's implicit return
}

#endif

TEST_CASE("example: print") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex([ 3.140e+00]
[3.141e+00*]
)ex"));
}
}  // namespace ex_print

// libs/decimal/example/promotion.cpp
namespace ex_promotion {
int example_main()
{
    using boost::decimal::decimal32_t;
    using boost::decimal::decimal64_t;
    using boost::decimal::decimal128_t;

    // First construct two values that we will perform arithmetic with
    const decimal32_t a {"5.2"};
    const decimal64_t b {"3.9"};

    ex_out << "decimal32_t value (a): " << a << '\n'
              << "decimal64_t value (b): " << b << '\n';

    // Mixed decimal comparisons are allowed by default
    if (a > b)
    {
        ex_out << "a is greater than b" << '\n';
    }

    // Even comparison of unrepresentable values is fine
    // For example decimal32_t can't represent decimal64_t max value
    constexpr decimal64_t dec64_max {std::numeric_limits<decimal64_t>::max()};
    if (a < dec64_max)
    {
        ex_out << a << " is less than " << dec64_max << '\n';
    }

    // Danger awaits if you decide to do this yourself instead of letting the system do it for you,
    // since in this example the two should compare equal but overflowing decimal32_t makes infinity
    if (static_cast<decimal32_t>(dec64_max) < dec64_max)
    {
        ex_out << dec64_max << " is less than " << static_cast<decimal32_t>(dec64_max) << '\n';
    }

    // With mixed operations like +, -, *, / we promote to the higher precision type
    // Example: decimal32_t + decimal64_t -> decimal64_t

    // We use auto here for two reasons
    // 1) To demonstrate that it's safe
    // 2) To show the promotion with the conditional logic that follows
    const auto c {a + b};
    using c_type = std::remove_cv_t<decltype(c)>; // We used const auto so the result is const decimal64_t

    static_assert(std::is_same<c_type, decimal64_t>::value, "decimal32_t + decimal64_t is supposed to yield decimal64_t");
    ex_out << "The result of a + b is a decimal64_t: " << c << '\n';

    // Now we can look at similar promotion that occurs when an operation is performed between
    // a decimal type and an integer
    //
    // Similar to the above when we have mixed operations like +, -, *, / we always promote to the decimal type
    // Example: decimal64_t * int -> decimal64_t

    const auto d {2 * c};
    using d_type = std::remove_cv_t<decltype(d)>;
    static_assert(std::is_same<d_type, decimal64_t>::value, "decimal64_t * integer is supposed to yield decimal64_t");
    ex_out << "The result of 2 * c is a decimal64_t: " << d << '\n';

    // The full suite of comparison operators between decimal types and integers
    if (d > 5)
    {
        ex_out << d << " is greater than 5" << '\n';
    }
    return 0;  // main's implicit return
}

TEST_CASE("example: promotion") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(decimal32_t value (a): 5.2
decimal64_t value (b): 3.9
a is greater than b
5.2 is less than 1e+385
1e+385 is less than inf
The result of a + b is a decimal64_t: 9.1
The result of 2 * c is a decimal64_t: 18.2
18.2 is greater than 5
)ex"));
}
}  // namespace ex_promotion

// libs/decimal/example/rounding_mode.cpp
namespace ex_rounding_mode {
void print_rounding_mode(const boost::decimal::rounding_mode current_mode)
{
    // All 5 rounding modes are defined by the enum rounding_mode
    using boost::decimal::rounding_mode;

    switch (current_mode)
    {
        case rounding_mode::fe_dec_downward:
            ex_out << "fe_dec_downward\n";
            break;
        case rounding_mode::fe_dec_to_nearest:
            ex_out << "fe_dec_to_nearest\n";
            break;
        case rounding_mode::fe_dec_to_nearest_from_zero:
            ex_out << "fe_dec_to_nearest_from_zero\n";
            break;
        case rounding_mode::fe_dec_toward_zero:
            ex_out << "fe_dec_toward_zero\n";
            break;
        case rounding_mode::fe_dec_upward:
            ex_out << "fe_dec_upward\n";
            break;
    }
}

int example_main()
{
    // The rounding mode can only be changed at run-time if the compiler supports
    // 1. C++20 std::is_constant_evaluated()
    // 2. Intrinsics that do the same
    // If neither of the above are defined the library defines BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    // The current rounding mode can be queried with boost::decimal::fegetround
    const boost::decimal::rounding_mode default_rounding_mode = boost::decimal::fegetround();
    ex_out << "The default rounding mode is: ";
    print_rounding_mode(default_rounding_mode);

    // To set a new rounding mode use boost::decimal::fesetround
    // fesetround returns current mode after updating the global state
    //
    // If your compiler set defines BOOST_DECIMAL_NO_CONSTEVAL_DETECTION the global state can not be updated,
    // so this can be a useful check to make sure that state is what you expect it to be
    auto new_rounding_mode = boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_upward);
    ex_out << "The current rounding mode is: ";
    print_rounding_mode(new_rounding_mode);

    #ifndef BOOST_DECIMAL_NO_CONSTEVAL_DETECTION

    using namespace boost::decimal::literals;
    using boost::decimal::decimal32_t;

    const decimal32_t lhs {"5e+50"_DF};
    const decimal32_t rhs {"4e+40"_DF};

    ex_out << "lhs equals: " << lhs << '\n'
              << "rhs equals: " << rhs << '\n';

    // With upward rounding the result will be "5.000001e+50"_DF
    // Even though the difference in order of magnitude is greater than the precision of the type,
    // any addition in this mode will result in at least a one ULP difference
    const decimal32_t upward_res {lhs + rhs};
    ex_out << "  Sum with upward rounding: " << upward_res << '\n';


    new_rounding_mode = boost::decimal::fesetround(boost::decimal::rounding_mode::fe_dec_downward);
    ex_out << "The current rounding mode is: ";
    print_rounding_mode(new_rounding_mode);

    // Similar to above in the downward rounding mode any subtraction will result in at least a one ULP difference
    const decimal32_t downward_res {lhs - rhs};
    ex_out << "Sum with downward rounding: " << downward_res << '\n';

    #endif // BOOST_DECIMAL_NO_CONSTEVAL_DETECTION
    return 0;  // main's implicit return
}

TEST_CASE("example: rounding_mode") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
    CHECK(ex_out.str() == std::string_view(R"ex(The default rounding mode is: fe_dec_to_nearest
The current rounding mode is: fe_dec_upward
lhs equals: 5e+50
rhs equals: 4e+40
  Sum with upward rounding: 5.000001e+50
The current rounding mode is: fe_dec_downward
Sum with downward rounding: 4.999999e+50
)ex"));
}
}  // namespace ex_rounding_mode

// libs/decimal/example/to_from_file.cpp
namespace ex_to_from_file {
int example_main()
{
   using boost::decimal::decimal32_t;  // The type decimal32_t

   // First we need to generate some values that we will use for further usage
   // This constructs a decimal32_t from random significand and exponent within the domain of decimal32_t
   std::mt19937_64 rng {42};
   std::uniform_int_distribution<std::int32_t> significand_dist {-9'999'999, 9'999'999};
   std::uniform_int_distribution<std::int32_t> exp_dist {-50, 50};

   std::array<decimal32_t, 10> values;
   for (auto& v : values)
   {
      v = decimal32_t{significand_dist(rng), exp_dist(rng)};
   }

   // Now that we have our random decimal32_ts we will write them to file
   // using their bitwise representations with the to_bid function
   //
   // This allows us to losslessly and rapidly recover them from file
   // It is more efficient than writing the string to file with to_chars,
   // and then recovering via the string constructor or from_chars

   std::ofstream file("example_values.txt");
   if (!file.is_open())
   {
      ex_out << "Failed to open file for writing" << std::endl;
      return 1;
   }

   for (const auto& value : values)
   {
      std::uint32_t bid_value {boost::decimal::to_bid(value)};

      ex_out << " Current value: " << std::dec << value << '\n'
                << "Value as bytes: " << std::hex << bid_value << "\n\n";

      file.write(reinterpret_cast<char*>(&bid_value), sizeof(bid_value));
   }
   file.close();

   // Now that we have written all the values to file we will read them in,
   // and then convert them back them to the decimal values using from_bid
   std::ifstream read_file("example_values.txt", std::ios::binary);
   if (!read_file.is_open())
   {
      ex_out << "Failed to open file for reading" << std::endl;
      return 1;
   }

   std::array<decimal32_t, 10> recovered_values;
   for (auto& value : recovered_values)
   {
      std::uint32_t bid_value;
      read_file.read(reinterpret_cast<char*>(&bid_value), sizeof(bid_value));
      value = boost::decimal::from_bid(bid_value);
   }

   read_file.close();
   if (std::remove("example_values.txt"))
   {
      ex_out << "Failed to remove file" << std::endl;
   }

   // Verify that we recovered the same values
   bool success {true};
   for (std::size_t i {}; i < values.size(); ++i)
   {
      if (values[i] != recovered_values[i])
      {
         success = false;
         break;
      }
   }

   if (success)
   {
      ex_out << "Successfully recovered all values from file" << std::endl;
   }
   else
   {
      ex_out << "Warning: Some values did not match after recovery" << std::endl;
   }

   return 0;
    return 0;  // main's implicit return
}

TEST_CASE("example: to_from_file") {
    ex_out = std::ostringstream();  // fresh state: no flags left by another example
    int rc = example_main();
    CHECK(rc == 0);
#if !(1)  // this output is not portable there (see the file header)
    CHECK(ex_out.str() == std::string_view(R"ex( Current value: 0.000506
Value as bytes: 2dcd4c57

 Current value: -3.808117e+34
Value as bytes: c0ba1b75

 Current value: -1.656579e-12
Value as bytes: a9994703

 Current value: 2.040449e+10
Value as bytes: 349f2281

 Current value: -5.16665e+43
Value as bytes: c587e239

 Current value: -9.25265e+32
Value as bytes: c00e1e51

 Current value: -5.766669e-11
Value as bytes: aa57fe0d

 Current value: -7.641908e+38
Value as bytes: c2f49b34

 Current value: 6.31977e+29
Value as bytes: 3e606e9a

 Current value: 9.210438e-24
Value as bytes: 68ec8a46

Successfully recovered all values from file
)ex"));
#endif
}
}  // namespace ex_to_from_file
