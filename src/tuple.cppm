// mcpplibs.mwrap.boost.tuple — module facade over generated/exports/tuple.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/tuple/tuple_comparison.hpp>
#include <boost/tuple/tuple_io.hpp>

export module mcpplibs.mwrap.boost.tuple;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/tuple.inc"

