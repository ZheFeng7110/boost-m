#!/usr/bin/env python3
"""gen_features.py — generate the [features] block of mcpp.toml (M8).

One feature per Boost library. Each feature's `sources` = that library's
`.cppm` plus (for compiled libraries) its `libs/*/src/**` TU globs; `implies` =
the library's module import edges (read from generated/exports/<lib>.deps). The
generator also keeps `[build].sources` (the union of every feature's globs) in
sync — both are spliced between markers, so they cannot drift.

C1 (2026-09-06, usage-reclassification plan §1.2/§2): compiled include-only
libraries (boost_common.LIBS_COMPILED_INCLUDE_ONLY — log, test) keep a feature
with their library TU globs but have NO `.cppm` (no module interface, no CMI
to export). test's feature is renamed `unit_test_framework` (FEATURE_NAME_
OVERRIDE, aligned with the upstream CMake target boost_unit_test_framework);
the internal lib keys stay `log` / `test`. build.mcpp skips those features
when generating the `import boost;` aggregate (no `export import` edge exists
for a module-less feature).

Why base keeps every per-lib glob: `mcpp test` (includeDevDeps) skips the DROP
(only ADD runs), so the base `[build].sources` is what test mode compiles. If
per-lib globs lived ONLY in features, the default `mcpp test` would not build
the opt-in libraries and their tests would fail (plan §1.8 describes exactly
this feature-only shape; M8 keeps the base globs so the full test suite is
green by default, and `--features all` / per-group runs remain additive).
`mcpp build` (build mode) runs the DROP, so only active features' globs compile
— that is where the default-features selection actually takes effect. See
.agents/docs/2026-08-15-m8-mcpp-features-infra.md §1.1/§2.

Exception — FEATURE_ONLY_SOURCES: the unit_test_framework TU globs are
deliberately NOT put into the base set. In test mode the base compiles
unconditionally and every TU links into every test program (no archive
pull-on-demand), so the framework TUs and the official header-only aggregate
(<boost/test/included/**>, tests/test_included.cpp) would double-define the
framework symbols in one program (M11 §3 constraint: the two consumption
forms must never link together). Feature-only sources compile only when the
feature is active — in build AND test mode (M8 §1.1) — which makes the two
test forms mutually exclusive exactly as the plan requires: default `mcpp
test` runs the header-only form, `mcpp test --features unit_test_framework`
runs the compiled form.

Usage:
    uv run tools/gen_features.py          # splice mcpp.toml + generated/meta
    uv run tools/gen_features.py --check  # verify committed output is current
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MCPP_TOML = ROOT / "mcpp.toml"
LIBS_JSON = ROOT / "generated" / "meta" / "libs.json"
DEPS_DIR = ROOT / "generated" / "exports"
FEATURES_LST = ROOT / "generated" / "meta" / "features.lst"
PROFILES_LST = ROOT / "generated" / "meta" / "profiles.lst"
MODULES_LST = ROOT / "generated" / "meta" / "modules.lst"

# The full module-library list — derived from boost_common.TARGET_LIBS (which
# knows the tier tables). boost_common has no hard dependency on libclang at
# import time, so plain `uv run tools/gen_features.py` works. The compiled
# include-only libs (log, test — C1) are re-inserted at their former T2 slots
# so the generated feature block stays diff-stable.
sys.path.insert(0, str(ROOT / "tools"))
try:
    import boost_common as bc
    _T2_REINSERT = {"iostreams": "log", "serialization": "test"}
    LIBS = []
    for _lib in bc.TARGET_LIBS:
        LIBS.append(_lib)
        if _lib in _T2_REINSERT:
            LIBS.append(_T2_REINSERT[_lib])
    COMPILED_INCLUDE_ONLY = list(bc.LIBS_COMPILED_INCLUDE_ONLY)
except Exception:
    LIBS = [
        "optional", "variant", "variant2", "any", "core", "container_hash",
        "mp11", "static_string", "scope", "type_traits",
        "algorithm", "iterator", "range", "io", "rational", "endian",
        "tuple", "system",
        "filesystem", "regex", "thread", "chrono", "program_options",
        "stacktrace", "json", "url",
    ]
    COMPILED_INCLUDE_ONLY = []

MODULE_PREFIX = "mcpplibs.mwrap.boost"

# mwrap feature names are lowercase kebab-case; the module suffix keeps the
# upstream grouping spelling (underscores). `test` is the one internal key whose
# feature does not follow the mechanical rule: it is the upstream CMake target
# name boost_unit_test_framework, and it has no module (compiled include-only).
FEATURE_NAME_OVERRIDE = {"test": "unit-test-framework"}


def feature_name(lib):
    """[features] table key: lowercase kebab-case."""
    return FEATURE_NAME_OVERRIDE.get(lib, lib.replace("_", "-"))


def module_name(lib):
    """Full module name of a module-bearing library, else ''."""
    if lib in COMPILED_INCLUDE_ONLY:
        return ""
    return MODULE_PREFIX + "." + lib

# Default feature set = 18-lib closure (plan §3.2). The closure is computed from
# .deps below and asserted to be closed before writing; this is the candidate list.
# C5 (2026-09-09, plan .agents/plan/2026-09-09-boost-version-module-promotion.md):
# `version` joins the default candidates. Before C5, the BOOST_VERSION /
# BOOST_LIB_VERSION constexprs lived on `boost.core` (M3 re-homing); the move
# to a dedicated `boost.version` module keeps them visible to every default
# consumer without depending on `core`. `version` has no .deps edges, so the
# closure expands by exactly one.
DEFAULT_CANDIDATES = [
    "any", "algorithm", "chrono", "core", "filesystem", "io", "iterator",
    "json", "mp11", "optional", "range", "regex", "system", "thread",
    "tuple", "type_traits", "variant", "variant2", "version",
]

# Compiled libraries: feature sources = .cppm + these TU globs.
COMPILED_TU_GLOBS = {
    "filesystem":       ["upstream/boost/libs/filesystem/src/*.cpp"],
    "regex":            ["upstream/boost/libs/regex/src/*.cpp"],
    "chrono":           ["upstream/boost/libs/chrono/src/*.cpp"],
    "program_options":  ["upstream/boost/libs/program_options/src/*.cpp"],
    # stacktrace: 1.92 split this_thread_frames::collect out of basic.cpp into
    # the new dump.cpp (upstream boost_stacktrace_basic links _dump); both are
    # needed to resolve the symbol.
    "stacktrace":       ["upstream/boost/libs/stacktrace/src/basic.cpp",
                         "upstream/boost/libs/stacktrace/src/dump.cpp"],
    "json":             ["upstream/boost/libs/json/src/*.cpp"],
    "url":              ["upstream/boost/libs/url/src/**/*.cpp"],
    # thread: all-platform globs live in the feature; the target tables below
    # `!`-exclude the cross-platform ones (plan §3.1 "per-OS TU 互斥").
    "thread":           ["upstream/boost/libs/thread/src/future.cpp",
                         "upstream/boost/libs/thread/src/win32/*.cpp",
                         "upstream/boost/libs/thread/src/pthread/once.cpp",
                         "upstream/boost/libs/thread/src/pthread/thread.cpp"],
    # M9: parser pulls boost/charconv when std::from_chars is unavailable
    # (detail/numeric.hpp picks boost_charconv branch; seen on the linux-llvm
    # CI leg). boost.charconv is a module since M11 — parser now *implies* it
    # (EXTRA_IMPLIES below) instead of shipping its TUs, so the TU glob is not
    # duplicated across two features (double ADD when both are active).
    # M11 T2 compiled libs (design doc §2 — per-lib TU table with exclusions):
    # atomic: find_address_sse41.cpp excluded — upstream compiles it with
    # per-TU -msse4.1 flags only after a compiler probe; without its
    # BOOST_ATOMIC_USE_SSE41 the lock_pool takes the SSE2 path (this is the
    # upstream "probe failed" fallback, which we mirror).
    "atomic":           ["upstream/boost/libs/atomic/src/lock_pool.cpp",
                         "upstream/boost/libs/atomic/src/find_address_sse2.cpp"],
    "charconv":         ["upstream/boost/libs/charconv/src/*.cpp"],
    # cobalt: io/*.cpp enumerated to exclude ssl.cpp (OpenSSL — external, M13);
    # main.cpp defines main_promise::run_main only, not ::main.
    "cobalt":           ["upstream/boost/libs/cobalt/src/*.cpp",
                         "upstream/boost/libs/cobalt/src/detail/*.cpp",
                         "upstream/boost/libs/cobalt/src/io/steady_timer.cpp",
                         "upstream/boost/libs/cobalt/src/io/system_timer.cpp",
                         "upstream/boost/libs/cobalt/src/io/signal_set.cpp",
                         "upstream/boost/libs/cobalt/src/io/sleep.cpp",
                         "upstream/boost/libs/cobalt/src/io/read.cpp",
                         "upstream/boost/libs/cobalt/src/io/write.cpp",
                         "upstream/boost/libs/cobalt/src/io/serial_port.cpp",
                         "upstream/boost/libs/cobalt/src/io/pipe.cpp",
                         "upstream/boost/libs/cobalt/src/io/file.cpp",
                         "upstream/boost/libs/cobalt/src/io/random_access_file.cpp",
                         "upstream/boost/libs/cobalt/src/io/stream_file.cpp",
                         "upstream/boost/libs/cobalt/src/io/endpoint.cpp",
                         "upstream/boost/libs/cobalt/src/io/socket.cpp",
                         "upstream/boost/libs/cobalt/src/io/datagram_socket.cpp",
                         "upstream/boost/libs/cobalt/src/io/seq_packet_socket.cpp",
                         "upstream/boost/libs/cobalt/src/io/stream_socket.cpp",
                         "upstream/boost/libs/cobalt/src/io/resolver.cpp",
                         "upstream/boost/libs/cobalt/src/io/acceptor.cpp"],
    # container: alloc_lib.c excluded (C TU) AND dlmalloc.cpp excluded — its
    # dlmalloc_* wrappers call the boost_cont_* C API defined in alloc_lib.c,
    # so shipping either alone leaves undefined symbols. This mirrors the
    # upstream "BOOST_CONTAINER_HEADER_ONLY" degradation (no extended
    # allocators); the pmr resource TUs are self-contained.
    "container":        ["upstream/boost/libs/container/src/global_resource.cpp",
                         "upstream/boost/libs/container/src/monotonic_buffer_resource.cpp",
                         "upstream/boost/libs/container/src/pool_resource.cpp",
                         "upstream/boost/libs/container/src/synchronized_pool_resource.cpp",
                         "upstream/boost/libs/container/src/unsynchronized_pool_resource.cpp"],
    "contract":         ["upstream/boost/libs/contract/src/contract.cpp"],
    # date_time: upstream CMake compiles only greg_month.cpp — the other
    # b2-era gregorian/posix_time TUs conflict with the 1.92 headers
    # (greg_weekday.hpp defines the as_*_string methods inline unconditionally).
    "date_time":        ["upstream/boost/libs/date_time/src/gregorian/greg_month.cpp"],
    # exception is include-only on gcc 16.1 (see boost_common.py
    # LIBS_INCLUDE_ONLY_M11) — its clone TU ships no feature.
    "graph":            ["upstream/boost/libs/graph/src/*.cpp"],
    # iostreams: external-library backends (zlib/gzip/bzip2/lzma/zstd, M13
    # boundary) excluded — only the system-library-free device TUs ship.
    "iostreams":        ["upstream/boost/libs/iostreams/src/file_descriptor.cpp",
                         "upstream/boost/libs/iostreams/src/mapped_file.cpp"],
    # log: windows/posix dirs are per-OS exclusive via the target tables'
    # `!` rules (same basename symbols in ipc_reliable_message_queue.cpp /
    # object_name.cpp). dump_avx2/dump_ssse3 are `!`-excluded on BOTH targets
    # (unconditional <immintrin.h> breaks arm64; their symbols are only
    # referenced under consumer-set BOOST_LOG_USE_AVX2/SSSE3, which the fixed
    # module interface cannot honor — M4 §9 pattern). event_log_backend.cpp
    # ships: its generated windows/simple_event_log.h is vendored as a
    # hand-written stub (see that file; the .mc constants only need to be
    # self-consistent event IDs).
    "log":              ["upstream/boost/libs/log/src/*.cpp",
                         "upstream/boost/libs/log/src/setup/*.cpp",
                         "upstream/boost/libs/log/src/windows/*.cpp",
                         "upstream/boost/libs/log/src/posix/*.cpp"],
    # math: no TU — libs/math/src has only the deprecated std::tr1 wrappers
    # (upstream CMake: add_library(boost_math INTERFACE)). Header-only module.
    "nowide":           ["upstream/boost/libs/nowide/src/*.cpp"],
    # process: flat glob — every platform TU is internally guarded with
    # `#if defined(BOOST_PROCESS_V2_POSIX/WINDOWS)` (verified); all src TUs
    # are the v2 API.
    "process":          ["upstream/boost/libs/process/src/**/*.cpp"],
    "random":           ["upstream/boost/libs/random/src/random_device.cpp"],
    "serialization":    ["upstream/boost/libs/serialization/src/*.cpp"],
    # test: unit_test_main.cpp / cpp_main.cpp / test_main.cpp are EXCLUDED —
    # they are upstream's prg_exec_monitor / test_exec_monitor entry points
    # that define ::main or reference the user's test_main(); `mcpp test`
    # links every TU into each test program, so they collide there (no archive
    # pull-on-demand in that mode). Consumers own main + the unit_test_main
    # runner (the runner comes from #including impl/unit_test_main.ipp with
    # BOOST_TEST_NO_MAIN, cf. tests/test_utf.cpp); all framework services are
    # linked from the remaining TUs. C1: these TUs ship under the module-less
    # unit_test_framework feature (FEATURE_ONLY_SOURCES — kept out of the
    # base set so the included/* aggregate test can link without them).
    "test":             ["upstream/boost/libs/test/src/compiler_log_formatter.cpp",
                         "upstream/boost/libs/test/src/debug.cpp",
                         "upstream/boost/libs/test/src/decorator.cpp",
                         "upstream/boost/libs/test/src/execution_monitor.cpp",
                         "upstream/boost/libs/test/src/framework.cpp",
                         "upstream/boost/libs/test/src/junit_log_formatter.cpp",
                         "upstream/boost/libs/test/src/plain_report_formatter.cpp",
                         "upstream/boost/libs/test/src/progress_monitor.cpp",
                         "upstream/boost/libs/test/src/results_collector.cpp",
                         "upstream/boost/libs/test/src/results_reporter.cpp",
                         "upstream/boost/libs/test/src/test_framework_init_observer.cpp",
                         "upstream/boost/libs/test/src/test_tools.cpp",
                         "upstream/boost/libs/test/src/test_tree.cpp",
                         "upstream/boost/libs/test/src/unit_test_log.cpp",
                         "upstream/boost/libs/test/src/unit_test_monitor.cpp",
                         "upstream/boost/libs/test/src/unit_test_parameters.cpp",
                         "upstream/boost/libs/test/src/xml_log_formatter.cpp",
                         "upstream/boost/libs/test/src/xml_report_formatter.cpp"],
    "timer":            ["upstream/boost/libs/timer/src/*.cpp"],
    "type_erasure":     ["upstream/boost/libs/type_erasure/src/dynamic_binding.cpp"],
    "wave":             ["upstream/boost/libs/wave/src/*.cpp",
                         "upstream/boost/libs/wave/src/cpplexer/**/*.cpp"],
}

# Hand-maintained implies additions (union with the generated .deps edges).
# parser: its GMF includes boost/charconv.hpp only where std::from_chars is
# unavailable (linux-llvm CI leg) — the mingw-generated .deps lacks the edge,
# but the consumer-side link dependency is unconditional on that branch.
# C1: log / test have no module, hence no .deps file — their implies are
# hand-pinned to the module import edges their module TUs carried before the
# downgrade (generated/exports/{log,test}.deps, deleted). The library TUs still
# need these features' TU globs at link time (e.g. log → filesystem/thread/
# chrono), so `--features log` / `--features unit_test_framework` must pull
# them in build mode.
EXTRA_IMPLIES = {
    "parser": ["charconv"],
    # C5 moved the version constants out of the module layer into the macro
    # mapping module; `version` is the thin entry that re-exports `.macros`.
    "version": ["macros"],
    "log": ["assert", "atomic", "config", "core", "date_time", "filesystem",
            "integer", "intrusive", "io", "iterator", "move", "mp11",
            "numeric", "optional", "property_tree", "range", "regex",
            "smart_ptr", "system", "thread", "throw_exception", "type_index",
            "type_traits", "utility", "variant", "winapi"],
    "test": ["algorithm", "assert", "config", "core", "function", "iterator",
             "smart_ptr", "type_traits", "utility"],
}

# C1: features whose TU globs are feature-ONLY — deliberately excluded from
# the base [build].sources union. In test mode the base compiles
# unconditionally and every TU links into every test program, so the
# unit_test_framework TUs and the official header-only aggregate
# (<boost/test/included/**>, consumed by tests/test_included.cpp) would
# double-define the framework symbols when both land in one program (M11 §3).
# Feature-only sources compile only when the feature is active (M8 §1.1),
# making the two test consumption forms mutually exclusive. log's TUs stay in
# the base: its consumers only #include headers, no aggregate conflict.
FEATURE_ONLY_SOURCES = ["unit_test_framework"]

# Per-lib private compile flags (feature `flags` = private per-TU, never
# propagated to consumers — plan §3.1). NOTE: thread's BOOST_THREAD_BUILD_LIB
# deliberately does NOT live here — mcpp test compiles the base [build].sources
# unconditionally regardless of feature selection, so on Windows
# win32/thread.cpp would reference tss_cleanup_implemented() without the macro
# and fail to link (cf. win32/thread.cpp:86, tss_pe.cpp gating). It lives in
# the base [build].flags instead (per-glob, feature-independent).
FEATURE_FLAGS = {
    # log: the windows/ TUs include <security.h>, whose sspi.h chain requires
    # one of SECURITY_WIN32/KERNEL/MAC — upstream CMake defines SECURITY_WIN32
    # for exactly these TUs (CMakeLists.txt:324).
    "log":    [{"glob": "upstream/boost/libs/log/src/windows/**",
                "defines": ["SECURITY_WIN32"]}],
}

# ---------------------------------------------------------------------------
# Profile features (mcpp `backend = "<axis>-<impl>"` sugar)
# ---------------------------------------------------------------------------
# Consumer-selectable build profiles for Boost feature macros whose value the
# package otherwise pins at build time (architecture.md §4.1 "特性宏一律构建期
# 固定"). Every entry is declared as a `[features.backend-*]` table so mcpp's
# `backend = "<axis>-<impl>"` dependency-spec sugar desugars 1:1 into
# `features = ["backend-<axis>-<impl>"]` (docs/05-dependencies.md §"backend").
#
# Fields (all optional but `feature`/`axis`):
#   feature  — [features] key; MUST start with `backend-`
#   axis     — mutual-exclusion axis; build.mcpp refuses >1 active feature per
#              axis (they set contradictory macros)
#   defines  — package-wide -D<macro> when active (must be uniform across every
#              TU of the package or the module ODRs)
#   sources  — extra TU globs compiled only while the feature is active
#   flags    — [{glob, defines?, cxxflags?}] private per-TU build flags
#   implies  — base library features pulled in (so default-features=false works)
#   arch     — restrict activation to one mcpp::target_arch() value; build.mcpp
#              refuses the feature elsewhere (e.g. `-mavx2` is x86-only)
#
# A profile may only add sources/defines that the base features do NOT already
# provide. Features whose macro swaps an *existing* TU (stacktrace backends) or
# changes the generated export entity set (BOOST_FILESYSTEM_VERSION,
# BOOST_THREAD_VERSION) need a full alternate export tree and are not declared
# here — see .agents/docs/2026-09-21-feature-macro-profiles.md §"Extension".
PROFILE_FEATURES = [
    # log is a compiled include-only library: its consumer #includes the log
    # headers. dump_avx2.cpp / dump_ssse3.cpp are excluded from the base build
    # on all targets (unconditional <immintrin.h>, breaks arm64), so the profile
    # is the only way to compile them. The package-side define selects the
    # dispatch table the library TUs emit; the public log headers do not read
    # BOOST_LOG_USE_*, so this consumer needs no macro of its own.
    {
        "feature": "backend-log-avx2",
        "axis": "log-dump",
        "arch": "x86_64",
        # dump.cpp references the SSSE3 dump symbols whenever EITHER
        # BOOST_LOG_USE_SSSE3 or BOOST_LOG_USE_AVX2 is set (the AVX2 runtime
        # probe falls back to SSSE3 first), so the AVX2 profile must also
        # provide the SSSE3 TU + macro.
        "defines": ["BOOST_LOG_USE_SSSE3", "BOOST_LOG_USE_AVX2"],
        "sources": ["upstream/boost/libs/log/src/dump_ssse3.cpp",
                    "upstream/boost/libs/log/src/dump_avx2.cpp"],
        "flags": [
            {"glob": "upstream/boost/libs/log/src/dump_ssse3.cpp",
             "cxxflags": ["-mssse3"]},
            {"glob": "upstream/boost/libs/log/src/dump_avx2.cpp",
             "cxxflags": ["-mavx2"]},
        ],
        "implies": ["log"],
    },
    {
        "feature": "backend-log-ssse3",
        "axis": "log-dump",
        "arch": "x86_64",
        "defines": ["BOOST_LOG_USE_SSSE3"],
        "sources": ["upstream/boost/libs/log/src/dump_ssse3.cpp"],
        "flags": [{"glob": "upstream/boost/libs/log/src/dump_ssse3.cpp",
                   "cxxflags": ["-mssse3"]}],
        "implies": ["log"],
    },
]

# Files a profile feature owns exclusively. A base feature's coarse glob would
# also match them; because a `sources` entry — including a target `!`-exclusion
# — wins over a profile's re-add (verified 2026-09-21), the base glob must be
# expanded at generation time to keep these files out unless the profile is
# active. lib -> set of repo-relative paths.
PROFILE_OWNED = {
    "log": {
        "upstream/boost/libs/log/src/dump_avx2.cpp",
        "upstream/boost/libs/log/src/dump_ssse3.cpp",
    },
}


def _expand_globs(globs, drop=()):
    """Expand glob entries to explicit repo-relative paths, dropping `drop`.
    Non-glob entries pass through unchanged."""
    drop = set(drop)
    out = []
    for s in globs:
        if any(ch in s for ch in "*?["):
            for f in sorted(ROOT.glob(s)):
                rel = f.relative_to(ROOT).as_posix()
                if rel not in drop:
                    out.append(rel)
        else:
            out.append(s)
    return out

# Library-owned extras (non-module TU needed by the module, M5/M7).
EXTRAS = {
    "system": ["port/system_extras.cpp"],
    "thread": ["port/thread_extras.cpp"],
    # M9: gcc 16.1.0 module consumers emit a partial vtable for
    # leaf::detail::exception<bad_result> whose non-virtual thunks stay
    # undefined (same pipeline bug as thread's clone_impl, cf. M7c). The
    # extern template in leaf.inc suppresses the consumer's instantiation;
    # this TU's explicit instantiation provides the complete vtable + thunks.
    "leaf": ["port/leaf_extras.cpp"],
}

# Base `[build].flags` / ldflags that no longer live in features.
BASE_LDFLAGS_UNIX = ["-pthread"]

GEN_START = "# -- <gen-features> generated by tools/gen_features.py - do not edit --"
GEN_END = "# -- </gen-features> --"


def load_libs_json():
    if LIBS_JSON.exists():
        return json.loads(LIBS_JSON.read_text(encoding="utf-8"))
    return None


def deps_of(lib):
    """Module import edges for a lib: {home lib} from generated/exports/<lib>.deps.

    The files record complete module names (`mcpplibs.mwrap.boost.<lib>`, one
    export owner per entity); the internal lib key is the last segment."""
    p = DEPS_DIR / (lib + ".deps")
    if not p.exists():
        return set()
    prefix = MODULE_PREFIX + "."
    return {line.strip()[len(prefix):]
            for line in p.read_text(encoding="utf-8").splitlines()
            if line.strip().startswith(prefix)}


def module_imports(lib):
    """Module import edges written in src/<lib>.cppm.

    An interface that names another module requires that module's feature at
    build time, so the edges `export import` states are authoritative; the
    generated .deps hints add body-only references that no import line shows.
    Keeping both is what makes `default-features = false, features = ["x"]`
    build: without the import edges, x's own imports would be missing."""
    p = ROOT / "src" / (lib + ".cppm")
    if not p.exists():
        return set()
    prefix = "mcpplibs.mwrap.boost."
    out = set()
    for line in p.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not (line.startswith("export import ") or line.startswith("import ")):
            continue
        name = line.split(None, -1)[-1].rstrip(";")
        if name.startswith(prefix):
            out.add(name[len(prefix):])
    return out


def feature_sources(lib):
    """Feature sources for a lib. Compiled include-only libs (log, test — C1)
    ship their library TU globs but no `.cppm`: the module interface (and the
    CMI it would produce) is gone. SPECIAL libs (C5: e.g. `version`) ship a
    `.cppm` and no TU globs — mirror image of C1, treated the same way as a
    regular header-only module here.

    Globs are expanded to explicit files here (PROFILE_OWNED): a profile
    feature's exclusive files must not be matched by the base feature's glob,
    or the profile could never re-add them."""
    srcs = []
    if lib not in COMPILED_INCLUDE_ONLY:
        srcs.append("src/{}.cppm".format(lib))
    srcs.extend(COMPILED_TU_GLOBS.get(lib, ()))
    srcs.extend(EXTRAS.get(lib, ()))
    owned = PROFILE_OWNED.get(lib)
    if owned:
        return _expand_globs(srcs, owned)
    return srcs


# The package's interface root ([lib].path) is feature-independent: it is the
# module build.mcpp completes with the active closure partition. It belongs to
# the base sources, never to a feature (a feature would drop it, and the root
# module is what every umbrella import needs).
ROOT_SOURCES = ["src/boost.cppm"]


def all_sources():
    """Union of every feature's sources = [build].sources, minus the
    FEATURE_ONLY_SOURCES exclusions (C1: unit_test_framework's framework TUs
    must not compile unconditionally in test mode — see the docstring)."""
    skip = {feature_name(lib) for lib in FEATURE_ONLY_SOURCES}
    seen, out = set(ROOT_SOURCES), list(ROOT_SOURCES)
    for lib in LIBS:
        if feature_name(lib) in skip:
            continue
        for g in feature_sources(lib):
            if g not in seen:
                seen.add(g)
                out.append(g)
    return out


def dep_edges():
    """{lib: set(lib)} — .cppm import edges, .deps body edges, hand pins."""
    graph = {lib: (deps_of(lib) | module_imports(lib)) for lib in LIBS}
    for lib, extra in EXTRA_IMPLIES.items():
        graph.setdefault(lib, set()).update(extra)
    return graph


def default_closure(candidates):
    """Candidates + their implies closure (within the full lib set)."""
    graph = dep_edges()
    active = set(candidates)
    changed = True
    while changed:
        changed = False
        for lib in list(active):
            for dep in graph[lib]:
                if dep not in active:
                    active.add(dep)
                    changed = True
    return [lib for lib in LIBS if lib in active]


def assert_closed(closure):
    graph = dep_edges()
    missing = sorted({d for lib in closure for d in graph[lib]} - set(closure))
    if missing:
        raise SystemExit(
            "default closure not closed; missing implies: {}".format(missing))


def _flags_toml(flags):
    return "[{}]".format(", ".join(
        '{{ glob = "{}", defines = [{}] }}'.format(
            f["glob"], ", ".join('"{}"'.format(d) for d in f["defines"]))
        for f in flags))


def _toml_list(items):
    return "[{}]".format(", ".join('"{}"'.format(i) for i in items))


def _profile_table(p):
    """Render one [features.backend-*] table (PROFILE_FEATURES entry)."""
    out = ["[features.{}]".format(p["feature"])]
    if p.get("defines"):
        out.append("  defines = {}".format(_toml_list(p["defines"])))
    if p.get("sources"):
        out.append("  sources = {}".format(_toml_list(p["sources"])))
    if p.get("flags"):
        entries = []
        for f in p["flags"]:
            bits = ['glob = "{}"'.format(f["glob"])]
            if f.get("defines"):
                bits.append("defines = {}".format(_toml_list(f["defines"])))
            if f.get("cxxflags"):
                bits.append("cxxflags = {}".format(_toml_list(f["cxxflags"])))
            entries.append("{ " + ", ".join(bits) + " }")
        out.append("  flags = [{}]".format(", ".join(entries)))
    if p.get("implies"):
        out.append("  implies = {}".format(_toml_list(p["implies"])))
    return out


def render_toml_block():
    closure = default_closure(DEFAULT_CANDIDATES)
    assert_closed(closure)
    optin = [lib for lib in LIBS if lib not in closure]

    lines = [GEN_START]
    lines.append("[features]")
    lines.append("default = [{}]".format(
        ", ".join('"{}"'.format(feature_name(l)) for l in closure)))
    module_libs = [l for l in LIBS if l not in COMPILED_INCLUDE_ONLY]
    lines.append("all-modules = {{ implies = [{}] }}".format(
        ", ".join('"{}"'.format(feature_name(l)) for l in module_libs)))
    for lib in LIBS:
        lines.append("[features.{}]".format(feature_name(lib)))
        lines.append("  sources = [{}]".format(
            ", ".join('"{}"'.format(s) for s in feature_sources(lib))))
        if lib in FEATURE_FLAGS:
            lines.append("  flags = {}".format(_flags_toml(FEATURE_FLAGS[lib])))
        deps = sorted(deps_of(lib) | module_imports(lib)
                      | set(EXTRA_IMPLIES.get(lib, ())))
        if deps:
            # `implies` names FEATURES, not internal lib keys: an underscore
            # spelling would reference a feature that does not exist and mcpp
            # would silently ignore the edge.
            lines.append("  implies = [{}]".format(
                ", ".join('"{}"'.format(feature_name(d)) for d in deps)))
    for p in PROFILE_FEATURES:
        lines.extend(_profile_table(p))
    lines.append(GEN_END)
    return "\n".join(lines) + "\n"


def render_profiles_lst():
    """axis<TAB>feature<TAB>arch per active profile (build.mcpp reads it for
    conflict/arch validation). Kept beside features.lst."""
    return "\n".join(
        "{}\t{}\t{}".format(p["axis"], p["feature"], p.get("arch", ""))
        for p in PROFILE_FEATURES) + "\n"


def render_modules_lst():
    """feature<TAB>module-suffix per module-bearing library. build.mcpp reads it
    to emit the root module's closure partition; the module-less compiled
    features (log, unit-test-framework) are absent because they have no CMI."""
    return "\n".join(
        "{}\t{}".format(feature_name(l), l)
        for l in LIBS if l not in COMPILED_INCLUDE_ONLY) + "\n"


def render_sources_block():
    srcs = all_sources()
    return ("# -- <gen-sources> generated by tools/gen_features.py - do not edit --\n"
            "sources = [\n"
            + "\n".join('    "{}",'.format(s) for s in srcs)
            + "\n]\n"
            + "# -- </gen-sources> --\n")


def splice(text, start_marker, end_marker, replacement):
    start = text.find(start_marker)
    end = text.find(end_marker)
    if start == -1 or end == -1 or end <= start:
        raise SystemExit("markers not found in mcpp.toml: {} / {}".format(
            start_marker, end_marker))
    end = text.find("\n", end) + 1
    return text[:start] + replacement + text[end:]


def write_if_changed(path, content):
    """Byte-idempotent write: an unchanged file keeps its bytes AND its mtime.

    The committed generated files are build inputs; touching an unchanged
    manifest or list invalidates every downstream build for nothing."""
    if path.exists() and path.read_text(encoding="utf-8") == content:
        return False
    path.write_text(content, encoding="utf-8", newline="\n")
    return True


def write_committed():
    text = MCPP_TOML.read_text(encoding="utf-8")
    text = splice(text, "# -- <gen-sources>", "# -- </gen-sources>",
                  render_sources_block())
    text = splice(text, GEN_START, GEN_END, render_toml_block())
    write_if_changed(MCPP_TOML, text)
    # features.lst lists every FEATURE name (documentation, tests); modules.lst
    # is the feature -> module map build.mcpp consumes for the root closure.
    feature_names = [feature_name(l) for l in LIBS]
    write_if_changed(FEATURES_LST, "\n".join(feature_names) + "\n")
    write_if_changed(MODULES_LST, render_modules_lst())
    write_if_changed(PROFILES_LST, render_profiles_lst())
    closure = default_closure(DEFAULT_CANDIDATES)
    print("wrote {}{} with {} features; default={} opt-in={}".format(
        MCPP_TOML, "" , len(LIBS), ",".join(closure),
        ",".join(l for l in LIBS if l not in closure)))
    print("wrote {} ({} libs)".format(FEATURES_LST, len(LIBS)))
    print("wrote {} ({} module entries)".format(MODULES_LST, len(LIBS) - len(COMPILED_INCLUDE_ONLY)))
    print("wrote {} ({} profiles)".format(PROFILES_LST, len(PROFILE_FEATURES)))
    print("default closure: {}".format(" ".join(closure)))
    print("opt-in: {}".format(" ".join(l for l in LIBS if l not in closure)))


def check_committed():
    text = MCPP_TOML.read_text(encoding="utf-8")
    ok = True
    for name, marker_start, marker_end, expected in (
            ("sources", "# -- <gen-sources>", "# -- </gen-sources>",
             render_sources_block()),
            ("features", GEN_START, GEN_END, render_toml_block())):
        s = text.find(marker_start)
        e = text.find(marker_end)
        if s == -1 or e == -1:
            print("MISSING {} markers".format(name))
            ok = False
            continue
        e = text.find("\n", e) + 1
        actual = text[s:e]
        if actual != expected:
            print("DRIFT in {} block: run tools/gen_features.py".format(name))
            ok = False
    lst = FEATURES_LST.read_text(encoding="utf-8").splitlines() if FEATURES_LST.exists() else []
    if lst != [feature_name(l) for l in LIBS]:
        print("DRIFT in features.lst")
        ok = False
    plst = PROFILES_LST.read_text(encoding="utf-8") if PROFILES_LST.exists() else ""
    if plst != render_profiles_lst():
        print("DRIFT in profiles.lst")
        ok = False
    mlst = MODULES_LST.read_text(encoding="utf-8") if MODULES_LST.exists() else ""
    if mlst != render_modules_lst():
        print("DRIFT in modules.lst")
        ok = False
    return ok


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--check":
        sys.exit(0 if check_committed() else 1)
    write_committed()
    return 0


if __name__ == "__main__":
    main()
