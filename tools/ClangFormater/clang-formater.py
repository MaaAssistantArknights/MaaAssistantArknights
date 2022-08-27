from argparse import ArgumentParser
import os

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument("--input", help="input dir", metavar="I", dest="src", required=True)
    return parser

args = ArgParser().parse_args()

input_dir = args.src

for root, dirs, files in os.walk(input_dir):
    for f in files:
        file = os.path.join(root, f)
        if os.path.splitext(file)[-1] in [".c", ".h", ".cpp", ".hpp"]:
            print(file)
            os.system(f"clang-format -i -style=file \"{file}\"");

# example: python tools\ClangFormater\clang-formater.py --input=src\MeoAssistant
