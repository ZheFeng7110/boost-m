// mcpplibs.mwrap.boost.poly_collection — module facade over generated/exports/poly_collection.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/poly_collection/algorithm.hpp>
#include <boost/poly_collection/any_collection.hpp>
#include <boost/poly_collection/base_collection.hpp>
#include <boost/poly_collection/exception.hpp>
#include <boost/poly_collection/function_collection.hpp>
#include <boost/poly_collection/variant_collection.hpp>

export module mcpplibs.mwrap.boost.poly_collection;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.type_erasure;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/poly_collection.inc"

