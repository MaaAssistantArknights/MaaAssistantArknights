from .utils import TaskTest
from ..Task import Task


class TaskLoadTest(TaskTest):
    def test_specialParams(self):
        Task("test", {
            "specialParams": [
                200,
                1,
                2,
                0
            ]
        })
