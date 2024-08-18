---
order: 2
icon: material-symbols:summarize
---

# Usage

maa-cli is a command-line interface for MaaCore that automates tasks in the game Arknights. Additionally, maa-cli can manage MaaCore.

## Manage MaaCore

maa-cli can install and update MaaCore and resources, just run the following commands:

```bash
maa install # Install MaaCore and resources
maa update # Update MaaCore and resources
```

## Update maa-cli itself

maa-cli can update itself, just run the following command:

```bash
maa self update
```

**Note**: Users who install maa-cli via a package manager should use the package manager to update maa-cli, this command is invalid for these users.

## Initialize Configuration

Once MaaCore is installed, you can run tasks directly without additional configuration. The default configuration may not be suitable for all users. Therefore, you can initialize the configuration by running the following command:

```bash
maa init
```

With this command, you can configure [the relevant configurations of MaaCore][config-core] interactively.

## Run Tasks

After installing and configuring MaaCore, you can run tasks. maa-cli supports two types of tasks: predefined tasks and custom tasks.

### Predefined tasks

- `maa startup [client]`: start the game client and enter the main screen, the `client` is the client type of game, leave it empty to don't start the game;
- `maa closedown [client]`: close the game client, the `client` is the client type of game, default is `Official`;
- `maa fight [stage]`: run a "fight" task, the `stage` is the stage to fight, like `1-7`, `CE-6`, etc.; if not given, the user will be prompted to input one;
- `maa copilot <maa_uri>`: run a "copilot" task, the `maa_uri` is the URI of a copilot task; it can be `maa://1234` or local file path;
- `maa roguelike [theme]`: run a "roguelike" task, the `theme` is the theme of roguelike, and available themes are `Phantom`, `Mizuki`, `Sami` and `Sarkaz`.

The above tasks accept some parameters, you can view the specific parameters by `maa <task> --help`.

For example, if you want to open the game, use 3 sanity medicines to farm BB-7, and then close the game, you can run the following command:

```bash
maa startup YoStarEN && maa fight BB-7 -m 3 && maa closedown
```

### Custom Tasks

Due to the multitude of tasks supported by MAA, maa-cli cannot provide predefined options for all tasks. Additionally, you may need to run multiple tasks as shown in the example above. To address this issue, maa-cli offers custom task functionality. Custom tasks allow for the combination of different tasks and provide finer control over the parameters of each task as well as the execution order. Furthermore, custom tasks support conditional statements, enabling you to decide whether to execute a task based on certain conditions or to execute a task with specific parameters. This can be used to automate your daily tasks. A custom task is defined in a configuration file. The location and format of the configuration file are described in the [Custom Task Document][custom-task]. After defining the configuration file, you can run the custom task by `maa run <task>`, where `<task>` is the name of the custom task, excluding the extension.

### Task Summary

maa-cli will output a summary of the task after the task is terminated, including the running time of each subtask (start time, end time, running time). For some tasks, it will also output a summary of the task results:

- `fight` task: stage name, times, sanity cost, and drop statistics;
- `infrast`: operators stationed in each facility, for the factory and trading post, it also includes the type of product;
- `recruit`: tags, star ratings, and status of each recruitment, as well as the total number of recruitments;
- `roguelike`: exploration times, investment times.

If you don't want the task summary, you can turn it off by `--no-summary`.

### Loggings

maa-cli will output logs, the log output levels from low to high are `Error`, `Warn`, `Info`, `Debug`, and `Trace`. The default log output level is `Warn`. The log level can be set by the `MAA_LOG` environment variable, for example, `MAA_LOG=debug`. You can also increase or decrease the log output level by `-v` or `-q`.

maa-cli will output logs to stderr by default. The `--log-file` option can output logs to a file, the logs are saved in `$(maa dir log)/YYYY/MM/DD/HH:MM:SS.log`, where `$(maa dir log)` is the log directory, you can get it by `maa dir log`. You can also specify the log file path by `--log-file=path/to/log`.

By default, all output logs will include a timestamp and a log-level prefix. You can change this behavior by the `MAA_LOG_PREFIX` environment variable. When set to `Always`, the prefix will always be included, when set to `Auto`, the prefix will be included when writing to the log file, and not included when writing to stderr, and when set to `Never`, the prefix will not be included even when writing to the log file.

### Other subcommands

Except for the above subcommands, maa-cli also provides other subcommands:

- `maa list`: list all available tasks;
- `maa dir <dir>`: get the path of a specific directory, for example, `maa dir config` can be used to get the path of the configuration directory;
- `maa version`: get the version information of `maa-cli` and `MaaCore`;
- `maa convert <input> [output]`: convert a file in `JSON`, `YAML`, or `TOML` format to another format;
- `maa complete <shell>`: generate an auto-completion script;
- `maa activity [client]`: get the current activity information of the game, the `client` is the client type, default is `Official`.
- `maa cleanup`: clean up the cache of `maa-cli` and `MaaCore`.
- `maa import <file> [-t <type>]`: import a configuration file, the `file` is the path of the configuration file. The `-t` option can specify the type of the configuration file, such as `cli`, `profile`, `infrast`, etc.

More command usage can be viewed by `maa help`, and the usage of specific commands can be viewed by `maa help <command>`.

[config-core]: config.md#maacore-related-configurations
[custom-task]: config.md#custom-tasks
