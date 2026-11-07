#!/usr/bin/env python3
r"""Regression tests against an actual Debug MaaCore (Python standard library only).

Offline API validation, without an emulator::

    python tools/SmokeTesting/test_startup_timeout.py --maa-dir build/bin/Debug

Opt-in timing tests, using screenshots only (no game launch, login or clicks)::

    python tools/SmokeTesting/test_startup_timeout.py --maa-dir build/bin/Debug \
        --adb /path/to/adb --address 127.0.0.1:16384

The directory must contain MaaCore and a compatible resource directory. No cache
or local patch is loaded. Timing tests use temporary resource overrides and take
one to several minutes, depending on screenshot transport. Native code runs in a
child process so its log handles are
closed before the temporary directory is removed, including on Windows.
"""

import argparse
import ctypes
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import time
import traceback


ERROR = 10000
STARTED = 10001
COMPLETED = 10002
STOPPED = 10004
TERMINAL = (ERROR, COMPLETED, STOPPED)
CALLBACK = ctypes.CFUNCTYPE(None, ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p)
ENTRY_TASKS = (
    "StartAtHome",
    "StartWithSanity",
    "SwitchTheme@ToggleSettingsMenu",
    "StartUpBegin",
    "SwitchAccount@StartUpBegin",
)


def check(condition, message):
    if not condition:
        raise AssertionError(message)


def encoded(value):
    return json.dumps(value, allow_nan=False).encode("utf-8")


def startup_params(timeout=120):
    return {
        "client_type": "Official",
        "start_game_enabled": False,
        "account_name": "",
        "startup_timeout": timeout,
    }


class Core:
    def __init__(self, root, scratch):
        names = {
            "win32": "MaaCore.dll",
            "darwin": "libMaaCore.dylib",
        }
        library = root / names.get(sys.platform, "libMaaCore.so")
        check(library.is_file(), f"Native library not found: {library}")
        check((root / "resource").is_dir(), f"Resource directory not found: {root}")
        os.chdir(root)
        self.log_path = scratch / "debug" / "asst.log"
        self.dll_directory = None
        if sys.platform == "win32":
            self.dll_directory = os.add_dll_directory(str(root))
            self.lib = ctypes.WinDLL(str(library))
        else:
            self.lib = ctypes.CDLL(str(library))
        signatures = {
            "AsstSetUserDir": (ctypes.c_bool, [ctypes.c_char_p]),
            "AsstLoadResource": (ctypes.c_bool, [ctypes.c_char_p]),
            "AsstGetVersion": (ctypes.c_char_p, []),
            "AsstSetConnectionExtras": (None, [ctypes.c_char_p, ctypes.c_char_p]),
            "AsstCreateEx": (ctypes.c_void_p, [CALLBACK, ctypes.c_void_p]),
            "AsstDestroy": (None, [ctypes.c_void_p]),
            "AsstAppendTask": (
                ctypes.c_int,
                [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p],
            ),
            "AsstSetTaskParams": (
                ctypes.c_bool,
                [ctypes.c_void_p, ctypes.c_int, ctypes.c_char_p],
            ),
            "AsstConnect": (
                ctypes.c_bool,
                [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p],
            ),
            "AsstStart": (ctypes.c_bool, [ctypes.c_void_p]),
            "AsstStop": (ctypes.c_bool, [ctypes.c_void_p]),
        }
        for name, (restype, argtypes) in signatures.items():
            function = getattr(self.lib, name)
            function.restype = restype
            function.argtypes = argtypes
        check(
            self.lib.AsstSetUserDir(str(scratch).encode("utf-8")),
            "AsstSetUserDir failed",
        )
        self.load(root)
        print(f"MaaCore: {self.lib.AsstGetVersion().decode('utf-8')}", flush=True)

    def load(self, directory):
        check(
            self.lib.AsstLoadResource(str(directory).encode("utf-8")),
            f"Resource loading failed: {directory}",
        )


class Instance:
    def __init__(self, core):
        self.core = core
        self.events = []
        self.started = threading.Event()
        self.finished = threading.Event()

        @CALLBACK
        def callback(message, raw, _):
            timestamp = time.monotonic()
            try:
                details = json.loads(raw.decode("utf-8"))
            except (ValueError, AttributeError, UnicodeError):
                return
            self.events.append((message, timestamp, details))
            if message == STARTED:
                self.started.set()
            elif message in TERMINAL:
                self.finished.set()

        self.callback = callback
        self.handle = core.lib.AsstCreateEx(self.callback, None)
        check(self.handle, "AsstCreateEx failed")

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.core.lib.AsstStop(self.handle)
        self.core.lib.AsstDestroy(self.handle)
        self.handle = None

    def append(self, kind, params):
        return self.core.lib.AsstAppendTask(self.handle, kind.encode(), encoded(params))

    def start(self):
        check(self.core.lib.AsstStart(self.handle), "AsstStart failed")
        check(self.started.wait(10), "No TaskChainStart callback")

    def wait(self, timeout):
        check(self.finished.wait(timeout), f"No terminal callback within {timeout}s")
        terminal = [event for event in self.events if event[0] in TERMINAL]
        check(len(terminal) == 1, f"Unexpected terminal callbacks: {terminal}")
        return terminal[0]

    def connect(self, args):
        if args.connection_extras is not None:
            self.core.lib.AsstSetConnectionExtras(
                args.config.encode("utf-8"), encoded(args.connection_extras)
            )
        check(
            self.core.lib.AsstConnect(
                self.handle,
                str(args.adb).encode("utf-8"),
                args.address.encode("utf-8"),
                args.config.encode("utf-8"),
            ),
            "Connection failed (keep the emulator running and idle for these tests)",
        )


def test_parameters(core):
    valid = (10, 120, 600)
    invalid = (None, True, False, "120", 120.0, 120.5, 9, 601, 2**32 + 120, 2**80)
    with Instance(core) as instance:
        omitted = startup_params()
        del omitted["startup_timeout"]
        task_id = instance.append("StartUp", omitted)
        check(task_id != 0, "Omitted startup_timeout was rejected")
        for value in valid:
            check(
                instance.append("StartUp", startup_params(value)) != 0,
                f"Rejected {value!r}",
            )
        for value in invalid:
            check(
                instance.append("StartUp", startup_params(value)) == 0,
                f"Accepted {value!r}",
            )
        for value in invalid:
            check(
                not core.lib.AsstSetTaskParams(
                    instance.handle, task_id, encoded(startup_params(value))
                ),
                f"AsstSetTaskParams accepted {value!r}",
            )
        for value in valid:
            check(
                core.lib.AsstSetTaskParams(
                    instance.handle, task_id, encoded(startup_params(value))
                ),
                f"AsstSetTaskParams rejected {value!r}",
            )
        check(
            core.lib.AsstSetTaskParams(instance.handle, task_id, encoded(omitted)),
            "AsstSetTaskParams rejected the omitted default",
        )
    print("PASS: API defaults, bounds, types and parameter updates", flush=True)


def write_overlay(root, scratch, action_delay=None, task_delay=500):
    overlay = scratch / "overlay"
    resources = overlay / "resource"
    tasks_dir = resources / "tasks"
    tasks_dir.mkdir(parents=True, exist_ok=True)
    # GeneralConfig requires the complete object. Keep all connection settings,
    # but normalize taskDelay so timing tests have known retry intervals.
    config = json.loads(
        (root / "resource" / "config.json").read_text(encoding="utf-8-sig")
    )
    config["options"]["taskDelay"] = task_delay
    (resources / "config.json").write_text(json.dumps(config), encoding="utf-8")
    never_matches = {
        "baseTask": "#none",
        "algorithm": "MatchTemplate",
        "template": "GameStart.png",
        "templThreshold": 1.1,
        "action": "DoNothing",
        "next": [],
        "onErrorNext": [],
    }
    tasks = {name: dict(never_matches) for name in ENTRY_TASKS}
    tasks["StartupTimeoutTestNeverMatches"] = dict(never_matches)
    if action_delay is not None:
        tasks["StartAtHome"] = {
            "baseTask": "#none",
            "algorithm": "JustReturn",
            "action": "DoNothing",
            "postDelay": action_delay,
            "next": ["StartupTimeoutTestNeverMatches"] if action_delay else [],
            "onErrorNext": [],
        }
    (tasks_dir / "startup-timeout-test.json").write_text(
        json.dumps(tasks), encoding="utf-8"
    )
    return overlay


def test_failure(core, args, kind, params, expected_seconds, label):
    with Instance(core) as instance:
        instance.connect(args)
        check(instance.append(kind, params) != 0, "Task append failed")
        instance.start()
        message, ended, _ = instance.wait(expected_seconds + 8)
        began = next(event[1] for event in instance.events if event[0] == STARTED)
        elapsed = ended - began
        check(message == ERROR, f"{label}: expected error, received {message}")
        check(
            expected_seconds - 0.15 <= elapsed <= expected_seconds + 5,
            f"{label}: expected about {expected_seconds}s, got {elapsed:.2f}s",
        )
    print(f"PASS: {label} ({elapsed:.2f}s)", flush=True)


def test_legacy_retry_limit(core, args):
    with Instance(core) as instance:
        instance.connect(args)
        task_id = instance.append(
            "Custom", {"task_names": ["StartupTimeoutTestNeverMatches"]}
        )
        check(task_id != 0, "Task append failed")
        instance.start()
        message, ended, _ = instance.wait(120)
        began = next(event[1] for event in instance.events if event[0] == STARTED)
        elapsed = ended - began
        check(message == ERROR, f"Legacy task should fail, received {message}")

    # Screenshot acquisition time varies widely across ADB/native transports.
    # The legacy contract is 21 attempts (initial + 20 retries), not ten seconds.
    attempts = []
    for log in (core.log_path.with_name("asst.bak.log"), core.log_path):
        if not log.exists():
            continue
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            brace = line.find("{")
            if brace < 0:
                continue
            try:
                record = json.loads(line[brace:])
            except ValueError:
                continue
            if (
                record.get("taskchain") == "Custom"
                and record.get("taskid") == task_id
                and record.get("first") == ["StartupTimeoutTestNeverMatches"]
                and "cur_retry" in record.get("details", {})
            ):
                attempts.append(record["details"])
    check(
        [attempt["cur_retry"] for attempt in attempts] == list(range(21)),
        f"Unexpected legacy retry sequence: {attempts}",
    )
    check(
        all(attempt.get("retry_times") == 20 for attempt in attempts),
        "Legacy retry limit changed",
    )
    check(
        all("recognition_timeout_ms" not in attempt for attempt in attempts),
        "Startup recognition timeout leaked into Custom tasks",
    )
    print(f"PASS: legacy Custom uses exactly 20 retries ({elapsed:.2f}s)", flush=True)


def test_immediate_success(core, args):
    with Instance(core) as instance:
        instance.connect(args)
        check(
            instance.append("StartUp", startup_params(120)) != 0, "Task append failed"
        )
        instance.start()
        message, ended, _ = instance.wait(2)
        began = next(event[1] for event in instance.events if event[0] == STARTED)
        elapsed = ended - began
        check(message == COMPLETED, f"Expected completed callback, received {message}")
        check(
            elapsed <= 2, f"Already-ready startup unnecessarily waited {elapsed:.2f}s"
        )
    print(
        f"PASS: successful recognition continues immediately ({elapsed:.2f}s)",
        flush=True,
    )


def test_cancellation(core, args):
    with Instance(core) as instance:
        instance.connect(args)
        check(
            instance.append("StartUp", startup_params(120)) != 0, "Task append failed"
        )
        instance.start()
        time.sleep(0.25)
        began = time.monotonic()
        check(core.lib.AsstStop(instance.handle), "AsstStop failed")
        message, ended, _ = instance.wait(2)
        elapsed = ended - began
        check(message == STOPPED, f"Expected stopped callback, received {message}")
        check(elapsed <= 2, f"Stopping took {elapsed:.2f}s (limit: 2s)")
    print(f"PASS: cancellation during the 120s wait ({elapsed:.2f}s)", flush=True)


def run_worker(args):
    scratch = args.worker_dir
    try:
        core = Core(args.maa_dir, scratch)
        test_parameters(core)
        if args.adb is None:
            print(
                "Timing tests skipped: pass both --adb and --address to enable.",
                flush=True,
            )
            return 0
        # A short retry interval exposes accidentally retaining the old 30-retry
        # account-navigation cap. The impossible entry never reaches account UI.
        core.load(write_overlay(args.maa_dir, scratch, task_delay=50))
        account_params = startup_params(10)
        account_params["account_name"] = "startup-timeout-test-account"
        test_failure(
            core, args, "StartUp", account_params, 10, "account navigation timeout 10s"
        )
        core.load(write_overlay(args.maa_dir, scratch))
        for seconds in (10, 15):
            test_failure(
                core,
                args,
                "StartUp",
                startup_params(seconds),
                seconds,
                f"timeout {seconds}s",
            )
        test_cancellation(core, args)
        test_legacy_retry_limit(core, args)
        core.load(write_overlay(args.maa_dir, scratch, action_delay=0))
        test_immediate_success(core, args)
        core.load(write_overlay(args.maa_dir, scratch, action_delay=11000))
        test_failure(
            core,
            args,
            "StartUp",
            startup_params(10),
            22,
            "fresh deadline after an 11s action",
        )
        return 0
    except Exception:
        traceback.print_exc()
        log = scratch / "debug" / "asst.log"
        if log.exists():
            print("Last native log lines:", flush=True)
            print(
                "\n".join(
                    log.read_text(encoding="utf-8", errors="replace").splitlines()[-30:]
                )
            )
        return 1


def output_tail(stdout, stderr):
    def lines(value):
        if isinstance(value, bytes):
            value = value.decode("utf-8", errors="replace")
        return (value or "").splitlines()

    for line in (lines(stdout) + lines(stderr))[-80:]:
        print(line, file=sys.stderr)


def main():
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--maa-dir", required=True, type=Path)
    parser.add_argument(
        "--adb", type=Path, help="Enable opt-in screenshot-only timing tests"
    )
    parser.add_argument("--address", help="ADB device address for timing tests")
    parser.add_argument(
        "--config", default="General", help="MAA connection profile (default: General)"
    )
    parser.add_argument(
        "--connection-extras",
        type=json.loads,
        help="Optional JSON object passed to AsstSetConnectionExtras for --config",
    )
    parser.add_argument("--worker-dir", type=Path, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if bool(args.adb) != bool(args.address):
        parser.error("--adb and --address must be provided together")
    if args.connection_extras is not None and not isinstance(
        args.connection_extras, dict
    ):
        parser.error("--connection-extras must be a JSON object")
    args.maa_dir = args.maa_dir.resolve()
    if args.adb:
        args.adb = args.adb.resolve()
        if not args.adb.is_file():
            parser.error(f"ADB executable not found: {args.adb}")
    if args.worker_dir:
        return run_worker(args)
    with tempfile.TemporaryDirectory(prefix="maa-startup-timeout-") as directory:
        command = [
            sys.executable,
            str(Path(__file__).resolve()),
            "--maa-dir",
            str(args.maa_dir),
        ]
        command += ["--worker-dir", directory, "--config", args.config]
        if args.adb:
            command += ["--adb", str(args.adb), "--address", args.address]
        if args.connection_extras is not None:
            command += ["--connection-extras", json.dumps(args.connection_extras)]
        environment = dict(os.environ, PYTHONIOENCODING="utf-8")
        try:
            result = subprocess.run(
                command,
                check=False,
                timeout=480,
                capture_output=True,
                encoding="utf-8",
                errors="replace",
                env=environment,
            )
            if result.returncode == 0:
                for line in result.stdout.splitlines():
                    if line.startswith(("PASS:", "MaaCore:", "Timing tests skipped:")):
                        print(line)
            else:
                output_tail(result.stdout, result.stderr)
            return result.returncode
        except subprocess.TimeoutExpired as error:
            output_tail(error.stdout, error.stderr)
            print("FAIL: native test process exceeded 480 seconds", file=sys.stderr)
            return 1


if __name__ == "__main__":
    raise SystemExit(main())
