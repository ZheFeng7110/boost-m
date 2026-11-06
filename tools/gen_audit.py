#!/usr/bin/env python3
"""gen_audit.py — audit target libraries for entities that cannot be exported.

Default mode (linkage audit), per library, from clang's AST of the bundle TU
(tools/ast_json.py — the same compile gen_exports.py runs):
  - namespace-scope `static` / internal-linkage functions (the static-inline
    manual-replacement list — M0: 27 target libs should be ~0)
  - anonymous-namespace entities
  - explicit specializations of templates (exported via the primary template
    only; a specialization declared here is lost to module consumers)
  - friend operators (informational: re-exported through the class by ADL)

Output: target/gen/audit/<lib>.txt + stdout summary. Each library's report is
cached in target/gen/exports-cache/<lib>.audit.json, keyed on the compile gate
key, this tool's and the generator's code, and valid only while every file
clang read for it is unchanged: a hit skips the AST dump.

--macros mode (M10, T3 boundary check): macro-surface statistics per library —
  how many macros the library's own public header set #defines (minus #undefs),
  bucketed by canonical macro family (BOOST_PP_/BOOST_MPL_/BOOST_FUSION_...),
  and — for module libs — the entity count of the generated module export list
  for contrast. This is the plan's T3 input ("公共 API 宏注入面"): macro-driven
  libraries keep their macros as the API and stay include-only (a named module
  can never export macros), while module libs keep a minimal macro face inside
  their GMF that never reaches consumers.

Output: target/gen/audit/macro_surface.txt + stdout table.

Usage:
    python3 tools/gen_audit.py [--libs optional system ...] [--out target/gen]
    python3 tools/gen_audit.py --macros [--libs ...]
"""

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

import boost_common as bc

# ---------------------------------------------------------------------------
# --macros mode (M10): macro-surface statistics
# ---------------------------------------------------------------------------

# Canonical macro family per macro-driven candidate lib (plan §2: "公共 API
# 宏注入面 (BOOST_PP_/BOOST_FUSION_/BOOST_SPIRIT_ 等族)"); function_types
# upstream spells its config macros BOOST_FT_*, foreach has the bare
# BOOST_FOREACH object macro plus a BOOST_FOREACH_* namespace.
_LIB_MACRO_FAMILY = {
    "preprocessor": ("BOOST_PP_", "BOOST_PREPROCESSOR_"),
    "mpl": ("BOOST_MPL_",),
    "fusion": ("BOOST_FUSION_",),
    "proto": ("BOOST_PROTO_",),
    "spirit": ("BOOST_SPIRIT_",),
    "phoenix": ("BOOST_PHOENIX_",),
    "xpressive": ("BOOST_XPRESSIVE_",),
    "lambda": ("BOOST_LAMBDA_",),
    "lambda2": ("BOOST_LAMBDA2_",),
    "bind": ("BOOST_BIND_",),
    "typeof": ("BOOST_TYPEOF_",),
    "vmd": ("BOOST_VMD_",),
    "parameter": ("BOOST_PARAMETER_",),
    "metaparse": ("BOOST_METAPARSE_",),
    "function_types": ("BOOST_FT_", "BOOST_FUNCTION_TYPES_"),
    "tti": ("BOOST_TTI_",),
    "local_function": ("BOOST_LOCAL_FUNCTION_",),
    "msm": ("BOOST_MSM_",),
    "foreach": ("BOOST_FOREACH",),
    # predef's API is its detection families (BOOST_OS_/BOOST_COMP_/...)
    "predef": ("BOOST_PREDEF", "BOOST_OS_", "BOOST_COMP_", "BOOST_ARCH_",
               "BOOST_HW_", "BOOST_LIB_", "BOOST_PLAT_", "BOOST_ENDIAN_",
               "BOOST_LANG_"),
    "static_assert": ("BOOST_STATIC_ASSERT",),
    "hof": ("BOOST_HOF_",),
    "units": ("BOOST_UNITS_",),
}
# Longest-first so nested families bucket correctly (BOOST_LAMBDA2_ before
# BOOST_LAMBDA_).
_FAMILY_PREFIXES = sorted(
    {p for fams in _LIB_MACRO_FAMILY.values() for p in fams},
    key=len, reverse=True)

_DEFINE_RE = re.compile(
    r"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)(\([^)\n]*\))?", re.MULTILINE)
_UNDEF_RE = re.compile(r"^[ \t]*#[ \t]*undef[ \t]+([A-Za-z_]\w*)", re.MULTILINE)


def macro_surface_of(lib, headers):
    """(kinds {name: 'function'|'object'}, header count) — macros #defined in
    the library's own header set minus names #undef'd within the same set.
    Text-level scan: the macro face is a preprocessor notion, not an AST one."""
    kinds, undefed = {}, set()
    n = 0
    for h in headers:
        try:
            text = h.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        n += 1
        for m in _DEFINE_RE.finditer(text):
            kinds[m.group(1)] = "function" if m.group(2) else "object"
        undefed.update(m.group(1) for m in _UNDEF_RE.finditer(text))
    for name in undefed:
        kinds.pop(name, None)
    return kinds, n


def _macro_family(name):
    for prefix in _FAMILY_PREFIXES:
        if name.startswith(prefix):
            return prefix
    return "other"


def _inc_entity_count(lib):
    """Exported-entity count from the generated module export list header
    ('// lib=core ... entities=267'), or None for non-module libs."""
    inc = bc.GEN_DIR / (lib + ".inc")
    if not inc.is_file():
        return None
    m = re.search(r"entities=(\d+)",
                  inc.read_text(encoding="utf-8", errors="ignore")[:400])
    return int(m.group(1)) if m else None


def macro_headers_of(lib):
    """Header set for the macro audit: libs.json entry (module libs), else the
    non-detail heuristic; predef-style pure-.h libs (no .hpp under the root at
    all) get the matching .h set."""
    headers = bc.headers_of(lib)
    if headers:
        return headers
    d = bc.BOOST_ROOT / lib
    out = []
    if d.is_dir():
        out = [p for p in d.rglob("*.h") if p.is_file()
               and "detail" not in p.relative_to(bc.BOOST_ROOT).parts]
    single = bc.BOOST_ROOT / (lib + ".h")
    if single.is_file():
        out.append(single)
    return out


def macros_audit(libs, out):
    """Aggregate macro-surface report → out/'macro_surface.txt' + stdout."""
    rows = []
    for lib in libs:
        kinds, n_headers = macro_surface_of(lib, macro_headers_of(lib))
        fam_counts = Counter(_macro_family(name) for name in kinds)
        own_prefixes = _LIB_MACRO_FAMILY.get(lib, ())
        own = sum(1 for name in kinds
                  if any(name.startswith(p) for p in own_prefixes))
        top = ", ".join("{} {}".format(fam, cnt)
                        for fam, cnt in fam_counts.most_common(3))
        rows.append((lib, n_headers, len(kinds),
                     sum(1 for k in kinds.values() if k == "function"),
                     own, _inc_entity_count(lib), top))
        print("{}: headers={} macros={} (function-like={}) own-family={} "
              "top: {}".format(lib, n_headers, len(kinds),
                               rows[-1][3], own, top))

    t3 = set(bc.LIBS_T3 + bc.LIBS_INCLUDE_ONLY_M9)
    out.mkdir(parents=True, exist_ok=True)
    with open(out / "macro_surface.txt", "w", encoding="utf-8") as fh:
        fh.write("# boost-module macro-surface audit (boost {}) - M10 T3 "
                 "boundary check\n".format(bc.BOOST_VERSION))
        fh.write("# macros = unique names #defined in the library's own "
                 "public header set (libs.json for module libs,\n")
        fh.write("#   boost/<lib>/**.hpp non-detail heuristic otherwise), "
                 "minus #undef'd names.\n")
        fh.write("# Module libs keep this face inside their GMF (macros never "
                 "reach module consumers); include-only\n")
        fh.write("# libs inject it into every consumer TU - hence no module "
                 "(plan sec. 2, user decision sec. 5.3).\n")
        fh.write("# exports = entities in the generated module export list "
                 "(module libs only).\n\n")
        fh.write("{:<18} {:>8} {:>7} {:>9} {:>6} {:>8}\n".format(
            "lib", "headers", "macros", "func-like", "own", "exports"))
        for lib, n_headers, total, nfun, own, exports, _top in rows:
            fh.write("{:<18} {:>8} {:>7} {:>9} {:>6} {:>8}\n".format(
                lib + ("*" if lib in t3 else ""), n_headers, total, nfun,
                own, exports if exports is not None else "-"))
        fh.write("\n# * = include-only (T3 candidate or M9 downgrade)\n")
    print("wrote {}".format(out / "macro_surface.txt"))
    return 0


def _audit_key(lib, gfm, policy):
    import gen_exports as ge
    h = hashlib.sha256()
    h.update(b"gate:" + ge.gate_key(lib, gfm, policy).encode())
    h.update(b"code:" + ge._code_hash().encode())
    h.update(b"audit:" + hashlib.sha256(
        Path(__file__).read_bytes()).hexdigest().encode())
    return h.hexdigest()


def audit_lib(lib, out_dir, policy, use_cache=True):
    """Linkage audit of one library from the compiler's AST of its bundle TU
    (the same compile gen_exports.py runs; its pruned-GMF cache is reused)."""
    import gen_exports as ge
    g = policy.gmf(lib)
    gfm = [bc.DEPS / h for h in g] if g else bc.gfm_headers_of(lib)
    out = out_dir / (lib + ".txt")
    cache = bc.CACHE_DIR / (lib + ".audit.json")
    key = _audit_key(lib, gfm, policy) if use_cache else None
    cached = ge._load_json(cache) if use_cache else None
    if cached is not None and cached.get("key") == key \
            and ge._manifest_ok(cached.get("inputs")):
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(cached["text"], encoding="utf-8", newline="\n")
        return tuple(cached["counts"])

    parsed = ge.parse_bundle(lib, gfm, policy, use_cache=use_cache)
    if parsed is None:
        raise SystemExit("error: {} does not compile".format(lib))
    tu = parsed[0]
    m = ge.Model(tu)

    static_fns, anon, specs, friends = [], [], [], []
    for d in m.decls:
        if ge.home_of_file(d.file) != lib:
            continue
        loc = "{}:{}".format(Path(d.file).name, d.line)
        if d.kind == "FriendDecl":
            friends.append(loc)
            continue
        if d.attrs.get("isImplicit") or d.friend or m.is_pattern(d):
            continue
        parent = d.parent
        if parent is not None and parent.kind == "NamespaceDecl" and \
                not parent.name and d.kind in ge.EXPORT_KINDS:
            anon.append("{} {}".format(loc, d.name or "<unnamed>"))
            continue
        qn = m.qualified_name(d)
        if not qn.startswith("boost::"):
            continue
        if d.kind in ("FunctionDecl", "FunctionTemplateDecl") and \
                m.namespace_chain(d) and not m.linkage_ok(d):
            static_fns.append("{} {} (internal linkage)".format(loc, qn))
        if m.is_specialization(d) and m.namespace_chain(m.primary(d)):
            specs.append("{} {}".format(loc, qn))

    text = "".join((
        "# audit {} (boost {})\n".format(lib, bc.BOOST_VERSION),
        "# {} static/internal free functions\n".format(len(static_fns)),
        "\n".join(sorted(static_fns)) + "\n\n",
        "# {} anonymous-namespace entities\n".format(len(anon)),
        "\n".join(sorted(anon)) + "\n\n",
        "# {} explicit specializations\n".format(len(specs)),
        "\n".join(sorted(specs)) + "\n\n",
        "# {} friend operator decls (info)\n".format(len(friends)),
        "\n".join(sorted(friends)) + "\n",
    ))
    counts = (len(static_fns), len(anon), len(specs), len(friends))
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(text, encoding="utf-8", newline="\n")
    if use_cache:
        bc.CACHE_DIR.mkdir(parents=True, exist_ok=True)
        cache.write_text(json.dumps(
            {"key": key, "inputs": ge._input_manifest(tu, bc.ROOT),
             "text": text, "counts": list(counts)}),
            encoding="utf-8")
    return counts


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--libs", nargs="*", default=None,
                    help="subset of libraries (default: all target libs, "
                         "+T3/include-only candidates in --macros mode)")
    ap.add_argument("--out", type=Path, default=bc.AUDIT_DIR)
    ap.add_argument("--macros", action="store_true",
                    help="macro-surface statistics (M10 T3 boundary check) "
                         "instead of the linkage audit")
    args = ap.parse_args()

    if args.macros:
        libs = args.libs if args.libs is not None else \
            bc.TARGET_LIBS + bc.LIBS_T3 + bc.LIBS_INCLUDE_ONLY_M9
        return macros_audit(libs, args.out)

    import gen_exports as ge
    ge._FILE_TO_LIB.update(bc.build_file_to_lib())
    policy = ge.Policy.load()
    libs = args.libs or bc.TARGET_LIBS
    total = [0, 0, 0, 0]
    for lib in libs:
        # Hand-written modules such as mcpplibs.mwrap.boost.version intentionally have no
        # gen_exports bundle TU. Their headers still participate in --macros.
        if lib in bc.LIBS_SPECIAL:
            print("skip {} (hand-written module, no generated bundle)".format(lib))
            continue
        n = audit_lib(lib, args.out, policy)
        total = [t + x for t, x in zip(total, n)]
        print("{}: static={} anon={} specs={} friends={}".format(lib, *n))
    print("TOTAL: static={} anon={} specs={} friends={}".format(*total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
