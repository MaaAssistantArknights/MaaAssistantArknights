"""Run with python3 tools/SmokeTesting/test_androws_mac.py (no device required)."""

import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[2]
CONFIG = json.loads((ROOT / "resource/config.json").read_text(encoding="utf-8"))
PROFILES = {item["configName"]: item for item in CONFIG["connection"]}


class AndrowsMacTest(unittest.TestCase):
    def query(
        self, activities, package="com.hypergryph.arknights", profile="AndrowsMac"
    ):
        # Execute both shell layers: macOS host -> adb shell -> Android command.
        with tempfile.TemporaryDirectory(prefix="maa adb ") as directory:
            directory = Path(directory)
            fixture = directory / "activities.txt"
            fixture.write_text(activities, encoding="utf-8")
            adb = directory / "adb"
            adb.write_text(
                '#!/bin/sh\nshift 2\n[ "$1" = shell ] || exit 1\nshift\nexec /bin/sh -c "$*"\n'
            )
            dumpsys = directory / "dumpsys"
            dumpsys.write_text('#!/bin/sh\ncat "$MAA_ACTIVITIES"\n')
            adb.chmod(0o755)
            dumpsys.chmod(0o755)
            command = (
                PROFILES[profile]["displayId"]
                .replace("[Adb]", shlex.quote(str(adb)))
                .replace("[AdbSerial]", "emulator-5554")
                .replace("[PackageName]", package)
            )
            env = dict(os.environ, MAA_ACTIVITIES=str(fixture))
            env["PATH"] = str(directory) + os.pathsep + env["PATH"]
            return subprocess.check_output(
                command, shell=True, executable="/bin/sh", env=env, text=True
            )

    def test_dynamic_display(self):
        for display in range(1, 7):
            with self.subTest(display=display):
                fixture = (
                    "Display #0 (activities from top to bottom):\n"
                    "  packageName=com.yyb.launcher processName=com.yyb.launcher\n"
                    f"Display #{display} (activities from top to bottom):\n"
                    "  packageName=com.hypergryph.arknights processName=com.hypergryph.arknights\n"
                )
                self.assertEqual(self.query(fixture), f"mDisplayId:{display}\n")

    def test_client_channels_and_package_prefix(self):
        fixture = (
            "Display #5 (activities from top to bottom):\n"
            "  packageName=com.hypergryph.arknights.bilibili processName=game\n"
            "Display #3 (activities from top to bottom):\n"
            "  packageName=com.hypergryph.arknights\n"
        )
        self.assertEqual(self.query(fixture), "mDisplayId:3\n")
        self.assertEqual(
            self.query(fixture, "com.hypergryph.arknights.bilibili"), "mDisplayId:5\n"
        )

    def test_missing_game_does_not_select_launcher(self):
        fixture = "Display #0 (activities from top to bottom):\n  packageName=com.yyb.launcher\n"
        self.assertEqual(self.query(fixture), "")

    def test_multiple_matching_activities_return_one_display(self):
        fixture = (
            "Display #3 (activities from top to bottom):\n"
            "  packageName=com.hypergryph.arknights\n"
            "  packageName=com.hypergryph.arknights\n"
            "Display #2 (activities from top to bottom):\n"
            "  packageName=com.hypergryph.arknights\n"
        )
        self.assertEqual(self.query(fixture), "mDisplayId:3\n")

    def test_host_shell_regression(self):
        fixture = "Display #3 (activities from top to bottom):\n  packageName=com.hypergryph.arknights\n"
        self.assertNotEqual(self.query(fixture, profile="Androws"), "mDisplayId:3\n")
        self.assertEqual(self.query(fixture), "mDisplayId:3\n")

    def test_display_targeted_commands(self):
        resolved = {}
        profile = "AndrowsMac"
        while profile:
            entry = PROFILES[profile]
            resolved = entry | resolved
            profile = entry.get("baseConfig")
        for field in (
            "display",
            "screencapRawByNC",
            "screencapRawWithGzip",
            "screencapEncode",
            "click",
            "swipe",
            "pressEsc",
            "input",
        ):
            with self.subTest(field=field):
                self.assertIn("-d [DisplayId]", resolved[field])
        self.assertEqual(resolved["eventId"], "")


if __name__ == "__main__":
    unittest.main()
