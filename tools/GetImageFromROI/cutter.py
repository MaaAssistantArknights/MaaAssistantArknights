import json
from pathlib import Path

import cv2

from tools.ImageCropper.main import crop

"""
This is a tool for cropping one image into several images based on current roi.
Use this tool when a raw image contains many templates.
For example, when adding a new homepage theme.
Put one picture in ./src and add the template's tasks to 'task_list',
This tools will try to read roi (if not 'crop_doc') from tasks in tasks.json and then crop the image.
"""

std_width: int = 1280
std_height: int = 720
std_ratio = std_width / std_height

project_base_path = Path(__file__).parent.parent.parent
task_paths = {
    "CN": project_base_path / "resource" / "tasks",
    "twxy": project_base_path / "resource/global/txwy/resource/tasks",
    "YoStarEN": project_base_path / "resource/global/YoStarEN/resource/tasks",
    "YoStarJP": project_base_path / "resource/global/YoStarJP/resource/tasks",
    "YoStarKR": project_base_path / "resource/global/YoStarKR/resource/tasks",
}

task_files = task_paths["CN"].glob("UiTheme/*.json")

src_path = Path(__file__).parent / "src"
dst_path = Path(__file__).parent / "dst"


if __name__ == "__main__":
    tasks = {}
    for task_file in task_files:
        print("Processing task file:", str(task_file))
        with task_file.open("r", encoding="utf-8") as f:
            task_data = json.load(f)

        crop_doc = task_data.get("crop_doc", {})
        for name, task in task_data.items():
            if name == "crop_doc":
                continue
            if crop_doc:
                task.setdefault("crop_doc", crop_doc)
            tasks[name] = task

    for raw_image in src_path.glob("*.png"):
        print("Processing file:", str(raw_image))
        image = cv2.imread(str(raw_image))
        if image is None:
            print(f"Failed to read image: {raw_image}")
            continue

        cur_ratio = image.shape[1] / image.shape[0]
        if cur_ratio >= std_ratio:  # 说明是宽屏或默认16:9，按照高度计算缩放
            dsize_width: int = (int)(cur_ratio * std_height)
            dsize_height: int = std_height
        else:  # 否则可能是偏正方形的屏幕，按宽度计算
            dsize_width: int = std_width
            dsize_height: int = std_width / cur_ratio
        dsize = (dsize_width, dsize_height)
        image = cv2.resize(image, dsize, interpolation=cv2.INTER_AREA)

        for i in tasks:
            filename = ""
            if "template" not in tasks[i]:
                continue

            template = tasks[i]["template"]
            if isinstance(template, str):
                filename = template
            elif isinstance(template, list):
                filename = template[0]

            if not filename.startswith(f"{raw_image.stem}/"):
                continue

            crop_doc = tasks[i].get("crop_doc", {})
            roi = crop_doc.get("roi")

            if not roi:
                curr = tasks[i]
                while curr:
                    if "roi" in curr:
                        roi = curr["roi"]
                        break
                    if "baseTask" in curr and curr["baseTask"] in tasks:
                        curr = tasks[curr["baseTask"]]
                    else:
                        break

            if not roi:
                continue

            mask = crop_doc.get("mask", [])

            cropped = image[roi[1] : roi[1] + roi[3], roi[0] : roi[0] + roi[2]]

            if len(mask) > 0:
                cv2.rectangle(
                    cropped,
                    (mask[0], mask[1]),
                    (mask[0] + mask[2], mask[1] + mask[3]),
                    (0, 0, 0),
                    -1,
                )

            output_file = dst_path / filename
            output_file.parent.mkdir(parents=True, exist_ok=True)
            print("Saving", output_file)
            cv2.imwrite(str(output_file), cropped)

        print("Finished processing file:", str(raw_image))
