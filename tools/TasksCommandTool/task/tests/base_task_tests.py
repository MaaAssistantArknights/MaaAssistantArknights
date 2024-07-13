import math

from .utils import test_info_a, test_pipeline_a, test_pipeline_b, test_match_template_a, TaskTest
from ..Task import Task


class BaseTaskTest(TaskTest):

    def test_base_task_override(self):
        Task("A", {**test_pipeline_a, **test_match_template_a})
        Task("B@A", {
            "baseTask": "C",
            "algorithm": "OcrDetect",
        })
        Task("C", test_info_a)
        task = Task._build_base_task(Task.get("B@A"))
        self.assertTaskEqual(task, {
            'action': 'DoNothing',
            'algorithm': 'OcrDetect',
            'cache': False,
            'maxTimes': math.inf,
            'postDelay': 0,
            'preDelay': 0,
            'retryTimes': 0,
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
        self.assertTaskEqual(task, {
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
        self.assertTaskEqual(task, {
            "sub": ["B_sub", "B_sub2"],
            "next": ["B_next"],
            "onErrorNext": ["B_onErrorNext"],
            "exceededNext": ["B_exceededNext"],
            "reduceOtherTimes": ["B_reduceOtherTimes"],
            "algorithm": "MatchTemplate",
            "template": "B.png",
        })
