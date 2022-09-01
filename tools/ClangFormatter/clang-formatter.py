from argparse import ArgumentParser
import os

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument("--input", help="input dir", metavar="I", dest="src")
    parser.add_argument("--clang-format", help="the path of clang-format.exe", metavar="PATH", dest="exe", default="clang-format")
    return parser

args = ArgParser().parse_args()

clang_format_exe = args.exe
input_dir = args.src

os.system(f"{clang_format_exe} --version");

if not input_dir:
    print("No input dir, exit.")
    exit()

for root, dirs, files in os.walk(input_dir):
    for f in files:
        file = os.path.join(root, f)
        if os.path.splitext(file)[-1] in [".c", ".h", ".cpp", ".hpp", ".json"]:
            print(file)
            os.system(f"{clang_format_exe} -i -style=file \"{file}\"");

# example: python tools\ClangFormater\clang-formater.py --input=src\MeoAssistant
