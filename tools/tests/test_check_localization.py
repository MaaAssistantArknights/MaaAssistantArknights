"""Regression cases for localization migration checks."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[1] / "CheckLocalization.py"
SPEC = importlib.util.spec_from_file_location("check_localization", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
checker = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(checker)

HEADER = (
    '<ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" '
    'xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" '
    'xmlns:system="clr-namespace:System;assembly=mscorlib">\n'
)


class LocalizationCheckerTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.gui = Path(self.temp.name) / "src/MaaWpfGui"
        self.catalog = self.gui / "Res/Localizations"
        self.catalog.mkdir(parents=True)
        for name, value in (("GUI", self.gui), ("CATALOG", self.catalog)):
            patcher = patch.object(checker, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)

    def write_catalog(self, keys: dict[str, str], aliases: dict[str, str]) -> None:
        source = HEADER
        for key, value in sorted(keys.items()):
            source += f'    <system:String x:Key="{key}">{value}</system:String>\n'
        source += "</ResourceDictionary>\n"
        for lang in checker.LANGS:
            (self.catalog / f"{lang}.xaml").write_text(source, encoding="utf-8")
        rows = ["# frozen", *(f"{old}\t{new}" for old, new in aliases.items())]
        (self.catalog / "legacy-key-map.tsv").write_text(
            "\n".join(rows) + "\n", encoding="utf-8"
        )

    def test_reports_every_static_reference_breakage(self) -> None:
        self.write_catalog({"Common.One": "one"}, {})
        (self.gui / "Broken.xaml").write_text(
            '<TextBlock Text="{StaticResource Common.One}" '
            'ToolTip="{DynamicResource Common.Missing}" />\n',
            encoding="utf-8",
        )
        (self.gui / "Broken.cs").write_text(
            'menu.SetResourceReference(Property, "Common.Missing");\n'
            'var label = $"{LocalizationHelper.GetString("Common.Missing")}";\n',
            encoding="utf-8",
        )
        messages = checker.check()
        self.assertEqual(4, len(messages), messages)
        self.assertTrue(
            any("localized StaticResource" in message for message in messages)
        )
        self.assertTrue(
            any("missing DynamicResource" in message for message in messages)
        )
        self.assertTrue(any("missing WPF resource" in message for message in messages))
        self.assertTrue(
            any("missing localized call" in message for message in messages)
        )

    def test_task_controls_can_only_reference_whitelisted_domains(self) -> None:
        self.write_catalog(
            {
                "Common.Cancel": "cancel",
                "Combat.UseFormation": "formation",
                "Mall.Good": "shop task option",
                "Roguelike.Wrong": "other task option",
            },
            {},
        )
        task_view = self.gui / "Views/UserControl/TaskQueue/MallTaskUserControl.xaml"
        task_view.parent.mkdir(parents=True)
        task_view.write_text(
            '<TextBlock Text="{DynamicResource Mall.Good}" '
            'ToolTip="{DynamicResource Common.Cancel}" />\n'
            '<TextBlock Text="{DynamicResource Combat.UseFormation}" />\n'
            '<TextBlock Text="{DynamicResource Roguelike.Wrong}" />\n',
            encoding="utf-8",
        )
        messages = checker.check()
        self.assertEqual(1, len(messages), messages)
        self.assertIn("Roguelike.Wrong", messages[0])

    def test_new_task_control_needs_an_explicit_policy(self) -> None:
        self.write_catalog({"Mall.Good": "shop task option"}, {})
        task_view = (
            self.gui / "Views/UserControl/TaskQueue/NewFeatureTaskUserControl.xaml"
        )
        task_view.parent.mkdir(parents=True)
        task_view.write_text(
            '<TextBlock Text="{DynamicResource Mall.Good}" />\n', encoding="utf-8"
        )
        self.assertTrue(
            any(
                "no localization ownership policy" in message
                for message in checker.check()
            )
        )

    def test_reports_every_reviewed_ownership_and_pair_breakage(self) -> None:
        self.write_catalog(
            {
                "Roguelike.AccountSwitch": "switch account",
                "Roguelike.Algorithm.Badge": "badge",
                "Settings.CreditFight": "credit fight",
                "Settings.SwitchLanguage": "switch language",
                "Mall.CreditFightAlt.Tip": "tip",
            },
            {
                "AccountSwitch": "Roguelike.AccountSwitch",
                "AlgorithmBadge": "Roguelike.Algorithm.Badge",
                "CreditFight": "Settings.CreditFight",
                "CreditFightTip": "Mall.CreditFightAlt.Tip",
                "SwitchLanguage": "Settings.SwitchLanguage",
            },
        )
        messages = checker.check()
        for old in (
            "AccountSwitch",
            "AlgorithmBadge",
            "CreditFight",
            "CreditFightTip",
            "SwitchLanguage",
        ):
            self.assertTrue(
                any(old in message for message in messages), (old, messages)
            )

    def test_allows_distinct_switch_theme_meanings(self) -> None:
        self.write_catalog(
            {
                "Appearance.SwitchTheme": "switch app theme",
                "GameTheme.Switch.Tip": "switch game theme",
            },
            {
                "SwitchTheme": "Appearance.SwitchTheme",
                "SwitchThemeTip": "GameTheme.Switch.Tip",
            },
        )
        self.assertEqual([], checker.check())


if __name__ == "__main__":
    unittest.main()
