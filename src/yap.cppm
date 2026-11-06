// mcpplibs.mwrap.boost.yap — module facade over generated/exports/yap.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/yap/print.hpp>
#include <boost/yap/yap.hpp>

export module mcpplibs.mwrap.boost.yap;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.hana;
export import mcpplibs.mwrap.boost.type_index;

#include "../generated/exports/yap.inc"

