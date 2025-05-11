---
order: 4
icon: material-symbols:task
---

# Task Schema

Usage of `resource/tasks` and description of each field

::: tip
Please note that JSON files do not support comments. The comments in the text are for demonstration purposes only and should NOT be copied directly.
:::

## Overview

```json
{
    "TaskName" : {                          // Task name, when it has @ it may be a special task, default field values may differ, see Special Task Types below for details

        "baseTask": "xxx",                  // Use xxx task as a template to generate tasks, see Base Task in the special task type section below

        "algorithm": "MatchTemplate",       // Optional, indicating the type of recognition algorithm
                                            // defaults to MatchTemplate when not filled in
                                            // - JustReturn: Execute the action directly without recognition
                                            // - MatchTemplate: match the image
                                            // - OcrDetect: text recognition
                                            // - Hash: Hash calculation

        "action": "ClickSelf",              // Optional, indicates the action after recognition
                                            // defaults to DoNothing if not filled in
                                            // - ClickSelf: Click on the recognized location (a random point within the recognized target range)
                                            // - ClickRect: Click on the specified area, corresponds to the specificRect field, not recommended to use this option
                                            // - DoNothing: Do nothing
                                            // - Stop: Stop the current task
                                            // - Swipe: slide, corresponds to the specificRect and rectMove fields
                                            // - Input: Input text, requires algorithm to be JustReturn, corresponds to the inputText field

        "sub": ["SubTaskName1", "SubTaskName2"],
                                            // Optional, subtasks, not recommended. Will execute each subtask in turn after the current task is executed
                                            // Can be nested, subtasks can have their own subtasks. But be careful not to create a dead loop

        "subErrorIgnored": true,            // Optional, whether to ignore subtask errors.
                                            // default false if not filled
                                            // If false, if a subtask has an error, it will not continue to execute subsequent tasks (equivalent to this task having an error)
                                            // When true, it does not affect whether a subtask has an error or not

        "next": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, indicating the next task to be executed after the current task and the subtask are executed
                                            // will be identified from front to back, and the first matching one will be executed
                                            // default stop after the current task is executed if not filled in
                                            // For the same task, the second time will not be recognized after the first recognition:
                                            //     "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // Do not allow JustReturn type tasks to be located in non-last positions

        "maxTimes": 10,                     // Optional, indicates the maximum number of times the task can be executed
                                            // default infinite if not filled
                                            // When the maximum number of times is reached, if the exceededNext field exists, execute exceededNext; otherwise, stop the task directly

        "exceededNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, indicates the task to be executed when the maximum number of executions is reached
                                            // If not filled in, it will stop when the maximum number of executions is reached; if filled in, it will execute these tasks instead of those in next

        "onErrorNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, indicating the subsequent tasks to be performed in case of execution errors

        "preDelay": 1000,                   // Optional, indicates how long to delay after recognition before executing the action, in milliseconds; default 0 if not filled in
        "postDelay": 1000,                  // Optional, indicates how long to delay after executing the action before recognizing next, in milliseconds; default 0 if not filled in

        "roi": [0, 0, 1280, 720],           // Optional, indicating the range of recognition, in the format [ x, y, width, height ]
                                            // Auto-scaling based on 1280 * 720; default [ 0, 0, 1280, 720 ] if not filled in
                                            // Try to fill this in, reducing the recognition range can reduce performance consumption and speed up recognition

        "cache": false,                     // Optional, indicates whether the task uses caching, default is false;
                                            // After the first recognition, only the first recognized position will be used for recognition from then on, enabling this can significantly save performance
                                            // but only suitable for tasks where the target position to be recognized never changes, set to false if the target position may change

        "rectMove": [0, 0, 0, 0],           // Optional, target movement after recognition, not recommended to use this option. Auto-scaling based on 1280 * 720
                                            // For example, if A is recognized, but you actually want to click somewhere in the 5 * 2 area 10 pixels below A,
                                            // you could fill in [ 0, 10, 5, 2 ], if possible try to directly recognize the position to be clicked, not recommended to use this option
                                            // Additionally, when action is Swipe this is required and indicates the swipe end point.

        "reduceOtherTimes": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, reduces the execution count of other tasks after execution.
                                            // For example, if a sanity potion was used, it means the last click on the blue start button didn't work, so the blue start action count needs to be reduced by 1

        "specificRect": [100, 100, 50, 50],
                                            // Required when action is ClickRect, indicates the specified click position (a random point within the range).
                                            // Required when action is Swipe, indicates the swipe starting point.
                                            // Auto-scaling based on 1280 * 720
                                            // When algorithm is "OcrDetect", specificRect[0] and specificRect[1] represent the grayscale lower and upper threshold values.

        "specialParams": [int, ...],        // Parameters needed for some special recognizers
                                            // Additionally, optional when action is Swipe, [0] indicates duration, [1] indicates whether to enable extra swipe

        /* The following fields are only valid when algorithm is MatchTemplate */

        "template": "xxx.png",              // Optional, the name of the image file to match, can be a string or list of strings
                                            // Default is "TaskName.png"

        "templThreshold": 0.8,              // Optional, threshold value for image template matching score, recognition is considered successful if above this threshold, can be a number or list of numbers
                                            // Default is 0.8, you can check the actual score in the logs

        "maskRange": [1, 255],              // Optional, grayscale mask range for matching. array<int, 2>
                                            // For example, parts of the image that don't need to be recognized can be painted black (grayscale value 0)
                                            // Then setting this to [ 1, 255 ] will ignore the black parts during matching

        "colorScales": [                    // Required when method is HSVCount or RGBCount, color mask range. 
            [                               // list<array<array<int, 3>, 2> | array<int, 2>>
                [23, 150, 40],              // Structure is [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              //     Inner layer is int for grayscale,
            ],                              //     　　is array<int, 3> for three-channel color, method determines whether it is RGB or HSV;
            ...                             //     Middle layer array<*, 2> is the color (or grayscale) lower and upper limits:
        ],                                  //     Outermost layer represents different color ranges, region to be recognized is the union of their corresponding masks on the template image.

        "colorWithClose": true,             // Optional, effective when method is HSVCount or RGBCount, default is true
                                            // Whether to use morphological closing to process the mask range when counting colors.
                                            // Closing operation can fill small black dots, generally improving color counting match results, but if the image contains text, setting to false is recommended

        "method": "Ccoeff",                 // Optional, template matching algorithm, can be a list
                                            // Default is Ccoeff if not specified
                                            //      - Ccoeff:       Template matching algorithm insensitive to color, corresponds to cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     Color-sensitive template matching algorithm,
                                            //                      First binarizes the region to be matched and template image based on colorScales,
                                            //                      Calculates similarity in RGB color space using F1-score as metric,
                                            //                      Then dot products the result with Ccoeff result
                                            //      - HSVCount:     Similar to RGBCount, but uses HSV color space

        /* The following fields are only valid when algorithm is OcrDetect */

        "text": [ "Take over", "Auto Deploy" ],  // Required, text content to recognize, considered successful if any match

        "ocrReplace": [                     // Optional, replacements for commonly misrecognized text (supports regex)
            [ "Ooerator", "Operator" ],
            [ ".+iper", "Sniper" ]
        ],

        "fullMatch": false,                 // Optional, whether text recognition requires exact matching (no extra characters), default is false
                                            // When false, substring matching is sufficient: e.g., if text: [ "Start" ], recognition of "Start Action" is considered successful;
                                            // When true, must recognize exactly "Start", not even one extra character

        "isAscii": false,                   // Optional, whether the text content to recognize is ASCII characters
                                            // Default false if not filled in

        "withoutDet": false,                // Optional, whether to not use the detection model
                                            // Default false if not filled in

        /* The following fields are only valid when algorithm is JustReturn and action is Input */

        "inputText": "A string text."       // Required, the text to input, as a string

    }
}
```

## Special Task Types

### Template Task (`@` Type Task)

Template task and base task are collectively called **Template tasks**.

Allows a task "A" to be used as a template, and then "B@A" represents the task generated from "A".

- If task "B@A" is not explicitly defined in `tasks.json`, then add `B@` prefix to the `sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes` fields (or `B` if the task name starts with `#`) and the rest of the parameters are the same as task "A". That is, if task "A" has the following parameters:

    ```json
    "A": {
        "template": "A.png",
        ...,
        "next": [ "N1", "N2" ]
    }
    ```

    It's equivalent to also defining:

    ```json
    "B@A": {
        "template": "A.png",
        ...,
        "next": [ "B@N1", "B@N2" ]
    }
    ```

- If task "B@A" is defined in `tasks.json`, then:
    1. If "B@A" has a different `algorithm` field from "A", derived class parameters are not inherited (only the parameters defined by `TaskInfo` are inherited)
    2. For image matching tasks, if `template` is not explicitly defined it becomes `B@A.png` (rather than inheriting "A"'s `template` name), in other cases any derived class parameters not explicitly defined are inherited directly from task "A"
    3. For parameters defined in the `TaskInfo` base class (parameters common to all task types, such as `algorithm`, `roi`, `next`, etc.), if not explicitly defined in "B@A", all parameters are inherited directly from task "A" except the five fields mentioned above like `sub`, which will have "B@" prefix added when inherited

### Base Task

Base task and template task are collectively called **template tasks**.

A task with the `baseTask` field is a base task.

Base task logic takes precedence over template task. This means that `"B@A": { "baseTask": "C", ... }` has no relation to task A.

Any parameter not explicitly defined will directly use the corresponding parameter from the `baseTask` without adding a prefix, except for `template` which remains `"TaskName.png"` if not explicitly defined.

#### Multi-File Tasks

If a task defined in a later loaded task file (e.g., foreign server `tasks.json`; referred to as file two) is also defined in a previously loaded task file (e.g., official server `tasks.json`; referred to as file one), then:

- If the task in file two does not have a `baseTask` field, it directly inherits the fields from the same-named task in file one.
- If the task in file two has a `baseTask` field, it does not inherit the fields from the same-named task in file one, but directly overwrites them.

### Virtual Task

Virtual task is also called sharp task (`#` type task).

A task with `#` in its name is a virtual task. `#` can be followed by `next`, `back`, `self`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times`.

| Virtual Task Type | Meaning | Simple Example |
|:---------:|:---:|:--------:|
| self | Parent task name | In `"A": {"next": "#self"}`, `"#self"` is interpreted as `"A"`<br>In `"B": {"next": "A@B@C#self"}`, `"A@B@C#self"` is interpreted as `"B"`.<sup>1</sup> |
| back | Task name before # | `"A@B#back"` is interpreted as `"A@B"`<br>`"#back"` appearing directly will be skipped |
| next, sub, etc. | Field corresponding to the task name before # | Taking `next` as an example:<br>`"A#next"` is interpreted as `Task.get("A")->next`<br>`"#next"` appearing directly will be skipped |

_Note<sup>1</sup>: `"XXX#self"` has the same meaning as `"#self"`._

#### Simple Example

```json
{
    "A": { "next": ["N1", "N2"] },
    "C": { "next": ["B@A#next"] },

    "Loading": {
        "next": ["#self", "#next", "#back"]
    },
    "B": {
        "next": ["Other", "B@Loading"]
    }
}
```

This gives:

```cpp
Task.get("C")->next = { "B@N1", "B@N2" };

Task.get("B@Loading")->next = { "B@Loading", "Other", "B" };
Task.get("Loading")->next = { "Loading" };
Task.get_raw("B@Loading")->next = { "B#self", "B#next", "B#back" };
```

#### Some Uses

- When several tasks have `"next": [ "#back" ]`, `"Task1@Task2@Task3"` represents executing `Task3`, `Task2`, `Task1` in that order.

#### Other Related Examples

```json
{
    "A": { "next": ["N0"] },
    "B": { "next": ["A#next"] },
    "C@A": { "next": ["N1"] }
}
```

In this case, `"C@B" -> next` (i.e., `C@A#next`) is `[ "N1" ]` not `[ "C@N0" ]`.

## Runtime Task Modification

- `Task.lazy_parse()` can load JSON task configuration files at runtime. The lazy_parse rules are the same as for [Multi-File Tasks](#multi-file-tasks).
- `Task.set_task_base()` can modify a task's `baseTask` field.

### Usage Example

Suppose you have the following task configuration file:

```json
{
    "A": {
        "baseTask": "A_default"
    },
    "A_default": {
        "next": ["xxx"]
    },
    "A_mode1": {
        "next": ["yyy"]
    },
    "A_mode2": {
        "next": ["zzz"]
    }
}
```

The following code can change task "A" based on the value of mode, and will also change other tasks that depend on task "A", such as "B@A":

```cpp
switch (mode) {
case 1:
    Task.set_task_base("A", "A_mode1");  // Essentially equivalent to directly replacing A with A_mode1's content, same below
    break;
case 2:
    Task.set_task_base("A", "A_mode2");
    break;
default:
    Task.set_task_base("A", "A_default");
    break;
}
```

## Expression Calculation

| Symbol | Meaning | Example |
|:---------:|:---:|:--------:|
| `@` | Template task | `Fight@ReturnTo` |
| `#` (unary) | Virtual task | `#self` |
| `#` (binary) | Virtual task | `StartUpThemes#next` |
| `*` | Repeat tasks | `(ClickCornerAfterPRTS+ClickCorner)*5` |
| `+` | Task list merge (in next series properties, only the leftmost occurrence of the same task is kept) | `A+B` |
| `^` | Task list difference (in the former but not in the latter, order preserved) | `(A+A+B+C)^(A+B+D)` (result is `C`) |

Operators `@`, `#`, `*`, `+`, `^` have precedence: `#` (unary) > `@` = `#` (binary) > `*` > `+` = `^`.

## Schema Validation

This project has configured JSON schema validation for `tasks.json`, the schema file is `docs/maa_tasks_schema.json`.

### Visual Studio

It has been configured in `MaaCore.vcxproj`, ready to use out of the box. The hints are somewhat cryptic and some information is missing.

### Visual Studio Code

It has been configured in `.vscode/settings.json`, just open the **project folder** with VSCode to use it. The hints are better.
