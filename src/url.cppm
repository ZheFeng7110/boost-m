// mcpplibs.mwrap.boost.url — module facade over generated/exports/url.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/url.hpp>

export module mcpplibs.mwrap.boost.url;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.variant2;

#include "../generated/exports/url.inc"

