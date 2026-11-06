// mcpplibs.mwrap.boost.container_hash — module facade over generated/exports/container_hash.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/container_hash/extensions.hpp>
#include <boost/container_hash/is_tuple_like.hpp>

export module mcpplibs.mwrap.boost.container_hash;

export import mcpplibs.mwrap.boost.mp11;

#include "../generated/exports/container_hash.inc"

