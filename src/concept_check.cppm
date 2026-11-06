// mcpplibs.mwrap.boost.concept_check — module facade over generated/exports/concept_check.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/concept_check.hpp>
#include <boost/concept_check/borland.hpp>
#include <boost/concept_check/general.hpp>

export module mcpplibs.mwrap.boost.concept_check;

export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/concept_check.inc"

