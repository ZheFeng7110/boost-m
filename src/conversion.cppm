// mcpplibs.mwrap.boost.conversion — module facade over generated/exports/conversion.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/implicit_cast.hpp>
#include <boost/polymorphic_cast.hpp>
#include <boost/polymorphic_pointer_cast.hpp>

export module mcpplibs.mwrap.boost.conversion;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.throw_exception;

#include "../generated/exports/conversion.inc"
