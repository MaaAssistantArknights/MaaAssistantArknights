"""Run with clang-format on PATH to verify real formatter subprocess behavior."""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("clang-formatter.py")
FORMATTER = shutil.which("clang-format")


@unittest.skipUnless(FORMATTER, "clang-format is required for integration tests")
class FormatterIntegrationTests(unittest.TestCase):
    def test_only_requested_directory_is_ignored(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            contents = "int  main(){return  0;}\n"
            files = ["skip/a.cpp", "skip/deep/b.cpp", "skip-extra/c.cpp", "skip2/d.cpp"]
            for name in files:
                file = root / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(contents, encoding="utf-8")
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--input",
                    str(root),
                    "--clang-format",
                    "clang-format",
                    "--style",
                    "LLVM",
                    "--ignore",
                    json.dumps([str(root / "skip")]),
                ],
                check=True,
                capture_output=True,
                text=True,
            )
            self.assertIn("clang-format", result.stdout)
            expected = subprocess.run(
                [FORMATTER, "--style=LLVM"],
                input=contents,
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            self.assertNotEqual(contents, expected)
            for name in files[:2]:
                self.assertEqual((root / name).read_text(encoding="utf-8"), contents)
            for name in files[2:]:
                self.assertEqual((root / name).read_text(encoding="utf-8"), expected)


if __name__ == "__main__":
    unittest.main()
