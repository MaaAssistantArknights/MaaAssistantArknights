import cmd
import json
import os

from .Task import Task, _ALL_TASKS, _ORIGINAL_TASKS

project_root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))
json_path = os.path.join(project_root_path, 'resource', 'tasks.json')


# noinspection PyMethodMayBeStatic
class TasksCommandTool(cmd.Cmd):
    prompt = 'tasks> '

    def get_task(self, arg):
        task = Task.get(arg)
        if task is None:
            raise ValueError(f"Task {arg} not found.")
        return task

    def do_load(self, arg):
        """Load tasks from a json file."""
        global json_path
        if arg:
            json_path = os.path.abspath(arg)
        print(f"Loading tasks from {json_path}")
        success_count = 0
        fail_count = 0
        with open(json_path, 'r', encoding='utf-8') as f:
            tasks = json.load(f)
            for name, task_dict in tasks.items():
                try:
                    Task(name, task_dict).define()
                    success_count += 1
                except Exception as e:
                    print(f"Error loading task {name}: {e}")
                    fail_count += 1
        print(f"Loaded {success_count} tasks, {fail_count} failed.")

    def do_find(self, arg):
        """Find a task by name."""
        self.get_task(arg).print()

    def complete_find(self, text, line, begidx, endidx):
        return ([name for name in _ALL_TASKS if name.startswith(text)]
                + [name for name in _ORIGINAL_TASKS if name.startswith(text)])

    def do_exec(self, arg):
        """Execute a task by name."""
        self.get_task(arg).interpret().print()

    def do_show(self, arg):
        """Show all tasks."""
        self.get_task(arg).interpret().show()

    def onecmd(self, line):
        try:
            return super().onecmd(line)
        except Exception as e:
            print(e)
