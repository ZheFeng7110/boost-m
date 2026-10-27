"""Module-side patches must replay over the current generator's drafts."""
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import gen_exports
import reapply_hand_edits


class WinapiPatchTests(unittest.TestCase):
    def test_regenerated_winapi_patch_is_idempotent(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "patches").mkdir()
            patch = reapply_hand_edits.PATCH_DIR / "winapi.patch"
            (root / "patches/winapi.patch").write_bytes(patch.read_bytes())
            deps = root / "generated/exports"
            deps.mkdir(parents=True)
            (deps / "winapi.deps").write_bytes(
                (gen_exports.bc.GEN_DIR / "winapi.deps").read_bytes())
            gen_exports.emit_cppm("winapi", root)
            expected = (gen_exports.bc.ROOT / "src/winapi.cppm").read_bytes()

            # Require git apply to work without relying on patch(1)'s fuzz or
            # its availability on the host. The second pass must reverse-check.
            with mock.patch.object(reapply_hand_edits, "ROOT", root), \
                    mock.patch("subprocess.run", wraps=subprocess.run) as run:
                reapply_hand_edits.apply_patch_file("winapi")
                self.assertEqual((root / "src/winapi.cppm").read_bytes(), expected)
                reapply_hand_edits.apply_patch_file("winapi")
                self.assertEqual((root / "src/winapi.cppm").read_bytes(), expected)
            self.assertTrue(all(call.args[0][0] == "git"
                                for call in run.call_args_list))


if __name__ == "__main__":
    unittest.main()
