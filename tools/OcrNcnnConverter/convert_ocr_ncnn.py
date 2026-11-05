#!/usr/bin/env python3
"""将 resource/ 下 PP-OCR 的 det/rec inference.onnx 转换为 Android 专用的 ncnn 权重。

产物按相同相对路径写入 resource_android/，仅 Android 安装时叠加到 resource/。
增删 OCR 语言或替换 inference.onnx 后，须在本地重新运行本脚本并提交 resource_android/。

用法：
    python -m pip install pnnx==20260526
    python tools/OcrNcnnConverter/convert_ocr_ncnn.py          # 转换并清理多余产物
    python tools/OcrNcnnConverter/convert_ocr_ncnn.py --check  # 仅检查，无需 pnnx
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import sys
import tempfile
from importlib import metadata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# 产出已验证转换结果的 pnnx 版本，升级须重新验证识别结果
PNNX_VERSION = "20260526"

INPUTSHAPE = {"det": "[1,3,640,640]", "rec": "[1,3,48,320]"}
# rec 需动态宽度：FastDeploy 按行宽高比把画布加宽到 48 * max(320 / 48, ratio)，
# 只给一个 inputshape 时 pnnx 会把序列长度 40 写死进 Reshape / Gemm，长文本行只能横向压缩
INPUTSHAPE2 = {"rec": "[1,3,48,960]"}
NCNN_SUFFIXES = ("param", "bin")


def find_models(resource: Path) -> list[tuple[Path, str]]:
    models = []
    for onnx in sorted(resource.rglob("inference.onnx")):
        kind = onnx.parent.name
        if kind in INPUTSHAPE:
            models.append((onnx, kind))
    if not models:
        raise SystemExit(f"no det/rec inference.onnx under {resource}")
    return models


def ncnn_targets(onnx: Path, kind: str, resource: Path, output: Path) -> list[Path]:
    rel_dir = onnx.parent.relative_to(resource)
    return [output / rel_dir / f"{kind}.ncnn.{suffix}" for suffix in NCNN_SUFFIXES]


def existing_ncnn(output: Path) -> set[Path]:
    if not output.is_dir():
        return set()
    return {
        p
        for suffix in NCNN_SUFFIXES
        for p in output.rglob(f"*.ncnn.{suffix}")
        if p.is_file()
    }


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def find_pnnx() -> str:
    exe = shutil.which("pnnx")
    try:
        installed = metadata.version("pnnx")
    except metadata.PackageNotFoundError:
        installed = None
    if exe is None or installed is None:
        raise SystemExit(
            f"pnnx not found, run: python -m pip install pnnx=={PNNX_VERSION}"
        )
    if installed != PNNX_VERSION:
        raise SystemExit(
            f"pnnx {installed} installed, "
            f"but the recipe is validated with pnnx=={PNNX_VERSION}"
        )
    return exe


def pnnx_once(pnnx: str, workdir: Path, cmd: list[str]) -> subprocess.CompletedProcess:
    for p in workdir.iterdir():
        if p.suffix != ".onnx" or p.name.endswith(".pnnxsim.onnx"):
            p.unlink()
    return subprocess.run(
        [pnnx, *cmd],
        cwd=workdir,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
    )


def run_pnnx(
    pnnx: str, onnx: Path, kind: str, workdir: Path
) -> tuple[tuple[Path, Path], bool]:
    """返回 (param, bin) 与是否为动态形状转换。"""
    work_onnx = workdir / f"{kind}.onnx"
    shutil.copy(onnx, work_onnx)
    # det 用 fp16 会破坏 DBNet 概率图
    cmd = [work_onnx.name, f"inputshape={INPUTSHAPE[kind]}", "fp16=0"]
    dynamic = kind in INPUTSHAPE2
    res = None
    if dynamic:
        res = pnnx_once(pnnx, workdir, [*cmd, f"inputshape2={INPUTSHAPE2[kind]}"])
        # 部分导出的 onnx（如 PaddleCharOCR）带动态 Shape/Slice，pnnx 处理 inputshape2 时会崩溃，
        # 退回静态宽度：OcrPackNcnn 加载时探测得到，长行仍按 320 宽压缩
        dynamic = res.returncode == 0
    if not dynamic:
        res = pnnx_once(pnnx, workdir, cmd)
    if res.returncode != 0:
        sys.stderr.write(res.stdout)
        raise SystemExit(f"pnnx failed (exit={res.returncode}): {' '.join(cmd)}")
    outputs = (workdir / f"{kind}.ncnn.param", workdir / f"{kind}.ncnn.bin")
    for p in outputs:
        if not p.is_file() or p.stat().st_size == 0:
            sys.stderr.write(res.stdout)
            raise SystemExit(f"pnnx output missing or empty: {p.name}")
    return outputs, dynamic


def remove_empty_dirs(start: Path, stop: Path) -> None:
    d = start
    while d != stop and d.is_dir() and not any(d.iterdir()):
        d.rmdir()
        d = d.parent


def convert(resource: Path, output: Path) -> None:
    models = find_models(resource)
    pnnx = find_pnnx()

    expected: set[Path] = set()
    with tempfile.TemporaryDirectory(prefix="maa_ocr_ncnn_") as tmp:
        # 同一份 onnx 只转换一次（多个目录放同一模型时复用转换结果）
        converted: dict[str, tuple[Path, Path]] = {}
        staged: list[tuple[tuple[Path, Path], list[Path]]] = []
        for onnx, kind in models:
            rel = onnx.parent.relative_to(resource).as_posix()
            key = f"{kind}-{sha256(onnx)}"
            print(f"  [....] {rel}", end="", flush=True)
            if key in converted:
                tag = "REUSE"
            else:
                workdir = Path(tmp) / key
                workdir.mkdir()
                converted[key], dynamic = run_pnnx(pnnx, onnx, kind, workdir)
                tag = "CONVERT" if dynamic or kind not in INPUTSHAPE2 else "STATIC"
            print(f"\r  [{tag}] {rel}")
            targets = ncnn_targets(onnx, kind, resource, output)
            staged.append((converted[key], targets))
            expected.update(targets)

        # 全部转换成功后再落盘，避免中途失败留下新旧混杂的产物
        for sources, targets in staged:
            for src, dst in zip(sources, targets, strict=True):
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(src, dst)

    orphans = sorted(existing_ncnn(output) - expected)
    for p in orphans:
        p.unlink()
        remove_empty_dirs(p.parent, output)
        print(f"  [REMOVE] {p.relative_to(output).as_posix()}")

    print(
        f"done: {len(models)} models, {len(converted)} converted, "
        f"{len(orphans)} removed"
    )


def check(resource: Path, output: Path) -> None:
    expected: set[Path] = set()
    missing: list[Path] = []
    for onnx, kind in find_models(resource):
        for target in ncnn_targets(onnx, kind, resource, output):
            expected.add(target)
            if not target.is_file() or target.stat().st_size == 0:
                missing.append(target)
    orphans = sorted(existing_ncnn(output) - expected)

    for p in missing:
        print(f"  [MISSING] {p.relative_to(output).as_posix()}")
    for p in orphans:
        print(f"  [ORPHAN] {p.relative_to(output).as_posix()}")
    if missing or orphans:
        raise SystemExit(
            "Android ncnn OCR models are out of sync with resource/, run: "
            "python tools/OcrNcnnConverter/convert_ocr_ncnn.py"
        )
    print(f"ok: {len(expected) // len(NCNN_SUFFIXES)} models")


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Convert PP-OCR onnx under resource/ to Android ncnn models"
    )
    ap.add_argument(
        "--resource", type=Path, default=ROOT / "resource", help="source resource dir"
    )
    ap.add_argument(
        "--output",
        type=Path,
        default=ROOT / "resource_android",
        help="Android overlay dir",
    )
    ap.add_argument(
        "--check",
        action="store_true",
        help="only check that every onnx has its ncnn models",
    )
    args = ap.parse_args()

    resource = args.resource.resolve()
    output = args.output.resolve()
    if not resource.is_dir():
        raise SystemExit(f"resource dir not found: {resource}")

    if args.check:
        check(resource, output)
    else:
        convert(resource, output)


if __name__ == "__main__":
    main()
