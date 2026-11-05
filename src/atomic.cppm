// mcpplibs.mwrap.boost.atomic — module facade over generated/exports/atomic.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/atomic.hpp>

export module mcpplibs.mwrap.boost.atomic;

export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.winapi;

#include "../generated/exports/atomic.inc"

