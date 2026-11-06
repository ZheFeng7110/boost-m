// mcpplibs.mwrap.boost.array — module facade over generated/exports/array.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/array.hpp>

export module mcpplibs.mwrap.boost.array;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.throw_exception;

#include "../generated/exports/array.inc"

