// mcpplibs.mwrap.boost.tokenizer — module facade over generated/exports/tokenizer.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/tokenizer.hpp>

export module mcpplibs.mwrap.boost.tokenizer;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/tokenizer.inc"

