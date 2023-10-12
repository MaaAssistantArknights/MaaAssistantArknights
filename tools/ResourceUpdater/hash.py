from pathlib import Path
import hashlib

resource = str(Path(__file__).parents[2])
output = resource + "/hash.txt"

files = [
    "resource/Arknights-Tile-Pos/overview.json",
    "resource/stages.json",
    "resource/recruitment.json",
    "resource/item_index.json",
    "resource/battle_data.json",
    "resource/infrast.json",
    "resource/version.json",
    "resource/global/YoStarJP/resource/recruitment.json",
    "resource/global/YoStarJP/resource/item_index.json",
    "resource/global/YoStarJP/resource/version.json",
    "resource/global/YoStarEN/resource/recruitment.json",
    "resource/global/YoStarEN/resource/item_index.json",
    "resource/global/YoStarEN/resource/version.json",
    "resource/global/YoStarKR/resource/recruitment.json",
    "resource/global/YoStarKR/resource/item_index.json",
    "resource/global/YoStarKR/resource/version.json",
    "resource/global/txwy/resource/recruitment.json",
    "resource/global/txwy/resource/item_index.json",
    "resource/global/txwy/resource/version.json"
]
results = []
for f in files:
    full_path = path / f
    sha512_hash = hashlib.sha512()

    with open(full_path, "rb") as file:
        while True:
            data = file.read(65536)
            if not data:
                break
            sha512_hash.update(data)
        
    filename_with_hash = f"{prefix}{f}\t{sha512_hash.hexdigest()}"
    results.append(filename_with_hash)
    return files

context = '\n'.join(results)
print(context)

with open(output, 'w') as f:
    f.write(results)
