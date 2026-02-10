---
order: 3
icon: material-symbols:settings
---

# 配置

## 配置目录

maa-cli 配置文件位于特定的配置目录中，你可以通过 `maa dir config` 获取配置目录。配置目录也可以通过环境变量 `MAA_CONFIG_DIR` 更改。在下面的例子中，我们将用 `$MAA_CONFIG_DIR` 来表示配置目录。

所有的配置文件都可以使用 TOML，YAML 或者 JSON 格式，在下面的例子中，我们将使用 TOML 格式，并使用 `.toml` 作为文件扩展名。但是你可以混合这三种格式中的任意一种，只要你的文件扩展名正确。

此外，部分任务接受 `filename` 作为参数，如果你使用相对路径，那么相对路径将会相对于配置目录的对应子目录。比如自定义基建计划文件的相对路径应该相对于 `$MAA_CONFIG_DIR/infrast`，而保全派驻的作业文件则相对于 `$MAA_CONFIG_DIR/ssscopilot`。

## 自定义任务

每一个自定义任务都是一个单独的文件，它们应该位于 `$MAA_CONFIG_DIR/tasks` 目录中。

### 基本结构

一个任务文件包含多个子任务，每一个子任务是一个 MAA 任务，其包含一下几个选项：

```toml
[[tasks]]
name = "启动游戏" # 任务的名字，可选，默认为任务类型
type = "StartUp" # 任务的类型
params = { client_type = "Official", start_game_enabled = true } # 对应任务的参数
```

具体的任务类型和参数可以在 [MAA 集成文档][task-types] 中找到。注意，目前 maa-cli 并不会验证参数名和参数值是否正确，即使出错也不会产生任何错误信息，除非 MaaCore 在运行时检测到错误。

### 任务条件

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

# 在 18:00:00到第二天的 04:00:00 使用计划 0，在 12:00:00 之前使用计划 1，之后使用计划 2
[[tasks.variants]]
condition = { type = "Time", start = "18:00:00", end = "04:00:00" } # 当结束时间小于开始时间时，结束时间被视为第二天的时间
params = { plan_index = 0 }

[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # 如果开始时间被省略，那么只要当前时间小于结束时间时，这个条件就会被匹配
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00" } # 如果结束时间被省略，那么只要当前时间大于开始时间时，这个条件就会被匹配
params = { plan_index = 2 }
```

这里的 `condition` 字段用于确定哪一个变体应该被使用，而匹配的变体的 `params` 字段将会被合并到任务的参数中。

**注意**：如果你的自定义基建计划文件使用相对路径，应该相对于 `$MAA_CONFIG_DIR/infrast`。此外，由于基建文件是由 MaaCore 而不是 maa-cli 读取的，因此这些文件的格式必须是 JSON。同时，maa-cli 不会读取基建文件，也不会根据其中定义的时间段来选择相应的子计划。因此，必须通过 `condition` 字段来指定在相应时间段使用正确的基建计划的参数中的 `plan_index` 字段。这样可以确保在适当的时间段使用正确的基建计划。

除了 `Time` 条件，还有 `DateTime`，`Weekday`，`DayMod`条件。`DateTime` 条件用于指定一个时间段，`Weekday` 条件用于指定一周中的某些天，`DayMod` 用于指定一个自定义周期的某些天。

```toml
[[tasks]]
type = "Fight"

# 在夏活期间，刷SL-8
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# 在夏活期间以外的周二、周四和周六，刷CE-6
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"], timezone = "Official"}
params = { stage = "CE-6" }

# 其他时间，刷1-7
[[tasks.variants]]
params = { stage = "1-7" }
```

对与上述所有时间相关的条件，其都可以通过 `timezone` 参数来指定时区，这个参数的值可以是一个数字，表示与 UTC 的偏移量，如果你的时区是东八区，那么你可以指定 `timezone = 8`。这个参数也可以是一个客户端类型，比如 `timezone = "Official"`，这样将会使用官服对应的服务器时间来判断。**注意**，官服的时区不是东八区而是东四区，因为游戏中每天开始时间是 04:00:00 而不是 00:00:00。如果不指定时区，那么直接使用你的本地时区。

除了上述确定的条件之外，还有一个依赖于热更新资源的条件 `OnSideStory`，当你启动该条件后，maa-cli 会尝试读取相应的资源来判断当前是否有正在开启的活动，如果有那么对应的变体会被匹配。 比如上述夏活期间刷 `SL-8` 的条件就可以简化为 `{ type = "OnSideStory", client = "Official" }`，这里的 `client` 参数用于确定你使用的客户端，因为不同的客户端的活动时间不同，对于使用官服或者 b 服的用户，这可以省略。通过这个条件，每次活动更新之后你可以只需要更新需要刷的关卡而不需要手动编辑对应活动的开放时间。

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

### 用户输入

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

# 当输入的关卡是 1-7 时，需要输入使用理智药的数量
[tasks.variants.params.medicine]
# 参数可以设置为条件参数，这样只有满足条件时才需要输入
# conditions 字段是一个表，其中键是同一层级下其他参数名，值是期望的值
# 这里的条件是 stage 是 1-7， 如果存在多个条件，那么所有条件都必须满足
conditions = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

对于 `Input` 类型，当运行任务时，你将会被提示输入一个值。如果你输入了一个空值，如果有默认值，那么默认值将会被使用，否则你将会被提示重新输入。
对于 `Select` 类型，当运行任务时，你将会被提示输入一个的索引或者自定义的值（如果允许）。如果你输入了一个空值，如果有默认值，那么默认值将会被使用，否则你将会被提示重新输入。

`--batch` 选项可以用于在运行任务时跳过所有的输入，这将会使用默认值；如果有任何输入没有默认值，那么将会导致错误。

## MaaCore 相关配置

和 MaaCore 相关的配置需要放在 `$MAA_CONFIG_DIR/profiles` 目录中。该目录下的每一个文件都是一个配置文件，你可以通过 `-p` 或者 `--profile` 选项来指定配置文件名，不指定时尝试读取 `default` 配置文件。

目前支持的配置字段如下：

```toml
[connection]
preset = "MuMuPro"
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
touch_mode = "MaaTouch"
deployment_with_pause = false
adb_lite_enabled = false
kill_adb_on_exit = false
```

### 连接配置

`[connection]` 相关字段用于指定 MaaCore 连接游戏的参数：

```toml
[connection]
adb_path = "adb" # adb 可执行文件的路径，默认值为 "adb"，这意味着 adb 可执行文件在环境变量 PATH 中
address = = "emulator-5554" # 连接地址，比如 "emulator-5554" 或者 "127.0.0.1:5555"
config = "General" # 连接配置，通常不需要修改
```

`adb_path` 是 `adb` 可执行文件的路径，你可以指定其路径，或者将其添加到环境变量 `PATH` 中，以便 MaaCore 可以找到它。大多数模拟器自带 `adb`，你可以直接使用其自带的 `adb`，而不需要额外安装，否则你需要自行安装 `adb`。`address` 是 `adb` 的连接地址。对于模拟器，你可以使用 `127.0.0.1:[端口号]`，常用的模拟器端口号参见[常见问题][emulator-ports]。如果你没有指定 `address`，那么会尝试通过 `adb devices` 来获取连接的设备，如果有多个设备连接，那么将会使用第一个设备，如果没有找到任何设备，那么将会尝试连接到 `emulator-5554`。`config` 用于指定一些平台和模拟器相关的配置。对于 Linux 他默认为 `CompatPOSIXShell`，对于 macOS 他默认为 `CompatMac`，对于 Windows 他默认为 `General`。更多可选配置可以在资源文件夹中的 `config.json` 文件中找到。

对于一些常用的模拟器，你可以直接使用 `preset` 来使用预设的配置：

```toml
[connection]
preset = "MuMuPro" # 使用 MuMuPro 预设的连接配置
adb_path = "/path/to/adb" # 如果你需要的话，你可以覆盖预设的 adb 路径，大多数情况下你不需要这么做
address = "127.0.0.1:7777" # 如果你需要的话，你可以覆盖预设的地址
```

目前只有 `MuMuPro` 一个模拟器的预设，如果有其他常用模拟器的预设，欢迎提交 issue 或者 PR。

#### 特殊预设

目前预配置了两种预设，为 `PlayCover (macOS)`, `Waydroid (Linux)`

- `PlayCover`用于在 macOS 上连接直接通过 `PlayCover` 原生运行的游戏客户端。这种情况下不需要指定 `adb_path` 且 `address` 不是 `adb` 连接的地址而是 `PlayTools` 的地址，具体使用参见 [PlayCover 支持文档][playcover-doc].

- `Waydroid`用于在 Linux 上连接直接通过 `Waydroid` 原生运行的游戏客户端。这种情况下仍需要指定 `adb_path`，具体使用参见 [Waydroid 支持文档][waydroid-doc].

### 资源配置

`[resource]` 相关字段用于指定 MaaCore 加载的资源：

```toml
[resource]
global_resource = "YoStarEN" # 非中文版本的资源
platform_diff_resource = "iOS" # 非安卓版本的资源
user_resource = true # 是否加载用户自定义的资源
```

当使用非简体中文游戏客户端时，由于 MaaCore 默认加载的资源是简体中文的，你需要指定 `global_resource` 字段来加载非中文版本的资源。当使用 iOS 版本的游戏客户端时，你需要指定 `platform_diff_resource` 字段来加载 iOS 版本的资源。这两者都是可选的，如果你不需要加载这些资源，你可以将这两个字段设置为空。其次，这两者也会被自动设置，如果你的 `startup` 任务中指定了 `client_type` 字段，那么 `global_resource` 将会被设置为对应客户端的资源，而当你使用 `PlayTools` 连接时，`platform_diff_resource` 将会被设置为 `iOS`。最后，当你想要加载用户自定义的资源时，你需要将 `user_resource` 字段设置为 `true`。

### 静态选项

`[static_options]` 相关字段用于指定 MaaCore 静态选项：

```toml
[static_options]
cpu_ocr = false # 是否使用 CPU OCR，默认使用 CPU OCR
gpu_ocr = 1 # 使用 GPU OCR 时使用的 GPU ID，如果这个值被留空，那么将会使用 CPU OCR
```

### 实例选项

`[instance_options]` 相关字段用于指定 MaaCore 实例的选项：

```toml
[instance_options]
touch_mode = "ADB" # 使用的触摸模式，可选值为 "ADB"，"MiniTouch"，"MaaTouch" 或者 "MacPlayTools"
deployment_with_pause = false # 是否在部署时暂停游戏
adb_lite_enabled = false # 是否使用 adb-lite
kill_adb_on_exit = false # 是否在退出时杀死 adb
```

注意，`touch_mode` 可选项 `MacPlayTools` 和连接方式 `PlayTools` 绑定。当你使用 `PlayTools` 连接时，`touch_mode` 将会被强制设置为 `MacPlayTools`。

## CLI 相关配置

CLI 相关的配置需要放在 `$MAA_CONFIG_DIR/cli.toml` 中。目前其包含的配置如下：

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
warn_on_update_failure = true # 是否在更新失败时发出警告而不是直接报错
backend = "libgit2" # 资源热更新后端，可选值为 "git" 或者 "libgit2"，默认为 "git"

# 资源热更新远程仓库相关配置
[resource.remote]
branch = "main" # 远程仓库的分支，默认为 "main"
# 远程资源仓库的 URL，留空以使用默认 URL
# GitHub 仓库支持 HTTPS 和 SSH 两种协议访问，建议使用 HTTPS 协议，因为通常情况下不需要额外配置
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# 如果你必须使用 SSH 协议，你需要提供 SSH 密钥，最简单的方法是提供密钥的路径
ssh_key = "~/.ssh/id_ed25519" # ssh 密钥的路径
# maa 默认密钥是未加密的，如果你的密钥是受密码保护的，你需要提供密码来解密密钥
# 注意：只有你使用 libgit2 后端时 maa 才会将密码传递给 libgit2
# 当你使用 git 后端时，git 会自己提示你输入密码
# 如果你使用 git 后端且你的密钥受密码保护，请使用 ssh-agent 来管理你的密钥
passphrase = "password"       # ssh 密钥的密码
# 然而在配置文件中存储明文密码是不安全的，因此有一些方法可以避免这种情况
# 1. 将 `passphrase` 设置为 true，然后 maa-cli 将每次提示你输入密码
# 这种方法安全但是较为繁琐且无法在 batch 模式下使用
# passphrase = true
# 2. 将 `passphrase` 设置为环境变量名，然后 maa-cli 将使用环境变量作为密码
# 这种方法比明文密码更安全，但是仍然有一定的风险，因为环境变量可能被任何程序访问
# passphrase = { env = "MAA_SSH_PASSPHRASE" }
# 3. 将 `passphrase` 设置为命令，然后 maa-cli 将执行该命令以获取密码
# 如果你使用了密码管理器来管理你的密码，这种方法可能是最安全的且方便的
# passphrase = { cmd = ["pass", "show", "ssh/id_ed25519"] }
# 4. 使用 ssh-agent 来管理你的密钥，**推荐**
# ssh-agent 会将你的密钥保存在内存中，这样你就不需要每次输入密码
# 注意，你需要确保 ssh-agent 已经启动并且已经添加了你的密钥，同时 SSH_AUTH_SOCK 环境变量已经设置
# use_ssh_agent = true # 使用 ssh-agent 进行身份验证，如果设置为 true，将忽略 ssh_key 和 passphrase 字段
```

**注意事项**：

- MaaCore 的更新通道中 `Alpha` 只在 Windows 上可用；
- 由于 CLI 默认的 API 链接和下载链接都是 GitHub 的链接，因此在国内可能会有一些问题，你可以通过配置 `api_url` 和 `download_url` 来使用镜像。
- 即使启动了资源热更新，你依然需要安装 MaaCore 的资源，因为资源热更新并不包含所有的资源文件，只是包含部分可更新的资源文件，基础资源文件仍然需要安装。
- 资源热更新是通过 Git 来拉取远程仓库，如果后端设置为 `git` 那么 `git` 命令行工具必须可用。
- 如果你想要使用 SSH 协议来拉取远程仓库，你必须配置 `ssh_key` 字段，这个字段应该是一个路径，指向你的 SSH 私钥。
- 如果你的 SSH 私钥是受密码保护的，你需要提供密码来解密私钥，或者使用 ssh-agent 来管理你的密钥。
- 远程仓库的 `url` 设置目前只对首次安装资源有效，如果你想要更改远程仓库的地址，你需要通过 `git` 命令行工具手动更改，或者删除对应的仓库。仓库所在位置可以通过 `maa dir hot-update` 获取。

## 参考配置

- [示例配置][example-config]
- [自用配置][wangl-cc-dotfiles]

## JSON Schema

你可以在 [`schemas` 目录][schema-dir] 中找到 maa-cli 的 JSON Schema 文件，你可以使用这些文件来验证你的配置文件，或者在编辑器中获得自动补全。

- 自定任务文件的 JSON Schema 文件为 [`task.schema.json`][task-schema]；
- MaaCore 配置的 JSON Schema 文件为 [`asst.schema.json`][asst-schema]；
- CLI 配置的 JSON Schema 文件为 [`cli.schema.json`][cli-schema]。

[task-types]: ../../protocol/integration.md#任务类型一览
[emulator-ports]: ../../manual/connection.md#获取端口号
[playcover-doc]: ../../manual/device/macos.md#✅-playcover-原生运行最流畅-🚀
[waydroid-doc]: ../../manual/device/linux.md#✅-waydroid
[example-config]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/config_examples
[wangl-cc-dotfiles]: https://github.com/wangl-cc/dotfiles/tree/main/home/dot_config/maa
[schema-dir]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/
[task-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/task.schema.json
[asst-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/asst.schema.json
[cli-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/cli.schema.json
