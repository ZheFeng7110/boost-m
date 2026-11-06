// mcpplibs.mwrap.boost.dll — module facade over generated/exports/dll.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/dll.hpp>
#include <boost/dll/import_class.hpp>
#include <boost/dll/shared_library_load_mode.hpp>

export module mcpplibs.mwrap.boost.dll;

export import mcpplibs.mwrap.boost.assert;
export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.filesystem;
export import mcpplibs.mwrap.boost.io;
export import mcpplibs.mwrap.boost.type_index;
export import mcpplibs.mwrap.boost.winapi;

#include "../generated/exports/dll.inc"

