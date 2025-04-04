#!/usr/bin/env python3
import argparse
import os
import sys
import math
import json
import time
import platform
from pathlib import Path
import shutil

import requests

BASE_TIMEOUT = 60
TARGET_TAG = "2024-08-17"
# FIXME: update DirectML to 1.15.2 for Windows
if platform.system() == "Windows":
    TARGET_TAG = "2024-10-16"

# The root directory of the project such as "E:\ProgramProjects\MaaAssistantArknights"
basedir = Path(__file__).parent


class Colors:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN = "\033[96m"
    PINK = "\033[38;5;206m"
    PURPLE = "\033[38;5;99m"


def detect_host_triplet():
    """detect and get host_triplet

    Raises:
        Exception: unsupported architecture
        Exception: unsupported system

    Returns:
        str: host_triplet such as "x64-windows"
    """
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
    elif "mingw" in system or "cygwin" in system:
        system = "windows"
    elif system == "darwin":
        system = "osx"
    else:
        raise Exception("unsupported system: " + system)
    return f"{machine}-{system}"


def sanitize_filename(filename: str):
    system = platform.system()
    if system == "Windows":
        filename = filename.translate(str.maketrans('/\\:"?*|\0', "________")).rstrip(
            "."
        )
    elif system == "Darwin":
        filename = filename.translate(str.maketrans("/:\0", "___"))
    else:
        filename = filename.translate(str.maketrans("/\0", "__"))
    return filename


def retry_urlopen(url, retries=3, delay=1, **kwargs):
    """retry urlopen with exponential backoff"""
    for attempt in range(retries):
        try:
            # Adaptive timeout based on attempt number
            timeout = BASE_TIMEOUT * (attempt + 1)
            response = requests.get(url, timeout=timeout, **kwargs)
            response.raise_for_status()
            return response.content
        except requests.exceptions.HTTPError as e:
            if (
                e.response.status_code == 403
                and "x-ratelimit-remaining" in e.response.headers
            ):
                # rate limit
                t0 = time.time()
                reset_time = t0 + 10
                try:
                    reset_time = int(e.response.headers.get("x-ratelimit-reset", 0))
                except ValueError:
                    pass
                reset_time = max(reset_time, t0 + 10)
                print(
                    f"Rate limit exceeded, retrying after {reset_time - t0:.1f} seconds"
                )
                time.sleep(reset_time - t0)
            else:
                print(f"HTTPError: {e} (attempt {attempt + 1}/{retries})!")
                time.sleep(delay)
        except (
            requests.exceptions.RequestException,
            requests.exceptions.ConnectionError,
        ) as e:
            print(f"Network error: {e} (attempt {attempt + 1}/{retries})!")
            time.sleep(delay)
    raise Exception(f"Failed after {retries} attempts with retry!")


def split_asset_name(name: str):
    """get target_triplet in asset name

    Args:
        name (str): asset name such as "MaaDeps-x64-windows-dbg.tar.xz"

    Returns:
        tuple(str, str): get target_triplet in asset name,such as ('x64-windows', 'dbg')
    """
    *remainder, component_suffix = name.rsplit("-", 1)
    component = component_suffix.split(".", 1)[0]
    if remainder:
        _, *target = remainder[0].split("-", 1)
        if target:
            return target[0], component
    return None, None


def convert_size(size_bytes):
    """Convert bytes to KB, MB, GB, etc."""
    if size_bytes == 0:
        return "0B"
    size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB")
    i = int(math.floor(math.log(size_bytes, 1024)))
    p = math.pow(1024, i)
    s = round(size_bytes / p, 2)
    return f"{s}{size_name[i]}"


def download_file(url, local_file):
    try:
        response = requests.get(url, stream=True)
        response.raise_for_status()

        # get file download size
        total_size = int(response.headers.get("content-length", 0))
        downloaded_size = 0

        # open file with write chunk
        with open(local_file, "wb") as f:
            for chunk in response.iter_content(chunk_size=8192):
                if chunk:
                    f.write(chunk)
                    downloaded_size += len(chunk)
                    # show progress
                    progress = (
                        (downloaded_size / total_size) * 100 if total_size > 0 else 0
                    )
                    print(
                        f"{Colors.YELLOW}[Download]: {local_file.name}: {convert_size(downloaded_size)} / {convert_size(total_size)} ({progress:.2f}%){Colors.RESET}",
                        end="\r",
                    )
        print(f"\n{Colors.GREEN}[Download completed]: {local_file}{Colors.RESET}")
    except requests.exceptions.RequestException as e:
        print(f"{Colors.RED}[Failed to download]: {url}: {e}{Colors.RESET}")
        raise


def main():
    maadeps_url = "https://api.github.com/repos/MaaAssistantArknights/MaaDeps/releases"
    parser = argparse.ArgumentParser()
    parser.add_argument("target_triplet", nargs="?", default=None)
    parser.add_argument("tag", nargs="?", default=None)
    parser.add_argument(
        "-f",
        "--force",
        action="store_true",
        help="force download even if already exists",
    )
    args = parser.parse_args()

    if args.target_triplet is not None:
        target_triplet = args.target_triplet
    else:
        target_triplet = detect_host_triplet()

    if args.tag is not None:
        target_tag = args.tag
    else:
        target_tag = TARGET_TAG
    print(Colors.BOLD + Colors.PINK + "MaaDeps Downloader" + Colors.RESET)
    print(
        "  - ",
        "About to download prebuilt dependency libraries for",
        f"{target_triplet} of {target_tag}",
    )
    if args.target_triplet is None:
        print(
            "  - ",
            "to specify another triplet [and tag], ",
            f"run `{sys.argv[0]} <target triplet> [tag]`",
        )
        print(
            "  - ",
            f"e.g. `{sys.argv[0]} arm64-windows` ",
            f"or `{sys.argv[0]} arm64-windows 2023-04-24-3`",
        )

    maadeps_dir = Path(basedir, "MaaDeps")
    maadeps_version_file = Path(maadeps_dir, ".versions.json")
    try:
        with open(maadeps_version_file, "r") as f:
            versions = json.load(f)
    except:
        versions = {}
    if not args.force and versions.get(target_triplet) == target_tag:
        print(
            f"prebuilt dependencies for {target_triplet} of {target_tag} ",
            "already exist, skipping download",
        )
        print(f"to force download, run `{sys.argv[0]} -f`")
        return

    token = os.environ.get("GH_TOKEN", os.environ.get("GITHUB_TOKEN", None))
    headers = {}
    if token:
        headers["Authorization"] = f"Bearer {token}"
    resp = retry_urlopen(maadeps_url, headers=headers)
    releases = json.loads(resp)

    devel_asset = None
    runtime_asset = None
    for release in releases:
        if release["tag_name"] != target_tag:
            continue
        for asset in release["assets"]:
            target, component = split_asset_name(asset["name"])
            if target == target_triplet:
                if component == "devel":
                    devel_asset = asset
                elif component == "runtime":
                    runtime_asset = asset
        if devel_asset and runtime_asset:
            break
    if devel_asset and runtime_asset:
        print(f"\n{Colors.GREEN}[Found assets]:{Colors.RESET}")
        print("  -  " + devel_asset["name"])
        print("  -  " + runtime_asset["name"])
        download_dir = Path(maadeps_dir, "tarball")
        download_dir.mkdir(parents=True, exist_ok=True)
        try:
            shutil.rmtree(
                maadeps_dir / "runtime" / f"maa-{target_triplet}", ignore_errors=True
            )
            shutil.rmtree(
                maadeps_dir / "vcpkg" / "installed" / f"maa-{target_triplet}",
                ignore_errors=True,
            )
            for asset in [devel_asset, runtime_asset]:
                url = asset["browser_download_url"]
                print(f"\n{Colors.BLUE}[Downloading from]:{url}{Colors.RESET}")
                local_file = download_dir / sanitize_filename(asset["name"])

                download_file(url, local_file)
                print(f"{Colors.YELLOW}[Extracting]:{asset['name']}{Colors.RESET}")
                shutil.unpack_archive(local_file, maadeps_dir)
                print(f"{Colors.GREEN}[Unpacked]: {asset['name']}{Colors.RESET}")
            versions[target_triplet] = target_tag

            print(f"\n{Colors.PINK}Prebuilt dependency download complete.{Colors.RESET}")
        except:
            versions[target_triplet] = None
            raise
        finally:
            with open(maadeps_version_file, "w") as f:
                json.dump(versions, f, indent=2)
    else:
        raise Exception(f"{Colors.RED}No binary release found for {target_triplet}{Colors.RESET}")


if __name__ == "__main__":
    # use command such as "python3 maadeps-download.py" to download prebuilt dependency libraries
    # or use "python3 maadeps-download.py -f" to force download prebuilt dependency libraries
    main()
