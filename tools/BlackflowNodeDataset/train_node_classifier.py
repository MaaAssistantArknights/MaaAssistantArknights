#!/usr/bin/env python3
"""Train a Drowning Seekers node-type classifier from manually sorted crops.

The exported model is consumed by the C++ map analyzer as the node-type
classifier.  Keep the feature extractor stable: the C++ implementation must
produce the same 2118 values in the same order.

The feature extractor deliberately normalizes brightness per crop and combines
shape (HOG), grayscale layout, and HSV histograms.  This makes it less sensitive
to the different brightness levels seen in the screenshots than raw pixels.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

import cv2
import numpy as np
from sklearn.ensemble import ExtraTreesClassifier
from sklearn.metrics import accuracy_score, balanced_accuracy_score, f1_score, top_k_accuracy_score
from sklearn.model_selection import GroupShuffleSplit, train_test_split
from skl2onnx import convert_sklearn
from skl2onnx.common.data_types import FloatTensorType


DEFAULT_DATASET = Path(r"C:\Users\10067\OneDrive\文档\MuMu共享文件夹\Screenshots\map\fin\node_dataset")
DEFAULT_OUTPUT = DEFAULT_DATASET / "model"
SPECIAL_DIRS = {"_failed", "_road", "_unclassified", "previews", "model"}
GROUP_RE = re.compile(r"^(.*)_n\d+_c\d+_r\d+(?:_copy\d+)?_(?:object|road)\.png$")


@dataclass(frozen=True)
class Record:
    path: Path
    label: str
    group: str
    digest: str


def read_image(path: Path) -> np.ndarray:
    data = np.fromfile(path, dtype=np.uint8)
    image = cv2.imdecode(data, cv2.IMREAD_COLOR)
    if image is None:
        raise RuntimeError(f"cannot read image: {path}")
    return image


def hog_features(gray: np.ndarray) -> np.ndarray:
    """Small dependency-free HOG implementation (8x8 cells, 9 bins)."""
    gx = cv2.Sobel(gray, cv2.CV_32F, 1, 0, ksize=3)
    gy = cv2.Sobel(gray, cv2.CV_32F, 0, 1, ksize=3)
    magnitude, angle = cv2.cartToPolar(gx, gy, angleInDegrees=True)
    angle %= 180.0
    cell = 8
    bins = 9
    cell_hist = np.zeros((8, 8, bins), dtype=np.float32)
    for row in range(8):
        for col in range(8):
            cell_mag = magnitude[row * cell:(row + 1) * cell, col * cell:(col + 1) * cell]
            cell_angle = angle[row * cell:(row + 1) * cell, col * cell:(col + 1) * cell]
            positions = cell_angle / 20.0
            lower = np.floor(positions).astype(np.int32) % bins
            upper = (lower + 1) % bins
            upper_weight = positions - np.floor(positions)
            lower_weight = 1.0 - upper_weight
            for bin_index in range(bins):
                cell_hist[row, col, bin_index] = np.sum(
                    cell_mag * ((lower == bin_index) * lower_weight + (upper == bin_index) * upper_weight)
                )
    blocks = []
    for row in range(7):
        for col in range(7):
            block = cell_hist[row:row + 2, col:col + 2].ravel()
            block = block / np.sqrt(np.sum(block * block) + 1e-6 ** 2)
            block = np.minimum(block, 0.2)
            block = block / np.sqrt(np.sum(block * block) + 1e-6 ** 2)
            blocks.append(block)
    return np.concatenate(blocks).astype(np.float32)


def feature_vector(image: np.ndarray) -> np.ndarray:
    """Extract a fixed-size, brightness-tolerant feature vector."""
    image = cv2.resize(image, (64, 64), interpolation=cv2.INTER_AREA)
    hsv = cv2.cvtColor(image, cv2.COLOR_BGR2HSV)
    gray = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY).astype(np.float32)

    # Per-image luminance normalization removes most emulator/screenshot
    # brightness differences while retaining the icon's spatial structure.
    gray = (gray - gray.mean()) / (gray.std() + 1e-5)
    gray_small = cv2.resize(gray, (16, 16), interpolation=cv2.INTER_AREA).ravel()

    normalized_gray = np.clip((gray - gray.min()) / (gray.max() - gray.min() + 1e-5) * 255, 0, 255).astype(np.float32)
    hog = hog_features(normalized_gray)

    # Histograms retain robust color/material information without depending on
    # the exact crop size or absolute brightness.
    hist_h = cv2.calcHist([hsv], [0], None, [18], [0, 180]).ravel()
    hist_s = cv2.calcHist([hsv], [1], None, [16], [0, 256]).ravel()
    hist_v = cv2.calcHist([hsv], [2], None, [16], [0, 256]).ravel()
    hist = np.concatenate((hist_h, hist_s, hist_v)).astype(np.float32)
    hist /= hist.sum() + 1e-6

    # Coarse spatial HSV means are useful for similarly shaped but differently
    # colored node symbols, and are stable under small crop shifts.
    spatial = []
    for channel in (hsv[:, :, 0] / 180.0, hsv[:, :, 1] / 255.0, hsv[:, :, 2] / 255.0):
        for row in np.array_split(channel, 4, axis=0):
            for cell in np.array_split(row, 4, axis=1):
                spatial.append(float(cell.mean()))

    return np.concatenate((hog.astype(np.float32), gray_small, hist, np.asarray(spatial, dtype=np.float32)))


def image_digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def records_from_dataset(dataset: Path) -> list[Record]:
    records: list[Record] = []
    for class_dir in sorted(dataset.iterdir()):
        if not class_dir.is_dir() or class_dir.name in SPECIAL_DIRS:
            continue
        for path in sorted(class_dir.glob("*.png")):
            match = GROUP_RE.match(path.name)
            group = match.group(1) if match else path.stem
            records.append(Record(path, class_dir.name, group, image_digest(path)))
    return records


def print_dataset_report(records: list[Record]) -> None:
    by_label: dict[str, int] = {}
    for record in records:
        by_label[record.label] = by_label.get(record.label, 0) + 1
    duplicate_groups: dict[str, list[str]] = {}
    for record in records:
        duplicate_groups.setdefault(record.digest, []).append(str(record.path))
    duplicates = [paths for paths in duplicate_groups.values() if len(paths) > 1]
    print(f"samples: {len(records)}")
    print(f"classes: {len(by_label)}")
    print("class counts:")
    for label, count in sorted(by_label.items(), key=lambda item: (item[1], item[0])):
        print(f"  {label}: {count}")
    print(f"exact duplicate image groups: {len(duplicates)}")


def evaluate(model: ExtraTreesClassifier, x: np.ndarray, y: np.ndarray, labels: list[str], title: str) -> dict[str, float]:
    prediction = model.predict(x)
    probabilities = model.predict_proba(x)
    metrics = {
        "accuracy": float(accuracy_score(y, prediction)),
        "balanced_accuracy": float(balanced_accuracy_score(y, prediction)),
        "macro_f1": float(f1_score(y, prediction, average="macro", zero_division=0)),
    }
    print(f"{title}:")
    print(f"  accuracy: {metrics['accuracy']:.4f}")
    print(f"  balanced_accuracy: {metrics['balanced_accuracy']:.4f}")
    print(f"  macro_f1: {metrics['macro_f1']:.4f}")
    try:
        metrics["top2_accuracy"] = float(top_k_accuracy_score(y, probabilities, k=2, labels=np.arange(len(labels))))
        print(f"  top2_accuracy: {metrics['top2_accuracy']:.4f}")
    except ValueError:
        # A group holdout can legitimately omit a rare class from the test set.
        pass
    return metrics


def make_model(seed: int) -> ExtraTreesClassifier:
    return ExtraTreesClassifier(
        n_estimators=600,
        max_features=0.5,
        min_samples_leaf=1,
        class_weight="balanced",
        n_jobs=-1,
        random_state=seed,
    )


def load_features(records: list[Record]) -> np.ndarray:
    rows = []
    for index, record in enumerate(records, 1):
        rows.append(feature_vector(read_image(record.path)))
        if index % 100 == 0 or index == len(records):
            print(f"features: {index}/{len(records)}")
    return np.asarray(rows, dtype=np.float32)


def duplicate_safe_stratified_split(records: list[Record], y: np.ndarray, seed: int) -> tuple[np.ndarray, np.ndarray]:
    """Split by unique image content so copied samples cannot leak across splits."""
    digest_to_indices: dict[str, list[int]] = {}
    for index, record in enumerate(records):
        digest_to_indices.setdefault(record.digest, []).append(index)
    unique_digests = list(digest_to_indices)
    unique_indices = np.asarray([digest_to_indices[digest][0] for digest in unique_digests], dtype=np.int64)
    unique_y = y[unique_indices]
    unique_train, unique_test = train_test_split(
        np.arange(len(unique_digests)), test_size=0.2, random_state=seed, stratify=unique_y
    )
    train_digests = {unique_digests[index] for index in unique_train}
    test_digests = {unique_digests[index] for index in unique_test}
    train_indices = np.asarray([index for index, record in enumerate(records) if record.digest in train_digests], dtype=np.int64)
    test_indices = np.asarray([index for index, record in enumerate(records) if record.digest in test_digests], dtype=np.int64)
    return train_indices, test_indices


def unique_indices(records: list[Record], indices: np.ndarray) -> np.ndarray:
    """Keep one representative per image digest for unbiased reporting."""
    seen: set[str] = set()
    result = []
    for index in indices:
        digest = records[int(index)].digest
        if digest not in seen:
            seen.add(digest)
            result.append(int(index))
    return np.asarray(result, dtype=np.int64)


def export_model(model: ExtraTreesClassifier, output_dir: Path, labels: list[str], feature_count: int, report: dict) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    onnx = convert_sklearn(
        model,
        initial_types=[("X", FloatTensorType([None, feature_count]))],
        target_opset=15,
        options={id(model): {"zipmap": False}},
    )
    (output_dir / "Blackflow_node_type.onnx").write_bytes(onnx.SerializeToString())
    (output_dir / "labels.json").write_text(json.dumps(labels, ensure_ascii=False, indent=2), encoding="utf-8")
    (output_dir / "report.json").write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
    (output_dir / "README.txt").write_text(
        "Drowning Seekers node type classifier.\n"
        "Input: float32 feature vector generated by train_node_classifier.py (not raw pixels).\n"
        "The model is consumed by the C++ analyzer as Blackflow_node_type.\n",
        encoding="utf-8",
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", type=Path, default=DEFAULT_DATASET)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--seed", type=int, default=20260719)
    args = parser.parse_args()

    records = records_from_dataset(args.dataset)
    if not records:
        raise SystemExit(f"no class images found in {args.dataset}")
    print_dataset_report(records)
    labels = sorted({record.label for record in records})
    label_to_id = {label: index for index, label in enumerate(labels)}
    y = np.asarray([label_to_id[record.label] for record in records], dtype=np.int64)
    groups = np.asarray([record.group for record in records])
    x = load_features(records)

    repo_root = Path(__file__).resolve().parents[2]
    try:
        map_data = json.loads((repo_root / "resource" / "roguelike" / "Blackflow" / "map.json").read_text(encoding="utf-8"))
        expected_labels = sorted({str(node["type"]) for node in map_data["nodes"]})
    except (OSError, KeyError, TypeError, json.JSONDecodeError):
        expected_labels = labels
    missing_labels = sorted(set(expected_labels) - set(labels))
    print(f"empty expected classes: {', '.join(missing_labels) if missing_labels else 'none'}")
    report: dict = {
        "labels": labels,
        "expected_labels": expected_labels,
        "missing_labels": missing_labels,
        "samples": len(records),
        "feature_count": int(x.shape[1]),
    }

    train_idx, test_idx = duplicate_safe_stratified_split(records, y, args.seed)
    split_model = make_model(args.seed)
    split_model.fit(x[train_idx], y[train_idx])
    random_test_unique = unique_indices(records, test_idx)
    random_metrics = evaluate(split_model, x[random_test_unique], y[random_test_unique], labels, "stratified random holdout")

    group_train, group_test = next(GroupShuffleSplit(n_splits=1, test_size=0.2, random_state=args.seed).split(x, y, groups))
    group_model = make_model(args.seed)
    group_model.fit(x[group_train], y[group_train])
    group_test_unique = unique_indices(records, group_test)
    group_metrics = evaluate(group_model, x[group_test_unique], y[group_test_unique], labels, "screenshot-group holdout")

    final_model = make_model(args.seed)
    final_model.fit(x, y)
    report["random_holdout"] = {
        "train_samples_with_copies": len(train_idx),
        "test_unique_images": len(random_test_unique),
        **random_metrics,
    }
    report["group_holdout"] = {
        "train_samples_with_copies": len(group_train),
        "test_unique_images": len(group_test_unique),
        "test_groups": sorted(set(groups[group_test])),
        **group_metrics,
    }
    export_model(final_model, args.output, labels, int(x.shape[1]), report)
    print(f"saved: {args.output / 'Blackflow_node_type.onnx'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
