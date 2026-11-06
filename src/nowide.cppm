// mcpplibs.mwrap.boost.nowide — module facade over generated/exports/nowide.inc
// (hand-maintained; tools/gen_exports.py --check verifies its GMF include set)
module;
#include <boost/nowide/args.hpp>
#include <boost/nowide/cstdlib.hpp>
#include <boost/nowide/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/nowide/iostream.hpp>
// M11 platform guard (POSIX): cstdio/stackstring/convert were reached
// only via windows-flavored transitive includes in the mingw snapshot;
// include the cross-platform headers explicitly so the shared-surface
// `using` lines resolve.
#include <boost/nowide/cstdio.hpp>
#include <boost/nowide/stackstring.hpp>
#include <boost/nowide/convert.hpp>
#include <boost/nowide/quoted.hpp>
#include <boost/nowide/stat.hpp>

export module mcpplibs.mwrap.boost.nowide;

export import mcpplibs.mwrap.boost.config;
export import mcpplibs.mwrap.boost.filesystem;
export import mcpplibs.mwrap.boost.io;

#include "../generated/exports/nowide.inc"

