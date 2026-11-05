"""Run with clang-format on PATH to verify real formatter subprocess behavior."""

import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SCRIPT = Path(__file__).with_name("clang-formatter.py").resolve()
FORMATTER = shutil.which("clang-format")


@unittest.skipUnless(FORMATTER, "clang-format is required for integration tests")
class FormatterIntegrationTests(unittest.TestCase):
    def test_formatter_path_with_spaces_and_inline_style(self):
        with tempfile.TemporaryDirectory(prefix="maa formatter ") as temp:
            root = Path(temp)
            formatter = root / Path(FORMATTER).name
            shutil.copy2(FORMATTER, formatter)
            contents = "int  main(){return  0;}\n"
            style = "{BasedOnStyle: LLVM, IndentWidth: 3}"
            expected = subprocess.run(
                [FORMATTER, f"--style={style}"],
                input=contents,
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            for directory_input in [False, True]:
                with self.subTest(directory_input=directory_input):
                    source = root / "operator & name.cpp"
                    source.write_text(contents, encoding="utf-8")
                    subprocess.run(
                        [
                            sys.executable,
                            str(SCRIPT),
                            "--input",
                            str(root if directory_input else source),
                            "--clang-format",
                            str(formatter),
                            "--style",
                            style,
                        ],
                        check=True,
                        capture_output=True,
                        text=True,
                    )
                    self.assertEqual(source.read_text(encoding="utf-8"), expected)

    def test_formatter_errors_produce_nonzero_exit_status(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            source = root / "test.cpp"
            contents = "int  main(){return  0;}\n"
            source.write_text(contents, encoding="utf-8")
            for formatter, style in [
                (FORMATTER, "InvalidStyle"),
                (str(root / "missing-clang-format"), "LLVM"),
            ]:
                for directory_input in [False, True]:
                    with self.subTest(
                        formatter=formatter, directory_input=directory_input
                    ):
                        result = subprocess.run(
                            [
                                sys.executable,
                                str(SCRIPT),
                                "--input",
                                str(root if directory_input else source),
                                "--clang-format",
                                formatter,
                                "--style",
                                style,
                            ],
                            capture_output=True,
                            text=True,
                        )
                        self.assertNotEqual(result.returncode, 0)
                        self.assertEqual(source.read_text(encoding="utf-8"), contents)

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

    def test_ignored_current_directory_preserves_descendants(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            contents = "int  main(){return  0;}\n"
            files = ["root.cpp", "sub/child.cpp", "sub/deep/nested.cpp"]
            for name in files:
                file = root / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(contents, encoding="utf-8")
            subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--input",
                    ".",
                    "--ignore",
                    '["."]',
                    "--style",
                    "LLVM",
                ],
                cwd=root,
                check=True,
                capture_output=True,
                text=True,
            )
            for name in files:
                self.assertEqual((root / name).read_text(encoding="utf-8"), contents)

    def test_relative_nested_ignore_preserves_sibling(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            contents = "int  main(){return  0;}\n"
            ignored = root / "tree/skip/deep/a.cpp"
            sibling = root / "tree/skip-extra/b.cpp"
            for file in [ignored, sibling]:
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(contents, encoding="utf-8")
            subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--input",
                    "tree",
                    "--ignore",
                    '["tree/skip"]',
                    "--style",
                    "LLVM",
                ],
                cwd=root,
                check=True,
                capture_output=True,
                text=True,
            )
            expected = subprocess.run(
                [FORMATTER, "--style=LLVM"],
                input=contents,
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            self.assertNotEqual(contents, expected)
            self.assertEqual(ignored.read_text(encoding="utf-8"), contents)
            self.assertEqual(sibling.read_text(encoding="utf-8"), expected)


if __name__ == "__main__":
    unittest.main()
