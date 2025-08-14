---
order: 4
icon: material-symbols:task
---

# Task Schema

Usage of `resource/tasks` and description of each field

::: tip
Please note that JSON files do not support comments. The comments in this document are for demonstration purposes only. Do not copy them directly into your JSON files.
:::

## Complete Field Reference

```json
{
    "TaskName": {                           // Task name, using @ may indicate a special task, default values will differ, see Special Task Types below

        "baseTask": "xxx",                  // Use xxx task as template to generate tasks, see Base Task in Special Task Types for details

        "algorithm": "MatchTemplate",       // Optional, indicates the recognition algorithm type
                                            // Default is MatchTemplate when not specified
                                            //      - JustReturn:       No recognition, directly execute action
                                            //      - MatchTemplate:    Match image
                                            //      - OcrDetect:        Text recognition
                                            //      - FeatureMatch:     Feature matching

        "action": "ClickSelf",              // Optional, indicates action after recognition
                                            // Default is DoNothing when not specified
                                            //      - ClickSelf:        Click on recognized position (random point within target area)
                                            //      - ClickRect:        Click on specified area, corresponds to specificRect field, not recommended
                                            //      - DoNothing:        Do nothing
                                            //      - Stop:             Stop current task
                                            //      - Swipe:            Swipe, corresponds to specificRect and rectMove fields
                                            //      - Input:            Input text, requires algorithm to be JustReturn, corresponds to inputText field

        "sub": ["SubTaskName1", "SubTaskName2"],
                                            // Optional, subtasks, not recommended. Will execute each subtask after completing current task
                                            // Can be nested, with subtasks having their own subtasks. Be careful not to create infinite loops

        "subErrorIgnored": true,            // Optional, whether to ignore subtask errors
                                            // Default false when not specified
                                            // When false, if a subtask errors, subsequent tasks won't execute (equivalent to current task erroring)
                                            // When true, subtask errors don't affect execution

        "next": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, indicates tasks to execute after completing current task and subtasks
                                            // Will check from first to last and execute first matching task
                                            // Default is to stop after completing current task when not specified
                                            // For identical tasks, second recognition won't occur after first:
                                            //     "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // JustReturn type tasks must not be in non-last positions

        "maxTimes": 10,                     // Optional, maximum execution count for this task
                                            // Default is infinity when not specified
                                            // After reaching max count, will execute exceededNext if present, otherwise stop

        "exceededNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, tasks to execute after reaching maximum execution count
                                            // When not specified, stops after max count; when specified, executes these instead of next

        "onErrorNext": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, tasks to execute when an error occurs

        "preDelay": 1000,                   // Optional, delay in milliseconds after recognition before executing action; default 0
        "postDelay": 1000,                  // Optional, delay in milliseconds after executing action before recognizing next; default 0

        "roi": [0, 0, 1280, 720],           // Optional, recognition area in format [x, y, width, height]
                                            // Auto-scales to 1280 * 720 resolution; default [0, 0, 1280, 720] when not specified
                                            // Try to specify this to reduce recognition area, improve performance and recognition speed

        "cache": true,                      // Optional, whether to use cache for this task, default true
                                            // After first recognition, will always check at same position, greatly improves performance
                                            // Only suitable for targets whose position never changes, set false if target position may change

        "rectMove": [0, 0, 0, 0],           // Optional, target movement after recognition, not recommended. Auto-scales to 1280 * 720
                                            // For example, if A is recognized but actual click target is 10 pixels below in 5 * 2 area,
                                            // can use [0, 10, 5, 2], but if possible try to directly recognize target position instead
                                            // Additionally, when action is Swipe, indicates endpoint (required)

        "reduceOtherTimes": ["OtherTaskName1", "OtherTaskName2"],
                                            // Optional, reduces execution count of other tasks after execution
                                            // For example, if you used a sanity potion, previous blue start button click didn't work, so reduce by 1

        "specificRect": [100, 100, 50, 50],
                                            // Required when action is ClickRect, indicates specified click area (random point within)
                                            // Required when action is Swipe, indicates starting point
                                            // Auto-scales to 1280 * 720 resolution
                                            // When algorithm is "OcrDetect", specificRect[0] and specificRect[1] represent grayscale threshold limits

        "specialParams": [int, ...],        // Parameters for special recognizers
                                            // Optional when action is Swipe, [0] for duration, [1] for extra swipe toggle

        "highResolutionSwipeFix": false,    // Optional, whether to enable high-resolution swipe fix
                                            // Currently only needed for stage navigation which doesn't use unity swipe method
                                            // Default is false
    
        /* Fields below only valid when algorithm is MatchTemplate */

        "template": "xxx.png",              // Optional, image file to match, can be string or string list
                                            // Default "TaskName.png"
                                            // Template files can be in template folder or subfolders, recursive search when loading

        "templThreshold": 0.8,              // Optional, threshold score for image template matching, can be number or number list
                                            // Default 0.8, check actual score in logs

        "maskRange": [1, 255],              // Optional, grayscale mask range for matching. array<int, 2>
                                            // For example, paint parts of image that shouldn't be recognized black (grayscale 0)
                                            // Then set [1, 255] to ignore black parts during matching

        "colorScales": [                    // Required when method is HSVCount or RGBCount, color mask ranges
            [                               // list<array<array<int, 3>, 2>> / list<array<int, 2>>
                [23, 150, 40],              // Structure is [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              //     Inner layer is int for grayscale,
            ],                              //         or array<int, 3> for three-channel color (RGB or HSV based on method)
            ...                             //     Middle layer array<*, 2> is color (or grayscale) lower and upper limits
        ],                                  //     Outer layer represents different color ranges, recognition area is their union on template image

        "colorWithClose": true,             // Optional when method is HSVCount or RGBCount, default true
                                            // Whether to use morphological closing on mask range for color counting
                                            // Closing fills small black dots, generally improves color matching but set false for text

        "method": "Ccoeff",                 // Optional, template matching algorithm, can be list
                                            // Default is Ccoeff when not specified
                                            //      - Ccoeff:       Color-insensitive template matching, corresponds to cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     Color-sensitive template matching,
                                            //                      Binary thresholds target area and template using colorScales,
                                            //                      Calculates similarity using F1-score in RGB color space,
                                            //                      Then dot products result with Ccoeff result
                                            //      - HSVCount:     Similar to RGBCount, but uses HSV color space

        /* Fields below only valid when algorithm is OcrDetect */

        "text": [ "接管作战", "代理指挥" ],  // Required, text to recognize, match is successful if any one matches ("接管作战" = "Take over operation", "代理指挥" = "Proxy command")

        "ocrReplace": [                     // Optional, replacements for commonly misrecognized text (supports regex)
            [ "千员", "干员" ],              // ("千员" = "thousand staff", "干员" = "operator")
            [ ".+击干员", "狙击干员" ]       // (".+击干员" = "any-hit operator", "狙击干员" = "sniper operator")
        ],

        "fullMatch": false,                 // Optional, whether text recognition requires exact match (no extra characters), default false
                                            // When false, substring is sufficient: e.g., text: [ "开始" ], recognized "开始行动" is success
                                            // When true, must recognize exactly "开始", not one character more

        "isAscii": false,                   // Optional, whether text content is ASCII characters
                                            // Default false

        "withoutDet": false,                // Optional, whether to skip detection model
                                            // Default false

        /* Fields below only valid when algorithm is OcrDetect and withoutDet is true */

        "useRaw": true,                     // Optional, whether to use original image for matching
                                            // Default true, false for grayscale matching

        "binThreshold": [140, 255],         // Optional, threshold for grayscale binarization (default [140, 255])
                                            // Pixels with grayscale out of range are considered background, excluded from text area
                                            // Final text foreground includes pixels in [lower, upper] range

        /* Fields below only valid when algorithm is JustReturn and action is Input */

        "inputText": "A string text.",      // Required, text to input as string
        
        /* Fields below only valid when algorithm is FeatureMatch */

        "template": "xxx.png",              // Optional, image file to match, can be string or string list
                                            // Default "TaskName.png"

        "count": 4,                         // Required feature point count (threshold), default = 4

        "ratio": 0.6,                       // KNN matching algorithm distance ratio, [0 - 1.0], higher means looser matching, easier line connection. Default 0.6

        "detector": "SIFT",                 // Feature point detector type, options: SIFT, ORB, BRISK, KAZE, AKAZE, SURF; default = SIFT
                                            // SIFT: High computational complexity, has scale and rotation invariance. Best effect.
                                            // ORB: Very fast computation, has rotation invariance but no scale invariance.
                                            // BRISK: Very fast computation, has scale and rotation invariance.
                                            // KAZE: Suitable for 2D and 3D images, has scale and rotation invariance.
                                            // AKAZE: Fast computation, has scale and rotation invariance.
    }
}
```

## Special Task Types

### Template Task (`@` Type Task)

Template task and base task are collectively called **template tasks**.

Allows using task "A" as template, with "B@A" representing a task generated from "A".

- If task "B@A" is not explicitly defined in `tasks.json`, then add `B@` prefix to `sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes` fields (or `B` if task name begins with `#`), all other parameters match task "A". For example, if task "A" has:

    ```json
    "A": {
        "template": "A.png",
        ...,
        "next": [ "N1", "N2" ]
    }
    ```

    This would be equivalent to defining:

    ```json
    "B@A": {
        "template": "A.png",
        ...,
        "next": [ "B@N1", "B@N2" ]
    }
    ```

- If task "B@A" is defined in `tasks.json`, then:
    1. If "B@A" has different `algorithm` from "A", derived class parameters are not inherited (only `TaskInfo` parameters inherited)
    2. For image matching tasks, if `template` not explicitly defined, uses `B@A.png` (not inheriting "A" template name); otherwise any derived parameters not explicitly defined inherit directly from "A"
    3. For `TaskInfo` base class parameters (common to all task types like `algorithm`, `roi`, `next`, etc.), if not explicitly defined in "B@A", inherit directly from "A" except for the five fields mentioned above which get "B@" prefixed when inherited

### Base Task

Base task and template task are collectively called **template tasks**.

Tasks with `baseTask` field are base tasks.

Base task logic takes precedence over template task. This means `"B@A": { "baseTask": "C", ... }` has no relation to task A.

Any parameter not explicitly defined uses the corresponding parameter from `baseTask` without prefix, except `template` which remains `"TaskName.png"` when not explicitly defined.

#### Multi-File Tasks

If a task defined in a later-loaded file (e.g., foreign server `tasks.json`, called File 2) is also defined in an earlier-loaded file (e.g., CN server `tasks.json`, called File 1), then:

- If File 2's task has no `baseTask` field, it directly inherits fields from File 1's same-name task.
- If File 2's task has a `baseTask` field, it doesn't inherit fields from File 1's same-name task, but overwrites them instead.

### Virtual Task (`#` Task)

Virtual tasks are also called sharp tasks (`#` type tasks).

Tasks with `#` in name are virtual tasks. `#` can be followed by `next`, `back`, `self`, `sub`, `on_error_next`, `exceeded_next`, `reduce_other_times`.

|  Virtual Task Type  |        Meaning        |                                                                 Simple Example                                                                 |
| :----------: | :----------------: | :--------------------------------------------------------------------------------------------------------------------------------------: |
|     self     |      Parent task name      | `"A": {"next": "#self"}` where `"#self"` interpreted as `"A"`<br>`"B": {"next": "A@B@C#self"}` where `"A@B@C#self"` interpreted as `"B"`<sup>1</sup> |
|     back     |   Task name before #   |                                      `"A@B#back"` interpreted as `"A@B"`<br>`"#back"` directly appears will be skipped                                       |
| next, sub, etc. | Field of task name before # |                      For example with `next`:<br>`"A#next"` interpreted as `Task.get("A")->next`<br>`"#next"` directly appears will be skipped                       |

_Note<sup>1</sup>: `"XXX#self"` has same meaning as `"#self"`._

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

Results in:

```cpp
Task.get("C")->next = { "B@N1", "B@N2" };

Task.get("B@Loading")->next = { "B@Loading", "Other", "B" };
Task.get("Loading")->next = { "Loading" };
Task.get_raw("B@Loading")->next = { "B#self", "B#next", "B#back" };
```

#### Some Use Cases

- When several tasks have `"next": [ "#back" ]`, `"Task1@Task2@Task3"` means execute `Task3`, then `Task2`, then `Task1` in sequence.

#### Other Related Cases

```json
{
    "A": { "next": ["N0"] },
    "B": { "next": ["A#next"] },
    "C@A": { "next": ["N1"] }
}
```

In this case, `"C@B" -> next` (i.e., `C@A#next`) is `[ "N1" ]`, not `[ "C@N0" ]`.

## Runtime Task Modification

- `Task.lazy_parse()` loads JSON task configuration files at runtime. Lazy_parse rules match [Multi-File Tasks](#multi-file-tasks).
- `Task.set_task_base()` modifies a task's `baseTask` field.

### Usage Example

Consider this task configuration:

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

The following code changes task "A" based on mode value, also affecting tasks that depend on "A", like "B@A":

```cpp
switch (mode) {
case 1:
    Task.set_task_base("A", "A_mode1");  // Basically equivalent to directly replacing A with A_mode1 content
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

|    Symbol     |                           Meaning                           |                  Example                  |
| :---------: | :------------------------------------------------------: | :------------------------------------: |
|     `@`     |                         Template task                         |            `Fight@ReturnTo`            |
| `#` (unary) |                          Virtual task                          |                `#self`                 |
| `#` (binary) |                          Virtual task                          |          `StartUpThemes#next`          |
|     `*`     |                       Repeat multiple tasks                       | `(ClickCornerAfterPRTS+ClickCorner)*5` |
|     `+`     | Task list merge (in next-type attributes, only first occurrence of same-name tasks kept) |                 `A+B`                  |
|     `^`     |         Task list difference (in first but not second, order preserved)         |   `(A+A+B+C)^(A+B+D)` (result is `C`)    |

Operators `@`, `#`, `*`, `+`, `^` have precedence: `#` (unary) > `@` = `#` (binary) > `*` > `+` = `^`.

## Schema Validation

This project configures JSON schema validation for `tasks.json`, schema file is `docs/maa_tasks_schema.json`.

### Visual Studio

Configured in `MaaCore.vcxporj`, works out of the box. Prompts are somewhat cryptic with some missing information.

### Visual Studio Code

Configured in `.vscode/settings.json`, use VSCode to open the **project folder** to use. Better prompt quality.
