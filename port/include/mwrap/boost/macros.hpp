// <mwrap/boost/macros.hpp> — the include entry of the Boost macro surface.
//
// `import mcpplibs.mwrap.boost.macros;` maps the object-like constants
// (BOOST_VERSION, BOOST_LIB_VERSION). Everything else in Boost's macro surface
// must expand where it is written — token pasting, stringification, conditional
// compilation, assertion/control-flow families and the Boost.Preprocessor
// machinery cannot cross a module boundary.
//
// Include this header *before* the imports of a translation unit that needs
// those macros:
//
//     #include <mwrap/boost/macros.hpp>
//     import mcpplibs.mwrap.boost.macros;
//
// This header re-includes the upstream headers byte-for-byte; it does not
// reimplement Boost.Preprocessor, and it defines no macro of its own. A
// translation unit that includes it keeps the raw macros defined, which means
// the qualified spellings of the mapped constants expand as macros there — use
// the mapped constants from a translation unit that does not include this
// header, or #undef the macro after recording its value.
#ifndef MWRAP_BOOST_MACROS_HPP
#define MWRAP_BOOST_MACROS_HPP

// Version macros (the raw form; `BOOST_VERSION` is also mapped as a constant in
// mcpplibs::mwrap::boost::macros).
#include <boost/version.hpp>

// The macro-driven libraries have no named module: their API is a macro family
// and consumers include the upstream header for it. These are the roots most
// consumers need; include the specific family header for anything narrower.
#include <boost/assert.hpp>
#include <boost/config.hpp>

#endif // MWRAP_BOOST_MACROS_HPP
