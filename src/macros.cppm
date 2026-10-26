// mcpplibs.mwrap.boost.macros — the macro surface that can be mapped.
//
// A named module cannot export preprocessing macros, so the Boost macro surface
// is split in two (see docs/usage.md "Macros"):
//
//   * this module maps the object-like constants that keep their value, type
//     and evaluation semantics when written as `inline constexpr` objects;
//   * <mwrap/boost/macros.hpp> is the documented include entry for the macros
//     that must expand at the call site (stringification, token pasting,
//     conditional compilation, control flow) and for the raw upstream headers.
//
// The mapped entities live in `mcpplibs::mwrap::boost::macros` under their
// original macro spelling. The upstream C++ API stays in `boost::`.
//
// Each mapping reads the macro into a detail constant and then #undef's it, so
// the names below are afterwards usable only as qualified constants. A consumer
// translation unit that still sees the raw macro (because it included
// <boost/version.hpp> itself) expands it even in a qualified name; use the
// mapped constant from a translation unit without the raw macro, or #undef it
// after recording the value. tests/macros.cpp and tests/version.cpp cover both.
module;

#include <boost/version.hpp>

export module mcpplibs.mwrap.boost.macros;

namespace mcpplibs::mwrap::boost::macros::detail {

inline constexpr int version_value = BOOST_VERSION;
inline constexpr const char* lib_version_value = BOOST_LIB_VERSION;

} // namespace mcpplibs::mwrap::boost::macros::detail

// The raw macros end here: the mapped constants below are the only spelling
// left in this translation unit.
#undef BOOST_VERSION
#undef BOOST_LIB_VERSION

export namespace mcpplibs::mwrap::boost::macros {

// `#define BOOST_VERSION 109200` — the numeric release of the vendored headers.
inline constexpr int BOOST_VERSION = detail::version_value;

// `#define BOOST_LIB_VERSION "1_92"` — the tag Boost's own library names use.
inline constexpr const char* BOOST_LIB_VERSION = detail::lib_version_value;

} // namespace mcpplibs::mwrap::boost::macros
