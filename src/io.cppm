// M3 final form (derived from the tools/gen_exports.py draft; hand-finalized)
module;
#include <boost/io/nullstream.hpp>
#include <boost/io/ostream_joiner.hpp>
// M11: ostream_put.hpp curated out of the io module GMF — its
// buffer_fill enum mismatches between the mcpplibs.mwrap.boost.io/mcpplibs.mwrap.boost.utility CMIs
// on gcc 16.1; consumers include the header themselves.
#include <boost/io/quoted.hpp>

export module mcpplibs.mwrap.boost.io;

#include "../generated/exports/io.inc"

