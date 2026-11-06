// mcpplibs.mwrap.boost.convert — module facade over generated/exports/convert.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/convert.hpp>

export module mcpplibs.mwrap.boost.convert;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/convert.inc"

