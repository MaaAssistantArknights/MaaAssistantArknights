#!/usr/bin/env python3
"""Validate the WPF localization catalog and statically addressable references."""

from __future__ import annotations

import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GUI = ROOT / "src/MaaWpfGui"
CATALOG = GUI / "Res/Localizations"
LANGS = ("zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr")
XKEY = "{http://schemas.microsoft.com/winfx/2006/xaml}Key"
KEY_PATTERN = re.compile(r"^[A-Z][A-Za-z0-9]*(?:\.[A-Za-z0-9_+\-]+)+$")
INLINE = re.compile(r"\{key=([^}]+)\}")
RESOURCE = re.compile(r"\{(DynamicResource|StaticResource)\s+([^}\s]+)")
# These DynamicResource keys come from external theme dictionaries.
EXTERNAL_DYNAMIC_RESOURCES = frozenset(
    {
        "BorderBrush",
        "ButtonPrimary",
        "DangerBrush",
        "EffectShadow2",
        "InfoBrush",
        "PrimaryColor",
        "RadioGroupItemSingle",
        "SecondaryTextBrush",
    }
)
CALL = re.compile(
    r'LocalizationHelper\.(?:GetString|GetRawString|TryGetString|GetStringFormat|HasTranslation)\(\s*"([^"\n]+)"'
)


def check() -> list[str]:
    failures: list[str] = []
    catalog_keys: set[str] | None = None
    all_entries: list[tuple[Path, str, str]] = []
    for lang in LANGS:
        path = CATALOG / f"{lang}.xaml"
        try:
            root = ET.parse(path).getroot()
        except ET.ParseError as error:
            failures.append(f"{path}: invalid XML: {error}")
            continue
        entries = [(node.get(XKEY), node.text or "") for node in root]
        keys = [key for key, _ in entries if key is not None]
        if len(keys) != len(entries):
            failures.append(f"{path}: contains a resource without x:Key")
        if len(keys) != len(set(keys)):
            failures.append(f"{path}: duplicate x:Key")
        if keys != sorted(keys):
            failures.append(f"{path}: keys are not in ordinal order")
        for key in keys:
            if not KEY_PATTERN.fullmatch(key):
                failures.append(f"{path}: invalid domain key {key!r}")
        if catalog_keys is None:
            catalog_keys = set(keys)
        elif set(keys) != catalog_keys:
            failures.append(
                f"{path}: key set differs from {LANGS[0]} "
                f"(missing: {sorted(catalog_keys - set(keys))}, extra: {sorted(set(keys) - catalog_keys)})"
            )
        all_entries.extend(
            (path, key, value) for key, value in entries if key is not None
        )

    if catalog_keys is None:
        return failures
    for path, key, value in all_entries:
        for nested in INLINE.findall(value):
            if nested not in catalog_keys:
                failures.append(f"{path}: {key} references missing inline key {nested}")

    legacy_path = CATALOG / "legacy-key-map.tsv"
    old_keys: set[str] = set()
    for number, row in enumerate(
        legacy_path.read_text(encoding="utf-8").splitlines(), 1
    ):
        if row.startswith("#"):
            continue
        fields = row.split("\t")
        if len(fields) != 2 or not all(fields):
            failures.append(f"{legacy_path}:{number}: expected old<TAB>new")
            continue
        old, new = fields
        if old in old_keys:
            failures.append(f"{legacy_path}:{number}: duplicate legacy key {old}")
        old_keys.add(old)
        if old == new or new not in catalog_keys:
            failures.append(
                f"{legacy_path}:{number}: invalid legacy mapping {old} -> {new}"
            )
        if old in catalog_keys:
            failures.append(
                f"{legacy_path}:{number}: legacy key also exists in the catalog: {old}"
            )

    # Resource references can also address brushes and styles in other dictionaries.
    other_resources: set[str] = set()
    for path in GUI.rglob("*.xaml"):
        if (
            path.parent != CATALOG
            and "obj" not in path.parts
            and "bin" not in path.parts
        ):
            other_resources.update(
                re.findall(r'x:Key="([^"]+)"', path.read_text(encoding="utf-8-sig"))
            )
    domains = {key.split(".", 1)[0] for key in catalog_keys}
    for path in GUI.rglob("*.xaml"):
        if path.parent == CATALOG or "obj" in path.parts or "bin" in path.parts:
            continue
        source = path.read_text(encoding="utf-8-sig")
        for kind, key in RESOURCE.findall(source):
            if kind == "DynamicResource":
                if (
                    key not in catalog_keys
                    and key not in other_resources
                    and key not in EXTERNAL_DYNAMIC_RESOURCES
                ):
                    failures.append(f"{path}: missing DynamicResource {key}")
                continue
            if (
                (key.split(".", 1)[0] in domains or key in old_keys)
                and key not in catalog_keys
                and key not in other_resources
            ):
                failures.append(f"{path}: missing localized {kind} {key}")

    for path in GUI.rglob("*.cs"):
        if "obj" in path.parts or "bin" in path.parts:
            continue
        source = re.sub(
            r"/\*.*?\*/", "", path.read_text(encoding="utf-8-sig"), flags=re.S
        )
        for key in CALL.findall(source):
            if key not in catalog_keys:
                failures.append(f"{path}: missing localized call key {key}")
    return failures


def main() -> int:
    failures = check()
    if failures:
        for failure in failures:
            print(f"::error::{failure}")
        return 1
    print("localization catalog, ordering, mapping and static references valid")
    return 0


if __name__ == "__main__":
    sys.exit(main())
