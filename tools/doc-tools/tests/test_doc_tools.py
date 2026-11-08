"""The design-record tools on a scratch repository: index, checks, moves."""
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path, PurePosixPath

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import archive_records  # noqa: E402
import check_docs  # noqa: E402
import gen_index  # noqa: E402
import records as rc  # noqa: E402

NEW = "2026-11-09"


def front(subject="docs", status="landed", extra=""):
    return f"---\nsubject: {subject}\nstatus: {status}\n{extra}---\n\n"


class Repo:
    def __init__(self):
        self.dir = tempfile.mkdtemp()
        self.root = Path(self.dir)

    def write(self, rel, text):
        p = self.root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        with open(p, "w", encoding="utf-8", newline="") as f:
            f.write(text)

    def read(self, rel):
        return rc.read_text(self.root / rel)

    def index(self):
        gen_index.main(["--root", str(self.root)])

    def errors(self):
        return check_docs.check(self.root)

    def git_init(self):
        run = lambda *a: subprocess.run(["git", *a], cwd=self.root, check=True, capture_output=True)
        run("init", "-q")
        run("add", "-A")

    def tracked(self):
        return rc.tracked_text_files(self.root)

    def close(self):
        shutil.rmtree(self.dir, ignore_errors=True)


class DocToolsTest(unittest.TestCase):
    def setUp(self):
        self.repo = Repo()
        self.addCleanup(self.repo.close)
        self.repo.write("docs/usage.md", "# Usage\n")
        self.repo.write(".agents/docs/2026-09-01-old.md", "# 旧记录\n\n[gone](../../docs/gone.md)\n")

    def assertClean(self):
        self.assertEqual(self.repo.errors(), [])

    def assertFails(self, needle):
        errors = self.repo.errors()
        self.assertTrue(any(needle in e for e in errors), errors)

    # ── index ──
    def test_index_lists_records_and_is_checked(self):
        self.repo.write(f".agents/docs/{NEW}-a-design.md", front("generator", "active") + "# A\n")
        self.assertFails("README.md: stale")
        self.repo.index()
        self.assertClean()
        idx = self.repo.read(rc.INDEX)
        self.assertIn("## 进行中", idx)
        self.assertIn("### generator", idx)
        self.assertIn("- [旧记录](2026-09-01-old.md)\n", idx)
        self.assertIn(f"- [A]({NEW}-a-design.md) — active", idx)

    def test_index_check_ignores_crlf(self):
        self.repo.index()
        p = self.repo.root / rc.INDEX
        p.write_bytes(p.read_bytes().replace(b"\n", b"\r\n"))
        self.assertTrue(gen_index.is_current(self.repo.root))

    # ── front matter ──
    def test_old_records_are_exempt(self):
        self.repo.index()
        self.assertClean()  # no front matter, broken link: both tolerated

    def test_new_record_needs_front_matter(self):
        self.repo.write(f".agents/docs/{NEW}-b.md", "# B\n")
        self.repo.index()
        self.assertFails("opens with front matter")

    def test_bad_status_and_subject(self):
        self.repo.write(f".agents/docs/{NEW}-b.md", front("Big Word", "done") + "# B\n")
        self.repo.index()
        self.assertFails("status is one of")
        self.assertFails("subject is one short")

    def test_superseded_needs_existing_successor(self):
        self.repo.write(f".agents/docs/{NEW}-b.md",
                        front(status="superseded", extra="superseded_by: 2026-12-01-x.md\n") + "# B\n")
        self.repo.index()
        self.assertFails("names no record")
        self.repo.write(".agents/docs/2026-12-01-x.md", front() + "# X\n")
        self.repo.index()
        self.assertClean()

    def test_new_record_links_must_resolve(self):
        self.repo.write(f".agents/docs/{NEW}-b.md", front() + "# B\n\n[x](../../docs/nope.md)\n")
        self.repo.index()
        self.assertFails("link does not resolve: ../../docs/nope.md")

    # ── placement ──
    def test_only_docs_and_skills_under_agents(self):
        self.repo.write(".agents/plan/2026-09-02-p.md", "# P\n")
        self.repo.index()
        self.assertFails("only docs/ and skills/")

    def test_record_name_and_archive_year(self):
        self.repo.write(".agents/docs/Notes.md", "# N\n")
        self.repo.write(".agents/docs/archive/2025/2026-01-01-y.md", "# Y\n")
        self.repo.index()
        self.assertFails("Notes.md: a record is named")
        self.assertFails("lives in archive/2026/")

    def test_user_docs_do_not_cite_records(self):
        self.repo.write("README.md", "see .agents/docs/2026-09-01-old.md\n")
        self.repo.index()
        self.assertFails("README.md:1: user documentation")

    # ── moves ──
    def test_relocate_rewrites_links_both_ways(self):
        self.repo.write(".agents/docs/2026-09-02-b.md",
                        "# B\n\n[old](2026-09-01-old.md#s) [u](../../docs/usage.md)\n"
                        "```\n[skip](2026-09-01-old.md)\n```\n")
        self.repo.write("tools/x.py", "# see .agents/docs/2026-09-01-old.md\r\n")
        self.repo.git_init()
        moves = {".agents/docs/2026-09-02-b.md": ".agents/docs/archive/2026/2026-09-02-b.md"}
        moves[".agents/docs/2026-09-01-old.md"] = ".agents/docs/archive/2026/2026-09-01-old.md"
        rc.relocate(self.repo.root, moves, self.repo.tracked())
        b = self.repo.read(".agents/docs/archive/2026/2026-09-02-b.md")
        self.assertIn("[old](2026-09-01-old.md#s)", b)
        self.assertIn("[u](../../../../docs/usage.md)", b)
        self.assertIn("[skip](2026-09-01-old.md)", b)  # fenced code untouched
        self.assertEqual(self.repo.read("tools/x.py"),
                         "# see .agents/docs/archive/2026/2026-09-01-old.md\r\n")

    def test_archive_moves_past_years_but_keeps_active(self):
        self.repo.write(f".agents/docs/{NEW}-c.md", front(status="active") + "# C\n")
        self.repo.write(".agents/docs/2026-12-30-d.md", front() + "# D\n\n[o](2026-09-01-old.md)\n")
        self.repo.index()
        self.repo.git_init()
        self.assertEqual(archive_records.main(["--root", str(self.repo.root), "--today", "2027-01-05"]), 0)
        base = self.repo.root / rc.ARCHIVE / "2026"
        self.assertEqual(sorted(p.name for p in base.iterdir()),
                         ["2026-09-01-old.md", "2026-12-30-d.md"])
        self.assertTrue((self.repo.root / rc.RECORDS / f"{NEW}-c.md").is_file())
        self.assertClean()

    def test_archive_refuses_the_current_year(self):
        with self.assertRaises(SystemExit):
            archive_records.main(["--root", str(self.repo.root), "--year", "2027", "--today", "2027-01-05"])


if __name__ == "__main__":
    unittest.main()
