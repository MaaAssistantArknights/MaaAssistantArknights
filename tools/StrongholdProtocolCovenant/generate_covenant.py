"""生成卫戍协议商店盟约与干员数据（resource/stronghold_protocol/regulation/stronghold_protocol_data.json）。

数据源为解包游戏数据表 activity_table.json（赛季商店与盟约定义）与 character_table.json
（干员名），由 .github/workflows/res-update-game.yml 在游戏数据更新后自动调用重生成，
资源文件随赛季数据自动更新。

用法：
    python tools/StrongholdProtocolCovenant/generate_covenant.py \
        --activity <activity_table.json 路径> \
        --character <character_table.json 路径> \
        --output <stronghold_protocol_data.json 输出路径>
"""

import argparse
import json
import sys
from pathlib import Path


def load_json(path):
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def pick_season(activity_table):
    """取 activity.AUTOCHESS 下开始时间最新的赛季（当前赛季）

    返回 (act_id, version, season_data)；无赛季数据时返回 None
    """
    acts = activity_table.get("activity", {}).get("AUTOCHESS", {})
    basic = activity_table.get("basicInfo", {})
    best_id, best_time = None, -1
    for act_id in acts:
        start = basic.get(act_id, {}).get("startTime", 0)
        if start > best_time:
            best_id, best_time = act_id, start
    if best_id is None:
        return None
    season_data = acts[best_id]
    version = next(iter(season_data))
    return best_id, version, season_data[version]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--activity", required=True, help="activity_table.json 路径")
    parser.add_argument("--character", required=True, help="character_table.json 路径")
    parser.add_argument(
        "--output", required=True, help="stronghold_protocol_data.json 输出路径"
    )
    args = parser.parse_args()

    activity_table = load_json(args.activity)
    character_table = load_json(args.character)

    picked = pick_season(activity_table)
    if picked is None:
        # 活动表中无赛季数据（赛季结束或未开放）时保留现有资源不动
        print("activity_table 中没有赛季数据，跳过生成")
        return
    act_id, version, season = picked

    # 盟约 id→名称：全局表打底，当季表覆盖
    bond_infos = dict(activity_table["autoChessData"].get("bondInfoDict", {}))
    bond_infos.update(season.get("bondInfoDict", {}))
    bond_names = {
        bond_id: bond.get("name") or bond_id for bond_id, bond in bond_infos.items()
    }

    # 同名干员保留最高档位对应的 ID，盟约取并集。跳过自选位及隐藏条目。
    operators = {}
    for chess_id, entry in season["charShopChessDatas"].items():
        if entry.get("isHidden") or not entry.get("charId"):
            continue
        char = character_table.get(entry["charId"])
        name = char.get("name") if char else None
        if not name:
            print(f"!! 干员名缺失: {entry['charId']} ({chess_id})")
            continue
        chess = season["charChessDataDict"].get(chess_id, {})
        level = entry.get("chessLevel") or 0
        oper = operators.setdefault(
            name, {"id": entry["charId"], "level": level, "bonds": []}
        )
        if level > oper["level"]:
            oper.update(id=entry["charId"], level=level)
        for bond_id in chess.get("bondIds") or []:
            bond = bond_names.get(bond_id, bond_id)
            if bond not in oper["bonds"]:
                oper["bonds"].append(bond)

    if not operators:
        print("赛季商店干员池为空，跳过生成")
        return

    # 盟约池只保留实际出现在干员身上的盟约，按盟约定义顺序排列（identifier 即场上展示序，阵营盟约在前）
    used_bonds = {bond for oper in operators.values() for bond in oper["bonds"]}
    bonds = [
        bond["name"]
        for bond in sorted(
            bond_infos.values(),
            key=lambda b: (b.get("identifier") is None, b.get("identifier") or 0),
        )
        if bond.get("name") and bond["name"] in used_bonds
    ]

    result = {
        "doc": (
            "卫戍协议极速模拟商店盟约与干员数据，由游戏数据表自动生成（tools/StrongholdProtocolCovenant）。"
            "id 对应 battle_data.json 中的干员 ID，level 为卡牌费用等阶（1~6），"
            "bonds 为干员携带的盟约，缺省时按卡面识别。"
        ),
        "season": f"{act_id} {version}",
        "bonds": bonds,
        "operators": dict(sorted(operators.items())),
    }

    output = Path(args.output)
    output.write_text(
        json.dumps(result, ensure_ascii=False, indent=4) + "\n",
        encoding="utf-8",
        newline="\n",
    )
    print(
        f"赛季 {result['season']}：干员 {len(operators)}，盟约 {len(bonds)} -> {output}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
