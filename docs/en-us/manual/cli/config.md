---
order: 3
icon: material-symbols:settings
---

# Configuration

## Configuration Directory

The maa-cli configuration files are located in a specific configuration directory, which you can get by running `maa dir config`. The configuration directory can also be changed by the environment variable `MAA_CONFIG_DIR`. In the following examples, we will use `$MAA_CONFIG_DIR` to represent the configuration directory.

All configuration files can be in TOML, YAML, or JSON format. In the following examples, we will use the TOML format and use `.toml` as the file extension. But you can mix these three formats, as long as your file extension is correct.

In addition, some tasks accept `filename` as a parameter. When the relative path is used, the relative path will be relative to the corresponding subdirectory of the configuration directory. For example, the custom infrastructure plan files should be relative to `$MAA_CONFIG_DIR/infrast`, while the copilot files of Stationary Security Service should be relative to `$MAA_CONFIG_DIR/ssscopilot`.

## Custom Tasks

A custom task is a separate file located in the `$MAA_CONFIG_DIR/tasks` directory.

### Basic Structure

A task file contains multiple subtasks, each of which is an MAA task, which contains the following options:

```toml
[[tasks]]
name = "Start the game" # The name of the task, optional, defaults to the task type
type = "StartUp" # The type of the task
params = { client_type = "Official", start_game_enabled = true } # The parameters of the task
```

The specific task types and parameters can be found in the [MAA Integration Document][task-types]. Note that maa-cli does not validate parameter names and values, and no error message will be generated even if an error occurs unless MaaCore` detects an error at runtime.

### Task variants and conditions

In some cases, you may want to run a task with different parameters in different conditions. You can define multiple variants for a task, and use the `condition` field to determine whether the variant should be used. For example, you may want to use a different infrastructure plan at different periods of the day:

```toml
[[tasks]]
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json" # the filename of custom infrast plan

# use plan 0 from 18:00:00 to 04:00:00 of next day, use plan 1 before 12:00:00, use plan 2 after 12:00:00
[[tasks.variants]]
condition = { type = "Time", start = "18:00:00", end = "04:00:00" } # when end time is less than start time, end time will be treated as time of next day
params = { plan_index = 0 }

[[tasks.variants]]
condition = { type = "Time", end = "12:00:00" } # if start time is omitted, this condition will be matched if current time is less than end time
params = { plan_index = 1 }

[[tasks.variants]]
condition = { type = "Time", start = "12:00:00" } # if end time is omitted, this condition will be matched if current time is greater than start time
params = { plan_index = 2 }
```

The `condition` field is used to determine whether the variant should be used,
and the `params` field of the matched variant will be merged into the parameters of the task.

**Note**: If the `filename` field is a relative path, it will be relative to `$MAA_CONFIG_DIR/infrast`. Besides, the custom infrastructure plan file will not be read by `maa-cli` but MaaCore. So the format of the file must be `JSON` and the time period defined in the file will not be used to select the corresponding sub-plan. So you must specify the `plan_index` field in the parameters of the task to use the correct infrastructure plan in the corresponding time period. This will ensure that the correct infrastructure plan is used in the appropriate time period.

Besides of `Time` condition, there are also `DateTime`, `Weekday`, `DayMod` conditions.
`DateTime` condition is used to specify a specific date-time period,
`Weekday` condition is used to specify some days in a week,
`DayMod` condition is similar to `Weekday`, but the period can be specified by `divisor` and `remainder`.

```toml
[[tasks]]
type = "Fight"

# fight SL-8 on summer event
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
# fight CE-6 on Tue, Thu, Sat if not on summer event
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"], timezone = "Official"}
params = { stage = "CE-6" }
# fight 1-7 otherwise
[[tasks.variants]]
params = { stage = "1-7" }
```

All the above conditions related to time have a `timezone` field, which is used to specify the timezone of the condition. The value of `timezone` can be an offset of UTC, like `8` or `-7`, or a name of the client type of game, like `Official`. Note, even though the official server is in China, the timezone of the official server is `UTC+4` instead of `UTC+8`, because the start of the game day is `04:00:00` instead of `00:00:00`. When the `timezone` is omitted, the condition will be matched in the local timezone of the system.
Besides of above conditions, there is a condition `OnSideStory` which depends on hot update resource to check if there is any opening side story. Thus, the condition of fight `SL-8` can be simplified as `{ type = "OnSideStory", client = "Official" }`, where the `client` is the client type of game.

Beside of above basic condition, `{ type = "And", conditions = [...] }` `{ type = "Or", conditions = [...] }`, and `{ type = "Not", condition = ... }` can be used for logical combination of conditions.

By the combination of of above conditions, you can define an infrastructure plan for multiple days,
here is an example of 6 plans for 2 days:

```toml
[[tasks]]
name = "Infrast (6 plan for 2 days)"
type = "Infrast"

[tasks.params]
mode = 10000
facility = ["Trade", "Reception", "Mfg", "Control", "Power", "Office", "Dorm"]
dorm_trust_enabled = true
filename = "normal.json"

# First shift, 04:00:00 - 12:00:00 on the first day
[[tasks.variants]]
condition = {
    type = "And",
    conditions = [
        # The divisor use to specify the period, the remainder use to specify the offset
        # The offset is equal to num_days_since_ce % divisor
        # The num_days_since_ce is the number of days since the Common Era, 0001-01-01 is the first day
        # The offset of current day can be got by `maa remainder <divisor>`
        # for 2024-01-27, num_days_since_ce is 738,912,
        # the offset of 2024-01-27 is 738,912 % 2 = 0
        # so this condition will be matched on 2024-01-27
        { type = "DayMod", divisor = 2, remainder = 0 },
        { type = "Time", start = "04:00:00", end = "12:00:00" },

    ]
}
params = { plan_index = 0 }

# The second shift, 12:00:00 - 20:00:00 on the first day
[[tasks.variants]]
condition = {
   type = "And",
   conditions = [
      { type = "DayMod", divisor = 2, remainder = 0 },
      { type = "Time", start = "12:00:00", end = "20:00:00" },
   ]
}
params = { plan_index = 1 }

# The third shift, 20:00:00 (first day) - 04:00:00 (second day)
[[tasks.variants]]
# Note, we must use Or condition here, otherwise the second day 00:00:00 - 04:00:00 will not be matched
condition = {
   type = "Or",
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
}
params = { plan_index = 2 }

# The fourth shift, 04:00:00 - 12:00:00 on the second day
[[tasks.variants]]
condition = {
   type = "And",
   conditions = [
      { type = "DayMod", divisor = 2, remainder = 1 },
      { type = "Time", start = "04:00:00", end = "12:00:00" },
   ]
}
params = { plan_index = 3 }

# The fifth shift, 12:00:00 - 20:00:00 on the second day
[[tasks.variants]]
condition = {
   type = "And",
   conditions = [
      { type = "DayMod", divisor = 2, remainder = 1 },
      { type = "Time", start = "12:00:00", end = "20:00:00" },
   ]
}
params = { plan_index = 4 }

# The sixth shift, 20:00:00 (second day) - 04:00:00 (new first day)
[[tasks.variants]]
condition = {
   type = "Or",
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
}
params = { plan_index = 5 }
```

With the default strategy, if multiple variants are matched, only the first one will be used. If the condition is not given, the variant will always be matched. So you can put a variant without condition at the end of variants.

The strategy of matching variants can be changed by `strategy` field:

```toml
[[tasks]]
type = "Fight"
strategy = "merge" # or "first" (default)

# use all expiring medicine on Sunday night
[[tasks.variants]]
params = { expiring_medicine = 1000 }
[tasks.variants.condition]
type = "And"
conditions = [
  { type = "Time", start = "18:00:00" },
  { type = "Weekday", weekdays = ["Sun"] },
]

# fight 1-7 by default
[[tasks.variants]]
params = { stage = "1-7" }

# fight CE-6 on Tue, Thu, Sat if not on summer event
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# fight SL-8 on summer event
[[tasks.variants]]
params = { stage = "SL-8" }
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }
```

The outcome stage of this example should be identical to the previous one, but expiring medicine will be used on Sunday night additionally.
With the `merge` strategy, if multiple variants are matched, the parameters of all matched variants will be merged. If multiple variants have the same parameters, the last one will be used.

If no variant is matched, the task will not be executed,
which is useful when you want to only run a task in some conditions:

```toml
# Mall after 18:00
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

### User input

In some cases, you may want to input some value at runtime, instead of hard code it in the task file. Such as the stage to fight, the item to buy, etc. You can specify the value as `Input` or `Select` type:

```toml
[[tasks]]
type = "Fight"

# Select a stage to fight
[[tasks.variants]]
condition = { type = "DateTime", start = "2023-08-01T16:00:00", end = "2023-08-21T03:59:59" }

# Set the stage to a `Select` type with alternatives and description
[tasks.variants.params.stage]
# the alternatives of stage, at least one alternative should be given
# the element of alternatives can be a single value or a table with `value` and `desc` fields·
alternatives = [
    "SL-7", # will be displayed as "1. SL-7"
    { value = "SL-8", desc = "Manganese Ore" } # will be displayed as "2. SL-8 (Manganese Ore)"
]
default_index = 1 # the index of default value, start from 1, if not given, empty value will be re-prompt
description = "a stage to fight in summer event" # description of the input, optional
allow_custom = true # whether allow input custom value, default to false, if allow, non-integer value will be treated as custom value

# Task without input
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# Input a stage to fight
[[tasks.variants]]

# Set the stage to a `Input` type with default value and description
[tasks.variants.params.stage]
default = "1-7" # default value of stage, optional (if not given, user can input empty value to re-prompt)
description = "a stage to fight" # description of the input, optional

# query the medicine to use only when stage is 1-7
[tasks.variants.params.medicine]
# a parameter can be optional with `optional` field
# if the condition is not matched, the parameter will be ignored
# the `condition` field can be used to specify the condition of the parameter
# where the condition can be a table, whose keys are name of other parameters and values are the expected value
conditions = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

For an `Input` type, a prompt will be shown to ask the user to input a value. If the default value is given, it will be used if the user inputs an empty value, otherwise, it will re-prompt.
For `Select` type, a prompt will be shown to ask the user to input an index or custom value (if `allow_custom` is `true`). If the default index is given, it will be used if the user inputs an empty value, otherwise, it will re-prompt.

`--batch` option can be used to run tasks in batch mode, which will use the default value for all inputs and panic if no default value is given.

## MaaCore related configurations

The related configuration files of MaaCore is called "Profile" and located in `$MAA_CONFIG_DIR/profiles` directory. Each files in this directory is a profile, while the default profile is `default.toml`. If you want to use a profile other than the default one, you can specify it by `-p` or `--profile` option.

The currently available configurations are:

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

### Connection

The `connection` section is used to specify how to connect to the game:

```toml
[connection]
adb_path = "adb" # the path of adb executable, default to "adb", which means use adb in PATH
address = "emulator-5554" # the address of device, such as "emulator-5554" or "127.0.0.1:5555"
config = "General" # the config of maa, should not be changed most of time
```

`adb_path` is the path of `adb` executable, you can set it to the absolute path of `adb` or or leave it empty if it is in PATH. The `address` is the address of the device used by `adb`, like `emulator-5554` or `127.0.0.1:[port]`, the port of some common emulators can be found in the [MAA FAQ][emulator-ports]. If the `address` is absent, the cli will try to find the device automatically by `adb devices`, if there are multiple online devices, the first one will be used. If cli can not find any device, it will try to use the default address `emulator-5554`. The `config` is used to specify some configurations of the host and emulator, whose default value is `CompatMac` on macOS, `CompatPOSIXShell` on Linux and `General` on other platforms. More optional configs can be found in `config.json` in the resource directory.

For some common emulators, you can use `preset` to use predefined configurations:

```toml
[connection]
preset = "MuMuPro"
adb_path = "/path/to/other/adb" # override predefined adb executable path
address = "127.0.0.1:7777" # override the predefined address
```

Currently, there is only one preset `MuMuPro` for emulators. Issue and PR are welcome for the new preset.

There is a special preset `PlayCover`, used for the iOS app running on macOS by PlayCover. In this case, `adb_path` is ignored and `address` is used to specify the address of `MaaTools` set in `PlayCover`, more details can be found in the [PlayCover documentation][playcover-doc].

### Resource

The `resource` section is used to specify the resource to use:

```toml
[resource]
global_resource = "YoStarEN" # the global resource to use
platform_diff_resource = "iOS" # the platform diff resource to use
user_resource = true # whether use user resource
```

When your game is not in Simplified Chinese, you should set `global_resource` to non-Chinese resource. If you connect to the game with `PlayCover`, you should set `platform_diff_resource` to `iOS`.
Leave those two fields empty if you don't want to use global resource or platform diff resource. Besides, those two fields will also be set up automatically by maa-cli based on your task and connection type.
Lastly, if you want to use user resources, you should set `user_resource` to `true`. When `user_resource` is `true`, maa-cli will try to find user resources in `$MAA_CONFIG_DIR/resource` directory.

### Static options

The `static_options` section is used to configure MAA static options:

```toml
[static_options]
cpu_ocr = false # whether use CPU OCR, CPU OCR is enabled by default
gpu_ocr = 1 # the ID of your GPU, leave it to empty if you don't want to use GPU OCR
```

### Instance options

The `instance_options` section is used to configure MAA instance options:

```toml
[instance_options]
touch_mode = "ADB" # touch mode to use, can be "ADB", "MiniTouch", "MaaTouch" or "MacPlayTools" (only for PlayCover)
deployment_with_pause = false # whether pause the game when deployment
adb_lite_enabled = false # whether use adb-lite
kill_adb_on_exit = false # whether kill adb when exit
```

Note: If you connect to the game with `PlayCover`, the `touch_mode` will be ignored and `MacPlayTools` will be used.

## CLI related configurations

The CLI related configurations should be located in `$MAA_CONFIG_DIR/cli.toml`. Currently, it only contains one section: `core`:

```toml
# MaaCore install and update  configurations
[core]
channel = "Stable" # update channel, can be "Stable", "Beta" or "Alpha"
test_time = 0 # the time to test download mirrors in seconds, 0 to skip
# the url to query the latest version of MaaCore, leave it to empty to use default url
apit_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
[core.components]
library = true # whether install MaaCore library
resource = false # whether install resource resource

# CLI update configurations
[cli]
channel = "Stable" # update channel, can be "Stable", "Beta" or "Alpha"
# the url to query the latest version of maa-cli, leave it to empty to use default url
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# the url to download prebuilt binary, leave it to empty to use default url
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

[cli.components]
binary = true # whether install maa-cli binary


# hot update resource configurations
[resource]
auto_update = true # whether auto update resource before running task
backend = "libgit2" # the backend of resource, can be "libgit2" or "git"

# the remote of resource
[resource.remote]
branch = "main" # Branch of remote resource repository
# URL of remote resource repository, leave it empty to use the default URL
url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# If you want to use ssh, a certificate is needed which can be "ssh-agent" or "ssh-key"
# To use ssh-agent, set `use_ssh_agent` to true, and leave `ssh_key` and `passphrase` empty
# use_ssh_agent = true # Use ssh-agent to authenticate
# To use ssh-key, set `ssh_key` to path of ssh key,
ssh_key = "~/.ssh/id_ed25519" # Path of ssh key
# A Passphrase is needed if the ssh key is encrypted
passphrase = "password" # Passphrase of ssh key
# Store plain text password in configuration file is unsafe, so there are some ways to avoid it
# 1. set `passphrase` to true, then maa-cli will prompt you to input passphrase each time
# passphrase = true
# 2. set `passphrase` to a environment variable, then maa-cli will use the environment variable as passphrase
# passphrase = { env = "MAA_SSH_PASSPHRASE" }
# 3. set `passphrase` to a command, then maa-cli will execute the command to get passphrase
# which is useful when you use a password manager to manage your passphrase
# passphrase = { cmd = ["pass", "show", "ssh/id_ed25519"] }
```

**NOTE**:

- The `Alpha` channel of MaaCore is only available on Windows;
- The hot update resource can not work separately, it should be used with basic resources installed with MaaCore;
- If you want to use `git` backend, `git` command is required;
- If you want to fetch resources with ssh, the `ssh_key` is required;
- The `resource.remote.url` only affects first-time installation, it will be ignored when updating resource. If you want to change the remote URL, you should change it manually or delete the resource directory and reinstall the resources. The directory of the repository can be located by `maa dir hot-update`.

## Example of config file

- [Example configuration][example-config];
- [Configuration used by maintainer][wangl-cc-dotfiles].

## JSON schema

The JSON schema of configuration files can be found at [`schemas` directory][schema-dir]:

- The schema of the task configuration is [`task.schema.json`][task-schema];
- the schema of the MaaCore configuration file is [`asst.schema.json`][asst-schema];
- the schema of the CLI configuration file is [`cli.schema.json`][cli-schema].

With the help of JSON schema, you can get auto-completion and validation in some editors with plugins.

[task-types]: ../../protocol/integration.md#list-of-task-types
[emulator-ports]: ../../manual/connection.md#obtain-port-number
[playcover-doc]: ../../manual/device/macos.md#%E2%9C%85-playcover-the-software-runs-most-fluently-for-its-nativity-%F0%9F%9A%80
[example-config]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/config_examples
[wangl-cc-dotfiles]: https://github.com/wangl-cc/dotfiles/tree/master/.config/maa
[schema-dir]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/
[task-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/task.schema.json
[asst-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/asst.schema.json
[cli-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/cli.schema.json
