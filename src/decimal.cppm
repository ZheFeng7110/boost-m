// mcpplibs.mwrap.boost.decimal — module facade over generated/exports/decimal.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
// <boost/decimal/format.hpp> declares the std::formatter specializations only
// when __cpp_lib_format is defined; <version> defines it before decimal.hpp
// looks (otherwise std::format of a decimal type fails in every consumer).
#include <version>
#include <boost/decimal.hpp>
#include <boost/decimal/fmt_format.hpp>

export module mcpplibs.mwrap.boost.decimal;

#include "../generated/exports/decimal.inc"

#if defined(BOOST_DECIMAL_HAS_FORMAT_SUPPORT)
// The std::formatter partial specializations live in the GMF and nothing the
// module exports names them, so a consumer's std::format would not reach them.
// Naming them here (unexported) keeps them reachable.
namespace mcpplibs::mwrap::boost::decimal_detail {
using formatter_d32 = std::formatter<::boost::decimal::decimal32_t>;
using formatter_d64 = std::formatter<::boost::decimal::decimal64_t>;
using formatter_d128 = std::formatter<::boost::decimal::decimal128_t>;
using formatter_df32 = std::formatter<::boost::decimal::decimal_fast32_t>;
using formatter_df64 = std::formatter<::boost::decimal::decimal_fast64_t>;
using formatter_df128 = std::formatter<::boost::decimal::decimal_fast128_t>;
}  // namespace mcpplibs::mwrap::boost::decimal_detail
#endif

