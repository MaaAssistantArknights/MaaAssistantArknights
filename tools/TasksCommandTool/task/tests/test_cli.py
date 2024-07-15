from .utils import TaskTest
from ..TasksCli import TasksCommandTool
from ..Task import _ORIGINAL_TASKS


class CliTest(TaskTest):

    def test_cli(self):
        cli = TasksCommandTool()
        cli.do_load("")
        success_count = 0
        fail_count = 0
        fail_tasks = []
        for task in _ORIGINAL_TASKS:
            try:
                cli.do_find(task)
                cli.do_exec(task)
                success_count += 1
            except Exception as e:
                print(f"Failed to execute task {task}: {e}")
                fail_tasks.append(task)
                fail_count += 1
        print(f"Success count: {success_count}, Fail count: {fail_count}")
        print(f"Failed tasks: {fail_tasks}")

    def test_1(self):
        cli = TasksCommandTool()
        cli.do_load("")
        cli.do_find("ClickCornerAfterPRTS")
        cli.do_exec("ClickCornerAfterPRTS")
