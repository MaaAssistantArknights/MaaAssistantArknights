from .utils import TaskTest, test_pipeline_a, test_pipeline_b, test_virtual_task, test_match_template_a
from ..Task import Task, _ALL_TASKS


class TemplateTaskTest(TaskTest):

    def test_template_task_inheritance(self):
        Task("A", test_pipeline_a).define()
        task = Task._build_template_task("B@A")
        self.assertTaskEqual(task, {
            "sub": ["B@A_sub", "B@A_sub2"],
            "next": ["B@A_next"],
            "onErrorNext": ["B@A_onErrorNext"],
            "exceededNext": ["B@A_exceededNext"],
            "reduceOtherTimes": ["B@A_reduceOtherTimes"],
        })

    def test_nested_template_task_inheritance(self):
        Task("A", test_pipeline_a).define()
        Task("B", test_pipeline_b).define()
        task = Task._build_template_task("C@B@A")
        self.assertTaskEqual(task, {
            "sub": ["C@B@A_sub", "C@B@A_sub2"],
            "next": ["C@B@A_next"],
            "onErrorNext": ["C@B@A_onErrorNext"],
            "exceededNext": ["C@B@A_exceededNext"],
            "reduceOtherTimes": ["C@B@A_reduceOtherTimes"],
        })

    def test_template_task_with_override(self):
        Task("A", test_pipeline_a).define()
        Task("B", test_pipeline_b).define()
        Task("B@A", {
            "sub": ["no_new_sub", "no_new_sub2"],
            "next": ["no_new_next"],
            "onErrorNext": ["no_new_onErrorNext"],
            "exceededNext": ["no_new_exceededNext"],
            "reduceOtherTimes": ["no_new_reduceOtherTimes"],
        }).define()
        task = Task.get("C@B@A")
        self.assertTaskEqual(task, {
            "sub": ["C@no_new_sub", "C@no_new_sub2"],
            "next": ["C@no_new_next"],
            "onErrorNext": ["C@no_new_onErrorNext"],
            "exceededNext": ["C@no_new_exceededNext"],
            "reduceOtherTimes": ["C@no_new_reduceOtherTimes"],
        })

    def test_virtual_task(self):
        Task("virtual", test_virtual_task).define()
        task = Task._build_template_task("B@virtual")
        self.assertTaskEqual(task, {
            "sub": ["B#virtual_sub", "B#virtual_sub2"],
            "next": ["B#virtual_next"],
            "onErrorNext": ["B#virtual_onErrorNext"],
            "exceededNext": ["B#virtual_exceededNext"],
        })

    def test_algorithm_override(self):
        Task("A", {**test_pipeline_a, **test_match_template_a}).define()
        Task("B@A", {
            "algorithm": "OcrDetect",
        }).define()
        task = Task.get("B@A")
        self.assertTaskEqual(task, {
            "sub": ["B@A_sub", "B@A_sub2"],
            "next": ["B@A_next"],
            "onErrorNext": ["B@A_onErrorNext"],
            "exceededNext": ["B@A_exceededNext"],
            "reduceOtherTimes": ["B@A_reduceOtherTimes"],
            "algorithm": "OcrDetect",
        })
