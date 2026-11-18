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
# (test, doctest arguments): the three crashing programs
PROGRAMS = [
    ("asio/examples_cpp20", ["-tc=example: cpp20/operations/composed_4"]),
    ("cobalt/doc", []),
    ("math/doc_sf", ["-tc=doc: daubechies"]),
]
# minimal programs, every case run in its own process
REPROS = ["zz_repro/textual", "zz_repro/std_only", "zz_repro/with_asio", "zz_repro/future_only",
          "zz_repro/future_fixed"]


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


def headers():
    """The MSVC STL definitions behind the null ambient scheduler."""
    incs = set()
    for cc in ROOT.glob("target/*/*/compile_commands.json"):
        for e in json.loads(cc.read_text(encoding="utf-8")):
            cmd = e.get("command") or " ".join(e.get("arguments", []))
            incs.update(re.findall(r"-(?:isystem|imsvc|I)\s*\"?([^\s\"]*MSVC[^\s\"]*[/\\]include)", cmd))
    if not incs:  # clang found the installation itself
        incs = {str(p) for p in pathlib.Path(r"C:\Program Files\Microsoft Visual Studio").glob("*/*/VC/Tools/MSVC/*/include")}
    print("MSVC include dirs:", sorted(incs))
    ver = lambda d: [int(x) for x in re.findall(r"MSVC[/\\](\d+)\.(\d+)\.(\d+)", d)[0]]
    for inc in sorted(incs, key=ver)[-1:]:
        print("using", inc)
        for f in sorted(pathlib.Path(inc).iterdir()):
            if not f.is_file():
                continue
            name = f.name
            lines = f.read_text(encoding="utf-8", errors="replace").splitlines()
            for i, line in enumerate(lines):
                if re.search(r"get_ambient_scheduler|_GetStaticAmbientSchedulerRef|_Ambient_scheduler|"
                             r"alternatename", line):
                    print("== %s:%d" % (name, i + 1))
                    print("\n".join(lines[max(0, i - 3): i + 12]))


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


def run_once(prog, args, env, cwd, log, timeout=600):
    r = subprocess.run([prog, *args], env=env, cwd=cwd, capture_output=True, text=True,
                       errors="replace", timeout=timeout)
    log.write_text(r.stdout + "\n--- stderr ---\n" + r.stderr, encoding="utf-8")
    return r


def under_cdb(cdb, prog, args, env, cwd, log):
    script = log.with_suffix(".cdbscript")
    mod = pathlib.Path(prog).stem
    script.write_text("\n".join([
        ".lines -e", "sxd eh", "g", ".echo ==STOP==", ".lastevent", "r", "kn 100",
        ".echo ==SYMBOLS==",
        # decorated names: a CatchableType's name carries its copy constructor
        ".symopt- 2",
        "x %s!_CT??_R0?AV*error*" % mod, "x %s!_CT??_R0?AVexception@std@@*" % mod,
        "x %s!_CT??_R0?AV*exception*" % mod,
        "x %s!*_S_scheduler_address*" % mod, "x %s!*_GetStaticAmbientSchedulerRef*" % mod,
        ".symopt+ 2",
        ".frame 3", "ub @$scopeip L30", "u @$scopeip L4",
        ".echo ==THREADS==", "~* kn 40", ".echo ==ANALYZE==", "!analyze -v", "q",
    ]) + "\n", encoding="utf-8")
    denv = dict(env)
    denv["_NT_SYMBOL_PATH"] = "%s;srv*C:\\symcache*https://msdl.microsoft.com/download/symbols" % os.path.dirname(prog)
    d = subprocess.run([str(cdb), "-g", "-G", "-cf", str(script), prog, *args], env=denv, cwd=cwd,
                       capture_output=True, text=True, errors="replace", timeout=2400)
    out = d.stdout + d.stderr
    log.write_text(out, encoding="utf-8")
    i = out.find("==STOP==")
    j = out.find("==THREADS==")
    print("cdb exit %s; from the stop:" % code(d.returncode))
    print(out[i:j if j > i else None][:40000] if i >= 0 else out[-20000:])


def program(test):
    info = launch(test)
    prog = info["program"]
    if not prog.lower().endswith(".exe") and os.path.exists(prog + ".exe"):
        prog += ".exe"
    env = dict(os.environ)
    env.update(info.get("env") or {})
    return prog, env, info.get("cwd") or str(ROOT)


def run():
    cdb = find_cdb()
    print("cdb:", cdb)
    logs = ROOT / "target" / "repro-logs"
    logs.mkdir(parents=True, exist_ok=True)

    print("\n" + "#" * 78 + "\n# minimal programs, one process per case")
    for test in REPROS:
        prog, env, cwd = program(test)
        r = subprocess.run([prog, "-ltc"], env=env, cwd=cwd, capture_output=True, text=True, errors="replace")
        cases = [l.strip() for l in r.stdout.splitlines() if l.strip().startswith("repro:")]
        print("\n== %s (%d cases)" % (test, len(cases)), flush=True)
        for c in cases:
            tag = "%s.%s" % (test.replace("/", "_"), re.sub(r"\W+", "_", c))
            r = run_once(prog, ["-tc=" + c], env, cwd, logs / (tag + ".log"))
            page_heap(os.path.basename(prog), True)
            try:
                h = run_once(prog, ["-tc=" + c], env, cwd, logs / (tag + ".pageheap.log"))
            finally:
                page_heap(os.path.basename(prog), False)
            print("  %-45s exit %-24s page heap %s" % (c, code(r.returncode), code(h.returncode)), flush=True)
            for x in (r, h):
                if x.returncode != 0:
                    for line in (x.stdout + x.stderr).strip().splitlines()[-6:]:
                        print("      " + line)
            for line in r.stderr.splitlines():
                if line.startswith(("ambient scheduler", "orig.what()")):
                    print("      " + line)
            if (r.returncode != 0 or h.returncode != 0) and cdb:
                under_cdb(cdb, prog, ["-tc=" + c], env, cwd, logs / (tag + ".cdb.log"))

    print("\n" + "#" * 78 + "\n# the three crashing programs")
    for test, args in PROGRAMS:
        prog, env, cwd = program(test)
        print("\n" + "=" * 78 + "\n%s  %s  %s" % (test, prog, args), flush=True)
        for i in range(RUNS):
            r = run_once(prog, args, env, cwd, logs / ("%s.plain%d.log" % (test.replace("/", "_"), i + 1)))
            print("plain run %d: exit %s" % (i + 1, code(r.returncode)))
            for line in (r.stdout + r.stderr).strip().splitlines()[-3:]:
                print("    " + line)
        if cdb:
            under_cdb(cdb, prog, args, env, cwd, logs / ("%s.cdb.log" % test.replace("/", "_")))
        sys.stdout.flush()


if __name__ == "__main__":
    {"unskip": unskip, "flags": flags, "headers": headers, "run": run}[sys.argv[1]]()
