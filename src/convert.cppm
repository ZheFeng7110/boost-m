// mcpplibs.mwrap.boost.convert — module facade over generated/exports/convert.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
// The converters follow <boost/convert.hpp>: they are not self-contained
// (tools/export_policy.toml [convert]).
module;
#include <boost/convert.hpp>
#include <boost/convert/charconv.hpp>
#include <boost/convert/lexical_cast.hpp>
#include <boost/convert/printf.hpp>
#include <boost/convert/stream.hpp>
#include <boost/convert/strtol.hpp>

export module mcpplibs.mwrap.boost.convert;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.lexical_cast;
export import mcpplibs.mwrap.boost.math;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.utility;

#include "../generated/exports/convert.inc"
