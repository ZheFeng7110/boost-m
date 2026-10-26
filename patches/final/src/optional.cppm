// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>

export module mcpplibs.mwrap.boost.optional;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/optional.inc"

