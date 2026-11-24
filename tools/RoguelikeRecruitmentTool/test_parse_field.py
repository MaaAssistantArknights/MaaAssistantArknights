import json
import unittest
from typing import Annotated, List
from unittest.mock import patch

from PyQt5.QtCore import Qt
from PyQt5.QtTest import QSignalSpy
from roguelike.recruitment import Configuration, RecruitPriorityOffset
from roguelike_recruitment_tool.common import parse_field
from roguelike_recruitment_tool.models.oper_offset_A_table_model import (
    OperOffsetATableModel,
)


class ParseFieldTests(unittest.TestCase):
    def test_string_list_literals(self):
        field_types = (
            list,
            List,
            Annotated[list[str], "groups"],
            Annotated[List[str], "groups"],
            RecruitPriorityOffset.__annotations__["groups"],
        )
        for field_type in field_types:
            for value, expected in (
                ("['近卫', '先锋']", ["近卫", "先锋"]),
                ('["group", "another group"]', ["group", "another group"]),
                (r"['group\'s name', 'line\nfeed']", ["group's name", "line\nfeed"]),
            ):
                with self.subTest(field_type=field_type, value=value):
                    self.assertEqual(parse_field(value, field_type), expected)

    def test_unconstrained_lists_can_be_empty(self):
        for field_type in (list, List, Annotated[list[str], "groups"]):
            with self.subTest(field_type=field_type):
                self.assertEqual(parse_field("[]", field_type), [])

    def test_group_list_constraints(self):
        field_type = RecruitPriorityOffset.__annotations__["groups"]
        for value in ("[]", "['']", "['   ']", "['group', '']"):
            with self.subTest(value=value):
                self.assertIsNone(parse_field(value, field_type))
        self.assertEqual(
            parse_field("[' 近卫 ', ' 先锋 ']", field_type), ["近卫", "先锋"]
        )

    def test_invalid_lists_are_rejected(self):
        for value in (
            "",
            "[",
            "unknown_group",
            "1",
            "True",
            "None",
            "'group'",
            "('group',)",
            "(1,)",
            "{'group': 1}",
            "{'group'}",
            "[1]",
            "['group', False]",
            "[['group']]",
            "['group'] + ['other']",
        ):
            with self.subTest(value=value):
                self.assertIsNone(parse_field(value, list))

    def test_expressions_are_not_executed(self):
        with patch("os.getcwd", return_value=["group"]) as getcwd:
            result = parse_field("__import__('os').getcwd()", list)
            getcwd.assert_not_called()
            self.assertIsNone(result)

    def test_scalar_fields_keep_existing_behavior(self):
        for value, field_type, expected in (
            ("group", str, "group"),
            ("42", int, 42),
            ("invalid", int, None),
            ("true", bool, True),
            ("0", bool, False),
            ("invalid", bool, None),
            ("42", Annotated[int, "priority"], 42),
        ):
            with self.subTest(value=value, field_type=field_type):
                self.assertEqual(parse_field(value, field_type), expected)


class GroupEditingTests(unittest.TestCase):
    def setUp(self):
        self.offset = RecruitPriorityOffset(groups=["original"], offset=100)
        self.model = OperOffsetATableModel()
        self.model.set_offset_list([self.offset])
        self.index = self.model.index(0, 0)
        self.changed = QSignalSpy(self.model.dataChanged)

    def test_valid_edit_updates_groups_and_display(self):
        self.assertTrue(self.model.setData(self.index, "['近卫', '先锋']"))
        self.assertEqual(self.offset.groups, ["近卫", "先锋"])
        self.assertEqual(
            self.model.data(self.index, Qt.DisplayRole).value(), "近卫, 先锋"
        )
        self.assertEqual(len(self.changed), 1)

    def test_invalid_edit_preserves_groups_and_display(self):
        for value in (
            "[1]",
            "unknown_group",
            "1",
            "['group', False]",
            "[]",
            "['']",
            "['   ']",
        ):
            with self.subTest(value=value):
                self.assertFalse(self.model.setData(self.index, value))
                self.assertEqual(self.offset.groups, ["original"])
                self.assertEqual(
                    self.model.data(self.index, Qt.DisplayRole).value(), "original"
                )
                self.assertEqual(len(self.changed), 0)

    def test_rejected_group_edits_preserve_reloadable_config(self):
        config = Configuration.json2config(
            json.dumps(
                {
                    "theme": "Phantom",
                    "priority": [
                        {
                            "name": "original",
                            "opers": [
                                {
                                    "name": "test operator",
                                    "recruit_priority_offsets": [
                                        {"groups": ["original"], "offset": 100}
                                    ],
                                }
                            ],
                        }
                    ],
                    "team_complete_condition": [],
                }
            )
        )
        self.model.set_offset_list(config.priority[0].opers[0].recruit_priority_offsets)
        for value in ("[]", "['']", "['   ']", "['original', '']"):
            with self.subTest(value=value):
                self.assertFalse(self.model.setData(self.index, value))
                saved = Configuration.config2json(config)
                loaded = Configuration.json2config(saved)
                self.assertEqual(
                    loaded.priority[0].opers[0].recruit_priority_offsets[0].groups,
                    ["original"],
                )


if __name__ == "__main__":
    unittest.main()
