from ..Task import Task, InterpretedTask
from .utils import TaskTest


class EvaluateTest(TaskTest):

    def assertInterpretEqual(self, actual: InterpretedTask, expected: dict):
        actual = actual.to_task_dict()
        for key in expected:
            if isinstance(actual[key], list):
                for i, value in enumerate(actual[key]):
                    if isinstance(value, Task):
                        value = value.name
                    self.assertEqual(expected[key][i], value, f"Key: {key}")
            else:
                self.assertEqual(expected[key], actual[key], f"Key: {key}")

    def test_evaluate(self):
        Task("A", {
            "algorithm": "MatchTemplate",
            "action": "ClickSelf",
            "subErrorIgnored": False,
            "maxTimes": 1,
            "preDelay": 0,
            "postDelay": 0,
            "retryTimes": 0,
            "roi": [0, 0, 1280, 720],
            "rectMove": [0, 0, 0, 0],
            "cache": False,
            "specificRect": [100, 100, 50, 50],
            "specialParams": [], "sub": ["A_sub", "A_sub2"],

            "next": ["A_next"],
            "onErrorNext": ["A_onErrorNext"],
            "exceededNext": ["A_exceededNext"],
            "reduceOtherTimes": ["A_reduceOtherTimes"],
        }).define()
        Task("B", {
            "algorithm": "MatchTemplate",
            "action": "ClickSelf",
            "subErrorIgnored": False,
            "maxTimes": 1,
            "preDelay": 0,
            "postDelay": 0,
            "retryTimes": 0,
            "roi": [0, 0, 1280, 720],
            "rectMove": [0, 0, 0, 0],
            "cache": False,
            "specificRect": [100, 100, 50, 50],
            "specialParams": [],

            "sub": ["B_sub", "B_sub2"],
            "next": ["B_next"],
            "onErrorNext": ["B_onErrorNext"],
            "exceededNext": ["B_exceededNext"],
            "reduceOtherTimes": ["B_reduceOtherTimes"],
        }).define()
        print(Task.evaluate("C@B@A"))

    def define_tasks(self):
        Task("A", {"next": ["N1", "N2"]}).define()
        Task("C", {"next": ["B@A#next"]}).define()
        Task("Loading", {"next": ["#self", "#next", "#back"]}).define()
        Task("B", {"next": ["Other", "B@Loading"]}).define()

    def test_2(self):
        self.define_tasks()
        self.assertInterpretEqual(Task.get("C").interpret(), {
            "name": "C",
            "next": ["B@N1", "B@N2"],
        })

    def test_3(self):
        self.define_tasks()
        self.assertInterpretEqual(Task.get("B@Loading").interpret(), {
            "name": "B@Loading",
            "next": ["B@Loading", "Other", "B"],
        })

    def test_4(self):
        self.define_tasks()
        self.assertInterpretEqual(Task.get("Loading").interpret(), {
            "name": "Loading",
            "next": ["Loading"],
        })

    def test_5(self):
        self.define_tasks()
        self.assertTaskEqual(Task.get("B@Loading"), {
            "name": "B@Loading",
            "next": ["B#self", "B#next", "B#back"],
        })

    def test_6(self):
        Task("A", {"next": ["N0"], }).define()
        Task("B", {"next": ["A#next"]}).define()
        Task("C@A", {"next": ["N1"]}).define()
        self.assertInterpretEqual(Task.get("C@B").interpret(), {
            "name": "C@B",
            "next": ["N1"],
        })

    def test_7(self):
        # (ClickCornerAfterPRTS+ClickCorner)*5
        Task("ClickCornerAfterPRTS", {
            "algorithm": "MatchTemplate",
            "action": "ClickSelf",
            "subErrorIgnored": False,
            "maxTimes": 1,
            "preDelay": 0,
            "postDelay": 0,
            "retryTimes": 0,
            "roi": [0, 0, 1280, 720],
            "rectMove": [0, 0, 0, 0],
            "cache": False,
            "specificRect": [100, 100, 50, 50],
            "specialParams": [],
        }).define()
        Task("ClickCorner", {
            "algorithm": "MatchTemplate",
            "action": "ClickSelf",
            "subErrorIgnored": False,
            "maxTimes": 1,
            "preDelay": 0,
            "postDelay": 0,
            "retryTimes": 0,
            "roi": [0, 0, 1280, 720],
            "rectMove": [0, 0, 0, 0],
            "cache": False,
            "specificRect": [100, 100, 50, 50],
            "specialParams": [],
        }).define()
        Task("Test", {"next": ["(ClickCornerAfterPRTS+ClickCorner)*5"]}).define()
        self.assertInterpretEqual(Task.get("Test").interpret(), {
            "name": "Test",
            "next": ["ClickCornerAfterPRTS", "ClickCorner"] * 5,
        })

    def test_8(self):
        # (A+A+B+C)^(A+B+D)
        Task("A", {"next": ["N1"]}).define()
        Task("B", {"next": ["N2"]}).define()
        Task("C", {"next": ["N3"]}).define()
        Task("D", {"next": ["N4"]}).define()
        Task("Test", {"next": ["(A+A+B+C)^(A+B+D)"]}).define()
        self.assertInterpretEqual(Task.get("Test").interpret(), {
            "name": "Test",
            "next": ["C"],
        })
