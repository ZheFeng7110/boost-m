// mcpplibs.mwrap.boost.integer — module facade over generated/exports/integer.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/integer/common_factor.hpp>
#include <boost/integer/integer_log2.hpp>
#include <boost/integer/integer_mask.hpp>
#include <boost/integer/mod_inverse.hpp>
#include <boost/integer/static_log2.hpp>
#include <boost/integer/static_min_max.hpp>

export module mcpplibs.mwrap.boost.integer;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/integer.inc"

