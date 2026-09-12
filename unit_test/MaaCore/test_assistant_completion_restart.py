"""Start again immediately after completion, using disabled tasks and no device."""

import ctypes
import os
from pathlib import Path
import sys
import threading


def main():
    root = Path(__file__).resolve().parents[2]
    binary = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build/bin/Release"
    output = root / "build/requirement-review/restart"
    output.mkdir(parents=True, exist_ok=True)
    dll_directory = os.add_dll_directory(str(binary.resolve()))
    core = ctypes.CDLL(str((binary / "MaaCore.dll").resolve()))
    callback_type = ctypes.CFUNCTYPE(
        None, ctypes.c_int, ctypes.c_char_p, ctypes.c_void_p
    )
    signatures = {
        "AsstSetUserDir": ([ctypes.c_char_p], ctypes.c_bool),
        "AsstLoadResource": ([ctypes.c_char_p], ctypes.c_bool),
        "AsstCreateEx": ([callback_type, ctypes.c_void_p], ctypes.c_void_p),
        "AsstAppendTask": (
            [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_char_p],
            ctypes.c_int,
        ),
        "AsstStart": ([ctypes.c_void_p], ctypes.c_bool),
        "AsstRunning": ([ctypes.c_void_p], ctypes.c_bool),
        "AsstStop": ([ctypes.c_void_p], ctypes.c_bool),
        "AsstDestroy": ([ctypes.c_void_p], None),
    }
    for name, (args, result) in signatures.items():
        function = getattr(core, name)
        function.argtypes, function.restype = args, result
    completed = threading.Event()

    @callback_type
    def callback(message, details, custom):
        if message == 3:  # AllTasksCompleted
            completed.set()

    assert core.AsstSetUserDir(str(output).encode())
    assert core.AsstLoadResource(str(root).encode())
    handle = core.AsstCreateEx(callback, None)
    assert handle
    try:
        for iteration in range(100):
            completed.clear()
            # Disabled tasks exercise scheduling without screenshots or input.
            assert core.AsstAppendTask(
                handle, b"Custom", b'{"task_names":["Stop"],"enable":false}'
            )
            assert core.AsstStart(handle), f"Restart failed at round {iteration}"
            assert completed.wait(5), f"Completion missing at round {iteration}"
            assert not core.AsstRunning(handle), (
                f"Completion published before idle at round {iteration}"
            )
        print("100 immediate completion/restart cycles passed; no device connected")
    finally:
        core.AsstStop(handle)
        core.AsstDestroy(handle)
        dll_directory.close()


if __name__ == "__main__":
    main()
