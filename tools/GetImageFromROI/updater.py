import json
from pathlib import Path

from cutter import dst_path, src_path, task_paths

"""
Update tasks.json for multi template tasks
"""

task_file = task_paths["CN"]
template_path = task_file.parent / "template"


def update_ui_theme_tasks():
    for json_path in task_file.glob("UiTheme/*.json"):
        print(f"Processing {json_path}")
        with json_path.open("r", encoding="utf-8") as f:
            tasks = json.load(f)

        prefix = json_path.stem
        entry_task_name = f"{prefix}-Entry"
        default_task_name = f"{prefix}Default"

        if entry_task_name not in tasks or default_task_name not in tasks:
            print(f"Skipping {json_path}: Entry or Default task not found.")
            continue

        default_task = tasks[default_task_name]
        default_template = default_task.get("template")
        if not default_template or not isinstance(default_template, str):
            print(f"Skipping {json_path}: Default template invalid.")
            continue

        template_suffix = default_template.split("/")[-1]

        entry_task = tasks[entry_task_name]
        next_list = entry_task.get("next", [])

        modified = False

        theme_variants = {}
        for image in src_path.rglob("*.png"):
            theme_path = image.relative_to(src_path).with_suffix("")
            group_key = theme_path.parts[0]
            theme_variants.setdefault(group_key, []).append(theme_path)

        for group_key, variants in theme_variants.items():
            new_task_name = f"{prefix}{group_key}"
            if new_task_name in tasks:
                continue
            print(f"Adding new task: {new_task_name}")
            templates = [(variant / template_suffix).as_posix() for variant in variants]
            template_value = templates[0] if len(templates) == 1 else templates
            new_task = {
                "baseTask": default_task_name,
                "template": template_value,
            }
            tasks[new_task_name] = new_task
            if new_task_name not in next_list:
                next_list.append(new_task_name)
            modified = True

        if modified:
            entry_task["next"] = next_list
            with json_path.open("w", encoding="utf-8") as f:
                json.dump(tasks, f, ensure_ascii=False, indent=4)
                print(f"Updated {json_path}")


def move_generated_images():
    for image_path in dst_path.rglob("*.png"):
        rel_path = image_path.relative_to(dst_path)
        target_path = template_path / rel_path

        if not target_path.parent.exists():
            target_path.parent.mkdir(parents=True, exist_ok=True)

        if not target_path.exists():
            print(f"Moving {image_path} to {target_path}")
            image_path.rename(target_path)


if __name__ == "__main__":
    update_ui_theme_tasks()
    move_generated_images()
