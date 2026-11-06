// mcpplibs.mwrap.boost.intrusive — module facade over generated/exports/intrusive.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/intrusive/any_hook.hpp>
#include <boost/intrusive/avl_set.hpp>
#include <boost/intrusive/bs_set.hpp>
#include <boost/intrusive/derivation_value_traits.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/member_value_traits.hpp>
#include <boost/intrusive/pointer_plus_bits.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/sg_set.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/splay_set.hpp>
#include <boost/intrusive/treap_set.hpp>
#include <boost/intrusive/trivial_value_traits.hpp>
#include <boost/intrusive/unordered_set.hpp>

export module mcpplibs.mwrap.boost.intrusive;

export import mcpplibs.mwrap.boost.move;

#include "../generated/exports/intrusive.inc"

