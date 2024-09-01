import json
from cutter import task_paths, task_list, src_path, dst_path

"""
Update tasks.json for multi template tasks
"""

task_file = task_paths["CN"]
template_path = task_file.parent / "template"

with task_file.open("r", encoding="utf-8") as f:
    tasks = json.load(f)

default_tmpl_names = [tasks[i]["template"][0] for i in task_list]

for image in src_path.glob("*.png"):
    image_name = image.stem
    for task in tasks:
        task_info = tasks[task]
        if "template" in task_info.keys() and isinstance(task_info["template"], list):
            if (default_tmpl_name := task_info["template"][0]) in default_tmpl_names:
                task_info["template"].append(
                    default_tmpl_name.split(".")[0] + image_name + ".png"
                )

with task_file.open("w", encoding="utf-8") as f:
   json.dump(tasks, f, ensure_ascii=False, indent=4)

for image in dst_path.glob("*.png"):
    new_file = template_path / image.name
    if not new_file.exists():
        image.rename(new_file)
