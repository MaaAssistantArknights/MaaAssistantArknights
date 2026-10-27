"""Offline regressions for operator development green-ticket purchases.

Set MAA_CORE_LIBRARY to a Debug MaaCore library to also validate resources and
the public OperProgress parameter API. No device is connected and no task runs.
Set MAA_TEST_CLIENT to an overseas resource name to validate that overlay.
"""

import ctypes
import json
import os
from pathlib import Path
import tempfile
import unittest
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]


class ResourceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tasks = {}
        for path in (ROOT / "resource/tasks").rglob("*.json"):
            cls.tasks.update(json.loads(path.read_text(encoding="utf-8")))

    def resolved(self, name):
        task = self.tasks[name]
        base = self.resolved(task["baseTask"]) if "baseTask" in task else {}
        return {**base, **task}

    def test_second_floor_does_not_start_the_normal_shopping_list(self):
        entry = self.resolved("OperProgress@GreenTicketChooseTicketType")
        self.assertEqual(entry["text"], ["资质凭证区"])
        self.assertEqual(entry["next"], ["OperProgress@GreenTicketSecondFloor"])
        floor = self.resolved(entry["next"][0])
        self.assertEqual(floor["text"], ["2"])
        self.assertEqual(floor["next"], [])

    def test_purchase_stops_after_the_confirmed_receipt(self):
        purchase = self.resolved("OperProgress@GreenTicketPurchase")
        self.assertEqual(purchase["next"], ["OperProgress@GreenTicketPurchasedConfirm"])
        receipt = self.resolved(purchase["next"][0])
        self.assertEqual(receipt["template"], "CreditShop-Bought.png")
        self.assertEqual(receipt["next"], [])
        self.assertEqual(purchase["maxTimes"], 1)

    def test_receipt_retries_cannot_restart_payment(self):
        payment = "OperProgress@GreenTicketPurchase"
        self.assertEqual(self.resolved(payment)["maxTimes"], 1)
        pending = ["OperProgress@GreenTicketPurchasedConfirm"]
        visited = set()
        while pending:
            name = pending.pop()
            self.assertNotEqual(name, payment)
            if name in visited:
                continue
            visited.add(name)
            task = self.resolved(name)
            for field in ["next", "onErrorNext", "exceededNext", "sub"]:
                pending.extend(task.get(field, []))

    def test_quantity_observations_fit_the_screen(self):
        names = [
            name
            for name in self.tasks
            if name.endswith("Quantity")
            and (
                name.startswith("MiniGame@MaterialSynthesis@Ingredient")
                or name.startswith("OperProgress@GreenTicket")
            )
        ]
        self.assertTrue(names)
        for name in names:
            with self.subTest(name=name):
                x, y, width, height = self.resolved(name)["roi"]
                self.assertGreater(width, 0)
                self.assertGreater(height, 0)
                self.assertGreaterEqual(x, 0)
                self.assertGreaterEqual(y, 0)
                self.assertLessEqual(x + width, 1280)
                self.assertLessEqual(y + height, 720)

    def test_localized_setting_and_receipt_are_available(self):
        keys = [
            "TaskQueue.OperProgress.BuyMaterialsWithGreenTickets",
            "TaskQueue.OperProgress.BuyMaterialsWithGreenTicketsTip",
            "TaskQueue.OperProgress.GreenTicketPurchasedLog",
        ]
        key_attribute = "{http://schemas.microsoft.com/winfx/2006/xaml}Key"
        for lang in ["zh-cn", "zh-tw", "en-us", "ja-jp", "ko-kr"]:
            path = ROOT / f"src/MaaWpfGui/Res/Localizations/{lang}.xaml"
            entries = list(ET.parse(path).getroot())
            for key in keys:
                with self.subTest(lang=lang, key=key):
                    matches = [
                        entry for entry in entries if entry.get(key_attribute) == key
                    ]
                    self.assertEqual(len(matches), 1)
                    self.assertTrue(matches[0].text)


@unittest.skipUnless(
    os.environ.get("MAA_CORE_LIBRARY"), "Set MAA_CORE_LIBRARY to a Debug build"
)
class CoreTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.user_dir = tempfile.TemporaryDirectory(prefix="maa-green-ticket-core-")
        cls.core = ctypes.CDLL(os.environ["MAA_CORE_LIBRARY"])
        lib = cls.core
        lib.AsstSetUserDir.argtypes, lib.AsstSetUserDir.restype = (
            [ctypes.c_char_p],
            ctypes.c_bool,
        )
        lib.AsstLoadResource.argtypes, lib.AsstLoadResource.restype = (
            [ctypes.c_char_p],
            ctypes.c_bool,
        )
        lib.AsstCreate.argtypes, lib.AsstCreate.restype = [], ctypes.c_void_p
        lib.AsstDestroy.argtypes, lib.AsstDestroy.restype = [ctypes.c_void_p], None
        lib.AsstAppendTask.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]
        lib.AsstAppendTask.restype = ctypes.c_int
        assert lib.AsstSetUserDir(cls.user_dir.name.encode())
        assert lib.AsstLoadResource(str(ROOT).encode())
        if client := os.environ.get("MAA_TEST_CLIENT"):
            assert lib.AsstLoadResource(str(ROOT / "resource/global" / client).encode())

    def setUp(self):
        self.handle = self.core.AsstCreate()
        self.assertTrue(self.handle)

    def tearDown(self):
        self.core.AsstDestroy(self.handle)

    def append(self, **extra):
        params = {"plans": [{"role": "Caster", "name": "阿米娅", "elite": 2}], **extra}
        return self.core.AsstAppendTask(
            self.handle,
            b"OperProgress",
            json.dumps(params, ensure_ascii=False).encode(),
        )

    def test_existing_plan_without_the_new_option(self):
        self.assertGreater(self.append(), 0)

    def test_boolean_opt_in_and_opt_out(self):
        for value in [True, False]:
            with self.subTest(value=value):
                self.assertGreater(
                    self.append(buy_materials_with_green_tickets=value), 0
                )

    def test_invalid_option_cannot_enable_purchases(self):
        for value in [None, 0, 1, "true", "false", [], {}]:
            with self.subTest(value=value):
                self.assertEqual(self.append(buy_materials_with_green_tickets=value), 0)


if __name__ == "__main__":
    unittest.main()
