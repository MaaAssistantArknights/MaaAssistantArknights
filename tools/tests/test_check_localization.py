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

    def write_catalog(self, keys: dict[str, str]) -> None:
        source = HEADER
        for key, value in sorted(keys.items()):
            source += f'    <system:String x:Key="{key}">{value}</system:String>\n'
        source += "</ResourceDictionary>\n"
        for lang in checker.LANGS:
            (self.catalog / f"{lang}.xaml").write_text(source, encoding="utf-8")
    def test_reports_every_static_reference_breakage(self) -> None:
        self.write_catalog({"Common.One": "one"})
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

    def test_missing_old_key_fails_without_a_compatibility_map(self) -> None:
        self.write_catalog({"TaskQueue.Fight": "fight"})
        (self.gui / "OldReference.cs").write_text(
            'LocalizationHelper.GetString("Fight");\n', encoding="utf-8"
        )
        messages = checker.check()
        self.assertEqual(1, len(messages), messages)
        self.assertIn("missing localized call key Fight", messages[0])

    def test_missing_canonical_key_in_dynamic_selection_fails(self) -> None:
        self.write_catalog({"TaskQueue.Fight": "fight"})
        (self.gui / "SelectedKey.cs").write_text(
            'var key = useFight ? "TaskQueue.Fight" : "TaskQueue.Missing";\n'
            'LocalizationHelper.GetString(key);\n',
            encoding="utf-8",
        )
        messages = checker.check()
        self.assertEqual(1, len(messages), messages)
        self.assertIn("TaskQueue.Missing", messages[0])

    def test_localized_list_rejects_old_key_but_allows_plain_labels(self) -> None:
        self.write_catalog({"TaskQueue.Fight": "fight"})
        (self.gui / "Items.cs").write_text(
            'LocalizedObservableList<int> Items = new((0, "TaskQueue.Fight"), '
            '(1, "OldKey"), (2, "AUTO"), (3, "10"));\n',
            encoding="utf-8",
        )
        messages = checker.check()
        self.assertEqual(1, len(messages), messages)
        self.assertIn("OldKey", messages[0])

    def test_localized_list_property_factory_and_source_array_are_checked(self) -> None:
        self.write_catalog({"TaskQueue.Fight": "fight"})
        (self.gui / "Items.cs").write_text(
            'LocalizedObservableList<int> Items { get; } = new((0, "OldProperty"));\n'
            'LocalizedObservableList<int> Make() => new((0, "OldFactory"));\n'
            '(string Value, string LocalizationKey)[] Entries = [("protocol", "OldArray")];\n',
            encoding="utf-8",
        )
        messages = checker.check()
        self.assertEqual(3, len(messages), messages)
        for key in ("OldProperty", "OldFactory", "OldArray"):
            self.assertTrue(any(key in message for message in messages), messages)

    def test_task_control_dynamic_literals_and_lists_obey_ownership(self) -> None:
        self.write_catalog({"Mall.Good": "good", "Roguelike.Wrong": "wrong"})
        task_dir = self.gui / "ViewModels/UserControl/TaskQueue"
        task_dir.mkdir(parents=True)
        (task_dir / "MallTaskUserControlModel.cs").write_text(
            'LocalizedObservableList<int> Items = new((0, "Roguelike.Wrong"));\n'
            'var key = condition ? "Roguelike.Wrong" : "Mall.Good";\n'
            'LocalizationHelper.GetString(key);\n',
            encoding="utf-8",
        )
        messages = checker.check()
        self.assertEqual(2, len(messages), messages)
        self.assertTrue(all("Roguelike.Wrong" in message for message in messages))

    def test_task_controls_can_only_reference_whitelisted_domains(self) -> None:
        self.write_catalog(
            {
                "Common.Cancel": "cancel",
                "Combat.UseFormation": "formation",
                "Mall.Good": "shop task option",
                "Roguelike.Wrong": "other task option",
            }
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
        self.write_catalog({"Mall.Good": "shop task option"})
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

    def test_task_control_wpf_resource_calls_follow_the_same_policy(self) -> None:
        self.write_catalog(
            {
                "Mall.Good": "shop task option",
                "Roguelike.Wrong": "other task option",
            }
        )
        task_views = self.gui / "Views/UserControl/TaskQueue"
        task_views.mkdir(parents=True)
        (task_views / "MallTaskUserControl.xaml.cs").write_text(
            'FindResource("Mall.Good");\n'
            'FindResource("Roguelike.Wrong");\n'
            'Application.Current.Resources["Roguelike.Wrong"];\n'
            'SetResourceReference(Property, "Roguelike.Wrong");\n',
            encoding="utf-8",
        )
        (task_views / "NewFeatureTaskUserControl.xaml.cs").write_text(
            'FindResource("Mall.Good");\n', encoding="utf-8"
        )
        messages = checker.check()
        self.assertEqual(4, len(messages), messages)
        self.assertEqual(
            3,
            sum("outside the MallTaskUserControl" in message for message in messages),
        )
        self.assertEqual(
            1, sum("no localization ownership policy" in message for message in messages)
        )

    def test_only_task_control_directories_are_auto_enrolled(self) -> None:
        self.write_catalog({"Mall.Good": "shop task option"})
        task_models = self.gui / "ViewModels/UserControl/TaskQueue"
        task_models.mkdir(parents=True)
        (task_models / "NewFeatureTaskUserControlModel.cs").write_text(
            'LocalizationHelper.GetString("Mall.Good");\n', encoding="utf-8"
        )
        unrelated = self.gui / "Services/TaskQueue"
        unrelated.mkdir(parents=True)
        (unrelated / "NewFeatureTaskUserControl.cs").write_text(
            'LocalizationHelper.GetString("Mall.Good");\n', encoding="utf-8"
        )
        messages = checker.check()
        self.assertEqual(1, len(messages), messages)
        self.assertIn("NewFeatureTaskUserControlModel.cs", messages[0])

if __name__ == "__main__":
    unittest.main()
