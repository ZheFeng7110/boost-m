// mcpplibs.mwrap.boost.circular_buffer — module facade over generated/exports/circular_buffer.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/circular_buffer.hpp>

export module mcpplibs.mwrap.boost.circular_buffer;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/circular_buffer.inc"

