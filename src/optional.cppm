// mcpplibs.mwrap.boost.optional — module facade over generated/exports/optional.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

export module mcpplibs.mwrap.boost.optional;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/optional.inc"

