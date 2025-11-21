import json
from pathlib import Path

from cutter import dst_path, src_path, task_paths

"""
Update tasks.json for multi template tasks
"""

task_file = task_paths["CN"]
template_path = task_file.parent / "template"


def update_ui_theme_tasks():
    # Iterate over UiTheme json files
    for json_path in task_file.glob("UiTheme/*.json"):
        print(f"Processing {json_path}")
        with json_path.open("r", encoding="utf-8") as f:
            tasks = json.load(f)

        prefix = json_path.stem  # e.g. Depot
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

        # default_template is like "Default/DepotEnter.png"
        # We want "DepotEnter.png"
        template_suffix = default_template.split("/")[-1]

        entry_task = tasks[entry_task_name]
        next_list = entry_task.get("next", [])

        modified = False

        for image in src_path.glob("*.png"):
            theme_name = image.stem  # e.g. Together
            new_task_name = f"{prefix}{theme_name}"

            # Check if task already exists
            if new_task_name in tasks:
                continue

            print(f"Adding new task: {new_task_name}")

            # Create new task
            new_task = {
                "baseTask": default_task_name,
                "template": f"{theme_name}/{template_suffix}",
            }

            # Add to tasks
            tasks[new_task_name] = new_task

            # Add to next list
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
