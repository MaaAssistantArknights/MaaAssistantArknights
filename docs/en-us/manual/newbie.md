---
order: 1
icon: ri:guide-fill
---

# Getting Started

Quick start guide!

::: info About This Documentation
This manual primarily covers **Windows MAA (WPF desktop client)**. The MAA Team mainly maintains the Windows WPF UI. maa-cli and the macOS version are maintained by fewer developers and may lag behind or differ in features.

Some features (such as Depot Maintain, scheduled execution, and drag-and-drop package updates) are implemented by the UI layer and are not included in the core (MaaCore). When using maa-cli, the macOS version, or third-party UIs that call MaaCore directly, these features may be unavailable and must be implemented separately. Pages that cover UI-only features are marked at the top.
:::

## Prerequisites

::::steps

1. Confirm system version

   MAA on Windows only supports Windows 10 and 11. For older Windows versions, please refer to the system issues section in [FAQ](./faq.md#system-issues).

   Non-Windows users, please refer to [Emulator and Device Support](./device/).

2. Confirm emulator support

   Check [Emulator and Device Support](./device/) to verify the compatibility of your emulator.

3. Download and installation

   See [Download & Installation](./install.md).

4. Set the correct emulator resolution

   Emulator resolution should be landscape `1280x720` or `1920x1080`; for YostarEN players, it must be `1920x1080`.

::::

## Initial Configuration

0. If you want to use automatic detection, run **one** emulator and ensure no other Android devices are connected to your computer.

1. Follow the setup guide for configuration. MAA will automatically detect the running emulator. If detection fails or you need multiple instances, refer to [Connection Settings](./connection.md).

2. MAA automatically checks for and updates game resources on startup; updates are loaded automatically when the system is idle, without requiring a restart.

3. Drag the task list on the left to reorder tasks, and check or uncheck the checkboxes to select tasks to run. Click the gear icon on the right of a task to expand its detailed settings.

4. Link Start!

## Advanced Configuration

Read the documentation!

## Other Notes

::: center
**When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!**
**When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!**
**When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!**
:::

1. Use `Settings` - `Issue Report` - `Generate Support Payload` to generate a log package convenient for sharing. Logs are extremely important. When seeking help from others, always bring the log package with you.
   If MAA fails to start entirely and you cannot access the settings panel, manually go to the MAA installation directory (the folder containing `MAA.exe`) and package the `debug`, `config`, and `cache` folders yourself.
   Feedback: [GitHub Issues](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues) (also accessible via `Settings` - `Issue Report` - `Issues`).

2. MAA provides many customization options to meet all users' various needs. If you think MAA is missing a feature, it might just be hard to find rather than non-existent, such as `Manual stage name input` and `Do not place stationed operators in dormitory`.

3. For some checkboxes, right-clicking will change them to a half-selected state. Checkboxes in this state will automatically clear when MAA is next started, which can be understood as `one-time only`.

4. ~~If you don't know what you need and can't easily resolve potential problems, do not use beta versions.~~
