"""Focused path-boundary checks, without requiring a Windows host or clang-format.

The actual directory-ignore predicate is extracted from the script's AST and
executed with POSIX and Windows path modules. These checks do not exercise a
Windows filesystem or the external formatter.
"""

import ast
import ntpath
from pathlib import Path
import posixpath
from types import SimpleNamespace
import unittest


DIRECTORY = Path(__file__).parent


def load_predicate(filename):
    tree = ast.parse((DIRECTORY / filename).read_text(encoding="utf-8"))
    for node in ast.walk(tree):
        if (
            isinstance(node, ast.For)
            and isinstance(node.target, ast.Tuple)
            and [name.id for name in node.target.elts] == ["root", "dirs", "files"]
        ):
            return compile(ast.Expression(node.body[0].test), filename, "eval")
    raise AssertionError("Could not locate the directory-ignore predicate")


PREDICATE = load_predicate("clang-formatter.py")


class FormatterPathBoundaryTests(unittest.TestCase):
    def assert_ignored(self, path_module, root, ignore, expected):
        actual = eval(
            PREDICATE,
            {
                "os": SimpleNamespace(path=path_module),
                "root": root,
                "ignore_dirs": [path_module.normpath(ignore)],
            },
        )
        self.assertEqual(actual, expected, (path_module.__name__, root, ignore))

    def test_posix_relative_directory_boundary(self):
        for root, expected in [
            ("skip", True),
            ("skip/deep", True),
            ("skip-extra", False),
            ("skip-extra/deep", False),
            ("skip2", False),
        ]:
            with self.subTest(root=root):
                self.assert_ignored(posixpath, root, "skip", expected)

    def test_posix_absolute_directory_boundary(self):
        for root, expected in [
            ("/work/skip", True),
            ("/work/skip/deep", True),
            ("/work/skip-extra", False),
        ]:
            with self.subTest(root=root):
                self.assert_ignored(posixpath, root, "/work/skip", expected)

    def test_posix_normalization(self):
        for root, expected in [
            ("./skip/./deep", True),
            ("./skip-extra/./deep", False),
            ("skip/../keep", False),
        ]:
            with self.subTest(root=root):
                self.assert_ignored(posixpath, root, "./skip/../skip/", expected)

    def test_posix_root_directories(self):
        for root, ignore in [("/", "/"), ("/work/skip", "/"), ("//server/share", "//")]:
            with self.subTest(root=root, ignore=ignore):
                self.assert_ignored(posixpath, root, ignore, True)

    def test_windows_directory_boundaries(self):
        for prefix in ["", "C:", "C:\\", "\\\\server\\share\\"]:
            ignore = prefix + "skip"
            for suffix, expected in [
                ("", True),
                ("\\deep", True),
                ("-extra", False),
                ("2", False),
            ]:
                root = ignore + suffix
                with self.subTest(root=root, ignore=ignore):
                    self.assert_ignored(ntpath, root, ignore, expected)

    def test_windows_roots(self):
        for ignore in ["C:\\", "\\", "\\\\server\\share"]:
            for root in [ignore, ntpath.join(ignore, "deep")]:
                with self.subTest(root=root, ignore=ignore):
                    self.assert_ignored(ntpath, root, ignore, True)

    def test_windows_normalization_and_alternate_separator(self):
        for root, expected in [
            ("C:/work/skip/./deep", True),
            ("C:/work/skip-extra/deep", False),
            ("C:/work/skip/../keep", False),
        ]:
            with self.subTest(root=root):
                self.assert_ignored(ntpath, root, "C:/work/skip/../skip/", expected)


if __name__ == "__main__":
    unittest.main()
