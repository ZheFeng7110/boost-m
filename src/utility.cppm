// mcpplibs.mwrap.boost.utility — module facade over generated/exports/utility.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/compressed_pair.hpp>
#include <boost/utility.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/utility/compare_pointees.hpp>
#include <boost/utility/declval.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/utility/result_of.hpp>
#include <boost/utility/string_ref_fwd.hpp>
#include <boost/utility/string_view.hpp>
#include <boost/utility/swap.hpp>
#include <boost/utility/typed_in_place_factory.hpp>
#include <boost/utility/value_init.hpp>

export module mcpplibs.mwrap.boost.utility;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.io;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/utility.inc"

