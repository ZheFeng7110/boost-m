// mcpplibs.mwrap.boost.stl_interfaces — module facade over generated/exports/stl_interfaces.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/stl_interfaces/sequence_container_interface.hpp>
#include <boost/stl_interfaces/view_interface.hpp>

export module mcpplibs.mwrap.boost.stl_interfaces;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;

#include "../generated/exports/stl_interfaces.inc"

