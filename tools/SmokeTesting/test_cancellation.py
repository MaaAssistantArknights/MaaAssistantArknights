"""Windows NativeIO regression: real child processes, no emulator or ADB server.

Build MaaCore in Debug and compile FakeAdb.cs with the .NET Framework csc:
  csc /r:System.Drawing.dll /out:fake-adb.exe FakeAdb.cs
Run with --core-dir, --resource-dir, --fake-adb and --work-dir absolute paths.
The resource directory must contain resource/config.json. Logs stay in work-dir.
"""

import argparse
import atexit
import ctypes as C
import json
import os
from pathlib import Path
import threading
import time


def wait_until(predicate, message, timeout=3):
    deadline = time.monotonic() + timeout
    while not predicate():
        assert time.monotonic() < deadline, message
        time.sleep(0.01)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ("core-dir", "resource-dir", "fake-adb", "work-dir"):
        parser.add_argument("--" + option, type=Path, required=True)
    parser.add_argument("--iterations", type=int, default=10)
    args = parser.parse_args()
    root = args.work_dir.resolve()
    root.mkdir(parents=True, exist_ok=True)
    os.environ["MAA_FAKE_ADB_ROOT"] = str(root)
    dll_directory = os.add_dll_directory(str(args.core_dir.resolve()))
    lib = C.WinDLL(str(args.core_dir.resolve() / "MaaCore.dll"))
    callback_type = C.WINFUNCTYPE(None, C.c_int32, C.c_char_p, C.c_void_p)
    signatures = {
        "AsstSetUserDir": (C.c_uint8, [C.c_char_p]),
        "AsstLoadResource": (C.c_uint8, [C.c_char_p]),
        "AsstCreateEx": (C.c_void_p, [callback_type, C.c_void_p]),
        "AsstSetInstanceOption": (C.c_uint8, [C.c_void_p, C.c_int32, C.c_char_p]),
        "AsstConnect": (C.c_uint8, [C.c_void_p, C.c_char_p, C.c_char_p, C.c_char_p]),
        "AsstAsyncConnect": (
            C.c_int32,
            [C.c_void_p, C.c_char_p, C.c_char_p, C.c_char_p, C.c_uint8],
        ),
        "AsstAppendTask": (C.c_int32, [C.c_void_p, C.c_char_p, C.c_char_p]),
        "AsstStop": (C.c_uint8, [C.c_void_p]),
        "AsstStart": (C.c_uint8, [C.c_void_p]),
        "AsstRunning": (C.c_uint8, [C.c_void_p]),
        "AsstDestroy": (None, [C.c_void_p]),
    }
    for name, (result, arguments) in signatures.items():
        function = getattr(lib, name)
        function.restype, function.argtypes = result, arguments
    kernel = C.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = [C.c_uint32, C.c_int, C.c_uint32]
    kernel.OpenProcess.restype = C.c_void_p
    kernel.WaitForSingleObject.argtypes = [C.c_void_p, C.c_uint32]
    kernel.CloseHandle.argtypes = [C.c_void_p]
    kernel.TerminateProcess.argtypes = [C.c_void_p, C.c_uint32]
    assert lib.AsstSetUserDir(str(root).encode())
    assert lib.AsstLoadResource(str(args.resource_dir.resolve()).encode())
    config = json.loads(
        (args.resource_dir / "resource/config.json").read_text(encoding="utf-8-sig")
    )
    general = next(
        item for item in config["connection"] if item["configName"] == "General"
    )
    general["screencapRawByNC"] = ""
    general["screencapRawWithGzip"] = ""
    (root / "resource").mkdir(exist_ok=True)
    (root / "resource/config.json").write_text(json.dumps(config), encoding="utf-8")
    assert lib.AsstLoadResource(str(root).encode())
    fake = str(args.fake_adb.resolve()).encode()
    events = []
    children = []
    results = []

    def cleanup():
        for process in children:
            if kernel.WaitForSingleObject(process, 0) == 258:
                print(
                    "Cleaning up a failed test's retained fake-ADB process handle",
                    flush=True,
                )
                kernel.TerminateProcess(process, 1)
                kernel.WaitForSingleObject(process, 3000)
            kernel.CloseHandle(process)

    atexit.register(cleanup)

    @callback_type
    def callback(message, details, _):
        events.append((message, json.loads(details)))

    def create():
        handle = lib.AsstCreateEx(callback, None)
        assert handle
        for key, value in ((2, b"adb"), (4, b"0"), (5, b"0")):
            assert lib.AsstSetInstanceOption(handle, key, value)
        return handle

    def mode(value):
        (root / "fake-mode.txt").write_text(value)
        (root / "fake-task-started").unlink(missing_ok=True)
        (root / "fake-adb.log").write_text("")

    def calls():
        return (root / "fake-adb.log").read_text().splitlines()

    def blocked_child():
        wait_until(
            lambda: any("\tBLOCKED\t" in line for line in calls()),
            "fake ADB did not block",
            10,
        )
        pid = int(
            next(line.split("\t")[1] for line in calls() if "\tBLOCKED\t" in line)
        )
        process = kernel.OpenProcess(0x100001, False, pid)
        assert process, "could not retain the blocked child's process handle"
        children.append(process)
        return process

    def connect(handle, asynchronous=False):
        params = (handle, fake, b"127.0.0.1:65530", b"General")
        return (
            lib.AsstAsyncConnect(*params, 0)
            if asynchronous
            else lib.AsstConnect(*params)
        )

    def restart(handle):
        mode("healthy")
        assert connect(handle), "same-instance reconnect failed"
        completed = sum(msg == 3 for msg, _ in events)
        assert lib.AsstAppendTask(handle, b"Debug", b"{}") > 0
        assert lib.AsstStart(handle), "restart failed"
        wait_until(
            lambda: sum(msg == 3 for msg, _ in events) > completed,
            "restarted task did not finish",
        )
        assert not lib.AsstRunning(handle)

    def stop(handle, process, label):
        before = calls()
        started = time.monotonic()
        assert lib.AsstStop(handle)
        assert lib.AsstStop(handle)
        wait_until(lambda: not lib.AsstRunning(handle), label + " did not become idle")
        elapsed = (time.monotonic() - started) * 1000
        assert kernel.WaitForSingleObject(process, 0) == 0, (
            label + " left the child alive"
        )
        assert calls() == before, label + " sent a command after cancellation"
        results.append({"case": label, "milliseconds": round(elapsed, 2)})

    def destroy(handle, process, label):
        before = calls()
        started = time.monotonic()
        worker = threading.Thread(target=lib.AsstDestroy, args=(handle,), daemon=True)
        worker.start()
        worker.join(3)
        assert not worker.is_alive(), label + " blocked destruction"
        assert kernel.WaitForSingleObject(process, 0) == 0, (
            label + " left the child alive"
        )
        assert calls() == before, label + " sent a command during destruction"
        results.append(
            {
                "case": label,
                "milliseconds": round((time.monotonic() - started) * 1000, 2),
            }
        )

    handle = create()
    for _ in range(args.iterations):
        mode("connect_hang")
        first = connect(handle, True)
        assert first > 0
        process = blocked_child()
        queued = connect(handle, True)
        assert queued > first
        stop(handle, process, "connect-stop")
        wait_until(
            lambda: any(
                msg == 4 and data["async_call_id"] == queued for msg, data in events
            ),
            "queued connect was not settled",
        )
        assert len([line for line in calls() if "\tCALL\t" in line]) == 1, (
            "queued connect survived Stop"
        )
        restart(handle)
        mode("work_hang")
        assert (
            lib.AsstAppendTask(
                handle,
                b"StartUp",
                b'{"start_game_enabled":true,"client_type":"Official"}',
            )
            > 0
        )
        assert lib.AsstStart(handle)
        process = blocked_child()
        stop(handle, process, "work-stop")
        restart(handle)
    lib.AsstDestroy(handle)

    for stage in ("connect", "work"):
        handle = create()
        mode("healthy")
        if stage == "work":
            assert connect(handle)
            mode("work_hang")
            assert (
                lib.AsstAppendTask(
                    handle,
                    b"StartUp",
                    b'{"start_game_enabled":true,"client_type":"Official"}',
                )
                > 0
            )
            assert lib.AsstStart(handle)
        else:
            mode("connect_hang")
            assert connect(handle, True) > 0
        process = blocked_child()
        destroy(handle, process, stage + "-destroy")

    # The GUI uses synchronous Connect on a background thread. Destroy must also join this I/O.
    handle = create()
    mode("connect_hang")
    worker = threading.Thread(target=connect, args=(handle,), daemon=True)
    worker.start()
    process = blocked_child()
    destroy(handle, process, "sync-connect-destroy")
    worker.join(3)
    assert not worker.is_alive()
    for process in children:
        kernel.CloseHandle(process)
    children.clear()
    (root / "results.json").write_text(json.dumps(results, indent=2))
    print("CANCELLATION PASSED " + json.dumps(results), flush=True)
    dll_directory.close()


if __name__ == "__main__":
    main()
