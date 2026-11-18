#!/usr/bin/env python3
"""Regenerate the ported upstream tests (tests/<lib>/examples*.cpp, ...).

The official examples of the hardened libraries are ported mechanically into
doctest cases whose expected output comes from an oracle: the same ported code
built with clang++ against upstream/boost headers alone and run twice. The
committed files under tests/ are the result; this tool re-creates them, for a
Boost upgrade or after changing a port's options.

    python3 tools/port_tests/regen.py                 # every library
    python3 tools/port_tests/regen.py asio math:2     # a library, or one group of it
    python3 tools/port_tests/regen.py --list          # the libraries and their groups
    python3 tools/port_tests/regen.py --prepare       # only extract and build (see below)

Before the jobs run it prepares target/port-tests/ (common.py): libs/<lib>/
of the release archive tools/import_boost.py pins (downloaded unless
--archive or target/vendor-import/ has it), the example sources edited before
porting (sources/*.patch), and the upstream library TUs the oracle programs
link (oracle-libs/, built once). Jobs run in parallel, one log each under
target/port-tests/logs/; the summary lists every FLAG line (an oracle that
did not compile, output that differed between two runs: its output is not
compared) and every job that failed.

Needs clang++ (the oracle) and git (to apply sources/*.patch). Upgrading Boost:

    python3 tools/import_boost.py && python3 tools/apply_patches.py
    python3 tools/port_tests/regen.py
    git diff tests/                     # review: new outputs, FLAG lines
    python3 tools/ci/test_coverage.py scan --archive <tarball>
    python3 tools/ci/test_coverage.py link -v && python3 tools/ci/test_coverage.py check

A library's module (libs/<lib>.py) names the examples, the per-example
options of port_examples.py and the header comment of each generated file;
the reasons for each option are in comments there and in that header.
"""

import argparse
import concurrent.futures
import importlib.util
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import common as c  # noqa: E402

LIB_DIR = HERE / "libs"


def load(lib):
    spec = importlib.util.spec_from_file_location("port_tests_" + lib, LIB_DIR / (lib + ".py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def all_jobs():
    """[(lib, group)] of every library, in file order of libs/."""
    jobs = []
    for path in sorted(LIB_DIR.glob("*.py")):
        groups = getattr(load(path.stem), "GROUPS", [None])
        jobs += [(path.stem, g) for g in groups]
    return jobs


def job_name(lib, group):
    return lib if group is None else "%s:%s" % (lib, group)


def select(targets):
    jobs = all_jobs()
    if not targets:
        return jobs
    out = []
    for t in targets:
        lib, _, group = t.partition(":")
        hit = [(l, g) for l, g in jobs if l == lib and (not group or g == group)]
        if not hit:
            raise SystemExit("unknown target %r (see --list)" % t)
        out += [j for j in hit if j not in out]
    return out


def run_job(spec):
    """In the child process: run one job, its report on stdout."""
    lib, _, group = spec.partition(":")
    os.chdir(c.ROOT)
    return load(lib).run(group or None) or 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n", 1)[0])
    ap.add_argument("targets", nargs="*", help="LIB or LIB:GROUP (default: all)")
    ap.add_argument("--list", action="store_true", help="list the libraries and their groups")
    ap.add_argument("--prepare", action="store_true", help="only extract the release and build the oracle libraries")
    ap.add_argument("--archive", help="the release tarball (default: target/vendor-import/, else downloaded)")
    ap.add_argument("-j", "--jobs", type=int, default=min(8, os.cpu_count() or 1), help="parallel jobs (default: %(default)s)")
    ap.add_argument("--job", help=argparse.SUPPRESS)
    args = ap.parse_args()

    if args.job:
        sys.exit(run_job(args.job))
    if args.list:
        for lib, group in all_jobs():
            print(job_name(lib, group))
        return

    jobs = select(args.targets)
    c.WORK.mkdir(parents=True, exist_ok=True)
    c.extract_release(sorted({lib for lib, _ in all_jobs()}), args.archive)
    c.apply_sources()
    c.build_oracle_libs()
    if args.prepare:
        print("prepared", c.WORK)
        return

    c.LOGS.mkdir(parents=True, exist_ok=True)

    def one(job):
        name = job_name(*job)
        log = c.LOGS / (name.replace(":", "-") + ".log")
        start = time.time()
        with open(log, "w") as f:
            rc = subprocess.call([sys.executable, str(Path(__file__).resolve()), "--job", name],
                                 stdout=f, stderr=subprocess.STDOUT, cwd=c.ROOT)
        return name, rc, time.time() - start, log

    failed, flags = [], []
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        for name, rc, secs, log in pool.map(one, jobs):
            print("%-24s %s %4.0fs  %s" % (name, "ok  " if rc == 0 else "FAIL", secs, log.relative_to(c.ROOT)))
            sys.stdout.flush()
            if rc:
                failed.append(name)
            flags += ["%s: %s" % (name, l.strip()) for l in log.read_text(errors="replace").splitlines()
                      if "FLAG" in l or "Traceback" in l]
    if flags:
        print("\nreview (an oracle that failed or ran differently twice; such a case does not compare output):")
        print("\n".join("  " + f for f in flags))
    if failed:
        raise SystemExit("\nfailed: " + " ".join(failed))
    print("\ndone; review `git diff tests/`, then tools/ci/test_coverage.py link / check")


if __name__ == "__main__":
    main()
