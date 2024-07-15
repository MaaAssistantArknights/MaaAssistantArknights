from task.TasksCli import TasksCommandTool

if __name__ == '__main__':
    try:
        TasksCommandTool().cmdloop()
    except KeyboardInterrupt:
        print()
        print("Bye!")
