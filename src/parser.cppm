// mcpplibs.mwrap.boost.parser — module facade over generated/exports/parser.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/parser/split.hpp>
#include <boost/parser/transform_replace.hpp>

export module mcpplibs.mwrap.boost.parser;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.type_index;

#include "../generated/exports/parser.inc"

