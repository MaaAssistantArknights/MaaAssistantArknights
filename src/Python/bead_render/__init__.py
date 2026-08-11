"""bead_render - MAA PixelPaint 拼豆预览渲染库

独立自包含的拼豆预览渲染库：不依赖外部项目代码，
核心算法基于 bead_converter（image_to_beads 主流程）。

用法（API）：
    from bead_render import render_preview
    out = render_preview("a.png", "out_dir", grid_size=24,
                         render_mode="illustration", dither=False, bg_color="FFFFFF")

用法（命令行）：
    python -m bead_render --input a.png --output_dir out_dir ...
"""
from .renderer import render_preview, build_groups, write_groups_json
from .palette import PERLER_PALETTE

__all__ = ["render_preview", "build_groups", "write_groups_json", "PERLER_PALETTE"]
__version__ = "1.1.0"
