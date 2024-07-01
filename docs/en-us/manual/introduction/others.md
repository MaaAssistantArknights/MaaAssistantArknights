---
order: 11
---

# Others

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

## Automatically switch configurations on startup

MAA 支持通过启动参数自动切换配置，在 MAA 进程名后附加 `--config <配置名>` 即可。例子：`./MAA/MAA.exe --config 官服`。

部分符号需要转义，参考 Json。例子：在配置名为 `"官服"` 时，参数应为 `--config \"官服\"`。

## Pre/Post-Script

Starting from v4.13.0, it is possible to set pre/post-script that automatically executes batch files before and after the task.

You need to provide the absolute or relative path to the batch file, which should have a `.bat` extension.

## Note

- Tasks order can be changed on the UI. So can the shifting order in the infrastructure.
- Almost all configuration changes will be saved automatically, except for options with `*`, `(only once)` and half-select by right-clicking on the checkbox.
- All clicking event is randomized within a region, following Poisson distribution (higher probability at the center, lower probability around).
- Developed in C++, the core algorithm supports multi-level cache, in order to reduce CPU and memory usage as much as possible.
- Our software supports auto-update ✿✿ ヽ(°▽°)ノ ✿ Beta-testers can try beta versions, which updates faster and less buggy (maybe).
- If auto-update fails, you can download the OTA package manually and put the ZIP file under the same directory. The update will start automatically.
- 在 Windows 版本中，MAA 目录下 `config` 文件夹中的 `gui.json` 记录了所有设置，如果下载了新的完整包可以将此文件夹复制到新的 MAA 目录下。
