---
order: 11
icon: icon-park-solid:other
---

# Others

## GPU-Accelerated Inference

Uses DirectML to call the GPU for recognition inference acceleration<sup>[PR](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9236)</sup>, reducing CPU usage significantly with minimal GPU usage. Recommended to enable.

Testing has shown that some graphics cards may experience recognition issues when using this feature due to missing functionality or lower performance. MAA has built-in a blacklist for certain GPUs<sup>[PR1](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/9990)[PR2](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12134)</sup>. If you encounter recognition issues with GPUs not on the blacklist after enabling this feature, please submit an Issue.

## One-Time Only

Configuration changes in the main interface and settings are usually saved automatically, but the following will reset after MAA restarts:

- Options marked with an `*`
- Options marked with `(One-time only)`
- Half-selected checkboxes obtained by right-clicking

## Auto-Switch Configuration at Startup

MAA supports automatically switching configurations via launch parameters by adding `--config <configuration name>` after the MAA process name. Example: `./MAA/MAA.exe --config Official`.

Some characters need to be escaped according to JSON conventions. Example: when the configuration name is `"Official"`, the parameter should be `--config \"Official\"`.

## Start/End Scripts

Since v4.13.0, MAA supports setting scripts to run before starting and after finishing tasks. Enter the absolute or relative path to a batch file (`.bat`) in the input field.

## Configuration Migration

In the Windows version, all MAA configurations are stored in `gui.json` inside the `config` folder. Migrating this folder transfers all MAA settings.

## Additional Notes

- Tasks on the left side of the main page can be dragged to change their order, as can facilities in the base management settings.
- All click operations target random positions within buttons, following a Poisson distribution (higher probability at the center, decreasing with distance from center).
- The core algorithms are developed in C++ with multi-level caching to minimize CPU and memory usage.
- The software supports automatic updates ✿✿ ヽ(°▽°)ノ ✿. We recommend non-critical users try the beta version, which typically updates faster and has fewer bugs. (What MIUI? (╯‵□′)╯︵┻━┻)
- If automatic downloads fail for new versions, you can manually download the OTA package and place it in the MAA directory for automatic updating.
