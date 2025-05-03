import json
from pathlib import Path
from cutter import task_paths, task_list, src_path, dst_path

"""
Update tasks.json for multi template tasks
"""

task_file = task_paths["CN"]
template_path = task_file.parent / "template"

for json_path in task_file.glob("*.json"):
    with json_path.open("r", encoding="utf-8") as f:
        tasks = json.load(f)

    default_tmpl_names = []
    for i in task_list:
        if i in tasks:
            tmpl = tasks[i].get("template")
            if isinstance(tmpl, list) and tmpl:
                default_tmpl_names.append(tmpl[0])
            elif isinstance(tmpl, str):
                default_tmpl_names.append(tmpl)

    for image in src_path.glob("*.png"):
        image_name = image.stem
        for task in tasks:
            if task not in task_list:
                continue
            task_info = tasks[task]
            if "template" in task_info:
                if isinstance(task_info["template"], str):
                    task_info["template"] = [task_info["template"]]
            if isinstance(task_info.get("template"), list):
                if task_info["template"]:
                    default_tmpl_name = task_info["template"][0]
                    if default_tmpl_name in default_tmpl_names:
                        new_tmpl = f"{default_tmpl_name.split('.')[0]}{image_name}.png"
                        task_info["template"].append(new_tmpl)

    with json_path.open("w", encoding="utf-8") as f:
        json.dump(tasks, f, ensure_ascii=False, indent=4)

for image in dst_path.glob("*.png"):
    new_file = template_path / image.name
    if not new_file.exists():
        image.rename(new_file)
