#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

sub_path = Path(__file__).parent.parent / "src" / "MaaUtils" / "tools"
sys.path.append(str(sub_path))

from maafw_download import detect_host_platform
from maafw_download import main as download_main

REPO = "MaaXYZ/MaaFramework"
VERSION = None

parser = argparse.ArgumentParser()
parser.add_argument("platform", nargs="?")
parser.add_argument("--cache-asset", action="store_true")

if __name__ == "__main__":
    args = parser.parse_args()

    if args.platform:
        target_platform = args.platform
    else:
        target_platform = detect_host_platform()

    download_main(target_platform, REPO, VERSION, args.cache_asset)
