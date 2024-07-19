from task.TasksCli import TasksCommandTool
import argparse


def main():
    parser = argparse.ArgumentParser(description='Command line tool for tasks.')
    parser.add_argument('--no-default-load', action='store_true', help='Do not load tasks from the default json file.')
    parser.add_argument('--load', help='Load tasks from a json file.')
    parser.add_argument('--headless', action='store_true', help='Run in headless mode.')
    args = parser.parse_args()
    cli = TasksCommandTool()
    if args.headless:
        cli.disable_gui()
    # 默认加载
    if not args.no_default_load:
        cli.do_load(args.load if args.load else None)
    try:
        cli.cmdloop()
    except KeyboardInterrupt:
        print('\n')
        print("Exiting...")
        return


if __name__ == '__main__':
    main()
