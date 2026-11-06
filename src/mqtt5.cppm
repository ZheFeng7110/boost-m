// mcpplibs.mwrap.boost.mqtt5 — module facade over generated/exports/mqtt5.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/mqtt5.hpp>
#include <boost/mqtt5/websocket.hpp>

export module mcpplibs.mwrap.boost.mqtt5;

export import mcpplibs.mwrap.boost.asio;
export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.beast;
export import mcpplibs.mwrap.boost.container;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.random;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.static_string;
export import mcpplibs.mwrap.boost.system;
export import mcpplibs.mwrap.boost.type_traits;
export import mcpplibs.mwrap.boost.utility;

#include "../generated/exports/mqtt5.inc"

