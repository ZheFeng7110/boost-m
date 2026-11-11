# Architecture

> `mcpplibs.mwrap.boost@1.92.0` · English · [简体中文](architecture.zh.md)
> Consumer guide: [usage.md](usage.md)

## 1. What this repository is

A module wrapper: Boost 1.92.0's headers stay untouched upstream code, and a
C++23 module layer makes them importable while preserving the upstream API.
The wrapper does not re-implement Boost, does not relocate its entities into a
new namespace, and does not link a second Boost implementation.

- 117 library module interfaces + the root module + the macro mapping module.
- 22 pure include-only libraries and 2 compiled include-only features (`log`,
  `unit-test-framework`): consumed by header, listed in [usage.md](usage.md).
- 11 libraries need external toolchains/SDKs and are not declared.

## 2. Layout

```text
boost-m/
├── mcpp.toml                    package identity, features, targets (generated blocks)
├── build.mcpp                   root-closure partition generator
├── upstream/boost/              pinned official Boost tree (pruned) + SOURCE.md
├── upstream/boost-module/       retained pre-mwrap wrapper licence/README/history
├── patches/                     vendored adaptations of upstream/boost (apply_patches.py)
├── port/                        wrapper TUs (system/thread/leaf extras)
├── port/include/mwrap/boost/    <mwrap/boost/macros.hpp> include entry
├── src/                         hand-maintained module facades, root module, macros module
├── generated/exports/           export lists (.inc) and edges (.deps) — generator output only
├── generated/meta/              api-surface.json, skipped.txt, lists/maps build.mcpp reads
├── templates/                   basic (default) and lib templates
├── tests/                       one standalone program per file; tests/<lib>/ for the hardened libraries
├── examples/                    basic, git-dep, profiles consumers
├── tools/                       import, generation (+ export_policy.toml), audit, packaging, CI
└── docs/
```

Nothing wrapper-side lives under `upstream/`; nothing upstream-side lives under
`src/` or `port/`.

## 3. Module model

Each `src/<lib>.cppm` follows one shape:

```cpp
module;                        // global module fragment
#include <boost/optional.hpp>  // the exact headers the export list came from
#include <boost/optional/optional_io.hpp>

export module mcpplibs.mwrap.boost.optional;

export import mcpplibs.mwrap.boost.core;
export import mcpplibs.mwrap.boost.type_traits;

#include "../generated/exports/optional.inc"   // export namespace boost { using ...; }
```

- The facade is hand-maintained; the generator never writes it. Its GMF include
  set is exactly the bundle translation unit the export list was computed from
  (the library's public include DAG, minus headers that do not compile on the
  snapshot platform), and the `.inc` file carries the generated
  `export namespace boost { using ...; }` list. `gen_exports.py --check`
  verifies both halves agree: the GMF equals the bundle (plus any
  `facade-includes` the policy declares), and the `export import` block
  covers every `.deps` edge, because the `.inc` leaves out what an earlier
  module owns. An edge the facade cannot take (range does not import regex:
  abi-tag mismatch under gcc) is declared in the policy's `[<lib>.facade]`
  `omit-imports` with a reason.
- One entity has one export owner. A dependency that another target library owns
  becomes an `export import` edge in the primary module, recorded in `<lib>.deps`
  as the complete module name.
- Cross-library type references, friend operators, ADL helpers and constants
  that a patch gave external linkage are part of the audit; an unexportable
  entity is documented in [usage.md](usage.md) with a raw-header alternative,
  never silently pruned.
- `src/version.cppm` is a thin entry re-exporting `mcpplibs.mwrap.boost.macros`;
  the mapped constants live only in `src/macros.cppm`.

## 4. The root module

`import mcpplibs.mwrap.boost;` must re-export exactly the modules of the active
feature set. A committed file cannot express that, and a build-time rewrite of a
committed file cannot be safe when several consumers build the same checkout.

The interface root `src/boost.cppm` is therefore fixed:

```cpp
export module mcpplibs.mwrap.boost;
export import :closure;
```

and `build.mcpp` writes the partition `:closure` into `MCPP_OUT_DIR`
(`target/.build-mcpp/out/<configuration>/<package>/boost_closure.cppm`), then
selects it with `mcpp::source()` using the absolute path. The output directory is
per configuration — target, profile, toolchain and feature set — so two builds
with different features cannot overwrite each other's closure, and the package
root stays read-only. The file is written only when its content changes, so an
unchanged feature set does not rebuild the module.

`build.mcpp` also validates the profile features: one profile per axis, and an
arch-gated profile only on its architecture. Both failures are diagnosed before
the module graph is scanned.

Known aggregate limits are reported, not hidden: under GCC the closure leaves out
`mcpplibs.mwrap.boost.sort` (BMI ICE when an aggregate re-exports it) and warns;
a clang build of the whole closure can hit the source-location limit.

## 5. Generation chain

The principle: **the compiler computes the public surface, the generator only
lists it, and nothing edits the result.** The upstream headers are the single
source of truth; the generated files are reproducible from them byte for byte.

```sh
python3 tools/import_boost.py --tarball boost_1_92_0.tar.gz   # repopulates upstream/boost/
python3 tools/apply_patches.py             # vendored adaptations (patches/*.patch)
python3 tools/gen_exports.py               # generated/exports/*, api-surface.json, skipped.txt
python3 tools/gen_features.py              # mcpp.toml blocks + generated/meta/*.lst
python3 tools/gen_exports.py --check       # byte-compare, writes nothing
python3 tools/gen_features.py --check
python3 tools/gen_audit.py                 # linkage audit (AST, target/gen/audit)
python3 tools/gen_audit.py --macros        # macro-surface inventory
```

### 5.1 The AST comes from clang++

For each library one bundle TU includes the facade's GMF headers and is compiled
once:

```sh
clang++ -std=c++23 --target=x86_64-w64-mingw32 --sysroot=<pinned MinGW> \
        -fsyntax-only -Xclang -ast-dump=json -Xclang -ast-dump-filter=boost bundle.cpp
```

The same invocation is the syntax gate: when it fails, the GMF headers named by
the diagnostics (platform-exclusive alternatives such as the pthread/win32 thread
backends) are pruned and the bundle is recompiled; the pruned set is recorded in
`api-surface.json`. `-ast-dump-filter=boost` keeps the `namespace boost`
subtrees only. The dump of an instantiation-heavy library is still several GB,
so `tools/ast_json.py` reads it as a stream into a flat declaration table:

- clang prints a location's file only when it differs from the previously
  printed location, so the reader carries that state across every location in
  output order — including the ones it drops — and never trusts a single
  `loc.file` in isolation;
- implicit template instantiations (every declaration of a `*TemplateDecl`
  after its pattern), `UsingShadowDecl` copies, comments, source ranges and
  record definition data are dropped while streaming; references to a dropped
  instantiation resolve to its template;
- references found in a declaration's subtree (declaration references,
  lookups, type-node declarations, written types) are attributed to the
  namespace-scope entity that owns it.

Only the clang++ driver pinned in `.xlings.json` (`llvm`) is needed — no
libclang, no Python bindings, no LLVM release archive.

### 5.2 What is exported

- **Candidates**: namespace-scope `boost::` entities with external linkage whose
  definition lives in a header the library owns (`boost/<lib>/**`,
  `boost/<lib>.hpp`) or in no target library's headers ("shared"). Linkage is
  read from the AST: anonymous namespaces, `static`, non-inline const variables
  and Itanium `L`-marked manglings are internal; typedef names are exportable
  members. Compiler-implicit declarations, friend-only declarations, explicit
  and partial specializations (reachable through the primary template),
  deduction guides and class members are not candidates.
- **Using-injections**: namespace-scope `using X::y;` / `using namespace X;`
  that give entities their public spelling (ADL barriers, `detail` re-exports).
- **Signature closure**: entities named by the candidates' signatures
  (parameters, results, bases, fields, typedef targets — with typedefs looked
  through), so the API's own types are exported with it.
- **Ownership**: libraries are processed dependencies-first; the first library
  to claim an entity owns it, later ones get an `export import` edge to the
  owner in `<lib>.deps`. Pass 2 walks the bodies of every exported entity and
  adds the edges a consumer instantiating them needs (cycle-closing and
  transitive edges skipped).
- Output is sorted, carries no paths or timestamps, and uses LF; a name
  shadowed inside the export block (`boost::hana::ext::boost`) is spelled with a
  leading `::`.

### 5.3 What the AST cannot decide: `tools/export_policy.toml`

The snapshot is one platform flavor (MinGW x86_64, libstdc++). Every human
decision about the surface is declared in `tools/export_policy.toml` with a
written reason and applied by the generator, never patched into its output:

| Rule | Effect |
|---|---|
| `[[<lib>.guard]]` | wrap the matching exports in the upstream header's own condition (`#if defined(_WIN32)` ...), optional `#else` lines |
| `[[<lib>.drop]]` | never export the matching names / namespaces |
| `[[<lib>.extra]]` | verbatim declarations (the gcc `extern template` vtable workarounds) |
| `add` | names no collector reaches (body-only or ADL-only references) |
| `gmf`, `defines`, `facade-includes` | the bundle's include set and macros; facade-only includes |
| `inline-namespaces` | export a macro-selected inline namespace through its parent |
| `[<lib>.deps]` | module edge corrections; edges of the hand-written special modules |

`generated/meta/skipped.txt` lists every name a rule guards, drops or adds, so
the boundary is reviewable in one file, and `generated/meta/api-surface.json`
records the generator identity (clang release, target, defines) and per-library
GMF, export count and edges. A diff after a compiler update therefore shows as a
generator change, not as an unexplained API change.

### 5.4 Checks

- `gen_exports.py --check` regenerates into a scratch copy and compares every
  generated file byte for byte, then verifies each facade's GMF against the
  bundle; it never writes the tree. CI runs it on Linux, macOS and Windows.
- `import_boost.py` is the single upstream import: URL, archive SHA-256, kept
  entries, pruned directories/files and the CRLF-normalized files are recorded in
  `upstream/boost/SOURCE.md`. The release `README.md` and `LICENSE_1_0.txt` are
  copied byte-for-byte; `.gitattributes` disables text conversion under
  `upstream/**`.
- `apply_patches.py` replays `patches/*.patch` onto `upstream/boost/` only
  (forward/reverse idempotent, `patch -p1` fallback without Git); see
  [patches/README.md](../patches/README.md). It never touches generated files or
  facades.
- `gen_features.py` splices two marked blocks in `mcpp.toml` and writes
  `generated/meta/{libs.json,features.lst,modules.lst,profiles.lst}`. It is
  byte- and mtime-idempotent. Feature names are kebab-case; `implies` lists
  feature names (never internal keys), derived from each interface's
  `export import` lines plus the generated body edges. `modules.lst` is the
  feature → module map `build.mcpp` reads; `all-modules` expands to the 118 module
  entries.
- `gen_audit.py` reports internal-linkage functions, anonymous-namespace
  entities and explicit specializations per library from the same AST; its
  report is reviewed with the committed tree, not treated as "not public API".

Generation needs the clang++ pinned in `.xlings.json` and the pinned MinGW
sysroot (`tools/fetch_mingw_sysroot.py`); set `CLANG_NO_DEFAULT_CONFIG=1` when
the xlings LLVM package installs a host config file. Consumers need neither:
they use the committed output.

## 6. Features

One feature per library, additive across dependency edges. A feature's sources
are its interface plus, for compiled libraries, its upstream `src/**` TU globs.
`log`'s and `unit-test-framework`'s TUs are feature-gated; the rest also appear
in the base source list so `mcpp test` sees the full surface while `mcpp build`
compiles only active features.

`mcpp build` mode drops every feature-listed source and re-adds the active ones;
`mcpp test` keeps the base list and excludes only feature-provided sources. That
pair of rules is why the base list is generated from the union and why the
Boost.Test framework TUs are deliberately feature-only (the two Boost.Test forms
must never link together).

## 7. ABI and ownership

The module TUs, the library TUs and raw-header consumers share one configuration:

- `BOOST_FILESYSTEM_VERSION=3`, `BOOST_THREAD_VERSION=2` — the baseline the
  exports were generated against, pinned in `[build].defines`;
- `BOOST_ALL_NO_LIB`, `_MT`, `WIN32_LEAN_AND_MEAN`, `SECURITY_WIN32`;
- per-glob `BOOST_THREAD_BUILD_LIB` on `upstream/boost/libs/thread/src/**`;
- `port/include` and `upstream/boost` public; log/atomic private include dirs are
  listed in `private_include_dirs`;
- `-w` for vendored header noise; `-Wno-invalid-specialization` under clang
  (poly_collection specializes `std::is_void`);
- the profile's `NDEBUG` state reaches the headers, so release-only declaration
  differences follow upstream: Boost.uBLAS's tensor `BOOST_UBLAS_INLINE` ordering
  is corrected by `patches/numeric.patch` (it is ill-formed when the macro expands
  to `inline`), and the checked-build-only `same_impl_ex` export is guarded by
  the `numeric` rule of `tools/export_policy.toml`.

The same `upstream/boost` tree feeds the interfaces, the library TUs and raw
header consumption. One Boost implementation per program: there is no
`compat.boost` dependency, and a second copy must not be added.

## 8. Licensing

- Wrapper contribution (modules, port, tools, templates, tests, docs):
  Apache-2.0, `LICENSE` + `NOTICE` (mcpp-community).
- Boost upstream under `upstream/boost/`: BSL-1.0, its own `README.md` and
  `LICENSE_1_0.txt`, hashed in `upstream/documents.json`.
- Retained pre-mwrap wrapper contribution under `upstream/boost-module/`: BSL-1.0,
  `SOURCE.md` records repository, commits and old identity.

`runtime.deploy` ships all three under distinct `licenses/boost/...`
destinations, so equal basenames cannot overwrite one another; a consumer bundle
keeps byte-identical notices.

## 9. CI gates

- Native build + `mcpp test` on Linux x86_64 (GCC 16.1, LLVM), Linux arm64 (GCC),
  macOS arm64 (LLVM), Windows x86_64 (clang-msvc).
- `tests/<lib>/` (34 libraries): the official examples ported as cases with an
  output oracle (`examples*.cpp`), the documentation's code (`doc*.cpp`,
  `api*.cpp`), user types at each customization point (`custom_types.cpp`), and
  `coverage.toml`, which maps every example and documentation section of the
  1.92.0 release to a case or a reason; `tools/ci/test_coverage.py check` runs
  in CI. `tests/dll/` (plugin shared libraries as workspace members) and
  `tests/test/utf/` (the compiled Boost.Test form) are projects of their own,
  excluded from the root `[test] discover` and run by CI in their directories.
- `tools/ci/verify_generated.py`: patch replay, `gen_exports.py --check`,
  `gen_features.py --check` and the audits on a clean tree on all three host
  platforms, then `git status` must be clean; a second build on the same
  configuration records the incremental TU count and mtimes.
- `tools/check_mwrap_release.py`: identity, version scheme, tag, licence split and
  retained-document bytes.
- `tools/ci/test_template.py`: installs an isolated `MCPP_HOME` fixture, runs a real
  `mcpp new`, parses the rendered manifest (complete selector, version, features),
  then builds and runs the project — no sed rendering.
- `tools/package-source.sh`: `boost-m-<version>.tar.gz` from the tracked tree,
  hash next to the archive, compressed with a bounded thread count. The release
  workflow publishes that asset for the immutable `mwrap-<version>` tag only.

## 10. Local resource budget

Heavy builds stay below 80% of the machine's effective CPU/memory: set
`MCPP_JOBS`/`-j`, avoid parallel full-tree builds, and keep AST generation
single-threaded per library. The vendored tree is 400+ MB; a local fixture copy
hardlinks it when the filesystem allows.
