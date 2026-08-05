---
order: 11
icon: icon-park-solid:other
---

# Others

::: info UI-Only Feature
Most features on this page are implemented by the UI layer. See [Getting Started](../newbie.md#about-this-documentation) for details.
:::

## GPU-Accelerated Inference

Uses DirectML to call the GPU for recognition inference acceleration<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9236)</sup>, reducing CPU usage significantly with minimal GPU usage. Recommended to enable.

Testing has shown that some graphics cards may experience recognition issues when using this feature due to missing functionality or lower performance. MAA has built-in a blacklist for certain GPUs<sup>[PR1](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9990)[PR2](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12134)</sup>. If you encounter recognition issues with GPUs not on the blacklist after enabling this feature, please submit an Issue.

## One-Time Only

Configuration changes in the main interface and settings are usually saved automatically, but the following will reset after MAA restarts:

- Options marked with an `*`
- Options marked with `(One-time only)`
- Half-selected checkboxes obtained by right-clicking

Many checkboxes support right-clicking for a half-selected state (one-time only, resets after MAA restart). Common use cases include:

- **Main task list**: Right-click a task to half-select it — it will run this time but revert after restart. By default, right-click means "apply once"; this can be switched to "skip once" (i.e., the half-selected task is skipped this time) in `Settings` - `GUI` - `Reverse right-click effect for main task`.
- **Combat settings**: `Use Sanity Potion`, `Use Originium` and similar options become one-time only when half-selected via right-click.

## Auto-Switch Configuration at Startup

MAA supports automatically switching configurations via launch parameters by adding `--config <configuration name>` after the MAA process name. Example: `./MAA/MAA.exe --config Official`.

Some characters need to be escaped according to JSON conventions. Example: when the configuration name is `"Official"`, the parameter should be `--config \"Official\"`.

## Start/End Scripts

Since v4.13.0, MAA supports setting scripts to run before starting and after finishing tasks. Enter the absolute or relative path to a batch file (`.bat`) in the input field.

## Configuration Migration

In the Windows version, all MAA configurations are stored in the `config` folder. Migrating this folder transfers all MAA settings.

## Scheduled Execution

In `Settings` - `Scheduled Execution`, you can set up to 8 timed tasks. Each includes an enable toggle, time (hour:minute), and a corresponding configuration.

- When the scheduled time is reached, MAA will automatically connect to the emulator and start executing the tasks checked in the current configuration.
- Each timer's checkbox supports right-click half-select (one-time only), effective only for the current session and resetting after MAA restart.

### Force Scheduled Start

When checked, MAA will **stop the currently running task**, restart the game, and start a new task. Useful for handling the daily 04:00 (server local time) flash update — after the flash update, it forcibly restarts the game and re-executes tasks to prevent them from getting stuck.

To automatically reconnect and continue tasks after a flash update, also enable the `Start Up` task.

### Custom Configuration Selection

When checked, each timer item can specify a different configuration. MAA will restart and switch to the corresponding configuration **two minutes before** the scheduled trigger, then start executing tasks.

## Update

MAA updates come in two types of content:

- **Software version**: Updates to the main program's UI and features.
- **Resource version**: Material data updates, including new stage maps, drop item icons, recruitment operator tags, etc. Software releases typically include the latest resources at the time of release.

::: tip Navigation Hot Update
There is also a "navigation hot update" that triggers automatically to update the main interface prompts and event stage entries. A notification appears in the top-right corner upon success.
:::

### Update Channel

You can switch update channels in `Settings` - `Update Settings`:

- **Stable**: Fewer bugs, slower support for new features and themes. New features may take weeks to be supported.
- **Beta**: May have minor bugs, faster support for new features and themes.
- **Nightly**: Contains unstable features, for testing only. Do not discuss nightly issues elsewhere.

::: tip
If the interface prompts that your version is too low and the minimum required version is beta, but you are already on the latest version, this means the feature is currently only available in the beta channel. Wait for the stable release or switch to beta to get it.
:::

### Auto Update

In `Settings` - `Update Settings`, you can choose the update source:

| Update Source | Description |
| :--- | :--- |
| **Global Source** (GitHub) | Software version pulled from GitHub; resource version uses [MirrorChyan](https://mirrorchyan.com/en/projects?rid=MAA) for free update detection, manually click `Resource Update` to download from GitHub. |
| **MirrorChyan** | Enter CDK to enable automatic updates for both software and resources, prioritizing high-speed CDN downloads. More stable and faster. |

You can toggle the following options in update settings:

- `Check for updates on startup`
- `Scheduled update check` (server local time 04:00 / 22:00, corresponding to daily reset and full sanity recovery times)
- `Auto-download update packages`: When disabled, only notifications are shown; you can download the package yourself and drag it into MAA (see Manual Update below).
- `Auto-install update packages`

When a resource update is detected during a task, it will be automatically loaded when the system is idle, taking effect on the next task.

### Manual Update

If automatic downloads fail or your network is poor, you can manually download the update package and **drag the `.zip` file directly into the MAA main window** to complete the update. MAA identifies the package type via the filename and processes it accordingly.

- Dragging **software update packages** (full / OTA): Supported since v6.8.0-beta.2.
- Dragging **resource update packages**: Supported since v6.16.5.

::: tip
Only packages downloaded from [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases) are supported. [MirrorChyan](https://mirrorchyan.com/en/projects?rid=MAA) packages have a different format and are for automatic updates only — **they cannot be dragged in**.
:::

#### Software Update Packages (Full / OTA)

- Download the corresponding update package from [GitHub Releases](https://github.com/MaaAssistantArknights/MaaAssistantArknights/releases). Do not modify the filename format:
  - **Full package**: `MAA-<version>-win-<arch>.zip`, e.g., `MAA-v5.20.0-win-x64.zip`.
  - **OTA package**: `MAAComponent-OTA-<current_version>_<target_version>-win-<arch>.zip`. The source version must match your current version.
- After dragging, MAA will automatically extract and apply the update on next restart.
- **Full package updates require manual confirmation**: Full packages clean old files in the installation directory (preserving `config`, `data`, `debug`, `cache`, `achievement`, `background`, etc.). Ensure MAA is installed in a standalone folder.

::: danger Full Package Update Risk
If you place MAA directly in a disk root, Desktop, Downloads, or mix it with other programs/personal files at the same directory level, full package updates may delete sibling files. Always install MAA in a standalone folder and manually back up before updating.
:::

::: details Traditional method (without drag-and-drop)
If your MAA version is too old to support drag-and-drop (before v6.8.0-beta.2), extract the full package into a **new** folder, then copy the `config` and `data` folders from the old directory to preserve your data. ⚠️ Do not overwrite the old folder directly — incorrect operations may cause resource corruption.
:::

#### Resource Update Packages

- Download the resource repository zip from [GitHub](https://github.com/MaaAssistantArknights/MaaResource/archive/refs/heads/main.zip).
- After dragging, MAA will verify the resource version (packages older than the current version are rejected), extract and merge into the `resource` directory, and automatically reload.
- Resource packages are incremental and do not delete existing resource files.
- Without drag-and-drop, you can also manually extract and overwrite the `resource` folder (⚠️ incremental content — do not delete the original folder).

#### Unsupported Packages

If the dragged file is not one of the above update packages (e.g., modified filename causing regex mismatch, version mismatch, etc.), MAA will display a notification that it cannot be recognized.

::: warning File contents are not verified
MAA only matches by filename and does not verify the actual contents of the archive. If you manually rename a file (e.g., renaming an arm64 package to x64), MAA will still extract it, potentially installing the wrong version. Do not modify the original filename of update packages.
:::

## Additional Notes

- Tasks on the left side of the main page can be dragged to change their order, as can facilities in the base management settings.
- All click operations target random positions within buttons, following a Poisson distribution (higher probability at the center, decreasing with distance from center).
- The core algorithms are developed in C++ with multi-level caching to minimize CPU and memory usage.
- The software supports automatic updates ✿✿ ヽ(°▽°)ノ ✿. We recommend non-critical users try the beta version, which typically updates faster and has fewer bugs. (What MIUI? (╯‵□′)╯︵┻━┻)
- If automatic downloads fail for new versions, you can manually download the OTA package and place it in the MAA directory for automatic updating.
