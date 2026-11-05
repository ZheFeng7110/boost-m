// mcpplibs.mwrap.boost.assign — module facade over generated/exports/assign.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/assign.hpp>
#include <boost/assign/ptr_list_inserter.hpp>
#include <boost/assign/ptr_list_of.hpp>
#include <boost/assign/ptr_map_inserter.hpp>

export module mcpplibs.mwrap.boost.assign;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.move;
export import mcpplibs.mwrap.boost.ptr_container;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/assign.inc"

