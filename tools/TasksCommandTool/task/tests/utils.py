import unittest
from ..Task import clear_cache, _ORIGINAL_TASKS
from ..debug import enable_tracing


class TaskTest(unittest.TestCase):
    enable_tracing()

    def tearDown(self):
        clear_cache()
        _ORIGINAL_TASKS.clear()

    def assertTaskEqual(self, actual, expected):
        actual = actual.to_task_dict()
        for key in expected:
            self.assertEqual(expected[key], actual[key], f"Key: {key}")
