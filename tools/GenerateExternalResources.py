#!/usr/bin/env python3
"""从 HandyControls 源码生成 WPF 外部资源 key 清单（tools/ExternalWpfResources.txt）。

MAA 引用 NuGet 包 HandyControls（HandyControl 社区 fork，仓库
https://github.com/ghost1372/HandyControls ），其主题/样式资源以 BAML 内嵌于 dll，
无法从包内直接枚举 x:Key。本脚本按 csproj 中的包版本拉取对应 tag 的源码 tarball，
提取全部 xaml 的 x:Key 定义生成清单，供 CheckLocalizationReferences.py 作为
DynamicResource 校验的第三层有效集合（语言 key ∪ 本仓 x:Key 之外的外部资源）。

升级 HandyControls 版本后需重跑本脚本再提交更新后的清单。CI 不运行本脚本
（无网络依赖），只消费清单文件。访问 GitHub 需要代理的网络环境传 --proxy，
例如 --proxy http://127.0.0.1:7890；不传则直连。

用法：python tools/GenerateExternalResources.py <版本号，如 3.6.0> [--proxy <地址>]
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tools/ExternalWpfResources.txt"
REPO = "ghost1372/HandyControls"

XKEY_RE = re.compile(r'x:Key="([^"]+)"')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="HandyControls 版本号，如 3.6.0")
    parser.add_argument(
        "--proxy", help="HTTP 代理地址（访问 GitHub 受阻的环境使用），缺省直连"
    )
    args = parser.parse_args()
    version = args.version.lstrip("v")

    with tempfile.TemporaryDirectory() as tmp:
        tar_path = Path(tmp) / "src.tar.gz"
        cmd = [
            "curl",
            "-fsSL",
            f"https://github.com/{REPO}/archive/refs/tags/v{version}.tar.gz",
            "-o",
            str(tar_path),
        ]
        if args.proxy:
            cmd += ["-x", args.proxy]
        subprocess.run(cmd, check=True)
        keys: set[str] = set()
        with tarfile.open(tar_path) as tf:
            for member in tf.getmembers():
                if not member.name.endswith((".xaml", ".xaml.cs")):
                    continue
                f = tf.extractfile(member)
                if f is None:
                    continue
                text = f.read().decode("utf-8-sig", errors="replace")
                keys.update(XKEY_RE.findall(text))

    lines = sorted(keys)
    header = (
        f"# WPF 外部资源 key 清单（HandyControls v{version}，仓库 {REPO}）\n"
        "# 生成：python tools/GenerateExternalResources.py " + version + "\n"
        "# 升级 HandyControls 版本后重跑生成脚本并提交本文件\n"
    )
    OUT.write_text(header + "\n".join(lines) + "\n", encoding="utf-8")
    print(f"{OUT.name}: {len(lines)} keys")
    return 0


if __name__ == "__main__":
    sys.exit(main())
