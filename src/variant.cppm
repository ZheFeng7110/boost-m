// mcpplibs.mwrap.boost.variant — module facade over generated/exports/variant.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/variant.hpp>
#include <boost/variant/multivisitors.hpp>
#include <boost/variant/polymorphic_get.hpp>

export module mcpplibs.mwrap.boost.variant;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/variant.inc"

