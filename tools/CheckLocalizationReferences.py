#!/usr/bin/env python3
"""校验本地化引用完整性（Res/Localizations/*.xaml 与其引用方）。

六类校验，任一失败以非零退出码退出（供 CI 拦截）：

1. 五语言文件 key 集合完全一致（翻译改动漏某个语言直接失败）；
2. 代码侧引用的 key 必须存在：XAML 的 DynamicResource/StaticResource、
   C# 的 GetString/TryGetString/GetRawString/GetStringFormat 字符串字面量参数
   （含首参换行的调用形态）与 Resources["X"] 索引读取形态。
   XAML 资源引用的有效集合为三层：语言 key ∪ 本仓 xaml 自身 x:Key 定义（动态
   收集，视图内部资源天然放行）∪ tools/ExternalWpfResources.txt（HandyControls
   外部资源清单，由 GenerateExternalResources.py 生成）；
3. 语言文件值内 {key=Xxx} 内联引用的 key 必须存在；
4. 代码内静态映射值（枚举/协议转换函数的 switch 右侧 key）必须存在；
5. LocalizedObservableList 列表条目 key 必须存在（切换语言的推荐写法）；
6. 单语言文件内 x:Key 不得重复（WPF 运行时后项覆盖前项，重复即事故）。

静态校验的固有盲区（不在校验范围）：key 经包装函数以变量传参
（FormatText/ShowToast 等）、运行时拼接家族（枚举 ToString、前缀 + N）、
内核与远端配置下发的动态 key。新增引用形态时优先落上述可扫描形态。

用法：python tools/CheckLocalizationReferences.py
"""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GUI = ROOT / "src/MaaWpfGui"
LOC = GUI / "Res/Localizations"
LANGS = ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr"]
XAML_NS_KEY = "{http://schemas.microsoft.com/winfx/2006/xaml}Key"

INLINE_KEY_RE = re.compile(r"\{key=([^}]+)\}")
# 标记扩展形式 {DynamicResource X} / {StaticResource X}；排除 <StaticResource ResourceKey=
# 属性形式里的属性名（ResourceKey 本身不是 key）
XAML_RES_RE = re.compile(
    r"(?:Dynamic|Static)Resource\s+(?!ResourceKey[=\s])([A-Za-z0-9._@-]+)"
)
# 元素属性形式 <StaticResource ResourceKey="X" />；只收静态字面量值，
# {Binding ...} 等运行时表达式（ResourceReferenceHelper 附加属性的值由 VM 提供）跳过
XAML_RES_ATTR_RE = re.compile(r'ResourceKey="([^"{]+)"')
XKEY_RE = re.compile(r'x:Key="([^"]+)"')
# 其他第三方库资源（HandyControls 清单之外）：MdXaml（Markdown 渲染）包内字典
EXTRA_EXTERNAL_RESOURCES = frozenset({"MdXamlStyle", "MdXamlPlugin"})
# 末尾负向前瞻排除 "前缀" + N 之类拼接形态（家族成员无法静态判定，不校验）
CS_CALL_RE = re.compile(
    r'(?:GetString|TryGetString|GetRawString|GetStringFormat)\(\s*@?"([^"]+)"(?!\s*\+)'
)
# 首参换行的调用：上一行以调用名加左括号结尾，下一行以字符串字面量开头
CS_CALL_OPEN_RE = re.compile(
    r"(?:GetString|TryGetString|GetRawString|GetStringFormat)\($"
)
CS_CALL_CONT_RE = re.compile(r'^\s*@?"([^"]+)"(?!\s*\+)')
# Resources["X"] 索引读取（排除赋值形态，赋值是写入不是引用）
RES_INDEX_RE = re.compile(r'Resources\["([^"]+)"\](?!\s*=)')


def line_contains_inline_key(line: str) -> bool:
    return bool(INLINE_KEY_RE.search(line))


def lang_keys() -> dict[str, set[str]]:
    result = {}
    for lang in LANGS:
        tree = ET.parse(LOC / f"{lang}.xaml")
        result[lang] = {
            el.get(XAML_NS_KEY) for el in tree.iter() if el.get(XAML_NS_KEY)
        }
    return result


def duplicate_keys() -> list[str]:
    """单语言文件内重复的 x:Key（WPF 运行时后项覆盖前项，直接报失败）。"""
    failures = []
    for lang in LANGS:
        seen: set[str] = set()
        for el in ET.parse(LOC / f"{lang}.xaml").iter():
            key = el.get(XAML_NS_KEY)
            if key is None:
                continue
            if key in seen:
                failures.append(f"{lang}.xaml: x:Key {key!r} 重复定义")
            seen.add(key)
    return failures


def collect_local_xaml_keys() -> set[str]:
    """本仓全部 xaml 自身定义的 x:Key（视图内部资源天然放行）。"""
    keys: set[str] = set()
    for p in GUI.rglob("*.xaml"):
        if any(part in {".git", "bin", "obj"} for part in p.parts):
            continue
        keys.update(XKEY_RE.findall(p.read_text(encoding="utf-8-sig")))
    return keys


def load_external_resources() -> set[str]:
    path = ROOT / "tools/ExternalWpfResources.txt"
    return {
        line.strip()
        for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    }


def scan_code_refs() -> list[tuple[str, int, str, str]]:
    """返回 [(file, line, key, source_line)]，只收静态可判定的引用。"""
    refs = []
    xaml_files = [p for p in GUI.rglob("*.xaml") if LOC not in p.parents]
    cs_files = [p for p in GUI.rglob("*.cs")]
    for p in xaml_files + cs_files:
        if any(part in {".git", "bin", "obj"} for part in p.parts):
            continue
        rel = str(p.relative_to(ROOT)).replace("\\", "/")
        text = p.read_text(encoding="utf-8-sig")
        res_re = XAML_RES_RE if p.suffix == ".xaml" else None
        in_block_comment = False
        pending_call_open = False
        for lineno, line in enumerate(text.splitlines(), 1):
            stripped = line.strip()
            if in_block_comment:
                if "*/" in line:
                    in_block_comment = False
                    # 只保留闭合符之后的代码部分
                    line = line.split("*/", 1)[1]
                    stripped = line.strip()
                else:
                    continue
            if (
                stripped.startswith("//")
                or stripped.startswith("*")
                or stripped.startswith("<!--")
            ):
                pending_call_open = False
                continue
            # 剥掉单行块注释段 /* ... */，跨行块注释由状态机跟踪
            line = re.sub(r"/\*.*?\*/", "", line)
            if "/*" in line and "*/" not in line:
                in_block_comment = True
                line = line.split("/*", 1)[0]
                stripped = line.strip()
            if res_re:
                for m in res_re.finditer(line):
                    refs.append((rel, lineno, m.group(1), stripped))
                for m in XAML_RES_ATTR_RE.finditer(line):
                    refs.append((rel, lineno, m.group(1), stripped))
            if p.suffix == ".cs":
                # 上一行以调用名加左括号结尾时，本行开头的字符串字面量即首参
                if pending_call_open:
                    m = CS_CALL_CONT_RE.match(line)
                    if m:
                        refs.append((rel, lineno, m.group(1), stripped))
                    pending_call_open = False
                for m in CS_CALL_RE.finditer(line):
                    refs.append((rel, lineno, m.group(1), stripped))
                pending_call_open = pending_call_open or bool(
                    CS_CALL_OPEN_RE.search(stripped)
                )
                for m in RES_INDEX_RE.finditer(line):
                    refs.append((rel, lineno, m.group(1), stripped))
    return refs


def scan_observable_list_keys() -> list[tuple[str, int, str, str]]:
    """扫描 LocalizedObservableList 列表条目里的本地化 key。

    两层识别：
    1. `new LocalizedObservableList` 构造的集合初始化块（花括号配对跟踪），
       块内全部含点字符串参数按 key 校验；
    2. `(非字符串, "含点字符串"` 元组形态兜底，覆盖 target-typed new（`new() { ... }`）
       与 Add 调用里的条目——裸词字符串不校验，避免撞上协议元组。
    完全动态的条目（变量传 key）静态不可判定，不在覆盖范围。
    """
    refs = []
    ctor_re = re.compile(r"new\s+LocalizedObservableList")
    tuple_re = re.compile(
        r'\(\s*[A-Za-z_][A-Za-z0-9_.]*\s*,\s*"([A-Za-z][A-Za-z0-9.]*)"'
    )
    dotted_str_re = re.compile(r'"([A-Za-z][A-Za-z0-9.]*)"')
    # 本地化 key 形态：至少两段、每段以大写字母或数字开头；
    # 排除 gui.json / NoxConsole.exe 这类路径与文件名（含小写段）
    pascal_key_re = re.compile(r"^(?:[A-Z0-9][A-Za-z0-9]*\.)+[A-Z0-9][A-Za-z0-9]*$")

    for p in GUI.rglob("*.cs"):
        if any(part in {".git", "bin", "obj"} for part in p.parts):
            continue
        rel = str(p.relative_to(ROOT)).replace("\\", "/")
        in_block_comment = False
        tracking = False
        depth = 0
        for lineno, line in enumerate(
            p.read_text(encoding="utf-8-sig").splitlines(), 1
        ):
            stripped = line.strip()
            if in_block_comment:
                if "*/" in line:
                    in_block_comment = False
                    line = line.split("*/", 1)[1]
                    stripped = line.strip()
                else:
                    continue
            if stripped.startswith("//") or stripped.startswith("*"):
                continue
            line = re.sub(r"/\*.*?\*/", "", line)
            if "/*" in line and "*/" not in line:
                in_block_comment = True
                line = line.split("/*", 1)[0]
                stripped = line.strip()

            if tracking:
                depth += line.count("{") - line.count("}")
                for m in dotted_str_re.finditer(line):
                    k = m.group(1)
                    if "." in k:
                        refs.append((rel, lineno, k, stripped))
                if depth <= 0:
                    tracking = False
                continue

            m = ctor_re.search(line)
            if m:
                seg = line[m.end() :]
                depth = seg.count("{") - seg.count("}")
                for sm in dotted_str_re.finditer(seg):
                    k = sm.group(1)
                    if "." in k:
                        refs.append((rel, lineno, k, stripped))
                if depth > 0 or ("{" not in seg and "}" not in seg):
                    # 块已开始未闭合，或初始化器 { 在后续行
                    tracking = True
                    depth = max(depth, 0)
            else:
                for tm in tuple_re.finditer(line):
                    k = tm.group(1)
                    if pascal_key_re.match(k):
                        refs.append((rel, lineno, k, stripped))
    return refs


def main() -> int:
    keys = lang_keys()
    base = keys["zh-cn"]

    # 分类收集：一致性类只汇总计数，其余类完整列出，避免大批量一致性失败淹没引用缺失
    consistency, missing, inline_missing, map_values = [], [], [], []

    for lang in LANGS[1:]:
        missing_keys = base - keys[lang]
        extra_keys = keys[lang] - base
        consistency.append(
            f"{lang}.xaml: 缺少 {len(missing_keys)} 个 zh-cn 存在的 key，多出 {len(extra_keys)} 个"
        )

    # 2. 代码引用存在。XAML 资源引用的三层有效集合：语言 key ∪ 本仓 xaml
    #    自身 x:Key 定义 ∪ HandyControls 外部清单（tools/ExternalWpfResources.txt）。
    #    不在任何一层即硬失败——视图内部资源被本地收集天然放行，HandyControl
    #    库资源由清单覆盖，剩下未命中的就是真断链或清单缺项。
    valid_resources = (
        base
        | collect_local_xaml_keys()
        | load_external_resources()
        | EXTRA_EXTERNAL_RESOURCES
    )
    for rel, lineno, key, line in scan_code_refs():
        if key in valid_resources:
            continue
        missing.append(
            f"{rel}:{lineno}: 引用的 key {key!r} 不在语言文件或资源集合中: {line[:120]}"
        )

    # 3. 值内 {key=} 内联引用存在
    for lang in LANGS:
        for lineno, line in enumerate(
            (LOC / f"{lang}.xaml").read_text(encoding="utf-8").splitlines(), 1
        ):
            for m in INLINE_KEY_RE.finditer(line):
                if m.group(1) not in base:
                    inline_missing.append(
                        f"{lang}.xaml:{lineno}: 内联引用的 key {m.group(1)!r} 不存在"
                    )

    # 4. 代码内静态映射值（枚举/协议转换函数的 switch 右侧 key）必须存在；
    #    这类值不经过 GetString("字面量") 调用形态，扫描器天然覆盖不到
    map_value_re = re.compile(r'=>\s*"([A-Za-z][A-Za-z0-9.]*)"')
    ext_files = [
        GUI / "Extensions/LocalizationKeyExtensions.cs",
        GUI / "Main/AsstProxy.cs",
    ]
    for p in ext_files:
        rel = str(p.relative_to(ROOT)).replace("\\", "/")
        for lineno, line in enumerate(
            p.read_text(encoding="utf-8-sig").splitlines(), 1
        ):
            for m in map_value_re.finditer(line):
                v = m.group(1)
                if v not in base and "." in v:
                    map_values.append(f"{rel}:{lineno}: 映射值 {v!r} 不在语言文件中")

    # 5. LocalizedObservableList 列表条目 key 必须存在（切换语言的推荐写法）
    list_missing = []
    for rel, lineno, key, line in scan_observable_list_keys():
        if key not in base:
            list_missing.append(
                f"{rel}:{lineno}: 列表条目 key {key!r} 不在语言文件中: {line[:120]}"
            )

    # 6. 单语言文件内 x:Key 不得重复
    duplicates = duplicate_keys()

    for f in missing + inline_missing + map_values + list_missing + duplicates:
        print(f"::error::{f}")
    for c in consistency:
        if not c.endswith("缺少 0 个 zh-cn 存在的 key，多出 0 个"):
            print(f"::error::[五语言一致性] {c}")

    if (
        missing
        or inline_missing
        or map_values
        or list_missing
        or duplicates
        or any(
            not c.endswith("缺少 0 个 zh-cn 存在的 key，多出 0 个") for c in consistency
        )
    ):
        return 1
    print(f"localization references ok ({len(base)} keys, 5 langs consistent)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
