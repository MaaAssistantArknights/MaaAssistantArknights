import argparse
import json
import os
import re
from pathlib import Path
from typing import Any

import json5


def sort_tasks(res: dict[str, Any]):
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
        "...@Reclamation...": [],
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
        (r"", unclassified_list),
    ]

    for k in res.keys():
        for rule, target_list in classify_rules:
            if re.search(rule, k):
                target_list.append(k)
                break

    return {
        k: res[k]
        for k in unclassified_list
        + sum([sorted(target_list) for target_list in classified_lists.values()], [])
    }


def raise_on_duplicate_keys(pairs):
    """检查重复键，存在时抛出 ValueError"""
    seen = {}
    for key, value in pairs:
        if key in seen:
            raise ValueError(f"Duplicate key detected: {repr(key)}")
        seen[key] = value
    return seen


def load_jsonc(file_obj):
    """Parse a task file.

    json5 is a pure-Python parser and is much slower than the
    C-accelerated stdlib json module. Most task files are plain JSON,
    so try the fast path first and only pay json5's cost for the rare
    file that actually needs JSON5 syntax (comments, trailing commas,
    unquoted keys).
    """
    start = file_obj.tell()
    try:
        return json.load(file_obj, object_pairs_hook=raise_on_duplicate_keys)
    except json.JSONDecodeError:
        file_obj.seek(start)
        return json5.load(file_obj, object_pairs_hook=raise_on_duplicate_keys)


def main(cn_base_path, global_resources):
    cn_base_path = Path(cn_base_path)

    # --- Load CN files ---
    cn_raw = {}
    for root, dirs, files in os.walk(cn_base_path):
        for file in files:
            if not file.endswith(".json"):
                continue
            file_path = Path(root) / file
            with open(file_path, "r", encoding="utf-8-sig") as f:
                cn_raw[file_path.relative_to(cn_base_path)] = load_jsonc(f)

    # --- Sort each CN file exactly once (previously done twice) ---
    cn_tasks = {}
    cn_order = {}
    for task_path, task in cn_raw.items():
        sorted_task = sort_tasks(task)
        cn_tasks[task_path] = sorted_task
        cn_order[task_path] = list(sorted_task.keys())

    # --- Single-pass duplicate detection (was O(files^2 * tasks)) ---
    task_owner: dict[str, Path] = {}
    for task_path, tasks in cn_order.items():
        for task in tasks:
            owner = task_owner.get(task)
            if owner is not None:
                raise ValueError(
                    f"Duplicate task found: {owner} and {task_path} have the same task '{task}'"
                )
            task_owner[task] = task_path

    # --- Write CN files ---
    for task_path, task in cn_tasks.items():
        with open(cn_base_path / task_path, "w", encoding="utf8", newline="\n") as f:
            json.dump(task, f, ensure_ascii=False, indent=4)

    print(
        "CN:",
        str(sum(len(tasks) for tasks in cn_order.values())).rjust(4, " "),
        "tasks",
    )

    # --- Overseas files: reorder to match CN using precomputed index maps ---
    # (previously called list(...).index(...) inside the sort key itself,
    # turning each sort into an O(n^2) scan)
    for server, path in global_resources.items():
        overseas_path = Path(path)
        count = 0

        for root, dirs, files in os.walk(overseas_path):
            for file in files:
                if not file.endswith(".json"):
                    continue
                file_path = Path(root) / file
                relative_path = file_path.relative_to(overseas_path)
                with open(file_path, "r", encoding="utf-8-sig") as f:
                    tasks = load_jsonc(f)

                base_order = cn_order.get(relative_path, [])
                base_tasks = cn_tasks.get(relative_path, {})
                base_index = {k: i for i, k in enumerate(base_order)}

                sorted_keys = sorted(tasks.keys(), key=lambda k: base_index.get(k, -1))

                new_tasks = {}
                for k in sorted_keys:
                    field_order = base_tasks.get(k)
                    if field_order:
                        field_index = {x: i for i, x in enumerate(field_order.keys())}
                        sorted_fields = sorted(
                            tasks[k].keys(), key=lambda x: field_index.get(x, -1)
                        )
                    else:
                        # No base reference for this task: keep original order
                        # (stable sort with a constant key would give the same result)
                        sorted_fields = list(tasks[k].keys())
                    new_tasks[k] = {x: tasks[k][x] for x in sorted_fields}

                with open(file_path, "w", encoding="utf8", newline="\n") as f:
                    json.dump(new_tasks, f, ensure_ascii=False, indent=4)
                count += len(new_tasks)
        print(server + ":", str(count).rjust(4, " "), "tasks")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Sort tasks in JSON files.")
    parser.add_argument("--cn", type=str, help="Path to the CN tasks JSON file.")
    parser.add_argument(
        "--overseas",
        type=str,
        help="Comma-separated paths to the global tasks JSON files in the format 'EN:path,JP:path,KR:path,TW:path'.",
    )
    args = parser.parse_args()

    resource_dir = Path(__file__).parents[2] / "resource"
    cn_task_path = args.cn if args.cn else resource_dir / "tasks"

    default_global_resources = {
        "EN": resource_dir / "global/YoStarEN/resource/tasks",
        "JP": resource_dir / "global/YoStarJP/resource/tasks",
        "KR": resource_dir / "global/YoStarKR/resource/tasks",
        "TW": resource_dir / "global/txwy/resource/tasks",
    }

    global_resources = default_global_resources
    if args.overseas:
        global_resources = {
            k: Path(v)
            for k, v in (item.split(":") for item in args.overseas.split(","))
        }

    main(cn_task_path, global_resources)
