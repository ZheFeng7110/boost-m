"""Paths and preparation shared by the example porting tools.

Everything the tools produce besides tests/ lives under target/port-tests/:

    boost_<ver>/   libs/<lib>/ of the release archive (examples and docs,
                   which the vendored upstream/boost/ is pruned of)
    patched/       the few example sources edited before porting
                   (sources/<lib>.patch applied to copies of the release files)
    oracle-libs/   upstream library TUs compiled with clang++ for the oracle
                   programs (upstream/boost headers, BOOST_ALL_NO_LIB)
    gen/           concatenated helper headers
    logs/          one log per job of regen.py

The archive is the one tools/import_boost.py pins (its SHA-256 is checked).
"""

import concurrent.futures
import os
import subprocess
import sys
import tarfile
import tomllib
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import import_boost  # noqa: E402  (the pinned release: version, URL, SHA-256)

# The oracle builds with clang++'s default configuration (with the xlings LLVM
# package, its libc++ setup); CLANG_NO_DEFAULT_CONFIG, which the export
# generators need, would make the oracle programs fail to link, and a case
# whose oracle fails compares no output.
os.environ.pop("CLANG_NO_DEFAULT_CONFIG", None)

VERSION = import_boost.VERSION
TARBALL = import_boost.TARBALL_NAME
INC = ROOT / "upstream" / "boost"
TESTS = ROOT / "tests"
WORK = ROOT / "target" / "port-tests"
RELEASE = WORK / TARBALL.replace(".tar.gz", "")
PATCHED = WORK / "patched"
ORACLE_LIBS = WORK / "oracle-libs"
GEN = WORK / "gen"
LOGS = WORK / "logs"
HELPERS = HERE / "helpers"
SOURCES = HERE / "sources"
PORT_EXAMPLES = HERE / "port_examples.py"


def lib_dir(lib):
    """libs/<lib>/ of the release."""
    return RELEASE / "libs" / lib


def patched(rel):
    """An example source edited before porting (sources/<lib>.patch)."""
    return PATCHED / rel


def helper(name):
    return str(HELPERS / name)


def oracle_lib(name):
    """An oracle library to link by path (lib<name>.a)."""
    return str(ORACLE_LIBS / ("lib%s.a" % name))


def oracle_link(*names):
    """port_examples.py --oracle-arg options linking the oracle libraries."""
    return ["--oracle-arg=-L" + str(ORACLE_LIBS)] + ["--oracle-arg=-l" + n for n in names]


def port(args):
    """Run port_examples.py; its report goes to this process's stdout."""
    sys.stdout.flush()
    return subprocess.call([sys.executable, str(PORT_EXAMPLES), *args], cwd=ROOT)


def release_rel(path):
    """The path of a release (or patched) file relative to the release root."""
    path = Path(path).resolve()
    for base in (RELEASE, PATCHED):
        try:
            return path.relative_to(base).as_posix()
        except ValueError:
            pass
    return Path(path).as_posix()


def in_release_libs(path):
    path = Path(path).resolve()
    return any(path.is_relative_to(base / "libs") for base in (RELEASE, PATCHED))


# ---------------------------------------------------------------------------
# preparation (idempotent; each step leaves a stamp)
# ---------------------------------------------------------------------------

def _stamp_ok(stamp, value):
    return stamp.is_file() and stamp.read_text() == value


def extract_release(libs, archive=None):
    """Extract libs/<lib>/ of the pinned release archive."""
    archive = Path(archive) if archive else import_boost.DEFAULT_TARBALL
    want = sorted(set(libs))
    stamp = RELEASE / ".extracted"
    have = stamp.read_text().split() if stamp.is_file() else []
    missing = [lib for lib in want if lib not in have]
    if not missing:
        return
    if not archive.is_file():
        archive.parent.mkdir(parents=True, exist_ok=True)
        import_boost.download(import_boost.DEFAULT_URL, archive)
    got = import_boost.sha256_of(archive)
    if got != import_boost.PIN_SHA256:
        raise SystemExit("%s: SHA-256 %s, tools/import_boost.py pins %s" % (archive, got, import_boost.PIN_SHA256))
    top = RELEASE.name + "/libs/"
    prefixes = tuple(top + lib + "/" for lib in missing)
    print("extracting libs/{%s} of %s" % (",".join(missing), archive.name))
    with tarfile.open(archive, "r:gz") as tar:
        members = [m for m in tar if m.name.startswith(prefixes)]
        tar.extractall(WORK, members=members, filter="data")
    stamp.write_text(" ".join(sorted(set(have) | set(missing))))


def apply_sources():
    """Copy the release files the sources/*.patch edit and apply them."""
    patches = sorted(SOURCES.glob("*.patch"))
    value = "".join(p.name + ":" + str(p.stat().st_mtime_ns) + "\n" for p in patches)
    stamp = PATCHED / ".applied"
    if _stamp_ok(stamp, value):
        return
    for p in patches:
        for line in p.read_text().splitlines():
            if line.startswith("+++ b/"):
                rel = line[len("+++ b/"):]
                dst = PATCHED / rel
                dst.parent.mkdir(parents=True, exist_ok=True)
                dst.write_bytes((RELEASE / rel).read_bytes())
        r = subprocess.run(["git", "apply", str(p)], cwd=PATCHED, capture_output=True, text=True)
        if r.returncode:
            raise SystemExit("sources/%s does not apply to the release:\n%s" % (p.name, r.stderr))
    stamp.write_text(value)


def _cobalt_sources():
    manifest = tomllib.loads((ROOT / "mcpp.toml").read_text(encoding="utf-8"))
    out = []
    for pattern in manifest["features"]["cobalt"]["sources"]:
        if pattern.endswith(".cpp"):
            out += sorted(ROOT.glob(pattern))
    return out


def _glob(pattern):
    return sorted(INC.glob(pattern))


# Upstream library TUs the oracle programs link (clang++, upstream/boost
# headers): name -> (sources, extra flags).
ORACLE_LIBRARIES = {
    "boost_serialization": (lambda: _glob("libs/serialization/src/*.cpp"), []),
    "container_oracle": (lambda: [INC / "libs/container/src" / (s + ".cpp") for s in (
        "global_resource", "monotonic_buffer_resource", "pool_resource",
        "synchronized_pool_resource", "unsynchronized_pool_resource")], []),
    "json_oracle": (lambda: [GEN / "json_src.cpp"], []),
    # error_category -> std::error_category (port/system_extras.cpp)
    "system_extras": (lambda: [ROOT / "port" / "system_extras.cpp"], ["-I", str(ROOT / "port" / "include")]),
    "regex_oracle": (lambda: _glob("libs/regex/src/*.cpp"), []),
    "po_oracle": (lambda: _glob("libs/program_options/src/*.cpp"), []),
    "url_oracle": (lambda: _glob("libs/url/src/**/*.cpp"), ["-DBOOST_URL_SOURCE"]),
    "thread_oracle": (lambda: [INC / "libs/thread/src/pthread/thread.cpp", INC / "libs/thread/src/pthread/once.cpp",
                               INC / "libs/thread/src/future.cpp"], ["-O1", "-DBOOST_THREAD_BUILD_LIB"]),
    "cobalt_oracle": (_cobalt_sources, ["-DBOOST_COBALT_SOURCE"]),
}


def build_oracle_libs(jobs=None):
    """Compile the oracle libraries (once; delete oracle-libs/ to rebuild)."""
    GEN.mkdir(parents=True, exist_ok=True)
    (GEN / "json_src.cpp").write_text("#include <boost/json/src.hpp>\n")
    todo = [n for n in ORACLE_LIBRARIES if not Path(oracle_lib(n)).is_file()]
    if not todo:
        return
    objs = ORACLE_LIBS / "obj"
    tasks = []
    for name in todo:
        sources, flags = ORACLE_LIBRARIES[name]
        for src in sources():
            rel = src.relative_to(ROOT) if src.is_relative_to(ROOT) else Path(src.name)
            obj = objs / name / (rel.as_posix().replace("/", "_") + ".o")
            tasks.append((name, src, obj, flags))
    print("building oracle libraries: %s (%d TUs)" % (" ".join(todo), len(tasks)))
    sys.stdout.flush()

    def compile_one(task):
        _, src, obj, flags = task
        obj.parent.mkdir(parents=True, exist_ok=True)
        cmd = ["clang++", "-std=c++23", "-w", "-c", "-I", str(INC), "-DBOOST_ALL_NO_LIB", *flags, str(src), "-o", str(obj)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        return task, r

    failed = []
    with concurrent.futures.ThreadPoolExecutor(jobs or os.cpu_count()) as pool:
        for (name, src, _, _), r in pool.map(compile_one, tasks):
            if r.returncode:
                failed.append(name)
                print("  %s: %s failed\n%s" % (name, src, r.stderr[-2000:]))
    for name in todo:
        if name in failed:
            continue
        members = sorted(str(t[2]) for t in tasks if t[0] == name)
        subprocess.run(["ar", "rcs", oracle_lib(name), *members], check=True)
    if failed:
        raise SystemExit("oracle libraries failed: " + " ".join(sorted(set(failed))))


def write_helpers(name, parts):
    """Concatenate helper headers into gen/<name>; returns its path."""
    GEN.mkdir(parents=True, exist_ok=True)
    out = GEN / name
    out.write_text("".join(Path(h).read_text() for h in parts))
    return str(out)
