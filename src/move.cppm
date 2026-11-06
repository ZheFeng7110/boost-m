// mcpplibs.mwrap.boost.move — module facade over generated/exports/move.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/move/algo/adaptive_merge.hpp>
#include <boost/move/algo/adaptive_sort.hpp>
#include <boost/move/algo/predicate.hpp>
#include <boost/move/algo/unique.hpp>
#include <boost/move/make_unique.hpp>
#include <boost/move/move.hpp>

export module mcpplibs.mwrap.boost.move;

export import mcpplibs.mwrap.boost.config;

#include "../generated/exports/move.inc"

