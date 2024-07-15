import math

from .utils import test_info_a, test_pipeline_a, test_pipeline_b, test_match_template_a, TaskTest
from ..Task import Task


class BaseTaskTest(TaskTest):

    def test_base_task_override(self):
        Task("A", {**test_pipeline_a, **test_match_template_a}).define()
        Task("B@A", {
            "baseTask": "C",
            "algorithm": "OcrDetect",
        }).define()
        Task("C", test_info_a).define()
        task = Task._build_base_task(Task.get("B@A"))
        self.assertTaskEqual(task, {
            'action': 'ClickSelf',
            'algorithm': 'OcrDetect',
            'cache': False,
            'maxTimes': 1,
            'postDelay': 0,
            'preDelay': 0,
            'subErrorIgnored': False
        })

    def test_base_task_1(self):
        Task("A", test_pipeline_a).define()
        Task("B", {
            **test_pipeline_b,
            "baseTask": "A"
        }).define()
        task = Task.get("B")
        self.assertTaskEqual(task, {
            "sub": ["B_sub", "B_sub2"],
            "next": ["B_next"],
            "onErrorNext": ["B_onErrorNext"],
            "exceededNext": ["B_exceededNext"],
            "reduceOtherTimes": ["B_reduceOtherTimes"],
        })

    def test_base_task_2(self):
        Task("A", test_pipeline_a).define()
        Task("B", {
            **test_pipeline_b,
            "baseTask": "C"
        }).define()
        Task("C", test_info_a).define()
        task = Task.get("B")
        self.assertTaskEqual(task, {
            "sub": ["B_sub", "B_sub2"],
            "next": ["B_next"],
            "onErrorNext": ["B_onErrorNext"],
            "exceededNext": ["B_exceededNext"],
            "reduceOtherTimes": ["B_reduceOtherTimes"],
            "algorithm": "MatchTemplate",
            "template": "C.png",
        })
