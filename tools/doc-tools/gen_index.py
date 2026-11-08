#!/usr/bin/env python3
"""Generate .agents/docs/README.md, the index of the design records.

The index is derived from the records alone: the date from the file name, the
title from the first `#` heading, and subject and status from front matter
when a record declares it. A record that predates the convention is listed by
date and title only; a subject guessed from its name would misfile it.

check_docs.py compares the committed index with what this produces, so the
index cannot drift from the records.

Usage:
    python3 tools/doc-tools/gen_index.py            # write
    python3 tools/doc-tools/gen_index.py --check    # exit 1 if stale
"""

import argparse
import sys
from pathlib import Path

import records as rc

HEADER = f"""\
# 设计记录

<!-- 本文件由 tools/doc-tools/gen_index.py 生成，不要手工编辑；CI 检查它与记录一致。 -->

本目录保存每次改动的理由：调研了什么、测到了什么、决定了什么、否决了什么。
一篇记录描述的是它写成的那一刻，**落地后不再修改**，因此这里的内容都不是对
现状的陈述；现状见 [docs/](../../docs/)。

## 规则

- 文件名为 `YYYY-MM-DD-<slug>.md`，slug 只用小写字母、数字、`.` 与 `-`；
  建议按性质以 `-design`、`-plan` 或 `-analysis` 结尾。
- {rc.FRONT_MATTER_SINCE} 及以后的记录必须以 front matter 开头（更早的记录不补）：

  ```yaml
  ---
  subject: generator                  # 简短、可复用的主题词
  status: active                      # {" | ".join(rc.STATUSES)}
  superseded_by: 2026-12-01-....md    # 仅 status 为 superseded 时，写文件名
  ---
  ```

- 落地后只允许修改 front matter 的 `status` / `superseded_by`，以及由脚本完成的
  路径修正。结论变了就写一篇新记录，并把旧记录标为 `superseded`。
- **不删除记录。** 往年且不处于 `active` 的记录由
  `tools/doc-tools/archive_records.py` 移入 `archive/<YYYY>/`，引用随之改写。
- `todos/` 放尚未完成的工作项，它们不是决定的记录。
- 移动或改名用 `tools/doc-tools/move_record.py`，它同时改写仓库中对该文件的引用。
"""


def row(r):
    note = f" — {r.status}" if r.status else ""
    if r.status == "superseded" and r.superseded_by:
        note += f"，由 {r.superseded_by} 取代"
    return f"- [{r.title}]({r.rel.relative_to(rc.RECORDS).as_posix()}){note}"


def render(root=rc.ROOT):
    recs = rc.collect(root)
    current = sorted((r for r in recs if r.kind == "current"),
                     key=lambda r: (r.date, r.name), reverse=True)
    archived = sorted((r for r in recs if r.kind == "archived"),
                      key=lambda r: (r.date, r.name), reverse=True)
    todos = sorted((r for r in recs if r.kind == "todo"), key=lambda r: r.name)

    out = [HEADER]
    out.append(f"共 {len(current) + len(archived)} 篇记录（其中 {len(archived)} 篇已归档），"
               f"{len(todos)} 项待办。")
    out.append("")

    active = [r for r in current + archived if r.status == "active"]
    if active:
        out += ["## 进行中", ""] + [row(r) for r in active] + [""]

    subjects = {}
    for r in current + archived:
        if r.subject:
            subjects.setdefault(r.subject, []).append(r)
    if subjects:
        out += ["## 按主题", "", "只列出声明了 subject 的记录。", ""]
        for subject in sorted(subjects):
            out += [f"### {subject}", ""] + [row(r) for r in subjects[subject]] + [""]

    if current:
        out += ["## 按日期", ""]
        month = None
        for r in current:
            if r.date[:7] != month:
                month = r.date[:7]
                if out[-1]:
                    out.append("")
                out += [f"### {month}", ""]
            out.append(row(r))
        out.append("")

    if archived:
        out += ["## 归档", ""]
        year = None
        for r in archived:
            if r.date[:4] != year:
                year = r.date[:4]
                if out[-1]:
                    out.append("")
                out += [f"### {year}", ""]
            out.append(row(r))
        out.append("")

    if todos:
        out += ["## 待办", ""] + [row(r) for r in todos] + [""]

    return "\n".join(out).rstrip("\n") + "\n"


def is_current(root=rc.ROOT):
    path = Path(root) / rc.INDEX
    if not path.is_file():
        return False
    return rc.read_text(path).replace("\r\n", "\n") == render(root)


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--check", action="store_true", help="exit 1 if the index is stale")
    ap.add_argument("--root", type=Path, default=rc.ROOT, help=argparse.SUPPRESS)
    args = ap.parse_args(argv)
    if args.check:
        if is_current(args.root):
            return 0
        print(f"{rc.INDEX} is stale; run: python3 tools/doc-tools/gen_index.py", file=sys.stderr)
        return 1
    (args.root / rc.INDEX).parent.mkdir(parents=True, exist_ok=True)
    with open(args.root / rc.INDEX, "w", encoding="utf-8", newline="\n") as f:
        f.write(render(args.root))
    print(f"wrote {rc.INDEX}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
