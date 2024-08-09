import os
import json

cur_dir = os.path.dirname(os.path.abspath(__file__))
proj_dir = os.path.join(cur_dir, "../../")

battle_data_path = os.path.join(proj_dir, "resource/battle_data.json")
theme_paths = {
    "Phantom": os.path.join(proj_dir, "resource/roguelike/Phantom/recruitment.json"),
    "Mizuki": os.path.join(proj_dir, "resource/roguelike/Mizuki/recruitment.json"),
    "Sami": os.path.join(proj_dir, "resource/roguelike/Sami/recruitment.json"),
    "Sarkaz": os.path.join(proj_dir, "resource/roguelike/Sarkaz/recruitment.json")
}

def read_battle_data_names():
    with open(battle_data_path, 'r', encoding='utf-8') as f:
        data = json.load(f)
        names = []
        for key, value in data.get("chars", {}).items():
            if key.startswith("char_"):
                if value.get("rarity") and value.get("rarity") >=3:
                    name = value.get("name")
                    if name:
                        names.append(name)
        return names

def check_recruitment_files(names):
    missing_oper = {}
    for theme, path in theme_paths.items():
        missing_oper[theme] = []
        with open(path, 'r', encoding='utf-8') as f:
            content = f.read()
            for name in names:
                if content.find(f'"{name}"') == -1:
                    missing_oper[theme].append(name)

    # 输出到missing_oper.txt
    with open(os.path.join(cur_dir, 'missing_oper.txt'), 'w', encoding='utf-8') as f:
        for theme, missing_names in missing_oper.items():
            if missing_names:
                f.write(f"{theme}: {' , '.join(missing_names)}\n")

if __name__ == "__main__":
    names = read_battle_data_names()
    check_recruitment_files(names)
