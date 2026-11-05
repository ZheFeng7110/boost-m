// mcpplibs.mwrap.boost.ptr_container — module facade over generated/exports/ptr_container.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/ptr_container/clone_allocator.hpp>
#include <boost/ptr_container/exception.hpp>
#include <boost/ptr_container/nullable.hpp>
#include <boost/ptr_container/ptr_container.hpp>
#include <boost/ptr_container/serialize_ptr_container.hpp>

export module mcpplibs.mwrap.boost.ptr_container;

export import mcpplibs.mwrap.boost.array;
export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.circular_buffer;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.range;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.unordered;

#include "../generated/exports/ptr_container.inc"

