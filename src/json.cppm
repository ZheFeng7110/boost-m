// mcpplibs.mwrap.boost.json — module facade over generated/exports/json.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/json/debug_printers.hpp>
#include <boost/json.hpp>

export module mcpplibs.mwrap.boost.json;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.container;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.endian;
export import mcpplibs.mwrap.boost.mp11;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.variant2;

#include "../generated/exports/json.inc"


#if defined(__clang__)
// The GMF's boost/container/pmr/polymorphic_allocator.hpp brings
// allocator_traits::construct, which places elements with the global
// operator new(std::size_t, void*, boost_container_new_t). Nothing this module
// exports names that operator, so clang drops it from the module and a
// consumer's instantiation (pmr::vector<json::value>::resize) finds no match;
// naming it here (unexported) keeps it reachable. gcc: the container module
// exports it instead (export_policy.toml), which on clang would re-export
// every global operator new and make libc++'s own allocations ambiguous.
namespace mcpplibs::mwrap::boost::json_detail {
inline void* container_placement_new(void* p) noexcept {
    return ::operator new(sizeof(int), p, ::boost_container_new_t());
}
}  // namespace mcpplibs::mwrap::boost::json_detail
#endif
