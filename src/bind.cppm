// mcpplibs.mwrap.boost.bind — module facade over generated/exports/bind.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/bind.hpp>
#include <boost/bind/apply.hpp>
#include <boost/bind/make_adaptable.hpp>
#include <boost/bind/protect.hpp>

export module mcpplibs.mwrap.boost.bind;

export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.core;

#include "../generated/exports/bind.inc"

