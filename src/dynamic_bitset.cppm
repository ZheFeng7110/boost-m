// mcpplibs.mwrap.boost.dynamic_bitset — module facade over generated/exports/dynamic_bitset.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/dynamic_bitset.hpp>
#include <boost/dynamic_bitset/serialization.hpp>

export module mcpplibs.mwrap.boost.dynamic_bitset;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.container_hash;
export import mcpplibs.mwrap.boost.core;

#include "../generated/exports/dynamic_bitset.inc"

