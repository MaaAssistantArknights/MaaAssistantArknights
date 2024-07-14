from ..Task import Task
from .utils import TaskTest, test_info_a, test_pipeline_a, test_match_template_a, test_pipeline_b


class EvaluateTest(TaskTest):

    def test_evaluate(self):
        Task("A", {**test_info_a, **test_pipeline_a})
        Task("B", {**test_info_a, **test_pipeline_b})
        print(Task.evaluate("C@B@A"))
