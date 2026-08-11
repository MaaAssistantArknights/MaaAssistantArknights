"""命令行入口：python -m bead_render 或 python bead_render_cli.py

协议：
  成功打印 OK
  失败打印 FAIL: <原因>，退出码 1
"""
import argparse
import sys

from .renderer import render_preview, RENDER_MODES


class _Parser(argparse.ArgumentParser):
    def error(self, message):
        # 参数错误也走 FAIL 协议，而不是打印 usage
        raise ValueError(message)


def build_parser():
    ap = _Parser(prog="bead_render", add_help=True,
                 description="拼豆预览渲染（MAA PixelPaint）")
    ap.add_argument("--input", required=True, help="输入图片路径")
    ap.add_argument("--output_dir", required=True, help="输出目录（固定写 pixel_preview.png）")
    ap.add_argument("--grid_size", type=int, default=24, help="拼豆网格尺寸，默认 24")
    ap.add_argument("--render_mode", default="photo",
                    choices=RENDER_MODES,
                    help=f"渲染模式: {'/'.join(RENDER_MODES)}，默认 photo")
    ap.add_argument("--dither", type=str, default="", metavar="BOOL",
                    help="强制抖动模式: true/false/1/0，可选")
    ap.add_argument("--bg_color", default="FFFFFF", metavar="HEX",
                    help="背景色 6 位十六进制，默认 FFFFFF")
    ap.add_argument("--output_json", default="", metavar="PATH",
                    help="可选：色号分组 JSON 输出路径（供 MAA Core 绘制，C++ 消费）；不传则不生成")
    return ap


def _parse_bool(s):
    s = (s or "").strip().lower()
    if s in ("", "false", "0", "no"):
        return False
    if s in ("true", "1", "yes"):
        return True
    raise ValueError(f"--dither 只能传 true/false/1/0，收到: {s!r}")


def main(argv=None):
    try:
        args = build_parser().parse_args(argv)
        dither = _parse_bool(args.dither)
        out = render_preview(
            input_path=args.input,
            output_dir=args.output_dir,
            grid_size=args.grid_size,
            render_mode=args.render_mode,
            dither=dither,
            bg_color=args.bg_color,
            output_json=args.output_json,
        )
    except Exception as e:
        print(f"FAIL: {type(e).__name__}: {e}")
        return 1
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
