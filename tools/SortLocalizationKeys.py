#!/usr/bin/env python3
"""按 key 字节序重排本地化 xaml 文件（Res/Localizations/*.xaml）。

重命名规范落地后，语言文件内条目按 x:Key 字节序排序，同域 key 天然聚簇。
本脚本无参数运行为归位重排（原地改写），--check 运行为校验模式（排序后与
现状不一致时以非零退出码失败，供 CI 拦截乱插 key 的改动）。

区块标记注释（<!-- 名称 --> / <!-- !名称 -->，仅纯标识符形态）与注释掉的旧条目
死行在重排时一并移除；说明性短注释（含空格/标点，如 TODO）与长注释（如 stub
同步提醒）跟随其后第一个条目移动；跨行 XML 块注释整块移除。

用法：
  python tools/SortLocalizationKeys.py                 # 重排全部五语言文件
  python tools/SortLocalizationKeys.py zh-cn           # 只重排指定语言
  python tools/SortLocalizationKeys.py --check         # CI 校验模式
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOC = ROOT / "src/MaaWpfGui/Res/Localizations"
LANGS = ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr"]

ENTRY_RE = re.compile(r'^(\s*)<system:String\s+x:Key="([^"]+)"')
# 区块标记（<!-- 名称 --> / <!-- !名称 -->）：仅纯标识符形态视为标记并丢弃，
# 含空格/标点的短注释（如 <!-- TODO: ... -->）是说明性内容，保留并随条目移动
SECTION_RE = re.compile(r"^\s*<!--\s*!?[A-Za-z0-9_.-]+\s*-->\s*$")
LONG_COMMENT_RE = re.compile(r"^\s*<!--.*-->\s*$")


def parse_file(path: Path):
    """解析为 (head_lines, entries, tail_lines)。

    entries: [{key, lines: [完整原始行], leading_comments: [注释行]}]
    多行条目（字面换行的 value）从开始标签累积到 </system:String> 行。
    区块标记注释与死行被丢弃；说明性长注释附着到下一个条目。
    """
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines()
    head, tail_candidates, entries = [], [], []
    pending_entry = None
    pending_comments = []
    in_xaml_comment = False

    for line in lines:
        stripped = line.strip()

        # 跨行 XML 块注释（<!-- 开、后续行才 -->）：整块丢弃，防止中间行
        # 被误当条目内容或尾部内容而打乱文件结构
        if in_xaml_comment:
            if "-->" in line:
                in_xaml_comment = False
            continue

        if pending_entry is not None:
            pending_entry["lines"].append(line)
            if "</system:String>" in line:
                entries.append(pending_entry)
                pending_entry = None
            continue

        m = ENTRY_RE.match(line)
        if m:
            entry = {
                "key": m.group(2),
                "lines": [line],
                "leading_comments": pending_comments,
            }
            pending_comments = []
            if "</system:String>" not in line:
                pending_entry = entry
            else:
                entries.append(entry)
            continue

        if stripped.startswith("<!--"):
            # 注释掉的旧条目：丢弃
            inner = stripped.removeprefix("<!--").removesuffix("-->").strip()
            if inner.startswith("<system:String"):
                continue
            # 未闭合的块注释开头：进入跨行注释状态
            if "-->" not in stripped:
                in_xaml_comment = True
                continue
            # 区块标记（<!-- 名称 --> / <!-- !名称 -->）：丢弃
            if SECTION_RE.match(line):
                continue
            # 说明性长注释：附着到下一个条目；若之后没有条目则保留在尾部
            pending_comments.append(line)
            continue

        if not entries and not pending_comments:
            head.append(line)
        elif stripped:
            # 条目序列中的空行丢弃（重排后统一无空行），非空行留作尾部（闭合标签等）
            tail_candidates.append(line)

    # 文件尾：ResourceDictionary 闭合标签前可能残留注释/空行
    if pending_entry is not None:
        raise ValueError(
            f"条目 {pending_entry['key']!r} 未闭合（缺少 </system:String>），拒绝重排以免静默丢失"
        )
    return head, entries, tail_candidates


def render(path: Path, head, entries, tail) -> str:
    entries = sorted(entries, key=lambda e: e["key"])
    parts = []
    for line in head:
        parts.append(line)
    if parts and parts[-1].strip():
        parts.append("")
    for e in entries:
        parts.extend(e["leading_comments"])
        parts.extend(e["lines"])
    for line in tail:
        parts.append(line)
    text = "\n".join(parts)
    if not text.endswith("\n"):
        text += "\n"
    return text


def process(path: Path, check: bool) -> bool:
    try:
        head, entries, tail = parse_file(path)
    except ValueError as e:
        print(f"::error::{path.name}: {e}")
        return False
    new_text = render(path, head, entries, tail)
    old_text = path.read_text(encoding="utf-8")
    if new_text == old_text:
        return True
    if check:
        print(
            f"::error::{path.name} 条目顺序不符合 key 字节序（跑 tools/SortLocalizationKeys.py 归位）"
        )
        return False
    path.write_text(new_text, encoding="utf-8")
    print(f"{path.name}: 重排 {len(entries)} 个条目")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "langs", nargs="*", help="语言代码（如 zh-cn），缺省为全部五语言"
    )
    parser.add_argument(
        "--check", action="store_true", help="校验模式：不写文件，乱序时退出码 1"
    )
    args = parser.parse_args()

    langs = args.langs or LANGS
    ok = True
    for lang in langs:
        path = LOC / f"{lang}.xaml"
        if not path.exists():
            print(f"::error::找不到 {path}")
            ok = False
            continue
        ok = process(path, args.check) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
