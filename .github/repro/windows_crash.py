"""Experiment driver for the windows-llvm-msvc process crashes (open issue 1).

  unskip   remove the Windows-only skips of composed_4, cobalt/doc and
           daubechies_scaling, and link with debug info (a PDB for cdb)
  flags    print the effective compile/link flags of the Windows build
  run      run each crashing program plainly, then under full page heap in cdb

Only for the experiment branch: nothing here is part of the package.
"""
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
TOOLCHAIN = os.environ.get("TOOLCHAIN", "llvm@22.1.8")
FEATURES = os.environ.get("FEATURES", "asio,cobalt,math")
RUNS = int(os.environ.get("RUNS", "5"))

# (test, doctest arguments)
PROGRAMS = [
    ("asio/examples_cpp20", ["-tc=example: cpp20/operations/composed_4"]),
    ("cobalt/doc", []),
    ("math/doc_sf", ["-tc=doc: daubechies"]),
]


def sub(path, old, new, count=1):
    p = ROOT / path
    s = p.read_text(encoding="utf-8")
    n = s.count(old)
    if n < count:
        sys.exit("unskip: %r not found in %s" % (old, path))
    p.write_text(s.replace(old, new), encoding="utf-8")


def unskip():
    for v in ("cpp11", "cpp14", "cpp20"):
        p = ROOT / ("tests/asio/examples_%s.cpp" % v)
        s = p.read_text(encoding="utf-8")
        guard = "#if !defined(BOOST_ASIO_WINDOWS)  // the program uses what only exists there (see the file header)\n"
        marker = guard + "// libs/asio/example/%s/operations/composed_4.cpp\n" % v
        if marker not in s:
            sys.exit("unskip: composed_4 guard not found in " + p.name)
        p.write_text(s.replace(marker, "#if 1\n" + marker[len(guard):]), encoding="utf-8")
    sub("tests/cobalt/doc.cpp", 'TEST_SUITE_BEGIN("cobalt doc" * doctest::skip(ex_on_windows));',
        'TEST_SUITE_BEGIN("cobalt doc");')
    sub("tests/math/doc_sf.cpp", "#if !defined(_WIN32)  // segfaults on windows-llvm-msvc",
        "#if 1  // segfaults on windows-llvm-msvc")
    sub("mcpp.toml", 'ldflags = ["-lws2_32",', 'ldflags = ["-g", "-lws2_32",')
    print("unskipped composed_4 (cpp11/14/20), cobalt/doc, daubechies_scaling; ldflags += -g")


def flags():
    for ninja in sorted(ROOT.glob("target/*/*/build.ninja")):
        print("==", ninja.relative_to(ROOT))
        text = ninja.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            if re.match(r"\s*(cxx|cc|cxxflags|cflags|ldflags|ar|link\w*)\s*=", line):
                print(line[:4000])
        rules = re.findall(r"^rule .*(?:\n  .*)*", text, re.M)
        for r in rules:
            print(r[:2000])
    for cc in sorted(ROOT.glob("target/*/*/compile_commands.json")):
        print("==", cc.relative_to(ROOT))
        entries = json.loads(cc.read_text(encoding="utf-8"))
        shown = set()
        for e in entries:
            f = e.get("file", "")
            key = None
            if re.search(r"std(\.compat)?\.(ixx|cppm)$", f):
                key = "std:" + f
            elif f.endswith(("asio.cppm", "cobalt.cppm", "math.cppm")):
                key = f
            elif f.replace("\\", "/").endswith(("tests/cobalt/doc.cpp", "tests/math/doc_sf.cpp")):
                key = f
            if key and key not in shown:
                shown.add(key)
                print("--", f)
                print(e.get("command") or " ".join(e.get("arguments", [])))
        cmds = " ".join((e.get("command") or " ".join(e.get("arguments", []))) for e in entries)
        for pat in (r"-D_DEBUG\b", r"_ITERATOR_DEBUG_LEVEL=\d", r"-fms-runtime-lib=\S+", r"/M[DT]d?\b",
                    r"-D_DLL\b", r"-D_MT\b", r"-fexceptions", r"-fcxx-exceptions", r"-g\S*", r"-O\S*",
                    r"--target=\S+|-target \S+", r"-fno-aligned-allocation|-faligned-allocation",
                    r"-fsized-deallocation|-fno-sized-deallocation", r"-fms-compatibility-version=\S+"):
            found = sorted(set(re.findall(pat, cmds)))
            print("  %-40s %s" % (pat, found))


def launch(test):
    out = subprocess.run(["mcpp", "test", "--print-launch", "--toolchain", TOOLCHAIN,
                          "--features", FEATURES, test], capture_output=True, text=True, cwd=ROOT)
    m = re.search(r"\{.*\}\s*$", out.stdout, re.S)
    if out.returncode != 0 or not m:
        print(out.stdout[-4000:], out.stderr[-4000:])
        sys.exit("print-launch failed for " + test)
    doc = json.loads(m.group(0))
    lau = doc.get("launch") or doc
    # the envelope nests the program under a key; find it wherever it is
    def find(d):
        if isinstance(d, dict):
            if "program" in d:
                return d
            for v in d.values():
                r = find(v)
                if r:
                    return r
        return None
    return find(lau)


def find_cdb():
    for base in (r"C:\Program Files (x86)\Windows Kits\10\Debuggers\x64",
                 r"C:\Program Files\Windows Kits\10\Debuggers\x64"):
        p = pathlib.Path(base) / "cdb.exe"
        if p.exists():
            return p
    w = shutil.which("cdb")
    return pathlib.Path(w) if w else None


def page_heap(exe, on):
    import winreg
    key = r"SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options" + "\\" + exe
    if on:
        with winreg.CreateKeyEx(winreg.HKEY_LOCAL_MACHINE, key, 0, winreg.KEY_SET_VALUE) as k:
            winreg.SetValueEx(k, "GlobalFlag", 0, winreg.REG_DWORD, 0x02000000)
            winreg.SetValueEx(k, "PageHeapFlags", 0, winreg.REG_DWORD, 0x3)
    else:
        try:
            winreg.DeleteKeyEx(winreg.HKEY_LOCAL_MACHINE, key)
        except FileNotFoundError:
            pass


def code(rc):
    return "%d (0x%08X)" % (rc, rc & 0xFFFFFFFF)


def run():
    cdb = find_cdb()
    print("cdb:", cdb)
    logs = ROOT / "target" / "repro-logs"
    logs.mkdir(parents=True, exist_ok=True)
    for test, args in PROGRAMS:
        info = launch(test)
        prog = info["program"]
        if not prog.lower().endswith(".exe") and os.path.exists(prog + ".exe"):
            prog += ".exe"
        env = dict(os.environ)
        env.update(info.get("env") or {})
        cwd = info.get("cwd") or str(ROOT)
        exe = os.path.basename(prog)
        print("\n" + "=" * 78 + "\n%s  %s  %s" % (test, prog, args), flush=True)
        pdbs = sorted(p.name for p in pathlib.Path(prog).parent.glob("*.pdb"))
        print("pdb next to the program:", pdbs or "none")

        for i in range(RUNS):
            r = subprocess.run([prog, *args], env=env, cwd=cwd, capture_output=True, text=True,
                               errors="replace", timeout=600)
            tail = (r.stdout + r.stderr).strip().splitlines()[-6:]
            print("plain run %d: exit %s" % (i + 1, code(r.returncode)))
            for line in tail:
                print("    " + line)
            (logs / ("%s.plain%d.log" % (test.replace("/", "_"), i + 1))).write_text(
                r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")

        page_heap(exe, True)
        try:
            r = subprocess.run([prog, *args], env=env, cwd=cwd, capture_output=True, text=True,
                               errors="replace", timeout=1200)
            print("page-heap run: exit %s" % code(r.returncode))
            for line in (r.stdout + r.stderr).strip().splitlines()[-6:]:
                print("    " + line)
            if cdb:
                script = logs / ("%s.cdb.txt" % test.replace("/", "_"))
                script.write_text("\n".join([
                    ".lines -e",
                    "sxe -c \".echo ==AV==; .lastevent; r; kn 100; !analyze -v; q\" av",
                    "sxd eh",
                    "g",
                    ".echo ==STOP==",
                    ".lastevent",
                    "r",
                    "kn 100",
                    "!analyze -v",
                    "q",
                ]) + "\n", encoding="utf-8")
                denv = dict(env)
                denv["_NT_SYMBOL_PATH"] = "%s;srv*C:\\symcache*https://msdl.microsoft.com/download/symbols" % os.path.dirname(prog)
                d = subprocess.run([str(cdb), "-g", "-G", "-cf", str(script), prog, *args], env=denv, cwd=cwd,
                                   capture_output=True, text=True, errors="replace", timeout=2400)
                out = d.stdout + d.stderr
                (logs / ("%s.cdb.log" % test.replace("/", "_"))).write_text(out, encoding="utf-8")
                i = max(out.find("==AV=="), out.find("==STOP=="))
                print("cdb exit %s; debugger output from the stop:" % code(d.returncode))
                print(out[i:][:30000] if i >= 0 else out[-30000:])
        finally:
            page_heap(exe, False)
        sys.stdout.flush()


if __name__ == "__main__":
    {"unskip": unskip, "flags": flags, "run": run}[sys.argv[1]]()
