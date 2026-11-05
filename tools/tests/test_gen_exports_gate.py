"""The compile gate must prune the same headers on every host platform."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import ast_json
import gen_exports


class ExportGateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.deps = self.root / "upstream" / "boost"
        self.icu = self.deps / "boost/regex/icu.hpp"
        self.regex = self.deps / "boost/regex.hpp"
        self.tu = ast_json.TU()
        self.policy = gen_exports.Policy({})
        for patcher in (
            mock.patch.object(gen_exports.bc, "ROOT", self.root),
            mock.patch.object(gen_exports.bc, "DEPS", self.deps),
            mock.patch.object(gen_exports.bc, "CACHE_DIR", self.root / "cache"),
            mock.patch.object(gen_exports.bc, "CLANG_ARGS", []),
            mock.patch.object(gen_exports, "_closure_hash", return_value="closure"),
            mock.patch.object(gen_exports.aj, "clang_version", return_value="clang 23"),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def test_prune_missing_optional_dependency(self):
        for prefix in ("/home/runner/boost-m/upstream/boost",
                       "D:/a/boost-m/boost-m/upstream/boost",
                       r"D:\a\boost-m\boost-m\upstream\boost",
                       "C:/Users/runner admin/boost-m/upstream/boost"):
            with self.subTest(prefix=prefix):
                stderr = (
                    "In file included from {0}/boost/regex/icu.hpp:23:\n"
                    "{0}/boost/regex/v5/icu.hpp:23:10: fatal error: "
                    "'unicode/utypes.h' file not found\n"
                ).format(prefix)
                with mock.patch.object(gen_exports.aj, "dump", side_effect=[
                    ast_json.DumpError(1, stderr), self.tu,
                ]) as dump:
                    result = gen_exports.parse_bundle(
                        "regex", [self.icu, self.regex], self.policy,
                        use_cache=False)
                self.assertEqual(result, (self.tu, [self.regex]))
                self.assertEqual(dump.call_count, 2)
                bundle = self.root / "cache/bundles/regex.cpp"
                self.assertEqual(bundle.read_text(), "#include <boost/regex.hpp>\n")

    def test_unrelated_error_still_fails(self):
        stderr = "D:/sdk/missing.hpp:12:3: fatal error: 'missing.h' file not found\n"
        with mock.patch.object(gen_exports.aj, "dump", side_effect=
                               ast_json.DumpError(1, stderr)) as dump:
            self.assertIsNone(gen_exports.parse_bundle(
                "regex", [self.icu, self.regex], self.policy, use_cache=False))
        dump.assert_called_once()

    def test_policy_defines_reach_the_compile(self):
        policy = gen_exports.Policy({"regex": {"defines": ["X_ONLY"]}})
        with mock.patch.object(gen_exports.aj, "dump",
                               return_value=self.tu) as dump:
            gen_exports.parse_bundle("regex", [self.regex], policy,
                                     use_cache=False)
        self.assertIn("-DX_ONLY", dump.call_args.args[1])


if __name__ == "__main__":
    unittest.main()
