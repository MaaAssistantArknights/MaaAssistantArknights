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

The specific task types and parameters can be found in the [MAA Integration Document][task-types]. Note that maa-cli does not validate parameter names and values, and no error message will be generated even if an error occurs unless MaaCore detects an error at runtime.

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

**Note**: If the `filename` field is a relative path, it will be relative to `$MAA_CONFIG_DIR/infrast`. Additionally, the custom infrastructure plan file will not be read by `maa-cli` but by MaaCore. Therefore, the file format must be `JSON` and the time period defined in the file will not be used to select the corresponding sub-plan. You must specify the `plan_index` field in the task parameters to use the correct infrastructure plan in the corresponding time period. This ensures that the correct infrastructure plan is used at the appropriate time.

Besides the `Time` condition, there are also `DateTime`, `Weekday`, and `DayMod` conditions.
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

All the above conditions related to time have a `timezone` field, which can be an offset from UTC (e.g., `8` for UTC+8) or a client type (e.g., `"Official"`). Note that the official server timezone is UTC+4, not UTC+8, because the game day starts at 04:00:00 rather than 00:00:00. If timezone is omitted, your local system timezone is used.

In addition to the above conditions, there's a `OnSideStory` condition that uses hot-update resources to check if there are any active events. For example, the condition to farm `SL-8` during a summer event can be simplified to `{ type = "OnSideStory", client = "Official" }`, where `client` is your game client type. Using this condition means you only need to update the stage to farm when an event changes, without manually editing event dates.

Besides the basic conditions, you can use `{ type = "And", conditions = [...] }`, `{ type = "Or", conditions = [...] }`, and `{ type = "Not", condition = ... }` for logical combinations.

By combining these conditions, you can define infrastructure plans spanning multiple days. Here's an example of 6 plans for 2 days:

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
        # The divisor specifies the period, the remainder specifies the offset
        # The offset equals num_days_since_ce % divisor
        # num_days_since_ce is the number of days since Common Era (0001-01-01 is day 1)
        # You can get the current day's offset with `maa remainder <divisor>`
        # For 2024-01-27, num_days_since_ce is 738,912
        # The offset is 738,912 % 2 = 0, so this condition matches on 2024-01-27
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
# Note: we must use Or condition here to properly handle the time across midnight
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

With the default strategy, if multiple variants are matched, only the first one will be used. If no condition is given, the variant will always be matched. So you can put a variant without condition at the end as a default case.

The strategy for matching variants can be changed with the `strategy` field:

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

This example will yield the same stage selection as the previous one, but will additionally use expiring medicine on Sunday night.
With the `merge` strategy, if multiple variants are matched, the parameters of all matched variants will be merged. If multiple variants have the same parameter keys, the later ones will override earlier ones.

If no variant is matched, the task will not be executed,
which is useful when you want to only run a task under certain conditions:

```toml
# Mall after 18:00
[[tasks]]
type = "Mall"

[[tasks.variants]]
condition = { type = "Time", start = "18:00:00" }
```

### User Input

For some tasks, you might want to input values at runtime rather than hardcoding them in the task file. You can set parameters to `Input` or `Select` type:

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
default_index = 1 # the index of default value, start from 1, if not given, empty input will re-prompt
description = "a stage to fight in summer event" # description of the input, optional
allow_custom = true # whether to allow custom value input, default is false; if true, non-integer inputs are treated as custom values

# Task without input
[[tasks.variants]]
condition = { type = "Weekday", weekdays = ["Tue", "Thu", "Sat"] }
params = { stage = "CE-6" }

# Input a stage to fight
[[tasks.variants]]

# Set the stage to an `Input` type with default value and description
[tasks.variants.params.stage]
default = "1-7" # default value of stage, optional (if not given, empty input will re-prompt)
description = "a stage to fight" # description of the input, optional

# query the medicine to use only when stage is 1-7
[tasks.variants.params.medicine]
# a parameter can be conditional based on other parameter values
# the condition uses a table where keys are other parameter names and values are the expected values
# this condition means "only prompt for medicine when stage is 1-7"
conditions = { stage = "1-7" }
default = 1000
description = "medicine to use"
```

For an `Input` type, a prompt will be shown to ask the user to input a value. If a default value is given, it will be used when the user inputs an empty value; otherwise, it will re-prompt.
For a `Select` type, a prompt will be shown to ask the user to input an index or custom value (if `allow_custom` is `true`). If a default index is given, it will be used when the user inputs an empty value; otherwise, it will re-prompt.

The `--batch` option can be used to run tasks in batch mode, which will use default values for all inputs and error if no default value is given.

## MaaCore Related Configurations

The MaaCore configuration files are called "Profiles" and located in the `$MAA_CONFIG_DIR/profiles` directory. Each file in this directory is a profile, with the default being `default.toml`. To use a different profile, specify it with the `-p` or `--profile` option.

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

The `connection` section specifies how to connect to the game:

```toml
[connection]
adb_path = "adb" # path to adb executable, default is "adb" (uses PATH)
address = "emulator-5554" # device address like "emulator-5554" or "127.0.0.1:5555"
config = "General" # connection configuration, rarely needs changing
```

`adb_path` is the path to the `adb` executable - you can set its absolute path or leave it empty to use it from PATH. Most emulators include `adb`, so you can use their built-in version without installing separately. `address` is the address used by `adb`. For emulators, use `127.0.0.1:[port]` - common emulator ports are listed in the [FAQ][emulator-ports]. If no address is specified, maa-cli tries to find a device with `adb devices`, using the first one found or defaulting to `emulator-5554` if none are found. `config` specifies platform/emulator configurations - defaults to `CompatPOSIXShell` on Linux, `CompatMac` on macOS, and `General` on other platforms. More options are in the resource folder's `config.json`.

For common emulators, you can use `preset` for predefined configurations:

```toml
[connection]
preset = "MuMuPro" # Use MuMu Pro emulator preset
adb_path = "/path/to/adb" # Override the preset's adb path if needed (optional)
address = "127.0.0.1:7777" # Override the preset's address if needed (optional)
```

Currently only `MuMuPro` preset is available. Issues and PRs for more presets are welcome.

#### Special Presets

There are two special presets: `PlayCover (macOS)` and `Waydroid (Linux)`

- `PlayCover` is for connecting to iOS apps running natively on macOS through PlayCover. In this case, `adb_path` is ignored and `address` is the address of `PlayTools`. See [PlayCover documentation][playcover-doc] for details.

- `Waydroid` is for connecting to Android apps running natively on Linux through Waydroid. `adb_path` is still required. See [Waydroid documentation][waydroid-doc] for details.

### Resource

The `resource` section specifies which resources MaaCore should load:

```toml
[resource]
global_resource = "YoStarEN" # Non-Chinese resources
platform_diff_resource = "iOS" # Non-Android resources
user_resource = true # Whether to load user-defined resources
```

When using a non-Simplified Chinese game client, set `global_resource` to load non-Chinese resources. When using an iOS client, set `platform_diff_resource` to `iOS`. Both are optional - leave empty to not load these resources. These are also auto-set based on your `startup` task's `client_type` and when using `PlayTools` connection. To load user-defined resources, set `user_resource` to `true`, which loads resources from `$MAA_CONFIG_DIR/resource`.

### Static options

The `static_options` section configures MaaCore static options:

```toml
[static_options]
cpu_ocr = false # Whether to use CPU OCR (enabled by default)
gpu_ocr = 1 # GPU ID for GPU OCR; leave empty to use CPU OCR
```

### Instance options

The `instance_options` section configures MaaCore instance options:

```toml
[instance_options]
touch_mode = "ADB" # Touch mode: "ADB", "MiniTouch", "MaaTouch", or "MacPlayTools"
deployment_with_pause = false # Whether to pause game during deployment
adb_lite_enabled = false # Whether to use adb-lite
kill_adb_on_exit = false # Whether to kill adb on exit
```

Note: When using `PlayTools` connection, `touch_mode` is forced to `MacPlayTools` regardless of setting.

## CLI Related Configurations

CLI-related configurations should be in `$MAA_CONFIG_DIR/cli.toml`. Current configurations include:

```toml
# MaaCore install and update configurations
[core]
channel = "Stable" # Update channel: "Alpha", "Beta", or "Stable" (default)
test_time = 0 # Time to test mirror speeds in seconds; 0 to skip, default is 3
# API URL to query latest MaaCore version; leave empty for default
api_url = "https://github.com/MaaAssistantArknights/MaaRelease/raw/main/MaaAssistantArknights/api/version/"

# Component installation config (not recommended to change)
[core.components]
library = true # Whether to install MaaCore library
resource = true # Whether to install MaaCore resources

# CLI update configurations
[cli]
channel = "Stable" # Update channel: "Alpha", "Beta", or "Stable" (default)
# API URL to query latest maa-cli version; leave empty for default
api_url = "https://github.com/MaaAssistantArknights/maa-cli/raw/version/"
# URL to download prebuilt binaries; leave empty for default
download_url = "https://github.com/MaaAssistantArknights/maa-cli/releases/download/"

[cli.components]
binary = true # Whether to install maa-cli binary

# Resource hot update configurations
[resource]
auto_update = true # Whether to auto-update resources before running tasks
warn_on_update_failure = true # Whether to warn instead of error on update failure
backend = "libgit2" # Hot update backend: "git" or "libgit2"

# Remote repository configuration
[resource.remote]
branch = "main" # Branch of remote repository
# Repository URL; leave empty for default
# GitHub repositories support both HTTPS and SSH; HTTPS recommended
url = "https://github.com/MaaAssistantArknights/MaaResource.git"
# url = "git@github.com:MaaAssistantArknights/MaaResource.git"
# If using SSH, you need to provide authentication:
# 1. Using ssh-agent (recommended)
# use_ssh_agent = true # Use ssh-agent for authentication
# 2. Using SSH key file
ssh_key = "~/.ssh/id_ed25519" # Path to SSH key
# If your key is password-protected:
passphrase = "password" # Passphrase for SSH key
# Storing plaintext passwords is insecure. Alternatives:
# 1. Set passphrase to true to be prompted each time
# passphrase = true
# 2. Use an environment variable
# passphrase = { env = "MAA_SSH_PASSPHRASE" }
# 3. Use a command output (useful with password managers)
# passphrase = { cmd = ["pass", "show", "ssh/id_ed25519"] }
```

**NOTE**:

- The `Alpha` channel for MaaCore is only available on Windows
- Hot update resources require the basic resources installed with MaaCore
- Using the `git` backend requires the `git` command to be available
- SSH authentication requires either `ssh_key` configuration or `ssh-agent`
- The `resource.remote.url` only affects first installation; to change it later, modify it manually or delete and reinstall resources. Get the repository location with `maa dir hot-update`.

## Example Configuration Files

- [Example configurations][example-config]
- [Maintainer's configuration][wangl-cc-dotfiles]

## JSON Schema

JSON schemas for configuration files are in the [`schemas` directory][schema-dir]:

- Task configuration schema: [`task.schema.json`][task-schema]
- MaaCore configuration schema: [`asst.schema.json`][asst-schema]
- CLI configuration schema: [`cli.schema.json`][cli-schema]

With these schemas, you can get auto-completion and validation in supported editors with plugins.

[task-types]: ../../protocol/integration.md#list-of-task-types
[emulator-ports]: ../../manual/connection.md#obtain-port-number
[playcover-doc]: ../../manual/device/macos.md#%E2%9C%85-playcover-the-software-runs-most-fluently-for-its-nativity-%F0%9F%9A%80
[waydroid-doc]: ../../manual/device/linux.md#✅-waydroid
[example-config]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/config_examples
[wangl-cc-dotfiles]: https://github.com/wangl-cc/dotfiles/tree/master/.config/maa
[schema-dir]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/
[task-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/task.schema.json
[asst-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/asst.schema.json
[cli-schema]: https://github.com/MaaAssistantArknights/maa-cli/blob/main/crates/maa-cli/schemas/cli.schema.json
