---
order: 3
icon: material-symbols:summarize
---

# 使用

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

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

## 初始化配置

一旦完成了 MaaCore 的安装，通常情况下，你无需额外配置就可以直接运行任务。默认配置可能不适用于所有用户，因此你可以通过以下命令来初始化配置：

```bash
maa init
```

通过这个命令，你可以交互式地配置 [MaaCore 的相关配置][config-core]。

## 运行任务

完成 MaaCore 的安装和配置后，你可以运行任务了。maa-cli 支持两种类型的任务：预定义任务和自定义任务。

### 预定义任务

对于常见任务，maa-cli 提供了一些预定义的任务：

- `maa startup [client]`: 启动游戏并进入主界面，`[client]` 是客户端类型，如果留空则不会启动游戏客户端。
- `maa closedown`: 关闭游戏客户端；
- `maa fight [stage]`: 运行战斗任务，`[stage]` 是关卡名称，例如 `1-7`；留空选择上次或者当前关卡；
- `maa copilot <maa_uri>`: 运行自动战斗任务，其中 `<maa_uri>` 是作业的 URI，其可以是 `maa://1234` 或者本地文件路径 `./1234.json`；
- `maa roguelike [theme]`: 自动集成战略，`[theme]` 是集成战略的主题，可选值为 `Phantom`，`Mizuki` 以及 `Sami`；

上述任务接受一些参数，你可以通过 `maa <task> --help` 来查看具体的参数。

对于官服玩家，如果你想要打开游戏，使用 3 个理智药刷 BB-7，然后关闭游戏，你可以运行以下命令：

```bash
maa startup Official && maa fight BB-7 -m 3 && maa closedown
```

### 自定义任务

由于MAA支持的任务繁多，maa-cli无法提供所有任务的预定义选项。除此之外，你可能需要像上述的例子一样运行多个任务。为了解决这个问题，maa-cli提供了自定义任务的功能。自定义任务能够组合不同的任务，并且更精细地控制每个任务的参数以及执行顺序。此外，自定义任务支持条件判断，可以根据条件来决定是否执行某个任务，或者以何种参数执行某个任务。这可以用于自动化你的日常任务。自定义任务通过配置文件定义，具体配置文件的位置和编写方式请参考 [自定义任务文档][custom-task]。在编写好配置文件后，你可以通过 `maa run <task>` 来运行自定义任务，这里的 `<task>` 是一个自定义任务文件名，不包括扩展名。

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

### 其他子命令

除了上述的命令外，maa-cli 还提供了其他一些子命令：

- `maa list`: 列出所有可用的任务；
- `maa dir <dir>`: 获取特定目录的路径，比如 `maa dir config` 可以用来获取配置目录的路径;
- `maa version`: 获取 `maa-cli` 以及 `MaaCore` 的版本信息；
- `maa convert <input> [output]`: 将 `JSON`，`YAML` 或者 `TOML` 格式的文件转换为其他格式;
- `maa complete <shell>`: 生成自动补全脚本;
- `maa activity [client]`: 获取游戏的当前活动信息，`client` 是客户端类型，默认为 `Official`。
- `maa cleanup`: 清除 `maa-cli` 和 `MaaCore` 的缓存。
- `maa import <file> [-t <type>]:` 导入配置文件，`file` 是配置文件的路径。`-t` 选项可以指定配置文件的类型，如 `cli`, `profile`, `infrast` 等。

更多命令的使用方法可以通过 `maa help` 查看，具体命令的使用方法可以 通过 `maa help <command>` 查看。

[config-core]: config.md#maacore-相关配置
[custom-task]: config.md#自定义任务
