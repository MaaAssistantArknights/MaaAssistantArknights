#!/usr/bin/env python3
import os
import sys
import urllib.request
import urllib.error
import json
import time
from pathlib import Path
import shutil

basedir = Path(__file__).parent
maadeps_dir = Path(basedir, "..", "MaaDeps")
download_dir = Path(maadeps_dir, "tarball")

def detect_host_arch():
    import platform
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
    if system != "linux":
        raise Exception("must on linux")
    return machine

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
                print(f"\r [{self.downloaded / total * 100.0:3.1f}%] {format_size(self.downloaded)} / {format_size(total)}      \r", end='')
        if self.downloaded == total:
            print("")
        

def sanitize_filename(filename: str):
    import platform
    system = platform.system()
    if system == "Windows":
        filename = filename.translate(str.maketrans("/\\:\"?*|\0", "________")).rstrip('.')
    elif system == "Darwin":
        filename = filename.translate(str.maketrans("/:\0", "___"))
    else:
        filename = filename.translate(str.maketrans("/\0", "__"))
    return filename


def retry_urlopen(*args, **kwargs):
    import time
    import http.client
    for _ in range(5):
        try:
            resp: http.client.HTTPResponse = urllib.request.urlopen(*args, **kwargs)
            return resp
        except urllib.error.HTTPError as e:
            if e.status == 403 and e.headers.get("x-ratelimit-remaining") == "0":
                # rate limit
                t0 = time.time()
                reset_time = t0 + 10
                try:
                    reset_time = int(e.headers.get("x-ratelimit-reset", 0))
                except ValueError:
                    pass
                reset_time = max(reset_time, t0 + 10)
                print(f"rate limit exceeded, retrying after {reset_time - t0:.1f} seconds")
                time.sleep(reset_time - t0)
                continue
            raise


def main():
    if len(sys.argv) == 2:
        target_arch = sys.argv[1]
    else:
        target_arch = detect_host_arch()
    print("about to download prebuilt dependency libraries for", target_arch)
    # if len(sys.argv) == 1:
    #     print(f"to specify another triplet, run `{sys.argv[0]} <target triplet>`")
    #     print(f"e.g. `{sys.argv[0]} x64-windows`")
    req = urllib.request.Request("https://api.github.com/repos/MaaXYZ/MaaLinuxToolchain/releases/latest")
    token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    resp = retry_urlopen(req).read()
    release = json.loads(resp)

    def split_asset_name(name: str):
        if not name.endswith('.tar.xz'):
            return None
        name = name.replace('.tar.xz', '')
        *remainder, architecture = name.rsplit('-', 1)
        print(f'{name.ljust(29)} arch:{architecture}')
        if architecture in { 'x64', 'arm64' }:
            return architecture
        return None
    toolchain_asset = None
    for asset in release["assets"]:
        arch = split_asset_name(asset["name"])
        if arch == target_arch:
            toolchain_asset = asset
    if toolchain_asset:
        print("found assets:")
        print("    " + toolchain_asset["name"])
        download_dir.mkdir(parents=True, exist_ok=True)
        for asset in [toolchain_asset]:
            url = asset['browser_download_url']
            print("downloading from", url)
            local_file = download_dir / sanitize_filename(asset["name"])
            urllib.request.urlretrieve(url, local_file, reporthook=ProgressHook())
            print("extracting", asset["name"])
            os.system(f'rm -rf {maadeps_dir}/x-tools')
            shutil.unpack_archive(local_file, maadeps_dir)
            os.system(f'chmod -R +w {maadeps_dir}/x-tools')
    else:
        raise Exception(f"no binary release found for {target_arch}")

if __name__ == "__main__":
    main()
