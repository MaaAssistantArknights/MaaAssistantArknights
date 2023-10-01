from argparse import ArgumentParser
import json
import os

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument("--input", help="input dir", metavar="I", dest="src")
    parser.add_argument("--clang-format", help="the path of clang-format.exe", metavar="PATH", dest="exe", default="clang-format")
    parser.add_argument("--style", help="clang-format style", dest="style", default="file")
    parser.add_argument("--rule", help="json-style '.xxx' array", dest="rule", default="[\".c\", \".h\", \".cpp\", \".hpp\", \".json\"]")
    return parser

if __name__ == "__main__":
    args = ArgParser().parse_args()
    clang_format_exe = args.exe
    input_path = args.src
    rule_array = json.loads(args.rule)

    os.system(f"{clang_format_exe} --version");

    if not input_path:
        print("No input dir.")
    elif os.path.isdir(input_path):
        if type(rule_array) != list:
            print("Invalid rule!")
        else:
            for root, dirs, files in os.walk(input_path):
                for f in files:
                    file = os.path.join(root, f)
                    if os.path.splitext(file)[-1] in rule_array:
                        print(file)
                        os.system(f"{clang_format_exe} -i -style={args.style} \"{file}\"")
    elif os.path.isfile(input_path):
        file = input_path
        print(file)
        os.system(f"{clang_format_exe} -i -style={args.style} \"{file}\"")
    else:
        print("Invalid input_path!")

# example: python tools\ClangFormatter\clang-formatter.py --input=src\MaaCore
