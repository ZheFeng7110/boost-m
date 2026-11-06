// mcpplibs.mwrap.boost.pool — module facade over generated/exports/pool.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>

export module mcpplibs.mwrap.boost.pool;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.integer;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/pool.inc"

