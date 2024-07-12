import cmd
import json
import os

from .Task import Task

project_root_path = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir, os.pardir, os.pardir))
json_path = os.path.join(project_root_path, 'resource', 'tasks.json')


class TasksCommandTool(cmd.Cmd):
    prompt = 'tasks> '

    def do_load(self, arg):
        # load json file
        print(f"Loading tasks from {json_path}")
        with open(json_path, 'r', encoding='utf-8') as f:
            Task.load_tasks(json.load(f))


if __name__ == '__main__':
    TasksCommandTool().cmdloop()
