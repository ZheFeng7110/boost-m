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

`container.patch` is the same upstream bug as `algorithm.patch`:
`boost/container/pmr/devector.hpp` reuses the include guard of
`pmr/vector.hpp`, so `boost::container::pmr::vector` and `vector_of` were
missing from the container module (and from any TU including devector first).

`convert.patch` is the TU-local-object case: `boost::throw_on_failure` was a
namespace-scope `const` object (internal linkage), which a module cannot
export; it becomes `inline constexpr`.

`bimap.patch` is the same case: the `_key` / `_data` lambda placeholders
of `boost/bimap/support/lambda.hpp` were references in an anonymous
namespace; they become `inline constexpr`, as `boost::lambda::_1` did.

`cobalt.patch` is the same case: the completion tokens `use_op` and
`use_task` and the awaitable tags `this_coro::allocator`, `cancelled` and
`initial`, and the protocol constants of `cobalt::io` (`tcp_v4`, `udp`,
`local_stream`, ...) are namespace-scope `constexpr` objects; they become
`inline constexpr`.

`interprocess.patch` is the same case: the creation tags (`create_only`,
`open_or_create`, ...), the lock options (`defer_lock`, `try_to_lock`,
`accept_ownership`) and `offset_type_alignment` were namespace-scope
`static const` objects, and `ec_table`, which an inline function reads, was a
`static` array (a consumer would read a module-internal copy, as with regex's
message tables); they become `BOOST_INLINE_CONSTEXPR`.

`json.patch` is the same case: `BOOST_JSON_INLINE_VARIABLE` declared
`object_kind`, `array_kind` and `string_kind` as references in an anonymous
namespace; the macro now declares them `inline constexpr`.

`url.patch` is the TU-local-object case at scale: Boost.URL's character sets
and grammar rules (`grammar::alnum_chars`, `unreserved_chars`, `uri_rule`,
`authority_rule`, ...) are namespace-scope `constexpr` objects, which have
internal linkage and cannot be exported, so the url module lacked them. The
patch spells them `BOOST_INLINE_CONSTEXPR`, as upstream already does for
`absolute_uri_rule` and `origin_form_rule`.

`math.patch`: the `float_constants` / `double_constants` /
`long_double_constants` objects of `BOOST_DEFINE_MATH_CONSTANT` and the
nonfinite facet flags (`legacy`, `signed_zero`, `trap_infinity`, `trap_nan`) had
internal linkage (`static constexpr`, `const int`) and were not exported; they
become `inline constexpr`. `tools/cubic_roots.hpp` called `sin` and the
optimizers' line search `fabs` unqualified with no `using std::...`, so they
found `::sin` / `::fabs` of the C library header, which a module's importers do
not see; the using-declarations are added.

`variant.patch` works around a gcc 16.1 internal compiler error:
`boost/variant/detail/has_result_type.hpp` declares its `no` type as a
typedef of an unnamed struct inside a class template, and gcc segfaults when
a module importer instantiates it (every `boost::geometry::index::rtree`
use, through the variant of its nodes). The struct is named.

Every patch is reviewed against the 1.92.0 tag it was written for;
`upstream/boost/SOURCE.md` records the archive and checksum the tree comes
from. A patch that neither applies nor is already applied fails the replay.

`config_cstdlib.patch` gives `boost/cstdlib.hpp`'s exit codes (`exit_success`,
`exit_failure`, `exit_exception_failure`, `exit_test_failure`) the
`BOOST_INLINE_CONSTEXPR` spelling (and the `boost/config.hpp` include that
defines it): namespace-scope `const int` objects are TU-local, so the config
module could not export them, and the Boost.Math examples return them from
`main`.
