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


def minify_json_file(
    json_path: Path, ensure_ascii: bool = False
) -> tuple[bool, int, int]:
    """
    Minify a single JSON file by removing unnecessary whitespace and newlines.

    :param json_path: Path to the JSON file
    :param ensure_ascii: Whether to escape non-ASCII characters to \\uxxxx
    :return: (success, original_size, minified_size)
    """
    try:
        original_size = json_path.stat().st_size

        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)

        # Write to a temporary file first, then atomically replace
        # to avoid corrupting the original file if interrupted
        tmp_path = f"{json_path}.tmp"
        try:
            with open(tmp_path, "w", encoding="utf-8") as f:
                json.dump(
                    data,
                    f,
                    ensure_ascii=ensure_ascii,
                    separators=(",", ":"),
                )
                # Flush to disk before atomic replace
                f.flush()
                os.fsync(f.fileno())
            os.replace(tmp_path, json_path)
        finally:
            # Clean up temp file on error (ignore cleanup failures)
            try:
                if os.path.exists(tmp_path):
                    os.remove(tmp_path)
            except Exception:
                pass

        minified_size = json_path.stat().st_size
        return True, original_size, minified_size
    except Exception as e:
        logger.error("Failed to process %s: %s", json_path, e)
        return False, 0, 0


def _format_size(size_bytes: int) -> str:
    """Format byte size to a human-readable string."""
    for unit in ("B", "KB", "MB", "GB"):
        if abs(size_bytes) < 1024:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024
    return f"{size_bytes:.2f} TB"


def minify_json_in_folder(
    folder_path: str,
    ensure_ascii: bool = False,
    dry_run: bool = False,
) -> None:
    """
    Recursively minify all JSON files in a folder and its subfolders.

    :param folder_path: Target folder path
    :param ensure_ascii: Whether to escape non-ASCII characters
    :param dry_run: If True, only list files without modifying them
    """
    root = Path(folder_path)
    if not root.is_dir():
        logger.error("'%s' is not a valid directory", folder_path)
        return

    json_files = list(root.rglob("*.json"))
    if not json_files:
        logger.warning("No JSON files found in '%s'", folder_path)
        return

    logger.info("Found %d JSON file(s)", len(json_files))
    if dry_run:
        for f in json_files:
            logger.info("  - %s", f)
        logger.info("(dry-run mode, no files modified)")
        return

    success_count = 0
    total_original = 0
    total_minified = 0
    for json_path in json_files:
        success, original_size, minified_size = minify_json_file(
            json_path, ensure_ascii
        )
        if success:
            success_count += 1
            total_original += original_size
            total_minified += minified_size
            saved = original_size - minified_size
            if saved > 0:
                logger.info(
                    "Minified: %s (%s -> %s, saved %s)",
                    json_path,
                    _format_size(original_size),
                    _format_size(minified_size),
                    _format_size(saved),
                )
            else:
                logger.info(
                    "Unchanged: %s (%s)", json_path, _format_size(original_size)
                )

    total_saved = total_original - total_minified
    logger.info(
        "Done: %d/%d file(s) processed. Total size: %s -> %s (saved %s, %.1f%%)",
        success_count,
        len(json_files),
        _format_size(total_original),
        _format_size(total_minified),
        _format_size(total_saved),
        (total_saved / total_original * 100) if total_original > 0 else 0,
    )


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
