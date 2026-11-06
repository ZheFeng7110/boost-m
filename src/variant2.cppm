// mcpplibs.mwrap.boost.variant2 — module facade over generated/exports/variant2.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/variant2.hpp>

export module mcpplibs.mwrap.boost.variant2;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.mp11;

#include "../generated/exports/variant2.inc"

