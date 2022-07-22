import json
import requests

url = "https://raw.githubusercontent.com/Kengxxiao/ArknightsGameData/master/zh_CN/gamedata/excel/character_table.json"
res = requests.get(url, proxies = {"https": "http://127.0.0.1:1080"})
raw_employee_infos = json.loads(res.text)
# print(raw_employee_infos)

# ArknightsGameData\zh_CN\gamedata\excel
# with open("~$character_table.json", "r", encoding="utf8") as f:
#     raw_employee_infos = json.load(f)

with open("resource/roguelike_recruit.json", "r", encoding="utf8") as f:
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
            print(item["name"])
            name_replace_map[item["name"]] = item["name"].replace("_N", "")
            old_dic[profession][name_replace_map[item["name"]]] = item
        else:
            old_dic[profession][item["name"]] = item

# res = {
#     "roles": [
#         "Warrior",
#         "Pioneer",
#         "Tank",
#         "Sniper",
#         "Caster",
#         "Special",
#         "Medic",
#         "Support",
#         "Alternate"
#     ],
#     "Warrior": [],
#     "Pioneer": [],
#     "Caster": [],
#     "Sniper": [],
#     "Medic": [],
#     "Special": [],
#     "Tank": [],
#     "Support": [],
#     "Alternate": [
#         {
#             "name": "预备干员-近战",
#             "level": 3,
#             "skill": 0
#         },
#         {
#             "name": "预备干员-狙击",
#             "level": 3,
#             "skill": 0
#         },
#         {
#             "name": "预备干员-术师",
#             "level": 3,
#             "skill": 0
#         },
#         {
#             "name": "预备干员-后勤",
#             "level": 3,
#             "skill": 0
#         }
#     ]
# }

res = old_res

# dic = {}

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
        # res[profession] = employee_info
    else:
        res[profession].append(employee_info)
    # dic[y["name"]] = {
    #     "level": y["rarity"] + 1,
    #     "profession": y["profession"],
    #     "subProfession": y["subProfessionId"],
    #     "position": y["position"]
    # }


# dic["预备干员"] = {
#     "Doc": "事实上没有叫这个名字的干员，但为了避免肉鸽中，requires设置了但找不到的问题，增设这个干员。",
#     "level": 3
# }


## FUCKING CRLF

# with open("resource/roguelike_recruit.json", "w", encoding="utf8") as f:
#     # json.dump(dic, f, ensure_ascii=False, indent=4)
#     json.dump(res, f, ensure_ascii=False, indent=4)

dump_cache = json.dumps(res, ensure_ascii=False, indent=4).encode("utf8").replace(b"\r\n", b"\n")
with open("resource/roguelike_recruit.json", "wb") as f:
    f.write(dump_cache)

