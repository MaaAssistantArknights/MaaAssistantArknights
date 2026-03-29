---
order: 8
icon: 'iconoir:code-brackets'
---
# VS Code Extension Tutorial

This extension provides a series of convenient development capabilities for MaaAssistantArknights/MaaFramework, including but not limited to the following features:

- tasks.json support, including `template preview`, `next jump`, `task references`, etc.
- Image capture/cropping

For specific content, please visit the [Extension Marketplace](https://marketplace.visualstudio.com/items?itemName=nekosu.maa-support) or the [Repository](https://github.com/neko-para/maa-support-extension).

## Installation

It is recommended to search for `Maa` directly in the VS Code extension list to install.

::: tip
On first use, the extension will automatically download the preset version of resources.
Search for the command `Maa: Select Download Source` to switch the download source (npm / cnpm).
:::

## Features

### Control Panel

A dedicated control panel is added on the left, with the icon ![MaaSupport ControlPanel](/images/maa-support-panel.svg).

Most of the extension's features are based on the `interface.json` configuration. At the top of the control panel, you can select the active `interface.json`.

The extension has a `Maa` compatibility mode. It will be automatically enabled when a `src/MaaCore` folder is detected in the opened workspace.

### Semantic Resource Analysis

Select the desired resource by switching the `Resource` dropdown in the control panel. The extension will index and perform diagnostics based on the corresponding path.

If you find that the JSON file you want to edit doesn't have extension hints, please check if the active resource does not include that file.

> The so-called `task definition` refers to the key of a task object.
>
> The so-called `task reference` refers to values in other tasks where a task name can be entered (e.g., in `next`).

#### Query Task Definition/Reference

The extension supports jumping to definition, jumping to references, and viewing task definitions.

When `Maa` compatibility mode is enabled, it can parse `template tasks`, supporting linked base class queries for task definitions and references; hovering over a task definition allows viewing images with the same name.

Use the `Ctrl+T` shortcut key to quickly query and jump to task definitions.

#### Query/Open Images

The extension supports opening images.

When `Maa` compatibility mode is enabled, recursive search is allowed for image paths.

#### Task Completion

The extension supports auto-completion based on all known tasks.

When `Maa` compatibility mode is enabled, typing `@` will trigger completion.

#### Image Path Completion

The extension supports auto-completion based on all known image paths.

When `Maa` compatibility mode is enabled, recursive search is allowed for image paths.

#### Validate Task/Image Paths

The extension supports scheduled scanning and analysis of all tasks.

- Check for duplicate task definitions
- Check for unknown task references
- Check for unknown image references
- Check for duplicate task references within a single task

#### Multi-path Resource Support

The extension supports resources containing multiple paths, which are logically overlaid in the specified order, meaning later-loaded content can reference earlier-loaded content.

### Compute Task / Task List Expression (Maa only)

Through the control panel, you can compute the actual expanded content of a task and the source of each item; you can also compute the result of expanding a task list expression.

### MaaPiCli Features (MaaFramework projects only)

Through the control panel, you can scan and select controllers, select resources, add and manage tasks, and execute tasks.

### Screenshot Cropping / Quick Recognition

Search for and execute the command `Maa: Open Screenshot Tool` in the VS Code command palette to open the `Screenshot / Crop` panel.

> Use `Ctrl+Shift+P` (or `Command+Shift+P` on macOS) to open the command palette.

- After selecting and connecting a controller, use the `Screenshot` button to directly capture a screenshot.
- Use the `Upload` button to manually upload an image.
- Hold the `Ctrl` key and drag to select the area to crop.
- Use the mouse wheel to zoom.
- After cropping, use the `Download` button to automatically save the cropped result to the top-level image directory of the active resource.
- Use the `Copy` button to copy the ROI as an array to the clipboard.
- Press the `Tool` button to open the recognition tool panel, where you can directly perform recognition tests on the current image.

::: warning

If the OCR recognition result is empty, please check if the [OCR model](https://github.com/MaaXYZ/MaaFramework/blob/main/docs/zh_cn/1.1-%E5%BF%AB%E9%80%9F%E5%BC%80%E5%A7%8B.md#%E6%96%87%E5%AD%97%E8%AF%86%E5%88%AB%E6%A8%A1%E5%9E%8B%E6%96%87%E4%BB%B6) is correctly configured.

For MAA, the extension will automatically maintain the model used; you just need to select the correct resource.

:::

### Log Viewing Features

#### MaaFramework Logs

Search for and execute the command `Maa: Open maa log` in the VS Code command palette to view the `maa.log` logs generated during debugging.

#### Maa Pipeline Support Extension Logs

Search for and execute the command `Maa: Open extension log` in the VS Code command palette to view the `mse.log` logs generated during debugging.

### Bottom Status Bar

#### MaaSupport `<Extension Version>`

Click to focus the control panel.

#### MaaFramework `<MaaFw Version>`

Click to switch the `MaaFramework` version used by the extension. The selectable versions are limited to those supported by the current extension version. If the version you need is not in the list, please consider changing the extension version.
