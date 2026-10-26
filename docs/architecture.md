# Architecture

> `mcpplibs.mwrap.boost@1.92.0` · English · [简体中文](architecture.zh.md)
> Consumer guide: [usage.md](usage.md)

## 1. What this repository is

A module wrapper: Boost 1.92.0's headers stay untouched upstream code, and a
C++23 module layer makes them importable while preserving the upstream API.
The wrapper does not re-implement Boost, does not relocate its entities into a
new namespace, and does not link a second Boost implementation.

- 116 library module interfaces + the root module + the macro mapping module.
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
├── patches/                     replayable adaptations; final/<path> = stored forms
├── port/                        wrapper TUs (system/thread/leaf extras)
├── port/include/mwrap/boost/    <mwrap/boost/macros.hpp> include entry
├── src/                         module interfaces, root module, macros module
├── generated/exports/           committed export lists (.inc) and edges (.deps)
├── generated/meta/              committed lists/maps the generators and build.mcpp read
├── templates/                   basic (default) and lib templates
├── tests/                       one standalone program per file
├── examples/                    basic, git-dep, profiles consumers
├── tools/                       import, generation, audit, packaging, CI helpers
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

- The GMF include set comes from the library's public include DAG, minus headers
  that are not standalone-parseable; the `.inc` file carries the
  `export namespace boost { using ...; }` list generated from a libclang AST
  snapshot.
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

```sh
python3 tools/import_boost.py --tarball boost_1_92_0.tar.gz   # repopulates upstream/boost/
python3 tools/reapply_hand_edits.py        # vendored adaptations
python3 tools/gen_exports.py --emit-cppm   # generated/exports/*.inc|*.deps + draft .cppm
python3 tools/reapply_hand_edits.py        # module-side adaptations, final forms
python3 tools/gen_features.py              # mcpp.toml blocks + generated/meta/*
python3 tools/gen_features.py --check
python3 tools/gen_audit.py                 # export completeness (AST)
python3 tools/gen_audit.py --macros        # macro-surface inventory
git diff --exit-code && git status --short
```

- `import_boost.py` is the single upstream import: URL, archive SHA-256, kept
  entries, pruned directories/files and the CRLF-normalized files are recorded in
  `upstream/boost/SOURCE.md`. The release `README.md` and `LICENSE_1_0.txt` are
  copied byte-for-byte; `.gitattributes` disables text conversion under
  `upstream/**`.
- `reapply_hand_edits.py` never reads Git history. It applies `patches/*.patch`
  (forward/reverse idempotent, with a `patch -p1` fallback when Git is absent)
  and replays hand-maintained final forms from `patches/final/<path>`. A patch
  whose anchor is missing is an error unless listed in `TOLERANT` with a written
  reason.
- `gen_exports.py` keeps its two passes: the libclang AST snapshot produces the
  export lists, and a body-reference pass completes the `.deps` edges. Caches and
  bundle TUs live under the gitignored `target/gen/`.
- `gen_features.py` splices two marked blocks in `mcpp.toml` and writes
  `generated/meta/{libs.json,features.lst,modules.lst,profiles.lst}`. It is
  byte- and mtime-idempotent. Feature names are kebab-case; `implies` lists
  feature names (never internal keys), derived from each interface's
  `export import` lines plus the generated body edges. `modules.lst` is the
  feature → module map `build.mcpp` reads; `all-modules` expands to the 117 module
  entries.
- `gen_audit.py` is the migration gate for export completeness; its report is
  reviewed with the committed tree, not treated as "not public API" on failure.

Generation needs LLVM 23.1.3 with matching libclang and the pinned MinGW sysroot
(`tools/fetch_mingw_sysroot.py`, `tools/uv.lock`). Consumers do not: they use the
committed output.

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
  (poly_collection specializes `std::is_void`).

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
- `tools/ci/verify_generated.py`: import/replay, generation, feature splice and
  audit on a clean tree, with `git diff`/`git status` as the oracle; a second
  build on the same configuration records the incremental TU count and mtimes.
- `tools/check_mwrap_release.py`: identity, version scheme, tag, licence split and
  retained-document bytes.
- `tools/test_template.py`: installs an isolated `MCPP_HOME` fixture, runs a real
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
