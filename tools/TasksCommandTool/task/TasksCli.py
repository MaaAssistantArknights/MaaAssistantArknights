import cmd
import json
import os

from .Task import Task, _ALL_TASKS

project_root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))
json_path = os.path.join(project_root_path, 'resource', 'tasks.json')


# noinspection PyMethodMayBeStatic
class TasksCommandTool(cmd.Cmd):
    prompt = 'tasks> '

    def do_load(self, arg):
        """Load tasks from a json file."""
        global json_path
        if arg:
            json_path = os.path.abspath(arg)
        print(f"Loading tasks from {json_path}")
        with open(json_path, 'r', encoding='utf-8') as f:
            Task.load_tasks(json.load(f))
        print("Tasks loaded.")

    def do_find(self, arg):
        """Find a task by name."""
        task = Task.get(arg)
        task.print()

    def complete_find(self, text, line, begidx, endidx):
        return [name for name in _ALL_TASKS if name.startswith(text)]

    def do_exec(self, arg):
        """Execute a task by name."""
        task = Task.get(arg)
        task.interpret()

    def do_show(self, arg):
        """Show all tasks."""
        task = Task.get(arg)
        task.show()
