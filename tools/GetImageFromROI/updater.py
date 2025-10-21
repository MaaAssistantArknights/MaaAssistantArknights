import json
from pathlib import Path

from cutter import dst_path, src_path, task_list, task_paths

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
    default_tmpl_names = [i.split("/")[-1] for i in default_tmpl_names]
    print(default_tmpl_names)

    for image in src_path.glob("*.png"):
        image_name = image.stem
        to_be_edited = [
            i
            for i in tasks
            if isinstance(tasks[i].get("template"), list)
            and "Default" in tasks[i].get("template")[0]
        ]
        print(to_be_edited)
        for task in to_be_edited:
            task_info = tasks[task]
            if "template" in task_info:
                if isinstance(task_info["template"], str):
                    task_info["template"] = [task_info["template"]]
            if isinstance(task_info.get("template"), list):
                if task_info["template"]:
                    default_tmpl_name = task_info["template"][0].split("/")[-1]
                    # print(default_tmpl_name, image_name)
                    if default_tmpl_name in default_tmpl_names:
                        new_tmpl = f"{image_name}/{default_tmpl_name}"
                        task_info["template"].append(new_tmpl)

    with json_path.open("w", encoding="utf-8") as f:
        json.dump(tasks, f, ensure_ascii=False, indent=4)

for image in dst_path.glob("*.png"):
    new_file = template_path / image.name
    # print(new_file)
    if not new_file.exists():
        image.rename(new_file)
