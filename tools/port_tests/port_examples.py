#!/usr/bin/env python3
"""Port upstream example programs to doctest cases with an output oracle.

The per-library invocations are tools/port_tests/libs/*.py, run through
tools/port_tests/regen.py; this script is also usable on its own when porting
new examples (after `regen.py --prepare` has extracted the release).

For each example .cpp:
  1. the oracle: the ported code (same namespace, same names) compiled with
     clang++ against upstream/boost headers alone (plus the oracle libraries
     named by --oracle-arg) and run twice; what it prints is the expected
     output (output that differs between the two runs is reported);
  2. emit a doctest case in namespace ex_<name>: includes dropped (import
     std + the given modules instead), `main` renamed, std::cout/cerr/printf
     redirected into a std::ostringstream, and the case checks the exit code
     and that the captured output equals the oracle's.

usage: port_examples.py --lib bimap --modules bimap,... --out tests/bimap/examples.cpp
         [--keep-include REGEX] [--header TEXT] [options, see --help] FILES...
Prints a report line per example: ok / FLAG <reason>.
"""
import argparse, os, re, subprocess, sys, tempfile, hashlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import common  # noqa: E402

ROOT = common.ROOT
INC = common.INC
KEEP_STD = {"cassert", "cstdio", "cstdlib", "cmath", "cstdarg", "climits", "cfloat", "cstddef", "cerrno", "csignal"}


ORACLE_PARTS = {}
EXTRA_HELPERS = ""
ORACLE_HELPERS = r'''
#include <cstdio>
#include <fstream>
#include <vector>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) { ex_out << s << '\n'; return 0; }
'''


def oracle(src, extra_args, stdin_text=None, timeout=60, text=None, main_call=None):
    """Build and run the oracle. With `text`, the oracle is the ported code
    itself (same namespace, same names) compiled against the headers, with a
    main that runs it and prints what it captured."""
    with tempfile.TemporaryDirectory() as d:
        exe = Path(d) / "a.out"
        if text is not None:
            tu = Path(d) / "oracle.cpp"
            tu.write_text(text + "\nint main() {\n" + (
                "    ex_in.str(std::string(std::istreambuf_iterator<char>(std::cin), {}));\n") +
                "    int rc = " + main_call + ";\n    std::cout << ex_out.str();\n    return rc;\n}\n")
            src_arg = str(tu)
            if os.environ.get("ORACLE_KEEP"):  # debugging: keep the oracle TU
                keep = Path(os.environ["ORACLE_KEEP"])
                keep.mkdir(parents=True, exist_ok=True)
                (keep / (src.stem + ".cpp")).write_text(tu.read_text())
        else:
            src_arg = str(src)
        cmd = ["clang++", "-std=c++23", "-w", "-I", str(INC), "-I", str(src.parent),
               "-DBOOST_ALL_NO_LIB", src_arg, "-o", str(exe), *extra_args]
        r = subprocess.run(cmd, capture_output=True, text=True)
        if r.returncode:
            errs = [l for l in r.stderr.splitlines() if "error:" in l or "undefined reference" in l or "undefined symbol" in l]
            return None, "oracle compile failed: " + (errs[0][-220:] if errs else "?")
        outs = []
        tries = 0
        while len(outs) < 2:
            try:
                # bytes, not text mode: the output keeps its \r\n
                p = subprocess.run([str(exe)], capture_output=True, timeout=timeout,
                                   input=(stdin_text if stdin_text is not None else "").encode(), cwd=d)
            except subprocess.TimeoutExpired:
                return None, "oracle timeout"
            text_out = p.stdout.decode("utf-8", errors="replace")
            # a free port taken by another process between probe and bind: run again
            if "Address already in use" in text_out and tries < 5:
                tries += 1
                continue
            outs.append((p.returncode, text_out))
        if outs[0] != outs[1]:
            return outs[0], "nondeterministic output"
        return outs[0], None


IN_PLACE = re.compile(r'\s*#\s*include\s*[<"]boost/asio/(un)?yield\.hpp[>"]')


def strip_includes(text, keep_re, pure=False, std_includes=False):
    kept, out = [], []
    if pure:
        for line in text.splitlines():
            if re.match(r'\s*#\s*include', line):
                kept.append(line.strip())
            else:
                out.append(line)
        return kept, "\n".join(out)
    for line in text.splitlines():
        m = re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', line)
        if m and IN_PLACE.match(line):
            out.append(line.strip())  # macro on/off switches: they stay where they are
            continue
        if m:
            h = m.group(1)
            if h in KEEP_STD or (keep_re and re.search(keep_re, h)) or (std_includes and "/" not in h and "." not in h):
                kept.append("#include <%s>" % h if not h.endswith('"') else line.strip())
            continue
        out.append(line)
    return kept, "\n".join(out)


def inline_local(path, seen=None):
    """The example's own quoted headers, inlined (once each), recursively."""
    seen = set() if seen is None else seen
    out = []
    for line in path.read_text(errors="replace").replace("/*<-*/", "").replace("/*->*/", "").splitlines():
        m = re.match(r'\s*#\s*include\s*"([^"]+)"', line)
        if m:
            inc = (path.parent / m.group(1)).resolve()
            if inc.is_file() and common.in_release_libs(inc):
                if inc not in seen:
                    seen.add(inc)
                    out.append("// ---- inlined %s ----" % m.group(1))
                    g = re.search(r"^\s*#\s*ifndef\s+(\w+)\s*\n\s*#\s*define\s+\1\b", inc.read_text(errors="replace"), re.M)
                    if g:  # another case of this TU may have inlined it already
                        out.append("#undef %s" % g.group(1))
                    out.append(inline_local(inc, seen))
                    out.append("// ---- end %s ----" % m.group(1))
                continue
        out.append(line)
    text = "\n".join(out)
    return re.sub(r"^\s*#\s*pragma\s+once\s*$", "", text, flags=re.M)


def relkey(path):
    """Option key of an example: its path under example/ without .cpp
    (stems repeat across directories: main.cpp, server.cpp, ...)."""
    pp = path.as_posix()
    m = re.search(r"/examples?/(?!.*/examples?/)(.*)\.cpp$", pp)
    return m.group(1) if m else path.stem


def kmatch(k, path):
    return k == relkey(path) or k == path.stem


def lookup(d, path, default=None):
    if relkey(path) in d:
        return d[relkey(path)]
    return d.get(path.stem, default)


def port_one(path, args):
    seen = set()  # one program: each of its headers is inlined once
    src = inline_local(path, seen) if args.inline_local else path.read_text(errors="replace")
    src = src.replace("/*<-*/", "").replace("/*->*/", "")  # QuickBook "hide" markers
    for kv in args.merge:
        k, _, v = kv.partition("=")
        if kmatch(k, path):
            for other in v.split(","):
                op = path.parent / other
                src += "\n// ---- merged %s (the same program) ----\n" % other + inline_local(op, seen)
    name = re.sub(r"\W", "_", path.stem)
    if args.case_from_rel and re.search(r"/examples?/", path.as_posix()):
        name = re.sub(r"\W", "_", relkey(path))
    flags = []
    if re.search(r"\bargv\b", src) and re.search(r"argv\s*\[\s*[1-9]", src) and lookup(args.argv_map, path) is None:
        flags.append("uses argv")
    stdin_text = lookup(args.stdin_map, path)
    if re.search(r"std::cin|\bcin\s*>>|getline\s*\(\s*(std::)?cin", src) and stdin_text is None:
        flags.append("reads stdin")
    macros = sorted(set(re.findall(r"\bBOOST_[A-Z0-9_]+\b", src)) - {"BOOST_ASSERT"})
    kept, body = strip_includes(src, args.keep_include, args.pure, args.std_includes)
    for h in args.add_include:
        kept.append("#include <%s>" % h)
    # Preprocessor conditions on Boost configuration macros must see them.
    if not args.pure and "#include <cassert>" not in kept:
        kept.append("#include <cassert>")  # often reached transitively in the original
    if not args.pure and re.search(r"\bBOOST_[A-Z0-9_]+\b", body):
        if "#include <boost/config.hpp>" not in kept:
            kept.append("#include <boost/config.hpp>")
        if re.search(r"^\s*#\s*(if|elif)\b[^\n]*\bBOOST_VERSION", body, re.M):
            kept.append("#include <boost/version.hpp>")
        if re.search(r"^\s*#\s*(if|elif)\b[^\n]*\bBOOST_WORKAROUND", body, re.M):
            kept.append("#include <boost/config/workaround.hpp>")
    # strip leading license comment block
    body = re.sub(r"^\s*(//[^\n]*\n|\s*\n)+", "", body)
    body = re.sub(r"(?s)^\s*/\*.*?\*/", "", body)
    # main -> example_main
    body, n = re.subn(r"\bint\s+main\s*\(([^)]*)\)", r"int example_main(\1)", body)
    if not n:
        flags.append("no main")
    # redirect output
    body = re.sub(r"\bstd::(cout|cerr|clog)\b", "ex_out", body)
    body = re.sub(r"(?<![\w:.>])(cout|cerr|clog)\b", "ex_out", body)
    body = re.sub(r"\bstd::cin\b", "ex_in", body)
    body = re.sub(r"(?<![\w:.>])cin\b", "ex_in", body)
    body = re.sub(r"\busing\s+(ex_out|ex_in)\s*;", "", body)  # was using std::cout;
    body = re.sub(r"\bstd::printf\s*\(", "ex_printf(", body)
    body = re.sub(r"(?<![\w:.>])printf\s*\(", "ex_printf(", body)
    body = re.sub(r"(?<![\w:.>])puts\s*\(", "ex_puts(", body)
    # `main` may end without return (an implicit return 0); a renamed function
    # may not, so append one to the body.
    pos = 0
    while True:
        m = re.search(r"int example_main\([^)]*\)\s*(try\s*)?\{", body[pos:])
        if not m:
            break
        start = pos + m.end() - 1
        if m.group(1):
            pos = start + 1
            continue
        depth, i = 0, start
        while i < len(body):
            if body[i] == "{":
                depth += 1
            elif body[i] == "}":
                depth -= 1
                if depth == 0:
                    body = body[:i] + "    return 0;  // main's implicit return\n" + body[i:]
                    break
            i += 1
        pos = i + 40
    for kv in args.post_re:
        stem, _, rule = kv.partition(":")
        if kmatch(stem, path):
            old, _, new = rule.partition("=>")
            body = re.sub(old, new, body)
    for kv in args.post_sub:
        stem, _, rule = kv.partition(":")
        if kmatch(stem, path):
            old, _, new = rule.partition("=>")
            assert old in body, (stem, old)
            body = body.replace(old, new)
    main_args = ""
    m = re.search(r"int example_main\(([^)]*)\)", body)
    if m and m.group(1).strip() and m.group(1).strip() != "void":
        main_args = "ex_argc, ex_argv_t{ex_argv}"
    setup = lookup(args.setup_map, path, "")
    argv_items = lookup(args.argv_map, path, [])
    setup_code = setup + "".join("    ex_args.push_back(%s);\n" % a for a in argv_items)
    per = [a for kv in args.oracle_for if kmatch(kv.split("=", 1)[0], path) for a in kv.split("=", 1)[1].split()]
    all_includes = [l.strip() for l in src.splitlines() if re.match(r"\s*#\s*include", l) and not IN_PLACE.match(l)]
    name_ns = "ex_" + name
    if args.namespace_macro:
        body = close_around_macros(body, args.namespace_macro, name_ns)
    if args.reopen_namespaces:
        body = reopen_blocks(body, name_ns)  # (leaves nothing for hoist_namespaces)
    for kv in args.global_re:
        stem, _, rx = kv.partition(":")
        if kmatch(stem, path):
            m = re.search(rx, body, re.S)
            assert m, (stem, rx)
            body = body[:m.start()] + "}  // namespace %s (reopened: at global scope in the original)\n%s\nnamespace %s {" % (
                name_ns, qualify(m.group(0), declared_names(body), name_ns), name_ns) + body[m.end():]
    hoisted, body = hoist_namespaces(body)
    call = "%s::example_main(%s)" % (name_ns, main_args)
    serve = lookup(args.serve_map, path)
    if serve:  # a server: it runs in the background, the harness client is the result
        call = "(ex_serve_async([a = ex_args] { (void)ex_call_main(&%s::example_main, a); }, ex_port), (%s))" % (name_ns, serve)
    part = dict(includes=all_includes, hoisted=hoisted, ns=name_ns, body=body)
    ORACLE_PARTS[path.stem] = ORACLE_PARTS[relkey(path)] = part
    def run_oracle():
        comp = [ORACLE_PARTS[c] for kv in args.with_ for k, _, v in [kv.partition("=")] if kmatch(k, path)
                for c in v.split(",")]
        incs = list(dict.fromkeys(all_includes + [i for c in comp for i in c["includes"]]))
        text = "\n".join(incs) + "\n" + ORACLE_HELPERS + EXTRA_HELPERS
        for c in comp:
            text += c["hoisted"] + "\nnamespace %s {\n%s\n}\n" % (c["ns"], c["body"])
        text += hoisted + "\nnamespace %s {\n%s\n}\n" % (name_ns, body)
        text += "\nstatic int ex_run() {\n    ex_args = {\"example\"};\n" + setup_code + \
            "    ex_argv_store.clear();\n    for (auto& a : ex_args) ex_argv_store.push_back(a.data());\n" \
            "    ex_argv_store.push_back(nullptr);\n    ex_argv = ex_argv_store.data();\n    ex_argc = int(ex_args.size());\n" \
            "    return " + call + ";\n}\n"
        exp, why = oracle(path, args.oracle_arg + per, stdin_text, text=text, main_call="ex_run()")
        if why:
            flags.append(why)
        return exp
    return name, kept, body, run_oracle, flags, main_args, macros, stdin_text, setup_code, hoisted, serve


def strip_std_boost_blocks(body):
    out, i = [], 0
    pat = re.compile(r"^namespace\s+(boost|std)\b[^{;=]*\{", re.M)
    while True:
        m = pat.search(body, i)
        if not m:
            out.append(body[i:])
            return "".join(out)
        depth, j = 0, m.end() - 1
        while j < len(body):
            if body[j] == "{":
                depth += 1
            elif body[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        out.append(body[i:m.start()])
        i = j + 1


def declared_names(body):
    # (not what a namespace boost/std block of the example declares)
    body = strip_std_boost_blocks(body)
    declared = set(re.findall(r"^(?:struct|class)\s+(\w+)", body, re.M))
    declared |= set(re.findall(r"^enum\s+(?:class\s+|struct\s+)?(\w+)", body, re.M))
    declared |= set(re.findall(r"^namespace\s+(\w+)\s*\{", body, re.M))
    declared |= set(re.findall(r"^using\s+(\w+)\s*=", body, re.M))
    declared |= set(re.findall(r"^typedef\s[^;]*?\b(\w+)\s*;", body, re.M))
    return declared


def qualify(text, declared, ns):
    """The example's own names, qualified with the case namespace (also when
    the original wrote them ::global)."""
    for n in sorted(declared, key=len, reverse=True):
        text = re.sub(r"(?<![\w:])(?:::)?%s\b" % re.escape(n), "::%s::%s" % (ns, n), text)
    return text


def reopen_blocks(body, ns):
    """Top-level namespace boost/std blocks stay where they are (they may use
    the example's types): the case namespace closes around them."""
    declared = declared_names(body)
    out, i = [], 0
    pat = re.compile(r"^namespace\s+(boost|std)\b[^{;=]*\{", re.M)
    while True:
        m = pat.search(body, i)
        if not m:
            out.append(body[i:])
            break
        prefix = body[:m.start()]
        if prefix.count("{") != prefix.count("}"):
            out.append(body[i:m.end()])
            i = m.end()
            continue
        depth, j = 0, m.end() - 1
        while j < len(body):
            if body[j] == "{":
                depth += 1
            elif body[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        block = body[m.start():j + 1]
        out.append(body[i:m.start()])
        out.append("}  // namespace %s (reopened after the block)\n %s\nnamespace %s {" % (ns, qualify(block, declared, ns), ns))
        i = j + 1
    return "".join(out)


def close_around_macros(body, macro_re, ns):
    """Top-level invocations of macros that open namespaces of their own
    (BOOST_GEOMETRY_REGISTER_*): the case namespace closes around them, in
    place, and the example's own type names in their arguments are qualified."""
    declared = declared_names(body)
    out, i = [], 0
    pat = re.compile(r"^(%s)\s*\(" % macro_re, re.M)
    while True:
        m = pat.search(body, i)
        if not m:
            out.append(body[i:])
            break
        prefix = body[:m.start()]
        if prefix.count("{") != prefix.count("}"):
            out.append(body[i:m.end()])
            i = m.end()
            continue
        depth, j = 0, m.end() - 1
        while j < len(body):
            if body[j] == "(":
                depth += 1
            elif body[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        call = body[m.start():j + 1]
        head, args_ = call[:m.end() - m.start()], call[m.end() - m.start():]
        args_ = qualify(args_, declared, ns)
        out.append(body[i:m.start()])
        # Several examples register the same type (BOOST_GEOMETRY_REGISTER_BOOST_TUPLE_CS
        # ...): the file keeps the first registration of each.
        guard = "EX_REGISTERED_" + hashlib.sha1((head + args_).encode()).hexdigest()[:12]
        out.append("}  // namespace %s (reopened after the registration)\n#ifndef %s\n#define %s\n%s%s\n#endif\nnamespace %s {"
                   % (ns, guard, guard, head, args_, ns))
        i = j + 1
    return "".join(out)


def hoist_namespaces(body):
    """Split out top-level `namespace boost|std {...}` blocks (an example
    adding overloads or specializations there must not be nested in the case
    namespace). Returns (hoisted, rest)."""
    hoisted, rest, i = [], [], 0
    pat = re.compile(r"^namespace\s+(boost|std)\b[^{;]*\{", re.M)
    while True:
        m = pat.search(body, i)
        if not m:
            rest.append(body[i:])
            break
        # depth-0 check: count braces before m
        prefix = body[:m.start()]
        if prefix.count("{") != prefix.count("}"):
            rest.append(body[i:m.end()])
            i = m.end()
            continue
        depth, j = 0, m.end() - 1
        while j < len(body):
            if body[j] == "{":
                depth += 1
            elif body[j] == "}":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        rest.append(body[i:m.start()])
        hoisted.append(body[m.start():j + 1])
        i = j + 1
    return "\n".join(hoisted), "".join(rest)


def raw(s):
    # A raw literal cannot hold \r (line endings in source are \n) or other
    # control characters: those outputs get an escaped literal, a line each.
    if any(c in s for c in "\r\0") or any(ord(c) < 32 and c not in "\n\t" for c in s):
        def esc(t):
            out = []
            for c in t:
                if c == "\\": out.append("\\\\")
                elif c == '"': out.append('\\"')
                elif c == "\n": out.append("\\n")
                elif c == "\r": out.append("\\r")
                elif c == "\t": out.append("\\t")
                elif ord(c) < 32: out.append("\\%03o" % ord(c))
                else: out.append(c)
            return "".join(out)
        lines = s.splitlines(keepends=True) or [""]
        return "std::string_view(" + "\n        ".join('"%s"' % esc(l) for l in lines) + ", %d)" % len(s.encode())
    d = "ex"
    while (")" + d + '"') in s:
        d += "x"
    return 'R"%s(%s)%s"' % (d, s, d)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--lib", required=True)
    ap.add_argument("--modules", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--keep-include", default="")
    ap.add_argument("--header", default="")
    ap.add_argument("--oracle-arg", action="append", default=[])
    ap.add_argument("--oracle-for", action="append", default=[], help="STEM=extra oracle args")
    ap.add_argument("--post-re", action="append", default=[], help="STEM:REGEX=>REPL applied to the port only")
    ap.add_argument("--post-sub", action="append", default=[], help="STEM:OLD=>NEW applied to the port only")
    ap.add_argument("--add-include", action="append", default=[], help="header to include before the imports")
    ap.add_argument("--std-includes", action="store_true",
                    help="keep the standard #includes instead of import std (names the original does not see stay unseen)")
    ap.add_argument("--inline-local", action="store_true", help="inline the example's own quoted headers")
    ap.add_argument("--output-unless", action="append", default=[], help="STEM=COND: skip the output check where COND holds")
    ap.add_argument("--reopen-namespaces", action="store_true",
                    help="namespace boost/std blocks stay in place, the case namespace closes around them")
    ap.add_argument("--global-re", action="append", default=[],
                    help="STEM:REGEX: code that must stay at global scope (the case namespace closes around it)")
    ap.add_argument("--macros-of", default="",
                    help="REGEX: kept headers replaced by copies of their function-like macro definitions")
    ap.add_argument("--namespace-macro", default="",
                    help="REGEX: top-level macros that open namespaces; the case namespace closes around them")
    ap.add_argument("--only-if", action="append", default=[],
                    help="STEM=COND: the whole case (program and test) only where COND holds")
    ap.add_argument("--with", dest="with_", action="append", default=[], help="STEM=OTHER[,OTHER]: companions in the oracle")
    ap.add_argument("--extra-helpers", default="", help="file of helper code for the tests and the oracle")
    ap.add_argument("--output-platform", action="store_true", help="compare outputs only where EX_OUTPUT_PLATFORM (the header defines it)")
    ap.add_argument("--reset-locale", action="store_true", help="each case restores the classic global locale first")
    ap.add_argument("--case-from-rel", action="store_true", help="name cases after the path under example/")
    ap.add_argument("--merge", action="append", default=[], help="STEM=a.cpp,b.cpp: other TUs of the same program")
    ap.add_argument("--pure", action="store_true", help="keep every include, import nothing")
    ap.add_argument("--rel", action="append", default=[], help="STEM=upstream path to cite")
    ap.add_argument("--setup", action="append", default=[], help="STEM=C++ statements run before the example")
    ap.add_argument("--argv", action="append", default=[], help="STEM=expr,expr (std::string argv[1..])")
    ap.add_argument("--stdin", action="append", default=[], help="NAME=TEXT fed as stdin")
    ap.add_argument("--serve", action="append", default=[],
                    help="KEY=EXPR: a server; it runs in the background (ex_serve_async on ex_port) and EXPR, a harness client, is the result")
    ap.add_argument("files", nargs="+")
    args = ap.parse_args()
    args.setup_map = {}
    for kv in args.setup:
        k, _, v = kv.partition("=")
        args.setup_map[k] = args.setup_map.get(k, "") + "    " + v + "\n"
    args.argv_map = {}
    for kv in args.argv:
        k, _, v = kv.partition("=")
        args.argv_map[k] = [x for x in v.split(",,") if x]
    args.serve_map = dict(kv.split("=", 1) for kv in args.serve)
    args.stdin_map = {}
    for kv in args.stdin:
        k, _, v = kv.partition("=")
        args.stdin_map[k] = v.encode().decode("unicode_escape")
    global EXTRA_HELPERS
    if args.extra_helpers:
        EXTRA_HELPERS = Path(args.extra_helpers).read_text()
    cases, kept_all, report = [], [], []
    ported = [(Path(f),) + port_one(Path(f), args) for f in args.files]
    for p, name, kept, body, run_oracle, flags, main_args, macros, stdin_text, setup_code, hoisted, serve in ported:
        exp = run_oracle()
        rel = lookup(dict(kv.split("=", 1) for kv in args.rel), p) or common.release_rel(p)
        for k in kept:
            if k not in kept_all:
                kept_all.append(k)
        rc, out = exp if exp else (0, None)
        check = ""
        if out is not None:
            check = "    CHECK(ex_out.str() == std::string_view(%s));\n" % raw(out)
            # every condition given for the example, the first outermost
            conds = [v for k, _, v in (kv.partition("=") for kv in args.output_unless) if kmatch(k, p)]
            if conds:
                check = "".join("#if !(%s)  // this output is not portable there (see the file header)\n" % cond
                                for cond in conds) + check + "#endif\n" * len(conds)
            elif args.output_platform:
                check = "#if EX_OUTPUT_PLATFORM  // the oracle's platform (see the file header)\n" + check + "#endif\n"
        only = lookup(dict(kv.split("=", 1) for kv in args.only_if), p)
        cases.append(
            ("#if %s  // the program uses what only exists there (see the file header)\n" % only if only else "") +
            ("// %s: its additions to namespace boost/std, outside the case namespace\n%s\n" % (rel, hoisted) if hoisted else "") +
            ("// %s\nnamespace ex_%s {\n%s\n\nTEST_CASE(\"example: %s\") {\n"
             + ("    std::locale::global(std::locale::classic());  // no global locale left by another example\n" if args.reset_locale else "") +
             "    ex_out = std::ostringstream();  // fresh state: no flags left by another example\n%s%s    int rc = %s;\n    CHECK(rc == %d);\n%s}\n}  // namespace ex_%s\n")
            % (rel, name, body.strip("\n"),
               (relkey(p) if args.case_from_rel else p.stem),
               ("    ex_in.clear();\n    ex_in.str(%s);  // the program's stdin\n" % raw(stdin_text)) if stdin_text is not None else "",
               ("    ex_args = {\"example\"};\n" + setup_code +
                "    ex_argv_store.clear();\n    for (auto& a : ex_args) ex_argv_store.push_back(a.data());\n"
                "    ex_argv_store.push_back(nullptr);\n    ex_argv = ex_argv_store.data();\n"
                "    ex_argc = int(ex_args.size());\n") if (main_args or setup_code or serve) else "",
               ("example_main(%s)" % main_args) if not serve else
               "(ex_serve_async([a = ex_args] { (void)ex_call_main(&example_main, a); }, ex_port), (%s))" % serve,
               rc, check, name) +
            ("#endif\n" if only else ""))
        report.append("%-40s %s%s" % (p.stem, "FLAG " + "; ".join(flags) if flags else "ok",
                                       ("  macros: " + " ".join(macros)) if macros else ""))
    mods = "".join("import mcpplibs.mwrap.boost.%s;\n" % m for m in args.modules.split(",") if m)
    if args.macros_of:
        # Headers whose declarations the module already has: only their macro
        # definitions are copied (an #include would give gcc a second set of
        # the same partial specializations next to the module's).
        repl = []
        for k in kept_all:
            m = re.match(r"#include <([^>]+)>", k)
            if m and re.search(args.macros_of, m.group(1)):
                text = (INC / m.group(1)).read_text()
                defs = re.findall(r"^#define (BOOST_\w+\(.*?(?:\\\n.*?)*)$", text, re.M)
                repl.append("// the macros of <%s> (its declarations are in the module)\n" % m.group(1) +
                            "".join("#ifndef %s\n#define %s\n#endif\n" % (d.split("(", 1)[0], d) for d in defs))
            else:
                repl.append(k)
        kept_all = repl
    head = args.header + """#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
%s
%s%s
// Output of the ported examples is captured here; each case compares it with
// what the original program printed (built from upstream/boost alone).
inline std::ostringstream ex_out;
inline std::istringstream ex_in;
inline std::vector<std::string> ex_args;  // argv of the ported main
inline std::vector<char*> ex_argv_store;
inline char** ex_argv = nullptr;
inline int ex_argc = 0;
struct ex_argv_t {  // converts to char** and char const** (both main signatures)
    char** p;
    operator char**() const { return p; }
    operator const char**() const { return const_cast<const char**>(p); }
};
template <class... A>
int ex_printf(const char* fmt, A... a) {
    int n = std::snprintf(nullptr, 0, fmt, a...);
    std::string s(std::size_t(n) + 1, '\\0');
    std::snprintf(s.data(), s.size(), fmt, a...);
    s.resize(std::size_t(n));
    ex_out << s;
    return n;
}
inline int ex_puts(const char* s) {
    ex_out << s << '\\n';
    return 0;
}

""" % ("\n".join(["#include <cstdio>"] + ([] if args.pure else []) + [k for k in kept_all if k != "#include <cstdio>"]
                  + (["#include <filesystem>", "#include <fstream>", "#include <sstream>", "#include <string>",
                      "#include <string_view>", "#include <vector>"] if (args.pure or args.std_includes) else [])),
       "" if (args.pure or args.std_includes) else "import std;\n", mods)
    Path(args.out).write_text(head + EXTRA_HELPERS + ("\n" if EXTRA_HELPERS else "") + "\n".join(cases))
    print("\n".join(report))


if __name__ == "__main__":
    main()
