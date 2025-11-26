import json

with open("roguelike_topic_table.json", "r", encoding="utf-8") as f:
    data = json.load(f)

items = data["details"]["rogue_5"]["archiveComp"]["relic"]["relic"]
item_details = data["details"]["rogue_5"]["items"]
item_exports = []
for item in items.values():
    item_unique_id = item["relicId"]
    item_id = int(item["orderId"])
    item_detail = item_details[item_unique_id]
    item_name = item_detail["name"]
    item_usage = item_detail["usage"]
    item_exports.append({
        "name": item_name,
        "effect": item_usage,
        "No": item_id
    })
final_data = {
    "theme": "JieGarden",
    "priority": item_exports,
    "other": [
        {
            "doc": "不会主动购买的藏品"
        }
    ]
}
with open("shopping.json", "w", encoding="utf-8") as f:
    json.dump(final_data, f, ensure_ascii=False, indent=4)