"""The syntax gate must prune the same headers on every host platform."""
import subprocess
import tempfile
import types
import unittest
from pathlib import Path
from unittest import mock

import gen_exports


class ExportGateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.deps = self.root / "upstream" / "boost"
        self.icu = self.deps / "boost/regex/icu.hpp"
        self.regex = self.deps / "boost/regex.hpp"
        self.tu = types.SimpleNamespace(diagnostics=[])
        self.index = mock.Mock()
        self.index.parse.return_value = self.tu
        self.ci = mock.Mock()
        self.ci.Index.create.return_value = self.index
        for patcher in (
            mock.patch.object(gen_exports.bc, "ROOT", self.root),
            mock.patch.object(gen_exports.bc, "DEPS", self.deps),
            mock.patch.object(gen_exports.bc, "CACHE_DIR", self.root / "cache"),
            mock.patch.object(gen_exports.bc, "CLANG_ARGS", []),
            mock.patch.object(gen_exports.bc, "get_ci", return_value=self.ci),
            mock.patch.object(gen_exports, "_closure_hash", return_value="closure"),
            mock.patch.object(gen_exports, "_clang_version", return_value="clang 23"),
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
                with mock.patch("subprocess.run", side_effect=[
                    subprocess.CompletedProcess([], 1, "", stderr),
                    subprocess.CompletedProcess([], 0, "", ""),
                ]) as gate:
                    result = gen_exports._parse_bundle(
                        "regex", [self.icu, self.regex], use_cache=False)
                self.assertEqual(result, (self.tu, [self.regex]))
                self.assertEqual(gate.call_count, 2)
                bundle = self.root / "cache/bundles/regex.cpp"
                self.assertEqual(bundle.read_text(), "#include <boost/regex.hpp>\n")

    def test_unrelated_error_still_fails(self):
        stderr = "D:/sdk/missing.hpp:12:3: fatal error: 'missing.h' file not found\n"
        with mock.patch("subprocess.run", return_value=
                        subprocess.CompletedProcess([], 1, "", stderr)) as gate:
            self.assertIsNone(gen_exports._parse_bundle(
                "regex", [self.icu, self.regex], use_cache=False))
        gate.assert_called_once()
        self.index.parse.assert_not_called()


if __name__ == "__main__":
    unittest.main()
