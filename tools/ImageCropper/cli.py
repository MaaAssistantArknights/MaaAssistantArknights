import argparse
import json
from pathlib import Path
from typing import Optional

import cv2
import numpy as np

import colormatcher

# 标准化目标短边，与 main.py 的 set_screenshot_target_short_side(720) 一致；
# 后续所有坐标均基于该标准化图，与 GUI 框选出的 roi 同一坐标系
TARGET_SHORT_SIDE = 720

# ROI 扩边像素，与 main.py 的 amplify 一致
AMPLIFY_PIXELS = 50


def imread_unicode(path: str) -> Optional[np.ndarray]:
    """cv2 的 imread/imwrite 在 Windows 不支持非 ASCII 路径且静默失败，改走编码字节流。"""
    try:
        data = np.fromfile(path, dtype=np.uint8)
    except OSError:
        return None
    if data.size == 0:
        # 空文件（0 字节）时 imdecode 会裸抛断言而非返回 None
        return None
    return cv2.imdecode(data, cv2.IMREAD_COLOR)


def imwrite_unicode(path: str, image: np.ndarray) -> None:
    try:
        ok, buf = cv2.imencode(Path(path).suffix or ".png", image)
    except cv2.error:
        # 未知后缀在部分 OpenCV 版本直接抛异常而非返回 False
        raise SystemExit(f"Failed to encode image: {path}") from None
    if not ok:
        raise SystemExit(f"Failed to encode image: {path}")
    buf.tofile(path)


def load_std_image(path: str, raw: bool = False) -> np.ndarray:
    """读图并等比缩放到目标短边；raw 时保持原分辨率（用于局部区域图的二次操作）。"""
    image = imread_unicode(path)
    if image is None:
        raise SystemExit(f"Failed to read image: {path}")
    if raw:
        return image
    height, width = image.shape[:2]
    short_side = min(width, height)
    if short_side == TARGET_SHORT_SIDE:
        return image
    scale = TARGET_SHORT_SIDE / short_side
    if scale > 1:
        # 小图（裁剪产物等）会被放大导致坐标系变化，标定等二次操作必须 --raw
        print(
            f"Note: input short side {short_side} < {TARGET_SHORT_SIDE}, "
            "upscaled and coordinates changed; use --raw to keep original resolution"
        )
    size = (int(round(width * scale)), int(round(height * scale)))
    return cv2.resize(image, size, interpolation=cv2.INTER_AREA)


def parse_roi(text: str, box: bool) -> list[int]:
    """解析坐标。默认 x,y,w,h（MAA roi 格式）；--box 时为 x1,y1,x2,y2 两点式（grid 读数），换算为 x,y,w,h。"""
    try:
        values = [int(v) for v in text.replace("，", ",").split(",")]
    except ValueError:
        raise SystemExit(f"Invalid roi: {text}") from None
    if len(values) != 4:
        raise SystemExit(f"Invalid roi: {text}")
    if box:
        x1, y1, x2, y2 = values
        values = [min(x1, x2), min(y1, y2), abs(x2 - x1), abs(y2 - y1)]
    if any(v < 0 for v in values):
        raise SystemExit(f"Roi components must be non-negative: {text}")
    if values[2] <= 0 or values[3] <= 0:
        raise SystemExit(f"Roi width/height must be positive: {text}")
    return values


def clamp_amplify(roi: list[int], width: int, height: int) -> list[int]:
    """ROI 向四周扩 AMPLIFY_PIXELS 像素并裁回图内。"""
    x, y, w, h = roi
    ax = max(0, x - AMPLIFY_PIXELS)
    ay = max(0, y - AMPLIFY_PIXELS)
    aw = min(width, x + w + AMPLIFY_PIXELS) - ax
    ah = min(height, y + h + AMPLIFY_PIXELS) - ay
    return [ax, ay, aw, ah]


def draw_grid(
    image: np.ndarray, cell: int, zoom: int = 1, origin: tuple[int, int] = (0, 0)
) -> np.ndarray:
    """在（可能已放大的）图上叠加网格，图像居中、四周扩出边距标注刻度。

    所有线只画在图内（黄色细分格线；基准线红/蓝交替，读数错位一根线时
    颜色立即暴露），数字放边距区且起始位置对齐基准线位置（x 刻度左缘=
    线的 x，y 刻度上缘=线的 y），读到的数字就是缩放前坐标系（origin 为
    当前图左上角在该坐标系中的位置）中的值，无需换算。
    """
    height, width = image.shape[:2]
    step = cell * zoom
    ox, oy = origin
    line_color = (0, 230, 255)  # BGR 亮黄，细分格线
    font = cv2.FONT_HERSHEY_SIMPLEX
    # 渲染规格与 zoom/step 全脱钩：字与线按固定像素绘制（1px 笔画在任何缩放下都清晰），
    # 仅当输出过宽、读图端会降采样展示时按输出宽度整体加粗（封顶 3px）保证降采样后存活
    font_scale = 0.5
    pad = 4
    survive = max(1, min(3, width // 1200))
    line_thickness = survive
    text_thickness = survive
    # 相邻基准线红/蓝交替，读数错位一根线时颜色立即暴露
    base_palette = [(0, 0, 230), (230, 120, 0)]

    # 标注密度自适应：刻度间隔不足约 60 图像素时按 2 的幂稀疏化，避免数字重叠
    label_every = 1
    while step * label_every < 60:
        label_every *= 2

    # 边距按 x/y 两方向最大标签的宽度取大者，竖图的 y 标签才不会被截字
    sample_x = str(ox + (width // step) * cell)
    sample_y = str(oy + (height // step) * cell)
    (text_w, text_h), _ = cv2.getTextSize(sample_x, font, font_scale, text_thickness)
    (text_w_y, _), _ = cv2.getTextSize(sample_y, font, font_scale, text_thickness)
    margin_x = max(text_w, text_w_y) + 2 * pad
    margin_y = text_h + 2 * pad

    canvas = np.full((height + 2 * margin_y, width + 2 * margin_x, 3), 255, np.uint8)
    canvas[margin_y : margin_y + height, margin_x : margin_x + width] = image

    for i, x in enumerate(range(0, width + 1, step)):
        X = margin_x + x
        labeled = i % label_every == 0
        base = base_palette[(i // label_every) % 2]
        color = base if labeled else line_color
        thickness = text_thickness if labeled else line_thickness
        cv2.line(canvas, (X, margin_y), (X, margin_y + height), color, thickness)
        if labeled:
            label = str(ox + i * cell)
            # putText 的 org 是文字左下角：左缘对齐线的 x，位于上/下边距内
            cv2.putText(
                canvas,
                label,
                (X, margin_y - pad),
                font,
                font_scale,
                base,
                text_thickness,
                cv2.LINE_AA,
            )
            cv2.putText(
                canvas,
                label,
                (X, margin_y + height + text_h + pad),
                font,
                font_scale,
                base,
                text_thickness,
                cv2.LINE_AA,
            )
    for i, y in enumerate(range(0, height + 1, step)):
        Y = margin_y + y
        labeled = i % label_every == 0
        base = base_palette[(i // label_every) % 2]
        color = base if labeled else line_color
        thickness = text_thickness if labeled else line_thickness
        cv2.line(canvas, (margin_x, Y), (margin_x + width, Y), color, thickness)
        if labeled:
            label = str(oy + i * cell)
            (lw, _), _ = cv2.getTextSize(label, font, font_scale, text_thickness)
            # org 是文字左下角：上缘对齐线的 y（org_y=Y+text_h），位于左/右边距内
            cv2.putText(
                canvas,
                label,
                (margin_x - lw - pad, Y + text_h),
                font,
                font_scale,
                base,
                text_thickness,
                cv2.LINE_AA,
            )
            cv2.putText(
                canvas,
                label,
                (margin_x + width + pad, Y + text_h),
                font,
                font_scale,
                base,
                text_thickness,
                cv2.LINE_AA,
            )
    return canvas


def build_color_match(roi_image: np.ndarray, roi: list[int], connected: bool) -> dict:
    """对 roi 聚类主色并生成 ColorMatch 参数草稿，覆盖全部聚类主色。

    注意 main.py GUI 的 C/c 键仅输出首个聚类且向量被展开为单层，本函数输出
    全部聚类、每条为完整的三通道 [lower, upper] 向量，与 MAA ColorMatch 协议对齐。
    """
    method = cv2.COLOR_BGR2RGB
    cluster = colormatcher.kmeansClusterColors(roi_image, method)
    ret = {
        "recognition": "ColorMatch",
        "roi": roi,
        "method": method,
        "lower": [],
        "upper": [],
        "count": [],
        "connected": connected,
    }
    for _, lower, upper in colormatcher.RGBDistance(cluster):
        ret["lower"].append([int(v) for v in lower])
        ret["upper"].append([int(v) for v in upper])
        ret["count"].append(
            colormatcher.getCount(roi_image, lower, upper, connected, method)
        )
    return ret


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Headless companion of main.py for agents: all coordinates are on the "
        "standardized (short side 720) image."
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p_grid = sub.add_parser(
        "grid", help="standardize image and overlay grid + edge scales for locating"
    )
    p_grid.add_argument("image")
    p_grid.add_argument("-o", "--out", help="output path, default dst/<stem>_grid.png")
    p_grid.add_argument(
        "--cell",
        type=int,
        default=40,
        help="grid cell size in pre-zoom coordinates, default 40",
    )
    p_grid.add_argument(
        "--region",
        help="x,y,w,h: crop this region (in standardized/raw coordinates) before gridding; scales are labeled in the same coordinates",
    )
    p_grid.add_argument(
        "--zoom",
        type=int,
        default=1,
        help="nearest-neighbor magnification for fine locating, default 1",
    )
    p_grid.add_argument(
        "--raw", action="store_true", help="skip standardization, keep input resolution"
    )

    p_crop = sub.add_parser("crop", help="crop roi (and amplified roi with --amp)")
    p_crop.add_argument("image")
    p_crop.add_argument("roi", help="x,y,w,h (default) or x1,y1,x2,y2 with --box")
    p_crop.add_argument("--box", action="store_true", help="roi is x1,y1,x2,y2")
    p_crop.add_argument(
        "--amp", action="store_true", help="also save +-50px amplified crop"
    )
    p_crop.add_argument("-o", "--out", help="output dir, default dst/")
    p_crop.add_argument(
        "--raw", action="store_true", help="skip standardization, keep input resolution"
    )

    p_color = sub.add_parser("color", help="print ColorMatch parameter draft as JSON")
    p_color.add_argument("image")
    p_color.add_argument("roi", help="x,y,w,h (default) or x1,y1,x2,y2 with --box")
    p_color.add_argument("--box", action="store_true", help="roi is x1,y1,x2,y2")
    p_color.add_argument(
        "--connected",
        action="store_true",
        help="ColorMatch connected: hits must form a connected region",
    )
    p_color.add_argument(
        "--raw", action="store_true", help="skip standardization, keep input resolution"
    )

    args = parser.parse_args()
    if args.command == "grid" and (args.cell < 1 or args.zoom < 1):
        # step=0 会让刻度稀疏化死循环，负值产生空网格
        raise SystemExit("--cell and --zoom must be >= 1")
    dst_dir = Path(__file__).parent / "dst"

    image = load_std_image(args.image, args.raw)
    height, width = image.shape[:2]

    if args.command == "grid":
        origin = (0, 0)
        if args.region:
            rx, ry, rw, rh = parse_roi(args.region, box=False)
            if rx < 0 or ry < 0 or rx + rw > width or ry + rh > height:
                raise SystemExit(
                    f"Region {rx},{ry},{rw},{rh} out of image bounds ({width}x{height}); "
                    "region coordinates follow the same space as --raw selects"
                )
            image = image[ry : ry + rh, rx : rx + rw]
            origin = (rx, ry)
        if args.zoom > 1:
            image = cv2.resize(
                image,
                (image.shape[1] * args.zoom, image.shape[0] * args.zoom),
                interpolation=cv2.INTER_NEAREST,
            )
        out = (
            Path(args.out)
            if args.out
            else dst_dir / f"{Path(args.image).stem}_grid.png"
        )
        out.parent.mkdir(parents=True, exist_ok=True)
        imwrite_unicode(str(out), draw_grid(image, args.cell, args.zoom, origin))
        print(
            f"grid: {out.resolve()} ({image.shape[1]}x{image.shape[0]}, cell={args.cell}, zoom={args.zoom}, origin={origin})"
        )
        return

    roi = parse_roi(args.roi, args.box)
    x, y, w, h = roi
    if x < 0 or y < 0 or x + w > width or y + h > height:
        # NumPy 越界切片会静默截断，产物文件名却记录完整 roi，必须显式拒绝
        raise SystemExit(f"Roi {roi} out of image bounds ({width}x{height})")
    roi_image = image[y : y + h, x : x + w]

    if args.command == "crop":
        if args.out:
            dst_dir = Path(args.out)
        dst_dir.mkdir(parents=True, exist_ok=True)
        stem = Path(args.image).stem
        name = f"{stem}_{x}_{y}_{w}_{h}.png"
        imwrite_unicode(str(dst_dir / name), roi_image)
        print(f"cropped: {(dst_dir / name).resolve()}")
        if args.amp:
            ax, ay, aw, ah = clamp_amplify(roi, width, height)
            name = f"{stem}_{x}_{y}_{w}_{h}__{ax}_{ay}_{aw}_{ah}.png"
            imwrite_unicode(str(dst_dir / name), image[ay : ay + ah, ax : ax + aw])
            print(f"amplified: {(dst_dir / name).resolve()}")
        return

    if args.command == "color":
        if w * h < 3:
            # kmeans 聚类数 K=3，少于 3 个像素无法聚类
            raise SystemExit("Roi too small for color clustering (need >= 3 pixels)")
        print(json.dumps(build_color_match(roi_image, roi, args.connected)))
        return


if __name__ == "__main__":
    main()
