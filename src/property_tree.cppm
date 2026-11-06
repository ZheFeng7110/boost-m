// mcpplibs.mwrap.boost.property_tree — module facade over generated/exports/property_tree.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/property_tree/info_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree_serialization.hpp>
#include <boost/property_tree/xml_parser.hpp>

export module mcpplibs.mwrap.boost.property_tree;

export import mcpplibs.mwrap.boost.any;
export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.bind;
export import mcpplibs.mwrap.boost.concept_check;
export import mcpplibs.mwrap.boost.optional;
export import mcpplibs.mwrap.boost.range;
export import mcpplibs.mwrap.boost.serialization;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/property_tree.inc"

