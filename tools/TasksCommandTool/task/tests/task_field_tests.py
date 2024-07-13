from .utils import TaskTest
from ..TaskField import TaskFieldEnum


class TaskFieldTests(TaskTest):
    def test_base_task_field(self):
        field = TaskFieldEnum.BASE_TASK.value
        self.assertEqual(field.field_name, "baseTask")
        self.assertEqual(field.python_field_name, "base_task")
        self.assertEqual(field.field_type, str)
        self.assertEqual(field.field_default, None)
        self.assertTrue(field.is_valid_with(None))
        self.assertFalse(field.is_valid_with(1))
        self.assertTrue(field.is_valid_with("test"))
        self.assertEqual(field.valid_for_algorithm, None)

    def test_roi_field(self):
        field = TaskFieldEnum.ROI.value
        self.assertEqual(field.field_name, "roi")
        self.assertEqual(field.python_field_name, "roi")
        self.assertFalse(field.is_valid_with([]))
        self.assertFalse(field.is_valid_with(1))
        self.assertTrue(field.is_valid_with([1, 2, 3, 4]))
        self.assertTrue(field.is_valid_with(None))
        self.assertEqual(field.valid_for_algorithm, None)
