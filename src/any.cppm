// mcpplibs.mwrap.boost.any — module facade over generated/exports/any.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/any.hpp>
#include <boost/any/basic_any.hpp>
#include <boost/any/unique_any.hpp>

export module mcpplibs.mwrap.boost.any;

#include "../generated/exports/any.inc"

