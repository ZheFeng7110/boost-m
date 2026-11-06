#!/usr/bin/env python3
"""ast_json.py — the compiler's own AST as the generator's only input.

The export lists are computed from what clang itself reports about a bundle
translation unit, never from a re-implementation of C++ parsing:

    clang++ <flags> -fsyntax-only -Xclang -ast-dump=json \
            -Xclang -ast-dump-filter=boost bundle.cpp

`-ast-dump-filter=boost` keeps the declarations whose qualified name contains
`boost` (every `namespace boost` reopening is dumped whole and nothing of the
std/SDK headers around it), which is everything the module layer exports.
The same invocation is the syntax gate: its exit status and diagnostics decide
whether the include set compiles, so the gate and the AST can never disagree.

The dump is still large (an instantiation-heavy library such as geometry
prints several GB), so it is consumed as a stream: clang's JSON writer
pretty-prints one key per line with a two-space indent, and the reader below
turns that line stream into a flat declaration table without materializing
the tree. While streaming it

  * tracks the delta-encoded `loc.file` / `loc.line` (clang prints a file only
    when it differs from the previously printed location, so the state is
    carried across every location in output order, skipped ones included);
  * drops implicit template instantiations (every declaration in a
    `*TemplateDecl` after the template pattern), `UsingShadowDecl` target
    copies, comments, source ranges and record definition data;
  * attributes the references found in a declaration's subtree (declaration
    references, lookups, type-node declarations, written types) to the
    namespace-scope entity that owns the subtree.

Only `clang++` is required — no libclang, no Python bindings.
"""

import os
import re
import shutil
import subprocess


# ---------------------------------------------------------------------------
# compiler invocation
# ---------------------------------------------------------------------------

def clang_command(*args):
    """clang++ command line. CXX selects the driver; CLANG_NO_DEFAULT_CONFIG
    drops the host config file xlings' LLVM installs for native builds (the
    generator parses for an explicit cross target and sysroot)."""
    command = [os.environ.get("CXX") or "clang++"]
    if os.environ.get("CLANG_NO_DEFAULT_CONFIG"):
        command.append("--no-default-config")
    return command + list(args)


_VERSION = None


def clang_version():
    """First line of `clang++ --version` — the generator identity recorded in
    caches and in the committed API surface."""
    global _VERSION
    if _VERSION is None:
        exe = shutil.which(clang_command()[0])
        if exe is None:
            raise RuntimeError("clang++ not found (set CXX or add LLVM to PATH)")
        r = subprocess.run(clang_command("--version"), capture_output=True,
                           text=True)
        out = (r.stdout or r.stderr).splitlines()
        _VERSION = out[0].strip() if out else "unknown"
    return _VERSION


def clang_release():
    """'clang version 23.1.3 (...)' -> '23.1.3' (stable across builds)."""
    m = re.search(r"version\s+(\d+(?:\.\d+)*)", clang_version())
    return m.group(1) if m else clang_version()


class DumpError(Exception):
    """The bundle did not compile; `stderr` holds the diagnostics."""

    def __init__(self, returncode, stderr):
        super().__init__("clang++ exited with {}".format(returncode))
        self.returncode = returncode
        self.stderr = stderr


def dump(source, args, cwd=None):
    """Compile `source` once and return its declaration table (`TU`).

    Raises DumpError with the compiler's diagnostics when the translation unit
    does not compile — the caller decides whether to prune includes and retry.
    The AST stream is consumed while clang writes it; stderr goes to a
    temporary file so a chatty failure cannot block the pipe."""
    import tempfile
    fd, depfile = tempfile.mkstemp(suffix=".d")
    os.close(fd)
    cmd = clang_command("-fsyntax-only", *args, "-MD", "-MF", depfile,
                        "-Xclang", "-ast-dump=json",
                        "-Xclang", "-ast-dump-filter=boost", str(source))
    try:
        with tempfile.TemporaryFile() as err:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=err,
                                    cwd=cwd, bufsize=1 << 20)
            try:
                tu = TU.from_stream(proc.stdout)
            finally:
                proc.stdout.close()
                rc = proc.wait()
            err.seek(0)
            stderr = err.read().decode("utf-8", errors="replace")
        if rc != 0:
            raise DumpError(rc, stderr)
        with open(depfile, encoding="utf-8", errors="surrogateescape") as f:
            tu.inputs = parse_depfile(f.read())
    finally:
        # clang removes the dependency file itself when the compile fails
        if os.path.exists(depfile):
            os.unlink(depfile)
    return tu


def parse_depfile(text):
    """Prerequisites of a Make-style dependency file (`clang -MD`): the
    files after the first `target: `, with backslash-newline continuations
    and the `\\ ` / `\\#` / `$$` escapes undone. Other backslashes are
    path separators (Windows) and stay."""
    text = re.sub(r"\\\r?\n", " ", text)
    head, sep, rest = text.partition(": ")
    if not sep:
        return []
    out, cur, i, n = [], [], 0, len(rest)
    while i < n:
        c = rest[i]
        if c == "\\" and i + 1 < n and rest[i + 1] in " #":
            cur.append(rest[i + 1])
            i += 2
            continue
        if c == "$" and i + 1 < n and rest[i + 1] == "$":
            cur.append("$")
            i += 2
            continue
        if c.isspace():
            if cur:
                out.append("".join(cur))
                cur = []
        else:
            cur.append(c)
        i += 1
    if cur:
        out.append("".join(cur))
    return out


# ---------------------------------------------------------------------------
# declaration table
# ---------------------------------------------------------------------------

# Declarations that open a scope other entities can be declared in.
CONTEXT_KINDS = frozenset((
    "NamespaceDecl", "LinkageSpecDecl", "CXXRecordDecl", "RecordDecl",
    "ClassTemplateDecl", "ClassTemplatePartialSpecializationDecl",
    "ClassTemplateSpecializationDecl", "EnumDecl", "ExportDecl",
))
RECORD_KINDS = frozenset((
    "CXXRecordDecl", "RecordDecl", "ClassTemplateSpecializationDecl",
    "ClassTemplatePartialSpecializationDecl",
))
TEMPLATE_KINDS = frozenset((
    "ClassTemplateDecl", "FunctionTemplateDecl", "VarTemplateDecl",
    "TypeAliasTemplateDecl",
))
TEMPLATE_PARAM_KINDS = frozenset((
    "TemplateTypeParmDecl", "NonTypeTemplateParmDecl",
    "TemplateTemplateParmDecl",
))
FUNCTION_KINDS = frozenset((
    "FunctionDecl", "CXXMethodDecl", "CXXConstructorDecl",
    "CXXDestructorDecl", "CXXConversionDecl", "CXXDeductionGuideDecl",
))
# Declarations whose `type` is spelled in the source (the stand-in for
# libclang's TYPE_REF / TEMPLATE_REF cursors, which JSON does not print).
WRITTEN_TYPE_KINDS = frozenset((
    "VarDecl", "ParmVarDecl", "FieldDecl", "TypedefDecl", "TypeAliasDecl",
    "CXXStaticCastExpr", "CXXDynamicCastExpr", "CXXReinterpretCastExpr",
    "CXXConstCastExpr", "CStyleCastExpr", "CXXFunctionalCastExpr",
    "CXXTemporaryObjectExpr", "CXXUnresolvedConstructExpr",
    "NonTypeTemplateParmDecl",
)) | FUNCTION_KINDS

_SCALAR_ATTRS = frozenset((
    "name", "mangledName", "storageClass", "inline", "constexpr",
    "isImplicit", "previousDecl", "parentDeclContextId", "completeDefinition",
    "tagUsed", "scopedEnumTag", "init", "isInline", "explicitlyDeleted",
    "explicitlyDefaulted", "fixedUnderlyingType", "templateName", "argType",
    "referencedMemberDecl", "language",
))


class Decl:
    """One declaration node in a declaration context (namespace / class /
    linkage specification / translation unit). Declarations inside function
    bodies are not recorded; their references belong to the owning entity."""

    __slots__ = ("id", "kind", "name", "parent", "file", "line", "attrs",
                 "type", "dtype", "children", "params", "bases", "refs",
                 "written", "has_body", "targs", "pattern", "friend",
                 "target", "seq", "_qn", "_ns")

    def __init__(self, nid, kind):
        self.id = nid
        self.seq = 0            # pre-order position in the dump
        self.kind = kind
        self.name = ""
        self.parent = None
        self.file = ""
        self.line = 0
        self.attrs = {}
        self.type = ""          # type.qualType
        self.dtype = ""         # type.desugaredQualType
        self.children = []
        self.params = []        # ParmVarDecl type strings (direct children)
        self.bases = []         # base-specifier type strings
        self.refs = None        # entity owner only: referenced decl ids
        self.written = None     # entity owner only: written type strings
        self.has_body = False
        self.targs = False      # carries template arguments (specialization)
        self.pattern = None     # *TemplateDecl -> templated declaration
        self.friend = False     # declared through a friend declaration
        self.target = ""        # UsingShadowDecl target / UsingDirective ns
        self._qn = None
        self._ns = None

    def __repr__(self):
        return "<{} {} {}>".format(self.kind, self.name, self.id)


class TU:
    """Flat declaration table of one translation unit."""

    def __init__(self):
        self.decls = []          # every recorded Decl, in close order
        self.by_id = {}          # id -> Decl (first occurrence wins)
        # id of every node inside a dropped implicit instantiation -> id of
        # the template whose instantiation it belongs to (references to
        # `foo<int>` and its members resolve to the template `foo`).
        self.alias = {}
        # every file clang read for this TU (its -MD dependency list), as
        # printed: absolute, or relative to the compile's working directory
        self.inputs = []

    def resolve(self, nid):
        """Decl for a referenced id, following instantiation aliases."""
        d = self.by_id.get(nid)
        if d is None:
            t = self.alias.get(nid)
            if t is not None:
                d = self.by_id.get(t)
        return d

    # -- construction ------------------------------------------------------

    @classmethod
    def from_stream(cls, stream):
        tu = cls()
        _Reader(tu).run(stream)
        return tu

    @classmethod
    def from_file(cls, path):
        with open(path, "rb", buffering=1 << 20) as f:
            return cls.from_stream(f)


class _Reader:
    """Line-oriented reader for clang's pretty-printed JSON AST.

    Frame layout (lists for speed): [indent, tag, node, extra]
      tag 'N'  AST node object (node = _Node)
      tag 'A'  array; extra = owning _Node, node = array key
      tag 'O'  nested object; node = owning _Node, extra = object key
    """

    def __init__(self, tu):
        self.tu = tu

    def run(self, stream):
        tu = self.tu
        frames = []
        cur_file = ""
        cur_line = 0
        ignore_file = False
        skip = -1
        loc_owner = None
        pending = None
        pend_ind = 0
        intern = {}
        seq = 0
        alias = tu.alias
        alias_to = None         # template id while skipping an instantiation
        after_open = False
        skip_arrays = []        # (indent, key) of arrays open in the skip

        for raw in stream:
            s = raw.lstrip(b" ")
            ind = len(raw) - len(s)
            # location state: every printed location, skipped ones included
            if s.startswith(b'"file": "'):
                if ignore_file:
                    ignore_file = False
                else:
                    cur_file = _unquote(s[8:])
            elif s.startswith(b'"line": '):
                cur_line = int(s[8:].rstrip(b",\r\n"))
            elif s.startswith(b'"includedFrom": {'):
                ignore_file = True
            if skip >= 0:
                if alias_to is not None:
                    # Only nodes of the instantiation (elements of "inner")
                    # alias to the template; elements of other arrays
                    # (lookups, ...) name declarations it merely refers to.
                    if after_open and s.startswith(b'"id": "'):
                        alias[_unquote(s[6:])] = alias_to
                    c = s[:1]
                    if c == b'"' and s.rstrip().endswith(b"["):
                        skip_arrays.append((ind, s[1:s.index(b'"', 1)]))
                    elif c == b"]" and skip_arrays \
                            and skip_arrays[-1][0] == ind:
                        skip_arrays.pop()
                    after_open = c == b"{" and bool(skip_arrays) \
                        and skip_arrays[-1][1] == b"inner"
                if ind == skip and s[:1] in (b"}", b"]"):
                    skip = -1
                    alias_to = None
                    skip_arrays.clear()
                    if loc_owner is not None:
                        f = intern.get(cur_file)
                        if f is None:
                            f = intern[cur_file] = cur_file
                        loc_owner.file = f
                        loc_owner.line = cur_line
                        loc_owner = None
                continue
            if pending is not None:
                # buffered node head: decide on the kind before descending
                if s.startswith(b'"kind": "'):
                    kind = _unquote(s[8:])
                    arr = frames[-1]
                    owner = arr[2]
                    # Top level: only `namespace boost` subtrees. The filter
                    # also matches std-side declarations whose printed name
                    # mentions boost (std::hash<boost::...> members, ...);
                    # those arrive without their context and are not ours.
                    drop = kind == "FullComment" or (
                        owner is None and kind != "NamespaceDecl")
                    if owner is not None and owner.kind in TEMPLATE_KINDS \
                            and arr[3] == b"inner" \
                            and kind not in TEMPLATE_PARAM_KINDS \
                            and kind != "TemplateArgument":
                        if owner.npat:
                            drop = True         # implicit instantiation
                        else:
                            owner.npat = 1
                    if drop:
                        skip = pend_ind
                        if owner is not None and kind != "FullComment":
                            alias_to = owner.id
                            if pending:
                                alias[pending] = owner.id
                            after_open = False
                        pending = None
                        continue
                    node = _Node(pending, kind, owner)
                    seq += 1
                    node.seq = seq
                    frames.append([pend_ind, "N", node, None])
                    pending = None
                elif s.startswith(b'"id": "'):
                    pending = _unquote(s[6:])
                continue
            c = s[:1]
            if c == b"}" or c == b"]":
                fr = frames.pop()
                if fr[1] == "N":
                    fr[2].close(tu)
                continue
            if c == b"{":
                if s[1:2] == b"}":
                    continue            # null child (`{}`)
                top = frames[-1] if frames else None
                if top is None or (top[1] == "A" and top[3] == b"inner"):
                    pending = ""
                    pend_ind = ind
                    if top is None:
                        frames.append([-1, "A", None, b"inner"])
                    continue
                # element of another array (lookups, bases, ...)
                frames.append([ind, "O", top[2], top[3]])
                continue
            if c != b'"':
                continue
            k_end = s.index(b'"', 1)
            key = s[1:k_end]
            rest = s[k_end + 3:]
            top = frames[-1]
            r0 = rest[:1]
            if r0 == b"{":
                if rest[1:2] == b"}":
                    continue
                if key == b"loc":
                    skip = ind
                    loc_owner = top[2] if top[1] == "N" else None
                    continue
                if key in (b"range", b"definitionData"):
                    skip = ind
                    continue
                owner = top[2]
                if top[1] == "O" and top[3] == b"bases" and key == b"type":
                    frames.append([ind, "O", owner, b"bases.type"])
                else:
                    frames.append([ind, "O", owner,
                                   key if top[1] == "N" else b"-"])
                continue
            if r0 == b"[":
                if rest[1:2] == b"]":
                    continue
                if top[1] != "N":
                    frames.append([ind, "A", None, b"-"])
                    continue
                node = top[2]
                if key == b"inner" and node.kind == "UsingShadowDecl":
                    skip = ind
                    continue
                frames.append([ind, "A", node, key])
                continue
            # scalar
            tag = top[1]
            if tag == "N":
                node = top[2]
                k = key.decode()
                if k in _SCALAR_ATTRS:
                    node.attrs[k] = _scalar(rest)
                continue
            if tag == "O":
                node = top[2]
                if node is None:
                    continue
                okey = top[3]
                if okey == b"type":
                    if key == b"qualType":
                        node.type = _scalar(rest)
                    elif key == b"desugaredQualType":
                        node.dtype = _scalar(rest)
                elif okey == b"bases.type":
                    if key == b"qualType":
                        node.bases.append(_scalar(rest))
                elif key == b"id" and okey in (b"referencedDecl", b"decl",
                                               b"lookups", b"target",
                                               b"nominatedNamespace"):
                    node.refs.append(_scalar(rest))
        return tu


class _Node:
    """Transient node while its subtree is being read."""

    __slots__ = ("id", "kind", "parent", "attrs", "type", "dtype", "bases",
                 "refs", "file", "line", "npat", "decl", "params", "children",
                 "has_body", "targs", "written", "seq")

    def __init__(self, nid, kind, parent):
        self.id = nid
        self.kind = kind
        self.parent = parent
        self.attrs = {}
        self.type = ""
        self.dtype = ""
        self.bases = []
        self.refs = []
        self.file = parent.file if parent is not None else ""
        self.line = parent.line if parent is not None else 0
        self.npat = 0
        self.decl = None
        self.params = []
        self.children = []
        self.has_body = False
        self.targs = False
        self.written = []
        self.seq = 0

    def close(self, tu):
        """Fold this node into its parent (or into the declaration table)."""
        kind = self.kind
        parent = self.parent
        rec = kind.endswith("Decl") and _in_context(self)
        if rec:
            d = Decl(self.id, kind)
            d.seq = self.seq
            d.name = self.attrs.get("name", "")
            d.attrs = self.attrs
            d.type = self.type
            d.dtype = self.dtype
            d.file = self.file
            d.line = self.line
            d.params = self.params
            d.bases = self.bases
            d.has_body = self.has_body
            d.targs = self.targs
            d.children = self.children
            d.friend = _under_friend(self)
            if kind in ("UsingShadowDecl", "UsingDirectiveDecl"):
                d.target = self.refs[0] if self.refs else ""
            for ch in d.children:
                ch.parent = d
                if d.kind in TEMPLATE_KINDS and d.pattern is None \
                        and ch.kind not in TEMPLATE_PARAM_KINDS:
                    d.pattern = ch
            if _is_entity_scope(self):
                refs = set(self.refs)
                written = set(self.written)
                if self.type and kind in WRITTEN_TYPE_KINDS:
                    written.add(self.type)
                written.update(self.bases)
                d.refs = refs
                d.written = written
            self.decl = d
            tu.decls.append(d)
            if d.id and d.id not in tu.by_id:
                tu.by_id[d.id] = d
        if parent is None:
            return
        # ---- fold into the parent node ----
        if rec:
            parent.children.append(self.decl)
        if kind == "ParmVarDecl" and parent.kind in FUNCTION_KINDS:
            parent.params.append(self.type)
        elif kind == "CompoundStmt" and parent.kind in FUNCTION_KINDS:
            parent.has_body = True
        elif kind == "TemplateArgument" and parent.kind in FUNCTION_KINDS:
            parent.targs = True
        if rec and _is_entity_scope(self):
            return          # entity owner keeps its own references
        if kind in ("NamespaceDecl", "LinkageSpecDecl"):
            return          # namespaces own no references
        parent.refs.extend(self.refs)
        if self.written:
            parent.written.extend(self.written)
        if self.type and kind in WRITTEN_TYPE_KINDS:
            parent.written.append(self.type)
        if self.bases:
            parent.written.extend(self.bases)
        tname = self.attrs.get("templateName")
        if tname:
            parent.written.append(tname)
        at = self.attrs.get("argType")
        if at:
            parent.written.append(at)
        mem = self.attrs.get("referencedMemberDecl")
        if mem:
            parent.refs.append(mem)
        # members keep their own scalars on the Decl; nothing else to fold


def _in_context(node):
    """True when every ancestor is a declaration context (namespace, class,
    linkage specification, template wrapper, friend) — i.e. the declaration is
    not local to a function body, an expression or a type."""
    p = node.parent
    while p is not None:
        k = p.kind
        if k in CONTEXT_KINDS or k in TEMPLATE_KINDS or k == "FriendDecl":
            p = p.parent
            continue
        return False
    return True


def _under_friend(node):
    p = node.parent
    while p is not None:
        if p.kind == "FriendDecl":
            return True
        if p.kind in ("NamespaceDecl", "LinkageSpecDecl"):
            return False
        p = p.parent
    return False


def _is_entity_scope(node):
    """The node is a namespace-scope declaration (directly in a namespace,
    linkage specification or the translation unit) — the unit references are
    attributed to, like libclang's per-entity cursor walk."""
    if node.kind in ("NamespaceDecl", "LinkageSpecDecl"):
        return False
    p = node.parent
    return p is None or p.kind in ("NamespaceDecl", "LinkageSpecDecl",
                                   "ExportDecl")


def _unquote(b):
    """b'"text",\\n' -> 'text' (JSON string escapes decoded)."""
    b = b.rstrip(b",\r\n")
    if b"\\" in b:
        import json
        return json.loads(b)
    return b[1:-1].decode("utf-8", errors="replace")


def _scalar(b):
    b = b.rstrip(b",\r\n")
    c = b[:1]
    if c == b'"':
        if b"\\" in b:
            import json
            return json.loads(b)
        return b[1:-1].decode("utf-8", errors="replace")
    if b == b"true":
        return True
    if b == b"false":
        return False
    try:
        return int(b)
    except ValueError:
        return b.decode()
