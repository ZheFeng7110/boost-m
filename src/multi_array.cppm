// mcpplibs.mwrap.boost.multi_array — module facade over generated/exports/multi_array.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/multi_array.hpp>

export module mcpplibs.mwrap.boost.multi_array;

export import mcpplibs.mwrap.boost.array;
export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.concept_check;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.functional;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/multi_array.inc"

