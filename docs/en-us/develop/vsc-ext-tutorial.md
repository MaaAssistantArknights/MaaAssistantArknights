---
order: 8
icon: iconoir:code-brackets
---

# Dedicated VS Code Extension Tutorial

The extension provides a series of convenient development capabilities for MaaAssistantArknights/MaaFramework, including but not limited to the following features:

- tasks.json support, including `template preview`, `next jump`, `task reference`, etc.
- Screenshot / Crop image

For details, please visit [Extension Store](https://marketplace.visualstudio.com/items?itemName=nekosu.maa-support) or [Repository](https://github.com/neko-para/maa-support-extension).

## Installation

Searching `Maa` and installing it from VS Code extensions list is recommended.

::: tip
When used for the first time, the extension will automatically download the preset version of resources.
Search command `Maa: select fetch registry` to switch the download source (npm / cnpm).
:::

## Features

### Control Panel

A dedicated control panel is added to the left, with icon ![MaaSupport ControlPanel](/images/maa-support-panel.svg)

The major features of the extension are based on the `interface.json` configuration. Select the activated `interface.json` on the top of the control panel.

The extension has a `Maa` compatiable mode. If `src/MaaCore` folder exists inside the opened workspace, the mode will be enabled automatically.

### Semantic resource analysis

Select expected resource via the `Resource` select inside the control panel. The extension will index and diagnose resource via corresponding paths.

If you find that the editing json isn't hinted by the extension, please check if the activated resource contains it.

> The term `definitions of task / task defs` refers to key props of task objects.
>
> The term `references of task / task refs` refers to values containing task name (e.g. in `next`) in other task objects.

#### Query task defs / task refs

The extension supports Jump to task defs, Jump to task refs and View task defs.

When enabling the `Maa` compatible mode, the extension will be able to parse `template task`, supporting querying task defs and task refs in conjunction with base tasks. Images that have the same name as the task will be shown when hovering the task defs.

Use `Ctrl+T` to quickly query and jump to a task definition.

#### Query / open images

The extension supports open images.

When enabling the `Maa` compatible mode, the extension will be able to recursively search for the image.

#### Task completion

The extension supports autocompletion according to all known tasks.

When enabling the `Maa` compatible mode, typing `@` will trigger completion.

#### Image path completion

The extension supports autocompletion according to the path of all known images.

When enabling the `Maa` compatible mode, the extension will be able to recursively search for the image.

#### Check tasks / image paths

The extension supports scheduled scanning and diagnosing all tasks.

- Check if contains task defs with same names.
- Check if contains unknown task refs.
- Check if contains unknown image refs.
- Check if contains duplicated task refs in a single task.

#### Multiple paths resource support

The extension supports resource with multiple paths. The extension will perform logical overlapping according to specified order, thus content loaded later can reference content loaded earlier.

### Evaluate Task / Tsak List Expression (Only Maa)

Evaluating the expanded task object and the sources of each properties, and the result of the task list expression via the control panel.

### MaaPiCli feature (Only MaaFramework projects)

Scanning and selecting controllers, selecting resource, adding and manipulating tasks, and launching tasks can be done via the control panel.

### Take screenshots and crop images / Quick recognition

Searching and launching `Maa: open crop tool` inside VS Code command panel can open `Screenshots / Crop` panel.

> Use `Ctrl+Shift+P` (`Command+Shift+P` on macOS) to open command panel

- After selecting and connecting to the controller, use `Screencap` button to obtain screenshots
- Use `Upload` button to manually upload images.
- Hold `Ctrl` key and select cropping area
- Use wheels to zoom
- After finishing cropping, use `Download` button to save the cropping result to the folder of the topest layer of the activated resource
- Use `Copy` button to copy the ROI as an array to the clipboard.
- Click `Tool` button to open the recognition tool panel, where you can directly test recognition on the current image.

::: warning

If OCR recognition returns an empty result, please check whether the [OCR model](https://github.com/MaaXYZ/MaaFramework/blob/main/docs/en_us/1.1-QuickStarted.md#text-recognition-model-files) is configured correctly.

For MAA, the extension will maintain the models used automatically; you only need to select the proper resource.

:::

### Log View

#### MaaFramework Log

Search and execute `Maa: open maa log` in the VS Code command panel to view the `maa.log` generated during debugging.

#### Maa Pipeline Support Extension Log

Search and execute `Maa: open extension log` in the VS Code command panel to view the `mse.log` generated during debugging.

### Bottom status bar

#### MaaSupport \[Extension Version\]

Click to reveal control panel

#### MaaFramework [MaaFw Version]

Click to switch `MaaFramework` version used by the extension. The selectable versions are limited to those supported by the current extension. If the version you need is not in the list, please consider changing the extension version.
