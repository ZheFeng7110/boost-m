// mcpplibs.mwrap.boost.random — module facade over generated/exports/random.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/random.hpp>
#include <boost/random/faure.hpp>
#include <boost/random/niederreiter_base2.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/sobol.hpp>

export module mcpplibs.mwrap.boost.random;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.dynamic_bitset;
export import mcpplibs.mwrap.boost.integer;
export import mcpplibs.mwrap.boost.io;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/random.inc"

