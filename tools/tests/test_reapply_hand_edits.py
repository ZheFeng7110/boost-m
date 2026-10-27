"""Module-side patches must replay over the current generator's drafts."""
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import gen_exports
import reapply_hand_edits


class ModulePatchTests(unittest.TestCase):
    def test_regenerated_patches_are_idempotent(self):
        for lib in ("winapi", "cobalt", "safe_numerics", "stacktrace"):
            with self.subTest(lib=lib):
                self.check_regenerated_patch(lib)

    def check_regenerated_patch(self, lib):
        name = "safe_numerics_src" if lib == "safe_numerics" else lib
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "patches").mkdir()
            patch = reapply_hand_edits.PATCH_DIR / (name + ".patch")
            (root / "patches" / patch.name).write_bytes(patch.read_bytes())
            deps = root / "generated/exports"
            deps.mkdir(parents=True)
            (deps / (lib + ".deps")).write_bytes(
                (gen_exports.bc.GEN_DIR / (lib + ".deps")).read_bytes())
            headers = gen_exports.bc.gfm_headers_of(lib)
            # The clang++ gate removes this non-self-contained header before
            # emitting the safe_numerics draft in the full generation pass.
            if lib == "safe_numerics":
                headers = [h for h in headers if h.name != "range_value.hpp"]
            gen_exports.emit_cppm(lib, root, gfm_headers=headers)
            output = root / "src" / (lib + ".cppm")
            expected = (gen_exports.bc.ROOT / "src" / output.name).read_bytes()

            # Require git apply to work without relying on patch(1)'s fuzz or
            # its availability on the host. The second pass must reverse-check.
            with mock.patch.object(reapply_hand_edits, "ROOT", root), \
                    mock.patch("subprocess.run", wraps=subprocess.run) as run:
                reapply_hand_edits.apply_patch_file(name)
                self.assertEqual(output.read_bytes(), expected)
                reapply_hand_edits.apply_patch_file(name)
                self.assertEqual(output.read_bytes(), expected)
            self.assertTrue(all(call.args[0][0] == "git"
                                for call in run.call_args_list))
            self.assertEqual(sorted(p.name for p in (root / "src").iterdir()),
                             [output.name])

    def test_windows_checkout_preserves_generation_bytes(self):
        # Exercise Git's actual Windows-style checkout conversion on every OS.
        paths = (
            "patches/winapi.patch", "src/winapi.cppm",
            "generated/exports/winapi.inc", "generated/exports/winapi.deps",
            "patches/final/generated/exports/algorithm.inc",
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)

            def git(*args):
                return subprocess.run(
                    ["git", "-c", "core.autocrlf=true", *args], cwd=root,
                    capture_output=True, check=True)

            git("init")
            (root / ".gitattributes").write_bytes(
                (gen_exports.bc.ROOT / ".gitattributes").read_bytes())
            for rel in paths:
                dst = root / rel
                dst.parent.mkdir(parents=True, exist_ok=True)
                dst.write_bytes((gen_exports.bc.ROOT / rel).read_bytes())
            git("add", ".")
            for rel in paths:
                (root / rel).unlink()
            git("checkout-index", "--all", "--force")
            for rel in paths:
                with self.subTest(path=rel):
                    self.assertEqual((root / rel).read_bytes(),
                                     (gen_exports.bc.ROOT / rel).read_bytes())


if __name__ == "__main__":
    unittest.main()
