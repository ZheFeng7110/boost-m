// mcpplibs.mwrap.boost.any — module facade over generated/exports/any.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/any.hpp>
#include <boost/any/basic_any.hpp>
#include <boost/any/unique_any.hpp>

export module mcpplibs.mwrap.boost.any;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.type_index;

#include "../generated/exports/any.inc"

