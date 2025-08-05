---
order: 1
icon: ri:guide-fill
---

# Getting Started

Quick start guide!

## Prerequisites

1. Confirm system version

    MAA on Windows only supports Windows 10 and 11. For older Windows versions, please refer to the system issues section in [FAQ](./faq.md#system-issues).

    Non-Windows users, please refer to [Emulator and Device Support](./device/).

2. Download the correct version

    The [MAA official website](https://maa.plus/) will automatically select the correct architecture for most users reading this article, which should be Windows x64.

3. Extract correctly

    Ensure the extraction is complete, and make sure to extract MAA to a separate folder. Do not extract MAA to paths requiring UAC permissions such as `C:\` or `C:\Program Files\`.

4. Install runtime libraries

    MAA requires VCRedist x64 and .NET 8. Please run `DependencySetup_依赖库安装.bat` in the MAA directory to install them.

    For more information, please refer to the pinned section in [FAQ](./faq.md).

5. Confirm emulator support

    Check [Emulator and Device Support](./device/) to verify the compatibility of your emulator.

6. Set the correct emulator resolution

    Emulator resolution should be landscape `1280x720` or `1920x1080`; for YostarEN players, it must be `1920x1080`.

## Initial Configuration

0. If you want to use automatic detection, run **one** emulator and ensure no other Android devices are connected to your computer.

1. Follow the setup guide for configuration. MAA will automatically detect the running emulator. If detection fails or you need multiple instances, refer to [Connection Settings](./connection.md).

2. ~~When running MAA for the first time, it will perform a hot update. Please close and restart MAA when prompted in the log on the right.~~

3. Drag the task list on the left to reorder tasks, and check or uncheck the checkboxes to select tasks to run.

4. Link Start!

## Advanced Configuration

Read the documentation!

## Other Notes

::: center
**When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!**
**When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!**
**When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!** &emsp;&emsp; **When encountering problems, read the documentation, read the documentation, read the documentation!**
:::

1. The logs are located in the `debug` folder inside the MAA directory. Logs are extremely important. When seeking help from others, always bring both `asst.log` and `gui.log` with you.

2. MAA provides many customization options to meet all users' various needs. If you think MAA is missing a feature, it might just be hard to find rather than non-existent, such as `Manual stage name input` and `Do not place stationed operators in dormitory`.

3. Hovering your mouse pointer over certain options will display detailed explanations, such as `Do not place stationed operators in dormitory`.

4. For some checkboxes, right-clicking will change them to a half-selected state. Checkboxes in this state will automatically clear when MAA is next started, which can be understood as `one-time only`.

5. ~~If you don't know what you need and can't easily resolve potential problems, do not use beta versions.~~
