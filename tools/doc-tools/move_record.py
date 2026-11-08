#!/usr/bin/env python3
"""Move or rename a document and rewrite the repository's references to it.

Markdown links to the file, and the file's own relative links, are recomputed
for the new location; a literal repository-relative path to it in any tracked
text file (a comment, a code span) is replaced. A path written relative to
something else in prose (`plan/x.md`, `docs/x.md`) is not recognised and is
reported by nothing, so search for the bare file name after a move.

The index is regenerated when a record is involved.

Usage:
    python3 tools/doc-tools/move_record.py SRC DST [SRC DST ...]
"""

import argparse
import sys
from pathlib import Path, PurePosixPath

import gen_index
import records as rc


def repo_rel(root, p):
    path = Path(p)
    if not path.is_absolute():
        path = Path.cwd() / path
    return PurePosixPath(path.resolve().relative_to(Path(root).resolve()).as_posix())


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("paths", nargs="+", metavar="SRC DST")
    ap.add_argument("--root", type=Path, default=rc.ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    if len(args.paths) % 2:
        ap.error("give SRC DST pairs")
    pairs = list(zip(args.paths[::2], args.paths[1::2]))
    moves = {repo_rel(args.root, s): repo_rel(args.root, d) for s, d in pairs}
    changed = rc.relocate(args.root, moves, rc.tracked_text_files(args.root))
    for old, new in moves.items():
        print(f"moved {old} -> {new}")
    for rel in changed:
        print(f"rewrote references in {rel}")
    if any(str(p).startswith(rc.RECORDS + "/") for p in list(moves) + list(moves.values())):
        gen_index.main(["--root", str(args.root)])
    return 0


if __name__ == "__main__":
    sys.exit(main())
