#!/usr/bin/env python3
"""Archive the design records of past years: .agents/docs/ -> archive/<YYYY>/.

Records are never deleted. Once a year is over, its records leave the top of
.agents/docs/ so that the directory a reader opens holds the current year.
A record whose status is `active` stays until it lands; running this again
later archives it then. References are rewritten as move_record.py does, and
the index is regenerated.

Usage:
    python3 tools/doc-tools/archive_records.py               # every past year
    python3 tools/doc-tools/archive_records.py --year 2026   # one past year
    python3 tools/doc-tools/archive_records.py --dry-run
"""

import argparse
import datetime
import sys
from pathlib import PurePosixPath, Path

import gen_index
import records as rc


def plan(root, years):
    moves, kept = {}, []
    for r in rc.collect(root):
        if r.kind != "current" or not r.date or r.date[:4] not in years:
            continue
        if r.status == "active":
            kept.append(r)
            continue
        moves[r.rel] = PurePosixPath(rc.ARCHIVE) / r.date[:4] / r.name
    return moves, kept


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--year", help="archive this year only (it must be over)")
    ap.add_argument("--dry-run", action="store_true", help="list the moves, change nothing")
    ap.add_argument("--today", help=argparse.SUPPRESS)
    ap.add_argument("--root", type=Path, default=rc.ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)

    today = args.today or datetime.date.today().isoformat()
    this_year = today[:4]
    if args.year:
        if args.year >= this_year:
            ap.error(f"{args.year} is not over; only past years are archived")
        years = {args.year}
    else:
        years = {r.date[:4] for r in rc.collect(args.root) if r.date and r.date[:4] < this_year}

    moves, kept = plan(args.root, years)
    for r in kept:
        print(f"kept (active): {r.rel}")
    if not moves:
        print("nothing to archive")
        return 0
    for old, new in moves.items():
        print(f"{'would move' if args.dry_run else 'moved'} {old} -> {new}")
    if args.dry_run:
        return 0
    changed = rc.relocate(args.root, moves, rc.tracked_text_files(args.root))
    for rel in changed:
        print(f"rewrote references in {rel}")
    return gen_index.main(["--root", str(args.root)])


if __name__ == "__main__":
    sys.exit(main())
