// mcpplibs.mwrap.boost.stacktrace — module facade over generated/exports/stacktrace.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
// M4: BOOST_STACKTRACE_LINK 切到外链模式, 定义来自 libs/stacktrace/src/basic.cpp — 2026-08-11-m4-compiled-libs.md §3
#define BOOST_STACKTRACE_LINK
#include <boost/stacktrace.hpp>

export module mcpplibs.mwrap.boost.stacktrace;

export import mcpplibs.mwrap.boost.config;

#include "../generated/exports/stacktrace.inc"

