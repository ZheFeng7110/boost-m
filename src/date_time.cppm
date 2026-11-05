// mcpplibs.mwrap.boost.date_time — module facade over generated/exports/date_time.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/date_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/local_timezone_defs.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

export module mcpplibs.mwrap.boost.date_time;

export import mcpplibs.mwrap.boost.algorithm;
export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.io;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.numeric;
export import mcpplibs.mwrap.boost.throw_exception;
export import mcpplibs.mwrap.boost.tokenizer;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/date_time.inc"

