// mcpplibs.mwrap.boost.compat — module facade over generated/exports/compat.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/compat/bind_back.hpp>
#include <boost/compat/bind_front.hpp>
#include <boost/compat/function_ref.hpp>
#include <boost/compat/latch.hpp>
#include <boost/compat/move_only_function.hpp>
#include <boost/compat/shared_lock.hpp>
#include <boost/compat/to_array.hpp>
#include <boost/compat/to_underlying.hpp>

export module mcpplibs.mwrap.boost.compat;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.throw_exception;

#include "../generated/exports/compat.inc"

