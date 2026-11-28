"""A `republish` policy entry re-exports what the owning module exports in the
namespace a using-directive names (boost::dll::fs -> boost::filesystem)."""
import io
import json
import tempfile
import unittest
from pathlib import Path

import ast_json
import gen_exports

HDR = "/b/boost/dll/config.hpp"


def loc(line=1):
    return {"offset": 1, "line": line, "col": 1, "file": HDR}


def ns(nid, name, inner):
    return {"id": nid, "kind": "NamespaceDecl", "loc": loc(),
            "range": {"begin": loc(), "end": loc()}, "name": name,
            "inner": inner}


def model():
    """namespace boost {
         namespace filesystem { struct path; }
         namespace dll { namespace fs { using namespace boost::filesystem; } }
       }"""
    text = json.dumps(ns("0x1", "boost", [
        ns("0x2", "filesystem", [
            {"id": "0x10", "kind": "CXXRecordDecl", "loc": loc(2),
             "name": "path", "tagUsed": "struct",
             "completeDefinition": True},
        ]),
        ns("0x3", "dll", [
            ns("0x4", "fs", [
                {"id": "0x20", "kind": "UsingDirectiveDecl", "loc": loc(4),
                 "nominatedNamespace": {"id": "0x2", "kind": "NamespaceDecl",
                                        "name": "filesystem"}},
            ]),
        ]),
    ]), indent=2) + "\n"
    return gen_exports.Model(ast_json.TU.from_stream(io.BytesIO(text.encode())))


def published():
    """What filesystem's emit_inc recorded (one guarded name, one enumerator
    exported through its enum's scope, one name of a nested namespace)."""
    return {"filesystem": [
        {"ns": ["boost", "filesystem"], "qname": "boost::filesystem::path",
         "if": ""},
        {"ns": ["boost", "filesystem"],
         "qname": "boost::filesystem::file_type::regular_file", "if": ""},
        {"ns": ["boost", "filesystem"], "qname": "boost::filesystem::fopen",
         "if": "defined(_WIN32)"},
        {"ns": ["boost", "filesystem", "detail"],
         "qname": "boost::filesystem::detail::status", "if": ""},
    ]}


ENTRY = {"namespace": "boost::dll::fs", "if": "!defined(USE_STD)",
         "reason": "fs spelling"}


def policy(**overrides):
    return gen_exports.Policy({"dll": {"republish": [dict(ENTRY, **overrides)]}})


class RepublishTests(unittest.TestCase):
    def setUp(self):
        self.m = model()

    def collect(self, pol=None, pub=None, claimed=None, ups=("filesystem",)):
        return gen_exports.collect_republished(
            "dll", self.m, pol or policy(), published() if pub is None else pub,
            set(ups), {} if claimed is None else claimed)

    def test_follows_the_owner_surface(self):
        out, sources, entries, errors = self.collect()
        self.assertEqual(errors, [])
        self.assertEqual(sorted(out), [
            "boost::dll::fs::fopen", "boost::dll::fs::path",
            "boost::dll::fs::regular_file"])
        rec = out["boost::dll::fs::regular_file"]
        self.assertEqual(rec["qname"],
                         "::boost::filesystem::file_type::regular_file")
        self.assertEqual(rec["ns"], ["boost", "dll", "fs"])
        self.assertEqual(rec["owner"], "filesystem")
        self.assertEqual(list(sources), ["boost::dll::fs=boost::filesystem"])
        self.assertEqual(entries, [("dll", "republish", "boost::dll::fs",
                                    "!defined(USE_STD)", "fs spelling")])

    def test_entry_condition_combines_with_the_owner_guard(self):
        out = self.collect()[0]
        base = out["boost::dll::fs::path"]["guard"]
        # One dict per condition: emit_inc groups consecutive names by it.
        self.assertIs(out["boost::dll::fs::regular_file"]["guard"], base)
        self.assertEqual(base, {"if": "!defined(USE_STD)",
                                "reason": "fs spelling"})
        self.assertEqual(out["boost::dll::fs::fopen"]["guard"]["if"],
                         "(!defined(USE_STD)) && (defined(_WIN32))")

    def test_without_condition_only_source_guards_apply(self):
        pol = gen_exports.Policy({"dll": {"republish": [
            {"namespace": "boost::dll::fs", "reason": "r"}]}})
        out = self.collect(pol)[0]
        self.assertIsNone(out["boost::dll::fs::path"]["guard"])
        self.assertEqual(out["boost::dll::fs::fopen"]["guard"]["if"],
                         "defined(_WIN32)")

    def test_name_claimed_earlier_is_left_out(self):
        claimed = {"boost::dll::fs::path": "other"}
        out = self.collect(claimed=claimed)[0]
        self.assertNotIn("boost::dll::fs::path", out)
        self.assertEqual(
            claimed["boost::dll::fs::fopen=boost::filesystem::fopen"], "dll")

    def test_owner_must_be_upstream(self):
        out, _, _, errors = self.collect(ups=("core",))
        self.assertEqual(out, {})
        self.assertEqual(len(errors), 1)
        self.assertIn("no upstream module", errors[0])

    def test_stale_entry_is_an_error(self):
        errors = self.collect(policy(namespace="boost::dll::gone"))[3]
        self.assertEqual(len(errors), 1)
        self.assertIn("no using-directive", errors[0])

    def test_cached_result_holds_only_while_the_owner_surface_does(self):
        _, sources, _, _ = self.collect()
        sk = {"republish": sources, "published": []}
        ups = {"filesystem"}
        self.assertTrue(gen_exports._republish_holds(sk, published(), ups))
        grown = published()
        grown["filesystem"].append({"ns": ["boost", "filesystem"],
                                    "qname": "boost::filesystem::space",
                                    "if": ""})
        self.assertFalse(gen_exports._republish_holds(sk, grown, ups))
        self.assertFalse(gen_exports._republish_holds(
            {"republish": sources}, published(), ups))

    def test_upstreams_are_transitive(self):
        graph = {"dll": {"filesystem"}, "filesystem": {"system"},
                 "system": {"dll"}}
        self.assertEqual(gen_exports.upstreams_of(graph, "dll"),
                         {"filesystem", "system"})


class EmitRepublishedTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def test_inc_and_published_list(self):
        out = gen_exports.collect_republished(
            "dll", model(), policy(), published(), {"filesystem"}, {})[0]
        out["own"] = {"home": "dll", "usr": "own", "qname":
                      "boost::dll::fs::error_code", "ns": ["boost", "dll", "fs"]}
        pub = []
        count, deps, skipped = gen_exports.emit_inc(
            "dll", out, self.root, {}, gen_exports.Policy({}),
            extra_deps={"filesystem"}, published=pub)
        self.assertEqual((count, deps, skipped), (4, ["filesystem"], []))
        inc = (self.root / "generated/exports/dll.inc").read_text()
        block = inc[inc.index("export namespace boost { namespace dll"):]
        self.assertEqual([ln for ln in block.splitlines()
                          if not ln.lstrip().startswith("//")], [
            "export namespace boost { namespace dll { namespace fs {",
            "#if !defined(USE_STD)",
            "  using ::boost::filesystem::file_type::regular_file;",
            "#endif",
            "#if (!defined(USE_STD)) && (defined(_WIN32))",
            "  using ::boost::filesystem::fopen;",
            "#endif",
            "#if !defined(USE_STD)",
            "  using ::boost::filesystem::path;",
            "#endif",
            "  using boost::dll::fs::error_code;",
            "}}}"])
        self.assertIn({"ns": ["boost", "dll", "fs"],
                       "qname": "boost::filesystem::path",
                       "if": "!defined(USE_STD)"}, pub)
        self.assertIn({"ns": ["boost", "dll", "fs"],
                       "qname": "boost::dll::fs::error_code", "if": ""}, pub)


if __name__ == "__main__":
    unittest.main()
