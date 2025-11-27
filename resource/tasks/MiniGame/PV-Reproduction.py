import re

# 映射：左/右 → Filter 的第二位
side_map = {
    "左": "1",
    "右": "2"
}

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

    return result


text = "左1-4 2-2 右1-2 1-4 2-1 2-2"
converted = convert_steps(text)

print('"sub": [')
for i, line in enumerate(converted):
    comma = "," if i < len(converted) - 1 else ""
    print(f'    "{line}"{comma}')
print("]")
