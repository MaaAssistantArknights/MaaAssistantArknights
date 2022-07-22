from argparse import ArgumentParser
import json
import requests
import os

def ArgParser():
    parser = ArgumentParser()
    parser.add_argument("--input", help="input dir", metavar="I", dest="src", required=True)
    parser.add_argument("--output", help="output dir", metavar="O", dest="det", required=True)
    return parser

args = ArgParser().parse_args()

input_dir = args.src
output_dir = args.det
input_file = os.path.join(input_dir, "gamedata/excel/character_table.json")
output_file = os.path.join(output_dir, "roguelike_recruit.json")

with open(input_file, "r", encoding="utf8") as f:
    raw_employee_infos = json.load(f)
with open(output_file, "r", encoding="utf8") as f:
    old_res = json.load(f)

old_dic = {
    "Warrior": {},
    "Pioneer": {},
    "Caster": {},
    "Sniper": {},
    "Medic": {},
    "Special": {},
    "Tank": {},
    "Support": {}
}

name_replace_map = {}

for profession in ["Warrior", "Pioneer", "Caster", "Sniper", "Medic", "Special", "Tank", "Support"]:
    for item in old_res[profession]:
        if "_N" in item["name"]:
            name_replace_map[item["name"]] = item["name"].replace("_N", "")
            old_dic[profession][name_replace_map[item["name"]]] = item
        else:
            old_dic[profession][item["name"]] = item

res = old_res

default_position = {
    "Warrior": "MELEE",
    "Pioneer": "MELEE",
    "Caster": "RANGED",
    "Sniper": "RANGED",
    "Medic": "RANGED",
    "Special": "MELEE",
    "Tank": "MELEE",
    "Support": "RANGED"
}

for x, y in raw_employee_infos.items():
    if y["profession"] not in [
        "WARRIOR","PIONEER","CASTER","SNIPER",
        "MEDIC", "SPECIAL", "TANK", "SUPPORT"
    ]: continue
    if "预备干员" in y["name"]:
        continue
    profession = y["profession"].title()
    if profession not in res:
        res[profession] = []

    employee_info = {
        "name": y["name"],
        "level": y["rarity"] + 1,
        # "position": y["position"]
    }
    # 和默认位置不同，直接禁用
    if default_position[profession] != y["position"]:
        employee_info["name"] += "_N"

    # 四星以上的干员设二技能
    if y["rarity"] + 1 >= 4:
        employee_info["skill"] = 2
    # 三星的干员设一技能
    elif y["rarity"] + 1 == 3:
        employee_info["skill"] = 1
    # 二星以下的干员不设技能
    else:
        employee_info["skill"] = 0

    if y["name"] in old_dic[profession]:
        old_employee_info = old_dic[profession][y["name"]]
        if "skill" in old_employee_info:
            employee_info["skill"] = old_employee_info["skill"]
        if "alternate_skill" in old_employee_info:
            employee_info["alternate_skill"] = old_employee_info["alternate_skill"]
        if "skill_usage" in old_employee_info:
            employee_info["skill_usage"] = old_employee_info["skill_usage"]
        for employee in res[profession]:
            if employee["name"] == y["name"]:
                employee.update(employee_info)
            elif employee["name"] in name_replace_map and name_replace_map[employee["name"]] == y["name"]:
                employee_info["name"] = employee["name"]
                employee.update(employee_info)
            else: continue
            break
    else:
        res[profession].append(employee_info)


## FUCKING CRLF
## 处理 CRLF 和 LF 大概有更好的办法，但是我不知道.jpg

# with open(output_file, "w", encoding="utf8") as f:
#     # json.dump(dic, f, ensure_ascii=False, indent=4)
#     json.dump(res, f, ensure_ascii=False, indent=4)

dump_cache = json.dumps(res, ensure_ascii=False, indent=4).encode("utf8").replace(b"\r\n", b"\n")
with open(output_file, "wb") as f:
    f.write(dump_cache)
