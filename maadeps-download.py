#!/usr/bin/env python3
import os
import sys
import urllib.request
import urllib.error
import json
import time
import platform
from pathlib import Path
import shutil
import http.client

TARGET_TAG = "2024-08-17"
# FIXME: temporarily hold maadeps version for windows package
if platform.system() == "Windows":
    TARGET_TAG = "2024-05-30"
basedir = Path(__file__).parent


def detect_host_triplet():
    machine = platform.machine().lower()
    system = platform.system().lower()
    if machine in {"amd64", "x86_64"}:
        machine = "x64"
    elif machine in {"x86", "i386", "i486", "i586", "i686"}:
        machine = "x86"
    elif machine in {"armv7l", "armv7a", "arm", "arm32"}:
        machine = "arm"
    elif machine in {"arm64", "armv8l", "aarch64"}:
        machine = "arm64"
    else:
        raise Exception("unsupported architecture: " + machine)
    if system in {"windows", "linux"}:
        pass
    elif 'mingw' in system or 'cygwin' in system:
        system = "windows"
    elif system == "darwin":
        system = "osx"
    else:
        raise Exception("unsupported system: " + system)
    return f"{machine}-{system}"


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
                    f"\r [{self.downloaded / total * 100.0:3.1f}%] ",
                    f"{format_size(self.downloaded)} / {format_size(total)}",
                    "      \r",
                    end=''
                )
        if self.downloaded == total:
            print("")


def sanitize_filename(filename: str):
    system = platform.system()
    if system == "Windows":
        filename = filename.translate(
            str.maketrans("/\\:\"?*|\0", "________")
        ).rstrip('.')
    elif system == "Darwin":
        filename = filename.translate(str.maketrans("/:\0", "___"))
    else:
        filename = filename.translate(str.maketrans("/\0", "__"))
    return filename


def retry_urlopen(*args, **kwargs):
    for _ in range(5):
        try:
            resp: http.client.HTTPResponse = urllib.request.urlopen(*args,
                                                                    **kwargs)
            return resp
        except urllib.error.HTTPError as e:
            if (e.status == 403 and
                    e.headers.get("x-ratelimit-remaining") == "0"):
                # rate limit
                t0 = time.time()
                reset_time = t0 + 10
                try:
                    reset_time = int(e.headers.get("x-ratelimit-reset", 0))
                except ValueError:
                    pass
                reset_time = max(reset_time, t0 + 10)
                print(
                    "rate limit exceeded, retrying after ",
                    f"{reset_time - t0:.1f} seconds"
                )
                time.sleep(reset_time - t0)
                continue
            raise


def main():
    if len(sys.argv) >= 2:
        target_triplet = sys.argv[1]
    else:
        target_triplet = detect_host_triplet()

    if len(sys.argv) >= 3:
        target_tag = sys.argv[2]
    else:
        target_tag = TARGET_TAG
    print(
        "about to download prebuilt dependency libraries for ",
        f"{target_triplet} of {target_tag}"
    )
    if len(sys.argv) == 1:
        print(
            "to specify another triplet [and tag], ",
            f"run `{sys.argv[0]} <target triplet> [tag]`"
        )
        print(
            f"e.g. `{sys.argv[0]} arm64-windows` ",
            f"or `{sys.argv[0]} arm64-windows 2023-04-24-3`"
        )
    req = urllib.request.Request(
        "https://api.github.com/repos/MaaAssistantArknights/MaaDeps/releases"
    )
    token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    resp = retry_urlopen(req).read()
    releases = json.loads(resp)

    def split_asset_name(name: str):
        *remainder, component_suffix = name.rsplit('-', 1)
        component = component_suffix.split(".", 1)[0]
        if remainder:
            _, *target = remainder[0].split("-", 1)
            if target:
                return target[0], component
        return None, None
    devel_asset = None
    runtime_asset = None
    for release in releases:
        if release["tag_name"] != target_tag:
            continue
        for asset in release["assets"]:
            target, component = split_asset_name(asset["name"])
            if target == target_triplet:
                if component == 'devel':
                    devel_asset = asset
                elif component == 'runtime':
                    runtime_asset = asset
        if devel_asset and runtime_asset:
            break
    if devel_asset and runtime_asset:
        print("found assets:")
        print("    " + devel_asset["name"])
        print("    " + runtime_asset["name"])
        maadeps_dir = Path(basedir, "MaaDeps")
        download_dir = Path(maadeps_dir, "tarball")
        download_dir.mkdir(parents=True, exist_ok=True)
        for asset in [devel_asset, runtime_asset]:
            url = asset['browser_download_url']
            print("downloading from", url)
            local_file = download_dir / sanitize_filename(asset["name"])
            urllib.request.urlretrieve(url, local_file,
                                       reporthook=ProgressHook())
            print("extracting", asset["name"])
            shutil.unpack_archive(local_file, maadeps_dir)
    else:
        raise Exception(f"no binary release found for {target_triplet}")


if __name__ == "__main__":
    main()
