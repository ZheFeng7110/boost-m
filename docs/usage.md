# Usage and API

> `mcpplibs.mwrap.boost@1.92.0` · English · [简体中文](usage.zh.md)
> Design and generation: [architecture.md](architecture.md)

Boost 1.92.0 is wrapped as C++23 named modules. Importing a module replaces the
library's `#include`, and everything the module exports keeps its upstream
spelling: `boost::optional`, `boost::filesystem::path`, `boost::json::value`,
free operators, ADL. The wrapper adds no `mcpplibs::mwrap::boost` types — that
namespace exists only for the macro mappings described below.

## 1. Dependency declaration

```toml
# The default feature set (50-library closure)
[dependencies]
"mcpplibs.mwrap.boost" = "1.92.0"

# A specific set; default-features = false stops this edge from also pulling
# the default closure in
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["optional", "json"] }

# Everything with a module interface
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["all-modules"] }

# From the repository at the immutable release tag
[dependencies]
"mcpplibs.mwrap.boost" = { git = "https://github.com/mcpplibs/boost-m", tag = "mwrap-1.92.0", default-features = false, features = ["optional"] }
```

Version scheme: the first mwrap release is the upstream version, `1.92.0`. A
wrapper-only fix appends a revision, `1.92.0.1`, and never publishes `1.92.0.0`
(which sorts equal to `1.92.0`). Repository tags use a separate series,
`mwrap-<version>`; the old `ZheFeng7110.boost` versions and tags are not reused.

## 2. Consumption modes

### 2.1 Module import

```cpp
import mcpplibs.mwrap.boost.optional;
import mcpplibs.mwrap.boost.filesystem;

boost::optional<int> value = 42;
boost::filesystem::path p = "notes.txt";
```

The root module re-exports the closure of the currently enabled features:

```cpp
import mcpplibs.mwrap.boost;   // exactly the active feature set
```

Each feature's `.cppm` is also a public module in its own right, so a consumer
may import a submodule directly; the root additively re-exports it when its
feature is active.

### 2.2 Raw headers next to modules

The wrapper does not own the headers: `upstream/boost` is on the public include
path. A translation unit may include a Boost header directly (needed for every
macro API) and import modules in the same file — the module graph and the header
graph share one vendored tree.

```cpp
#include <boost/scope_exit.hpp>          // macro API: include-only library
import mcpplibs.mwrap.boost.optional;    // module API

BOOST_SCOPE_EXIT(void) { /* ... */ } BOOST_SCOPE_EXIT_END
```

### 2.3 Include-only libraries

Some libraries are consumed by header only. They have no module and no feature.

| Group | Libraries | Reason |
|---|---|---|
| Macro APIs | `preprocessor`, `mpl`, `fusion`, `proto`, `spirit`, `xpressive`, `typeof`, `vmd`, `phoenix`, `parameter`, `metaparse`, `function_types`, `tti`, `local_function`, `msm`, `foreach` | The public surface is a macro family; a named module cannot export macros |
| Compiler/defect fallbacks | `predef`, `static_assert`, `exception`, `describe`, `openmethod`, `scope_exit` | Macro-body API, a keyword module name, include+import ODR conflicts, or gcc CMI pendings bugs |

`static_assert` cannot be a module at all: its name is a C++ keyword, so
`export module ...boost.static_assert;` is invalid. `boost/exception/all.hpp`
must be included by consumers: gcc 16.1 fails to load the module's pendings in
any real consumer translation unit.

### 2.4 Compiled libraries without a module

`log` and `unit-test-framework` ship compiled library TUs but no module
interface. Enable the feature and include the upstream headers:

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["log"] }
```

```cpp
#include <boost/log/trivial.hpp>
```

Boost.Test has two mutually exclusive forms: with `unit-test-framework` active
the framework TUs link and the consumer owns `main`; with it inactive, use the
official header-only aggregate `<boost/test/included/unit_test.hpp>`. Both forms
in one program double-define the framework symbols.

### 2.5 Unsupported libraries

`context`, `fiber`, `coroutine`, `locale`, `mpi`, `python`, `parameter_python`,
`graph_parallel`, `compute`, `mysql` and `redis` need assembly toolchains, ICU,
MPI, Python, OpenCL or OpenSSL beyond the vendored tree. They are not declared
and requesting them is an error.

## 3. Macros

A named module cannot export preprocessing macros. The macro surface is split:

| Surface | Entry | Use |
|---|---|---|
| Mapped object constants | `import mcpplibs.mwrap.boost.macros;` | `mcpplibs::mwrap::boost::macros::BOOST_VERSION`, `...::BOOST_LIB_VERSION` |
| Everything else | `#include <mwrap/boost/macros.hpp>` before the imports, or the specific upstream header | stringification, token pasting, conditional compilation, control flow, assertion families |

`version` is a thin module that re-exports `.macros`, so a consumer that wants
only the version constants can import `mcpplibs.mwrap.boost.version`. The
mapped entity has exactly one owner: `.macros`.

```cpp
import mcpplibs.mwrap.boost.macros;

static_assert(mcpplibs::mwrap::boost::macros::BOOST_VERSION == 109200);
const char* tag = mcpplibs::mwrap::boost::macros::BOOST_LIB_VERSION;   // "1_92"
```

An active macro expands even inside a qualified name. In a translation unit
that included `<boost/version.hpp>` (or `<mwrap/boost/macros.hpp>`), the
spelling above becomes `...::109200` and does not name the constant. Either use
the mapped constants from a translation unit without the raw macro, or record
the value and `#undef` it first:

```cpp
#include <mwrap/boost/macros.hpp>
import mcpplibs.mwrap.boost.macros;

constexpr int raw_version = BOOST_VERSION;
#undef BOOST_VERSION
#undef BOOST_LIB_VERSION
static_assert(mcpplibs::mwrap::boost::macros::BOOST_VERSION == raw_version);
```

The mapped values keep their type and evaluation frequency: `BOOST_VERSION` is
an `int`, `BOOST_LIB_VERSION` a `const char*`. Assertion macros stay macros —
`BOOST_ASSERT(expr)` evaluates `expr` once and disappears under `NDEBUG`; a
function cannot reproduce that. `tests/macros.cpp` and `tests/macros_raw.cpp`
cover both halves and the collision.

## 4. Features

Names are lowercase kebab-case. The module suffix keeps Boost's grouping
spelling, so feature `type-traits` enables `mcpplibs.mwrap.boost.type_traits`.

| Feature | Effect |
|---|---|
| `default` | 50-library closure (see below) |
| `all-modules` | All 117 module entries (116 library interfaces + `macros`); not `log`, not `unit-test-framework` |
| `macros`, `version` | The mapping module and its thin re-export |
| `backend-log-avx2`, `backend-log-ssse3` | Build-time Boost.Log dump implementation, x86_64 only, one per axis |

The default closure is closed over the module import edges:
`optional variant variant2 any core container-hash mp11 type-traits algorithm
iterator range io endian tuple system filesystem regex thread chrono json array
assert compat concept-check config dynamic-bitset functional integer intrusive
lambda lexical-cast move multi-array ratio smart-ptr throw-exception tokenizer
type-index units utility winapi atomic container date-time math random
serialization numeric version macros`.

Features are additive across dependency edges. `default-features = false` on one
edge only closes the default set that edge would have requested: another edge may
request defaults again, and explicit features always activate. Enabling a feature
adds its `implies` closure, so selecting one library pulls exactly what its
interface imports.

### 4.1 Build-time profiles

`backend-log-avx2` / `backend-log-ssse3` compile Boost.Log's AVX2/SSSE3 dump TUs
with per-TU `-mavx2` / `-mssse3` and the package-side dispatch macros. The base
build deliberately excludes those TUs (unconditional `<immintrin.h>` breaks
non-x86 targets). Both are x86_64-gated and mutually exclusive on the same axis;
`build.mcpp` refuses the other architecture and a second profile on the axis with
a diagnostic naming the features.

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, backend = "log-avx2" }
```

## 5. Platforms, ABI and runtime

Verified CI legs: Linux x86_64 with GCC 16.1 and LLVM, Linux arm64 with GCC,
macOS arm64 with LLVM (Apple linker; lld-macho duplicates a zero-unwind entry),
Windows x86_64 with clang-msvc. Real MSVC (`cl`), MinGW and external-SDK
libraries are not claimed.

ABI-relevant configuration is fixed by the package, not by the consumer:

- `BOOST_FILESYSTEM_VERSION=3` and `BOOST_THREAD_VERSION=2` — the upstream
  defaults the exports were generated against, pinned so an environment cannot
  desynchronize the module interface from the library TUs;
- `BOOST_ALL_NO_LIB`, `_MT`, `WIN32_LEAN_AND_MEAN`, `SECURITY_WIN32`;
- thread TUs additionally get `BOOST_THREAD_BUILD_LIB` (per-glob, never
  propagated);
- C++23, exceptions and RTTI on, the root's toolchain stdlib and runtime.

A consumer defining one of those macros does not change what is frozen in the
BMI. Configurations outside this set must use header-only consumption
(`#include`), not the modules.

## 6. Aggregate limits

- GCC 16.1 ICEs while writing the BMI of any module that re-exports
  `mcpplibs.mwrap.boost.sort` indirectly through the umbrella. The root module
  leaves `sort` out under GCC and emits a warning; import
  `mcpplibs.mwrap.boost.sort` directly (allowed and unaffected).
- Clang's 2^31 source-location limit can be reached while building the whole
  `all-modules` closure. Import library groups instead of the umbrella; the
  limit is a compiler boundary, not a package defect.
- Group-level success does not prove the umbrella closure: `all-modules` is a
  module set, not a claim that every compiler can aggregate it.

## 7. Migrating from `ZheFeng7110.boost`

| Old | New |
|---|---|
| `[dependencies.ZheFeng7110] boost = "1.92.0.0.0.1"` | `[dependencies] "mcpplibs.mwrap.boost" = { version = "1.92.0", features = [...] }` |
| `import boost.optional;` | `import mcpplibs.mwrap.boost.optional;` |
| `import boost;` | `import mcpplibs.mwrap.boost;` |
| `features = ["type_traits"]` | `features = ["type-traits"]` |
| `features = ["all"]` | `features = ["all-modules"]` |
| `boost::BOOST_VERSION` (old wrapper extension) | `mcpplibs::mwrap::boost::macros::BOOST_VERSION` |

`boost::optional<int>`, `boost::filesystem::path` and every other upstream
entity are unchanged. The old package's versions, tags and archives stay
immutable; this is a new identity, not a rename.

## 8. Toolchain pin and tests

The tested build tool is pinned in `.xlings.json` (`mcpp 2026.10.24.1`); CI runs
the same pin on every leg. Generation (regenerating export lists or auditing
headers) additionally needs LLVM 23.1.3 with matching libclang and the pinned
MinGW sysroot under `tools/`; ordinary consumers only use the committed
generated files.

```sh
mcpp build                      # default feature closure
mcpp test                       # every tests/*.cpp as its own program
mcpp test optional              # one test by pattern
mcpp test --features unit-test-framework   # compiled Boost.Test form
```

Per-OS shells, ABI variants and the include-only lists above define what the
test matrix covers; a build-only result is reported as such in CI.
