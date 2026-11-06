"""Namespace-scope using-declarations must export the name they publish."""
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import ast_json
import gen_exports

HDR = "/b/boost/algorithm/string/constants.hpp"


def loc(file=None, line=1):
    out = {"offset": 1, "line": line, "col": 1}
    if file is not None:
        out["file"] = file
    return out


def ns(nid, name, inner):
    return {"id": nid, "kind": "NamespaceDecl", "loc": loc(),
            "range": {"begin": loc(), "end": loc()}, "name": name,
            "inner": inner}


def using(nid, name, line):
    return {"id": nid, "kind": "UsingDecl", "loc": loc(line=line),
            "name": name}


def enum(nid, name, enumerators, line, scoped=False):
    d = {"id": nid, "kind": "EnumDecl", "loc": loc(line=line), "name": name,
         "inner": [{"id": "{}{}".format(nid, i), "kind": "EnumConstantDecl",
                    "loc": loc(line=line), "name": e,
                    "type": {"qualType": name}}
                   for i, e in enumerate(enumerators)]}
    if scoped:
        d["scopedEnumTag"] = "class"
    return d


def model():
    """namespace boost {
         namespace algorithm {
           enum token_compress_mode_type { token_compress_on, ... };
           enum class mode { fast };
           void join();
         }
         using algorithm::token_compress_on;
         using algorithm::join;
         namespace range_detail { struct formatted_range; }
         namespace range { using boost::range_detail::formatted_range; }
       }"""
    text = json.dumps(ns("0x1", "boost", [
        ns("0x2", "algorithm", [
            {**enum("0x10", "token_compress_mode_type",
                    ["token_compress_on", "token_compress_off"], 2),
             "loc": loc(HDR, 2)},
            enum("0x20", "mode", ["fast"], 3, scoped=True),
            {"id": "0x30", "kind": "FunctionDecl", "loc": loc(line=4),
             "name": "join", "mangledName": "_ZN5boost9algorithm4joinEv",
             "type": {"qualType": "void ()"}},
        ]),
        using("0x40", "algorithm::token_compress_on", 6),
        using("0x41", "algorithm::join", 7),
        ns("0x3", "range_detail", [
            {"id": "0x50", "kind": "CXXRecordDecl", "loc": loc(line=8),
             "name": "formatted_range", "tagUsed": "struct",
             "completeDefinition": True},
        ]),
        ns("0x4", "range", [
            using("0x60", "boost::range_detail::formatted_range", 9),
        ]),
    ]), indent=2) + "\n"
    tu = ast_json.TU.from_stream(io.BytesIO(text.encode()))
    return gen_exports.Model(tu)


class InjectionTests(unittest.TestCase):
    def setUp(self):
        patcher = mock.patch.object(gen_exports, "home_of_file",
                                    return_value="algorithm")
        patcher.start()
        self.addCleanup(patcher.stop)
        self.m = model()

    def test_unscoped_enumerator_found_through_enclosing_scope(self):
        idx = self.m.qindex()
        self.assertIn("boost::algorithm::token_compress_on", idx)
        self.assertIn(
            "boost::algorithm::token_compress_mode_type::token_compress_on",
            idx)
        self.assertNotIn("boost::algorithm::fast", idx)   # enum class
        out = gen_exports.collect_injections("algorithm", self.m, {})
        self.assertEqual(out["boost::token_compress_on"]["qname"],
                         "boost::token_compress_on")

    def test_qualified_target_injects_its_last_segment(self):
        out = gen_exports.collect_injections("algorithm", self.m, {})
        self.assertIn("boost::range::formatted_range", out)
        self.assertNotIn("boost::range_detail::formatted_range", out)
        d = self.m.by_id["0x60"]
        self.assertEqual(gen_exports._using_target_name(self.m, d),
                         ("boost::range::formatted_range",
                          "boost::range_detail::formatted_range"))

    def test_same_name_other_target_is_not_deduplicated(self):
        claimed = {"boost::join=boost::range::join": "range"}
        out = gen_exports.collect_injections("algorithm", self.m, claimed)
        self.assertEqual(out["boost::join"]["qname"], "boost::join")
        self.assertEqual(claimed["boost::join=boost::algorithm::join"],
                         "algorithm")

    def test_same_name_same_target_is_deduplicated(self):
        claimed = {"boost::join=boost::algorithm::join": "string_algo"}
        out = gen_exports.collect_injections("algorithm", self.m, claimed)
        self.assertNotIn("boost::join", out)


class FacadeImportTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.deps = self.root / "upstream" / "boost"
        for patcher in (
            mock.patch.object(gen_exports.bc, "ROOT", self.root),
            mock.patch.object(gen_exports.bc, "DEPS", self.deps),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)
        (self.root / "src").mkdir()
        (self.root / "src" / "range.cppm").write_text(
            "module;\n#include <boost/range.hpp>\n"
            "export module mcpplibs.mwrap.boost.range;\n"
            "export import mcpplibs.mwrap.boost.core;\n"
            '#include "../generated/exports/range.inc"\n')
        self.gmf = [self.deps / "boost/range.hpp"]

    def check(self, deps, omit=()):
        return gen_exports.check_facade(
            "range", self.gmf, (),
            ["mcpplibs.mwrap.boost." + d for d in deps], omit)

    def test_missing_edge_is_reported(self):
        problems = self.check(["core", "smart_ptr"])
        self.assertEqual(len(problems), 1)
        self.assertIn("export import mcpplibs.mwrap.boost.smart_ptr;",
                      problems[0])

    def test_declared_omission_is_accepted(self):
        self.assertEqual(self.check(["core", "regex"], ["regex"]), [])

    def test_stale_omission_is_reported(self):
        self.assertEqual(len(self.check(["core"], ["regex"])), 1)
        self.assertIn("but src/range.cppm imports it",
                      self.check(["core"], ["core"])[0])


if __name__ == "__main__":
    unittest.main()
