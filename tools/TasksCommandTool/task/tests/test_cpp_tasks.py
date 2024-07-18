import json
import math
from pathlib import Path
import os
import ast

from .utils import TaskTest
from ..TasksCli import json_path
from ..Task import Task, set_base_task_warning
from ..debug import disable_tracing, enable_tracing

_CPP_TASKS_FOLDER_PATH = os.path.dirname(__file__) + "/cpp_tasks"
_CPP_TASKS: dict[str, dict] = {}


class CPPTaskTests(TaskTest):

    def load_cpp_tasks(self):
        for filename in Path(_CPP_TASKS_FOLDER_PATH).glob("*.json"):
            with open(filename, 'r', encoding='utf-8') as f:
                task = json.load(f)
                task_name = filename.stem
                _CPP_TASKS[task_name] = task

    def load_tasks(self):
        with open(json_path, 'r', encoding='utf-8') as f:
            tasks = json.load(f)
            for name, task_dict in tasks.items():
                try:
                    Task(name, task_dict).define()
                except Exception as e:
                    print(f"Error loading task {name}: {e}")

    def assertTaskEqual(self, actual, expected, task_name=""):
        actual = actual.to_task_dict()
        for key in expected:
            # 目前分支上的cache默认值和主分支不一样，所以这里不比较
            if key == "cache":
                continue
            # 将2147483647视作math.inf
            if key == "maxTimes" and (expected[key] == 2147483647 and actual[key] == math.inf):
                continue
            if isinstance(expected[key], list) and not isinstance(actual[key], list) and len(expected[key]) == 1:
                self.assertEqual(expected[key][0], actual[key], f"Task: {task_name}, Key: {key}")
                continue

            self.assertEqual(expected[key], actual[key], f"Task: {task_name}, Key: {key}")

    def test_cpp_tasks(self):
        disable_tracing()
        set_base_task_warning(False)
        self.load_tasks()
        self.load_cpp_tasks()
        for task_name, task_dict in _CPP_TASKS.items():
            with self.subTest(task_name=task_name):
                self.assertTaskEqual(Task.get(task_name).interpret(), task_dict, task_name)
        enable_tracing()
