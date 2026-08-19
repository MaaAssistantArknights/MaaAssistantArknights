#!/usr/bin/env python3
"""Download MaaFramework control unit binaries into a build output directory.

The MaaCore Win32 controller (Win32Controller) and the MaaFramework touch-mode
ADB controller (MaaFwAdbController) dynamically load "control unit" binaries
that are shipped by MaaFramework releases rather than built from this
repository. Without them the PC window binding (AttachWindow) and the
MaaFramework touch mode fail to initialize.

This script mirrors what CI does (see .github/workflows/ci.yml): fetch the
MaaFramework release archive matching the host platform and copy the control
unit binaries (MaaWin32ControlUnit.dll, MaaAdbControlUnit.dll, ...) out of its
``bin`` directory into the target directory. The download is skipped when the
expected control units are already present unless ``--force`` is given.

Usage:
    python tools/maafw-control-unit-download.py [--tag v5.9.2] [--output-dir DIR] [--force]

Note: these are Release builds of MaaFramework. They are ABI-compatible with
Release/RelWithDebInfo builds of MAA, but NOT with Debug builds of MAA (MSVC
debug/release STL layouts differ and mixing them crashes). For Debug builds you
have to compile the Debug version of MaaFramework yourself, see
docs/zh-cn/develop/development.md. If a target control unit is locked by a
running MAA process, the script skips it with a note when it is already
present, or fails with a clear message otherwise — close MAA before replacing
them.
"""

import argparse
import hashlib
import json
import os
import platform
import shutil
import sys
import tempfile
import time
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

REPO = "MaaXYZ/MaaFramework"
# Keep in sync with the pin used in .github/workflows/ci.yml
DEFAULT_TAG = "v5.9.2"

ROOT = Path(__file__).resolve().parent.parent


def format_size(num, suffix="B"):
    for unit in ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
        if abs(num) < 1024.0:
            return f"{num:3.1f}{unit}{suffix}"
        num /= 1024.0
    return f"{num:.1f}Yi{suffix}"


class ProgressHook:
    def __init__(self):
        self.downloaded = 0
        self.last_print = 0

    def __call__(self, block, chunk, total):
        self.downloaded += chunk
        t = time.monotonic()
        if t - self.last_print >= 0.5 or self.downloaded == total:
            self.last_print = t
            if total > 0:
                print(
                    f"\r [{self.downloaded / total * 100.0:3.1f}%] {format_size(self.downloaded)} / {format_size(total)}      \r",
                    end="",
                )
        if self.downloaded == total:
            print("")


def host_platform():
    """Return (platform, arch) in MaaFramework asset naming, e.g. ("win", "x86_64")."""
    system = platform.system().lower()
    if system == "windows":
        system = "win"
    elif system == "darwin":
        system = "macos"
    elif system == "linux":
        system = "linux"
    elif "mingw" in system or "cygwin" in system:
        system = "win"
    else:
        raise RuntimeError(f"unsupported system: {system}")

    machine = platform.machine().lower()
    if machine in {"amd64", "x86_64"}:
        arch = "x86_64"
    elif machine in {"aarch64", "arm64", "armv8l"}:
        arch = "aarch64"
    else:
        raise RuntimeError(f"unsupported architecture: {machine}")

    return system, arch


def expected_control_units(platform_name: str):
    """Control unit names whose presence means the download can be skipped."""
    if platform_name == "win":
        return ["MaaWin32ControlUnit.dll", "MaaAdbControlUnit.dll"]
    if platform_name == "macos":
        return ["libMaaAdbControlUnit.dylib"]
    return ["libMaaAdbControlUnit.so"]


def query_release(tag: str):
    """Return the release JSON from the GitHub API, or None on failure."""
    req = urllib.request.Request(
        f"https://api.github.com/repos/{REPO}/releases/tags/{tag}"
    )
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=30) as resp:
            return json.loads(resp.read())
    except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError) as e:
        print(
            f"warning: failed to query GitHub API ({e}), will construct the download URL directly"
        )
        return None


def find_asset(release, platform_name: str, arch: str, tag: str):
    """Find the archive asset for the host platform."""
    needle = f"-{platform_name}-{arch}-{tag}.zip"
    if release:
        for asset in release.get("assets", []):
            if asset["name"].endswith(".zip") and needle in asset["name"]:
                return asset
        available = ", ".join(a["name"] for a in release.get("assets", []))
        raise RuntimeError(
            f"no asset matching {needle} in {REPO} {tag}; available: {available or '(none)'}"
        )
    # Fallback: the asset name is regular, build the direct download URL.
    return {
        "name": f"MAA-{platform_name}-{arch}-{tag}.zip",
        "browser_download_url": f"https://github.com/{REPO}/releases/download/{tag}/MAA-{platform_name}-{arch}-{tag}.zip",
        "digest": None,
    }


def sha256_of(path: Path) -> str:
    hasher = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            chunk = f.read(1024 * 1024)
            if not chunk:
                break
            hasher.update(chunk)
    return hasher.hexdigest()


def verify_digest(path: Path, digest):
    if not digest:
        print("note: no digest available from the release, skipped integrity check")
        return
    if not digest.startswith("sha256:"):
        print("warning: unsupported digest format:", digest)
        return
    expected = digest[len("sha256:") :]
    actual = sha256_of(path)
    if actual != expected:
        raise RuntimeError(
            f"sha256 mismatch for {path.name}: expected {expected}, got {actual}"
        )
    print("sha256 verified:", actual)


def download_asset(asset, dest: Path):
    url = asset["browser_download_url"]
    print("downloading from", url)
    urllib.request.urlretrieve(url, dest, reporthook=ProgressHook())
    verify_digest(dest, asset.get("digest"))


def extract_control_units(archive: Path, output_dir: Path, force: bool = False):
    staging = Path(tempfile.mkdtemp(prefix="maafw-"))
    try:
        print("extracting", archive.name)
        with zipfile.ZipFile(archive) as zf:
            zf.extractall(staging)
        bin_dir = staging / "bin"
        if not bin_dir.is_dir():
            raise RuntimeError(
                f"archive does not contain a bin/ directory: {archive.name}"
            )
        copied = []
        for entry in sorted(bin_dir.iterdir()):
            if entry.is_file() and "ControlUnit" in entry.name:
                dst = output_dir / entry.name
                try:
                    shutil.copy2(entry, dst)
                    copied.append(entry.name)
                except PermissionError:
                    # A running MAA process keeps the target DLL loaded and locked.
                    # If it is already present, the provisioning goal is met; only
                    # fail loudly when the user asked to replace it or it is missing.
                    if not force and dst.exists():
                        print(
                            f"note: {entry.name} is in use (a running MAA process?) "
                            f"and already present, skipped"
                        )
                    else:
                        raise RuntimeError(
                            f"cannot copy {entry.name} into {output_dir}: the target file "
                            f"is in use by a running MAA process. Close MAA and retry."
                        )
        if not copied:
            raise RuntimeError(f"no control unit binaries found in {bin_dir}")
        return copied
    finally:
        shutil.rmtree(staging, ignore_errors=True)


def detect_output_dir():
    """Pick the newest existing build output directory under build/bin."""
    base = ROOT / "build" / "bin"
    if not base.is_dir():
        return None
    candidates = []
    for d in base.iterdir():
        if d.is_dir() and any(
            (d / name).exists()
            for name in ("MAA.exe", "MaaCore.dll", "libMaaCore.dylib")
        ):
            candidates.append(d)
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--tag",
        default=DEFAULT_TAG,
        help=f"MaaFramework release tag (default: {DEFAULT_TAG})",
    )
    parser.add_argument(
        "--output-dir",
        default=None,
        help="directory to place the control units into (default: newest dir under build/bin)",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="re-download even if the control units already exist",
    )
    args = parser.parse_args()

    platform_name, arch = host_platform()
    print(f"host platform: {platform_name}-{arch}, MaaFramework tag: {args.tag}")

    if args.output_dir:
        output_dir = Path(args.output_dir)
    else:
        output_dir = detect_output_dir()
        if output_dir is None:
            parser.error(
                "could not find a build output directory; build the project first or pass --output-dir"
            )
        print("target output directory:", output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    if not args.force:
        missing = [
            name
            for name in expected_control_units(platform_name)
            if not (output_dir / name).exists()
        ]
        if not missing:
            print(
                "control units already present, nothing to do (use --force to re-download)"
            )
            return 0

    release = query_release(args.tag)
    asset = find_asset(release, platform_name, arch, args.tag)
    print("asset:", asset["name"])

    with tempfile.TemporaryDirectory(prefix="maafw-dl-") as tmp:
        archive = Path(tmp) / asset["name"]
        download_asset(asset, archive)
        copied = extract_control_units(archive, output_dir, args.force)

    print("copied control units into", output_dir)
    for name in copied:
        print("  -", name)

    if platform_name == "win" and missing:
        print(
            "note: these are Release builds; use them with Release/RelWithDebInfo builds of MAA, not Debug"
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
