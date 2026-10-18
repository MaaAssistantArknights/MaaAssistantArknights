#!/usr/bin/env python3
"""Add NCNN OCR models to a staged Android resource directory."""

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path


def convert_models(resource_dir: Path) -> None:
    models = sorted(
        path
        for path in resource_dir.rglob("inference.onnx")
        if path.parent.name in ("det", "rec")
    )
    if not models:
        raise ValueError(f"No OCR models found in {resource_dir}")

    for model in models:
        kind = model.parent.name
        shape = "[1,3,640,640]" if kind == "det" else "[1,3,48,320]"
        print(f"Converting {model.relative_to(resource_dir)}", flush=True)
        with tempfile.TemporaryDirectory(prefix="maa-ocr-ncnn-") as directory:
            work = Path(directory)
            shutil.copyfile(model, work / f"{kind}.onnx")
            # Keep DBNet probabilities in fp32 and resolve SVTR's dynamic shapes.
            subprocess.run(
                ["pnnx", f"{kind}.onnx", f"inputshape={shape}", "fp16=0"],
                cwd=work,
                check=True,
            )
            outputs = [work / f"{kind}.ncnn.{suffix}" for suffix in ("param", "bin")]
            for output in outputs:
                if not output.is_file() or output.stat().st_size == 0:
                    raise RuntimeError(f"Missing or empty NCNN output: {output}")
            for output in outputs:
                shutil.copyfile(output, model.parent / output.name)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("resource_dir", type=Path)
    convert_models(parser.parse_args().resource_dir)
