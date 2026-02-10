---
order: 4
icon: material-symbols:task
---

# Task Schema

Usage of `resource/tasks` and description of each field

::: tip
It is recommended to use [Visual Studio Code](https://code.visualstudio.com/) and install [Maa Pipeline Support](https://marketplace.visualstudio.com/items?itemName=nekosu.maa-support) extension for efficient editing. For more details, please visit the extension's homepage and the [docs](../develop/vsc-ext-tutorial.md).
:::

::: warning
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

        "pureColor": false,                 // Optional when method is HSVCount or RGBCount, default false
                                            // If true, ignore template matching score and rely solely on color matching results
                                            // Suitable for scenarios where color features are distinct but template matching performs poorly
                                            // When using this option, it is recommended to correspondingly increase the templThreshold

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

## Expression Calculation

Task list type fields (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`) support expression calculations.

|    Symbol    |                                         Meaning                                          |                 Example                 |
| :----------: | :--------------------------------------------------------------------------------------: | :-------------------------------------: |
|     `@`      |                                      `@`-type Task                                       |            `Fight@ReturnTo`             |
| `#` (unary)  |                                       Virtual task                                       |                 `#self`                 |
| `#` (binary) |                                       Virtual task                                       |          `StartUpThemes#next`           |
|     `*`      |                                       Repeat tasks                                       | `(ClickCornerAfterPRTS+ClickCorner)*10` |
|     `+`      | Task list merge (in next-type attributes, only first occurrence of same-name tasks kept) |                  `A+B`                  |
|     `^`      |             Task list difference (in first but not second, order preserved)              |   `(A+A+B+C)^(A+B+D)` (result is `C`)   |

Operators `@`, `#`, `*`, `+`, `^` have precedence: `#` (unary) > `@` = `#` (binary) > `*` > `+` = `^`.

## Special Task Types

### Template Task

**Template tasks** include derived tasks and `@`-type tasks. The core of the template task can be understood as **modifying the default values of fields** based on the _base task_.

#### Derived Task

Tasks with the `baseTask` field are derived tasks, and the task corresponding to the `baseTask` field is referred to as its _base task_ of this derived task. For a derived task,

1. If it is an template matching task, the default value for the `template` field remains `"TaskName.png"`;
2. If the `algorithm` field differs from its _base task_, the derived class parameters are not inherited (only parameters defined in `TaskInfo` are inherited);
3. The default values for all other fields are the corresponding fields of its _base task_.

#### Implicit `@`-type Task

If task `"A"` exists and there is not a task directly defined in any task file such as `"B@A"`, it is an implicit `@`-type task, and task `"A"` is called the _base task_ of task `"B@A"`. For an implicit `@`-type task,

1. The default values for task list type fields (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`) are set to the base task's corresponding fields with the `B@` prefix directly appended (if the task name starts with `#`, the `B` prefix is appended);
2. The default values for all other fields (including the `template` field) are the corresponding fields of its _base task_.

#### Explicit `@`-type Task

If task `"A"` exists and there is a task directly defined in the task file such as `"B@A"`, it is an explicit `@`-type task, and task `"A"` is called the _base task_ of task `"B@A"`. For an explicit `@`-type task,

1. The default values for task list type fields (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`) are set to the base task's corresponding fields with the `B@` prefix directly appended (if the task name starts with `#`, the `B` prefix is appended);
2. If it is an template matching task, the default value for the `template` field remains `"TaskName.png"`;
3. If the `algorithm` field differs from its _base task_, the derived class parameters are not inherited (only parameters defined in `TaskInfo` are inherited);
4. The default values for all other fields are the corresponding fields of its _base task_.

### Virtual Task (`#`-type Task)

Virtual tasks are tasks of the form `"#{sharp_type}"` or `"B#{sharp_type}"`, where `{sharp_type}` can be `none`, `self`, `back`, `next`, `sub`, `on_error_next`, `exceeded_next`, or `reduce_other_times`.

Virtual tasks can be divided into **command virtual tasks** (`#none` / `#self` / `#back`) and **field virtual tasks** (`#next`, etc.).

| Virtual Task Type |                       Meaning                       |                                                                    Simple example                                                                     |
| :---------------: | :-------------------------------------------------: | :---------------------------------------------------------------------------------------------------------------------------------------------------: |
|       none        |                     Empty Task                      |  Skip directly<sup>1</sup><br>`"A": {"next": ["#none", "T1"]}` is interpreted as `"A": {"next": ["T1"]}`<br>`"A#none + T1"` is interpreted as `"T1"`  |
|       self        |                  Current Task Name                  | `"A": {"next": "#self"}` in `"#self"` is interpreted as `"A"`<br>`"B": {"next": "A@B@C#self"}` in `"A@B@C#self"` is interpreted as `"B"`.<sup>2</sup> |
|       back        |                # Preceding task name                |                        `"A@B#back"` is interpreted as `"A@B"`<br>`"#back"` will be skipped if it appears directly<sup>3</sup>                         |
|  next, sub, etc.  | # The field corresponding to the previous task name |          Take `next` for example:<br>`"A#next"` is interpreted as `Task.get("A")->next`<br>`"#next"` will be skipped if it appears directly           |

_Note<sup>1</sup>: `"#none"` generally used in conjunction with the template task to add prefixes, or used in the `baseTask` field to avoid unnecessary fields in multi-file inheritance._

_Note<sup>2</sup>: `"XXX#self"` has the same meaning as `"#self"`._

_Note<sup>3</sup>: When several tasks have `"next": [ "#back" ]`, `"T1@T2@T3"` represents executing `T3`, `T2`, and `T1` in sequence._

### Multi-File Task

If a task defined in a later loaded task file (e.g. `tasks.json` for foreign services; hereinafter called File 2) also has a task of the same name defined in a earlier loaded task file (e.g. `tasks.json` for official services; hereinafter called File 1), then.

- if the task in File 2 does not have a `baseTask` field, then it inherits the fields of the task with the same name in File 1 directly.
- If the task in File 2 has a `baseTask` field, then it does not inherit the fields of the task with the same name in File 1, but overwrites them. In particular, when there is no template task, you can use `"baseTask": "#none"` to avoid inheriting unnecessary fields.

### Simple Example

- Derived task example (field `baseTask`)

  Assume the following two tasks are defined:

  ```json
  "Return": {
      "action": "ClickSelf",
      "next": [ "Stop" ]
  },
  "Return2": {
      "baseTask": "Return"
  },
  ```

  The parameters of the `"Return2"` task are directly inherited from the `"Return"` task, and actually include the following parameters:

  ```json
  "Return2": {
      "algorithm": "MatchTemplate", // Directly inherited
      "template": "Return2.png",    // "TaskName.png"
      "action": "ClickSelf",        // Directly inherited
      "next": [ "Stop" ]            // Directly inherited, without any prefix compared to the Template Task
  }
  ```

- `@`-type task example

  Assume that a task `"A"` with the following parameters is defined:

  ```json
  "A": {
      "template": "A.png",
      ...,
      "next": [ "N1", "#back" ]
  },
  ```

  If the task `"B@A"` is not directly defined, then the task `"B@A"` actually has the following parameters:

  ```json
  "B@A": {
      "template": "A.png",
      ...,
      "next": [ "B@N1", "B#back" ]
  }
  ```

  If the task `"B@A"` is defined as `"B@A": {}`, then the task `"B@A"` actually has the following parameters:

  ```json
  "B@A": {
      "template": "B@A.png",
      ...,
      "next": [ "B@N1", "B#back" ]
  }
  ```

- Virtual task example

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

### Precautions

If the tasks defined in the task list type field (`next`, etc.) contain low-priority operations, the actual results may not match expectations, so please be aware of this.

1. Special cases caused by the order of operations between `@` and double `#`

   ```json
   {
       "A": { "next": ["N0"] },
       "B": { "next": ["A#next"] },
       "C@A": { "next": ["N1"] }
   }
   ```

   In the above case, `"C@B" -> next` (i.e., `C@A#next`) is `[ "N1" ]` rather than `[ "C@N0" ]`.

2. Special cases caused by the order of operations between `@` and `+`

   ```json
   {
       "A": { "next": ["#back + N0"] },
       "B@A": {}
   }
   ```

   In the above case,

   ```cpp
   Task.get("A")->next = { "N0" };

   Task.get_raw("B@A")->next = { "B#back + N0" };
   Task.get("B@A")->next = { "B", "N0" }; // Note that it is not [ "B", "B@N0" ]
   ```

   In fact, you can use this feature to avoid adding unnecessary prefixes by simply defining

   ```json
   {
       "A": { "next": ["#none + N0"] }
   }
   ```

## Runtime Task Modification

- `Task.lazy_parse()` loads JSON task configuration files at runtime. Lazy_parse rules match [Multi-File Task](#multi-file-task).
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

## Schema Validation

This project configures JSON schema validation for `tasks.json`, schema file is `docs/maa_tasks_schema.json`.

### Visual Studio

Configured in `MaaCore.vcxporj`, works out of the box. Prompts are somewhat cryptic with some missing information.

### Visual Studio Code

Configured in `.vscode/settings.json`, use VS Code to open the **project folder** to use. Better prompt quality.
