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
WPF_RESOURCE_CALL = re.compile(
    r'(?:SetResourceReference\(\s*[^,]+,\s*|Application\.Current\.Resources\[\s*|(?:Try)?FindResource\(\s*)"([^"\n]+)"'
)
# Families with a single semantic owner. Match the old key because runtime data
# can still send that key, while the map points to its current catalog entry.
LEGACY_OWNER_RULES = (
    (re.compile(r"^AccountSwitch"), "TaskQueue"),
    (re.compile(r"^AdditionCommand"), "Connection"),
    (re.compile(r"^Adb"), "Connection"),
    (re.compile(r"^Algorithm"), "Reclamation"),
    (re.compile(r"^AutoRecruit"), "Recruitment"),
    (re.compile(r"^BackgroundLink"), "Hotkey"),
    (re.compile(r"^BlackFlow"), "Roguelike"),
    (re.compile(r"^Bosky"), "Roguelike"),
    (re.compile(r"^Credit"), "Mall"),
    (re.compile(r"^DeepExploration"), "Roguelike"),
    (re.compile(r"^EndsWithScript"), "TaskQueue"),
    (re.compile(r"^Executable$"), "Startup"),
    (re.compile(r"^ExternalNotification"), "ExternalNotification"),
    (re.compile(r"^Infrast(?=[A-Z])"), "Infrastructure"),
    (re.compile(r"^(?:LeftClick|RightClick|TaskSettings)$"), "Guide"),
    (re.compile(r"^Ld"), "Connection"),
    (re.compile(r"^MirrorChyan"), "Update"),
    (re.compile(r"^ManualStopWithScript"), "TaskQueue"),
    (re.compile(r"^MuMu|^Mumu"), "Connection"),
    (re.compile(r"^NewVersion"), "Update"),
    (re.compile(r"^OperBox"), "Operator"),
    (re.compile(r"^Penguin"), "PenguinStats"),
    (re.compile(r"^Roguelike(?=[A-Z0-9])"), "Roguelike"),
    (re.compile(r"^StartsWithScript"), "TaskQueue"),
    (re.compile(r"^(?:SwitchLanguage|HideTray)$"), "Tray"),
    (re.compile(r"^Test$"), "Connection"),
    (re.compile(r"^UserDataUpdate"), "TaskQueue"),
    (re.compile(r"^HelpUsWithOverseasServersTip$"), "Connection"),
    (re.compile(r"^WindowUpdateInfo"), "Update"),
    (re.compile(r"^Yituliu"), "Operator"),
)
LEGACY_PAIR_SUFFIXES = ("Tooltip", "ToolTip", "Tips", "Tip", "Desc", "Title", "Warning")
# These old names are homonyms for a different feature or describe the whole
# feature rather than the old base key.
LEGACY_PAIR_EXCEPTIONS = frozenset(
    {"CopilotTip", "GachaWarning", "PeepTip", "RecruitingTips", "SwitchThemeTip"}
)
# Task controls live in one flat directory, so the file stem acts as the
# feature boundary. Common.* is shared UI vocabulary; other cross-feature
# references must be listed by exact key below after review.
TASK_CONTROL_PRIMARY = {
    "AwardTaskUserControl": ("TaskQueue.Award",),
    "CustomTaskUserControl": ("TaskQueue.Custom",),
    "DepotMaintainTaskUserControl": ("Depot",),
    "FightTaskUserControl": ("Combat",),
    "InfrastTaskUserControl": ("Infrastructure",),
    "MallTaskUserControl": ("Mall",),
    "OperProgressTaskUserControl": ("Operator",),
    "PostActionUserControl": ("TaskQueue",),
    "RecruitTaskUserControl": ("Recruitment",),
    "RoguelikeTaskUserControl": ("Roguelike",),
    "StartUpTaskUserControl": ("Connection", "Startup"),
    "SwitchThemeTaskUserControl": ("GameTheme",),
    "UserDataUpdateTaskUserControl": ("TaskQueue.UserDataUpdate",),
}
TASK_CONTROL_SHARED: dict[str, frozenset[str]] = {
    "AwardTaskUserControl": frozenset({"Gacha.Warning"}),
    "DepotMaintainTaskUserControl": frozenset(
        {
            "Combat.CustomStageCode",
            "Combat.CustomStageCode.Tip",
            "Combat.DefaultStage",
            "Combat.UseOriginitePrime",
            "Combat.UseSanityPotion",
        }
    ),
    "FightTaskUserControl": frozenset(
        {
            "Depot.PlanInventoryDataEmpty",
            "Depot.PlanInventoryEnough",
            "Depot.PlanInventoryInsufficient",
            "Depot.PlanSanityInsufficient",
            "Inventory.Update.Tip",
            "TaskQueue.CheckBoxesNotSaved",
            "TaskQueue.CheckBoxesNotSavedAsNull",
            "TaskQueue.WeeklySchedule.Enabled",
            "TaskQueue.WeeklySchedule.Enabled.Tip",
        }
    ),
    "InfrastTaskUserControl": frozenset(
        {
            "Appearance.LabelSequence.Tip",
            "TaskQueue.WeeklySchedule.Enabled",
            "TaskQueue.WeeklySchedule.Enabled.Tip",
        }
    ),
    "MallTaskUserControl": frozenset(
        {
            "Combat.UseFormation",
            "Infrastructure.ReceptionClueExchange",
            "Infrastructure.ReceptionMessageBoardReceive",
            "Infrastructure.ReceptionSendClue",
            "TaskQueue.Visiting",
        }
    ),
    "OperProgressTaskUserControl": frozenset({"Appearance.LabelSequence.Tip"}),
    "RoguelikeTaskUserControl": frozenset({"TaskQueue.MultiTasksShare.Tip"}),
    "StartUpTaskUserControl": frozenset(
        {
            "TaskQueue.AccountSwitch",
            "TaskQueue.AccountSwitch.ManualRun",
            "TaskQueue.AccountSwitch.Tip",
            "TaskQueue.MultiTasksShare.Tip",
            "Window.Restore",
        }
    ),
    "UserDataUpdateTaskUserControl": frozenset(
        {
            "Depot.LastSyncTime",
            "Depot.PlanUpdateDepotFailed",
            "Depot.Recognition",
            "Operator.Recognition",
            "Operator.YituliuOperBoxCompleted",
            "Operator.YituliuTokenEmpty",
            "Operator.YituliuTokenNetworkError",
        }
    ),
}


def check_task_control_owner(
    path: Path, key: str, catalog_keys: set[str]
) -> str | None:
    if key not in catalog_keys or "TaskQueue" not in path.parts:
        return None
    for stem, prefixes in TASK_CONTROL_PRIMARY.items():
        if not path.name.startswith(stem):
            continue
        if key.startswith("Common.") or key in TASK_CONTROL_SHARED.get(
            stem, frozenset()
        ):
            return None
        if any(key == prefix or key.startswith(prefix + ".") for prefix in prefixes):
            return None
        return f"{path}: {key} is outside the {stem} localization whitelist"
    return f"{path}: no localization ownership policy for {path.name}"


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
    old_to_new: dict[str, str] = {}
    old_lines: dict[str, int] = {}
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
        old_to_new[old] = new
        old_lines[old] = number
        if old == new or new not in catalog_keys:
            failures.append(
                f"{legacy_path}:{number}: invalid legacy mapping {old} -> {new}"
            )
        if old in catalog_keys:
            failures.append(
                f"{legacy_path}:{number}: legacy key also exists in the catalog: {old}"
            )
        for pattern, owner in LEGACY_OWNER_RULES:
            if pattern.match(old) and new.split(".", 1)[0] != owner:
                failures.append(
                    f"{legacy_path}:{number}: {old} belongs to {owner}, mapped to {new}"
                )
                break

    for old, new in old_to_new.items():
        if old in LEGACY_PAIR_EXCEPTIONS:
            continue
        for suffix in LEGACY_PAIR_SUFFIXES:
            if not old.endswith(suffix):
                continue
            base = old[: -len(suffix)]
            if base in old_to_new and not new.startswith(old_to_new[base] + "."):
                failures.append(
                    f"{legacy_path}:{old_lines[old]}: {old} should be grouped under "
                    f"{old_to_new[base]}, mapped to {new}"
                )
            break

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
            ownership_failure = check_task_control_owner(path, key, catalog_keys)
            if ownership_failure:
                failures.append(ownership_failure)
            if kind == "DynamicResource":
                if (
                    key not in catalog_keys
                    and key not in other_resources
                    and key not in EXTERNAL_DYNAMIC_RESOURCES
                ):
                    failures.append(f"{path}: missing DynamicResource {key}")
                continue
            if key in catalog_keys:
                failures.append(
                    f"{path}: localized StaticResource {key} will not refresh after a language change"
                )
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
            ownership_failure = check_task_control_owner(path, key, catalog_keys)
            if ownership_failure:
                failures.append(ownership_failure)
            if key not in catalog_keys:
                failures.append(f"{path}: missing localized call key {key}")
        for key in WPF_RESOURCE_CALL.findall(source):
            if (
                (key.split(".", 1)[0] in domains or key in old_keys)
                and key not in catalog_keys
                and key not in other_resources
            ):
                failures.append(f"{path}: missing WPF resource key {key}")
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
