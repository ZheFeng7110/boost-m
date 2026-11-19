# Experiment helper for the open issue "tests/dll on Windows"
# (.agents/docs/todos/module-tests-open-issues.md). Reads the ninja graph of
# tests/dll, finds the auto-export (`coff_def`) edge of each shared library and
# reports, per DLL: the generated .def, whether the expected symbols are in it,
# and every input object whose .drectve declares exports (which makes
# `mcpp coff-def` publish nothing besides those). Not part of the package's CI.
import pathlib
import re
import sys

ROOT = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "tests/dll")
WANTED = ["path_algorithms", "error_category@system@boost@@QEBAAEBV0std", "init_stdcat"]


def ninja_lines(text):
    buf = ""
    for line in text.splitlines():
        if line.endswith("$") and not line.endswith("$$"):
            buf += line[:-1]
            continue
        yield buf + line
        buf = ""


def split_paths(s):
    out, cur, i = [], "", 0
    while i < len(s):
        c = s[i]
        if c == "$" and i + 1 < len(s):
            cur += s[i + 1]
            i += 2
            continue
        if c == " ":
            if cur:
                out.append(cur)
            cur = ""
        else:
            cur += c
        i += 1
    if cur:
        out.append(cur)
    return out


def drectve(data):
    # COFF section table: find ".drectve" and return its raw bytes.
    if len(data) < 20:
        return b""
    nsec = int.from_bytes(data[2:4], "little")
    opt = int.from_bytes(data[16:18], "little")
    if nsec == 0xFFFF:  # bigobj
        nsec = int.from_bytes(data[44:48], "little")
        base = 56
    else:
        base = 20 + opt
    out = b""
    for k in range(nsec):
        h = data[base + 40 * k: base + 40 * (k + 1)]
        if h[:8].rstrip(b"\0") == b".drectve":
            size = int.from_bytes(h[16:20], "little")
            ptr = int.from_bytes(h[20:24], "little")
            out += data[ptr:ptr + size]
    return out


for ninja in sorted(ROOT.glob("target/*/*/*.ninja")):
    build_dir = ninja.parent
    for line in ninja_lines(ninja.read_text(encoding="utf-8", errors="replace")):
        m = re.match(r"build (.+?) : coff_def (.*)$", line)
        if not m:
            continue
        outs = split_paths(m.group(1))
        ins = [p for p in split_paths(m.group(2).split(" | ")[0])]
        def_path = build_dir / outs[0]
        print(f"== {ninja.name}: {outs[0]} ({len(ins)} objects)")
        if def_path.exists():
            text = def_path.read_text(encoding="utf-8", errors="replace")
            lines = text.splitlines()
            print(f"   .def: {len(lines)} lines")
            for w in WANTED:
                hits = [l for l in lines if w in l]
                print(f"   {w}: {len(hits)}" + (f"  e.g. {hits[0].strip()}" if hits else ""))
        else:
            print("   .def: not generated")
        declaring = 0
        for obj in ins:
            p = build_dir / obj
            if not p.exists():
                continue
            d = drectve(p.read_bytes())
            exports = re.findall(rb"[/-]export:(\S+)", d, re.IGNORECASE)
            if exports:
                declaring += 1
                names = b", ".join(exports[:8]).decode(errors="replace")
                print(f"   DECLARES {len(exports)} exports: {obj}: {names}")
        print(f"   objects declaring exports: {declaring}")
