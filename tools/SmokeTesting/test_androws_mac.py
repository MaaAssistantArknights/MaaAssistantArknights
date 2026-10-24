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
DISPLAY_COMMAND_PREFIXES = {
    "display": ["wm", "size", "-d", "4"],
    "screencapRawByNC": ["screencap", "-d", "4"],
    "screencapRawWithGzip": ["screencap", "-d", "4"],
    "screencapEncode": ["screencap", "-d", "4", "-p"],
    "click": ["input", "-d", "4", "tap", "100", "200"],
    "swipe": ["input", "-d", "4", "swipe", "100", "200", "300", "400", "500"],
    "pressEsc": ["input", "-d", "4", "keyevent", "111"],
    "input": ["input", "-d", "4", "text", "maa-test"],
}


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

    def capture_shell_arguments(self, command):
        # Run the real host-shell command, but record argv instead of contacting a device.
        with tempfile.TemporaryDirectory(prefix="maa adb ") as directory:
            directory = Path(directory)
            adb = directory / "adb"
            argv_file = directory / "argv"
            adb.write_text('#!/bin/sh\nprintf "%s\\0" "$@" > "$MAA_ARGV"\n')
            adb.chmod(0o755)
            replacements = {
                "[Adb]": shlex.quote(str(adb)),
                "[AdbSerial]": "emulator-5554",
                "[DisplayId]": "4",
                "[x]": "100",
                "[y]": "200",
                "[x1]": "100",
                "[y1]": "200",
                "[x2]": "300",
                "[y2]": "400",
                "[duration]": "500",
                "[text]": "maa-test",
                "[NcAddress]": "127.0.0.1",
                "[NcPort]": "12345",
            }
            for token, value in replacements.items():
                command = command.replace(token, value)
            subprocess.run(
                command,
                shell=True,
                executable="/bin/sh",
                env=dict(os.environ, MAA_ARGV=str(argv_file)),
                check=True,
            )
            argv = argv_file.read_bytes().decode().split("\0")[:-1]
        self.assertEqual(argv[:3], ["-s", "emulator-5554", "shell"])
        # A quoted Android pipeline is one adb argument; unquoted input uses several.
        return shlex.split(argv[3]) if len(argv) == 4 else argv[3:]

    def assert_display_targeted_command(self, field, command):
        argv = self.capture_shell_arguments(command)
        prefix = DISPLAY_COMMAND_PREFIXES[field]
        self.assertEqual(argv[: len(prefix)], prefix)
        if field in ("click", "swipe", "pressEsc", "input", "screencapEncode"):
            self.assertEqual(argv, prefix)

    def test_display_targeted_commands(self):
        resolved = {}
        profile = "AndrowsMac"
        while profile:
            entry = PROFILES[profile]
            resolved = entry | resolved
            profile = entry.get("baseConfig")
        for field in DISPLAY_COMMAND_PREFIXES:
            with self.subTest(field=field):
                self.assert_display_targeted_command(field, resolved[field])
        self.assertEqual(resolved["eventId"], "")

    def test_rejects_misplaced_display_flags(self):
        invalid_commands = {
            "display": '"wm size | grep -o -E [0-9]+ -d [DisplayId]"',
            "screencapRawByNC": '"screencap | nc -w 3 [NcAddress] [NcPort] -d [DisplayId]"',
            "screencapRawWithGzip": '"screencap | gzip -1 -d [DisplayId]"',
            "click": "input tap [x] [y] -d [DisplayId]",
            "swipe": "input swipe [x1] [y1] [x2] [y2] [duration] -d [DisplayId]",
            "pressEsc": "input keyevent 111 -d [DisplayId]",
            "input": "input text [text] -d [DisplayId]",
        }
        for field, shell_command in invalid_commands.items():
            with self.subTest(field=field):
                command = "[Adb] -s [AdbSerial] shell " + shell_command
                with self.assertRaises(AssertionError):
                    self.assert_display_targeted_command(field, command)

    def test_rejects_display_flag_for_another_operation(self):
        command = "[Adb] -s [AdbSerial] shell echo screencap -d [DisplayId] -p"
        with self.assertRaises(AssertionError):
            self.assert_display_targeted_command("screencapEncode", command)


if __name__ == "__main__":
    unittest.main()
