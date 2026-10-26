# boost

> Boost 1.92.0 as C++23 named modules — import the library, keep writing `boost::`.

**English** · [简体中文](README.zh.md) · [繁體中文](README.zh.hant.md) · [Upstream](https://github.com/boostorg/boost) · [Releases](https://github.com/mcpplibs/boost-m/releases) · [mcpp](https://github.com/mcpp-community/mcpp) · [Index](https://github.com/mcpplibs/mcpp-index)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue.svg)](LICENSE)
[![mcpp-community](https://img.shields.io/badge/mcpp--community-mwrap-blue)](https://github.com/mcpp-community)
[![ci-linux](https://github.com/mcpplibs/boost-m/actions/workflows/ci-linux.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-linux.yml)
[![ci-macos](https://github.com/mcpplibs/boost-m/actions/workflows/ci-macos.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-macos.yml)
[![ci-windows](https://github.com/mcpplibs/boost-m/actions/workflows/ci-windows.yml/badge.svg?branch=main)](https://github.com/mcpplibs/boost-m/actions/workflows/ci-windows.yml)

116 Boost libraries are reachable as named modules: the wrapper runs
`#include` inside a module's global module fragment and re-exports the upstream
entities with `export using`, so types, overloads, ADL and spelling stay
Boost's. The header-only, compiled and macro-only libraries keep their own
consumption rules, and the vendored Boost tree is the program's only Boost
implementation — no second copy is linked.

## Modules

- `import mcpplibs.mwrap.boost;`: the enabled public module closure (umbrella)
- `import mcpplibs.mwrap.boost.optional;`: Boost.Optional
- `import mcpplibs.mwrap.boost.filesystem;`: Boost.Filesystem (compiled)
- `import mcpplibs.mwrap.boost.json;`: Boost.JSON (compiled)
- `import mcpplibs.mwrap.boost.type_traits;`: Boost.TypeTraits (feature `type-traits`)
- `import mcpplibs.mwrap.boost.macros;`: mapped macro constants (`BOOST_VERSION`, `BOOST_LIB_VERSION`)

> More Modules API -> [Usage and API](docs/usage.md)

## Getting started

```sh
mcpp new boost-app --template mcpplibs.mwrap.boost@1.92.0
cd boost-app
mcpp run
```

In an existing project:

```toml
[dependencies]
"mcpplibs.mwrap.boost" = { version = "1.92.0", default-features = false, features = ["optional"] }
```

```cpp
import std;
import mcpplibs.mwrap.boost.optional;

int main() {
    boost::optional<int> answer = 42;
    std::println("{}", *answer);
}
```

## Features

Feature names are lowercase kebab-case; the module suffix keeps Boost's own
grouping spelling (`type-traits` enables `mcpplibs.mwrap.boost.type_traits`).

| Feature | Purpose |
|---|---|
| `default` | The 50-library baseline closure (header-only core + filesystem/regex/thread/chrono/json/serialization) |
| `all-modules` | Every module interface (117 entries); `log` and `unit-test-framework` stay explicit |
| `macros` | The mapped macro constants module; `version` re-exports it |
| `log`, `unit-test-framework` | Compiled libraries with no module: include the upstream headers |
| `backend-log-avx2`, `backend-log-ssse3` | Build-time Boost.Log dump implementation (x86_64 only, mutually exclusive) |

Features are additive across dependency edges: `default-features = false` on one
edge does not cancel a set another edge requested.

## Tests and platforms

CI builds and runs on Linux x86_64 (GCC 16.1 and LLVM), Linux arm64 (GCC),
macOS arm64 (LLVM) and Windows x86_64 (clang-msvc). Requesting an unsupported
backend fails explicitly instead of being skipped.

Two aggregate limits are documented and diagnosed rather than hidden: on GCC the
root module leaves out `mcpplibs.mwrap.boost.sort` (BMI ICE when an aggregate
re-exports it; import it directly), and a clang build of the whole
`all-modules` closure can hit the source-location limit — import library groups
instead. Pure include-only libraries (macro APIs, plus libraries with compiler
defects) are listed in [Usage](docs/usage.md).

## Examples and documentation

- [Examples](examples) — umbrella-module, explicit-feature and profile consumers
- [Usage and API](docs/usage.md) — consumption modes, macro split, features, platforms
- [Architecture](docs/architecture.md) — module layout, generation, ABI and ownership

## License

The wrapper contribution is [Apache-2.0](LICENSE) with the attribution in
[NOTICE](NOTICE). The vendored Boost tree keeps its original BSL-1.0
[README and license](upstream/boost/); the pre-mwrap wrapper contribution kept
under [upstream/boost-module](upstream/boost-module/SOURCE.md) retains its own
BSL-1.0 notice. Neither upstream component is relicensed by the root licence.
