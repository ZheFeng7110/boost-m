#!/usr/bin/env python3
"""boost_common.py — shared plumbing for the module-export generators.

Handles the vendored layout: all library headers live under
upstream/boost/boost/
(the aggregated include root; libs/<lib>/ has no include/). This module knows
the target library list, resolves each library's public headers (the bundle
translation unit / .cppm GMF include set), the include-based dependency graph
and the clang++ command line shared by gen_exports.py / gen_audit.py. The AST
itself comes from the compiler (tools/ast_json.py); nothing here needs
libclang.
"""

import json
import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
# `upstream/boost/` is the pinned official release (pruned); its aggregated
# include root is `upstream/boost/boost`, and the compiled libraries' sources
# live under `upstream/boost/libs/<lib>/src`. Wrapper code never goes there.
DEPS = ROOT / "upstream" / "boost"
BOOST_ROOT = DEPS / "boost"          # aggregated include root (all lib headers)
TOOLS = ROOT / "tools"
LIBS_JSON = ROOT / "generated" / "meta" / "libs.json"
CURATED_DIR = TOOLS / "curated"
GEN_DIR = ROOT / "generated" / "exports"
# gitignored scratch area for generator caches (gate verdicts, incremental-skip
# keys, bundle TUs) — see gen_exports.py.
CACHE_DIR = ROOT / "target" / "gen" / "exports-cache"
AUDIT_DIR = ROOT / "target" / "gen" / "audit"
MODULE_PREFIX = "mcpplibs.mwrap.boost"


def _boost_version():
    """Upstream version from mcpp.toml [package.metadata.mwrap] (the single
    place it is recorded; generated headers and the API surface read it)."""
    import tomllib
    with open(ROOT / "mcpp.toml", "rb") as f:
        meta = tomllib.load(f)
    return meta["package"]["metadata"]["mwrap"]["upstream-version"]


BOOST_VERSION = _boost_version()

# M3 header-only libraries, then M4 compiled libraries (plan order), then the
# M9 T1a batch (regular header-only libs, no src/ TU, no public macro surface).
# C1 downgrade (2026-09-06): scope_exit is include-only now — its public API
# is the BOOST_SCOPE_EXIT_* macro family (macros never cross module
# boundaries, M10 rule), and gcc 16.1 ODR-conflicts on include+import mixing
# (describe/smoke.cpp precedent), so consumers #include <boost/scope_exit.hpp>.
LIBS_M3 = [
    "optional", "variant", "variant2", "any", "core", "container_hash",
    "mp11", "static_string", "scope", "type_traits",
    "algorithm", "iterator", "range", "io", "rational", "endian",
    "tuple", "system",
]
LIBS_M4 = [
    "filesystem", "regex", "thread", "chrono", "program_options",
    "stacktrace", "json", "url",
]
# M9 T1a (boost-mcpp-all-libs-features-plan.md §2): 58 regular header-only
# libraries. Excluded: predef (pure .h macro lib → T3 include-only), coroutine2
# (depends on boost/context → T4), property_map_parallel (no headers in the
# aggregated boost/ include root), static_assert (pure macro lib with 0
# exported entities; its module name is ALSO invalid C++ — `static_assert` is a
# keyword, clang rejects `export module boost.static_assert;` — so it stays
# include-only like the T3 macro libs), plus the T1b heavy-template opt-in
# batch (M12) and the T2 compiled batch (M11).
# C1 downgrade (2026-09-06): describe + openmethod moved to
# LIBS_INCLUDE_ONLY_C1 — macro-body APIs (BOOST_DESCRIBE_* /
# BOOST_OPENMETHOD_*, M10 boundary: macros never cross module boundaries);
# gcc 16.1 ODR-conflicts on include+import mixing in the same TU, so the
# module face forced consumers to pick one and the macro face won.
# C2 re-entry (2026-09-06): hof + units were excluded in M9 (public API is
# internal-linkage `static`/`const constexpr` objects — boost::hof::compose/_1
# and boost::units::si::meter etc.). The C2 vendored macro patches
# (hof/detail/static_const_var.hpp + units/static_constant.hpp →
# `inline constexpr`, replayed by apply_patches.py) give the objects
# external linkage, so both libraries are target libs again.
# C4 re-entry (2026-09-07): bind + lambda + lambda2 were excluded in M10 as T3
# "macro-driven" — a misattribution: their macro counts are tiny and every
# macro is an implementation detail (call-convention config, X-macro helpers,
# include guards); none appears in user code. The real APIs are function
# templates + placeholder objects. bind (boost::arg, boost::bind,
# boost::placeholders::_1.._9 via BOOST_INLINE_CONSTEXPR) and lambda2 (inline
# constexpr) were already module-safe as-is. Classic lambda needed the C2
# treatment: its _1.._3/_e placeholders were TU-local (anonymous namespace) —
# the C4 vendored patches (replayed by apply_patches.py) convert them to
# inline constexpr (external linkage). Caveat: the deprecated boost/bind.hpp
# global `using namespace boost::placeholders;` cannot be reproduced by a
# module — consumers of the module face write their own using-directive
# (the upstream-recommended spelling anyway).
# Conversion (2026-11-09, .agents/docs/2026-11-09-module-test-hardening-design.md
# §5.3): its public headers sit at the top of boost/ (implicit_cast.hpp,
# polymorphic_cast.hpp, polymorphic_pointer_cast.hpp) and boost/conversion/
# holds only detail/, so the prefix heuristic saw an empty library and the
# 2026-09-08 inventory filed it as a "no-header stub". See EXTRA_HEADERS.
LIBS_T1A = [
    "align", "array", "assert", "assign", "bimap", "bind", "bloom",
    "callable_traits", "circular_buffer", "compat", "concept_check",
    "config", "conversion", "convert", "crc", "decimal",
    "dll", "dynamic_bitset", "flyweight", "format", "function",
    "functional", "hash2", "heap", "histogram", "hof", "icl",
    "integer", "intrusive", "lambda", "lambda2", "leaf", "lexical_cast", "lockfree",
    "logic", "move", "multi_array", "multi_index",
    "outcome", "parser", "pfr", "poly_collection", "pool",
    "property_map", "property_tree", "ptr_container", "ratio",
    "safe_numerics", "signals2", "smart_ptr", "sort", "statechart",
    "stl_interfaces", "throw_exception", "tokenizer",
    "type_index", "units", "unordered", "utility", "uuid", "winapi",
    "yap",
]
# M11 T2 (boost-mcpp-all-libs-features-plan.md §2/§4): compiled libraries —
# one module each plus its libs/<lib>/src TUs (see the M11 design doc §2 for
# the per-lib TU table: exclusions for external-library backends, per-OS dirs
# and upstream CMake deviations). math has src/tr1 only (deprecated std::tr1
# wrappers; upstream CMake is INTERFACE) — its feature ships no TU.
# C1 downgrade (2026-09-06): log + test moved to LIBS_COMPILED_INCLUDE_ONLY.
LIBS_T2 = [
    "atomic", "charconv", "cobalt", "container", "contract", "date_time",
    "graph", "iostreams", "math", "nowide", "process",
    "random", "serialization", "timer", "type_erasure", "wave",
]
# M12 T1b (boost-mcpp-all-libs-features-plan.md §2/§4): heavy-template
# header-only opt-in libraries. All twelve ship no src/ TU (verified:
# header-only upstream too), so their features are module-interface-only.
# opt-in: NOT in DEFAULT_CANDIDATES (plan §5.2 keeps the lean 18-lib core).
# asio lands here as a module of its own (its detail surface was already
# reachable through cobalt/process since M11, guarded by WIN32_LEAN_AND_MEAN
# in CLANG_ARGS). compute/mysql/redis moved to T4 (user decision 2026-09-03):
# their core surfaces hard-require external SDK headers (OpenCL / OpenSSL)
# that are absent from the vendored tree — same boundary as M13's
# external-dependency libraries.
LIBS_T1B = [
    "accumulators", "asio", "beast", "geometry", "gil",
    "hana", "interprocess", "mqtt5", "multiprecision", "numeric",
    "polygon", "qvm",
]

# M10 T3 (boost-mcpp-all-libs-features-plan.md §2): macro-driven include-only
# libraries — no module, no feature, no build work (user decision §5.3). Their
# public API surface is macro-injection-driven (BOOST_PP_/BOOST_FUSION_/
# BOOST_SPIRIT_... families); a named module can neither export macros nor
# stay in sync with the config-macro-driven re-configuration these headers
# perform, so consumers #include them directly (import + include mixing is
# standard-compliant). The boundary is verified by gen_audit.py --macros.
# C4 reclassification (2026-09-07): bind + lambda + lambda2 moved out of T3 —
# they entered via the M10 own-family macro-percentage screen, which proves
# "large macro face", not "the API is a macro" (a real macro-API library
# cannot have single-digit macro counts). lambda's include-only verdict was
# coincidentally right but wrongly attributed: it is the internal-linkage
# placeholder objects (hof/units class, fixed C4), not macros. 19 -> 16 libs.
LIBS_T3 = [
    "preprocessor", "mpl", "fusion", "proto", "spirit", "xpressive",
    "typeof", "vmd", "phoenix", "parameter",
    "metaparse", "function_types", "tti", "local_function", "msm", "foreach",
]
# M13 T4 (boost-mcpp-all-libs-features-plan.md §2/§4): external-dependency /
# asm libraries — no module without the external SDK (M13 milestone).
# M12 addition (user decision 2026-09-03): compute (every real header includes
# CL/cl.h — OpenCL SDK), mysql + redis (boost/mysql.hpp / boost/redis.hpp and
# the connection core hard-require OpenSSL) — moved out of the T1b batch.
LIBS_T4 = [
    "mpi", "python", "parameter_python", "graph_parallel", "locale",
    "context", "fiber", "coroutine",
    "compute", "mysql", "redis",
]
# C5 (2026-09-09, plan .agents/plan/2026-09-09-boost-version-module-promotion.md):
# SPECIAL — module-shaped libraries that bypass gen_exports.py entirely. They
# have a hand-written `src/<lib>.cppm` but no libclang bundle TU (their exports
# are simple constexprs derived from upstream headers, not namespace-scope
# `using` candidates for the AST scanner). Mirror image of LIBS_COMPILED_INCLUDE_ONLY
# (C1): that tier has TU globs and NO module; SPECIAL has a module and NO TU
# globs. The feature still ships through gen_features.py — sources = .cppm +
# empty TU glob set, implies = empty (deps_of() returns ∅ when the .deps file
# is absent, which it intentionally is for SPECIAL).
LIBS_SPECIAL = ["version", "macros"]
TARGET_LIBS = LIBS_M3 + LIBS_M4 + LIBS_T1A + LIBS_T2 + LIBS_T1B + LIBS_SPECIAL

# Public headers the prefix rules (boost/<lib>/** minus detail/, boost/<lib>.hpp)
# do not see. Paths are relative to boost/.
#
# EXTRA_HEADERS: added to the library's public header set (its GMF) and owned
#   by it. Header resolution, file ownership and include-edge detection all
#   read it, so the three cannot disagree.
#   - conversion: its headers sit at the top of boost/ under other names.
#   - core: boost::core::string_view is a documented Core component
#     (libs/core/doc/string_view.qbk) whose only header is
#     boost/core/detail/string_view.hpp; without it no module exported the
#     string_view alias, which charconv's from_chars(string_view) overloads
#     take (only json exported basic_string_view, reached through its own GMF).
# TOP_LEVEL_OWNERS: ownership (and include edges) only, for top-level files;
#   the file stays out of the owner's header set, so the owner's GMF and
#   export list do not change. boost/pointer_cast.hpp is smart_ptr's
#   (upstream: Boost::conversion links Boost::smart_ptr for it); without the
#   entry conversion, which reaches it first in TARGET_LIBS order, would claim
#   smart_ptr's *_pointer_cast names.
EXTRA_HEADERS = {
    "conversion": ["implicit_cast.hpp", "polymorphic_cast.hpp",
                   "polymorphic_pointer_cast.hpp"],
    "core": ["core/detail/string_view.hpp"],
}
TOP_LEVEL_OWNERS = {
    "pointer_cast.hpp": "smart_ptr",
}
for _lib, _names in EXTRA_HEADERS.items():
    for _name in _names:
        if "/" not in _name:
            TOP_LEVEL_OWNERS[_name] = _lib

# M9 downgrades to include-only (plan §2 note + 2026-08-17 M9 doc §1): pure
# macro libs (predef: .h only; static_assert: module name is a keyword) and
# internal-linkage constexpr-object APIs (hof, units) — same consumer rule
# as T3, recorded in the M9 doc.
# C2 (2026-09-06): hof + units re-entered via the vendored macro patches (see
# LIBS_T1A above) — the include-only list is back to predef/static_assert.
LIBS_INCLUDE_ONLY_M9 = ["predef", "static_assert"]
# M11 downgrade to include-only: exception — the clone_impl<T> member bodies
# attached to the boost.exception CMI as lazily loaded pendings trip gcc 16.1
# ("recursive lazy load / failed to load pendings for clone_impl") in ANY
# consumer TU that includes <memory>/<string>/<functional> — i.e. every real
# consumer. Explicit instantiations in the module TU do not dodge it. The API
# is header-only, so consumers #include <boost/exception/all.hpp> (T3 rule).
LIBS_INCLUDE_ONLY_M11 = ["exception"]
# C1 downgrade to include-only (2026-09-06 usage-reclassification plan §1):
# compiled libraries that keep a feature (their libs/<lib>/src TU globs still
# ship and link) but lose the module interface — a new form: "compiled
# include-only (feature without module)":
#  - log: the gcc consumer face has long been unusable (M11 §7.4 — the
#    exported basic_formatting_ostream operator<< instantiations collide with
#    the consumer's own; the CMI/GMF name-family collisions of rollup §3.1#5
#    are not fixed at the root), so tests already consumed it pure-include.
#    Downgrading removes the three-compiler consumption mismatch.
#  - test: macro-body API (BOOST_TEST_*) with the official compiled Unit Test
#    Framework → dual-form consumption: with the `unit_test_framework` feature
#    active the framework TUs link and the consumer owns main + the runner
#    (BOOST_TEST_NO_MAIN + impl/unit_test_main.ipp); without it the library is
#    the official header-only configuration (<boost/test/included/**>
#    aggregate,自带 main). The two forms must never link together (the
#    included aggregate and the framework TUs double-define the framework
#    symbols, M11 §3): tests/test/utf/ is a project of its own that always
#    enables the feature; tests/test/included/ (root project) is gated on
#    MCPP_FEATURE_UNIT_TEST_FRAMEWORK.
# The feature names live in gen_features.py (test is renamed
# `unit_test_framework` there, aligned with the upstream CMake target
# boost_unit_test_framework); the internal keys here stay `log` / `test`
# (COMPILED_TU_GLOBS keys).
LIBS_COMPILED_INCLUDE_ONLY = ["log", "test"]

# Target triple for the bundle AST snapshot. The committed .inc files are a
# mingw-flavor snapshot (platform-specific entities are wrapped in the guards of
# tools/export_policy.toml), so the default stays x86_64-w64-mingw32. Set
# BOOST_MODULE_GEN_TARGET (e.g. x86_64-linux-gnu) to snapshot another flavor.
_DEFAULT_GEN_TARGET = "x86_64-w64-mingw32"
GEN_TARGET = os.environ.get("BOOST_MODULE_GEN_TARGET", _DEFAULT_GEN_TARGET)


def _looks_like_sysroot(p):
    return (p / GEN_TARGET / "include" / "windows.h").is_file()


def _bundled_sysroot():
    """Locate the tools/_deps sysroot bootstrapped by
    tools/fetch_mingw_sysroot.py (marker first, then a bounded scan)."""
    base = TOOLS / "_deps"
    marker = base / "mingw-sysroot.json"
    if marker.is_file():
        try:
            data = json.loads(marker.read_text(encoding="utf-8"))
            root = ROOT / data["root"]
            if _looks_like_sysroot(root):
                return root
        except Exception:
            pass
    for depth in (1, 2, 3):
        for p in sorted(base.glob("/".join(["*"] * depth))):
            if p.is_dir() and _looks_like_sysroot(p):
                return p
    return None


# Sysroot resolution:
#   1. BOOST_MODULE_GEN_SYSROOT wins when set;
#   2. otherwise the bundled tools/_deps sysroot is used for the DEFAULT
#      mingw triple (a cross/non-mingw target must not inherit it);
#   3. otherwise no --sysroot is passed.
_ENV_SYSROOT = os.environ.get("BOOST_MODULE_GEN_SYSROOT", "")
if _ENV_SYSROOT:
    GEN_SYSROOT = _ENV_SYSROOT
elif GEN_TARGET == _DEFAULT_GEN_TARGET:
    _bundled = _bundled_sysroot()
    GEN_SYSROOT = str(_bundled) if _bundled else ""
else:
    GEN_SYSROOT = ""

TARGET_ARGS = ["--target=" + GEN_TARGET]
if GEN_SYSROOT:
    TARGET_ARGS.append("--sysroot=" + GEN_SYSROOT)

# Platform defines every bundle TU is compiled under (the AST dump and the
# syntax gate are the same clang++ invocation, so they cannot disagree):
#   _WIN32_WINNT=0x0A00     — Win10 API set. MinGW's default is older and hides
#     e.g. WaitOnAddress from boost.winapi, which made the gate mis-prune
#     boost/atomic.hpp under the pinned sysroot.
#   WIN32_LEAN_AND_MEAN     — M11: asio (cobalt/log/process TUs) errors
#     "WinSock.h has already been included" when windows.h pulled winsock1
#     before asio's winsock2 — which any boost.winapi -> windows.h chain does.
#     WIN32_LEAN_AND_MEAN makes windows.h skip winsock.h entirely; asio then
#     includes winsock2.h itself (defining _WINSOCKAPI_ instead is wrong: asio
#     treats it as evidence that winsock1 is already in and hard-errors,
#     verified).
# These are the fixed default; set BOOST_MODULE_GEN_DEFINES (space-separated,
# replaces the whole list) to snapshot a different flavor.
_DEFAULT_GEN_DEFINES = ["_WIN32_WINNT=0x0A00", "WIN32_LEAN_AND_MEAN"]
_ENV_GEN_DEFINES = os.environ.get("BOOST_MODULE_GEN_DEFINES", "")
GEN_DEFINES = _ENV_GEN_DEFINES.split() if _ENV_GEN_DEFINES.strip() \
    else list(_DEFAULT_GEN_DEFINES)

# clang++ command line used for every bundle TU. The driver finds its own
# builtin headers; tools/ast_json.py adds the AST-dump flags.
CLANG_ARGS = [
    "-std=c++23",
    "-I" + str(DEPS),
    *TARGET_ARGS,
    "-DBOOST_ALL_NO_LIB",
    *["-D" + d for d in GEN_DEFINES],
    "-w",
]


# ---------------------------------------------------------------------------
# header resolution (official tarball layout: all headers under boost/boost/)
# ---------------------------------------------------------------------------

def resolve_headers_heuristic(lib: str):
    """Public-root header set for a library: boost/<lib>/**.hpp minus detail/
    subtrees and amalgamation helpers (src.hpp), plus the aggregate single
    header boost/<lib>.hpp when it exists.

    detail/ headers are excluded deliberately: they are not self-contained
    (they rely on the root headers having set up macros and dependencies first)
    and every detail header reachable through public API is included
    transitively by the root headers. src.hpp is kept: for json it is the
    documented way to pull the .ipp implementations into one TU (header-only
    mode) — exactly what the module GMF wants; url's src.hpp is a #error stub
    and is dropped by the standalone-compile filter.

    The bundle TU / .cppm GMF includes the include-DAG sources of this set
    (see gfm_headers_of), mirroring what an upstream consumer's #include
    exposes."""
    headers = set()
    d = BOOST_ROOT / lib
    if d.is_dir():
        headers.update(p for p in d.rglob("*.hpp") if p.is_file()
                       and "detail" not in p.relative_to(BOOST_ROOT).parts)
    single = BOOST_ROOT / (lib + ".hpp")
    if single.is_file():
        headers.add(single)
    headers.update(BOOST_ROOT / name for name in EXTRA_HEADERS.get(lib, ())
                   if (BOOST_ROOT / name).is_file())

    def key(p: Path):
        rel = p.relative_to(BOOST_ROOT).as_posix()
        return (0, rel) if p == single else (1, rel)

    return sorted(headers, key=key)


def _rel(path: Path) -> str:
    return path.relative_to(BOOST_ROOT.parent).as_posix()


def load_libs_json():
    """libs.json: {lib: [rel-header-path, ...]} — committed, curated."""
    if LIBS_JSON.exists():
        return json.loads(LIBS_JSON.read_text(encoding="utf-8"))
    return None


def headers_of(lib: str):
    """Header set for a library, from libs.json when present else heuristic."""
    db = load_libs_json()
    if db and lib in db:
        return [BOOST_ROOT.parent / rel for rel in db[lib]]
    return resolve_headers_heuristic(lib)


def gfm_headers_of(lib: str):
    """GMF/bundle include list for a library: the include-DAG sources of its
    header set (plus one representative per include cycle).

    Non-detail root headers are not mutually ordered by name (chrono's
    duration_get.hpp uses duration_units.hpp without including it — upstream
    both come via the chrono/io.hpp aggregate), so a flat alphabetical list
    does not compile. Including only the aggregate entry points — headers no
    other set member includes — pulls everything else in upstream order, and
    mirrors what an upstream consumer's #include surface looks like."""
    headers = headers_of(lib)
    names = {str(h.resolve()).lower() for h in headers}
    relmap = {str(h.resolve()).lower(): h for h in headers}

    # in-set include edges: which headers include which (by resolved path)
    inc_of = {}            # rel(lower) -> set of included lower paths
    for h in headers:
        key = str(h.resolve()).lower()
        inc_of.setdefault(key, set())
        try:
            text = h.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for m in _INC_RE.finditer(text):
            inc = str((BOOST_ROOT.parent / m.group(1)).resolve()).lower()
            if inc in names and inc != key:
                inc_of[key].add(inc)

    # SCCs (Tarjan-lite via iterative DFS) on the reverse edges:
    # a header X is a "source" if no other set member includes it.
    sources = [h for h in headers
               if not any(key in incs for key, incs in inc_of.items()
                          if str(h.resolve()).lower() in incs
                          and key != str(h.resolve()).lower())]
    # hmm — simpler: a header is included by another if its key appears in
    # any other header's include set.
    included_by_other = {k for incs in inc_of.values() for k in incs}
    sources = [h for h in headers if str(h.resolve()).lower() not in included_by_other]

    if not sources:
        return sorted(headers)          # fully cyclic set — fall back

    # ensure every header is reachable: repeatedly add sources of the
    # remaining unreached subset (breaks include cycles one representative
    # per cycle).
    reached = set()

    def reach_from(seed_key):
        stack = [seed_key]
        while stack:
            k = stack.pop()
            if k in reached:
                continue
            reached.add(k)
            stack.extend(inc_of.get(k, ()))

    chosen = []
    for h in sources:
        key = str(h.resolve()).lower()
        reach_from(key)
        chosen.append(h)
    missing = [h for h in headers if str(h.resolve()).lower() not in reached]
    while missing:
        # sources of the missing subset
        sub = {str(h.resolve()).lower(): h for h in missing}
        added = False
        for k in list(sub):
            if any(k in inc_of.get(other, ()) for other in sub if other != k):
                continue
            h = sub[k]
            reach_from(k)
            chosen.append(h)
            added = True
        if not added:
            # Every remaining header is included by another member of the
            # remaining subset — a pure include cycle with no entry source
            # (first seen with boost.math, M11). Pick one representative so
            # the loop terminates; its transitive in-set includes are pulled
            # in by reach_from, the rest of the cycle on a later pass.
            h = sorted(sub.values(), key=lambda p: p.as_posix())[0]
            reach_from(str(h.resolve()).lower())
            chosen.append(h)
        missing = [h for h in headers if str(h.resolve()).lower() not in reached]
    return sorted(chosen, key=lambda p: p.as_posix())


def scan_libs_json():
    """Regenerate libs.json from the heuristic, dropping root headers that do
    not compile standalone (legacy opt-in headers — e.g.
    boost/algorithm/string/std/rope_traits.hpp requires the SGI <rope> header
    that modern libstdc++ no longer ships). Human-curate the result
    afterwards if needed."""
    db = {}
    for lib in TARGET_LIBS:
        kept, dropped = standalone_compile_filter(resolve_headers_heuristic(lib))
        db[lib] = [_rel(p) for p in kept]
        for h, why in dropped:
            print("  drop {}: {}".format(_rel(h), why))
    return db


def standalone_compile_filter(headers):
    """Return (kept, dropped) — root headers whose standalone compile reports
    an error located under upstream/boost/ are dropped (they cannot be part of
    any compilable module GMF). System-header noise is ignored."""
    import subprocess
    import ast_json
    boost_prefix = str(BOOST_ROOT.resolve()).replace("\\", "/").lower()
    kept, dropped = [], []
    tmp = ROOT / "target" / "gen" / "standalone.cpp"
    tmp.parent.mkdir(parents=True, exist_ok=True)
    for h in headers:
        tmp.write_text("#include <{}>\n".format(
            h.relative_to(BOOST_ROOT.parent).as_posix()), encoding="utf-8")
        r = subprocess.run(ast_json.clang_command("-fsyntax-only", *CLANG_ARGS,
                                                  str(tmp)),
                           capture_output=True, text=True, cwd=str(ROOT))
        bad = [line for line in r.stderr.splitlines()
               if ": error:" in line or ": fatal error:" in line]
        bad = [line for line in bad
               if line.replace("\\", "/").lower().startswith(boost_prefix)]
        if bad:
            dropped.append((h, bad[0].split("error:", 1)[-1].strip()[:100]))
        else:
            kept.append(h)
    tmp.unlink(missing_ok=True)
    return kept, dropped


def build_file_to_lib():
    """Map every header file (absolute, normalized) to its owning target lib.

    Prefix-based: every file under boost/<lib>/ (including detail/ subtrees,
    which are enumerated but not GMF-included) plus the boost/<lib>.hpp
    aggregate belongs to the library; TOP_LEVEL_OWNERS adds the top-level
    files whose name is not a library's."""
    mapping = {}
    for lib in TARGET_LIBS:
        d = BOOST_ROOT / lib
        if d.is_dir():
            for p in d.rglob("*.hpp"):
                mapping[str(p.resolve()).lower()] = lib
        single = BOOST_ROOT / (lib + ".hpp")
        if single.is_file():
            mapping[str(single.resolve()).lower()] = lib
    for name, lib in TOP_LEVEL_OWNERS.items():
        mapping[str((BOOST_ROOT / name).resolve()).lower()] = lib
    return mapping


def home_lib_of_file(path: Path, file_to_lib) -> str:
    """Owner target lib of a file; 'shared' when owned by no target lib."""
    try:
        return file_to_lib.get(str(path.resolve()).lower(), "shared")
    except OSError:
        return "shared"


# ---------------------------------------------------------------------------
# include-based dependency graph / topological order
# ---------------------------------------------------------------------------

_INC_RE = re.compile(r'^\s*#\s*include\s*[<"](boost/[A-Za-z0-9_./]+)', re.MULTILINE)


def _lib_of_include(rel: str):
    """boost/<x>... -> target lib x ('' when not a target lib root).

    Handles both directory roots (boost/<lib>/...) and single-header
    aggregates (boost/<lib>.hpp, e.g. boost/tokenizer.hpp / boost/crc.hpp),
    and the TOP_LEVEL_OWNERS files (boost/polymorphic_cast.hpp -> conversion)."""
    top = rel[len("boost/"):].split("/")[0]
    if top in TOP_LEVEL_OWNERS:
        return TOP_LEVEL_OWNERS[top]
    if "." in top:
        # single-header aggregate include (boost/<lib>.hpp)
        stem = top.rsplit(".", 1)[0]
        return stem if stem in TARGET_LIBS else ""
    if top == "detail":
        return ""
    return top if top in TARGET_LIBS else ""


def lib_deps(lib, headers, file_to_lib=None):
    """Target libraries whose headers `headers` include (transitively through
    headers no target library owns) — one row of dep_graph()."""
    deps = set()
    visited = set()
    queue = list(headers)
    while queue:
        h = queue.pop()
        try:
            key = str(h.resolve()).lower()
        except OSError:
            continue
        if key in visited:
            continue
        visited.add(key)
        try:
            text = h.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for m in _INC_RE.finditer(text):
            rel = m.group(1)
            dep = _lib_of_include(rel)
            if dep:
                if dep != lib:
                    deps.add(dep)
                continue            # target-owned root: don't recurse
            p = BOOST_ROOT.parent / rel
            if p.suffix != ".hpp" or not p.is_file():
                continue
            if str(p.resolve()).lower() in visited:
                continue
            queue.append(p)
    return deps


def dep_graph(headers_by_lib=None):
    """{lib: set(lib)} — which target libs' headers a lib's headers include.
    headers_by_lib overrides the header set per lib (defaults to libs.json /
    heuristic). The generator passes the clang++-gate-pruned GFM set here so
    deps reachable only through headers pruned from the module (e.g. the regex
    family in algorithm, dropped for the gcc abi-tag workaround) do not leak
    into <lib>.deps.

    The walk is transitive through headers NOT owned by any target library
    (e.g. icl/gregorian.hpp -> boost/date_time/... -> boost/tokenizer.hpp —
    date_time is a T2 lib not yet a target, and single-header aggregates were
    invisible to the direct-include mapping): every non-target boost header
    reached from a lib's header set is scanned for further includes, and the
    walk stops at target-owned roots and system/std headers. This keeps the
    topological order (and first-wins claiming) consistent with what the
    module GMF actually compiles."""
    graph = {}
    for lib in TARGET_LIBS:
        headers = (headers_by_lib.get(lib) if headers_by_lib else None) \
            or headers_of(lib)
        graph[lib] = lib_deps(lib, headers)
    return graph


def topo_order(graph=None):
    """Dependencies first; cycles broken by TARGET_LIBS stable order."""
    if graph is None:
        graph = dep_graph()
    order, done = [], set()

    def visit(lib, stack):
        if lib in done or lib in stack:
            return
        stack.add(lib)
        for dep in sorted(graph[lib]):
            visit(dep, stack)
        stack.discard(lib)
        done.add(lib)
        order.append(lib)

    for lib in TARGET_LIBS:
        visit(lib, set())
    return order
