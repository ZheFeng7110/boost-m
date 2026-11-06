"""Hand-written modules must not require nonexistent generated bundle TUs;
the linkage audit reuses its last report while its inputs hold."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import ast_json
import gen_audit
import gen_exports


class AuditSpecialModulesTests(unittest.TestCase):
    def test_linkage_audit_skips_special_modules(self):
        with mock.patch("sys.argv", ["gen_audit.py"]), \
                mock.patch.object(gen_audit.bc, "TARGET_LIBS", ["atomic", "version"]), \
                mock.patch.object(gen_audit, "audit_lib", return_value=(0, 0, 0, 0)) as audit:
            self.assertEqual(gen_audit.main(), 0)
        audit.assert_called_once()
        self.assertEqual(audit.call_args.args[:2], ("atomic", gen_audit.bc.AUDIT_DIR))

    def test_explicit_special_module_needs_no_bundle(self):
        with mock.patch("sys.argv", ["gen_audit.py", "--libs", "version"]), \
                mock.patch.object(gen_audit, "audit_lib") as audit:
            self.assertEqual(gen_audit.main(), 0)
        audit.assert_not_called()

    def test_macro_audit_keeps_special_module_headers(self):
        with mock.patch("sys.argv", ["gen_audit.py", "--macros", "--libs", "version"]), \
                mock.patch.object(gen_audit, "audit_lib") as linkage, \
                mock.patch.object(gen_audit, "macros_audit", return_value=0) as audit:
            self.assertEqual(gen_audit.main(), 0)
        linkage.assert_not_called()
        audit.assert_called_once_with(["version"], gen_audit.bc.AUDIT_DIR)


class AuditCacheTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.deps = self.root / "upstream/boost"
        self.header = self.deps / "boost/x.hpp"
        self.header.parent.mkdir(parents=True)
        self.header.write_text("one")
        self.out = self.root / "audit"
        self.policy = gen_exports.Policy({"x": {"gmf": ["boost/x.hpp"]}})
        self.tu = ast_json.TU()
        self.tu.inputs = [str(self.header)]
        for patcher in (
            mock.patch.object(gen_exports.bc, "ROOT", self.root),
            mock.patch.object(gen_exports.bc, "DEPS", self.deps),
            mock.patch.object(gen_exports.bc, "CACHE_DIR", self.root / "cache"),
            mock.patch.object(gen_exports.bc, "CLANG_ARGS", []),
            mock.patch.object(gen_exports, "_closure_hash", return_value="closure"),
            mock.patch.object(gen_exports.aj, "clang_version", return_value="clang 23"),
            mock.patch.dict(gen_exports._FILE_HASH, clear=True),
        ):
            patcher.start()
            self.addCleanup(patcher.stop)

    def audit(self):
        with mock.patch.object(gen_exports, "parse_bundle", return_value=(
                self.tu, [self.header])) as parse:
            counts = gen_audit.audit_lib("x", self.out, self.policy)
        return counts, parse.call_count

    def test_hit_skips_the_dump(self):
        first, dumps = self.audit()
        self.assertEqual(dumps, 1)
        report = (self.out / "x.txt").read_text()
        (self.out / "x.txt").unlink()
        self.assertEqual(self.audit(), (first, 0))
        self.assertEqual((self.out / "x.txt").read_text(), report)

    def test_edit_to_a_read_file_invalidates(self):
        self.audit()
        self.header.write_text("two")
        gen_exports._FILE_HASH.clear()
        self.assertEqual(self.audit()[1], 1)

    def test_gate_key_change_invalidates(self):
        self.audit()
        self.policy = gen_exports.Policy(
            {"x": {"gmf": ["boost/x.hpp"], "defines": ["X_ONLY"]}})
        self.assertEqual(self.audit()[1], 1)

    def test_no_cache_always_dumps(self):
        self.audit()
        with mock.patch.object(gen_exports, "parse_bundle", return_value=(
                self.tu, [self.header])) as parse:
            gen_audit.audit_lib("x", self.out, self.policy, use_cache=False)
        parse.assert_called_once()


if __name__ == "__main__":
    unittest.main()
