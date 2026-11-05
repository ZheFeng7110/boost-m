// mcpplibs.mwrap.boost.filesystem — module facade over generated/exports/filesystem.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/filesystem.hpp>
#include <boost/filesystem/cstdio.hpp>
#include <boost/filesystem/fstream.hpp>

export module mcpplibs.mwrap.boost.filesystem;

export import mcpplibs.mwrap.boost.io;
export import mcpplibs.mwrap.boost.iterator;
export import mcpplibs.mwrap.boost.smart_ptr;
export import mcpplibs.mwrap.boost.system;

#include "../generated/exports/filesystem.inc"

