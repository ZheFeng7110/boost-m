"""The design records under .agents/docs/: where they live, what they declare,
and how a move rewrites the references to them.

A record is `YYYY-MM-DD-<slug>.md`. It lives either in `.agents/docs/` or, once
its year is over and it is no longer active, in `.agents/docs/archive/<YYYY>/`.
`.agents/docs/todos/` holds work items that are not records of a decision.

Records dated FRONT_MATTER_SINCE or later must open with front matter:

    ---
    subject: generator      # a short, reused word
    status: landed          # active | landed | superseded | abandoned
    superseded_by: 2026-12-01-....md   # a file name, only when superseded
    ---

Older records are not rewritten to add it; they are indexed by date and title.
`superseded_by` names a file, not a path, so archiving the successor does not
break it.

Only the standard library is used (a third-party package would be declared in
tools/pyproject.toml), and every path is handled with pathlib and
written out with "/" separators, so the tools behave the same on Windows.
"""

import os
import re
import subprocess
from pathlib import Path, PurePosixPath
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[2]

AGENTS = ".agents"
RECORDS = ".agents/docs"
ARCHIVE = ".agents/docs/archive"
TODOS = ".agents/docs/todos"
INDEX = ".agents/docs/README.md"

# The day the front matter convention started. Records dated earlier predate it.
FRONT_MATTER_SINCE = "2026-11-08"

STATUSES = ("active", "landed", "superseded", "abandoned")
DATED = re.compile(r"^(\d{4})-(\d{2})-(\d{2})-([a-z0-9][a-z0-9.-]*)\.md$")
SUBJECT = re.compile(r"^[a-z0-9][a-z0-9-]*$")

# Markdown inline links and images: [label](target) / ![alt](target "title").
LINK = re.compile(r"(!?\[(?:[^\]\\]|\\.)*\]\()([^)\s]+)((?:\s+\"[^\"]*\")?\))")
FENCE = re.compile(r"^\s*(```|~~~)")


class Record:
    """One markdown file under .agents/docs/ other than the index."""

    def __init__(self, root, rel):
        self.root = Path(root)
        self.rel = PurePosixPath(rel)
        text = read_text(self.root / rel)
        self.front, self.front_error = parse_front_matter(text)
        self.title = title_of(text) or self.rel.stem
        m = DATED.match(self.rel.name)
        self.date = f"{m.group(1)}-{m.group(2)}-{m.group(3)}" if m else ""

    @property
    def name(self):
        return self.rel.name

    @property
    def kind(self):
        """'todo', 'archived' or 'current', from where the file lives."""
        parts = self.rel.parts
        if len(parts) > 3 and parts[2] == "todos":
            return "todo"
        if len(parts) > 3 and parts[2] == "archive":
            return "archived"
        return "current"

    @property
    def status(self):
        return self.front.get("status", "")

    @property
    def subject(self):
        return self.front.get("subject", "")

    @property
    def superseded_by(self):
        return self.front.get("superseded_by", "")

    @property
    def needs_front_matter(self):
        return self.kind != "todo" and self.date >= FRONT_MATTER_SINCE


def read_text(path):
    # newline="" keeps CRLF intact, so a rewrite changes only what it means to.
    with open(path, encoding="utf-8", newline="") as f:
        return f.read()


def write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def parse_front_matter(text):
    """Return (fields, error). A file without front matter gives ({}, "")."""
    text = text.replace("\r\n", "\n")
    if not text.startswith("---\n"):
        return {}, ""
    end = text.find("\n---\n", 3)
    if end < 0 and text.endswith("\n---"):
        end = len(text) - 4
    if end < 0:
        return {}, "front matter is not closed by a '---' line"
    fields = {}
    for n, line in enumerate(text[4:end].split("\n"), start=2):
        bare = re.sub(r"\s+#.*$", "", line).strip()
        if not bare:
            continue
        if ":" not in bare:
            return fields, f"front matter line {n} is not 'key: value'"
        key, value = bare.split(":", 1)
        fields[key.strip()] = value.strip()
    return fields, ""


def title_of(text):
    in_front = text.startswith("---")
    in_fence = False
    for i, line in enumerate(text.replace("\r\n", "\n").split("\n")):
        if in_front:
            if i > 0 and line.strip() == "---":
                in_front = False
            continue
        if FENCE.match(line):
            in_fence = not in_fence
        elif not in_fence and line.startswith("# "):
            return line[2:].strip()
    return ""


def record_paths(root):
    """Every markdown file under .agents/docs/ except the index, sorted."""
    base = Path(root) / RECORDS
    if not base.is_dir():
        return []
    out = []
    for p in base.rglob("*.md"):
        rel = PurePosixPath(p.relative_to(root).as_posix())
        if str(rel) != INDEX:
            out.append(rel)
    return sorted(out)


def collect(root=ROOT):
    return [Record(root, rel) for rel in record_paths(root)]


# ── links ─────────────────────────────────────────────────────────────────


def iter_links(text):
    """Yield (match, line_number) for each inline link outside fenced code."""
    in_fence = False
    offset = 0
    for n, line in enumerate(text.split("\n"), start=1):
        if FENCE.match(line):
            in_fence = not in_fence
        elif not in_fence:
            for m in LINK.finditer(line):
                yield m, n, offset
        offset += len(line) + 1


def is_local(target):
    return not (target.startswith(("#", "/", "<")) or re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target))


def split_target(target):
    path, sep, anchor = target.partition("#")
    return path, sep + anchor


def resolve(source_rel, target_path):
    """The repository-relative POSIX path a relative link points at."""
    joined = os.path.normpath(os.path.join(str(PurePosixPath(source_rel).parent), unquote(target_path)))
    return PurePosixPath(joined.replace(os.sep, "/"))


def relative(from_rel, to_rel):
    rel = os.path.relpath(str(to_rel), str(PurePosixPath(from_rel).parent) or ".")
    return rel.replace(os.sep, "/")


def broken_links(root, rel):
    """[(line, target)] for relative links in `rel` whose file does not exist."""
    text = read_text(Path(root) / rel).replace("\r\n", "\n")
    out = []
    for m, n, _ in iter_links(text):
        path, _ = split_target(m.group(2))
        if not path or not is_local(path):
            continue
        if not (Path(root) / resolve(rel, path)).exists():
            out.append((n, m.group(2)))
    return out


# ── moves ─────────────────────────────────────────────────────────────────


def tracked_text_files(root):
    """Tracked files that may name a record: git ls-files, minus vendored trees."""
    out = subprocess.run(["git", "ls-files", "-z"], cwd=root, check=True,
                         capture_output=True).stdout.decode("utf-8")
    keep = []
    for rel in filter(None, out.split("\0")):
        if rel.startswith(("upstream/", "generated/", "patches/")):
            continue
        if Path(rel).suffix.lower() in {".md", ".py", ".toml", ".yml", ".yaml", ".sh",
                                         ".cppm", ".cpp", ".hpp", ".json", ".txt", ".mcpp"}:
            keep.append(PurePosixPath(rel))
    return keep


def rewrite_references(text, source_old, source_new, moves):
    """Return `text` with every reference to a moved file updated.

    `source_old`/`source_new` are where the file containing `text` was and
    will be; `moves` maps old repository-relative paths to new ones. Markdown
    links are recomputed relative to the new location of both ends (only in
    markdown files). Any literal repository-relative path, such as
    `.agents/docs/<name>` in a comment or a code span, is replaced as well.
    """
    if PurePosixPath(source_old).suffix == ".md":
        pieces = []
        last = 0
        for m, _, offset in iter_links(text):
            path, anchor = split_target(m.group(2))
            if not path or not is_local(path):
                continue
            target_old = resolve(source_old, path)
            target_new = moves.get(target_old, target_old)
            if target_new == target_old and source_new == source_old:
                continue
            new_target = relative(source_new, target_new) + anchor
            if new_target == m.group(2):
                continue
            start = offset + m.start(2)
            pieces.append(text[last:start])
            pieces.append(new_target)
            last = offset + m.end(2)
        pieces.append(text[last:])
        text = "".join(pieces)
    for old, new in moves.items():
        text = text.replace(str(old), str(new))
    return text


def relocate(root, moves, files):
    """Move files and rewrite references to them in `files`.

    `moves` maps repository-relative paths (old -> new); `files` lists the
    repository-relative paths whose contents may refer to them. Returns the
    paths, after the move, of the files whose contents changed.
    """
    root = Path(root)
    moves = {PurePosixPath(k): PurePosixPath(v) for k, v in moves.items()}
    for old, new in moves.items():
        if not (root / old).is_file():
            raise FileNotFoundError(old)
        if (root / new).exists():
            raise FileExistsError(new)
    changed = []
    for rel in files:
        rel = PurePosixPath(rel)
        if not (root / rel).is_file():
            continue
        text = read_text(root / rel)
        new_text = rewrite_references(text, rel, moves.get(rel, rel), moves)
        if new_text != text:
            write_text(root / rel, new_text)
            changed.append(moves.get(rel, rel))
    for old, new in moves.items():
        (root / new).parent.mkdir(parents=True, exist_ok=True)
        if git_mv(root, old, new):
            continue
        (root / old).rename(root / new)
    return changed


def git_mv(root, old, new):
    try:
        subprocess.run(["git", "mv", str(old), str(new)], cwd=root, check=True,
                       capture_output=True)
        return True
    except (OSError, subprocess.CalledProcessError):
        return False
