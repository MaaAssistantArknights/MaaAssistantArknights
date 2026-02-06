#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
压缩指定文件夹及其子文件夹中的所有 JSON 文件。
消除文件内不需要的空格和换行，使 JSON 紧凑化（仍为 .json 格式）。
"""

import argparse
import json
import logging
import os
from pathlib import Path

logger = logging.getLogger(__name__)


def minify_json_file(json_path: Path, ensure_ascii: bool = False) -> bool:
    """
    将单个 JSON 文件紧凑化：去除多余空格和换行。

    :param json_path: JSON 文件路径
    :param ensure_ascii: 是否将非 ASCII 字符转义为 \\uxxxx
    :return: 是否成功
    """
    try:
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)

        # 写入临时文件，再原子替换，避免中断时导致原文件损坏
        tmp_path = f"{json_path}.tmp"
        try:
            with open(tmp_path, "w", encoding="utf-8") as f:
                json.dump(
                    data,
                    f,
                    ensure_ascii=ensure_ascii,
                    separators=(",", ":"),
                )
                # 确保内容落盘，再进行原子替换
                f.flush()
                os.fsync(f.fileno())
            os.replace(tmp_path, json_path)
        finally:
            # 如果中间出错，尽量清理临时文件（忽略清理失败）
            try:
                if os.path.exists(tmp_path):
                    os.remove(tmp_path)
            except Exception:
                pass

        return True
    except Exception as e:
        logger.error("处理失败 %s: %s", json_path, e)
        return False


def minify_json_in_folder(
    folder_path: str,
    ensure_ascii: bool = False,
    dry_run: bool = False,
) -> None:
    """
    递归处理文件夹及子文件夹中的所有 JSON 文件，消除多余空格和换行。

    :param folder_path: 目标文件夹路径
    :param ensure_ascii: 是否将非 ASCII 字符转义
    :param dry_run: 若为 True，仅列出将要处理的文件，不实际修改
    """
    root = Path(folder_path)
    if not root.is_dir():
        logger.error("'%s' 不是有效文件夹路径", folder_path)
        return

    json_files = list(root.rglob("*.json"))
    if not json_files:
        logger.warning("在 '%s' 及其子文件夹中未找到 JSON 文件", folder_path)
        return

    logger.info("找到 %d 个 JSON 文件", len(json_files))
    if dry_run:
        for f in json_files:
            logger.info("  - %s", f)
        logger.info("(dry_run 模式，未实际修改)")
        return

    success_count = 0
    for json_path in json_files:
        logger.info("处理: %s", json_path)
        if minify_json_file(json_path, ensure_ascii):
            success_count += 1

    logger.info("完成: 成功处理 %d/%d 个文件", success_count, len(json_files))


def main():
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    parser = argparse.ArgumentParser(
        description="消除指定文件夹及子文件夹中所有 JSON 文件内多余的空格和换行，保留 .json 格式"
    )
    parser.add_argument(
        "folder",
        nargs="?",
        default=".",
        help="要处理的文件夹路径（默认为当前目录）",
    )
    parser.add_argument(
        "--ensure-ascii",
        action="store_true",
        help="将非 ASCII 字符（如中文）转义为 \\uxxxx",
    )
    parser.add_argument(
        "-n",
        "--dry-run",
        action="store_true",
        help="仅列出将要处理的文件，不实际修改",
    )
    args = parser.parse_args()

    minify_json_in_folder(
        folder_path=args.folder,
        ensure_ascii=args.ensure_ascii,
        dry_run=args.dry_run,
    )


if __name__ == "__main__":
    main()
