"""Generator cache entries stay valid only while their inputs hold: the files
clang read and the first-wins ownership seen when they were computed."""
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import ast_json
import gen_exports


class DepfileTests(unittest.TestCase):
    def test_continuations_and_escapes(self):
        text = ("bundle.o: /r/bundle.cpp /r/a\\ b.hpp \\\n"
                "  /r/c\\#d.hpp /r/$$e.hpp \\\r\n"
                "  D:\\a\\x.hpp\n")
        self.assertEqual(ast_json.parse_depfile(text), [
            "/r/bundle.cpp", "/r/a b.hpp", "/r/c#d.hpp", "/r/$e.hpp",
            "D:\\a\\x.hpp"])

    def test_empty(self):
        self.assertEqual(ast_json.parse_depfile(""), [])

    @unittest.skipUnless(shutil.which(ast_json.clang_command()[0]),
                         "clang++ not on PATH")
    def test_macro_computed_include_is_an_input(self):
        # The include-closure regex cannot see `#include NAME()`; clang's
        # dependency list must.
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            (d / "hidden.hpp").write_text("namespace boost { int h; }\n")
            (d / "t.cpp").write_text('#define NAME() "hidden.hpp"\n'
                                     "#include NAME()\n")
            tu = ast_json.dump(d / "t.cpp", ["-std=c++23"],
                                cwd=str(gen_exports.bc.ROOT))
            names = {Path(p).name for p in tu.inputs}
            self.assertIn("hidden.hpp", names)


class ManifestTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        for patcher in (
            mock.patch.object(gen_exports.bc, "ROOT", self.root),
            mock.patch.object(gen_exports.bc, "CACHE_DIR",
                              self.root / "cache"),
            mock.patch.dict(gen_exports._FILE_HASH, clear=True),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def test_edit_to_any_read_file_invalidates(self):
        inc = self.root / "upstream/boost/boost/x/detail/rel.hpp"
        inc.parent.mkdir(parents=True)
        inc.write_text("one")
        bundle = self.root / "cache/bundles/x.cpp"
        bundle.parent.mkdir(parents=True)
        bundle.write_text("#include <boost/x.hpp>\n")
        tu = ast_json.TU()
        tu.inputs = [str(bundle), "upstream/boost/boost/x/detail/rel.hpp"]
        man = gen_exports._input_manifest(tu, self.root)
        self.assertEqual(list(man), ["upstream/boost/boost/x/detail/rel.hpp"])
        self.assertTrue(gen_exports._manifest_ok(man))
        inc.write_text("two")
        gen_exports._FILE_HASH.clear()
        self.assertFalse(gen_exports._manifest_ok(man))

    def test_entry_without_manifest_is_stale(self):
        self.assertFalse(gen_exports._manifest_ok(None))
        self.assertFalse(gen_exports._manifest_ok({}))


class ClaimViewTests(unittest.TestCase):
    def test_view_records_the_owner_before_the_library(self):
        table = {"boost::a=boost::x::a": "range"}
        view = gen_exports._ClaimView(table)
        self.assertIn("boost::a=boost::x::a", view)
        self.assertNotIn("boost::b", view)
        view["boost::b"] = "algorithm"
        self.assertIn("boost::b", view)      # second probe: own claim
        view.setdefault("boost::c", "algorithm")
        self.assertEqual(view.seen, {"boost::a=boost::x::a": "range",
                                     "boost::b": None, "boost::c": None})
        self.assertEqual(table["boost::b"], "algorithm")

    def test_entry_holds_only_under_the_same_ownership(self):
        sk = {"claims": [["u1", "te"], ["u2", "te"]],
              "pre_claimed": ["u2"],
              "inject_view": [["boost::j", None]]}
        self.assertTrue(gen_exports._claims_hold(sk, {"u2": "pc"}, {}))
        # an earlier library (not an include-graph upstream) now claims u1
        self.assertFalse(gen_exports._claims_hold(
            sk, {"u1": "pc", "u2": "pc"}, {}))
        # ... or released u2
        self.assertFalse(gen_exports._claims_hold(sk, {}, {}))
        # ... or now injects the same name first
        self.assertFalse(gen_exports._claims_hold(
            sk, {"u2": "pc"}, {"boost::j": "range"}))
        # entries written before the view existed are stale
        self.assertFalse(gen_exports._claims_hold(
            {"claims": []}, {}, {}))


if __name__ == "__main__":
    unittest.main()
