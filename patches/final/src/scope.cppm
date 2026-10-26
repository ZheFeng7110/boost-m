// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/scope/defer.hpp>
#include <boost/scope/error_code_checker.hpp>
#include <boost/scope/scope_fail.hpp>
#include <boost/scope/scope_success.hpp>
#include <boost/scope/unique_fd.hpp>

export module mcpplibs.mwrap.boost.scope;

// NB: no `export import mcpplibs.mwrap.boost.core;` — gcc 16.1.0 ICEs (Segmentation fault at
// the export-module line) when this GMF include set (which pulls
// unique_fd.hpp, whose entities depend on mcpplibs.mwrap.boost.core) is combined with
// re-exporting mcpplibs.mwrap.boost.core. scope.inc carries no boost::core:: entities (they
// are claimed by mcpplibs.mwrap.boost.core itself), so consumers import mcpplibs.mwrap.boost.core on their
// own. (Clang has no such issue; tracked as a gcc bug for M6 CI.)

#include "../generated/exports/scope.inc"

