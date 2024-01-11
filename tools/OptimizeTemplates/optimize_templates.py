from argparse import ArgumentParser
import json
import os
import struct
import hashlib
import pathlib
import re

def remove_auxiliary_data(input_file, output_file):
    with open(input_file, 'rb') as f:
        data = f.read()

    output_data = bytearray()
    output_data.extend(data[:8])  # PNG signature

    # Locate and copy critical chunks
    start = 8
    while start < len(data):
        length, chunk_type = struct.unpack('>I4s', data[start:start+8])
        start += 8
        chunk_data = data[start:start+length]
        start += length
        crc = data[start:start+4]
        start += 4

        if chunk_type in [b'IHDR', b'PLTE', b'IDAT', b'IEND']:
            output_data.extend(struct.pack('>I4s', length, chunk_type))
            output_data.extend(chunk_data)
            output_data.extend(crc)

    with open(output_file, 'wb') as f:
        f.write(output_data)

def update_png(file_path: str, perfect_pngs: dict):
    # convert to absolute path
    file_path = str(pathlib.Path(file_path).resolve())

    if not file_path.endswith(".png"):
        return False

    # resource\\global\\{SERVERNAME}\\resource\\template\\xxx.png-> {SERVERNAME}/xxx
    file_id = file_path.replace("\\", "/")
    m = re.search(r"global/(\w+)/resource/template/([^/]+)\.png", file_id)
    if m:
        file_id = f"{m.group(1)}/{m.group(2)}"
    else:
        # resource\\template\\xxx.png-> official/xxx
        m = re.search(r"template/([^/]+)\.png", file_id)
        if m:
            file_id = f"official/{m.group(1)}"
        else:
            # resource\\PATH\\TO\\xxx.png-> other/PATH/TO/xxx
            m = re.search(r"resource/(.*)\.png", file_id)
            if m:
                file_id = f"other/{m.group(1)}"
            else:
                print("unknown file path", file_path)
                return False

    if file_id in perfect_pngs:
        with open(file_path, "rb") as f:
            data = f.read()
            sha256 = hashlib.sha256(data).hexdigest()
            if sha256 == perfect_pngs[file_id]:
                print("skip", file_path)
                return False
            else:
                print("update", file_path)
    
    input_file = output_file = file_path
    remove_auxiliary_data(input_file, output_file)
    os.system(f"optipng -o7 -zm1-9 -fix {output_file}")

    # update file sha256
    with open(file_path, "rb") as f:
        data = f.read()
        sha256 = hashlib.sha256(data).hexdigest()
        perfect_pngs.update({file_id: sha256})

    print("update", file_path)
    update_perfect_png_dict(perfect_pngs)
    return True

cur_dir = pathlib.Path(__file__).parent.resolve()
perfect_pngs_path = str(cur_dir / "optimize_templates.json")
def update_perfect_png_dict(perfect_pngs: dict):
    with open(perfect_pngs_path, "w") as f:
        json.dump(perfect_pngs, f, indent=4)

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument("-p", "--path", dest="path", nargs="*", help="the paths of png files or folders", default=[])
    return parser

if __name__ == '__main__':
    args = ArgParser().parse_args()
    paths = args.path

    perfect_pngs = {}
    optimized_png_paths = []

    if os.path.exists(perfect_pngs_path):
        with open(perfect_pngs_path, "r") as f:
            perfect_pngs = json.load(f)

    if len(paths) == 0:
        resource_dir = cur_dir.parent.parent / "resource"
        paths = [str(resource_dir / "global"), str(resource_dir / "template")]
        print("no path specified, use default paths:", paths)
    for path in paths:
        if pathlib.Path(path).is_dir():
            for root, dirs, files in os.walk(path):
                for f in files:
                    file = os.path.join(root, f)
                    if update_png(file, perfect_pngs):
                        optimized_png_paths.append(file)
        elif pathlib.Path(path).is_file():
            if update_png(path, perfect_pngs):
                optimized_png_paths.append(path)
    
    if len(optimized_png_paths) > 0:
        print("optimized pngs:")
        for path in optimized_png_paths: print(path)
    print("optimized pngs count:", len(optimized_png_paths))
