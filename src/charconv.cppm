// mcpplibs.mwrap.boost.charconv — module facade over generated/exports/charconv.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/charconv.hpp>

export module mcpplibs.mwrap.boost.charconv;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.core;

#include "../generated/exports/charconv.inc"

