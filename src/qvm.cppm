// mcpplibs.mwrap.boost.qvm — module facade over generated/exports/qvm.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/qvm.hpp>
#include <boost/qvm/operations.hpp>
#include <boost/qvm/quat_traits_array.hpp>

export module mcpplibs.mwrap.boost.qvm;

#include "../generated/exports/qvm.inc"

