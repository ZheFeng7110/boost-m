// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/variant.hpp>
#include <boost/variant/multivisitors.hpp>
#include <boost/variant/polymorphic_get.hpp>

export module mcpplibs.mwrap.boost.variant;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/variant.inc"

