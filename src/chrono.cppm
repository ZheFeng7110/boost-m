// mcpplibs.mwrap.boost.chrono — module facade over generated/exports/chrono.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/chrono.hpp>
#include <boost/chrono/typeof/boost/chrono/chrono.hpp>
#include <boost/chrono/typeof/boost/ratio.hpp>

export module mcpplibs.mwrap.boost.chrono;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.integer;
export import mcpplibs.mwrap.boost.move;
export import mcpplibs.mwrap.boost.ratio;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/chrono.inc"

