// mcpplibs.mwrap.boost.timer — module facade over generated/exports/timer.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/timer/progress_display.hpp>
#include <boost/timer/timer.hpp>

export module mcpplibs.mwrap.boost.timer;

export import mcpplibs.mwrap.boost.config;

#include "../generated/exports/timer.inc"

