---
order: 3
icon: material-symbols:format_list_bulleted
---

# 使用 maa-cli

maa-cli 主要功能是通过调用 MaaCore，自动化完成明日方舟的游戏任务。此外，为了方便使用，maa-cli 还提供了管理 MaaCore 的功能。

## 管理 MaaCore

maa-cli 可以安装和更新 MaaCore 及资源，只需运行以下命令：

```bash
maa install # 安装 MaaCore 及资源
maa update # 更新 MaaCore 及资源
```

## 更新 maa-cli 自身

maa-cli 可以更新自身，只需运行以下命令：

```bash
maa self update
```

**注意**：使用包管理器安装 maa-cli 的用户请使用包管理器更新 maa-cli，此命令在这些用户中无效。

## 运行任务

maa-cli 通过调用 MaaCore 来完成任务，这些包括一些预定义的任务和用户自定义的任务。

### 预定义任务

对于常见任务，maa-cli 提供了一些预定义的任务：

- `maa startup [client]`: 启动游戏并进入主界面，`[client]` 是客户端类型，如果留空则不会启动游戏客户端。
- `maa closedown`: 关闭游戏客户端；
- `maa fight [stage]`: 运行战斗任务，`[stage]` 是关卡名称，例如 `1-7`；留空选择上次或者当前关卡；
- `maa copilot <maa_uri>`: 运行自动战斗任务，其中 `<maa_uri>` 是作业的 URI，其可以是 `maa://1234` 或者本地文件路径 `./1234.json`；
- `maa roguelike [theme]`: 自动集成战略，`[theme]` 是集成战略的主题，可选值为 `Phantom`，`Mizuki` 以及 `Sami`；

### 用户自定义任务

你可以通过 `maa run <task>` 来运行自定义任务。这里的 `<task>` 是一个自定义任务名，
自定义任务通过配置文件定义，具体配置文件的位置和编写方式请参考 [自定义任务文档][custom-task].
在定义好自定义任务后，你可以通过 `maa list` 来列出所有可用的任务。

### 任务总结

不管是预定义任务还是自定义任务，maa-cli 都会在任务运行结束后输出任务的总结信息，
其包括每个子任务的运行时间（开始时间、结束时间、运行时长）。对于部份任务，还会输出任务的结果汇总：

- `fight` 任务: 关卡名称，次数，消耗理智药个数以及掉落统计；
- `infrast`: 各设施进驻的干员，对于制造站和贸易站，还会包括产物类型；
- `recruit`: 每次公招的 tag ，星级以及状态，以及总共的招募次数；
- `roguelike`: 探索次数，投资次数。

如果你不想要任务总结，可以通过 `--no-summary` 参数来关闭。

### 任务日志

maa-cli 会输出日志，日志输出级别从低到高分别为 `Error`，`Warn`，`Info`，`Debug` 和 `Trace`。默认的日志输出级别为 `Warn`。日志级别可以通过 `MAA_LOG` 环境变量来设置，例如 `MAA_LOG=debug`。你也可以通过 `-v` 或者 `-q` 来增加或者减少日志输出级别。

maa-cli 默认会向标准误 (stderr) 输出日志。`--log-file` 选项可以将日志输出到文件中，日志保存在 `$(maa dir log)/YYYY/MM/DD/HH:MM:SS.log` 中，其中 `$(maa dir log)` 是日志目录，你可以通过 `maa dir log` 获取。你也可以通过 `--log-file=path/to/log` 来指定日志文件的路径。

默认情况下，所有输出的日志会包含时间戳和日志级别的前缀。你可以通过环境变量 `MAA_LOG_PREFIX` 来改变这个行为。设置为 `Always` 时，总是会包含前缀，设置为 `Auto` 时输出到日志文件时会包含前缀，而输出到 stderr 时不会包含前缀，而设置为 `Never` 时即使是写入日志文件时也不会包含前缀。

[custom-task]: cli-config.md#自定义任务

<!-- markdownlint-disable-file MD013 -->
