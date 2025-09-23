---
order: 8
icon: iconoir:code-brackets
---

# Dedicated VSCode Extension Tutorial

* [Extension Store](https://marketplace.visualstudio.com/items?itemName=nekosu.maa-support)
* [Repository](https://github.com/neko-para/maa-support-extension)

## Installation

Searching `Maa` and installing it from VSCode extensions list is recommended.

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

When enabling the `Maa` compatible mode, the extension will be able to parse `template task`, supporting querying task defs and task refs in conjunction with base tasks. Images that has the same name of the task will be shown when hovering the task defs.

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

* Check if contains task defs with same names.
* Check if contains unknown task refs.
* Check if contains unknown image refs.
* Check if contains duplicated task refs in a single task.

#### Multiple paths resource support

The extension supports resource with multiple paths. The extension will perform logical overlapping according to specified order, thus content loaded later can reference content loaded earlier.

### Evaluate Task / Tsak List Expression (Only Maa)

Evaluating the expanded task object and the sources of each properties, and the result of the task list expression via the control panel.

### MaaPiCli feature (Only MaaFramework projects)

Scanning and selecting controllers, selecting resource, adding and manipulating tasks, and launching tasks can be done via the control panel.

### Take screenshots and crop images

Searching and launching `Maa: open crop tool` inside VSCode command panel can open `Screenshots / Crop` panel.

> Use `Ctrl+Shift+P` (`Command+Shift+P` on MacOS) to open command panel

* After selecting and connecting to the controller, use `Screencap` button to obtain screenshots
* Use `Upload` button to manually upload images.
* Hold `Ctrl` key and select cropping area
* Use wheels to zoom
* After finishing cropping, use `Download` button to save the cropping result to the folder of the topest layer of the activated resource

### Bottom status bar

#### MaaSupport \[Extension Version\]

Click to reveal control panel

#### MaaFramework \[MaaFw Version\]

Click to switch `MaaFramework` version used by the extension

> When used for the first time, the preset version will be downloaded.
>
> Search command `Maa: select fetch registry` to switch downloading source (npm / cnpm).
