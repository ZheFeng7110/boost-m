#!/usr/bin/env python3
"""Check where documents live and what the design records declare.

    1. .agents/ holds only docs/ and skills/
    2. every file under .agents/docs/ is a record in an allowed place:
       a dated record in .agents/docs/ or .agents/docs/archive/<its year>/,
       or a work item in .agents/docs/todos/
    3. no two records share a file name (superseded_by names a file)
    4. front matter, when present, is well formed; records dated
       FRONT_MATTER_SINCE or later must declare it
    5. an archived record is not active
    6. the generated index is current
    7. relative links resolve in docs/, the top-level READMEs and every record
       that must declare front matter
    8. user documentation does not cite the design records

Records older than the front matter convention are exempt from 4 and 7: they
describe a moment, and some of the files they link to have since gone.

What this does not check: whether a landed record was edited afterwards,
and whether the documentation matches the implementation. Those need a reader.

Usage:
    python3 tools/doc-tools/check_docs.py
"""

import argparse
import sys
from pathlib import Path, PurePosixPath

import gen_index
import records as rc

ALLOWED_UNDER_AGENTS = {"docs", "skills"}


def user_docs(root):
    root = Path(root)
    out = [PurePosixPath(p.name) for p in root.glob("README*.md")]
    docs = root / "docs"
    if docs.is_dir():
        out += [PurePosixPath(p.relative_to(root).as_posix()) for p in docs.rglob("*.md")]
    return sorted(out)


def check(root=rc.ROOT):
    root = Path(root)
    errors = []

    def bad(where, msg):
        errors.append(f"{where}: {msg}")

    # 1
    agents = root / rc.AGENTS
    if agents.is_dir():
        for p in sorted(agents.iterdir()):
            if p.name not in ALLOWED_UNDER_AGENTS:
                bad(f"{rc.AGENTS}/{p.name}", "only docs/ and skills/ belong under .agents/; "
                    "plans and analyses are records in .agents/docs/")

    base = root / rc.RECORDS
    if base.is_dir():
        for p in sorted(base.rglob("*")):
            if p.is_file() and p.suffix != ".md":
                bad(p.relative_to(root).as_posix(), "only markdown belongs under .agents/docs/")

    recs = rc.collect(root)
    names = {}
    for r in recs:
        where = r.rel.as_posix()
        parts = r.rel.parts[2:]  # below .agents/docs/
        # 2
        if r.kind == "todo":
            if len(parts) != 2:
                bad(where, "todos/ holds files, not directories")
        elif not r.date:
            bad(where, "a record is named YYYY-MM-DD-<slug>.md (slug: a-z 0-9 . -)")
        elif r.kind == "archived":
            if len(parts) != 3 or parts[1] != r.date[:4]:
                bad(where, f"an archived record lives in archive/{r.date[:4] or '<YYYY>'}/")
        elif len(parts) != 1:
            bad(where, "a record lives directly in .agents/docs/ or in archive/<YYYY>/")
        # 3
        if r.kind != "todo":
            if r.name in names:
                bad(where, f"same file name as {names[r.name]}")
            names.setdefault(r.name, where)

    for r in recs:
        where = r.rel.as_posix()
        # 4
        if r.front_error:
            bad(where, r.front_error)
        if r.needs_front_matter and not r.front:
            bad(where, f"a record dated {rc.FRONT_MATTER_SINCE} or later opens with front "
                "matter declaring subject and status")
        if r.front:
            unknown = sorted(set(r.front) - {"subject", "status", "superseded_by"})
            if unknown:
                bad(where, f"unknown front matter keys: {', '.join(unknown)}")
            if not rc.SUBJECT.match(r.subject):
                bad(where, "subject is one short lowercase word (a-z 0-9 -)")
            if r.status not in rc.STATUSES:
                bad(where, f"status is one of {' | '.join(rc.STATUSES)}")
            if r.status == "superseded":
                if not r.superseded_by:
                    bad(where, "a superseded record names its successor in superseded_by")
                elif r.superseded_by not in names:
                    bad(where, f"superseded_by names no record: {r.superseded_by}")
                elif r.superseded_by == r.name:
                    bad(where, "a record cannot supersede itself")
            elif r.superseded_by:
                bad(where, "superseded_by is only for status: superseded")
        if not r.title or r.title == r.rel.stem:
            if r.needs_front_matter:
                bad(where, "a record has a '# ' title")
        # 5
        if r.kind == "archived" and r.status == "active":
            bad(where, "an active record is not archived")

    # 6
    if not gen_index.is_current(root):
        bad(rc.INDEX, "stale; run: python3 tools/doc-tools/gen_index.py")

    # 7
    linked = user_docs(root) + [r.rel for r in recs if r.needs_front_matter]
    for rel in linked:
        for line, target in rc.broken_links(root, rel):
            bad(f"{rel.as_posix()}:{line}", f"link does not resolve: {target}")

    # 8
    for rel in user_docs(root):
        text = rc.read_text(root / rel)
        for n, line in enumerate(text.splitlines(), start=1):
            if ".agents/" in line:
                bad(f"{rel.as_posix()}:{n}", "user documentation does not cite design records; "
                    "state what the reader needs here instead")

    return errors


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", type=Path, default=rc.ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    errors = check(args.root)
    for e in errors:
        print(f"FAIL: {e}")
    if errors:
        print(f"{len(errors)} problem(s)", file=sys.stderr)
        return 1
    print("documents OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
