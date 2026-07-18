#!/usr/bin/env python3
"""Create contact sheets for node-classifier validation errors."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

import cv2
import numpy as np
from sklearn.model_selection import GroupShuffleSplit, train_test_split

from train_node_classifier import (
    DEFAULT_DATASET,
    DEFAULT_OUTPUT,
    feature_vector,
    duplicate_safe_stratified_split,
    unique_indices,
    load_features,
    make_model,
    records_from_dataset,
)


def make_sheet(items: list[tuple], output: Path, title: str) -> None:
    if not items:
        print(f"{title}: no errors")
        return
    cols = 4
    thumb_w, thumb_h = 240, 300
    rows = (len(items) + cols - 1) // cols
    sheet = np.full((rows * thumb_h, cols * thumb_w, 3), 28, dtype=np.uint8)
    for index, (record, actual, predicted, confidence, top2) in enumerate(items):
        image = cv2.imdecode(np.fromfile(record.path, dtype=np.uint8), cv2.IMREAD_COLOR)
        image = cv2.resize(image, (220, 220), interpolation=cv2.INTER_NEAREST)
        x = (index % cols) * thumb_w + 10
        y = (index // cols) * thumb_h + 10
        sheet[y:y + 220, x:x + 220] = image
        lines = [
            f"actual: {actual}",
            f"pred: {predicted} ({confidence:.2f})",
            f"2nd: {top2[0]} ({top2[1]:.2f})",
            record.path.name[:34],
        ]
        for line_index, line in enumerate(lines):
            cv2.putText(sheet, line, (x, y + 240 + line_index * 14), cv2.FONT_HERSHEY_SIMPLEX, 0.34, (235, 235, 235), 1, cv2.LINE_AA)
    output.parent.mkdir(parents=True, exist_ok=True)
    ok, encoded = cv2.imencode(".png", sheet)
    if not ok:
        raise RuntimeError(f"cannot encode {output}")
    encoded.tofile(output)
    print(f"{title}: {len(items)} errors -> {output}")


def collect_errors(records, labels, model, indices, x, y):
    probabilities = model.predict_proba(x[indices])
    predictions = model.predict(x[indices])
    result = []
    for local_index, record_index in enumerate(indices):
        if predictions[local_index] == y[record_index]:
            continue
        order = np.argsort(probabilities[local_index])[::-1]
        top2 = (labels[order[1]], float(probabilities[local_index, order[1]]))
        result.append((
            records[record_index],
            labels[y[record_index]],
            labels[predictions[local_index]],
            float(probabilities[local_index, predictions[local_index]]),
            top2,
        ))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dataset", type=Path, default=DEFAULT_DATASET)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--seed", type=int, default=20260719)
    args = parser.parse_args()

    records = records_from_dataset(args.dataset)
    labels = sorted({record.label for record in records})
    label_to_id = {label: index for index, label in enumerate(labels)}
    y = np.asarray([label_to_id[record.label] for record in records], dtype=np.int64)
    groups = np.asarray([record.group for record in records])
    x = load_features(records)
    all_indices = np.arange(len(records))

    train_idx, test_idx = duplicate_safe_stratified_split(records, y, args.seed)
    random_model = make_model(args.seed).fit(x[train_idx], y[train_idx])
    random_errors = collect_errors(records, labels, random_model, unique_indices(records, test_idx), x, y)
    make_sheet(random_errors, args.output / "errors_random_holdout.png", "random holdout")

    group_train, group_test = next(GroupShuffleSplit(n_splits=1, test_size=0.2, random_state=args.seed).split(x, y, groups))
    group_model = make_model(args.seed).fit(x[group_train], y[group_train])
    group_errors = collect_errors(records, labels, group_model, unique_indices(records, group_test), x, y)
    make_sheet(group_errors, args.output / "errors_screenshot_group_holdout.png", "screenshot-group holdout")

    details = {
        "random_holdout": [
            {"file": str(item[0].path), "actual": item[1], "predicted": item[2], "confidence": item[3], "second": item[4]}
            for item in random_errors
        ],
        "screenshot_group_holdout": [
            {"file": str(item[0].path), "actual": item[1], "predicted": item[2], "confidence": item[3], "second": item[4]}
            for item in group_errors
        ],
    }
    (args.output / "errors.json").write_text(json.dumps(details, ensure_ascii=False, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
