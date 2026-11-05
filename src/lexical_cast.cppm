// mcpplibs.mwrap.boost.lexical_cast — module facade over generated/exports/lexical_cast.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/lexical_cast.hpp>

export module mcpplibs.mwrap.boost.lexical_cast;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.container;
export import mcpplibs.mwrap.boost.core;

#include "../generated/exports/lexical_cast.inc"

