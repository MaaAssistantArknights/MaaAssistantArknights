
import json
import re
import argparse
import os
from pathlib import Path

def sort_tasks(res: dict[str, any]):
    classified_lists = {
        "UseSupportUnit...": [],
        "...@UseSupportUnit...": [],
        "Roguelike...": [],
        "Roguelike@...": [],
        "Phantom@Roguelike...": [],
        "Mizuki@Roguelike...": [],
        "Sami@Roguelike...": [],
        "Sarkaz@Roguelike...": [],
        "...@Roguelike...": [],
        "Reclamation...": [],
        "Reclamation@...": [],
        "Fire@Reclamation...": [],
        "Tales@RA...": [],
        "...@Reclamation...": []
    }
    unclassified_list = []

    classify_rules: list[tuple[str, list]] = [
        (r"^UseSupportUnit", classified_lists["UseSupportUnit..."]),
        (r"^(\w+)@UseSupportUnit", classified_lists["...@UseSupportUnit..."]),
        (r"^Roguelike@", classified_lists["Roguelike@..."]),
        (r"^Roguelike", classified_lists["Roguelike..."]),
        (r"^Phantom@Roguelike", classified_lists["Phantom@Roguelike..."]),
        (r"^Mizuki@Roguelike", classified_lists["Mizuki@Roguelike..."]),
        (r"^Sami@Roguelike", classified_lists["Sami@Roguelike..."]),
        (r"^Sarkaz@Roguelike", classified_lists["Sarkaz@Roguelike..."]),
        (r"^(\w+)@Roguelike", classified_lists["...@Roguelike..."]),
        (r"^Reclamation@", classified_lists["Reclamation@..."]),
        (r"^Reclamation", classified_lists["Reclamation..."]),
        (r"^Fire@Reclamation", classified_lists["Fire@Reclamation..."]),
        (r"^Tales@RA", classified_lists["Tales@RA..."]),
        (r"^(\w+)@Reclamation", classified_lists["...@Reclamation..."]),
        (r"", unclassified_list)
    ]

    for k in res.keys():
        for rule, target_list in classify_rules:
            if re.search(rule, k):
                target_list.append(k)
                break

    return {
        k: res[k]
        for k in unclassified_list + sum([
            sorted(target_list)
            for target_list in classified_lists.values()], [])}

def raise_on_duplicate_keys(pairs):
    """检查重复键，存在时抛出 ValueError"""
    seen = {}
    for key, value in pairs:
        if key in seen:
            raise ValueError(f"Duplicate key detected: {repr(key)}")
        seen[key] = value
    return seen

# 示例JSON数据（包含重复键）
json_str = '{"name": "Alice", "age": 30, "name": "Bob"}'


def main(cn_base_path, global_resources):
    
    cn_tasks = {}
    cn_order = {}
    # 使用 Path 处理路径，确保跨平台兼容
    cn_base_path = Path(cn_base_path)
    for root, dirs, files in os.walk(cn_base_path):
        for file in files:
            if not file.endswith(".json"):  # 判断是否为 JSON 文件
                continue
            file_path = Path(root) / file
            with open(file_path, "r", encoding="utf8") as f:
                cn_tasks[file_path.relative_to(cn_base_path)] = json.load(f, object_pairs_hook=raise_on_duplicate_keys)

    for task_path, task in cn_tasks.items():
        cn_tasks[task_path] = sort_tasks(task)
        cn_order[task_path] = list(task.keys())

    for task_path, tasks in cn_order.items():
        for task_path1, tasks1 in cn_order.items():
            if task_path == task_path1:
                continue
            for task in tasks:
                if task in tasks1:
                    raise ValueError(f"Duplicate task found: {task_path} and {task_path1} have the same task '{task}'")

    for task_path, task in cn_tasks.items():
        cn_tasks[task_path] = sort_tasks(task)
        cn_order[task_path] = list(task.keys())
        with open(cn_base_path / task_path, "w", encoding="utf8", newline="\n") as f:
            json.dump(task, f, ensure_ascii=False, indent=4)

    print("CN:", str(sum(len(tasks) for tasks in cn_order.values())).rjust(4, " "), "tasks")

    for server, path in global_resources.items():
        overseas_path = Path(path)
        count = 0

        for root, dirs, files in os.walk(overseas_path):
            for file in files:
                if not file.endswith(".json"):  # 判断是否为 JSON 文件
                    continue
                file_path = Path(root) / file
                relative_path = file_path.relative_to(overseas_path)
                with open(file_path, "r", encoding="utf8") as f:
                    tasks = json.load(f, object_pairs_hook=raise_on_duplicate_keys)

                base_order = cn_order.get(relative_path, [])
                base_tasks = cn_tasks.get(relative_path, {})

                tasks = {
                    k: {
                        x: tasks[k][x]
                        for x in sorted(tasks[k].keys(),
                                        key=lambda x: list(base_tasks[k].keys()).index(x) if k in base_tasks and x in base_tasks[k] else -1)
                    }
                    for k in sorted(tasks.keys(), key=lambda k: base_order.index(k) if k in base_order else -1)
                }

                with open(file_path, "w", encoding="utf8", newline="\n") as f:
                    json.dump(tasks, f, ensure_ascii=False, indent=4)
                count += len(tasks)
        print(server + ":", str(count).rjust(4, " "), "tasks")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Sort tasks in JSON files.")
    parser.add_argument("--cn", type=str, help="Path to the CN tasks JSON file.")
    parser.add_argument("--overseas", type=str, help="Comma-separated paths to the global tasks JSON files in the format 'EN:path,JP:path,KR:path,TW:path'.")
    args = parser.parse_args()

    resource_dir = Path(__file__).parents[2] / "resource"
    cn_task_path = args.cn if args.cn else resource_dir / "tasks"

    default_global_resources = {
        "EN": resource_dir / "global/YoStarEN/resource/tasks",
        "JP": resource_dir / "global/YoStarJP/resource/tasks",
        "KR": resource_dir / "global/YoStarKR/resource/tasks",
        "TW": resource_dir / "global/txwy/resource/tasks"
    }

    global_resources = default_global_resources
    if args.overseas:
        global_resources = {k: Path(v) for k, v in (item.split(':') for item in args.overseas.split(','))}

    main(cn_task_path, global_resources)
