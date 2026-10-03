import argparse
import json
from pathlib import Path

import cv2
import numpy as np

import colormatcher

# 标准化目标短边，与 main.py 的 set_screenshot_target_short_side(720) 一致；
# 后续所有坐标均基于该标准化图，与 GUI 框选出的 roi 同一坐标系
TARGET_SHORT_SIDE = 720

# grid 渲染输出上限（720p）：看图端会把更大输出降采样到约 810p，编号直接糊掉，
# 输出必须在预算内自适应而不是产出大图
DISPLAY_MAX_W = 1280
DISPLAY_MAX_H = 720

# 刻度字号候选（从大到小）：显示步长放不下线号时逐级降字号，
# 0.3 档仅为超大格子数的极端 region 兜底，正常参数组合用不到
FONT_SCALES = (0.7, 0.6, 0.5, 0.45, 0.4, 0.35, 0.3)

# ROI 扩边像素，与 main.py 的 amplify 一致
AMPLIFY_PIXELS = 50


def imread_unicode(path: str) -> np.ndarray | None:
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
    size = (round(width * scale), round(height * scale))
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


def resolve_cell(base: int, x: int | None, y: int | None) -> tuple[int, int]:
    """两轴格距生效值：--cell 为两轴同值简写，--cell-x/--cell-y 显式覆盖对应轴。"""
    return (x if x is not None else base, y if y is not None else base)


def parse_cell_roi(
    text: str, cell_x: int, cell_y: int, origin: tuple[int, int]
) -> list[int]:
    """解析线区间 c1,r1,c2,r2 → 像素 x,y,w,h。

    线区间语义：左/上闭、右/下开，目标夹在线 c1 与 c2、线 r1 与 r2 之间，
    区间两端即目标边界，故 x=origin+c1*cell_x、w=(c2-c1)*cell_x（y 轴同理
    用 cell_y）。定位读数全程以线号为第一公民（图上边距印的数字即线号），
    像素换算收敛在本函数，不由模型心算。
    """
    try:
        c1, r1, c2, r2 = [int(v) for v in text.replace("，", ",").split(",")]
    except ValueError:
        raise SystemExit(f"Invalid line interval: {text}") from None
    if min(c1, r1, c2, r2) < 0 or c2 <= c1 or r2 <= r1:
        # 区间端点即边界，c2==c1 意味着零宽，静默产出空图比报错更糟
        raise SystemExit(f"Invalid line interval: {text} (need c2>c1 and r2>r1)")
    ox, oy = origin
    return [
        ox + c1 * cell_x,
        oy + r1 * cell_y,
        (c2 - c1) * cell_x,
        (r2 - r1) * cell_y,
    ]


def parse_cell_origin(text: str) -> tuple[int, int]:
    """解析线坐标系原点 ox,oy（线 0，即图左上角基准线的像素位置）。"""
    try:
        values = [int(v) for v in text.replace("，", ",").split(",")]
    except ValueError:
        raise SystemExit(f"Invalid --cell-origin: {text}") from None
    if len(values) != 2 or any(v < 0 for v in values):
        raise SystemExit(f"Invalid --cell-origin: {text} (use ox,oy)")
    return (values[0], values[1])


def clamp_amplify(roi: list[int], width: int, height: int) -> list[int]:
    """ROI 向四周扩 AMPLIFY_PIXELS 像素并裁回图内。"""
    x, y, w, h = roi
    ax = max(0, x - AMPLIFY_PIXELS)
    ay = max(0, y - AMPLIFY_PIXELS)
    aw = min(width, x + w + AMPLIFY_PIXELS) - ax
    ah = min(height, y + h + AMPLIFY_PIXELS) - ay
    return [ax, ay, aw, ah]


def draw_grid(image: np.ndarray, cell_x: int, cell_y: int, zoom: int = 1) -> np.ndarray:
    """在（可能已放大的）图上叠加网格，图像居中、四周扩出边距标注线号。

    所有线只画在图内（黄色细分格线；基准线红/蓝交替，数线串行时颜色立即
    暴露），边距区标注每条线的 0-based 线号（N 格有 N+1 条线）且起始位置
    对齐基准线位置；两轴格距独立，线 c 的像素坐标 = origin + c*cell_x、
    线 r = origin + r*cell_y（cell/origin 由命令行打印）。
    """
    height, width = image.shape[:2]
    step_x = cell_x * zoom
    step_y = cell_y * zoom
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

    # 标注密度自适应：刻度间隔不足约 60 图像素时按 2 的幂稀疏化，避免数字重叠，
    # 两轴步长独立、各自稀疏化
    label_every_x = 1
    while step_x * label_every_x < 60:
        label_every_x *= 2
    label_every_y = 1
    while step_y * label_every_y < 60:
        label_every_y *= 2

    # 边距按 x/y 两方向最大标签的宽度取大者，竖图的 y 标签才不会被截字
    sample_x = str(width // step_x)
    sample_y = str(height // step_y)
    (text_w, text_h), _ = cv2.getTextSize(sample_x, font, font_scale, text_thickness)
    (text_w_y, _), _ = cv2.getTextSize(sample_y, font, font_scale, text_thickness)
    margin_x = max(text_w, text_w_y) + 2 * pad
    margin_y = text_h + 2 * pad

    canvas = np.full((height + 2 * margin_y, width + 2 * margin_x, 3), 255, np.uint8)
    canvas[margin_y : margin_y + height, margin_x : margin_x + width] = image

    for i, x in enumerate(range(0, width + 1, step_x)):
        X = margin_x + x
        labeled = i % label_every_x == 0
        base = base_palette[(i // label_every_x) % 2]
        color = base if labeled else line_color
        thickness = text_thickness if labeled else line_thickness
        cv2.line(canvas, (X, margin_y), (X, margin_y + height), color, thickness)
        if labeled:
            # 标竖线线号（0-based），像素换算靠命令行打印的 cell/origin
            label = str(i)
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
    for i, y in enumerate(range(0, height + 1, step_y)):
        Y = margin_y + y
        labeled = i % label_every_y == 0
        base = base_palette[(i // label_every_y) % 2]
        color = base if labeled else line_color
        thickness = text_thickness if labeled else line_thickness
        cv2.line(canvas, (margin_x, Y), (margin_x + width, Y), color, thickness)
        if labeled:
            # 标横线线号（0-based），像素换算靠命令行打印的 cell/origin
            label = str(i)
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


def choose_display_step(
    width: int, height: int, cell_x: int, cell_y: int, zoom: int
) -> tuple[float, int, int, float, int, int]:
    """在 720p 输出预算内搜索全局等比显示倍率 factor 与字号。

    定位本质是 ｢判断目标边缘夹在哪两根线之间｣（读数只认线号），边距标注
    0-based 线号。factor 是唯一自由度，两轴显示步长为派生值
    step_x=round(cell_x*factor)、step_y=round(cell_y*factor)：两轴同倍率
    缩放，图像内容保持源比例不变形（两轴各自步长会把窄带窗的图标拉成
    条带）。约束：细轴（cell 小者）序号放得下（序号宽+pad ≤ 细轴 step；
    粗轴 step ≥ 细轴 step，序号自动放得下）、输出画布 ≤1280×720（两轴
    各自的格数×step+边距分别计）。画布尺寸关于 factor 单调非降（格数与
    边距不随 factor 变），故每档字号取最小可行 factor=max(序号需求,
    --zoom 下限) 即完成搜索且有解性判定：其画布超预算时先回落序号需求，
    仍超则更大 factor 只会更超，降字号重试；全档无解必须报错指引缩小
    region 而不是产出会被看图端降采样糊掉的大图（定位工作流分层：全图
    粗格判断目标在哪一带，--region 截出的小图细格判断边界）。
    返回 (factor, step_x, step_y, font_scale, label_w, text_h)。
    """
    n_cols = -(-width // cell_x)
    n_rows = -(-height // cell_y)
    # 序号位数由格数决定，与 factor 无关，宽度只需每个字号量一次
    label_x = str(n_cols - 1)
    label_y = str(n_rows - 1)
    pad, gutter = 4, 1
    font = cv2.FONT_HERSHEY_SIMPLEX
    fine_cell = min(cell_x, cell_y)

    for fs in FONT_SCALES:
        (lwx, th), _ = cv2.getTextSize(label_x, font, fs, 2)
        (lwy, _), _ = cv2.getTextSize(label_y, font, fs, 2)
        lw = max(lwx, lwy)
        margin_x = lw + 2 * pad
        margin_y = th + 2 * pad
        budget_w = DISPLAY_MAX_W - 2 * margin_x - (n_cols - 1) * gutter
        budget_h = DISPLAY_MAX_H - 2 * margin_y - (n_rows - 1) * gutter
        # 细轴序号放得下所需的最小 factor（step=序号宽+pad 时相邻序号间隙
        # 恰为一个 pad，可读下限）
        min_factor = (lw + pad) / fine_cell
        factor = max(min_factor, zoom)
        step_x = round(cell_x * factor)
        step_y = round(cell_y * factor)
        over = n_cols * step_x > budget_w or n_rows * step_y > budget_h
        if over and factor > min_factor:
            # --zoom 下限装不下预算时回落到序号需求的最小可行 factor
            factor = min_factor
            step_x = round(cell_x * factor)
            step_y = round(cell_y * factor)
            over = n_cols * step_x > budget_w or n_rows * step_y > budget_h
        if over:
            continue
        return factor, step_x, step_y, fs, lw, th
    raise SystemExit(
        f"Cannot fit line numbers in {DISPLAY_MAX_W}x{DISPLAY_MAX_H} output for "
        f"region {width}x{height} cell={cell_x}x{cell_y}: shrink --region or "
        "increase --cell (locate on the coarse full grid first, then fine-tune "
        "the boundary on a small --region crop)"
    )


def draw_grid_cut(
    image: np.ndarray,
    step_x: int,
    step_y: int,
    n_cols: int,
    n_rows: int,
    line_color: tuple[int, int, int] | None,
    font_scale: float,
    label_w: int,
    text_h: int,
) -> np.ndarray:
    """切分渲染：图像按格切块排入白底表格，格间以 1px 分隔线相隔（不覆盖内容像素）。

    两轴显示步长与字号由 choose_display_step 在 720p 预算内定好（每个线号都
    保证放得下），两轴步长同源于全局 factor、图像整体等比缩放不变形。读数
    是 ｢目标边缘夹在哪两根线之间｣：边距标注 0-based 线号，红/蓝逐线交替的
    切缝用于数线防串行（竖线按列号、横线按行号各自交替，line_color 可指定
    单一颜色适配特殊底色）。线 c 的像素坐标 = origin + c*cell_x、线 r =
    origin + r*cell_y（cell/origin 由调用方打印），线区间 [c1,c2) 的两端
    即目标边界。
    """
    font = cv2.FONT_HERSHEY_SIMPLEX
    text_thickness = 2 if font_scale >= 0.6 else 1
    # 切缝仅 1px，整缝填分隔线（不触碰内容像素）
    gutter = 1
    pad = 4
    default_palette = [(0, 0, 230), (230, 120, 0)]
    margin_x = label_w + 2 * pad
    margin_y = text_h + 2 * pad

    table_w = n_cols * step_x + (n_cols - 1) * gutter
    table_h = n_rows * step_y + (n_rows - 1) * gutter
    canvas = np.full((table_h + 2 * margin_y, table_w + 2 * margin_x, 3), 255, np.uint8)

    for j in range(n_rows):
        for i in range(n_cols):
            tile = image[j * step_y : (j + 1) * step_y, i * step_x : (i + 1) * step_x]
            x = margin_x + i * (step_x + gutter)
            y = margin_y + j * (step_y + gutter)
            canvas[y : y + tile.shape[0], x : x + tile.shape[1]] = tile

    # 分隔线与编号同色：逐线红/蓝交替，line_color 可指定单一颜色
    def line_color_of(i: int):
        return line_color or default_palette[i % 2]

    def x_edge(i: int) -> int:
        # 第 i 条竖线（线号 i；i=n_cols 为最右一条）
        return margin_x + i * step_x + min(i, n_cols - 1) * gutter

    def y_edge(j: int) -> int:
        return margin_y + j * step_y + min(j, n_rows - 1) * gutter

    for i in range(1, n_cols):
        color = line_color_of(i)
        x0 = x_edge(i) - gutter
        cv2.rectangle(
            canvas, (x0, margin_y), (x0 + gutter, margin_y + table_h), color, -1
        )
    for j in range(1, n_rows):
        color = line_color_of(j)
        y0 = y_edge(j) - gutter
        cv2.rectangle(
            canvas, (margin_x, y0), (margin_x + table_w, y0 + gutter), color, -1
        )

    # 逐线标注：x 刻度=竖线号，y 刻度=横线号，颜色=线色
    for i in range(n_cols + 1):
        label = str(i)
        color = line_color_of(i)
        x = x_edge(i)
        cv2.putText(
            canvas,
            label,
            (x, margin_y - pad),
            font,
            font_scale,
            color,
            text_thickness,
            cv2.LINE_AA,
        )
        cv2.putText(
            canvas,
            label,
            (x, margin_y + table_h + text_h + pad),
            font,
            font_scale,
            color,
            text_thickness,
            cv2.LINE_AA,
        )
    for j in range(n_rows + 1):
        label = str(j)
        color = line_color_of(j)
        (lw, _), _ = cv2.getTextSize(label, font, font_scale, text_thickness)
        y = y_edge(j)
        cv2.putText(
            canvas,
            label,
            (margin_x - lw - pad, y + text_h),
            font,
            font_scale,
            color,
            text_thickness,
            cv2.LINE_AA,
        )
        cv2.putText(
            canvas,
            label,
            (margin_x + table_w + pad, y + text_h),
            font,
            font_scale,
            color,
            text_thickness,
            cv2.LINE_AA,
        )
    return canvas


def fit_overlay(image: np.ndarray, cell_x: int, cell_y: int, zoom: int) -> np.ndarray:
    """overlay 渲染的 720p 适配：输出=图+四周边距，超预算时按比例缩小图像。

    线密度（cell*zoom，两轴独立）不因此改变，序号密度由 draw_grid 内部的
    label_every 稀疏化兜底。
    """
    height, width = image.shape[:2]
    step_x = cell_x * zoom
    step_y = cell_y * zoom
    # 边距按 0.5 字号与最大序号位数保守估算（draw_grid 内部实际取值不会更大）
    label_x = str(width // step_x)
    label_y = str(height // step_y)
    font = cv2.FONT_HERSHEY_SIMPLEX
    (lwx, th), _ = cv2.getTextSize(label_x, font, 0.5, 1)
    (lwy, _), _ = cv2.getTextSize(label_y, font, 0.5, 1)
    margin_x = max(lwx, lwy) + 8
    margin_y = th + 8
    factor = min(
        1.0,
        (DISPLAY_MAX_W - 2 * margin_x) / width,
        (DISPLAY_MAX_H - 2 * margin_y) / height,
    )
    if factor < 1.0:
        image = cv2.resize(
            image,
            (max(1, round(width * factor)), max(1, round(height * factor))),
            interpolation=cv2.INTER_AREA,
        )
    return image


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
        default=64,
        help="grid cell size in pre-zoom coordinates, shorthand for both axes, default 64",
    )
    p_grid.add_argument(
        "--cell-x",
        type=int,
        default=None,
        help="override x-axis cell (column width), default: --cell",
    )
    p_grid.add_argument(
        "--cell-y",
        type=int,
        default=None,
        help="override y-axis cell (row height), default: --cell",
    )
    p_grid.add_argument(
        "--region",
        help="x,y,w,h: crop this region (in standardized/raw coordinates) before gridding; scales are labeled in the same coordinates",
    )
    p_grid.add_argument(
        "--region-lines",
        help="line-interval c1,r1,c2,r2: left/top inclusive, right/bottom exclusive, "
        "line numbers as printed on the grid image (needs --region-cell* and --cell-origin)",
    )
    p_grid.add_argument(
        "--region-cell",
        type=int,
        default=None,
        help="cell size of the --region-lines coordinate system (i.e. the previous layer's "
        "cell), shorthand for both axes, defaults to --cell",
    )
    p_grid.add_argument(
        "--region-cell-x",
        type=int,
        default=None,
        help="override x-axis cell of the --region-lines coordinate system, default: --region-cell",
    )
    p_grid.add_argument(
        "--region-cell-y",
        type=int,
        default=None,
        help="override y-axis cell of the --region-lines coordinate system, default: --region-cell",
    )
    p_grid.add_argument(
        "--cell-origin",
        default="0,0",
        help="pixel coords of line 0 (the previous layer's stdout origin) as ox,oy for --region-lines, default 0,0",
    )
    p_grid.add_argument(
        "--zoom",
        type=int,
        default=1,
        help="lower bound of the global display factor (display step = cell*factor per "
        "axis); lowered automatically when the 720p output budget cannot fit it, default 1",
    )
    p_grid.add_argument(
        "--raw", action="store_true", help="skip standardization, keep input resolution"
    )
    p_grid.add_argument(
        "--overlay",
        action="store_true",
        help="draw grid lines over the image instead of the default cut-table rendering",
    )
    p_grid.add_argument(
        "--line-color",
        help="separator line color as R,G,B or #RRGGBB; default alternating red/blue",
    )

    p_crop = sub.add_parser("crop", help="crop roi (and amplified roi with --amp)")
    p_crop.add_argument("image")
    p_crop.add_argument(
        "roi",
        help="x,y,w,h (default) or x1,y1,x2,y2 with --box or c1,r1,c2,r2 line interval with --in-lines",
    )
    p_crop.add_argument("--box", action="store_true", help="roi is x1,y1,x2,y2")
    p_crop.add_argument(
        "--in-lines",
        action="store_true",
        help="roi is a line-interval c1,r1,c2,r2: left/top inclusive, right/bottom exclusive, "
        "line numbers as printed on the grid image (needs --cell and --cell-origin)",
    )
    p_crop.add_argument(
        "--cell",
        type=int,
        default=64,
        help="cell size for --in-lines, shorthand for both axes, default 64",
    )
    p_crop.add_argument(
        "--cell-x",
        type=int,
        default=None,
        help="override x-axis cell for --in-lines, default: --cell",
    )
    p_crop.add_argument(
        "--cell-y",
        type=int,
        default=None,
        help="override y-axis cell for --in-lines, default: --cell",
    )
    p_crop.add_argument(
        "--cell-origin",
        default="0,0",
        help="pixel coords of line 0 as ox,oy for --in-lines, default 0,0",
    )
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
    if args.command == "grid":
        # 生效格距：--cell 系管本层渲染、--region-cell 系管开窗换算，两轴各自独立
        cell_x, cell_y = resolve_cell(args.cell, args.cell_x, args.cell_y)
        region_cell_x, region_cell_y = resolve_cell(
            args.region_cell if args.region_cell is not None else args.cell,
            args.region_cell_x,
            args.region_cell_y,
        )
        if (
            min(cell_x, cell_y) < 1
            or min(region_cell_x, region_cell_y) < 1
            or args.zoom < 1
        ):
            # step=0 会让刻度稀疏化死循环，负值产生空网格
            raise SystemExit("--cell*/--region-cell* and --zoom must be >= 1")
    line_color = None
    if args.command == "grid" and args.line_color:
        s = args.line_color.strip().lstrip("#")
        try:
            parts = (
                [int(v) for v in s.replace("，", ",").split(",")]
                if "," in s
                else [int(s[k : k + 2], 16) for k in (0, 2, 4)]
            )
        except ValueError:
            raise SystemExit(
                f"Invalid --line-color: {args.line_color} (use R,G,B or #RRGGBB)"
            ) from None
        if len(parts) != 3 or any(p < 0 or p > 255 for p in parts):
            raise SystemExit(
                f"Invalid --line-color: {args.line_color} (use R,G,B or #RRGGBB)"
            )
        line_color = (parts[2], parts[1], parts[0])  # RGB 输入转 BGR 存储
    dst_dir = Path(__file__).parent / "dst"

    image = load_std_image(args.image, args.raw)
    height, width = image.shape[:2]

    if args.command == "grid":
        origin = (0, 0)
        if args.region_lines:
            # 换算格距取上一层（--region-cell 系，x/y 分量各乘各的生效值），
            # 渲染格距取本层（--cell 系），两层解耦
            rx, ry, rw, rh = parse_cell_roi(
                args.region_lines,
                region_cell_x,
                region_cell_y,
                parse_cell_origin(args.cell_origin),
            )
        if args.region_lines or args.region:
            if not args.region_lines:
                rx, ry, rw, rh = parse_roi(args.region, box=False)
            if rx < 0 or ry < 0 or rx + rw > width or ry + rh > height:
                raise SystemExit(
                    f"Region {rx},{ry},{rw},{rh} out of image bounds ({width}x{height}); "
                    "region coordinates follow the same space as --raw selects"
                )
            image = image[ry : ry + rh, rx : rx + rw]
            origin = (rx, ry)
        height, width = image.shape[:2]
        out = (
            Path(args.out)
            if args.out
            else dst_dir / f"{Path(args.image).stem}_grid.png"
        )
        out.parent.mkdir(parents=True, exist_ok=True)
        # stdout 打印全部生效格距（本层两轴 + 开窗换算两轴）与换算式，供下一层照抄
        cell_info = f"cell={cell_x}x{cell_y}"
        if args.region_lines:
            cell_info += f", region-cell={region_cell_x}x{region_cell_y}"
        if args.overlay:
            # 序号密度由 label_every 稀疏化兜底，仅需把输出压进 720p 预算
            image = fit_overlay(image, cell_x, cell_y, args.zoom)
            imwrite_unicode(str(out), draw_grid(image, cell_x, cell_y, args.zoom))
            out_h, out_w = image.shape[:2]
            print(
                f"grid: {out.resolve()} ({out_w}x{out_h}, {cell_info}, "
                f"zoom={args.zoom}, origin={origin}); line (c,r) -> pixel "
                f"({origin[0]}+c*{cell_x}, {origin[1]}+r*{cell_y})"
            )
        else:
            factor, step_x, step_y, fs, lw, th = choose_display_step(
                width, height, cell_x, cell_y, args.zoom
            )
            n_cols = -(-width // cell_x)
            n_rows = -(-height // cell_y)
            if factor != 1.0:
                # 全局等比缩放：放大用最近邻保格子边界锐利，缩小用面积平均
                image = cv2.resize(
                    image,
                    (
                        max(1, round(width * factor)),
                        max(1, round(height * factor)),
                    ),
                    interpolation=cv2.INTER_NEAREST if factor > 1 else cv2.INTER_AREA,
                )
            canvas = draw_grid_cut(
                image, step_x, step_y, n_cols, n_rows, line_color, fs, lw, th
            )
            imwrite_unicode(str(out), canvas)
            print(
                f"grid: {out.resolve()} ({canvas.shape[1]}x{canvas.shape[0]}, "
                f"{cell_info}, factor={factor:g}, step={step_x}x{step_y}, "
                f"origin={origin}); line (c,r) -> pixel "
                f"({origin[0]}+c*{cell_x}, {origin[1]}+r*{cell_y})"
            )
        return

    if args.command == "crop" and args.in_lines:
        crop_cell_x, crop_cell_y = resolve_cell(args.cell, args.cell_x, args.cell_y)
        roi = parse_cell_roi(
            args.roi, crop_cell_x, crop_cell_y, parse_cell_origin(args.cell_origin)
        )
    else:
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
