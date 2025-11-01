#!/usr/bin/env python3
import sys
from pathlib import Path

sub_path = Path(__file__).parent.parent / "src" / "MaaUtils" / "tools"
sys.path.append(str(sub_path))

from maadeps_download import detect_host_triplet
from maadeps_download import main as download_main

REPO = "MaaAssistantArknights/MaaDeps"
VERSION = "v2.10.1-maa.1"

if __name__ == "__main__":
    if len(sys.argv) == 2:
        target_triplet = sys.argv[1]
    else:
        target_triplet = detect_host_triplet()

    download_main(target_triplet, REPO, VERSION)
