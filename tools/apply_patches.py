#!/usr/bin/env python3
"""apply_patches.py — replay the vendored adaptations under upstream/boost/.

`tools/import_boost.py` restores the pinned Boost tree byte for byte, which
removes the adaptations this package carries on it. This tool re-applies them:

    python3 tools/import_boost.py --tarball <boost_1_92_0.tar.gz>
    python3 tools/apply_patches.py

Only the vendored tree is patched. The module layer is not: the export lists
under generated/exports/ are written by tools/gen_exports.py alone (its
decisions live in tools/export_policy.toml) and src/*.cppm are hand-maintained
facades, so nothing has to be replayed over generator output.

Every `patches/<name>.patch` is a unified diff against upstream/boost/,
applied with `git apply` (or POSIX `patch -p1` where Git is unavailable) and
idempotent: a patch already contained in the tree is skipped by the reverse
check, so a checkout, a source archive and a bare worktree all end in the same
bytes. A patch that neither applies nor is already applied is an error.
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PATCH_DIR = ROOT / "patches"

# One entry per patches/<name>.patch; see patches/README.md for each reason.
PATCHES = [
    "accumulators", "algorithm", "asio", "beast", "bimap", "cobalt", "config_platform",
    "config_user", "container", "convert", "hof", "io", "json", "lambda",
    "mqtt5", "multi_array", "parameter", "range_adaptors", "numeric", "regex",
    "safe_numerics", "serialization", "system", "test", "ublas", "units",
    "url",
]

# A vendored file that is an addition, not a patch of an upstream file.
ADDED_FILES = {
    # M11: replacement for the mc.exe-generated header. Upstream builds it
    # from simple_event_log.mc at configure time (libs/log/CMakeLists.txt);
    # the vendored tree has no build-time code generation, so the constants
    # (self-consistent event IDs passed to ReportEvent) are written directly,
    # mirroring the .mc MessageId/Severity layout.
    "upstream/boost/libs/log/src/windows/simple_event_log.h": """
/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * M11: hand-written replacement for the mc.exe-generated header. Upstream
 * builds it from simple_event_log.mc at configure/build time (see
 * libs/log/CMakeLists.txt); the vendored tree has no build-time code
 * generation, so the constants (which only need to be self-consistent event
 * IDs passed to ReportEvent) are defined here directly, mirroring the .mc
 * MessageId/Severity layout.
 */

#pragma once

#define BOOST_LOG_SEVERITY_DEBUG   0x00000000L
#define BOOST_LOG_SEVERITY_INFO    0x00000001L
#define BOOST_LOG_SEVERITY_WARNING 0x00000002L
#define BOOST_LOG_SEVERITY_ERROR   0x00000003L

#define BOOST_LOG_MSG_DEBUG   ((DWORD)0x01000100L)
#define BOOST_LOG_MSG_INFO    ((DWORD)0x01000101L)
#define BOOST_LOG_MSG_WARNING ((DWORD)0x01000102L)
#define BOOST_LOG_MSG_ERROR   ((DWORD)0x01000103L)
""",
}


def _run(*cmd):
    """Run cmd in ROOT; a missing executable is a failed run (exit 127), so
    the patch -p1 fallback is reachable on hosts without Git."""
    try:
        return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                              encoding="utf-8", errors="replace")
    except FileNotFoundError as e:
        return subprocess.CompletedProcess(cmd, 127, "", str(e))


def apply_patch_file(name):
    """Apply patches/<name>.patch; skip when already applied."""
    rel = "patches/{}.patch".format(name)
    r = _run("git", "apply", "--whitespace=nowarn", rel)
    if r.returncode == 0:
        print("  patched {}".format(rel))
        return
    if _run("git", "apply", "--check", "--reverse", rel).returncode == 0:
        print("  skip   {} (already applied)".format(rel))
        return
    p = _run("patch", "-p1", "--dry-run", "--forward", "-i", rel)
    if p.returncode == 0:
        p = _run("patch", "-p1", "--forward", "-i", rel)
        if p.returncode == 0:
            print("  patched {} (patch -p1)".format(rel))
            return
    elif _run("patch", "-p1", "--dry-run", "--reverse",
              "-i", rel).returncode == 0:
        print("  skip   {} (already applied)".format(rel))
        return
    raise SystemExit(
        "{}: failed to apply\n  git: {}\n  patch: {}".format(
            rel, (r.stderr or r.stdout)[:300].strip(),
            (p.stderr or p.stdout)[:300].strip()))


def ensure_file(rel, content):
    """Create an added vendored file (no-op when already present)."""
    p = ROOT / rel
    if p.exists():
        print("  skip   {} (already present)".format(rel))
        return
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(content, encoding="utf-8", newline="\n")
    print("  created {}".format(rel))


def main():
    print("applying vendored patches (patches/ -> upstream/boost)...")
    for name in PATCHES:
        apply_patch_file(name)
    for rel, content in ADDED_FILES.items():
        ensure_file(rel, content)
    print("done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
