#!/usr/bin/env python3
"""Official-example coverage of the hardened library tests.

Each library under test has ``tests/<lib>/coverage.toml``. It lists every
example program (``libs/<lib>/**/example*/**.cpp``) and every documentation
section that carries a C++ code block, as shipped in the Boost release
archive, and maps each one to a test case or states why it is not covered.
The vendored tree under ``upstream/boost/`` is pruned of ``example/`` and
``doc/``, so the inventory is committed and refreshed when Boost is upgraded.

    scan   read the release archive (no extraction; its SHA-256 must match
           upstream/boost/SOURCE.md) and write or refresh the inventories.
           Existing entries keep their status; new ones are ``todo``; entries
           the archive no longer has are reported and dropped.
    link   fill in `case` for the ``todo`` entries whose test case follows the
           naming convention below; prints what is still ``todo``.
    check  consistency of the committed inventories, without the archive:
           no ``todo``; a ``case`` names an existing test file that declares
           that test case; every other status carries a reason; libraries
           with customization points have tests/<lib>/custom_types.cpp.

Entry status:

    covered      case = "<test name>:<test case name>" (status may be omitted)
    excluded     the example needs what CI cannot provide (§4.5 of the design)
    gap          the module does not export what the example needs
    unsupported  the library is not part of the package (context, coroutine2)

A <test name> is the path under tests/ without ``.cpp`` (``json/examples``);
for the Boost.Test projects it is the path of the source under tests/.
A <test case name> is the doctest ``TEST_CASE("...")`` string, or the
``BOOST_AUTO_TEST_CASE(name)`` identifier in tests/test/, or ``program`` for a
test program that is an example as a whole (it defines ``main``).

Naming convention (what ``link`` matches, in any tests/<lib>/**/*.cpp):

    libs/<lib>/**/<stem>.cpp             TEST_CASE("example: <stem>")
    libs/<lib>/**/<page>.qbk#<section>   TEST_CASE("doc: <page>#<section>"), or
                                         TEST_CASE("doc: <section>") when no
                                         other page of the library has <section>
    ...#example_<stem>                   TEST_CASE("example: <stem>") (sections
                                         that present an example program)

Documentation sections are the granularity for snippets: a qbk
``[section ...]`` or an AsciiDoc / Markdown heading whose body has a code
block. Code a qbk page imports from an example file (``[import ...]``) is
covered by that example's entry. Reference, change-log and history pages are
API listings, not usage, and are skipped, as are sections titled as a
synopsis or a header (``<boost/x.hpp>``): they declare, they do not use.

Usage:
    python3 tools/ci/test_coverage.py scan --archive .tmp/boost-ref/boost_1_92_0.tar.gz
    python3 tools/ci/test_coverage.py scan --archive ... --libs json url
    python3 tools/ci/test_coverage.py check
"""

import argparse
import hashlib
import re
import sys
import tarfile
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TESTS = ROOT / "tests"
SOURCE_MD = ROOT / "upstream" / "boost" / "SOURCE.md"

# The libraries of the hardening (design §2), in the order of the design.
LIBRARIES = [
    "asio", "beast", "bloom", "cobalt", "container", "decimal", "geometry",
    "describe", "hash2", "json", "math", "mqtt5", "regex", "test",
    "unordered", "url", "uuid", "variant2", "algorithm", "any", "bimap",
    "callable_traits", "charconv", "circular_buffer", "compat",
    "container_hash", "context", "conversion", "convert", "core",
    "coroutine2", "crc", "dll", "dynamic_bitset",
]
# Not part of the package (docs/usage.md §2.5): every entry is `unsupported`.
UNSUPPORTED = {
    "context": "Boost.Context needs an assembler toolchain; the package does "
               "not declare it (docs/usage.md, unsupported libraries)",
    "coroutine2": "Boost.Coroutine2 is built on Boost.Context, which the "
                  "package does not declare (docs/usage.md)",
}
# Libraries with customization points: tests/<lib>/custom_types.cpp is
# mandatory (design §4.3). charconv has none.
CUSTOM_TYPES = set(LIBRARIES) - {"charconv", "context", "coroutine2"}
STATUSES = {"covered", "excluded", "gap", "unsupported", "todo"}

_SKIP_DOC = re.compile(
    r"(^|/)(reference|ref|changes?|changelog|release[-_]?notes|history|"
    r"copyright|acknowledg\w*|credits|news)(/|[._-]|$)", re.I)
# Sections that list declarations rather than use them.
_SKIP_TITLE = re.compile(
    r"synopsis|^header\b|<boost/|\.hpp\b|^reference\b|^(revision|change)s?\b|"
    r"copyright|acknowledg|^license|^end matter|^history|^references?$|"
    r"bibliography", re.I)


# ---------------------------------------------------------------------------
# archive scan
# ---------------------------------------------------------------------------

def recorded_sha256():
    m = re.search(r"Archive SHA-256\s*\|\s*`([0-9a-f]{64})`",
                  SOURCE_MD.read_text(encoding="utf-8"))
    if not m:
        raise SystemExit("no archive SHA-256 in " + str(SOURCE_MD))
    return m.group(1)


def recorded_version():
    m = re.search(r"Upstream version\s*\|\s*([0-9.]+)",
                  SOURCE_MD.read_text(encoding="utf-8"))
    return m.group(1) if m else "?"


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def _slug(text):
    s = re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")
    return s or "section"


def _code_in_qbk(body):
    if re.search(r"^\s*\[c\+\+\]", body, re.M):
        return True
    if re.search(r"^\s*``\s*$", body, re.M):          # block-form ``...``
        return True
    if re.search(r"^\s*```", body, re.M):               # quickbook 1.7 fence
        return True
    # preformatted: an indented line (any depth, as quickbook reads it) that
    # looks like code; list items and markup brackets are not code
    return re.search(r"^[ \t]+[^\s*#\[`].*[;{}]\s*$", body, re.M) is not None


def qbk_sections(text):
    """[(id, title)] of the sections that hold a code block, in document
    order. A section's body is its own text only: what a nested
    [section ...] ... [endsect] encloses belongs to the nested section, and
    the parent's body continues after it."""
    entries = []                    # [sid, title, has_code] in document order
    stack = []                      # open sections: (entry, [own text])
    pos = 0
    for m in re.finditer(r"^\s*\[section(?P<head>[^\n]*)|\[endsect\]",
                         text, flags=re.M):
        if stack:
            stack[-1][1].append(text[pos:m.start()])
        pos = m.end()
        if m.group("head") is None:             # [endsect]
            if stack:
                entry, pieces = stack.pop()
                entry[2] = _code_in_qbk("".join(pieces))
            continue
        head = m.group("head")
        sm = re.match(r":([\w.-]+)\s*(.*?)\]?\s*$", head)
        if sm:
            sid, title = sm.group(1), sm.group(2).strip()
        else:
            title = head.strip().rstrip("]").strip()
            sid = _slug(title)
        entry = [sid, title, False]
        entries.append(entry)
        stack.append((entry, []))
    if stack:                                   # unterminated sections
        stack[-1][1].append(text[pos:])
    while stack:
        entry, pieces = stack.pop()
        entry[2] = _code_in_qbk("".join(pieces))
    return [(sid, title) for sid, title, has_code in entries if has_code]


def md_sections(text, adoc=False):
    """AsciiDoc (`=`/`#` headings) and Markdown sections with a code block.
    `adoc` enables the AsciiDoc block delimiters; in Markdown a line of
    dashes is a setext heading underline or a thematic break, not a fence."""
    out = []
    lines = text.splitlines()
    cur, title, anchor, has_code, fence = None, None, None, False, None
    pending_anchor = None

    def flush():
        if cur is not None and has_code:
            out.append((cur, title))

    for line in lines:
        if fence:
            if line.strip() == fence:
                fence = None
            continue
        s = line.strip()
        # ``` fences, and in AsciiDoc the ---- (listing) / .... (literal)
        # blocks of four or more delimiter characters.
        if s.startswith("```") or (adoc and re.fullmatch(r"-{4,}|\.{4,}", s)):
            fence = "```" if s.startswith("```") else s
            if not s.startswith("."):
                has_code = True
            continue
        m = re.match(r"^\[#([\w.-]+)\]\s*$", s)
        if m:
            pending_anchor = m.group(1)
            continue
        m = re.match(r"^(?:={2,5}|#{1,5})\s+(\S.*)$", line)
        if m:
            flush()
            title = m.group(1).strip()
            cur = pending_anchor or _slug(title)
            pending_anchor, has_code = None, False
            continue
    flush()
    return out


def scan_archive(archive, libs):
    """{lib: {"examples": [path], "snippets": [(doc_key, title)]}}"""
    found = {lib: {"examples": [], "snippets": []} for lib in libs}
    with tarfile.open(archive, "r:gz") as tar:
        for member in tar:
            if not member.isfile():
                continue
            parts = member.name.split("/")
            if len(parts) < 4 or parts[1] != "libs" or parts[2] not in found:
                continue
            lib, rest = parts[2], parts[3:]
            rel = "/".join(parts[1:])                  # libs/<lib>/...
            if "test" in rest[:-1]:
                continue
            if (member.name.endswith(".cpp")
                    and (any(p.startswith("example") for p in rest[:-1])
                         or "example" in rest[-1])):
                found[lib]["examples"].append(rel)
                continue
            if "doc" not in rest[:-1] or not rel.endswith((".qbk", ".adoc", ".md")):
                continue
            if _SKIP_DOC.search("/".join(rest[rest.index("doc") + 1:])):
                continue
            text = tar.extractfile(member).read().decode("utf-8", "replace")
            secs = (qbk_sections(text) if rel.endswith(".qbk")
                    else md_sections(text, adoc=rel.endswith(".adoc")))
            seen = {}
            for sid, title in secs:
                if _SKIP_TITLE.search(title):
                    continue
                seen[sid] = seen.get(sid, 0) + 1
                key = sid if seen[sid] == 1 else "{}-{}".format(sid, seen[sid])
                found[lib]["snippets"].append((rel + "#" + key, title))
    for lib in found:
        found[lib]["examples"].sort()
        found[lib]["snippets"].sort()
    return found


# ---------------------------------------------------------------------------
# inventory files
# ---------------------------------------------------------------------------

def coverage_path(lib):
    return TESTS / lib / "coverage.toml"


def load(lib):
    path = coverage_path(lib)
    if not path.is_file():
        return None
    with open(path, "rb") as f:
        return tomllib.load(f)


def _q(value):
    return '"' + str(value).replace("\\", "\\\\").replace('"', '\\"') + '"'


_FIELD_ORDER = ["path", "doc", "title", "status", "case", "reason"]


def dump(lib, data):
    lines = [
        "# tests/{}/coverage.toml — official examples and documentation "
        "snippets of".format(lib),
        "# Boost.{} {} mapped to test cases (tools/ci/test_coverage.py; "
        "design".format(lib, data["upstream"]),
        "# .agents/docs/2026-11-09-module-test-hardening-design.md §4.6).",
        "# `scan` adds and drops entries; status, case and reason are "
        "maintained by hand.",
        "upstream = " + _q(data["upstream"]),
        "library = " + _q(lib),
    ]
    for table in ("example", "snippet"):
        for entry in data.get(table, []):
            lines += ["", "[[{}]]".format(table)]
            keys = [k for k in _FIELD_ORDER if k in entry]
            keys += sorted(k for k in entry if k not in _FIELD_ORDER)
            width = max(len(k) for k in keys)
            for k in keys:
                lines.append("{} = {}".format(k.ljust(width), _q(entry[k])))
    text = "\n".join(lines) + "\n"
    path = coverage_path(lib)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8", newline="\n")


def merge(lib, scanned, version):
    old = load(lib) or {}
    old_ex = {e["path"]: e for e in old.get("example", [])}
    old_sn = {e["doc"]: e for e in old.get("snippet", [])}
    default = ({"status": "unsupported", "reason": UNSUPPORTED[lib]}
               if lib in UNSUPPORTED else {"status": "todo"})
    examples, snippets = [], []
    for path in scanned["examples"]:
        examples.append(old_ex.pop(path, None) or dict(path=path, **default))
    for doc, title in scanned["snippets"]:
        entry = old_sn.pop(doc, None) or dict(doc=doc, **default)
        entry["title"] = title
        snippets.append(entry)
    for key in list(old_ex) + list(old_sn):
        print("  {}: dropped {} (not in the archive)".format(lib, key))
    dump(lib, {"upstream": version, "example": examples, "snippet": snippets})
    todo = sum(1 for e in examples + snippets if e.get("status") == "todo")
    print("{:16} examples {:4}  snippets {:4}  todo {:4}".format(
        lib, len(examples), len(snippets), todo))


def cmd_scan(args):
    archive = Path(args.archive)
    want, got = recorded_sha256(), sha256_of(archive)
    if want != got:
        raise SystemExit("archive SHA-256 {} != {} recorded in {}".format(
            got, want, SOURCE_MD.relative_to(ROOT)))
    libs = args.libs or LIBRARIES
    unknown = sorted(set(libs) - set(LIBRARIES))
    if unknown:
        raise SystemExit("not a library of the hardening: " + ", ".join(unknown))
    found = scan_archive(archive, libs)
    for lib in libs:
        merge(lib, found[lib], recorded_version())


# ---------------------------------------------------------------------------
# check
# ---------------------------------------------------------------------------

_DOCTEST_CASE = re.compile(r'\bTEST_CASE(?:_TEMPLATE|_FIXTURE)?\s*\(\s*(?:\w+\s*,\s*)?"((?:[^"\\]|\\.)*)"')
# tests/{url,json}/doc_test_suite.hpp turn the libraries' own
# TEST_SUITE(type, "name") into TEST_CASE("doc: name").
_URL_SUITE = re.compile(r'\bTEST_SUITE\(\s*\w+\s*,\s*"((?:[^"\\]|\\.)*)"')
_BOOST_CASE = re.compile(r"\bBOOST_(?:AUTO|DATA|FIXTURE)_TEST_CASE(?:_TEMPLATE)?\s*\(\s*(\w+)")
# A whole program (an example ported as one test program, tests/test/): "program".
_MAIN = re.compile(r"^\s*int\s+main\s*\(", re.M)


def test_cases(test):
    path = TESTS / (test + ".cpp")
    if not path.is_file():
        return None
    text = path.read_text(encoding="utf-8", errors="replace")
    return (set(_DOCTEST_CASE.findall(text)) | set(_BOOST_CASE.findall(text))
            | {"doc: " + n for n in _URL_SUITE.findall(text)}
            | ({"program"} if _MAIN.search(text) else set()))


def check_entry(lib, entry, key, errors, cache):
    status = entry.get("status", "covered" if "case" in entry else "todo")
    where = "tests/{}/coverage.toml: {}".format(lib, entry.get(key, "?"))
    if status not in STATUSES:
        errors.append("{}: unknown status {!r}".format(where, status))
        return status
    if status == "todo":
        errors.append("{}: todo".format(where))
        return status
    if status in ("excluded", "gap", "unsupported") and not entry.get("reason"):
        errors.append("{}: {} without a reason".format(where, status))
    if status == "covered" and "case" not in entry:
        errors.append("{}: covered without a case".format(where))
    if "case" in entry:
        test, sep, case = entry["case"].partition(":")
        if not sep or not case:
            errors.append("{}: case {!r} is not <test>:<case>".format(
                where, entry["case"]))
            return status
        if test not in cache:
            cache[test] = test_cases(test)
        if cache[test] is None:
            errors.append("{}: no test file tests/{}.cpp".format(where, test))
        elif case not in cache[test]:
            errors.append("{}: tests/{}.cpp declares no test case {!r}".format(
                where, test, case))
    return status


def _cases_under(lib):
    """{test case name: [test name]} over every source under tests/<lib>/."""
    found = {}
    base = TESTS / lib
    if not base.is_dir():
        return found
    for path in sorted(base.rglob("*.cpp")):
        test = path.relative_to(TESTS).with_suffix("").as_posix()
        for case in test_cases(test) or ():
            found.setdefault(case, []).append(test)
    return found


def cmd_link(args):
    libs = args.libs or LIBRARIES
    for lib in libs:
        data = load(lib)
        if data is None:
            continue
        cases = _cases_under(lib)
        sids = {}
        for e in data.get("snippet", []):
            sid = e["doc"].partition("#")[2]
            sids[sid] = sids.get(sid, 0) + 1
        linked, todo = 0, []
        for table, key in (("example", "path"), ("snippet", "doc")):
            for e in data.get(table, []):
                if e.get("status", "todo") != "todo" or "case" in e:
                    continue
                if table == "example":
                    stem = Path(e["path"]).stem
                    wanted = ["example: " + stem]
                else:
                    doc, _, sid = e["doc"].partition("#")
                    wanted = ["doc: {}#{}".format(Path(doc).stem, sid)]
                    if sids[sid] == 1:
                        wanted.append("doc: " + sid)
                    if sid.startswith("example_"):  # a page section per example
                        wanted.append("example: " + sid[len("example_"):])
                hit = next((w for w in wanted if w in cases), None)
                if hit is None:
                    todo.append(e[key])
                    continue
                e.pop("status", None)
                e["case"] = "{}:{}".format(cases[hit][0], hit)
                linked += 1
        if linked:
            dump(lib, data)
        print("{:16} linked {:4}  todo {:4}".format(lib, linked, len(todo)))
        if args.verbose:
            for t in todo:
                print("    todo " + t)


def cmd_check(_args):
    errors, cache, totals = [], {}, {}
    for lib in LIBRARIES:
        data = load(lib)
        if data is None:
            errors.append("tests/{}/coverage.toml is missing".format(lib))
            continue
        if data.get("library") != lib:
            errors.append("tests/{}/coverage.toml: library = {!r}".format(
                lib, data.get("library")))
        counts = {}
        for table, key in (("example", "path"), ("snippet", "doc")):
            for entry in data.get(table, []):
                st = check_entry(lib, entry, key, errors, cache)
                counts[st] = counts.get(st, 0) + 1
        totals[lib] = counts
        if lib in CUSTOM_TYPES:
            base = TESTS / lib
            hits = list(base.rglob("*custom_types*.cpp")) if base.is_dir() else []
            if not hits:
                errors.append("tests/{}/custom_types.cpp is missing (the "
                              "library has customization points)".format(lib))
    for lib, counts in totals.items():
        print("{:16} {}".format(lib, "  ".join(
            "{} {}".format(k, counts[k]) for k in sorted(counts))))
    if errors:
        print("\n{} problem(s):".format(len(errors)), file=sys.stderr)
        for e in errors:
            print("  " + e, file=sys.stderr)
        return 1
    print("coverage OK")
    return 0


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("scan", help="refresh the inventories from the archive")
    s.add_argument("--archive", required=True)
    s.add_argument("--libs", nargs="*")
    s.set_defaults(func=cmd_scan)
    k = sub.add_parser("link", help="link todo entries to conventionally named cases")
    k.add_argument("--libs", nargs="*")
    k.add_argument("-v", "--verbose", action="store_true")
    k.set_defaults(func=cmd_link)
    c = sub.add_parser("check", help="check the committed inventories")
    c.set_defaults(func=cmd_check)
    args = ap.parse_args(argv)
    return args.func(args) or 0


if __name__ == "__main__":
    sys.exit(main())
