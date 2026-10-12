import contextlib
import io
import json
import os
from pathlib import Path
import runpy
import sys
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).with_name("clang-formatter.py")


class FormatterIgnoreTests(unittest.TestCase):
    def commands(self, files, ignores, input_name="."):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            for name in files:
                file = root / name
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text("int x;\n", encoding="utf-8")
            args = [
                str(SCRIPT),
                "--input",
                str(root / input_name),
                "--ignore",
                json.dumps([str(root / name) for name in ignores]),
            ]
            with (
                patch.object(sys, "argv", args),
                patch("os.system", return_value=0) as calls,
            ):
                with contextlib.redirect_stdout(io.StringIO()):
                    runpy.run_path(str(SCRIPT), run_name="__main__")
            return [
                call.args[0].replace(str(root), "ROOT") for call in calls.call_args_list
            ]

    def formatted(self, files, ignores, expected, input_name="."):
        commands = self.commands(files, ignores, input_name)
        self.assertEqual(commands[0], "clang-format --version")
        expected_commands = [
            f'clang-format -i -style=file "{Path("ROOT") / name}"' for name in expected
        ]
        self.assertCountEqual(commands[1:], expected_commands)

    def test_similar_sibling_is_not_ignored(self):
        self.formatted(
            ["skip/a.cpp", "skip-extra/b.cpp"], ["skip"], ["skip-extra/b.cpp"]
        )

    def test_ignored_descendants_are_skipped(self):
        self.formatted(["skip/deep/a.cpp", "keep/b.cpp"], ["skip"], ["keep/b.cpp"])

    def test_exact_ignored_file(self):
        self.formatted(["a.cpp", "b.cpp"], ["a.cpp"], ["b.cpp"])

    def test_no_ignores(self):
        self.formatted(["a.cpp", "deep/b.hpp"], [], ["a.cpp", "deep/b.hpp"])

    def test_extensions_preserved(self):
        self.formatted(["a.cpp", "readme.txt"], [], ["a.cpp"])

    def test_input_directory_ignored(self):
        self.formatted(["skip/a.cpp"], ["skip"], [], "skip")

    def test_normalized_ignore_directory(self):
        self.formatted(
            ["skip/a.cpp", "skip-extra/b.cpp"], ["skip/../skip"], ["skip-extra/b.cpp"]
        )

    def test_single_file_input(self):
        self.formatted(["a.cpp"], [], ["a.cpp"], "a.cpp")

    def test_ignored_descendants_are_not_traversed(self):
        walk = os.walk
        visited = []

        def record_walk(path):
            for root, dirs, files in walk(path):
                visited.append(Path(root).name)
                yield root, dirs, files

        with patch("os.walk", side_effect=record_walk):
            self.formatted(
                ["skip/deep/a.cpp", "skip-extra/b.cpp"],
                ["skip"],
                ["skip-extra/b.cpp"],
            )
        self.assertIn("skip", visited)
        self.assertIn("skip-extra", visited)
        self.assertNotIn("deep", visited)


if __name__ == "__main__":
    unittest.main()
