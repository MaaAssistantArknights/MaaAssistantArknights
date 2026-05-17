---
order: 2
icon: material-symbols:summarize
---

# Usage

maa-cli's main functionality is to automate Arknights game tasks by calling MaaCore. Additionally, for convenience, maa-cli also provides functions to manage MaaCore.

::: tip

For users who installed maa-cli via the Windows Package Manager (winget) , `maa` in the following commands needs to be replaced with `maa-cli` .

:::

## Manage MaaCore

maa-cli can install and update MaaCore and resources by running the following commands:

```bash
maa install # Install MaaCore and resources
maa update # Update MaaCore and resources
```

## Update maa-cli Itself

maa-cli can update itself by running the following command:

```bash
maa self update
```

::: warning
Users who installed maa-cli via a package manager should use the package manager to update maa-cli. This command will not work for those users.
:::

## Initialize Configuration

Once MaaCore is installed, you can typically run tasks directly without additional configuration. However, the default configuration may not be suitable for all users, so you can initialize the configuration with:

```bash
maa init
```

With this command, you can interactively configure [MaaCore-related settings][config-core].

## Run Tasks

After installing and configuring MaaCore, you can run tasks. maa-cli supports two types of tasks: predefined tasks and custom tasks.

### Predefined Tasks

For common tasks, maa-cli provides several predefined options:

::: collapse

- :+ Start the game and enter the main interface

  ```shell
  maa startup [client]
  ```
  
  `[client]` is the client type; leave empty to not start any game client.

- Close the game client
  
  ```shell
  maa closedown [client]
  ```
  `[client]` is the client type, defaulting to `Official`.

- Run a combat task

  ```shell
  maa fight [stage]
  ```

  `[stage]` is the stage name like `1-7`; leave empty to select the last or current stage.

- Auto-run copilot tasks
  ```shell
  maa copilot <maa_uri>
  ```
  `<maa_uri>` is the task URI, multiple URIs will execute in sequence. `maa_uri` can be `maa://1234` or a local file path like `./1234.json`.

- Auto-run Stationary Security Service
  ```shell
  maa sscopilot <maa_uri>
  ```

  `<maa_uri>` is the task URI.

- Auto-run Integrated Strategy
  ```shell
  maa roguelike <theme>
  ```

  `<theme>` is the theme, with options including `Phantom`, `Mizuki`, `Sami`, `Sarkaz`, and `JieGarden`.

- Auto-run Reclamation Algorithm
  ```shell
  maa reclamation <theme>
  ```

  `<theme>` is the theme, currently only `Tales` is available.

:::

These tasks accept various parameters. You can view the specific parameters with `maa <task> --help`.

For example, if you want to open the game, use 3 sanity potions to farm BB-7, and then close the game, you can run:

```bash
maa startup Official && maa fight BB-7 -m 3 && maa closedown
```

### Custom Tasks

Due to MAA's support for numerous tasks, maa-cli cannot provide predefined options for everything. Additionally, you may need to run multiple tasks as in the example above. To address this, maa-cli offers custom task functionality. Custom tasks can combine different tasks, providing finer control over parameters and execution order. They also support conditional execution based on specific criteria, automating your daily routines. Custom tasks are defined via configuration files—see the [Custom Task Documentation][custom-task] for details on location and format. After creating a configuration file, run your custom task with `maa run <task>`, where `<task>` is the filename without extension.

### Task Summary

Both predefined and custom tasks output summary information upon completion, including each subtask's runtime (start time, end time, duration). For certain tasks, result summaries include:

- `fight` task: Stage name, number of runs, sanity potions used, and drop statistics
- `infrast`: Operators assigned to each facility, including product types for factories and trading posts
- `recruit`: Tags, star ratings, and status for each recruitment, plus total recruitment count
- `roguelike`: Exploration count, investment count

If you don't want task summaries, disable them with the `--no-summary` parameter.

### Task Logging

maa-cli outputs logs with the following levels (low to high): `Error`, `Warn`, `Info`, `Debug`, and `Trace`. The default level is `Warn`. Set the log level via the `MAA_LOG` environment variable (e.g., `MAA_LOG=debug`) or use `-v` to increase and `-q` to decrease the level.

By default, logs go to standard error (stderr). The `--log-file` option can redirect logs to a file at `$(maa dir log)/YYYY/MM/DD/HH:MM:SS.log`, where `$(maa dir log)` is the log directory obtainable via `maa dir log`. You can also specify a custom log path with `--log-file=path/to/log`.

All logs normally include a timestamp and level prefix. The `MAA_LOG_PREFIX` environment variable controls this behavior: `Always` always includes prefixes, `Auto` includes prefixes in log files but not in stderr output, and `Never` omits prefixes even in log files.

### Other Subcommands

Besides the above commands, maa-cli provides additional subcommands:

::: collapse

- List all available tasks

  ```shell
  maa list
  ```

- Get a specific directory path
  ```shell
  maa dir <dir>
  ```
  For example, `maa dir config` gets the path to the configuration directory.

- Get version information

  ```shell
  maa version
  ```

- Convert between file formats

  ```shell
  maa convert <input> [output]
  ```

- Generate auto-completion scripts

  ```shell
  maa complete <shell>
  ```

- Get current in-game activity information

  ```shell
  maa activity [client]
  ```
  `[client]` is the client type, defaulting to `Official`.

- Clean caches
  
  ```shell
  maa cleanup
  ```

- Import a configuration file

  ```shell
  maa import <file> [-t <type>]
  ```
  `file` is the path to the configuration file. `-t` specifies the type (e.g., `cli`, `profile`, `infrast`).

:::

For more command information, use `maa help`. For specific command details, use `maa help <command>`.

[config-core]: config.md#maacore-related-configurations
[custom-task]: config.md#custom-tasks
