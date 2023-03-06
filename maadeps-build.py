import sys
import os
from pathlib import Path
import subprocess

basedir = Path(__file__).parent

def main():
    os.chdir(basedir)
    subprocess.check_call(["git", "submodule", "update", "--init", "--recommend-shallow", "--remote", "MaaDeps"])
    subprocess.check_call(["git", "submodule", "update", "--init", "--recommend-shallow", "MaaDeps/vcpkg"])
    ret = subprocess.call([sys.executable, "build.py", *sys.argv[1:]], cwd=Path(basedir, "MaaDeps"))
    sys.exit(ret)
