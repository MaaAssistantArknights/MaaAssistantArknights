import json


def find_skills_without_efficient(json_data):
    """
    分析JSON数据，找出所有设施中没有efficient字段的技能
    """
    results = {}

    # 遍历所有设施
    for facility, facility_data in json_data.items():
        # 检查是否有skills字段
        if "skills" in facility_data:
            skills_without_efficient = []

            # 遍历该设施的所有技能
            for skill_id, skill_data in facility_data["skills"].items():
                # 检查技能是否有efficient字段
                if "efficient" not in skill_data:
                    skill_info = {
                        "id": skill_id,
                        "name": skill_data.get("name", ["未知名称"])[0],  # 取第一个名称
                    }
                    skills_without_efficient.append(skill_info)

            # 如果该设施有缺少efficient的技能，添加到结果中
            if skills_without_efficient:
                results[facility] = skills_without_efficient

    return results


def main():
    try:
        with open("../../resource/infrast.json", "r", encoding="utf-8") as file:
            data = json.load(file)
    except FileNotFoundError:
        print("未找到 infrast.json 文件")
        return
    except json.JSONDecodeError:
        print("错误：JSON 文件格式不正确")
        return

    # 分析数据
    missing_efficient_skills = find_skills_without_efficient(data)

    # 输出结果
    if not missing_efficient_skills:
        print("所有技能都有 efficient 字段")
    else:
        print("缺少 efficient 字段的技能：")
        print("=" * 50)

        for facility, skills in missing_efficient_skills.items():
            print(f"\n设施: {facility}")
            print("-" * 30)

            for skill in skills:
                print(f"技能 ID: {skill['id']}")
                print(f"技能名称: {skill['name']}")
                print()


main()
