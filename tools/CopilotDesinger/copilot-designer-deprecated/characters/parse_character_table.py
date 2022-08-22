# https://github.com/yuanyan3060/Arknights-Bot-Resource/blob/main/gamedata/excel/character_table.json
# 去除character_table中不需要的字段
import json

with open('./character_table.json', 'r', encoding='utf-8') as f:
    character_table_dict = json.load(f)
character_name_list = []
for character_name in character_table_dict.keys():
    character_name_list.append(character_table_dict[character_name]['name'])
# print(new_character_table_dict)
with open('./character_name_list.json', 'w', encoding='utf-8') as f:
    json.dump(character_name_list, f, ensure_ascii=False)
