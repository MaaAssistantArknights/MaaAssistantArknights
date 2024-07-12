import unittest

from ..Task import Task, _ALL_TASKS
from .utils import test_info_a, test_pipeline_a, test_pipeline_b, test_virtual_task, test_match_template_a


class BaseTaskTest(unittest.TestCase):
    def tearDown(self):
        _ALL_TASKS.clear()

    def test_base_task_override(self):
        Task("A", {**test_pipeline_a, **test_match_template_a})
        Task("B@A", {
            "baseTask": "C",
            "algorithm": "OcrDetect",
        })
        Task("C", test_info_a)
        task = Task._build_base_task(Task.get("B@A"))
        self.assertEqual(task.task_dict, {
            'action': 'ClickSelf',
            'algorithm': 'OcrDetect',
            'cache': False,
            'maxTimes': 1,
            'postDelay': 0,
            'preDelay': 0,
            'rectMove': [],
            'retryTimes': 0,
            'roi': [],
            'specialParams': [],
            'specificRect': [100, 100, 50, 50],
            'subErrorIgnored': False
        })

    def test_base_task_1(self):
        Task("A", test_pipeline_a)
        Task("B", {
            **test_pipeline_b,
            "baseTask": "A"
        })
        task = Task._build_base_task(Task.get("B"))
        self.assertEqual(task.task_dict, {
            "sub": ["B_sub", "B_sub2"],
            "next": ["B_next"],
            "onErrorNext": ["B_onErrorNext"],
            "exceededNext": ["B_exceededNext"],
            "reduceOtherTimes": ["B_reduceOtherTimes"],
        })

    def test_base_task_2(self):
        Task("A", test_pipeline_a)
        Task("B", {
            **test_pipeline_b,
            "baseTask": "C"
        })
        Task("C", test_info_a)
        task = Task._build_base_task(Task.get("B"))
        self.assertEqual(task.task_dict, {
            "sub": ["B_sub", "B_sub2"],
            "next": ["B_next"],
            "onErrorNext": ["B_onErrorNext"],
            "exceededNext": ["B_exceededNext"],
            "reduceOtherTimes": ["B_reduceOtherTimes"],
            "algorithm": "MatchTemplate",
            # "template": "B.png",
            "action": "ClickSelf",
            "subErrorIgnored": False,
            "maxTimes": 1,
            "preDelay": 0,
            "postDelay": 0,
            "retryTimes": 0,
            "roi": [],
            "rectMove": [],
            "cache": False,
            "specificRect": [100, 100, 50, 50],
            "specialParams": [],
        })
        self.assertEqual(task.template, "B.png")
