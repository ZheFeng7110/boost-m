// mcpplibs.mwrap.boost.leaf — module facade over generated/exports/leaf.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/leaf.hpp>
#include <boost/leaf/common.hpp>
#include <boost/leaf/diagnostics.hpp>
#include <boost/leaf/on_error.hpp>
#include <boost/leaf/pred.hpp>
#include <boost/leaf/serialization/boost_json_encoder.hpp>
#include <boost/leaf/serialization/nlohmann_json_encoder.hpp>
#include <boost/leaf/to_variant.hpp>

export module mcpplibs.mwrap.boost.leaf;

#include "../generated/exports/leaf.inc"

