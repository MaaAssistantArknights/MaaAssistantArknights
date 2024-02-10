---
icon: material-symbols:terminal
---

# CLI ユーザーガイド

## 機能紹介

- 定義済みタスクまたはカスタムタスクの実行、例 `maa fight`， `maa run <task>`;
- `maa install` と `maa update` を使用して `MaaCore`と `resources` をインストールおよび更新する；
- `maa self-update` を使用して自身を更新する。

## インストール

### Appimage

CLIはLinuxプラットフォーム上のMAAのデフォルト インタフェースであり、最新のAppimageパッケージを直接ダウンロードしてCLIを使用できます。

### パッケージマネージャー

#### macOS

[Homebrew](https://brew.sh/) を使用してインストールします：

```bash
brew install MaaAssistantArknights/tap/maa-cli
```

#### Linux

ArchLinux ユーザーは [AUR パッケージ](https://aur.archlinux.org/packages/maa-cli/) をインストールできます:

```bash
yay -S maa-cli
```

LinuxBrew ユーザーは [LinuxBrew](https://docs.brew.sh/Homebrew-on-Linux) を使用してインストールできます：

```bash
brew install MaaAssistantArknights/tap/maa-cli
```

### プリコンパイル済みバイナリ

プリコンパイルされたバイナリーを [maa-cli リリース・ページ](https://github.com/MaaAssistantArknights/maa-cli/releases/latest) からダウンロードし、結果の実行可能ファイルを任意の場所に配置できます。 各プラットフォームのファイル名は次のとおりです：

<table>
    <thead>
        <tr>
            <th>オペレーティング システム</th>
            <th>プロセッサのアーキテクチャ</th>
            <th>ファイル名</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td rowspan=2>Linux</td>
            <td>x86_64</td>
            <td>maa_cli-x86_64-unknown-linux-gnu.tar.gz</td>
        </tr>
        <tr>
            <td>aarch64</td>
            <td>maa_cli-aarch64-unknown-linux-gnu.tar.gz</td>
        </tr>
        <tr>
            <td rowspan=2>macOS</td>
            <td>x86_64</td>
            <td rowspan=2>
              maa_cli-universal-apple-darwin.zip
            </td>
        </tr>
        <tr>
            <td>aaarch64</td>
        </tr>
        <tr>
            <td rowspan=2>Windows</td>
            <td>x86_64</td>
            <td>maa_cli-x86_64-pc-windows-msvc.zip</td>
        </tr>
    </tbody>
</table>

## 使用

`maa-cli` の主な機能はタスクを実行することであり、 `maa run <task>` でタスクを実行できます。ここでの `<task>` はタスクの名前であり、`maa list` を使用して使用可能なすべてのタスクを一覧表示できます。

詳細については、 `maa help` で見ることができます。

### タスクの実行

`maa-cli` の主な機能は、事前定義タスクやカスタム タスクなどのタスクを実行することです。

#### 事前定義タスク

- `maa startup [client]`: ゲームを起動し、メインインターフェイスに入ります。 `[client]` はクライアントバージョンで、空白のままにするとゲームクライアントは起動しません。
- `maa closedown`: ゲームを閉じます；
- `maa fight [stage]`: 作戦タスクを実行する。 `[stage]` 是关卡名称，例如 `1-7`；留空选择上次或者当前关卡；
- `maa copilot <maa_uri>`: 运行自动战斗任务，其中 `<maa_uri>` 是作业的 URI，其可以是 `maa://1234` 或者本地文件路径 `./1234.json`；
- `maa roguelike [theme]`: 运行 roguelike 模式的战斗任务，`[theme]` 是 roguelike 模式的主题，可选值为 `Phantom`，`Mizuki` 以及 `Sami`；

#### カスタム タスク

你可以通过 `maa run <task>` 来运行自定义任务。这里的 `<task>` 是一个任务的名字，你可以通过 `maa list` 来列出所有可用的任务。
具体的任务定义可以在 [設定サブセクション](#カスタムタスクの定義) 中找到。

#### 任务总结

`maa-cli` 会在任务运行结束后向 stdout 输出任务总结，包括每个子任务的运行时间和结果。你可以通过 `--no-summary` 选项来禁用任务总结。

任务总结主要包括各任务的运行时间。对于以下任务，还会包括其他信息：

- 刷理智 `fight`: 关卡名称，次数以及掉落统计；
- 基建换班 `infrast`: 各设施进驻的干员，对于制造站和贸易站，还会包括产物类型；
- 公招 `recruit`: 公招标签刷新次数，招募次数以及检测到的 tag 及星级。
- 肉鸽 `roguelike`: 进行的次数，投资的次数。

#### 日志输出

`maa-cli` 默认会向 stderr 输出日志。日志输出级别从低到高分别为 `Error`，`Warn`，`Info`，`Debug` 和 `Trace`。默认的日志输出级别为 `Warn`。日志级别可以通过 `MAA_LOG` 环境变量来设置，例如 `MAA_LOG=debug`。你也可以通过 `-v` 或者 `-q` 来增加或者减少日志输出级别。

`--log-file` 选项可以将日志输出到文件中，日志保存在 `$(maa dir log)/YYYY/MM/DD/HH:MM:SS.log` 中，其中 `$(maa dir log)` 是日志目录，你可以通过 `maa dir log` 获取。你也可以通过 `--log-file=path/to/log` 来指定日志文件的路径。

### 安装和更新

#### 安装和更新 MaaCore

你可以通过 `maa install` 和 `maa update` 来安装和更新 `MaaCore` 及资源，更多信息可以通过 `maa help install` 和 `maa help update` 获取。

#### 资源热更新

由于游戏的更新，`MaaCore` 需要最新的资源才能正常运行，你可以通过 `maa hot-update` 来更新资源，或者设置资源自动更新，详见 [CLI 相关配置](#maa-cli-相关配置)

#### 更新自身

你可以通过 `maa self update` 来更新 `maa-cli` 自身，注意对于由包管理器安装的 `maa-cli`，你应该使用包管理器来更新 `maa-cli`。

更多其他的命令可以通过 `maa help` 获取。

### 其他子命令

- `maa list`: 列出所有可用的任务；
- `maa dir <dir>`: 获取特定目录的路径，比如 `maa dir config` 可以用来获取配置目录的路径;
- `maa version`: 获取 `maa-cli` 以及 `MaaCore` 的版本信息；
- `maa convert <input> [output]`: 将 `JSON`，`YAML` 或者 `TOML` 格式的文件转换为其他格式;
- `maa complete <shell>`: 生成自动补全脚本;
- `maa activity [client]`: 获取游戏的当前活动信息，`client` 是客户端类型，默认为 `Official`。


## 設定

### 設定ディレクトリー

`maa-cli` 設定ファイルは、 `maa dir config` で取得できる特定の設定ディレクトリーにあります。 設定ディレクトリは、環境変数 `MAA_CONFIG_DIR` で変更することもできます。 以下の例では、設定ディレクトリを表すために `$MAA_CONFIG_DIR` を使用します。

すべての構成ファイルは、TOML、YAML、またはJSON形式にすることができますが、以下の例では、TOML形式を使用し、ファイル拡張子 `.toml` を使用します。 ただし、正しいファイル拡張子がある限り、これら3つの形式のいずれかを混在させることができます。

### カスタムタスクの定義

各カスタムタスクは個別のファイルであり、`$MAA_CONFIG_DIR/tasks` ディレクトリに配置する必要があります。

#### 基本结构

一个任务文件包含多个子任务，每一个子任务是一个 [MAA 任务](../协议文档/集成文档.md#asstappendtask)，其包含一下几个选项：

```toml
[[tasks]]
name = "启动游戏" # 任务的名字，可选，默认为任务类型
type = "StartUp" # maa任务的类型
params = { client_type = "Official", start_game_enabled = true } # maa任务的参数
```

#### 任务条件

如果你想要根据一些条件运行不同参数的任务，你可以定义多个任务的变体：

```toml
[[tasks]]
name = "基建换班"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # 自定义的基建计划的文件名应该位于`$MAA_CONFIG_DIR/infrast`

# 在12:00:00之前使用计划1，在12:00:00到18:00:00之间使用计划2，在18:00:00之后使用计划0
[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # 如果没有定义start，那么它将会是00:00:00
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00", end = "18:00:00" }
params = { plan_index = 2 }

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
params = { plan_index = 0 }
```

这里的 `condition` 字段用于确定哪一个变体应该被使用，而匹配的变体的 `params` 字段将会被合并到任务的参数中。

**注意**：如果你的自定义基建计划文件使用相对路径，应该相对于 `$MAA_CONFIG_DIR/infrast`。此外，由于基建文件是由 `MaaCore` 而不是 `maa-cli` 读取的，因此这些文件的格式必须是 `JSON`。同时，`maa-cli` 不会读取基建文件，也不会根据其中定义的时间段来选择相应的子计划。因此，必须通过 `condition` 字段来指定在相应时间段使用正确的基建计划的参数中的 `plan_index` 字段。这样可以确保在适当的时间段使用正确的基建计划。

除了 `Time` 条件，还有 `DateTime`，`Weekday`，`DayMod`条件。`DateTime` 条件用于指定一个时间段，`Weekday` 条件用于指定一周中的某些天，`DayMod` 见下文多天排班。

```toml
[[tasks]]
type = "Fight"

# 在夏活期间，刷SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 在夏活期间以外的周二、周四和周六，刷CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 其他时间，刷1-7
[[tasks.variants]]
params = { stage = "1-7" }
```

除了上述确定的条件之外，还有一个依赖于热更新资源的条件 `OnSideStory`，当你启动该条件后，`maa-cli` 会尝试读取相应的资源来判断当前是否有正在开启的活动，如果有那么对应的变体会被匹配。 比如上述夏活期间刷 `SL-8` 的条件就可以简化为 `{ type = "OnSideStory", client = "Official" }`，这里的 `client` 参数用于确定你使用的客户端，因为不同的客户端的活动时间不同，对于使用官服或者 b 服的用户，这可以省略。通过这个条件，每次活动更新之后你可以只需要更新需要刷的关卡而不需要手动编辑对应活动的开放时间。

除了以上基础条件之外，你可以使用 `{ type = "And", conditions = [...] }`，`{ type = "Or", conditions = [...] }`, `{ type = "Not", condition = ... }` 来对条件进行逻辑运算。

对于想要基建多天排班的用户，可以将 `DayMod` 和 `Time` 组合使用，可以实现多天排班。比如，你想要实现每两天换六次班，那么你可以这样写：

```toml
[[tasks]]
name = "基建换班 (2天6班)"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json"

# 第一班，第一天 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 0 }
[tasks.variants.condition]
type = "And"
conditions = [
    # 这里的 divisor 用来指定周期，remainder 用来指定偏移量
    # 偏移量等于 num_days_since_ce % divisor
    # 这里的 num_days_since_ce 是公元以来的天数，0001-01-01 是第一天
    # 当天偏移量你可以通过 `maa remainder <divisor>` 来获取.
    # 比如，2024-1-27 是第 738,912 天，那么 738912 % 2 = 0
    # 当天的偏移量为 0，那么本条件将会被匹配
    { type = "DayMod", divisor = 2, remainder = 0 },
    { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 第二班，第一天 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 1 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 0 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 第三班，第一天 20:00:00 - 第二天 4:00:00
[[tasks.variants]]
params = { plan_index = 2 }
[tasks.variants.condition]
# 注意这里必须使用 Or 条件，不能直接使用 Time { start = "20:00:00", end = "04:00:00" }
# 在这种情况下， 第二天的 00:00:00 - 04:00:00 不会被匹配
# 当然通过调整你的排班时间避免跨天是更好的选择，这里只是为了演示
type = "Or"
conditions = [
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 0 },
     { type = "Time", start = "20:00:00" },
  ] },
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 1 },
     { type = "Time", end = "04:00:00" },
  ] },
]

# 第四班，第二天 4:00:00 - 12:00:00
[[tasks.variants]]
params = { plan_index = 3 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "04:00:00", end = "12:00:00" },
]

# 第五班，第二天 12:00:00 - 20:00:00
[[tasks.variants]]
params = { plan_index = 4 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "DayMod", divisor = 2, remainder = 1 },
  { type = "Time", start = "12:00:00", end = "20:00:00" },
]

# 第六班，第二天 20:00:00 - 第三天（新的第一天）4:00:00
[[tasks.variants]]
params = { plan_index = 5 }
[tasks.variants.condition]
type = "Or"
conditions = [
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 1 },
     { type = "Time", start = "20:00:00" },
  ] },
  { type = "And", conditions = [
     { type = "DayMod", divisor = 2, remainder = 0 },
     { type = "Time", end = "04:00:00" },
  ] },
]
```

在默认的策略下，如果有多个变体被匹配，第一个将会被使用。如果没有给出条件，那么变体将会总是被匹配，所以你可以把没有条件的变体放在最后，作为默认的情况。

你可以使用 `strategy` 字段来改变匹配策略：

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # 或者 "first" (默认)

# 在周天晚上使用所有的将要过期的理智药
[[tasks.variants]]
params = { expiring_medicine = 1000 }

[tasks.variants.condition]
type = "And"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# 默认刷1-7
[[tasks.variants]]
params = { stage = "1-7" }

# 在周二、周四和周六，刷CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 在夏活期间，刷SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

这个例子和上面的例子将刷同样的关卡，但是在周天晚上，将会使用所有的将要过期的理智药。在 `merge` 策略下，如果有多个变体被匹配，后面的变体的参数将合并入前面的变体的参数中。如果多个变体都有相同的参数，那么后面的变体的参数将会覆盖前面的变体的参数。

如果没有变体被匹配，那么任务将不会被执行，这可以用于只在特定的条件下运行子任务：

```toml
# 只在在18:00:00之后进行信用商店相关的操作
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

#### 用户输入

对于一些任务，你可能想要在运行时输入一些参数，例如关卡名称。 你可以将对应需要输入的参数设置为 `Input` 或者 `Select` 类型：

```toml
[[tasks]]
type = "Fight"

# 选择一个关卡
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
[tasks.variants.params.stage]
# 可选的关卡，必须提供至少一个可选值
# 可选值可以是一个值，也可以是同时包含值和描述的一个表
alternatives = [
    "SL-7", # 将被显示为 "1. SL-7"
    { value = "SL-8", desc = "轻锰矿" } # 将被显示为 "2. SL-8 (轻锰矿)"
]
default_index = 1 # 默认值的索引，从 1 开始，如果没有设置，输入空值将会重新提示输入
description = "a stage to fight in summer event" # 描述，可选
allow_custom = true # 是否允许输入自定义的值，默认为 false，如果允许，那么非整数的值将会被视为自定义的值

# 无需任何输入
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# 输入一个关卡
[[tasks.variants]]
[tasks.variants.params.stage]
default = "1-7" # 默认的关卡，可选（如果没有默认值，输入空值将会重新提示输入）
description = "a stage to fight" # 描述，可选
[tasks.variants.params.medicine]
# 依赖的参数，键为参数名，值为依赖的参数的预期值
# 当设置时，只有所有的依赖参数都满足预期值时，这个参数才会被要求输入
deps = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

对于 `Input` 类型，当运行任务时，你将会被提示输入一个值。如果你输入了一个空值，如果有默认值，那么默认值将会被使用，否则你将会被提示重新输入。
对于 `Select` 类型，当运行任务时，你将会被提示输入一个的索引或者自定义的值（如果允许）。如果你输入了一个空值，如果有默认值，那么默认值将会被使用，否则你将会被提示重新输入。

`--batch` 选项可以用于在运行任务时跳过所有的输入，这将会使用默认值；如果有任何输入没有默认值，那么将会导致错误。

### MaaCore 相关配置

和 MaaCore 相关的配置需要放在 `$MAA_CONFIG_DIR/asst.toml` 中。
目前其包含的配置有：

```toml
[connection]
type = "ADB"
adb_path = "adb"
device = "emulator-5554"
config = "CompatMac"

[resource]
global_resource = "YoStarEN"
platform_diff_resource = "iOS"
user_resource = true

[static_options]
cpu_ocr = false
gpu_ocr = 1

[instance_options]
touch_mode = "MAATouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false
```

#### 连接配置

`[connection]` 相关字段用于指定 MaaCore 连接游戏的方式和参数。目前可用的连接方式有 `ADB` 和 `PlayTools`。

当你使用 `ADB` 连接时，你需要提供 `adb` 的路径和设备的序列号：

```toml
[connection]
type = "ADB"
adb_path = "adb" # adb可执行文件的路径
device = "emulator-5554" # 你的android设备的序列号
config = "General" # maa connect的配置
```

注意，此处的 device 是任何 `adb -s` 可以接受的值，比如 `emulator-5554` 和 `127.0.0.1:5555`。

当你使用 `PlayTools` 连接时，你需要提供 `PlayTools` 的地址：

```toml
[connection]
type = "PlayCover"
address = "localhost:1717" # PlayTools的地址
config = "CompatMac" # maa connect的配置
```

两者都需要提供 `config`，这个值将被传给 MaaCore，用于指定一些平台和模拟器相关的配置。对于 Linux 他默认为 `CompatPOSIXShell`，对于 macOS 他默认为 `CompatMac`，对于 Windows 他默认为 `General`。更多可选配置可以在资源文件夹中的 `config.json` 文件中找到。

#### 资源配置

`[resource]` 相关字段用于指定 MaaCore 加载的资源：

```toml
[resource]
global_resource = "YoStarEN" # 非中文版本的资源
platform_diff_resource = "iOS" # 非安卓版本的资源
user_resource = true # 是否加载用户自定义的资源
```

当使用非简体中文游戏客户端时，由于 `MaaCore` 默认加载的资源是简体中文的，你需要指定 `global_resource` 字段来加载非中文版本的资源。当使用 iOS 版本的游戏客户端时，你需要指定 `platform_diff_resource` 字段来加载 iOS 版本的资源。这两者都是可选的，如果你不需要加载这些资源，你可以将这两个字段设置为空。其次，这两者也会被自动设置，如果你的 `startup` 任务中指定了 `client_type` 字段，那么 `global_resource` 将会被设置为对应客户端的资源，而当你使用 `PlayTools` 连接时，`platform_diff_resource` 将会被设置为 `iOS`。最后，当你想要加载用户自定义的资源时，你需要将 `user_resource` 字段设置为 `true`。

#### 静态选项

`[static_options]` 相关字段用于指定 MaaCore 静态选项，详见 [集成文档](../协议文档/集成文档.html#asstsetstaticoption)：

```toml
[static_options]
cpu_ocr = false # 是否使用 CPU OCR，默认使用 CPU OCR
gpu_ocr = 1 # 使用 GPU OCR 时使用的 GPU ID，如果这个值被留空，那么将会使用 CPU OCR
```

#### 实例选项

`[instance_options]` 相关字段用于指定 MaaCore 实例的选项，详见 [集成文档](../协议文档/集成文档.html#asstsetinstanceoption)：

```toml
[instance_options]
touch_mode = "ADB" # 使用的触摸模式，可选值为 "ADB"，"MiniTouch"，"MAATouch" 或者 "MacPlayTools"
deployment_with_pause = false # 是否在部署时暂停游戏
adb_lite_enabled = false # 是否使用 adb-lite
kill_adb_on_exit = false # 是否在退出时杀死 adb
```

注意，`touch_mode` 可选项 `MacPlayTools` 和连接方式 `PlayTools` 绑定。当你使用 `PlayTools` 连接时，`touch_mode` 将会被强制设置为 `MacPlayTools`。

### `maa-cli` 相关配置

`maa-cli` 相关的配置需要放在 `$MAA_CONFIG_DIR/cli.toml` 中。目前其包含的配置如下：

```toml
# MaaCore 安装和更新相关配置
[core]
channel = "Stable" # 更新通道，可选值为 "Alpha"，"Beta" "Stable"，默认为 "Stable"
test_time = 0    # 用于测试镜像速度的时间，0 表示不测试，默认为 3
# 查询 MaaCore 最新版本的 api 地址，留空表示使用默认地址
api_url = "https://github.com/MaaAssistantArknights/MaaRelease/raw/main/MaaAssistantArknights/api/version/"

# 配置是否安装 MaaCore 对应的组件，不推荐使用，分开安装可能会导致版本不一致，从而导致一些问题，该选项可能在未来的版本中移除
[core.components]
library = true  # 是否安装 MaaCore 的库，默认为 true
resource = true # 是否安装 MaaCore 的资源，默认为 true

# CLI 更新相关配置
[cli]
channel = "Stable" # 更新通道，可选值为 "Alpha"，"Beta" "Stable"，默认为 "Stable"
# 查询 maa-cli 最新版本的 api 地址，留空表示使用默认地址
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# 下载预编译二进制文件的地址，留空表示使用默认地址
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

# 配置是否安装 maa-cli 对应的组件
[cli.components]
binary = true # 是否安装 maa-cli 的二进制文件，默认为 true

# 资源热更新相关配置
[resource]
auto_update = true  # 是否在每次运行任务时自动更新资源，默认为 false
backend = "libgit2" # 资源热更新后端，可选值为 "git" 或者 "libgit2"，默认为 "git"

# 资源热更新远程仓库相关配置
[resource.remote]
branch = "main" # 远程仓库的分支，默认为 "main"
# リモートリポジトリの URL 、 SSH を使用する場合は、ssh_key へのパスを設定する必要があります
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# ssh_key = "~/.ssh/id_ed25519" # SSH 秘密鍵のパス (拡張可能) 、ただし、他の環境変数はサポートされていません
```

**注意事項**：

- MaaCore の更新チャネルの  `Alpha` は、Windows でのみ使用できます。
- CLIのデフォルトのAPIリンクとダウンロードリンクは全てGitHubリンクなので、中国では多少の問題があるかもしれませんが、 `api_url` と `download_url` を設定することでミラーサイトを利用することができます。
- リソースのホット アップデートを有効にした場合でも、 `MaaCore` リソースをインストールする必要があります。リソースのホット アップデートにはすべてのリソース ファイルが含まれているわけではなく、更新可能なリソース ファイルの一部のみが含まれるため、基本リソース ファイルをインストールする必要があります。
- リソースのホット アップデートはGitを介してリモート リポジトリから取得され、バックエンドが `git` に設定されている場合は、 `git` コマンドラインツールが使用可能である必要があります。
- SSH プロトコルを使用してリモートリポジトリをプルする場合は、SSH 秘密鍵へのパスである `ssh_key` フィールドを構成する必要があります。
- リモートリポジトリの `url` 設定は現在、リソースの最初のインストールでのみ有効であり、リモートリポジトリのアドレスを変更する場合は、 `git` コマンドラインツールを使用して手動で変更するか、或いはリポジトリを削除する。 リポジトリの場所は、 `maa dir hot-update` で取得できます。
- リモートリポジトリの `url` はローカル言語に従って自動的に設定され、言語が簡体字中国語の場合、リモートリポジトリの `url` は国内ミラーの `https://git.maa-org.net/MAA/MaaResource.git` に設定され、それ以外の場合は Github に設定されます。 中国にいて簡体字中国語を使用していない場合、または海外で簡体字中国語を使用している場合は、最適なエクスペリエンスを得るために手動で設定する必要があります。

### JSON Schema

`maa-cli` の JSON Schema ファイルは、CLI リポジトリーの `maa-cli/schemas` ディレクトリーにあり、これを使用して構成ファイルを検証したり、エディターでオートコンプリートを取得したりできます。
タスク ファイルの JSON Schema ファイルは [`task.schema.json`](https://github.com/MaaAssistantArknights/maa-cli/raw/v0.4.0/maa-cli/schemas/task.schema.json)；
MaaCore の構成の JSON Schema ファイルは、 [`asst.schema.json`](https://github.com/MaaAssistantArknights/maa-cli/raw/v0.4.0/maa-cli/schemas/task.schema.json)；
CLI 設定の JSON Schema ファイルは [`cli.schema.json`](https://github.com/MaaAssistantArknights/maa-cli/raw/v0.4.0/maa-cli/schemas/task.schema.json)。
