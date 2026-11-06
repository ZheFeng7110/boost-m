// mcpplibs.mwrap.boost.signals2 — module facade over generated/exports/signals2.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/signals2.hpp>
#include <boost/signals2/trackable.hpp>

export module mcpplibs.mwrap.boost.signals2;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.bind;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.function;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.move;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.variant;

#include "../generated/exports/signals2.inc"

