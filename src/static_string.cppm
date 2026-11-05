// mcpplibs.mwrap.boost.static_string — module facade over generated/exports/static_string.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/static_string.hpp>

export module mcpplibs.mwrap.boost.static_string;

export import mcpplibs.mwrap.boost.container_hash;
export import mcpplibs.mwrap.boost.core;

#include "../generated/exports/static_string.inc"

