// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/container_hash/extensions.hpp>
#include <boost/container_hash/is_tuple_like.hpp>

export module mcpplibs.mwrap.boost.container_hash;

export import mcpplibs.mwrap.boost.mp11;

#include "../generated/exports/container_hash.inc"

