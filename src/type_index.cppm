// mcpplibs.mwrap.boost.type_index — module facade over generated/exports/type_index.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/type_index/runtime_cast.hpp>
#include <boost/type_index/runtime_cast/boost_shared_ptr_cast.hpp>
#include <boost/type_index/runtime_cast/std_shared_ptr_cast.hpp>

export module mcpplibs.mwrap.boost.type_index;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.container_hash;
export import mcpplibs.mwrap.boost.throw_exception;

#include "../generated/exports/type_index.inc"

