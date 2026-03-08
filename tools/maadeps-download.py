#!/usr/bin/env python3
import argparse
import sys
from pathlib import Path

sub_path = Path(__file__).parent.parent / "src" / "MaaUtils" / "tools"
sys.path.append(str(sub_path))

from maadeps_download import detect_host_triplet
from maadeps_download import main as download_main

REPO = "MaaAssistantArknights/MaaDeps"
VERSION = "v2.10.1-maa.1"

parser = argparse.ArgumentParser()
parser.add_argument("triplet", nargs="?")
parser.add_argument("--cache-asset", action="store_true")

if __name__ == "__main__":
    args = parser.parse_args()

    if args.triplet:
        target_triplet = args.triplet
    else:
        target_triplet = detect_host_triplet()

    download_main(target_triplet, REPO, VERSION, args.cache_asset)
