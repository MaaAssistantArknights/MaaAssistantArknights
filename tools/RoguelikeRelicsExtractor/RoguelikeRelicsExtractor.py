import json


def main():
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
        item_exports.append({"name": item_name, "effect": item_usage, "No": item_id})
    tickets_seen = set()
    for item in item_details.values():
        item_id = item["id"]
        item_name = item["name"]
        if item_name in tickets_seen:
            continue
        if item_id.startswith("rogue_5_recruit_ticket") or item_id.startswith(
            "rogue_5_upgrade_ticket"
        ):
            item_usage = item["usage"]
            item_exports.append({"name": item_name, "effect": item_usage})
            tickets_seen.add(item_name)
    final_data = {
        "theme": "JieGarden",
        "priority": item_exports,
        "other": [{"doc": "不会主动购买的藏品"}],
    }
    with open("shopping.json", "w", encoding="utf-8") as f:
        json.dump(final_data, f, ensure_ascii=False, indent=4)


if __name__ == "__main__":
    main()
