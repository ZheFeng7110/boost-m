"""The streaming reader of clang's JSON AST (tools/ast_json.py)."""
import io
import json
import unittest

import ast_json


def stream(*objs):
    """clang prints one pretty-printed object per filter match, indent 2;
    json.dumps(indent=2) produces the same line layout."""
    text = "".join(json.dumps(o, indent=2) + "\n" for o in objs)
    return io.BytesIO(text.encode())


def loc(file=None, line=None, included_from=None):
    out = {"offset": 1}
    if file is not None:
        out["file"] = file
    if line is not None:
        out["line"] = line
    out["col"] = 1
    if included_from is not None:
        out["includedFrom"] = {"file": included_from}
    return out


def ns(nid, name, inner, l=None):
    return {"id": nid, "kind": "NamespaceDecl", "loc": l or loc(),
            "range": {"begin": loc(), "end": loc()}, "name": name,
            "inner": inner}


class ReaderTests(unittest.TestCase):
    def test_file_is_delta_encoded_across_locations(self):
        tu = ast_json.TU.from_stream(stream(ns("0x1", "boost", [
            {"id": "0x2", "kind": "CXXRecordDecl",
             "loc": loc("/b/a.hpp", 3, included_from="/b/x.hpp"),
             "range": {"begin": loc(), "end": loc()}, "name": "a",
             "tagUsed": "struct"},
            # no "file": same file as the previously printed location, and
            # includedFrom's file must not count as one
            {"id": "0x3", "kind": "CXXRecordDecl",
             "loc": loc(line=9, included_from="/b/other.hpp"),
             "name": "b", "tagUsed": "struct"},
            # a file printed inside a (dropped) range still moves the state
            {"id": "0x4", "kind": "TypedefDecl", "loc": loc(line=1),
             "range": {"begin": loc("/b/c.hpp", 2), "end": loc()},
             "name": "c", "type": {"qualType": "int"}},
            {"id": "0x5", "kind": "TypedefDecl", "loc": loc(line=4),
             "name": "d", "type": {"qualType": "int"}},
        ], loc("/b/a.hpp", 1))))
        files = {d.name: (d.file, d.line) for d in tu.decls}
        self.assertEqual(files["a"], ("/b/a.hpp", 3))
        self.assertEqual(files["b"], ("/b/a.hpp", 9))
        self.assertEqual(files["c"], ("/b/a.hpp", 1))
        self.assertEqual(files["d"], ("/b/c.hpp", 4))

    def test_macro_location_uses_the_expansion_file(self):
        tu = ast_json.TU.from_stream(stream(ns("0x1", "boost", [
            {"id": "0x2", "kind": "VarDecl",
             "loc": {"spellingLoc": loc("/b/macro.hpp", 5),
                     "expansionLoc": loc("/b/user.hpp", 7)},
             "name": "v", "type": {"qualType": "int"}},
        ], loc("/b/user.hpp", 1))))
        v = next(d for d in tu.decls if d.name == "v")
        self.assertEqual((v.file, v.line), ("/b/user.hpp", 7))

    def test_implicit_instantiations_are_dropped_and_aliased(self):
        tu = ast_json.TU.from_stream(stream(ns("0x1", "boost", [
            {"id": "0x10", "kind": "ClassTemplateDecl", "loc": loc("/b/t.hpp", 1),
             "name": "box", "inner": [
                 {"id": "0x11", "kind": "TemplateTypeParmDecl", "name": "T"},
                 {"id": "0x12", "kind": "CXXRecordDecl", "name": "box",
                  "tagUsed": "struct", "completeDefinition": True},
                 {"id": "0x13", "kind": "ClassTemplateSpecializationDecl",
                  "loc": loc("/b/elsewhere.hpp", 8), "name": "box",
                  "inner": [{"id": "0x14", "kind": "CXXMethodDecl",
                             "name": "get"}]},
             ]},
            {"id": "0x20", "kind": "FunctionDecl", "loc": loc(line=30),
             "name": "use", "type": {"qualType": "void ()"}, "inner": [
                 {"id": "0x21", "kind": "CompoundStmt", "inner": [
                     {"id": "0x22", "kind": "DeclRefExpr",
                      "referencedDecl": {"id": "0x14", "kind": "CXXMethodDecl",
                                         "name": "get"}},
                 ]},
             ]},
        ])))
        kinds = [d.kind for d in tu.decls]
        self.assertNotIn("ClassTemplateSpecializationDecl", kinds)
        tmpl = tu.by_id["0x10"]
        self.assertEqual(tmpl.pattern.id, "0x12")
        self.assertIs(tu.resolve("0x13"), tmpl)
        self.assertIs(tu.resolve("0x14"), tmpl)
        use = tu.by_id["0x20"]
        self.assertTrue(use.has_body)
        self.assertIn("0x14", use.refs)
        # the skipped instantiation printed a file: clang's delta state moved,
        # and the reader followed it
        self.assertEqual(use.file, "/b/elsewhere.hpp")

    def test_lookups_inside_an_instantiation_are_not_aliased(self):
        # An UnresolvedLookupExpr in the instantiated body lists the
        # declarations it may call (std::swap); those are not part of the
        # instantiation and must keep resolving to themselves.
        tu = ast_json.TU.from_stream(stream(ns("0x1", "boost", [
            {"id": "0x10", "kind": "ClassTemplateDecl", "loc": loc("/b/t.hpp", 1),
             "name": "box", "inner": [
                 {"id": "0x11", "kind": "TemplateTypeParmDecl", "name": "T"},
                 {"id": "0x12", "kind": "CXXRecordDecl", "name": "box",
                  "tagUsed": "struct", "completeDefinition": True},
                 {"id": "0x13", "kind": "ClassTemplateSpecializationDecl",
                  "name": "box", "inner": [
                      {"id": "0x14", "kind": "CXXMethodDecl", "name": "swap",
                       "inner": [{"id": "0x15", "kind": "CompoundStmt",
                                  "inner": [{
                                      "id": "0x16",
                                      "kind": "UnresolvedLookupExpr",
                                      "lookups": [
                                          {"id": "0x99",
                                           "kind": "FunctionTemplateDecl",
                                           "name": "swap"}],
                                      "inner": [{"id": "0x17",
                                                 "kind": "DeclRefExpr"}]}]}]},
                  ]},
             ]},
        ])))
        tmpl = tu.by_id["0x10"]
        for nid in ("0x13", "0x14", "0x15", "0x16", "0x17"):
            self.assertIs(tu.resolve(nid), tmpl, nid)
        self.assertNotIn("0x99", tu.alias)

    def test_null_children_and_foreign_top_level_objects(self):
        tu = ast_json.TU.from_stream(stream(
            {"id": "0x90", "kind": "CXXRecordDecl", "loc": loc("/std/x.h", 1),
             "name": "hash"},
            ns("0x1", "boost", [
                {},
                {"id": "0x2", "kind": "EnumDecl", "loc": loc("/b/e.hpp", 2),
                 "name": "color", "inner": [
                     {"id": "0x3", "kind": "EnumConstantDecl", "name": "red",
                      "type": {"qualType": "boost::color"}},
                 ]},
            ])))
        names = sorted(d.name for d in tu.decls)
        self.assertEqual(names, ["boost", "color", "red"])
        red = tu.by_id["0x3"]
        self.assertIs(red.parent, tu.by_id["0x2"])
        self.assertIs(tu.by_id["0x2"].parent, tu.by_id["0x1"])

    def test_signature_and_using_details(self):
        tu = ast_json.TU.from_stream(stream(ns("0x1", "boost", [
            {"id": "0x2", "kind": "CXXRecordDecl", "loc": loc("/b/r.hpp", 1),
             "name": "r", "tagUsed": "struct", "completeDefinition": True,
             "bases": [{"access": "public",
                        "type": {"qualType": "iterators::facade<r>"},
                        "writtenAccess": "none"}],
             "inner": [
                 {"id": "0x3", "kind": "FieldDecl", "name": "f",
                  "type": {"qualType": "detail::impl"}},
             ]},
            {"id": "0x4", "kind": "UsingDecl", "name": "range::count"},
            {"id": "0x5", "kind": "UsingShadowDecl", "isImplicit": True,
             "target": {"id": "0x77", "kind": "FunctionDecl", "name": "count"},
             "inner": [{"id": "0x77", "kind": "FunctionDecl", "name": "count",
                        "loc": loc("/b/range.hpp", 40)}]},
            {"id": "0x6", "kind": "UsingDirectiveDecl",
             "nominatedNamespace": {"id": "0x9", "kind": "NamespaceDecl",
                                    "name": "adl"}},
            {"id": "0x7", "kind": "FunctionDecl", "loc": loc(line=50), "name": "g",
             "type": {"qualType": "int (const boost::r &)"},
             "inner": [{"id": "0x8", "kind": "ParmVarDecl", "name": "x",
                        "type": {"qualType": "const boost::r &"}}]},
        ], loc("/b/r.hpp", 1))))
        r = tu.by_id["0x2"]
        self.assertEqual(r.bases, ["iterators::facade<r>"])
        self.assertIn("iterators::facade<r>", r.written)
        self.assertEqual(tu.by_id["0x4"].name, "range::count")
        self.assertEqual(tu.by_id["0x5"].target, "0x77")
        self.assertNotIn("0x77", tu.by_id)       # shadow copy not recorded
        self.assertEqual(tu.by_id["0x6"].target, "0x9")
        self.assertEqual(tu.by_id["0x7"].params, ["const boost::r &"])
        # the location inside the skipped shadow copy still moved the state
        self.assertEqual(tu.by_id["0x7"].file, "/b/range.hpp")


if __name__ == "__main__":
    unittest.main()
