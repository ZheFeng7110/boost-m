// mcpplibs.mwrap.boost.hana — module facade over generated/exports/hana.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/hana.hpp>
#include <boost/hana/experimental/printable.hpp>
#include <boost/hana/experimental/type_name.hpp>
#include <boost/hana/experimental/types.hpp>
#include <boost/hana/experimental/view.hpp>
#include <boost/hana/ext/boost.hpp>
#include <boost/hana/ext/std.hpp>
#include <boost/hana/fwd/core.hpp>

export module mcpplibs.mwrap.boost.hana;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.tuple;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/hana.inc"

