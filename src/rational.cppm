// mcpplibs.mwrap.boost.rational — module facade over generated/exports/rational.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/rational.hpp>

export module mcpplibs.mwrap.boost.rational;

export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/rational.inc"

