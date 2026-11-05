// mcpplibs.mwrap.boost.type_erasure — module facade over generated/exports/type_erasure.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/type_erasure/any_cast.hpp>
#include <boost/type_erasure/dynamic_any_cast.hpp>
#include <boost/type_erasure/free.hpp>
#include <boost/type_erasure/is_empty.hpp>
#include <boost/type_erasure/iterator.hpp>
#include <boost/type_erasure/member.hpp>
#include <boost/type_erasure/register_binding.hpp>
#include <boost/type_erasure/same_type.hpp>
#include <boost/type_erasure/tuple.hpp>

export module mcpplibs.mwrap.boost.type_erasure;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.utility;

#include "../generated/exports/type_erasure.inc"

