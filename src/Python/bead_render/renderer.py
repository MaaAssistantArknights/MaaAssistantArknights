"""渲染管线：photo / illustration / edge / dither 四模式，基于 BeadEngine（bead_engine.py）

模式映射（假设，可调整）：
  photo        -> BeadEngine.pro1           border 裁 + 主色法 + LAB 量化（40 色自适应）
  illustration -> BeadEngine.pro2           中心方形 + 增强 + 主色 + mediancut 量化（20 色）
  edge         -> BeadEngine.palette_optimize  边缘+中心加权调色板优化（16 色）
  dither       -> pro1 主色结果 + FS 蛇形抖动量化到固定 40 色 PERLER_PALETTE
"""
import json
import os
import tempfile
from typing import List, Tuple

import numpy as np
import cv2
from PIL import Image

from .bead_engine import BeadEngine
from .palette import PERLER_PALETTE, get_palette_lab

RENDER_MODES = ("photo", "illustration", "edge", "dither")
DEFAULT_BG = "FFFFFF"
MAX_IMAGE_PX = 4096  # 大图保护：超限自动缩小

_ENGINE = BeadEngine()


def _parse_bg_color(bg_color: str) -> Tuple[int, int, int]:
    """'FFFFFF' -> (255,255,255)；支持 # 前缀；非法抛 ValueError"""
    s = (bg_color or DEFAULT_BG).lstrip("#")
    if len(s) != 6:
        raise ValueError(f"bg_color 格式应为 6 位十六进制，如 FFFFFF，收到: {bg_color!r}")
    try:
        r, g, b = (int(s[i:i + 2], 16) for i in (0, 2, 4))
    except ValueError:
        raise ValueError(f"bg_color 含非法十六进制字符: {bg_color!r}")
    return (r, g, b)


def prepare_image(img_path: str, bg_rgb: Tuple[int, int, int]) -> str:
    """读入图片：RGBA 透明按 bg 合成，存为临时 PNG 供引擎使用；超 4096px 自动缩小"""
    im = Image.open(img_path)
    if im.mode in ("RGBA", "LA", "P"):
        im = im.convert("RGBA")
        bg = Image.new("RGBA", im.size, bg_rgb + (255,))
        im = Image.alpha_composite(bg, im)
    else:
        im = im.convert("RGB")
    if max(im.size) > MAX_IMAGE_PX:
        im.thumbnail((MAX_IMAGE_PX, MAX_IMAGE_PX))
    fd, tmp = tempfile.mkstemp(suffix=".png", prefix="bead_render_")
    os.close(fd)
    try:
        im.save(tmp)
    except Exception:
        os.remove(tmp)
        raise
    return tmp


def _dither_fs(img: np.ndarray, n: int) -> np.ndarray:
    """把 (n,n,3) RGB 用蛇形 FS 误差扩散量化到固定 40 色板"""
    lab = cv2.cvtColor(img, cv2.COLOR_RGB2LAB).astype(np.float32)
    pal_lab = get_palette_lab()
    out = np.zeros((n, n, 3), np.uint8)
    cur = lab.copy()
    for y in range(n):
        row = range(n) if y % 2 == 0 else range(n - 1, -1, -1)
        for x in row:
            dist = np.sqrt(((cur[y, x] - pal_lab) ** 2).sum(1))
            k = int(np.argmin(dist))
            out[y, x] = PERLER_PALETTE[k]
            err = cur[y, x] - pal_lab[k]
            if y % 2 == 0:
                neighbors = [(y, x + 1, 7 / 16), (y + 1, x - 1, 3 / 16),
                             (y + 1, x, 5 / 16), (y + 1, x + 1, 1 / 16)]
            else:
                neighbors = [(y, x - 1, 7 / 16), (y + 1, x + 1, 3 / 16),
                             (y + 1, x, 5 / 16), (y + 1, x - 1, 1 / 16)]
            for ny, nx, w in neighbors:
                if 0 <= ny < n and 0 <= nx < n:
                    cur[ny, nx] += err * w
    return out


def _pixels_to_arr(pixels: List[Tuple[int, int, int]], n: int) -> np.ndarray:
    """BeadEngine 返回的 n*n 像素列表 -> (n,n,3) uint8 数组"""
    return np.asarray(pixels, np.uint8).reshape(n, n, 3)


def _save_preview(arr: np.ndarray, out_path: str, cell: int = 24, grid=(60, 60, 60)) -> None:
    """最近邻放大 + 绘制网格线，保存 PNG"""
    n = arr.shape[0]
    big = cv2.resize(arr, (n * cell, n * cell), interpolation=cv2.INTER_NEAREST)
    for i in range(n + 1):
        big[i * cell:i * cell + 1, :] = grid
        big[:, i * cell:i * cell + 1] = grid
    Image.fromarray(big).save(out_path)


def _arr_to_color_indices(arr: np.ndarray) -> np.ndarray:
    """把 (n,n,3) RGB 结果映射回 40 色板下标（LAB 最近邻），返回 (n,n) int32"""
    lab = cv2.cvtColor(arr, cv2.COLOR_RGB2LAB).astype(np.float32).reshape(-1, 3)
    pal_lab = get_palette_lab()
    dist = np.sqrt(((lab[:, None, :] - pal_lab[None, :, :]) ** 2).sum(2))
    return dist.argmin(1).reshape(arr.shape[0], arr.shape[1])


def build_groups(arr: np.ndarray, skip_white: bool = True) -> List[dict]:
    """按 40 色板下标分桶，输出供 MAA Core 绘制的分组点列（对齐 PixelPaintHelper.BuildGroups）。

    参数:
        arr: (n,n,3) RGB 渲染结果
        skip_white: 是否跳过纯白格（色板下标 3），默认 True，与 MAA 绘制行为一致

    返回:
        [{"color": 0~39, "points": [[x, y], ...]}, ...]，points 行优先（x 右 y 下）
    """
    n = arr.shape[0]
    idx = _arr_to_color_indices(arr)
    groups: List[dict] = []
    for c in range(len(PERLER_PALETTE)):
        if skip_white and c == 3:
            continue
        ys, xs = np.where(idx == c)
        if len(xs) == 0:
            continue
        points = [[int(x), int(y)] for x, y in zip(xs.tolist(), ys.tolist())]
        groups.append({"color": int(c), "points": points})
    return groups


def write_groups_json(arr: np.ndarray, out_path: str, skip_white: bool = True) -> str:
    """把色号分组写成 JSON（数据出口，供 MAA C#/C++ 消费）。

    结构:
        {
          "grid_size": 24,
          "skip_white": true,
          "groups": [{"color": 0~39, "points": [[x, y], ...]}, ...]
        }

    返回:
        写入的 JSON 绝对路径
    """
    payload = {
        "grid_size": int(arr.shape[0]),
        "skip_white": skip_white,
        "groups": build_groups(arr, skip_white),
    }
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(payload, f, ensure_ascii=False)
    return out_path


def render_photo(tmp: str, n: int) -> np.ndarray:
    """pro1：border 裁 + 主色法 + LAB 量化（40 色自适应）"""
    return _pixels_to_arr(_ENGINE.pro1(tmp, n=n, palette=40), n)


def render_illustration(tmp: str, n: int) -> np.ndarray:
    """pro2：中心方形 + 增强 + 主色 + mediancut 量化（20 色）"""
    return _pixels_to_arr(_ENGINE.pro2(tmp, n=n, max_colors=20), n)


def render_edge(tmp: str, n: int) -> np.ndarray:
    """palette_optimize：边缘+中心加权调色板优化（16 色）"""
    return _pixels_to_arr(_ENGINE.palette_optimize(tmp, n=n, palette_size=16), n)


def render_dither_mode(tmp: str, n: int) -> np.ndarray:
    """pro1 主色结果 + FS 蛇形抖动量化到固定 40 色板"""
    small = _pixels_to_arr(_ENGINE.pro1(tmp, n=n, palette=40), n)
    return _dither_fs(small, n)


def render_preview(input_path: str, output_dir: str, grid_size: int = 24,
                   render_mode: str = "photo", dither: bool = False,
                   bg_color: str = DEFAULT_BG, output_json: str = "") -> str:
    """渲染拼豆预览图。

    参数:
        input_path: 输入图片路径（必填）
        output_dir: 输出目录（必填），预览图固定保存为 pixel_preview.png
        grid_size:  拼豆网格尺寸，默认 24
        render_mode: photo/illustration/edge/dither，默认 photo
        dither:     布尔，True 时强制走 FS 抖动模式
        bg_color:   背景色 6 位十六进制，默认 FFFFFF
        output_json: 可选，色号分组 JSON 输出路径；传入时额外写出
                     {"grid_size", "skip_white", "groups"}，供 MAA Core 绘制（C++ 消费）

    返回:
        输出 PNG 的绝对路径

    异常:
        ValueError: 参数非法；IOError/OSError: 文件无法读取
    """
    if not input_path or not os.path.isfile(input_path):
        raise ValueError(f"输入文件不存在: {input_path!r}")
    if not output_dir:
        raise ValueError("output_dir 不能为空")
    if not isinstance(grid_size, int) or grid_size < 1:
        raise ValueError(f"grid_size 必须为正整数: {grid_size!r}")
    if render_mode not in RENDER_MODES:
        raise ValueError(f"render_mode 必须是 {'/'.join(RENDER_MODES)} 之一，收到: {render_mode!r}")

    bg_rgb = _parse_bg_color(bg_color)
    tmp = prepare_image(input_path, bg_rgb)
    try:
        if dither or render_mode == "dither":
            arr = render_dither_mode(tmp, grid_size)
        elif render_mode == "photo":
            arr = render_photo(tmp, grid_size)
        elif render_mode == "illustration":
            arr = render_illustration(tmp, grid_size)
        elif render_mode == "edge":
            arr = render_edge(tmp, grid_size)

        os.makedirs(output_dir, exist_ok=True)
        out_path = os.path.join(output_dir, "pixel_preview.png")
        _save_preview(arr, out_path)
        if output_json:
            write_groups_json(arr, output_json)
        return out_path
    finally:
        try:
            os.remove(tmp)  # 清理临时文件失败可忽略
        except OSError:
            pass
