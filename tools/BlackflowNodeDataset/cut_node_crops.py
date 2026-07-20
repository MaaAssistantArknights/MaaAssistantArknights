#!/usr/bin/env python3
"""从黑流树海地图截图批量切出节点小图，供人工归类和后续 ONNX 训练。"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

import cv2
import numpy as np


DEFAULT_INPUT = Path(r"C:\Users\10067\OneDrive\文档\MuMu共享文件夹\Screenshots\map\fin")
DEFAULT_EXTRACTOR = Path(r"C:\tmp\map_extractor_solution")
DEFAULT_OUTPUT_NAME = "node_dataset"

FALLBACK_CLASSES = [
    "MysteriousPresage",
    "FerociousPresage",
    "CombatOps",
    "EmergencyOps",
    "DreadfulFoe",
    "Encounter",
    "SafeHouse",
    "Boons",
    "FaceOff",
    "RogueTrader",
    "LostAndFound",
    "Scout",
    "PathEnd",
    "PathLane",
    "HiddenTrader",
    "EmergencyAid",
    "WindingPassage",
    "VantagePoint",
    "ResidentStronghold",
    "BoskyPassage",
    "Prophecy",
    "Recreation",
]


def read_image(path: Path) -> np.ndarray:
    """用 imdecode 读取，避免 Windows 中文路径经过 OpenCV ANSI 接口乱码。"""
    data = np.fromfile(path, dtype=np.uint8)
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if image is None:
        raise RuntimeError(f"无法读取图片: {path}")
    return image


def write_image(path: Path, image: np.ndarray, overwrite: bool) -> None:
    if path.exists() and not overwrite:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    ok, encoded = cv2.imencode(path.suffix or ".png", image)
    if not ok:
        raise RuntimeError(f"无法编码图片: {path}")
    encoded.tofile(path)


def natural_key(path: Path) -> tuple[int, str]:
    try:
        return int(path.stem), path.name
    except ValueError:
        return 10**9, path.name


def load_classes(repo_root: Path) -> list[str]:
    config_path = repo_root / "resource" / "roguelike" / "Blackflow" / "map.json"
    try:
        data = json.loads(config_path.read_text(encoding="utf-8"))
        classes = [str(node["type"]) for node in data["nodes"]]
        if classes:
            return classes
    except (OSError, KeyError, TypeError, json.JSONDecodeError):
        pass
    return FALLBACK_CLASSES


def load_extractor(extractor_dir: Path):
    if not extractor_dir.exists():
        raise RuntimeError(f"找不到地图提取器目录: {extractor_dir}")
    sys.path.insert(0, str(extractor_dir))
    try:
        from extract_map import MapExtractor
    except ImportError as exc:
        raise RuntimeError(
            f"无法导入 {extractor_dir / 'extract_map.py'}，请确认其虚拟环境和依赖已安装"
        ) from exc
    return MapExtractor()


def crop_from_box(image: np.ndarray, box: dict[str, Any]) -> np.ndarray:
    x = max(0, int(box["x"]))
    y = max(0, int(box["y"]))
    right = min(image.shape[1], x + int(box["w"]))
    bottom = min(image.shape[0], y + int(box["h"]))
    if right <= x or bottom <= y:
        raise RuntimeError(f"节点裁剪框为空: {box}")
    return image[y:bottom, x:right].copy()


def make_preview(image: np.ndarray, nodes: list[dict[str, Any]]) -> np.ndarray:
    preview = image.copy()
    for index, node in enumerate(nodes):
        box = node["coordinates"]
        x, y = int(box["x"]), int(box["y"])
        w, h = int(box["w"]), int(box["h"])
        color = (0, 220, 0) if node["type"] == "object" else (220, 180, 0)
        cv2.rectangle(preview, (x, y), (x + w, y + h), color, 2)
        cv2.putText(
            preview,
            f"{index}:c{node['col']}r{node['row']}",
            (x, max(18, y - 5)),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.45,
            color,
            1,
            cv2.LINE_AA,
        )
    return preview


def main() -> int:
    repo_root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT, help="截图目录")
    parser.add_argument(
        "--source-subdir",
        type=str,
        default=None,
        help="默认 fin 目录下的子目录，避免 Windows 中文路径参数编码问题",
    )
    parser.add_argument("--output", type=Path, default=None, help="输出数据集目录，默认是截图目录/node_dataset")
    parser.add_argument("--prefix", type=str, default=None, help="输出文件名前缀，避免追加素材时重名")
    parser.add_argument("--pattern", type=str, default="*.png", help="输入文件匹配模式，默认处理全部 PNG")
    parser.add_argument("--extractor", type=Path, default=DEFAULT_EXTRACTOR, help="现有无 OCR 地图提取器目录")
    parser.add_argument("--overwrite", action="store_true", help="覆盖已存在的小图和预览图")
    args = parser.parse_args()

    input_dir = DEFAULT_INPUT / args.source_subdir if args.source_subdir else args.input
    output_dir = args.output or (DEFAULT_INPUT / DEFAULT_OUTPUT_NAME if args.source_subdir else input_dir / DEFAULT_OUTPUT_NAME)
    if not input_dir.is_dir():
        raise SystemExit(f"输入目录不存在: {input_dir}")

    classes = load_classes(repo_root)
    (output_dir / "_unclassified").mkdir(parents=True, exist_ok=True)
    (output_dir / "_road").mkdir(parents=True, exist_ok=True)
    (output_dir / "_failed").mkdir(parents=True, exist_ok=True)
    (output_dir / "previews").mkdir(parents=True, exist_ok=True)
    for class_name in classes:
        (output_dir / class_name).mkdir(parents=True, exist_ok=True)

    extractor = load_extractor(args.extractor)
    manifest_path = output_dir / "manifest.json"
    failures_path = output_dir / "failures.json"
    try:
        manifest: list[dict[str, Any]] = json.loads(manifest_path.read_text(encoding="utf-8"))
        if not isinstance(manifest, list):
            manifest = []
    except (OSError, json.JSONDecodeError):
        manifest = []
    try:
        failures: list[dict[str, str]] = json.loads(failures_path.read_text(encoding="utf-8"))
        if not isinstance(failures, list):
            failures = []
    except (OSError, json.JSONDecodeError):
        failures = []
    existing_files = {str(item.get("file")) for item in manifest}
    prefix = args.prefix or (input_dir.name if args.source_subdir else "")

    for image_path in sorted(input_dir.glob(args.pattern), key=natural_key):
        try:
            image = read_image(image_path)
            result = extractor.extract(image, include_none=False)
            nodes = result["map"]["node"]
            stem = f"{prefix}_{image_path.stem}" if prefix else image_path.stem

            preview_path = output_dir / "previews" / f"{stem}_nodes.png"
            write_image(preview_path, make_preview(image, nodes), args.overwrite)

            for index, node in enumerate(nodes):
                if node.get("type") == "none":
                    continue
                crop = crop_from_box(image, node["coordinates"])
                kind = str(node["type"])
                target_dir = output_dir / ("_unclassified" if kind == "object" else "_road")
                filename = f"{stem}_n{index:02d}_c{int(node['col']):02d}_r{int(node['row']):02d}_{kind}.png"
                crop_path = target_dir / filename
                write_image(crop_path, crop, args.overwrite)
                relative_file = str(crop_path.relative_to(output_dir))
                if relative_file not in existing_files:
                    manifest.append(
                        {
                            "file": relative_file,
                            "source": str(image_path),
                            "source_stem": stem,
                            "index": index,
                            "col": int(node["col"]),
                            "row": int(node["row"]),
                            "kind": kind,
                            "coordinates": node["coordinates"],
                        }
                    )
                    existing_files.add(relative_file)
            print(f"{image_path.name}: {len(nodes)} nodes")
        except Exception as exc:  # noqa: BLE001 - 单张失败不能阻止其余截图处理
            failures.append({"image": str(image_path), "error": str(exc)})
            print(f"{image_path.name}: FAILED: {exc}")

    manifest_path.write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    failures_path.write_text(
        json.dumps(failures, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (output_dir / "README.txt").write_text(
        "把 _unclassified 中的 object 小图按节点类型移动到同名文件夹。\n"
        "_road 是自动分出的道路节点，不需要归类。\n"
        "previews 中是带网格编号的原图，manifest.json 记录每张小图来源。\n"
        "建议优先归类图标清晰、未被角色/连线遮挡的样本。\n",
        encoding="utf-8",
    )
    print(f"输出目录: {output_dir}")
    print(f"object 待归类: {sum(1 for item in manifest if item['kind'] == 'object')}")
    print(f"road: {sum(1 for item in manifest if item['kind'] == 'road')}")
    print(f"失败截图: {len(failures)}")
    return 0 if not failures else 2


if __name__ == "__main__":
    raise SystemExit(main())
