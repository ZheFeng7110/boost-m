// mcpplibs.mwrap.boost.format — module facade over generated/exports/format.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/format.hpp>

export module mcpplibs.mwrap.boost.format;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.utility;

#include "../generated/exports/format.inc"

