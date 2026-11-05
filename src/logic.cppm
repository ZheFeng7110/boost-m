// mcpplibs.mwrap.boost.logic — module facade over generated/exports/logic.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/logic/tribool_io.hpp>

export module mcpplibs.mwrap.boost.logic;

export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;

#include "../generated/exports/logic.inc"

