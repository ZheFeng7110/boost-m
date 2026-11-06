#!/usr/bin/env python3
"""Nightly check that the generator cache replays what a cold run computes.

``gen_exports.py --check`` on pull requests runs with the CI cache restored, so
a cache entry that survives a change of one of its real inputs makes the check
pass on stale output. This driver produces three versions of ``generated/``:

    A  regenerated with the restored CI cache (what --check compares)
    B  regenerated from an empty cache (the ground truth)
    C  the committed files

A != B means a cache key misses an input: a generator bug. B != C means the
committed output is wrong (most likely merged on the strength of a stale
cache) and needs a regeneration and review. Either fails the run. The
evidence (diffs, both versions of the affected cache entries, generator logs)
is written to --out for upload, and the cold run leaves a clean cache in
target/gen/exports-cache for the workflow to save over the stale one.

``--summarize DIR`` turns the per-platform results under DIR into the body
of the tracking issue.

Usage:
    python3 tools/ci/check_cache_drift.py --out target/cache-drift
    python3 tools/ci/check_cache_drift.py --summarize DIR --run-url URL
"""

import argparse
import difflib
import json
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TOOLS = ROOT / "tools"
sys.path.insert(0, str(TOOLS))
sys.path.insert(0, str(Path(__file__).resolve().parent))

DIFF_LINES = 60          # per file in report.md
ISSUE_DIFF_LINES = 120   # per platform in the issue body
MARKER = "<!-- cache-drift-report -->"


def generate(label, out):
    """Run gen_exports.py into the work tree, keep a copy of its output and
    log, then restore the committed files."""
    import gen_exports
    from verify_generated import git
    log = out / "logs" / (label + ".log")
    log.parent.mkdir(parents=True, exist_ok=True)
    command = [sys.executable, str(TOOLS / "gen_exports.py")]
    print("\n==> [{}] {}".format(label, " ".join(command)), flush=True)
    last = ""
    with open(log, "w", encoding="utf-8") as f:
        proc = subprocess.Popen(command, cwd=ROOT, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, text=True,
                                encoding="utf-8", errors="replace")
        for line in proc.stdout:
            sys.stdout.write(line)
            f.write(line)
            if line.strip():
                last = line
        rc = proc.wait()
    # Facade problems are reported after every generated file is written;
    # they are verify-scripts' business. Any other failure leaves partial
    # output that must not be read as drift.
    if rc != 0 and "facade problem(s)" not in last:
        raise SystemExit("[{}] gen_exports.py failed with exit code {}"
                         .format(label, rc))
    files = gen_exports._snapshot(ROOT)
    git("checkout", "--", "generated")
    git("clean", "-fdq", "--", "generated")
    return files, rc


def diff_files(old, new):
    return sorted(p for p in set(old) | set(new) if old.get(p) != new.get(p))


def unified(rel, old, new, a, b, limit):
    lines = list(difflib.unified_diff(
        old.get(rel, b"").decode("utf-8", "replace").splitlines(),
        new.get(rel, b"").decode("utf-8", "replace").splitlines(),
        "{}/{}".format(a, rel), "{}/{}".format(b, rel), lineterm="", n=1))
    if limit and len(lines) > limit:
        lines = lines[:limit] + ["... ({} more diff lines)".format(
            len(lines) - limit)]
    return lines


def affected_libs(paths, old, new):
    """Libraries whose output differs: their .inc/.deps, or their entry in
    the API surface."""
    libs = set()
    for p in paths:
        name = p.rsplit("/", 1)[-1]
        if p.startswith("generated/exports/"):
            libs.add(name.rsplit(".", 1)[0])
        elif name == "api-surface.json":
            o = json.loads(old.get(p) or b"{}").get("libraries", {})
            n = json.loads(new.get(p) or b"{}").get("libraries", {})
            libs |= {k for k in set(o) | set(n) if o.get(k) != n.get(k)}
    return sorted(libs)


def write_diff(path, paths, old, new, a, b):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        for p in paths:
            f.write("\n".join(unified(p, old, new, a, b, 0)) + "\n")


def copy_entries(src, dst, libs):
    for lib in libs:
        for name in (lib + ".skip.json", lib + ".gate.json"):
            if (src / name).is_file():
                dst.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src / name, dst / name)


def check(out):
    # The generator modules are imported here: --summarize runs from a
    # sparse checkout without the generator's inputs.
    import ast_json as aj
    import boost_common as bc
    import gen_exports
    from verify_generated import assert_clean, run_step

    out.mkdir(parents=True, exist_ok=True)
    assert_clean("initial checkout")
    run_step("fetch_mingw_sysroot.py")
    run_step("apply_patches.py")

    cache = bc.CACHE_DIR
    restored = out / "work" / "cache-restored"
    entries = sorted(cache.glob("*.skip.json")) if cache.is_dir() else []
    print("restored cache: {} library entries".format(len(entries)))
    if cache.is_dir():
        shutil.copytree(cache, restored)

    committed = gen_exports._snapshot(ROOT)
    cached, rc_a = generate("A-cached", out)
    shutil.rmtree(cache, ignore_errors=True)
    cold, rc_b = generate("B-cold", out)

    cache_paths = diff_files(cold, cached)
    commit_paths = diff_files(committed, cold)
    status = {
        "os": "{} ({})".format(os.environ.get("RUNNER_OS") or
                               platform.system(), platform.machine()),
        "clang": aj.clang_version(),
        "clang_command": aj.clang_command(*bc.CLANG_ARGS),
        "restored_entries": len(entries),
        "cache_drift": cache_paths,
        "cache_drift_libs": affected_libs(cache_paths, cold, cached),
        "committed_drift": commit_paths,
        "committed_drift_libs": affected_libs(commit_paths, committed, cold),
        "exit_codes": {"A-cached": rc_a, "B-cold": rc_b},
    }

    report = []
    if cache_paths:
        write_diff(out / "cache-vs-cold.diff", cache_paths, cold, cached,
                   "cold", "cached")
        libs = status["cache_drift_libs"]
        copy_entries(restored, out / "evidence" / "restored", libs)
        copy_entries(cache, out / "evidence" / "cold", libs)
        report += ["## Cache drift (A != B)", "",
                   "Libraries: " + (", ".join(libs) or "-"), "", "```diff"]
        for p in cache_paths:
            report += unified(p, cold, cached, "cold", "cached", DIFF_LINES)
        report += ["```", ""]
    if commit_paths:
        write_diff(out / "committed-vs-cold.diff", commit_paths, committed,
                   cold, "committed", "cold")
        report += ["## Committed output differs from a cold run (B != C)", "",
                   "Libraries: " + (", ".join(status["committed_drift_libs"])
                                    or "-"), "", "```diff"]
        for p in commit_paths:
            report += unified(p, committed, cold, "committed", "cold",
                              DIFF_LINES)
        report += ["```", ""]
    (out / "status.json").write_text(json.dumps(status, indent=1) + "\n",
                                     encoding="utf-8")
    if report:
        (out / "report.md").write_text("\n".join(report) + "\n",
                                       encoding="utf-8")
    shutil.rmtree(out / "work", ignore_errors=True)

    gh_out = os.environ.get("GITHUB_OUTPUT")
    if gh_out:
        with open(gh_out, "a", encoding="utf-8") as f:
            f.write("cache_drift={}\n".format(
                "true" if cache_paths else "false"))
            f.write("committed_drift={}\n".format(
                "true" if commit_paths else "false"))

    print("\ncache vs cold run: {} file(s) differ".format(len(cache_paths)))
    print("committed vs cold run: {} file(s) differ".format(
        len(commit_paths)))
    if cache_paths or commit_paths:
        print("drift detected; evidence in {}".format(out), file=sys.stderr)
        return 1
    print("no drift: the cache replays exactly what a cold run computes")
    return 0


def summarize(src, run_url):
    """Issue body for every platform that drifted; empty when none did."""
    legs = []
    for f in sorted(Path(src).glob("*/status.json")):
        status = json.loads(f.read_text(encoding="utf-8"))
        if status["cache_drift"] or status["committed_drift"]:
            legs.append((f.parent, status))
    if not legs:
        return ""
    body = [MARKER, "## Generator cache drift", "",
            "Run: {}".format(run_url), "",
            "| Platform | Cache vs cold run (A != B) | "
            "Committed vs cold run (B != C) | Restored entries |",
            "|---|---|---|---|"]
    for _, s in legs:
        body.append("| {} | {} | {} | {} |".format(
            s["os"], ", ".join(s["cache_drift_libs"]) or "-",
            ", ".join(s["committed_drift_libs"]) or "-",
            s["restored_entries"]))
    body += [
        "",
        "- **A != B**: a cache key misses an input; the CI cache replayed "
        "stale output, so `gen_exports.py --check` could pass on it. Fix the "
        "generator; the run has already saved a cold cache over the stale "
        "one.",
        "- **B != C**: the committed `generated/` is not what the generator "
        "computes. Regenerate and review the diff, including the facades "
        "that import the affected modules.",
        "",
        "The run's `cache-drift-*` artifacts hold the full diffs, the "
        "restored and cold cache entries of the affected libraries, and the "
        "generator logs.",
    ]
    budget = 60000
    for leg, s in legs:
        report = leg / "report.md"
        if not report.is_file():
            continue
        lines = report.read_text(encoding="utf-8").splitlines()
        if len(lines) > ISSUE_DIFF_LINES:
            lines = lines[:ISSUE_DIFF_LINES] + ["```", "(truncated)"]
        block = ["", "<details><summary>{}: diff excerpt</summary>".format(
            s["os"]), ""] + lines + ["", "</details>"]
        text = "\n".join(block)
        if len("\n".join(body)) + len(text) > budget:
            body.append("\n(further excerpts omitted: see the artifacts)")
            break
        body.append(text)
    return "\n".join(body) + "\n"


def main():
    ap = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, default=ROOT / "target" / "cache-drift",
                    help="evidence directory (default: target/cache-drift)")
    ap.add_argument("--summarize", type=Path, metavar="DIR",
                    help="print the issue body for the results under DIR")
    ap.add_argument("--run-url", default="", help="link to the workflow run")
    args = ap.parse_args()
    if args.summarize:
        sys.stdout.write(summarize(args.summarize, args.run_url))
        return 0
    return check(args.out.resolve())


if __name__ == "__main__":
    sys.exit(main())
