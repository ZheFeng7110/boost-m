# patches

Adaptations of the vendored Boost tree. `tools/apply_patches.py` applies every
`*.patch` here to `upstream/boost/` after `tools/import_boost.py` restored the
pinned release, and is idempotent in both directions (an applied patch is
skipped by the reverse check; `patch -p1` is the fallback without Git).

The patches are minimal upstream adaptations: TU-local objects promoted to
`inline constexpr` so a CMI and a consumer GMF cannot emit the same mangled
symbol twice; internal-linkage/static helpers given external linkage for the
export surface; platform guards for the vendored 1.92.0 tree; the `system`
split that moves function-local statics out of headers.

Only the vendored tree is patched. The module layer is not: the export lists
under `generated/exports/` are written by `tools/gen_exports.py` alone — every
guard, drop or verbatim line it needs is a rule with a written reason in
`tools/export_policy.toml` — and `src/*.cppm` are hand-maintained facades that
`gen_exports.py --check` verifies against the generated data. Nothing is
replayed over generator output.

`numeric.patch` is the one functional adaptation with a release-mode edge:
Boost.uBLAS tensor spells `BOOST_UBLAS_INLINE` before the `template<...>` head
in a few member functions. `BOOST_UBLAS_INLINE` is empty in a checked build but
`inline` under `NDEBUG`, and `inline template<...>` is ill-formed (gcc rejects
it, clang misreads it as an explicit instantiation), so a release build of the
numeric module needs the swap. The exported `same_impl_ex` follows the same
header condition through the `numeric` guard in `tools/export_policy.toml`, so
the release module neither exports a name the header hides nor fails on it.

`algorithm.patch` fixes an upstream bug rather than adapting to modules:
`boost/algorithm/cxx17/transform_inclusive_scan.hpp` reuses the include guard
of `transform_reduce.hpp`, so a translation unit that includes both loses the
second one. The algorithm module's GMF includes them in that order, and
`boost::algorithm::transform_reduce` was missing from its export list; plain
`#include` consumers hit the same bug. The patch gives the header its own guard.

Every patch is reviewed against the 1.92.0 tag it was written for;
`upstream/boost/SOURCE.md` records the archive and checksum the tree comes
from. A patch that neither applies nor is already applied fails the replay.
