#!/usr/bin/env python3
"""gen_exports.py — generate the export using-lists of the Boost module layer.

Principle (see docs/architecture.md §5): the compiler computes the public
surface, the generator only lists it. For each target library one bundle
translation unit includes the library's public headers (the module's GMF
include set) and is compiled once with

    clang++ -fsyntax-only -Xclang -ast-dump=json -Xclang -ast-dump-filter=boost

(tools/ast_json.py). From that AST the generator collects every namespace-
scope `boost::` entity with external linkage the library owns, closes the set
over the entity signatures, and writes

    generated/exports/<lib>.inc     export namespace boost { using ...; }
    generated/exports/<lib>.deps    export-import edges (complete module names)
    generated/meta/api-surface.json generator identity + per-library summary
    generated/meta/skipped.txt      every name the policy guards or drops

Nothing else edits these files. What the AST cannot decide — platform-only
declarations of the fixed MinGW snapshot, names that must not be exported,
GMF overrides, verbatim lines — is declared in tools/export_policy.toml with a
written reason and applied here, so the committed output is exactly what the
generator writes and `--check` can prove it byte for byte.

The `src/<lib>.cppm` interfaces are hand-maintained facades: GMF includes,
`export import` edges and `#include` of the generated list. The generator never
rewrites them; `--check` verifies that each facade's GMF include set equals the
bundle's (gate-pruned) include set the export list was computed from, and
that its `export import` block covers every <lib>.deps edge.

Ownership and dedup: libraries are processed dependencies-first (include-based
topological order); the first library to claim an entity owns it, later
libraries omit it and record `export import mcpplibs.mwrap.boost.<home>;` in
<lib>.deps. Pass 2 walks the bodies of every exported entity and adds the
edges a consumer of that module needs (cycles and transitive edges skipped).

Usage:
    python tools/gen_exports.py                   # all target libraries
    python tools/gen_exports.py --libs optional system
    python tools/gen_exports.py --check           # byte-compare, write nothing
    python tools/gen_exports.py --emit-cppm       # draft facades that are missing
    python tools/gen_exports.py --scan            # regenerate generated/meta/libs.json
"""

import argparse
import difflib
import hashlib
import json
import os
import re
import shutil
import sys
import tempfile
import time
import tomllib
from collections import deque
from pathlib import Path

import ast_json as aj
import boost_common as bc

POLICY_FILE = bc.TOOLS / "export_policy.toml"
SURFACE_JSON = bc.ROOT / "generated" / "meta" / "api-surface.json"
SKIPPED_TXT = bc.ROOT / "generated" / "meta" / "skipped.txt"


# ---------------------------------------------------------------------------
# policy (tools/export_policy.toml)
# ---------------------------------------------------------------------------

class Policy:
    """Per-library decisions the AST cannot make, each with a reason.

    [<lib>]
      gmf               exact GMF/bundle include set (relative to upstream/boost)
      defines           extra -D macros for the bundle (match the .cppm GMF)
      inline-namespaces export these inline namespaces' members through the
                        enclosing namespace (macro-selected inline namespace)
      add               qualified names to export even though no collector
                        reaches them (body-only references, ADL-only friends)
      facade-includes   headers the src/<lib>.cppm GMF includes beyond the
                        bundle (not part of the export computation)
    [[<lib>.guard]]     if / reason / names / else — wrap the matching exports
                        in `#if <if>`; `else` lines go to the `#else` branch
    [[<lib>.drop]]      reason / names / namespaces — never export the
                        matching names (or anything inside the namespaces)
    [[<lib>.extra]]     namespace / reason / lines — verbatim declarations
    [<lib>.deps]        add / remove / reason — module edge corrections
    [<lib>.facade]      omit-imports / reason — .deps edges src/<lib>.cppm
                        cannot `export import` (checked by check_facade)

    A `names` entry containing `::` matches a qualified name exactly; a bare
    identifier matches any `::` segment of the exported name.
    """

    def __init__(self, data):
        self.data = data

    @classmethod
    def load(cls, path=POLICY_FILE):
        if not path.is_file():
            return cls({})
        with open(path, "rb") as f:
            return cls(tomllib.load(f))

    def lib(self, lib):
        return self.data.get(lib, {})

    def gmf(self, lib):
        return self.lib(lib).get("gmf")

    def defines(self, lib):
        return list(self.lib(lib).get("defines", ()))

    def inline_namespaces(self, lib):
        return list(self.lib(lib).get("inline-namespaces", ()))

    def add(self, lib):
        return list(self.lib(lib).get("add", ()))

    def guards(self, lib):
        return list(self.lib(lib).get("guard", ()))

    def drops(self, lib):
        return list(self.lib(lib).get("drop", ()))

    def extras(self, lib):
        return list(self.lib(lib).get("extra", ()))

    def deps(self, lib):
        return self.lib(lib).get("deps", {})

    def facade_includes(self, lib):
        return list(self.lib(lib).get("facade-includes", ()))

    def facade_omit_imports(self, lib):
        return list(self.lib(lib).get("facade", {}).get("omit-imports", ()))

    def digest(self, lib):
        return hashlib.sha256(json.dumps(
            self.lib(lib), sort_keys=True).encode()).hexdigest()


def _matches(qname, names):
    segments = qname.lstrip(":").split("::")
    for n in names:
        if "::" in n:
            if qname.lstrip(":") == n.lstrip(":"):
                return True
        elif n in segments:
            return True
    return False


# ---------------------------------------------------------------------------
# semantic model over the AST declaration table
# ---------------------------------------------------------------------------

EXPORT_KINDS = frozenset((
    "CXXRecordDecl", "RecordDecl", "EnumDecl", "EnumConstantDecl",
    "FunctionDecl", "FunctionTemplateDecl", "VarDecl", "VarTemplateDecl",
    "TypedefDecl", "TypeAliasDecl", "TypeAliasTemplateDecl",
    "ClassTemplateDecl", "ClassTemplatePartialSpecializationDecl",
    "ConceptDecl",
))
SPECIALIZATION_KINDS = frozenset((
    "ClassTemplateSpecializationDecl", "ClassTemplatePartialSpecializationDecl",
    "VarTemplateSpecializationDecl", "VarTemplatePartialSpecializationDecl",
))
CLASS_KINDS = frozenset(("CXXRecordDecl", "RecordDecl", "ClassTemplateDecl",
                         "ClassTemplateSpecializationDecl"))
TYPE_ENTITY_KINDS = frozenset((
    "CXXRecordDecl", "RecordDecl", "EnumDecl", "ClassTemplateDecl",
    "ClassTemplatePartialSpecializationDecl",
    "ClassTemplateSpecializationDecl",
))
ALIAS_KINDS = frozenset(("TypedefDecl", "TypeAliasDecl",
                         "TypeAliasTemplateDecl"))
METHOD_KINDS = frozenset(("CXXMethodDecl", "CXXConstructorDecl",
                          "CXXDestructorDecl", "CXXConversionDecl"))
VAR_TEMPLATE_KIND = "VarTemplateDecl"

# Identifiers that can appear in a printed type but never name an entity.
_TYPE_WORDS = frozenset("""
    const volatile typename struct class union enum unsigned signed short long
    int char bool void float double auto decltype sizeof alignof noexcept
    wchar_t char8_t char16_t char32_t nullptr_t restrict template operator
    __int128 _Bool __restrict true false requires
""".split())
_ID_CHAIN = re.compile(r"(?<![\w:>])((?:::)?[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)")
_UNNAMED = re.compile(r"\((?:unnamed|anonymous|lambda)[^()]*\)")


def names_in_type(text):
    """Qualified-id chains spelled in a printed type (`boost::a::b<T, c::d>`
    yields boost::a::b, T, c::d). Members of dependent types (`X<T>::type`)
    are not chains of their own."""
    if not text:
        return
    text = _UNNAMED.sub("", text)
    for m in _ID_CHAIN.finditer(text):
        name = m.group(1)
        if name in _TYPE_WORDS:
            continue
        yield name


_UNKNOWN = aj.Decl("", "<unknown context>")


class Model:
    """Semantic queries over one bundle TU (ast_json.TU)."""

    def __init__(self, tu):
        self.tu = tu
        self.by_id = tu.by_id
        self.decls = sorted(tu.decls, key=lambda d: d.seq)
        self._canon = {}
        self._group = {}
        self._qn = {}
        self._qindex = None
        self._export_qindex = None
        self._export_keys = None
        for d in self.decls:
            self._group.setdefault(self.canon(d), []).append(d)
        # `typedef enum { a, b } name;` — the typedef gives the unnamed enum
        # its name for linkage purposes; the enumerators are spelled
        # `ns::name::a` and have external linkage.
        unnamed = {d.id: d for d in self.decls
                   if d.kind == "EnumDecl" and not d.name}
        if unnamed:
            for d in self.decls:
                if d.kind == "TypedefDecl" and d.refs:
                    for rid in d.refs:
                        e = unnamed.get(rid)
                        if e is not None and e.parent is d.parent:
                            e.name = d.name
                            del unnamed[rid]
                            break

    # -- structure ---------------------------------------------------------

    def canon(self, d):
        """First declaration of `d`'s redeclaration chain."""
        c = self._canon.get(d.id)
        if c is not None:
            return c
        cur, seen = d, set()
        while True:
            prev = cur.attrs.get("previousDecl")
            p = self.by_id.get(prev) if prev else None
            if p is None or p.id in seen:
                break
            seen.add(cur.id)
            cur = p
        self._canon[d.id] = cur
        return cur

    def redecls(self, d):
        return self._group.get(self.canon(d), [d])

    @staticmethod
    def is_pattern(d):
        p = d.parent
        return p is not None and p.kind in aj.TEMPLATE_KINDS and p.pattern is d

    def entity(self, d):
        """The declaration that stands for `d` as an exportable entity: the
        template for a template pattern, `d` itself otherwise."""
        if self.is_pattern(d):
            return d.parent
        return d

    def sem_parent(self, d):
        """Semantic parent: `parentDeclContextId` when clang prints one (an
        out-of-line definition), else the lexical parent. A context that was
        dropped as an instantiation resolves to its template; an unknown one
        yields the `_UNKNOWN` placeholder, never the lexical namespace."""
        pid = d.attrs.get("parentDeclContextId")
        if pid:
            p = self.tu.resolve(pid)
            return p if p is not None else _UNKNOWN
        return d.parent

    def qualified_name(self, d):
        """boost::a::b for namespace members and class members (class name
        included); '' inside anonymous namespaces, functions or linkage
        specifications, mirroring the scopes a using-declaration can name."""
        q = self._qn.get(d.id)
        if q is not None:
            return q
        names = []
        cur = d
        result = ""
        while True:
            p = self.sem_parent(cur)
            if p is not None and p.kind in aj.TEMPLATE_KINDS and \
                    p.pattern is cur:
                cur = p                     # template wrapper: same entity
                continue
            if p is None:
                if cur.name:
                    result = "::".join([cur.name] + names[::-1])
                break
            pk = p.kind
            if pk == "NamespaceDecl":
                if not p.name:
                    break                   # anonymous namespace
            elif pk == "EnumDecl":
                if not p.name:
                    if cur.name:            # unnamed enum is transparent
                        names.append(cur.name)
                    cur = p
                    continue
            elif pk not in CLASS_KINDS and pk not in ALIAS_KINDS:
                break
            if not cur.name:
                break
            names.append(cur.name)
            cur = p
        self._qn[d.id] = result
        return result

    def namespace_chain(self, d):
        """['boost', 'mp11'] for a namespace-scope declaration; [] for class
        members, locals, anonymous-namespace and linkage-spec contents."""
        chain = []
        cur = self.entity(d)
        while True:
            p = self.sem_parent(cur)
            if p is None:
                return chain[::-1]
            if p.kind != "NamespaceDecl" or not p.name:
                return []
            chain.append(p.name)
            cur = p

    def scope_names(self, d):
        """Qualified scope prefixes for unqualified lookup from inside `d`,
        innermost first (class scopes included)."""
        qn = self.qualified_name(self.entity(d))
        parts = qn.split("::") if qn else []
        return ["::".join(parts[:k]) for k in range(len(parts), 0, -1)]

    def in_anonymous_namespace(self, d):
        cur = d
        while cur is not None:
            p = self.sem_parent(cur)
            if p is not None and p.kind == "NamespaceDecl" and not p.name:
                return True
            cur = p
        return False

    # -- definitions, files, linkage --------------------------------------

    @staticmethod
    def is_definition(d):
        k = d.kind
        if k in aj.TEMPLATE_KINDS:
            return d.pattern is not None and Model.is_definition(d.pattern)
        if k in aj.RECORD_KINDS:
            return bool(d.attrs.get("completeDefinition"))
        if k in aj.FUNCTION_KINDS:
            return d.has_body or bool(d.attrs.get("explicitlyDefaulted")) or \
                bool(d.attrs.get("explicitlyDeleted"))
        if k == "VarDecl":
            return d.attrs.get("storageClass") != "extern" or "init" in d.attrs
        if k == "EnumDecl":
            return bool(d.children) or "fixedUnderlyingType" in d.attrs
        return True

    def definition(self, d):
        for r in self.redecls(d):
            if self.is_definition(r):
                return r
        return None

    def linkage_ok(self, d):
        """External linkage (what a using-declaration may export). Typedef
        names have no linkage but are exportable namespace members."""
        k = d.kind
        if k in ("TypedefDecl", "TypeAliasDecl"):
            return True
        if self.in_anonymous_namespace(d):
            return False
        if k == "EnumConstantDecl":
            enum = self.sem_parent(d)
            return enum is not None and bool(enum.name) and \
                self.linkage_ok(enum)
        if k in TYPE_ENTITY_KINDS or k in ("TypeAliasTemplateDecl",
                                           "ConceptDecl"):
            return bool(d.name)
        target = d.pattern if k in aj.TEMPLATE_KINDS else d
        if target is None:
            return True
        for r in self.redecls(d):
            t = r.pattern if r.kind in aj.TEMPLATE_KINDS else r
            if t is not None and t.attrs.get("storageClass") == "static":
                return False
        if target.kind == "VarDecl" and k == "VarDecl":
            mangled = target.attrs.get("mangledName", "")
            if mangled:
                return not _internal_mangling(mangled)
            return not (_top_const(target.type) and
                        not target.attrs.get("inline") and
                        target.attrs.get("storageClass") != "extern")
        if target.kind in aj.FUNCTION_KINDS:
            mangled = target.attrs.get("mangledName", "")
            if mangled and _internal_mangling(mangled):
                return False
        return True

    def is_specialization(self, d):
        if d.kind in SPECIALIZATION_KINDS:
            return True
        return d.kind in aj.FUNCTION_KINDS and d.targs

    # -- indexes ------------------------------------------------------------

    def indexed(self):
        """Pre-order declarations the module layer can see (not compiler-
        implicit, not friend-only declarations)."""
        for d in self.decls:
            if d.attrs.get("isImplicit") or d.friend:
                continue
            if self.is_pattern(d):
                continue
            yield d

    def qindex(self):
        """{qualified name: [decl, ...]} over every visible declaration.
        Enumerators of an unscoped enum are also found through the enclosing
        scope (`boost::algorithm::token_compress_on`), as name lookup finds
        them."""
        if self._qindex is None:
            idx = {}
            for d in self.indexed():
                qn = self.qualified_name(d)
                if not qn:
                    continue
                idx.setdefault(qn, []).append(d)
                if d.kind == "EnumConstantDecl":
                    enum = self.sem_parent(d)
                    if enum is not None and enum.name and \
                            not enum.attrs.get("scopedEnumTag") and \
                            qn.count("::") >= 2:
                        outer = qn.rsplit("::", 2)[0] + "::" + d.name
                        idx.setdefault(outer, []).append(d)
            self._qindex = idx
        return self._qindex

    def export_qindex(self):
        """{qualified name: [decl, ...]} over exportable kinds only."""
        if self._export_qindex is None:
            idx = {}
            for qn, ds in self.qindex().items():
                es = [d for d in ds if d.kind in EXPORT_KINDS]
                if es:
                    idx[qn] = es
            self._export_qindex = idx
        return self._export_qindex

    def export_index(self):
        """{stable key: decl} over exportable kinds, first declaration wins."""
        if self._export_keys is None:
            idx = {}
            for d in self.indexed():
                if d.kind in EXPORT_KINDS:
                    idx.setdefault(self.stable_key(d), d)
            self._export_keys = idx
        return self._export_keys

    def lookup(self, chain, scopes):
        """Declarations a written name chain denotes: relative names are
        looked up from the innermost enclosing scope outwards, then globally;
        `::x` is global only."""
        idx = self.qindex()
        if chain.startswith("::"):
            cands = [chain[2:]]
        else:
            cands = [s + "::" + chain for s in scopes] + [chain]
        for c in cands:
            ds = idx.get(c)
            if ds:
                return ds
        return ()

    def promote(self, d):
        """Climb from a class member to the namespace-scope entity; an
        explicit specialization stands for its primary template."""
        cur = self.primary(self.entity(d))
        while True:
            p = self.sem_parent(cur)
            if p is None:
                return cur
            if p.kind in CLASS_KINDS:
                cur = self.primary(self.entity(p))
                continue
            return cur

    def primary(self, d):
        """Primary template of a specialization (itself otherwise)."""
        if d.kind in SPECIALIZATION_KINDS or (
                d.kind in aj.FUNCTION_KINDS and d.targs):
            want = {"ClassTemplateSpecializationDecl": "ClassTemplateDecl",
                    "ClassTemplatePartialSpecializationDecl":
                        "ClassTemplateDecl",
                    "VarTemplateSpecializationDecl": "VarTemplateDecl",
                    "VarTemplatePartialSpecializationDecl":
                        "VarTemplateDecl"}.get(d.kind, "FunctionTemplateDecl")
            for t in self.qindex().get(self.qualified_name(d), ()):
                if t.kind == want:
                    return t
        return d

    def namespaces(self):
        """Qualified names of every named namespace in the TU."""
        return {self.qualified_name(d) for d in self.decls
                if d.kind == "NamespaceDecl" and d.name} - {""}

    def type_targets(self, text, scopes, depth=0, seen=None):
        """Entities a printed type refers to, with typedefs and alias
        templates looked through (the canonical-type view of a signature)."""
        if seen is None:
            seen = set()
        for name in names_in_type(text):
            for t in self.lookup(name, scopes):
                if t.id in seen:
                    continue
                seen.add(t.id)
                k = t.kind
                if k in ALIAS_KINDS:
                    if depth >= 8:
                        continue
                    a = t.pattern if k == "TypeAliasTemplateDecl" else t
                    if a is None:
                        continue
                    yield from self.type_targets(
                        a.dtype or a.type, self.scope_names(t), depth + 1,
                        seen)
                elif k in TYPE_ENTITY_KINDS:
                    yield t

    def stable_key(self, d):
        """Cross-TU identity of an entity (the role libclang's USR played):
        the Itanium mangling where clang prints one, else kind + qualified
        name (+ signature for function templates)."""
        d = self.canon(d)       # every redeclaration maps to one key
        k = d.kind
        qn = self.qualified_name(d)
        if k in aj.FUNCTION_KINDS or k == "VarDecl":
            m = d.attrs.get("mangledName")
            if m:
                return "m:" + m
            return "{}:{}|{}".format(k, qn, d.dtype or d.type)
        if k == "FunctionTemplateDecl":
            p = d.pattern
            return "ft:{}|{}".format(qn, p.type if p is not None else "")
        if k in ("CXXRecordDecl", "RecordDecl", "ClassTemplateDecl"):
            return "c:" + qn
        if k in SPECIALIZATION_KINDS:
            return "s:{}|{}".format(qn, d.id)
        return "{}:{}".format(k, qn)


def _internal_mangling(m):
    """Itanium marks internal-linkage names with `L` before the source name:
    `_ZL3foo`, `_ZN5boost6detailL3fooE`."""
    if m.startswith("_ZL"):
        return True
    if not m.startswith("_ZN"):
        return False
    i = 3
    while i < len(m) and m[i] in "rVK":
        i += 1
    while i < len(m):
        c = m[i]
        if c == "L":
            return True
        if c.isdigit():
            j = i
            while j < len(m) and m[j].isdigit():
                j += 1
            i = j + int(m[i:j])
        elif m.startswith("St", i):
            i += 2
        else:
            return False
    return False


def _top_const(qt):
    """Top-level const of a printed object type: `const T`, `T *const`."""
    t = re.sub(r"<[^<>]*>", "", qt or "")
    while "<" in t:
        n = re.sub(r"<[^<>]*>", "", t)
        if n == t:
            break
        t = n
    t = t.strip()
    if "(" in t:
        return False
    ptr = max(t.rfind("*"), t.rfind("&"))
    if ptr >= 0:
        return bool(re.search(r"\bconst\b", t[ptr + 1:]))
    return bool(re.search(r"\bconst\b", t))


# ---------------------------------------------------------------------------
# collectors
# ---------------------------------------------------------------------------

_FILE_TO_LIB = {}
_HOME_MEMO = {}


def home_of_file(path):
    h = _HOME_MEMO.get(path)
    if h is None:
        h = _HOME_MEMO[path] = bc.home_lib_of_file(Path(path), _FILE_TO_LIB) \
            if path else "shared"
    return h


def entity_record(m, d, home):
    kind = d.kind
    ns = (m.namespace_chain(m.sem_parent(d)) if kind == "EnumConstantDecl"
          else m.namespace_chain(d))
    return {
        "name": d.name,
        "qname": m.qualified_name(d),
        "usr": m.stable_key(d),
        "ns": ns,
        "file": d.file,
        "home": home,
        "kind": kind,
    }


def _owner_file(m, d):
    """Ownership follows the definition, not the first declaration: a forward
    declaration in another library's header must not let that library claim
    the entity (boost::rational forward-declared by boost.integer)."""
    dd = m.definition(d)
    return (dd or d).file


def collect_candidates(lib, m):
    """External-linkage namespace-scope boost:: entities reachable through the
    bundle TU, owned by this library or by no target library ('shared')."""
    found = {}
    for d in m.indexed():
        if d.kind not in EXPORT_KINDS:
            continue
        home = home_of_file(_owner_file(m, d))
        if home not in (lib, "shared"):
            continue
        if m.is_specialization(d):
            continue
        if not d.name or not (d.name[0].isalpha() or d.name[0] == "_"):
            continue                # deduction guides, anonymous entities
        lex = d.parent
        if lex is not None and lex.kind not in (
                "NamespaceDecl", "LinkageSpecDecl", "EnumDecl", "ExportDecl"):
            continue                # class member / friend-in-class
        qn = m.qualified_name(d)
        if not qn.startswith("boost::"):
            continue
        if d.kind == "EnumConstantDecl":
            enum = m.sem_parent(d)
            if not m.namespace_chain(enum):
                continue
            if enum.attrs.get("scopedEnumTag"):
                continue            # enum class: enumerators come with it
        elif not m.namespace_chain(d):
            continue
        if not m.linkage_ok(d):
            continue
        if any(r.parent is not None and r.parent.kind == "LinkageSpecDecl"
               for r in m.redecls(d)):
            continue                # C language linkage: not namespace API
        key = m.stable_key(d)
        if key in found:
            continue
        found[key] = entity_record(m, d, home)
    return found


def _enclosing_ns(m, d):
    """Namespace chain of a using-* declaration's lexical scope ([] inside an
    anonymous namespace)."""
    parts = []
    cur = d.parent
    while cur is not None and cur.kind == "NamespaceDecl":
        if not cur.name:
            return []
        parts.append(cur.name)
        cur = cur.parent
    return parts[::-1]


def _using_target_name(m, d):
    """(injected qname, target qname) of a namespace-scope using-declaration
    from the name clang prints as written (`range::count`,
    `boost::iterators::counting_iterator`, `::boost::bimaps::bimap`,
    `std::ratio`, `::GetCurrentProcessId`)."""
    rel = d.name or ""
    if not rel:
        return "", ""
    parts = _enclosing_ns(m, d)
    absolute = rel.startswith("::")
    if absolute:
        rel = rel[2:]
    if absolute or rel.startswith(("std::", "boost::")):
        # Fully qualified target: the declaration still injects its last
        # segment into the enclosing namespace (`namespace boost::range {
        # using boost::range_detail::formatted_range; }` publishes
        # boost::range::formatted_range).
        if not parts or parts[0] != "boost":
            return "", ""
        return "::".join(parts) + "::" + rel.rsplit("::", 1)[-1], rel
    if not parts or parts[0] != "boost":
        return "", ""
    prefix = "::".join(parts)
    return prefix + "::" + rel.split("::")[-1], prefix + "::" + rel


def collect_injections(lib, m, claimed):
    """Namespace-scope `using boost::X;` / `using namespace X;` that give
    entities their public spelling (ADL barriers, detail re-exports). The
    injected name must denote external-linkage entities only."""
    out = {}
    qmap = m.export_qindex()
    directives = []
    for d in m.indexed():
        if d.kind not in ("UsingDecl", "UsingDirectiveDecl"):
            continue
        if d.parent is None or d.parent.kind != "NamespaceDecl":
            continue
        if home_of_file(d.file) not in (lib, "shared"):
            continue
        if d.kind == "UsingDecl":
            q, target = _using_target_name(m, d)
            # First wins per (name, target): `boost::join` from range::join
            # and from algorithm::join are different overload sets, and an
            # exported using-declaration carries only what its own GMF sees.
            ckey = q + "=" + target
            if not q or q in claimed or ckey in claimed:
                continue
            if target.startswith("std::") or not target.startswith("boost::"):
                emit_q = target if target.startswith("std::") \
                    else "::" + target
            else:
                emit_q = q
                cur = qmap.get(target) or qmap.get(q)
                if not cur:
                    continue
                if any(not m.linkage_ok(c) for c in cur):
                    continue
            claimed[ckey] = lib
            out[q] = {
                "name": emit_q.rsplit("::", 1)[-1], "qname": emit_q,
                "usr": "inject:" + ckey, "ns": q.split("::")[:-1],
                "file": d.file, "home": lib, "kind": "USING_INJECT",
            }
        else:
            ns = m.by_id.get(d.target)
            if ns is None:
                continue
            tgt = m.qualified_name(ns)
            if not tgt.startswith("boost::"):
                continue
            parts = _enclosing_ns(m, d)
            if not parts:
                continue
            prefix = "::".join(parts)
            if not tgt.startswith(prefix + "::"):
                # Only the ADL-barrier shape re-publishes names: a namespace
                # nested in the directive's own scope (`namespace boost {
                # namespace range_adl_barrier {...} using namespace
                # range_adl_barrier; }`). A directive that pulls a sibling
                # detail namespace in (`using namespace detail;` inside
                # poly_collection::common_impl) is an implementation
                # convenience, not a public spelling.
                continue
            directives.append((tgt.split("::"), prefix, d.file))
    for tgt_chain, prefix, df in directives:
        for d in m.indexed():
            if d.kind not in EXPORT_KINDS:
                continue
            if m.namespace_chain(d) != tgt_chain:
                continue
            if not m.linkage_ok(d):
                continue
            q = prefix + "::" + d.name
            ckey = q + "=" + "::".join(tgt_chain) + "::" + d.name
            if not q.startswith("boost::") or q in claimed or ckey in claimed:
                continue
            claimed[ckey] = lib
            out[q] = {
                "name": d.name, "qname": q, "usr": "inject:" + ckey,
                "ns": q.split("::")[:-1], "file": df, "home": lib,
                "kind": "USING_INJECT",
            }
    return out


def collect_added(lib, m, policy, claimed):
    """Policy `add` names: exported like using-injections when the TU
    declares them (a name the GMF cannot see would not compile)."""
    out = {}
    qmap = m.export_qindex()
    for q in policy.add(lib):
        if not q.startswith("boost::"):
            continue
        if q not in qmap:
            print("    policy add skipped (not in TU): {}".format(q))
            continue
        claimed.setdefault(q, lib)
        out[q] = {
            "name": q.rsplit("::", 1)[-1], "qname": q, "usr": "inject:" + q,
            "ns": q.split("::")[:-1], "file": str(POLICY_FILE), "home": lib,
            "kind": "POLICY_ADD",
        }
    return out


# ---------------------------------------------------------------------------
# signature closure (pass 1)
# ---------------------------------------------------------------------------

def signature_types(m, d):
    """(type text, lookup scopes) pairs of an entity's declaration: function
    parameters and result, base classes, fields, member results, static data
    members, typedef/variable types."""
    k = d.kind
    scopes = m.scope_names(d)
    if k == "FunctionDecl":
        for t in d.params:
            yield t, scopes
        yield _result_type(d.type), scopes
    elif k == "FunctionTemplateDecl":
        if d.pattern is not None:
            for t in d.pattern.params:
                yield t, scopes
    elif k in ("CXXRecordDecl", "RecordDecl", "ClassTemplateDecl",
               "ClassTemplatePartialSpecializationDecl"):
        rec = d.pattern if k == "ClassTemplateDecl" else d
        if rec is None:
            return
        inner = m.scope_names(rec) if k != "ClassTemplateDecl" else scopes
        for b in rec.bases:
            yield b, inner
        for ch in rec.children:
            ck = ch.kind
            if ck == "FieldDecl" or ck == "VarDecl":
                yield ch.type, inner
            elif ck in METHOD_KINDS:
                yield _result_type(ch.type), inner
    elif k in ("TypedefDecl", "TypeAliasDecl", "VarDecl"):
        yield d.dtype or d.type, scopes


def _result_type(ftype):
    """Result part of a printed function type `R (params)`; the trailing
    form `auto (params) -> R` yields R."""
    if not ftype:
        return ""
    depth = 0
    for i, c in enumerate(ftype):
        if c in "<[":
            depth += 1
        elif c in ">]":
            depth -= 1
        elif c == "(" and depth == 0:
            head = ftype[:i].strip()
            if head == "auto":
                tail = ftype.rsplit("->", 1)
                return tail[1].strip() if len(tail) == 2 else ""
            return head
    return ""


def closure_from(m, cands):
    """BFS over candidate signatures; {key: record} for every boost:: entity
    reachable through the declarations (exports and closure pulls)."""
    records = dict(cands)
    queue = deque(r for r in cands.values()
                  if not r["usr"].startswith("inject:"))
    by_key = m.export_index()
    pulled = {}             # closure targets by_key lacks; it is shared
    seen = set(records)
    while queue:
        rec = queue.popleft()
        d = by_key.get(rec["usr"]) or pulled.get(rec["usr"])
        if d is None:
            continue
        for text, scopes in signature_types(m, d):
            for t in m.type_targets(text, scopes):
                target = m.promote(t)
                qn = m.qualified_name(target)
                if not qn.startswith("boost::"):
                    continue
                key = m.stable_key(target)
                if key in seen:
                    continue
                seen.add(key)
                if key not in by_key:
                    pulled.setdefault(key, target)
                home = home_of_file(_owner_file(m, target))
                records[key] = entity_record(m, target, home)
                queue.append(records[key])
    return records


# ---------------------------------------------------------------------------
# pass 2: body-reference edge completion
# ---------------------------------------------------------------------------

def collect_body_edges(lib, m, records, claimed):
    """{home: set(key)} — other target libraries whose entities the BODIES of
    this library's exported entities refer to (references clang resolved, and
    the types they spell). The signature closure under-approximates what a
    consumer instantiating those bodies needs (bimap -> boost.iterator)."""
    edges = {}
    seen_pairs = set()

    def add_edge(r):
        r = m.promote(r)
        if not m.qualified_name(r).startswith("boost::"):
            return
        key = m.stable_key(r)
        rec = records.get(key)
        if rec is not None and (rec["home"] == lib or key not in claimed):
            return              # rides on this module's own face
        home = home_of_file(_owner_file(m, r))
        if home == lib or home == "shared" or home not in bc.TARGET_LIBS:
            return
        if (key, home) in seen_pairs:
            return
        seen_pairs.add((key, home))
        edges.setdefault(home, set()).add(key)

    by_key = m.export_index()
    for rec in records.values():
        if rec["home"] != lib and rec["usr"] in claimed:
            continue            # claimed elsewhere: not exported here
        d = by_key.get(rec["usr"])
        if d is None:
            continue
        walked = [d]
        dd = m.definition(d)
        if dd is not None and dd is not d:
            walked.append(dd)
        for w in walked:
            if w.refs:
                for rid in w.refs:
                    r = m.tu.resolve(rid)
                    if r is not None:
                        add_edge(r)
            if w.written:
                scopes = m.scope_names(w)
                for text in w.written:
                    for name in names_in_type(text):
                        for r in m.lookup(name, scopes):
                            if r.kind in EXPORT_KINDS:
                                add_edge(r)
    return edges


def merge_body_edges(out_dir, order, body_edges, policy):
    """Union pass-2 candidate edges into generated/exports/<lib>.deps, in
    topological order: direct, transitive and cycle-closing edges are
    skipped (mcpp rejects import cycles)."""
    deps_dir = out_dir / "generated" / "exports"
    graph = {}
    for p in sorted(deps_dir.glob("*.deps")):
        edges = set()
        for line in p.read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].strip()
            if line.startswith(bc.MODULE_PREFIX + "."):
                edges.add(line[len(bc.MODULE_PREFIX) + 1:])
        graph[p.stem] = edges

    def reachable(src, dst):
        stack, seen = [src], set()
        while stack:
            x = stack.pop()
            if x == dst:
                return True
            if x in seen:
                continue
            seen.add(x)
            stack.extend(graph.get(x, ()))
        return False

    added = {}
    for lib in order:
        for home in sorted(body_edges.get(lib, ())):
            if home == lib or home in graph.get(lib, ()):
                continue
            if home in policy.deps(lib).get("remove", ()):
                continue
            if reachable(lib, home):
                continue
            if reachable(home, lib):
                print("  body-dep {} -> {} skipped (would close a cycle)"
                      .format(lib, home))
                continue
            graph.setdefault(lib, set()).add(home)
            added.setdefault(lib, {})[home] = len(body_edges[lib][home])
    for lib, homes in added.items():
        p = deps_dir / (lib + ".deps")
        lines = sorted(bc.MODULE_PREFIX + "." + h for h in graph[lib])
        p.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
        print("  body-dep {}: +{} ({})".format(
            lib, len(homes), ", ".join(
                "{} ({} refs)".format(h, n)
                for h, n in sorted(homes.items()))))
    return added


# ---------------------------------------------------------------------------
# output
# ---------------------------------------------------------------------------

def _spell(qname, ns, namespaces):
    """Qualified name as written inside `export namespace <ns>`: a leading
    `::` is required when a namespace visible from the block shadows the first
    segment (inside boost::hana::ext::std, `boost::x` finds
    boost::hana::ext::boost)."""
    if qname.startswith("::"):
        return qname
    first = qname.split("::", 1)[0]
    for j in range(1, len(ns) + 1):
        if "::".join(ns[:j]) + "::" + first in namespaces:
            return "::" + qname
    return qname


def emit_inc(lib, records, out_dir, claimed, policy, extra_deps=(),
             namespaces=frozenset()):
    """Write generated/exports/<lib>.inc (+ <lib>.deps).

    Cross-module dedup (first wins): entities claimed by an earlier module are
    left out and their homes become `.deps` edges. One using-declaration per
    qualified name (it brings every overload). Policy guards, drops, inline-
    namespace collapses and verbatim lines are applied here, and every name
    they touch is returned for generated/meta/skipped.txt.

    Returns (exported_count, deps, skipped_entries)."""
    own = [r for r in records.values() if r["home"] == lib]
    pulled = [r for r in records.values()
              if r["home"] != lib and r["usr"] not in claimed]
    deps = {r["home"] for r in records.values()
            if r["home"] != lib and r["home"] != "shared"
            and r["usr"] in claimed} | set(extra_deps)
    dpol = policy.deps(lib)
    deps |= set(dpol.get("add", ()))
    deps -= set(dpol.get("remove", ()))
    deps = sorted(deps)

    collapse = policy.inline_namespaces(lib)
    drops = policy.drops(lib)
    guards = policy.guards(lib)
    skipped = []

    exports, seen = [], set()
    for rec in sorted(own + pulled, key=lambda r: r["qname"]):
        q, ns = rec["qname"], list(rec["ns"])
        for inl in collapse:
            if q.startswith(inl + "::"):
                q = inl.rsplit("::", 1)[0] + q[len(inl):]
                ns = inl.split("::")[:-1] + ns[len(inl.split("::")):]
        if q in seen:
            continue
        seen.add(q)
        rule = next((r for r in drops if _matches(q, r.get("names", ()))
                     or any(q.startswith(n + "::")
                            for n in r.get("namespaces", ()))), None)
        if rule is not None:
            skipped.append((lib, "drop", q, "", rule["reason"]))
            continue
        guard = next((g for g in guards if _matches(q, g["names"])), None)
        if guard is not None:
            skipped.append((lib, "guard", q, guard["if"], guard["reason"]))
        exports.append((tuple(ns), q, guard))

    groups = {}
    for ns, q, guard in exports:
        groups.setdefault(ns, []).append((q, guard))
    extras = {}
    for x in policy.extras(lib):
        extras.setdefault(tuple(x["namespace"].split("::")), []).append(x)
        skipped.append((lib, "extra", x["namespace"], "", x["reason"]))

    lines = [
        "// GENERATED by tools/gen_exports.py - DO NOT EDIT",
        "// lib={} boost={} target={} entities={}".format(
            lib, bc.BOOST_VERSION, bc.GEN_TARGET, len(exports)),
        "",
    ]
    for key in sorted(set(groups) | set(extras)):
        lines.append("export " + " ".join(
            "namespace {} {{".format(n) for n in key))
        for x in extras.get(key, ()):
            lines += _comment(x["reason"])
            lines += ["  " + ln for ln in x["lines"]]
        open_guard = None
        for q, guard in sorted(groups.get(key, ()), key=lambda e: e[0]):
            if guard is not open_guard:
                if open_guard is not None:
                    lines += _close_guard(open_guard)
                if guard is not None:
                    lines.append("#if " + guard["if"])
                    lines += _comment(guard["reason"])
                open_guard = guard
            lines.append("  using {};".format(_spell(q, key, namespaces)))
        if open_guard is not None:
            lines += _close_guard(open_guard)
        lines.append("}" * len(key))
        lines.append("")

    gen = out_dir / "generated" / "exports"
    gen.mkdir(parents=True, exist_ok=True)
    (gen / (lib + ".inc")).write_text("\n".join(lines), encoding="utf-8",
                                       newline="\n")
    dfile = gen / (lib + ".deps")
    if deps:
        dfile.write_text("\n".join(bc.MODULE_PREFIX + "." + d for d in deps)
                         + "\n", encoding="utf-8", newline="\n")
    elif dfile.exists():
        dfile.unlink()
    return len(exports), deps, skipped


def _comment(reason):
    out = []
    for para in reason.strip().splitlines():
        out.append("  // " + para.strip() if para.strip() else "  //")
    return out


def _close_guard(guard):
    out = []
    if guard.get("else"):
        out.append("#else")
        out += ["  " + ln for ln in guard["else"]]
    out.append("#endif")
    return out


def emit_cppm(lib, out_dir, gfm_headers):
    """Draft a facade for a library that has none yet (never overwrites)."""
    path = out_dir / "src" / (lib + ".cppm")
    if path.exists():
        return False
    deps = out_dir / "generated" / "exports" / (lib + ".deps")
    imports = [l.strip() for l in deps.read_text(encoding="utf-8").splitlines()
               if l.strip()] if deps.exists() else []
    lines = ["// {}.{} — module facade".format(bc.MODULE_PREFIX, lib),
             "module;"]
    lines += ["#include <{}>".format(h.relative_to(bc.DEPS).as_posix())
              for h in gfm_headers]
    lines += ["", "export module {}.{};".format(bc.MODULE_PREFIX, lib), ""]
    if imports:
        lines += ["export import {};".format(i) for i in imports] + [""]
    lines += ['#include "../generated/exports/{}.inc"'.format(lib), ""]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    return True


_GMF_INCLUDE = re.compile(r'^\s*#\s*include\s*<(boost/[^>]+)>', re.MULTILINE)


_EXPORT_IMPORT = re.compile(r'^\s*export\s+import\s+([\w.]+)\s*;', re.MULTILINE)


def check_facade(lib, gfm_headers, extra_includes=(), deps=(),
                 omit_imports=()):
    """Problems of src/<lib>.cppm against the generated data: its GMF must
    include exactly the bundle's header set (the export list was computed
    from it), plus the `facade-includes` the policy declares. Its `export
    import` block must cover every .deps edge: the .inc leaves out what an
    earlier module claimed, so an edge the facade does not re-export loses
    those names for consumers. An edge the facade cannot take (range ->
    regex: abi-tag mismatch under gcc) is declared in the policy's
    [<lib>.facade] omit-imports with a reason."""
    path = bc.ROOT / "src" / (lib + ".cppm")
    if not path.is_file():
        return ["src/{}.cppm is missing (run --emit-cppm)".format(lib)]
    text = path.read_text(encoding="utf-8")
    head, sep, body = text.partition("export module ")
    if not sep:
        return ["src/{}.cppm has no module declaration".format(lib)]
    have = set(_GMF_INCLUDE.findall(head))
    want = {h.relative_to(bc.DEPS).as_posix() for h in gfm_headers}
    problems = []
    for h in sorted(want - have):
        problems.append("src/{}.cppm GMF lacks #include <{}>".format(lib, h))
    for h in sorted(have - want - set(extra_includes)):
        problems.append("src/{}.cppm GMF includes <{}>, which the export "
                        "list was not generated from".format(lib, h))
    imported = set(_EXPORT_IMPORT.findall(body))
    omit = {bc.MODULE_PREFIX + "." + d for d in omit_imports}
    for dep in sorted(set(deps) - imported - omit):
        problems.append("src/{}.cppm lacks `export import {};` (a .deps edge; "
                        "declare it in [{}.facade] omit-imports if the facade "
                        "cannot take it)".format(lib, dep, lib))
    for dep in sorted(omit - set(deps)):
        problems.append("[{}.facade] omit-imports lists {}, which is not a "
                        ".deps edge".format(lib, dep))
    for dep in sorted(omit & imported):
        problems.append("[{}.facade] omit-imports lists {}, but src/{}.cppm "
                        "imports it".format(lib, dep, lib))
    return problems


def write_meta(out_dir, summary, skipped):
    """generated/meta/api-surface.json + skipped.txt (sorted, no paths, no
    timestamps — byte-stable for --check)."""
    meta = out_dir / "generated" / "meta"
    meta.mkdir(parents=True, exist_ok=True)
    surface = {
        "boost": bc.BOOST_VERSION,
        "generator": {
            "tool": "tools/gen_exports.py",
            "ast": "clang++ -ast-dump=json",
            "clang": aj.clang_release(),
            "target": bc.GEN_TARGET,
            "defines": bc.GEN_DEFINES,
        },
        "libraries": {lib: summary[lib] for lib in sorted(summary)},
    }
    (meta / "api-surface.json").write_text(
        json.dumps(surface, indent=1, sort_keys=True) + "\n",
        encoding="utf-8", newline="\n")
    lines = [
        "# Names the export policy (tools/export_policy.toml) guards, drops or",
        "# adds verbatim. GENERATED by tools/gen_exports.py - DO NOT EDIT",
        "# <lib> <action> <name> [if <condition>] -- <reason>",
        "",
    ]
    for lib, action, name, cond, reason in sorted(set(skipped)):
        cond = " ".join(cond.split())
        lines.append("{} {} {}{} -- {}".format(
            lib, action, name, " if " + cond if cond else "",
            " ".join(reason.split())))
    (meta / "skipped.txt").write_text("\n".join(lines) + "\n",
                                      encoding="utf-8", newline="\n")


# ---------------------------------------------------------------------------
# bundle TU: compile, prune, cache
# ---------------------------------------------------------------------------

def _bundle_text(gfm):
    return "".join("#include <{}>\n".format(h.relative_to(bc.DEPS).as_posix())
                   for h in gfm)


def _code_hash():
    h = hashlib.sha256()
    for name in ("gen_exports.py", "boost_common.py", "ast_json.py"):
        h.update((bc.TOOLS / name).read_bytes())
    return h.hexdigest()


_FILE_HASH = {}


def _file_hash(path):
    h = _FILE_HASH.get(path)
    if h is None:
        try:
            h = hashlib.sha256(path.read_bytes()).hexdigest()
        except OSError:
            h = "missing"
        _FILE_HASH[path] = h
    return h


def _closure_hash(gfm):
    """Content hash over every boost header transitively reachable from the
    include set (any header edit invalidates the caches)."""
    h = hashlib.sha256()
    seen = set()
    stack = list(gfm)
    while stack:
        f = stack.pop()
        key = str(f)
        if key in seen:
            continue
        seen.add(key)
        try:
            rel = f.relative_to(bc.DEPS).as_posix()
        except ValueError:
            rel = f.as_posix()
        h.update("{} {}\n".format(rel, _file_hash(f)).encode())
        try:
            text = f.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for mt in bc._INC_RE.finditer(text):
            p = bc.DEPS / mt.group(1)
            if str(p) not in seen and p.is_file():
                stack.append(p)
    return h.hexdigest()


def _input_manifest(tu, cwd):
    """{file: content hash} of every file clang read for the TU (its
    depfile), keyed relative to the repository when inside it. The bundle
    itself is left out: its text is part of the cache keys."""
    out = {}
    root = os.path.normcase(os.path.normpath(str(bc.ROOT)))
    bundles = Path(os.path.normpath(str(bc.CACHE_DIR / "bundles")))
    for raw in tu.inputs:
        f = Path(os.path.normpath(os.path.join(str(cwd), raw)))
        if f.parent == bundles:
            continue
        if os.path.normcase(str(f)).startswith(root + os.sep):
            key = f.relative_to(bc.ROOT).as_posix()
        else:
            key = f.as_posix()
        out[key] = _file_hash(f)
    return out


def _manifest_ok(manifest):
    """The files an entry was computed from still have the recorded content.
    This covers what the include-closure walk cannot see (relative and
    macro-computed includes, the sysroot); an entry without one is stale."""
    if not manifest:
        return False
    for key, h in manifest.items():
        f = Path(key)
        if not f.is_absolute():
            f = bc.ROOT / key
        if _file_hash(f) != h:
            return False
    return True


class _ClaimView:
    """A claim table seen through one library's processing: records the
    owner (None = unclaimed) of every name it probes or claims, as it was
    before the library touched it. A cache entry is valid only while those
    observations still hold (first-wins ownership depends on every library
    processed earlier, not only on include-graph upstreams)."""

    def __init__(self, table):
        self.table = table
        self.seen = {}

    def _see(self, k):
        if k not in self.seen:
            self.seen[k] = self.table.get(k)

    def __contains__(self, k):
        self._see(k)
        return k in self.table

    def __setitem__(self, k, v):
        self._see(k)
        self.table[k] = v

    def setdefault(self, k, v):
        self._see(k)
        return self.table.setdefault(k, v)


def _claims_hold(sk, claimed, claimed_inject):
    """The ownership an entry was computed under still holds: the same
    closure members are already claimed, and every probed injection name has
    the same owner."""
    pre = sk.get("pre_claimed")
    view = sk.get("inject_view")
    if pre is None or view is None:
        return False
    probes = {u for u, _ in sk["claims"]}
    if {u for u in probes if u in claimed} != set(pre):
        return False
    return all(claimed_inject.get(k) == o for k, o in view)


def _load_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError):
        return None


def _bundle_args(lib, policy):
    return [*bc.CLANG_ARGS, *("-D" + d for d in policy.defines(lib))]


def gate_key(lib, headers, policy):
    """Cache key of the compile gate for an include set: bundle text,
    header-closure hash, the driver line (with the policy's defines) and the
    compiler version. gen_audit.py keys its results on it too."""
    gh = hashlib.sha256()
    gh.update(_bundle_text(headers).encode())
    gh.update(_closure_hash(headers).encode())
    gh.update(" ".join(aj.clang_command(*_bundle_args(lib, policy))).encode())
    gh.update(aj.clang_version().encode())
    return gh.hexdigest()


def parse_bundle(lib, headers, policy, use_cache=True):
    """Compile the bundle TU once per attempt; the compile is the gate.

    A failing compile names the GMF headers that cannot be included directly
    on this platform (both boost/thread/pthread and win32 define heap_new;
    the aggregate selects one): those are pruned and the bundle recompiled,
    up to six rounds. The pruned include set is cached by bundle content,
    header-closure hash, flags and compiler version. Returns (TU, gfm) or
    None when the include set cannot be made to compile."""
    bundle = (bc.CACHE_DIR / "bundles" / (lib + ".cpp")).resolve()
    bundle.parent.mkdir(parents=True, exist_ok=True)
    gfm = list(headers)
    args = _bundle_args(lib, policy)
    gkey = gate_key(lib, gfm, policy)
    gate_cache = bc.CACHE_DIR / (lib + ".gate.json")
    cached = _load_json(gate_cache) if use_cache else None
    if cached is not None and cached.get("key") == gkey \
            and _manifest_ok(cached.get("inputs")):
        gfm = [bc.DEPS / rel for rel in cached["gfm"]]

    for attempt in range(7):
        bundle.write_text(_bundle_text(gfm), encoding="utf-8", newline="\n")
        try:
            tu = aj.dump(bundle, args, cwd=str(bc.ROOT))
            break
        except aj.DumpError as e:
            stderr = e.stderr
        offenders = set()
        for line in stderr.splitlines():
            head = re.split(r":\d+(?::\d+)?:", line, maxsplit=1)[0]
            head = head.strip().replace("\\", "/")
            if not head.endswith(".hpp"):
                continue
            for h in gfm:
                if head.endswith(h.relative_to(bc.DEPS).as_posix()):
                    offenders.add(h)
        if not offenders or attempt == 6:
            print("    clang++ FAILED for {}:".format(lib))
            for line in stderr.splitlines()[:8]:
                print("      {}".format(line.strip()))
            return None
        for h in sorted(offenders, key=lambda p: p.as_posix()):
            print("    gate pruned from GMF: {}".format(
                h.relative_to(bc.DEPS).as_posix()))
            gfm.remove(h)
    if use_cache:
        bc.CACHE_DIR.mkdir(parents=True, exist_ok=True)
        gate_cache.write_text(json.dumps(
            {"key": gkey,
             "gfm": [h.relative_to(bc.DEPS).as_posix() for h in gfm],
             "inputs": _input_manifest(tu, bc.ROOT)}),
            encoding="utf-8")
    return tu, gfm


# ---------------------------------------------------------------------------
# driver
# ---------------------------------------------------------------------------

GENERATED = ("generated/exports", "generated/meta/api-surface.json",
             "generated/meta/skipped.txt")


def _skip_key(lib, gfm, upstream_keys, no_body_deps, policy):
    h = hashlib.sha256()
    h.update(b"code:" + _code_hash().encode())
    h.update(b"bundle:" + _bundle_text(gfm).encode())
    h.update(b"closure:" + _closure_hash(gfm).encode())
    # The driver line, as the gate key hashes it: CXX and
    # CLANG_NO_DEFAULT_CONFIG change what clang parses.
    h.update(b"clang_args:" +
             " ".join(aj.clang_command(*bc.CLANG_ARGS)).encode())
    h.update(b"clang:" + aj.clang_version().encode())
    h.update(b"policy:" + policy.digest(lib).encode())
    h.update(b"no_body_deps:" + str(no_body_deps).encode())
    for u in sorted(upstream_keys):
        h.update("up:{}:{}\n".format(u, upstream_keys[u]).encode())
    return h.hexdigest()


def generate(args, out):
    """Run the generation into `out` (a repo root). Returns (rc, facade
    problems)."""
    global _FILE_TO_LIB
    policy = Policy.load()
    print("AST: {} (target {})".format(aj.clang_version(), bc.GEN_TARGET),
          flush=True)
    _FILE_TO_LIB = bc.build_file_to_lib()

    libs = list(args.libs) if args.libs else list(bc.TARGET_LIBS)
    for lib in libs:
        if lib not in bc.TARGET_LIBS:
            print("error: unknown library '{}'".format(lib), file=sys.stderr)
            return 1, []
    use_cache = not args.no_cache
    graph = bc.dep_graph()
    order = bc.topo_order(graph)
    print("processing order: {}".format(" -> ".join(order)))

    gfm_memo, lib_keys = {}, {}
    for lib in order:
        if lib in bc.LIBS_SPECIAL:
            continue
        uk = {}
        for u in sorted(graph.get(lib, ())):
            if u in bc.LIBS_SPECIAL:
                continue
            ku = lib_keys.get(u)
            if ku is None:
                uk = None
                break
            uk[u] = ku
        g = policy.gmf(lib)
        gfm_memo[lib] = [bc.DEPS / h for h in g] if g else bc.gfm_headers_of(lib)
        lib_keys[lib] = None if uk is None else _skip_key(
            lib, gfm_memo[lib], uk, args.no_body_deps, policy)

    old_surface = _load_json(out / "generated" / "meta" / "api-surface.json")
    old_libs = (old_surface or {}).get("libraries", {})
    old_skipped = _read_skipped(out / "generated" / "meta" / "skipped.txt")

    claimed, claimed_inject = {}, {}
    summary, skipped, body_all, gfm_by_lib, deps_by_lib = {}, [], {}, {}, {}
    cache_payloads = {}
    for lib in order:
        if lib in bc.LIBS_SPECIAL:
            continue
        key = lib_keys[lib]
        sk = _load_json(bc.CACHE_DIR / (lib + ".skip.json")) \
            if use_cache and key else None
        hit = sk is not None and sk.get("key") == key \
            and _manifest_ok(sk.get("inputs")) \
            and _claims_hold(sk, claimed, claimed_inject)
        if lib not in libs:
            # Not regenerated: its committed output stays, but its claims
            # shape first-wins ownership downstream, so replay them.
            if lib in old_libs:
                summary[lib] = old_libs[lib]
            skipped += [e for e in old_skipped if e[0] == lib]
            if hit:
                for usr, owner in sk["claims"]:
                    claimed.setdefault(usr, owner)
                for q, owner in sk["inject_claims"]:
                    claimed_inject.setdefault(q, owner)
            else:
                print("warning: no current cache for {}; a --libs run "
                      "may claim differently from a full run".format(lib))
            continue
        if hit and (out / "generated" / "exports" / (lib + ".inc")).exists():
            for usr, owner in sk["claims"]:
                claimed.setdefault(usr, owner)
            for q, owner in sk["inject_claims"]:
                claimed_inject.setdefault(q, owner)
            gfm_by_lib[lib] = [bc.DEPS / rel for rel in sk["gfm"]]
            summary[lib] = sk["summary"]
            deps_by_lib[lib] = sk["summary"]["deps"]
            skipped += [tuple(e) for e in sk["skipped"]]
            gen = out / "generated" / "exports"
            (gen / (lib + ".inc")).write_text(sk["inc"], encoding="utf-8",
                                              newline="\n")
            dfile = gen / (lib + ".deps")
            if sk["deps_text"]:
                dfile.write_text(sk["deps_text"], encoding="utf-8",
                                 newline="\n")
            elif dfile.exists():
                dfile.unlink()
            if sk["body"]:
                body_all[lib] = {h: set(v) for h, v in sk["body"].items()}
            print("skipped {} (cache hit, exported={})".format(
                lib, sk["summary"]["exported"]), flush=True)
            continue
        t0 = time.time()
        print("parsing {} ({} headers in GMF)...".format(
            lib, len(gfm_memo[lib])), flush=True)
        parsed = parse_bundle(lib, gfm_memo[lib], policy, use_cache)
        if parsed is None:
            print("error: parse failed for {}".format(lib), file=sys.stderr)
            return 1, []
        tu, gfm_final = parsed
        m = Model(tu)
        cands = collect_candidates(lib, m)
        inject_view = _ClaimView(claimed_inject)
        injects = collect_injections(lib, m, inject_view)
        added = collect_added(lib, m, policy, inject_view)
        records = closure_from(m, dict(cands, **injects, **added))
        pre_claimed = sorted({r["usr"] for r in records.values()
                              if r["usr"] in claimed})
        exports, deps, sk_entries = emit_inc(
            lib, records, out, claimed, policy,
            extra_deps=bc.lib_deps(lib, gfm_final),
            namespaces=m.namespaces())
        body = {} if args.no_body_deps else \
            collect_body_edges(lib, m, records, claimed)
        if body:
            body_all[lib] = body
        gfm_by_lib[lib] = gfm_final
        deps_by_lib[lib] = deps
        for rec in records.values():
            claimed.setdefault(rec["usr"], lib)
        summary[lib] = {
            "gmf": [h.relative_to(bc.DEPS).as_posix() for h in gfm_final],
            "exported": exports,
            "deps": deps,
        }
        skipped += sk_entries
        print("  candidates={} closure={} exported={} deps={} body-deps={} "
              "({:.1f}s)".format(len(cands), len(records), exports,
                                 deps or "-", ", ".join(sorted(body)) or "-",
                                 time.time() - t0), flush=True)
        if use_cache and key:
            gen = out / "generated" / "exports"
            dfile = gen / (lib + ".deps")
            cache_payloads[lib] = {
                "key": key,
                "gfm": summary[lib]["gmf"],
                "summary": dict(summary[lib]),
                "skipped": sk_entries,
                "inc": (gen / (lib + ".inc")).read_text(encoding="utf-8"),
                "deps_text": dfile.read_text(encoding="utf-8")
                if dfile.exists() else "",
                "body": {h: sorted(v) for h, v in body.items()},
                "claims": [[r["usr"], lib] for r in records.values()],
                "inject_claims": [[q, o] for q, o in claimed_inject.items()
                                  if o == lib],
                "pre_claimed": pre_claimed,
                "inject_view": sorted([k, o] for k, o in
                                      inject_view.seen.items()),
                "inputs": _input_manifest(tu, bc.ROOT),
            }
        del m, tu

    added = {}
    if not args.no_body_deps:
        print("pass 2: body-reference .deps completion...")
        added = merge_body_edges(out, order, body_all, policy)
        if not added:
            print("  no edges to add")
        for lib, homes in added.items():
            if lib in summary:
                summary[lib]["deps"] = sorted(set(summary[lib]["deps"]) |
                                              set(homes))
                deps_by_lib[lib] = summary[lib]["deps"]

    # The cache holds each library's pass-1 output and its pass-2 candidate
    # edges; pass 2 is re-run from them on every run, so its additions never
    # invalidate an entry.
    if use_cache and cache_payloads:
        bc.CACHE_DIR.mkdir(parents=True, exist_ok=True)
        for lib, payload in cache_payloads.items():
            (bc.CACHE_DIR / (lib + ".skip.json")).write_text(
                json.dumps(payload, sort_keys=True) + "\n", encoding="utf-8")

    # Hand-written modules (version, macros) have no bundle; their import
    # edges are declared in the policy so generated/ holds generated files only.
    for lib in bc.LIBS_SPECIAL:
        if args.libs and lib not in args.libs:
            continue
        add = sorted(policy.deps(lib).get("add", ()))
        dfile = out / "generated" / "exports" / (lib + ".deps")
        if add:
            dfile.parent.mkdir(parents=True, exist_ok=True)
            dfile.write_text("".join(bc.MODULE_PREFIX + "." + d + "\n"
                                     for d in add), encoding="utf-8",
                             newline="\n")
        elif dfile.exists():
            dfile.unlink()

    write_meta(out, summary, skipped)

    if args.emit_cppm:
        for lib in order:
            if lib in gfm_by_lib and emit_cppm(lib, out, gfm_by_lib[lib]):
                print("drafted src/{}.cppm".format(lib))

    problems = []
    for lib in order:
        if lib in gfm_by_lib:
            dfile = out / "generated" / "exports" / (lib + ".deps")
            deps = dfile.read_text(encoding="utf-8").split() \
                if dfile.exists() else []
            problems += check_facade(lib, gfm_by_lib[lib],
                                     policy.facade_includes(lib), deps,
                                     policy.facade_omit_imports(lib))
    report = bc.ROOT / "target" / "gen" / "gen_report.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps(summary, indent=1, sort_keys=True) + "\n",
                      encoding="utf-8")
    return 0, problems


def _read_skipped(path):
    """Committed skipped.txt entries (kept for libraries not regenerated)."""
    out = []
    if not path.is_file():
        return out
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        head, _, reason = line.partition(" -- ")
        parts = head.split(" ", 3)
        if len(parts) < 3:
            continue
        lib, action, name = parts[:3]
        cond = ""
        if len(parts) == 4 and parts[3].startswith("if "):
            cond = parts[3][3:]
        out.append((lib, action, name, cond, reason))
    return out


def _snapshot(root):
    """{relative path: bytes} of every generated file below `root`."""
    files = {}
    for rel in GENERATED:
        p = root / rel
        if p.is_dir():
            for f in sorted(p.iterdir()):
                if f.is_file():
                    files[f.relative_to(root).as_posix()] = f.read_bytes()
        elif p.is_file():
            files[rel] = p.read_bytes()
    return files


def check(args):
    """Regenerate into a scratch copy and compare byte for byte; the
    repository is never written."""
    with tempfile.TemporaryDirectory(prefix="gen-exports-check-") as tmp:
        tmp = Path(tmp)
        for rel in GENERATED:
            src = bc.ROOT / rel
            if src.is_dir():
                shutil.copytree(src, tmp / rel)
            elif src.is_file():
                (tmp / rel).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, tmp / rel)
        rc, problems = generate(args, tmp)
        if rc:
            return rc
        want = _snapshot(tmp)
        have = _snapshot(bc.ROOT)
    stale = sorted(p for p in set(want) | set(have)
                   if want.get(p) != have.get(p))
    for p in stale:
        if p not in want:
            print("stale: {} (the generator no longer writes it)".format(p))
            continue
        if p not in have:
            print("missing: {}".format(p))
            continue
        diff = difflib.unified_diff(
            have[p].decode("utf-8", "replace").splitlines(),
            want[p].decode("utf-8", "replace").splitlines(),
            "a/" + p, "b/" + p, lineterm="", n=1)
        out = list(diff)
        print("\n".join(out[:40]))
        if len(out) > 40:
            print("... ({} more diff lines)".format(len(out) - 40))
    for line in problems:
        print("facade: " + line)
    if stale or problems:
        print("\ngen_exports --check: {} generated file(s) differ, {} facade "
              "problem(s); run tools/gen_exports.py and review the diff"
              .format(len(stale), len(problems)), file=sys.stderr)
        return 1
    print("gen_exports --check: generated exports are up to date")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--scan", action="store_true",
                    help="regenerate generated/meta/libs.json and exit")
    ap.add_argument("--libs", nargs="*", default=None,
                    help="subset of target libraries (default: all)")
    ap.add_argument("--check", action="store_true",
                    help="compare the committed output byte for byte with a "
                         "fresh generation; write nothing")
    ap.add_argument("--emit-cppm", action="store_true",
                    help="draft src/<lib>.cppm for libraries without a facade")
    ap.add_argument("--no-body-deps", action="store_true",
                    help="disable the pass-2 body-reference .deps completion")
    ap.add_argument("--no-cache", action="store_true",
                    help="ignore and do not write the generator caches")
    args = ap.parse_args()

    if args.scan:
        db = bc.scan_libs_json()
        bc.LIBS_JSON.write_text(json.dumps(db, indent=1, sort_keys=True) + "\n",
                                encoding="utf-8", newline="\n")
        print("wrote {} ({} headers, {} libraries)".format(
            bc.LIBS_JSON, sum(len(v) for v in db.values()), len(db)))
        return 0
    if args.check:
        if args.emit_cppm:
            ap.error("--check writes nothing; drop --emit-cppm")
        return check(args)
    rc, problems = generate(args, bc.ROOT)
    for line in problems:
        print("facade: " + line)
    if problems:
        print("error: {} facade problem(s)".format(len(problems)),
              file=sys.stderr)
        return 1
    return rc


if __name__ == "__main__":
    sys.exit(main())
