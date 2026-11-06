// mcpplibs.mwrap.boost.assert — module facade over generated/exports/assert.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/assert.hpp>
#include <boost/assert/source_location.hpp>

export module mcpplibs.mwrap.boost.assert;

export import mcpplibs.mwrap.boost.config;

#include "../generated/exports/assert.inc"

