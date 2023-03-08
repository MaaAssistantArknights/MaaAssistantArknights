import sys
import os
from pathlib import Path
import subprocess

basedir = Path(__file__).parent

def main():
    os.chdir(basedir)
    subprocess.check_call(["git", "submodule", "update", "--init", "--recommend-shallow", "--remote", "MaaDeps"])
    ret = subprocess.call([sys.executable, "build.py", *sys.argv[1:]], cwd=Path(basedir, "MaaDeps"))
    sys.exit(ret)

if __name__ == '__main__':
    main()
