import sys
import os
from pathlib import Path
import hashlib

if len(sys.argv) < 3:
    print("Usage: python3 list.py <resource> <output>")
    exit(1)

resource = str(Path(__file__).parents[2]) + "/resource"
output = resource + "/hash.txt"

def listfiles(path, prefix):
    files = []
    for f in os.listdir(path):
        if not f.endswith('.png') and not f.endswith('.json'):
            continue
        full_path = path / f
        sha512_hash = hashlib.sha512()

        with open(full_path, "rb") as file:
            # Read the file in chunks to handle large files
            while True:
                data = file.read(65536)  # 64KB chunks
                if not data:
                    break
                sha512_hash.update(data)
        
        # Append the SHA-512 hash to the filename
        filename_with_hash = f"{prefix}{f}\t{sha512_hash.hexdigest()}"
        files.append(filename_with_hash)
    return files

files = listfiles(resource / "template" / "infrast", "resource/template/infrast/")
files += listfiles(resource / "template" / "items", "resource/template/items/")
files += listfiles(resource / "Arknights-Tile-Pos", "resource/Arknights-Tile-Pos/")

context = '\n'.join(files)
print(context)

with open(output, 'w') as f:
    f.write(context)
