// mcpplibs.mwrap.boost.unordered — module facade over generated/exports/unordered.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/unordered/concurrent_flat_map.hpp>
#include <boost/unordered/concurrent_flat_set.hpp>
#include <boost/unordered/concurrent_node_map.hpp>
#include <boost/unordered/concurrent_node_set.hpp>
#include <boost/unordered/hash_traits.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_map_fwd.hpp>
#include <boost/unordered/unordered_node_map.hpp>
#include <boost/unordered/unordered_node_set.hpp>
#include <boost/unordered/unordered_set.hpp>
#include <boost/unordered/unordered_set_fwd.hpp>

export module mcpplibs.mwrap.boost.unordered;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.container_hash;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.throw_exception;

#include "../generated/exports/unordered.inc"

