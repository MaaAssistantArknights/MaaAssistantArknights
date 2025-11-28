import re

# 攻略：https://www.bilibili.com/video/BV16NSZYbE85/
# 视频格式：左1-1 2-3 3-1 右1-3 3-1 3-2
# MAA 内部格式：
# MiniGame@PV@Filter1-1 （左1-）
# MiniGame@PV@Options-1 （-1）
# 映射：左/右 → Filter 的第二位
side_map = {"左": "1", "右": "2"}


def convert_steps(text):
    parts = text.split()
    result = []

    current_filter = None
    current_side = None

    for part in parts:
        # 判断是否带 左/右
        m = re.match(r"(左|右)?(\d+)-(\d+)", part)
        if not m:
            continue

        side, group, option = m.groups()

        # 如果没有写左/右，则沿用前一次
        if side is None:
            side = current_side

        filter_id = f"Filter{group}-{side_map[side]}"
        option_id = f"Options-{option}"

        # 如果 Filter 改变了，先写 Filter
        if filter_id != current_filter:
            result.append(f"MiniGame@PV@{filter_id}")
            current_filter = filter_id
            current_side = side

        # 再写 Options
        result.append(f"MiniGame@PV@{option_id}")

    result.append("MiniGame@PV@Confirm")
    return result


texts = [
    "左1-4 2-2 右1-2 1-4 2-1 2-2",
    "左1-1 2-3 右2-2 2-4",
    "左1-2 1-4 2-1 3-2 右2-2",
    "左1-4 3-2 右1-3 3-4",
    "左1-1 1-3 2-1 2-3 右2-1 2-2",
    "左1-2 1-4 2-2 2-3 右1-3 2-2",
    "左1-1 1-3 2-3 右1-3 2-1",
    "左1-2 1-4 2-1 3-1 右1-2",
    "左1-4 2-2 3-3 右1-3 2-3",
    "左1-1 1-4 2-3 右1-1 1-2 2-2",
    "左1-2 2-3 3-2 右1-1",
    "左2-2 2-4 右1-1 2-3",
    "右1-1 2-2 2-4",
    "左1-1 2-4 右1-2 2-4",
    "左1-2 1-4 右1-2 1-4 2-1",
    "左2-2 3-2 3-4 右1-1 1-3 2-2",
    "左1-4 2-1 3-1 3-3 右1-2",
    "左2-1 3-1 右1-2 1-4 2-2",
    "左1-2 1-4 3-2 3-4 右2-1",
    "左1-1 2-3 3-1 右1-3 3-1 3-2",
]

for idx, text in enumerate(texts):
    print(f"\n// 第 {idx + 1} 行: {text}")
    converted = convert_steps(text)
    print('"sub": [')
    for i, line in enumerate(converted):
        comma = "," if i < len(converted) - 1 else ""
        print(f'            "{line}"{comma}')
    print("        ],")
