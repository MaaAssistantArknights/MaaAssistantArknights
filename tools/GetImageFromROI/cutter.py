import cv2
import json
from pathlib import Path

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

task_paths = {
    "CN": Path(__file__).parent.parent.parent / "resource" / "tasks",
    "twxy": Path(__file__).parent.parent.parent / "resource" / "global" / "txwy" / "resource" / "tasks",
    "YoStarEN": Path(__file__).parent.parent.parent / "resource" / "global" / "YoStarEN" / "resource" / "tasks",
    "YoStarJP": Path(__file__).parent.parent.parent / "resource" / "global" / "YoStarJP" / "resource" / "tasks",
    "YoStarKR": Path(__file__).parent.parent.parent / "resource" / "global" / "YoStarKR" / "resource" / "tasks"
}

# Change this to your client
task_file = task_paths["CN"]

# Add the tasks you want to get its image
# task_list = []
# You should get these names from tasks.json
#
# For example, when adding a new homepage theme:
task_list = [
    "Award",          # Task.png
    "GachaEnter",     # GachaEnter.png
    "Home",           # Terminal.png
    "Infrast",        # EnterInfrast.png
    "OperBoxEnter",   # OperBoxEnter.png
    "Recruit",        # Recruit.png
    "Visit",          # Friends.png
    "DepotEnter",     # DepotEnter.png
    "Mall",           # Mall.png
]

src_path = Path(__file__).parent / "src"
dst_path = Path(__file__).parent / "dst"


if __name__ == "__main__":
    tasks = {}
    if task_file.is_dir():
        for json_file in task_file.rglob("*.json"):
            with json_file.open("r", encoding="utf-8") as f:
                tasks.update(json.load(f))
    else:
        with task_file.open("r", encoding="utf-8") as f:
            tasks = json.load(f)

    for raw_image in src_path.glob("*.png"):
        print("Processing file:", str(raw_image))
        image = cv2.imread(str(raw_image))
        cur_ratio = image.shape[1] / image.shape[0]
        if cur_ratio >= std_ratio:  # 说明是宽屏或默认16:9，按照高度计算缩放
            dsize_width: int = (int)(cur_ratio * std_height)
            dsize_height: int = std_height
        else:  # 否则可能是偏正方形的屏幕，按宽度计算
            dsize_width: int = std_width
            dsize_height: int = std_width / cur_ratio
        dsize = (dsize_width, dsize_height)
        image = cv2.resize(image, dsize, interpolation=cv2.INTER_AREA)

        for i in task_list:
            if "template" not in tasks[i]:
                filename = i + ".png"
            elif isinstance(tasks[i]["template"], str):
                filename = tasks[i]["template"]
            elif isinstance(tasks[i]["template"], list):
                # this is for multi-temSplate:
                filename = (
                    tasks[i]["template"][0].split(".")[0] + raw_image.stem + ".png"
                )

            crop_doc = tasks[i].get("crop_doc", {})
            roi = crop_doc.get("roi", tasks[i]["roi"])
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

            print("Saving", dst_path / filename)
            cv2.imwrite(str(dst_path / filename), cropped)

        print("Finished processing file:", str(raw_image))
