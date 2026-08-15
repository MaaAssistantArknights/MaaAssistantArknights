#!/usr/bin/env python3
"""校验 MaaAppHostStub 内嵌文案与 Res/Localizations/*.xaml 同步。

MaaAppHostStub.cpp 硬编码了 ErrorCongratulations / ErrorSolutionMoveMaaExeOutOfFolder
的五种语言文案（stub 启动时 .NET 尚未加载，无法读取 xaml 资源），本脚本比对两者，
防止改动 xaml 后忘记同步 stub。在 ci.yml / release-nightly-ota.yml 中调用。
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CPP_PATH = ROOT / "src/MaaAppHostStub/MaaAppHostStub.cpp"
LANGS = ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr"]
KEYS = ["ErrorCongratulations", "ErrorSolutionMoveMaaExeOutOfFolder"]


def xaml_value(lang: str, key: str) -> str | None:
    path = ROOT / "src/MaaWpfGui/Res/Localizations" / f"{lang}.xaml"
    text = path.read_text(encoding="utf-8")
    match = re.search(rf'x:Key="{key}"[^>]*>(.*?)</system:String>', text, re.S)
    return match.group(1) if match else None


def main() -> int:
    cpp = CPP_PATH.read_text(encoding="utf-8-sig")
    # 拼接相邻的 L"..." 字面量，使被换行切断的文案恢复为完整子串
    cpp_joined = re.sub(r'"\s*L"', "", cpp)

    failures = []
    for lang in LANGS:
        for key in KEYS:
            value = xaml_value(lang, key)
            if value is None:
                failures.append(f"{lang}.xaml: 找不到 key {key}")
                continue
            if value not in cpp_joined:
                failures.append(f"{lang}.xaml: {key} 的文案未在 MaaAppHostStub.cpp 中找到，需同步")

    if failures:
        for failure in failures:
            print(f"::error::{failure}")
        return 1
    print(f"stub localization in sync ({len(LANGS)} langs x {len(KEYS)} keys)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
