"""Boost.Test documentation examples (libs/test/doc/examples/*.run[-fail].cpp)
as test programs: the source verbatim under #line 1 "test.cpp", its main
renamed, and a harness main that runs it with the command line of its .output
file, captures what it prints and compares it with what the same program
printed when built from upstream/boost with clang++ / libc++ (the oracle; the
.output files of the documentation are older than the 1.92.0 sources: line
numbers and messages differ), and checks the exit status (run: 0, run-fail:
not 0). Testing times and the file(line) / file:line spelling are normalised.
The libs/test/example programs likewise (tests/test/included/example/), with
the oracle's exit status."""
import subprocess, tempfile, os
from pathlib import Path
import re

import common as c

GROUPS = ["doc", "example"]

EX = c.lib_dir("test") / "doc" / "examples"

def raw(s):
    d = "ex"
    while (")" + d + '"') in s:
        d += "x"
    return 'R"%s(%s)%s"' % (d, s, d)

def expected(name):
    f = EX / (name + ".output")
    if not f.exists():
        return None, None
    lines = f.read_text().splitlines()
    runs, cur = [], None
    for l in lines:
        if l.startswith("//[") or l.startswith("//]"):
            continue
        if l.startswith("> "):
            cur = [l[2:], []]
            runs.append(cur)
        elif cur is not None:
            cur[1].append(l)
    if not runs:
        return None, None
    cmd, out = runs[0]
    return cmd, "\n".join(out).strip("\n") + "\n"

HARNESS = r'''
#line 1 "boost-m harness"
#undef main
// boost-m harness: run the example's main with the documented command line,
// compare what it printed with the documented output.
#include <cstdlib>
#include <cstring>
namespace ex_harness {
inline std::string normalize(std::string s) {
    static const std::regex time(R"(testing time: [0-9.]+ ?[mun]?s)");
    s = std::regex_replace(s, time, "testing time: <t>");
    // the source location: file(line) or file:line by platform, and the line
    // of a macro spanning lines differs between compilers
    static const std::regex ansi("\x1b\\[[0-9;]*m");  // colour codes some programs turn on
    s = std::regex_replace(s, ansi, "");
    static const std::regex loc(R"(([A-Za-z0-9_./\-]+\.(cpp|hpp|ipp)|unknown location)(\(([0-9]+)\)|:([0-9]+)):)");
    s = std::regex_replace(s, loc, "<src>:");
    // the system's description of a memory fault (POSIX: "at address: A: no
    // mapping ..." / "invalid permissions"; Windows: "occurred at address A, while ...")
    static const std::regex fault(R"(memory access violation (?:occurred )?at address:? (0x[0-9a-fA-F]+)[,:][^\n]*)");
    s = std::regex_replace(s, fault, "memory access violation at address $1: <fault>");
    // MSVC's type names carry the class-key ("class std::runtime_error")
    static const std::regex key(R"((: |error: )(?:class|struct) )");
    s = std::regex_replace(s, key, "$1");
    // trailing blanks and blank lines are not part of what is compared
    static const std::regex trailing(R"([ \t]+\n)");
    s = std::regex_replace(s, trailing, "\n");
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ')) s.pop_back();
    while (!s.empty() && s.front() == '\n') s.erase(0, 1);
    return s;
}
}
int main() {
    std::vector<std::string> args = ARGS;
    std::vector<char*> argv;
    for (auto& a : args) argv.push_back(a.data());
    argv.push_back(nullptr);
    std::ostringstream captured;
    auto* out = std::cout.rdbuf(captured.rdbuf());
    auto* err = std::cerr.rdbuf(captured.rdbuf());
    int rc = CALL;
    std::cout.rdbuf(out);
    std::cerr.rdbuf(err);
    bool ok = RC_CHECK;
    if (!ok)
        std::printf("unexpected exit status %d\n", rc);
    if (std::getenv("EX_PRINT")) {  // the oracle run
        std::fputs(ex_harness::normalize(captured.str()).c_str(), stdout);
        return rc;
    }
#if HAS_EXPECTED
    const std::string want = ex_harness::normalize(EXPECTED), got = ex_harness::normalize(captured.str());
    if (want != got) {
        std::printf("output differs\n--- expected\n%s\n--- got\n%s\n", want.c_str(), got.c_str());
        ok = false;
    }
#endif
    return ok ? 0 : 1;
}
'''

def oracle(src, head, text, args, should_fail):
    h = HARNESS.replace("ARGS", "{" + ", ".join('"%s"' % a.replace('"', '\\"') for a in args) + "}").replace("CALL", HARNESS_CALL)
    h = h.replace("RC_CHECK", "true").replace("HAS_EXPECTED", "0").replace("EXPECTED", '""')
    with tempfile.TemporaryDirectory() as d:
        tu = Path(d) / "t.cpp"
        tu.write_text(head + "#define main ex_original_main\n#line 1 \"test.cpp\"\n" + text + "\n" + h)
        exe = Path(d) / "a.out"
        r = subprocess.run(["clang++", "-std=c++23", "-w", "-I", str(c.INC), "-I", str(src.parent), "-DBOOST_ALL_NO_LIB", str(tu), "-o", str(exe)],
                           capture_output=True, text=True)
        if r.returncode:
            return None
        outs = []
        for _ in range(2):
            p = subprocess.run([str(exe)], capture_output=True, cwd=d, env=dict(os.environ, EX_PRINT="1"), timeout=60)
            outs.append(p.stdout.decode(errors="replace"))
            rcs = getattr(oracle, "rcs", [])
            rcs.append(p.returncode)
            oracle.rcs = rcs
        if outs[0] != outs[1]:
            print("nondeterministic", src.name)
            return None
        return outs[0] + "\n"


# outputs that depend on the C++ library: std random distributions
# (dataset_example64), the program and source file names (unit_test_example_13)
NO_COMPARE = {"dataset_example64", "unit_test_example_13",
              # std::rand's sequence is the C library's
              "example83_contexts"}

# outputs that differ on Windows: sizeof(long) is 4 (example09/10 print it
# through failed checks), and the timeout decorator needs SIGALRM (decorator_11)
WIN_NO_COMPARE = {"example09", "example10", "decorator_11"}

# libs/test/example programs not ported (tests/test/coverage.toml has the
# reasons): the shared-library framework, two TUs of one module, signal and
# timeout handling with undefined behaviour
SKIP = {"external_main_example_1", "external_main_example_2", "unit_test_example_03",
        "unit_test_example_09_1", "unit_test_example_09_2", "unit_test_example_16"}

# named .run but documented with a failing outcome ("Test setup error: test
# tree is empty"): the exit status is the oracle's
RC_FROM_ORACLE = {"example13"}

# programs that write a file into the working directory (example50's log
# file): they run in the temporary directory
CWD_TMP = {"example50"}

# (The static-library pages' examples, custom_main and custom_init, would
# need the framework built with BOOST_TEST_ALTERNATIVE_INIT_API, which the
# unit-test-framework feature does not offer: they run header-only too.)

INCLUDED = [("boost/test/unit_test.hpp", "boost/test/included/unit_test.hpp"),
            ("boost/test/prg_exec_monitor.hpp", "boost/test/included/prg_exec_monitor.hpp"),
            ("boost/test/execution_monitor.hpp", "boost/test/included/execution_monitor.hpp")]


def gen(src, out_dir, example=False):
    name = src.name.split(".")[0]
    should_fail = None if example else ".run-fail." in src.name
    cmd, _ = expected(name)
    args = cmd.split() if cmd else ["./" + name]
    args[0] = name
    if not any(a.startswith("--color_output") for a in args):
        args.append("--color_output=no")  # (the documented output has no colours)
    text = src.read_text()
    if example:
        for a, b in INCLUDED:
            text = text.replace("<%s>" % a, "<%s>" % b)
        head = ("// Boost.Test example %s (libs/test/example/), verbatim below #line 1\n"
                "// \"test.cpp\" except that it includes the header-only form of what it uses\n"
                "// (boost/test/included/...). The harness at the end runs it as `%s`,\n"
                "// compares its output and exit status with the oracle's.\n" % (src.name, " ".join(args)))
        head += "#include <cstdio>\n#include <iostream>\n#include <regex>\n#include <sstream>\n#include <string>\n#include <vector>\n"
        return finish(src, out_dir, name, args, text, head, None)
    head = ("// Boost.Test documentation example %s (libs/test/doc/examples/), verbatim\n"
            "// below #line 1 \"test.cpp\" (its documented output names that file). The\n"
            "// harness at the end runs it as `%s` (the command line of %s.output),\n"
            "// compares its output with the oracle's and checks %s.\n" % (
                src.name, " ".join(args), name, "the oracle's exit status" if name in RC_FROM_ORACLE
                else "a failing exit status" if should_fail else "a zero exit status"))
    head += "#include <cstdio>\n#include <iostream>\n#include <regex>\n#include <sstream>\n#include <string>\n#include <vector>\n"
    return finish(src, out_dir, name, args, text, head, should_fail)


def call_for(text):
    if re.search(r"\bint\s+main\s*\(\s*\)", text):
        return "ex_original_main()"
    # a main(argc, argv, envp) gets no environment
    if re.search(r"\bmain\s*\([^)]*,[^)]*,[^)]*\)", text):
        return "ex_original_main(int(args.size()), argv.data(), nullptr)"
    return "ex_original_main(int(args.size()), argv.data())"


def finish(src, out_dir, name, args, text, head, should_fail):
    global HARNESS_CALL
    HARNESS_CALL = call_for(text)
    exp = oracle(src, head, text, args, should_fail)
    if exp is not None and name in NO_COMPARE:
        exp = None
    if exp is None:
        print("FLAG", name)
    h = HARNESS
    if name in CWD_TMP:
        h = h.replace("int main() {\n", "#include <filesystem>\nint main() {\n"
                      "    std::filesystem::current_path(std::filesystem::temp_directory_path());  // it writes into the cwd\n", 1)
    h = h.replace("ARGS", "{" + ", ".join('"%s"' % a.replace('"', '\\"') for a in args) + "}").replace("CALL", HARNESS_CALL)
    if name in RC_FROM_ORACLE:
        should_fail = None
    if should_fail is None:  # libs/test/example (and RC_FROM_ORACLE): the oracle's exit status
        rc = oracle.rcs[-1] if exp is not None else 0
        h = h.replace("RC_CHECK", "rc == %d" % rc)
    else:
        h = h.replace("RC_CHECK", "rc != 0" if should_fail else "rc == 0")
    h = h.replace("HAS_EXPECTED", ("!defined(_WIN32)" if name in WIN_NO_COMPARE else "1") if exp is not None else "0")
    h = h.replace("EXPECTED", raw(exp) if exp is not None else '""')
    stub = ("// With the unit-test-framework feature the framework TUs link into every\n"
            "// test program and would collide with this header-only form (see\n"
            "// tests/test/included/smoke.cpp): the program is a passing stub then.\n"
            "#ifdef MCPP_FEATURE_UNIT_TEST_FRAMEWORK\nint main() { return 0; }\n#else\n")
    (out_dir / (name + ".cpp")).write_text(head + stub + "#define main ex_original_main\n#line 1 \"test.cpp\"\n" + text +
                                           ("\n" if not text.endswith("\n") else "") + h + "#endif\n")
    return name


def run(group):
    if group == "example":
        out = c.TESTS / "test/included/example"
        sources = [s for s in sorted((c.lib_dir("test") / "example").glob("*.cpp")) if s.stem not in SKIP]
    else:
        out = c.TESTS / "test/included/doc"
        sources = sorted(EX.glob("*.run*.cpp"))
    out.mkdir(parents=True, exist_ok=True)
    for src in sources:
        gen(src, out, example=(group == "example"))
    print(len(sources), "programs")
    return 0
