"""Generate Tales workbench names from the regional sandbox_perm_table.json files."""

import argparse
import json
from pathlib import Path


def read_tools(path):
    table = json.loads(path.read_text(encoding="utf-8"))
    recipes = table["detail"]["SANDBOX_V2"]["sandbox_1"]["craftItemData"]
    # The Official extractor emits the numeric enum. Overseas extractors emit its name.
    tools = {
        item_id: table["itemData"][item_id]["itemName"]
        for item_id, recipe in recipes.items()
        if recipe["type"] in (1, "TACTICAL") and not recipe["isHidden"]
    }
    if not tools or not all(isinstance(name, str) and name for name in tools.values()):
        raise ValueError(f"Missing tactical tool names in {path}")
    return tools


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", type=Path)
    parser.add_argument("tables", nargs=5, type=Path, help="Official, EN, JP, KR and TW tables")
    args = parser.parse_args()
    catalog = {}
    for table in args.tables:
        for item_id, name in read_tools(table).items():
            names = catalog.setdefault(item_id, [])
            if name not in names:
                names.append(name)
    if args.output.exists() and json.loads(args.output.read_text(encoding="utf-8")) == catalog:
        return
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(catalog, ensure_ascii=False, sort_keys=True, indent=4) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
