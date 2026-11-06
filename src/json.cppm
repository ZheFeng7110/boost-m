// mcpplibs.mwrap.boost.json — module facade over generated/exports/json.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/json/debug_printers.hpp>
#include <boost/json.hpp>

export module mcpplibs.mwrap.boost.json;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.container;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.endian;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.variant2;

#include "../generated/exports/json.inc"

