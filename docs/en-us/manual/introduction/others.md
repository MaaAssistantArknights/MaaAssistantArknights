---
order: 11
---

# Others

::: important This page may be outdated.
:::

## GPU-accelerated inference

Use DirectML to call the GPU for recognition and inference acceleration.<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9236)</sup>It can reduce a significant amount of CPU usage with minimal GPU usage, and it's recommended to enable it.

Testing has shown that some graphics cards may encounter recognition issues when using this feature due to lacking certain functions or having lower performance. MAA has already built-in a blacklist for some GPUs.<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9990)</sup>If graphics cards not on the list also encounter recognition issues after enabling this feature, please submit an issue.

## Only Once

Configuration changes in the main interface and settings are usually saved automatically, but the following types will reset after MAA is restarted.

- Options marked with a `*`
- Options marked with `(Only Once)`
- Semi-selected switches obtained by right-clicking the checkbox

## Automatically switch configurations on startup

MAA supports automatic configuration switching through launch parameters by appending `--config <configuration name>` after the MAA process name.
Example: `./MAA/MAA.exe --config Official`.

Some symbols need to be changed, following Json conventions.
Example: when the configuration name is `"Official"`, the parameter should be `--config \"Official\"`.

## Pre/Post-Script

Starting from v4.13.0, it is possible to set pre/post-script that automatically executes batch files before and after the task.

You need to provide the absolute or relative path to the batch file, which should have a `.bat` extension.

## Configuration Migration

In the Windows version, all MAA configurations are stored in the `gui.json` file within the `config` folder. Migrating this folder will transfer all MAA settings.

## Note

- Tasks order can be changed on the UI. So can the shifting order in the infrastructure.
- All clicking event is randomized within a region, following Poisson distribution (higher probability at the center, lower probability around).
- Developed in C++, the core algorithm supports multi-level cache, to reduce CPU and memory usage as much as possible.
- Our software supports auto-update ✿✿ ヽ(°▽°)ノ ✿ Beta-testers can try beta versions, which update faster and less buggy (maybe).
- If auto-update fails, you can download the OTA package manually and put the ZIP file under the same directory. The update will start automatically.
