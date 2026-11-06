// mcpplibs.mwrap.boost.bloom — module facade over generated/exports/bloom.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/bloom.hpp>

export module mcpplibs.mwrap.boost.bloom;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.container_hash;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/bloom.inc"

