---
order: 4
icon: material-symbols:task
---

# Task Schema

Usage of `resource/tasks` and description of each field

## Overview

```json
{
    "TaskName" : {                          // Task name

        "baseTask": "xxx",                  // use xxx task as a template to generate tasks, see Base Task in the special task type below for details

        "algorithm": "MatchTemplate",       // Optional, indicating the type of recognition algorithm
                                            // defaults to MatchTemplate when not filled in
                                            // - JustReturn: Execute the action directly without recognition
                                            // - MatchTemplate: match the image
                                            // - OcrDetect: text recognition
                                            // - Hash: Hash calculation

        "action": "ClickSelf",              // Optional, indicates the action after recognition
                                            // defaults to DoNothing if not filled in
                                            // - ClickSelf: Click on the recognized location (a random point within the recognized target range)
                                            // - ClickRand: Click on a random position in the whole screen
                                            // - ClickRect: Click on the specified area, that corresponds to the specificRect field, not recommended to use this option
                                            // - DoNothing: Do nothing
                                            // - Stop: Stop the current task
                                            // - Swipe: slide, corresponds to the specificRect and rectMove fields

        "sub": [ "SubTaskName1", "SubTaskName2" ],
                                            // Optional, subtasks. Will execute each subtask in turn after the current task is executed
                                            // Optional, sub-tasks and then sub-tasks. But be careful not to write a dead loop

        "subErrorIgnored": true,            // Optional, if or not to ignore subtask errors.
                                            // default false if not filled
                                            // If false, if a subtask has an error, it will not continue to execute subsequent tasks (equivalent to this task having an error)
                                            // When true, it does not affect whether a subtask has an error or not

        "next": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Optional, indicating the next task to be executed after the current task and the subtask are executed
                                            // will be identified from front to back, and the first matching one will be executed
                                            // default stop after the current task is executed
                                            // For the same task, the second time will not be recognized after the first recognition.
                                            // "next": [ "A", "B", "A", "A" ] -> "next": [ "A", "B" ]
                                            // Do not allow JustReturn type tasks to be located in a non-last item

        "maxTimes": 10,                     // Optional, indicates the maximum number of times the task can be executed
                                            // default infinite if not filled
                                            // When the maximum number of times is reached, if the exceededNext field exists, the task will be executed as exceededNext; otherwise, the task will be stopped.

        "exceededNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Optional, indicates the task to be executed when the maximum number of executions is reached
                                            // If not filled in, it will stop when the maximum number of executions is reached; if filled in, it will be executed here, not in next
        "onErrorNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Optional, indicating the subsequent tasks to be performed in case of execution errors

        "preDelay": 1000,                   // Optional, indicates how long the action will be executed after it is recognized, in milliseconds; default 0 if not filled in
        "postDelay": 1000,                  // Optional, indicates how long the action is delayed after execution before it is recognized next, in milliseconds; default 0 if not filled in

        "roi": [ 0, 0, 1280, 720 ],         // Optional, indicating the range of recognition, in the format [ x, y, width, height ]
                                            // Auto-scaling to 1280x720; default when not filled [ 0, 0, 1280, 720 ]
                                            // Fill in as much as possible, reducing the recognition range can reduce performance consumption and speed up recognition

        "cache": false,                      // Optional, indicates whether the task uses caching or not, default is false;
                                            // After the first recognition, only the first recognized position will be recognized forever, enable to save performance significantly
                                            // but only for tasks where the location of the target to be recognized will not change at all, set to false if the location of the target to be recognized will change

        "rectMove": [ 0, 0, 0, 0 ],         // Optional, target movement after recognition, not recommended Auto-scaling with 1280 * 720 as base
                                            // For example, if A is recognized, but the actual location to be clicked is somewhere in the 10 pixel 5 * 2 area below A.
                                            //Then you can fill in [ 0, 10, 5, 2 ], and try to recognize the position to be clicked directly if you can, this option is not recommended
                                            // Additional, valid and mandatory when action is Swipe, indicates the end of the slide.

        "reduceOtherTimes": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Optional; executes to reduce the execution count of other tasks.
                                            // For example, if you take a sanity pill, it means that the last click on the blue start action button did not take effect, so the blue start action is -1

        "specificRect": [ 100, 100, 50, 50 ],
                                            // Valid and mandatory when action is ClickRect, indicates the specified click position (a random point in the range).
                                            // Valid and mandatory when action is Swipe, it means the starting point of swipe.
                                            // Auto-scaling with 1280 * 720 as base
                                            // When the algorithm is "OcrDetect", specificRect[0] and specificRect[1] represent the grayscale upper and lower threshold values.

        "specialParams": [ int, ... ],      // Parameters needed for some special recognizers
                                            // extra, optional when action is Swipe, [0] for duration, [1] for whether to enable extra sliding

        "highResolutionSwipeFix": false,    // Optional. Whether to enable high-resolution swipe fix.
                                            // Currently, only stage navigation does not use the Unity swipe method, so it needs to be enabled there.
                                            // Default is false

        /* The following fields are only valid if the algorithm is MatchTemplate */

        "template": "xxx.png",              // Optional, the name of the image file to be matched
                                            // default "TaskName.png"

        "templThreshold": 0.8,              // Optional, a threshold value for image template matching score, above which the image is considered recognized.
                                            // default 0.8, you can check the actual score according to the log

        "maskRange": [ 1, 255 ],            // Optional, the grayscale mask range.
                                            // For example, the part of the image that does not need to be recognized will be painted black (grayscale value of 0)
                                            // Then set "maskRange" to [ 1, 255 ], to instantly ignore the blacked-out parts when matching

        "colorScales": [                    // Effective and required when method is HSVCount or RGBCount, color mask range. 
            [                               // list<array<array<int, 3>, 2>> / list<array<int, 2>>
                [23, 150, 40],              // Structure is [[lower1, upper1], [lower2, upper2], ...]
                [25, 230, 150]              //     Inner layer is int if grayscale,
            ],                              //     　　is array<int, 3> if three-channel color, method determines whether it is RGB or HSV;
            ...                             //     Middle layer array<*, 2> is the color (or grayscale) lower and upper limits:
        ],                                  //     Outer layer represents different color ranges, the region to be identified is the union of their corresponding masks on the template image.

        "colorWithClose": true,             // Optional, effective when method is HSVCount or RGBCount, default is true
                                            // Whether to first apply morphological closing to the mask range when counting colors.
                                            // Morphological closing can fill small black spots, generally improving color counting matching results, but if the image contains text, it is recommended to set it to false

        "method": "Ccoeff",                 // Optional, template matching algorithm, can be a list
                                            // Default is Ccoeff if not specified
                                            //      - Ccoeff:       Template matching algorithm insensitive to color, corresponds to cv::TM_CCOEFF_NORMED
                                            //      - RGBCount:     Template matching algorithm sensitive to color,
                                            //                      First binarize the region to be matched and the template image based on colorScales,
                                            //                      Calculate the similarity in RGB color space using F1-score as the indicator,
                                            //                      Then dot the result with the Ccoeff result
                                            //      - HSVCount:     Similar to RGBCount, but the color space is changed to HSV

        /* The following fields are only valid if the algorithm is OcrDetect */

        "text": [ "接管作战", "代理指挥" ],  // Required, the text content to be recognized, as long as any match is considered to be recognized

        "ocrReplace": [                     // Optional, a replacement for commonly misidentified text (regular support)
            [ "千员", "干员" ],
            [ ".+击干员", "狙击干员" ]
        ],

        "fullMatch": false,                 // optional, whether the text recognition needs to match all words (not multiple words), default is false
                                            // false, as long as it is a substring: for example, text: [ "start" ], the actual recognition to "start action", is also considered successful.
                                            //If true, "start" must be recognized, not one more word

        "isAscii": false,                   // optional, whether the text content to be recognized is ASCII characters
                                            // default false if not filled

        "withoutDet": false,                // Optional, whether to not use the detection model
                                            // default false if not filled

        /* The following fields are only valid when the algorithm is Hash */
        // The algorithm is not mature, and is only used in some special cases, so it is not recommended for now
        // Todo

        /* The following fields are valid only when algorithm is FeatureMatch */

        "template": "xxx.png",              // Optional, the image file name to match, can be a string or a string list
                                            // Default "taskname.png"

        "count": 4,                         // The number of feature points to match (threshold), default value = 4

        "ratio": 0.6,                       // The distance ratio of the KNN matching algorithm, [0 - 1.0], the larger the ratio, the looser the match and the easier it is to connect. Default 0.6

        "detector": "SIFT",                 // Feature point detector type, optional values ​​are SIFT, ORB, BRISK, KAZE, AKAZE, SURF; default value = SIFT
                                            // SIFT: High computational complexity, scale invariance, rotation invariance. Best effect.
                                            // ORB: Very fast computational speed, rotation invariance. But not scale invariance.
                                            // BRISK: Very fast computational speed, scale invariance, rotation invariance.
                                            // KAZE: Applicable to 2D and 3D images, with scale invariance and rotation invariance.
                                            // AKAZE: Fast calculation speed, with scale invariance and rotation invariance.
    }
}
```

## Expression Calculation

Task list type fields (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`) support expression calculations.

| Symbol | Meaning | Example |
|:---------:|:---:|:--------:|
| `@` | `@`-type Task | `Fight@ReturnTo` |
| `#` (unary) | Virtual task | `#self` |
| `#` (binary) | Virtual task | `StartUpThemes#next` |
| `*` | Repeat tasks | `(ClickCornerAfterPRTS+ClickCorner)*5` |
| `+` | Task list merge | `A+B` |
| `^` | Task list difference (in the former but not in the latter, the order remains the same) | `(A+A+B+C)^(A+B+D)` (= `C`) |

The operators `@`, `#`, `*`, `+`, `^` have priority: `#` (unary) > `@` = `#` (binary) > `*` > `+` = `^`.

## Special Task Type

### Template Task

**Template tasks** include derived tasks and `@`-type tasks. The core of the template task can be understood as **modifying the default values of fields** based on the *base task*.

#### Derived Task

Tasks with the `baseTask` field are derived tasks, and the task corresponding to the `baseTask` field is referred to as its *base task* of this derived task. For a derived task,

1. If it is an template matching task, the default value for the `template` field remains `"TaskName.png"`;
2. If the `algorithm` field differs from its *base task*, the derived class parameters are not inherited (only parameters defined in `TaskInfo` are inherited);
3. The default values for all other fields are the corresponding fields of its *base task*.

#### Implicit `@`-type Task

If task `"A"` exists and there is not a task directly defined in any task file such as `"B@A"`, it is an implicit `@`-type task, and task `"A"` is called the *base task* of task `"B@A"`. For an implicit `@`-type task,

1. The default values for task list type fields (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`) are set to the base task's corresponding fields with the `B@` prefix directly appended (if the task name starts with `#`, the `B` prefix is appended);
2. The default values for all other fields (including the `template` field) are the corresponding fields of its *base task*.

#### Explicit `@`-type Task

If task `"A"` exists and there is a task directly defined in the task file such as `"B@A"`, it is an explicit `@`-type task, and task `"A"` is called the *base task* of task `"B@A"`. For an explicit `@`-type task,

1. The default values for task list type fields (`sub`, `next`, `onErrorNext`, `exceededNext`, `reduceOtherTimes`) are set to the base task's corresponding fields with the `B@` prefix directly appended (if the task name starts with `#`, the `B` prefix is appended);
2. If it is an template matching task, the default value for the `template` field remains `"TaskName.png"`;
3. If the `algorithm` field differs from its *base task*, the derived class parameters are not inherited (only parameters defined in `TaskInfo` are inherited);
4. The default values for all other fields are the corresponding fields of its *base task*.

### Virtual Task

Virtual tasks are tasks of the form `"#{sharp_type}"` or `"B#{sharp_type}"`, where `{sharp_type}` can be `none`, `self`, `back`, `next`, `sub`, `on_error_next`, `exceeded_next`, or `reduce_other_times`.

Virtual tasks can be divided into **command virtual tasks** (`#none` / `#self` / `#back`) and **field virtual tasks** (`#next`, etc.).

| Virtual Task Type | Meaning | Simple example |
|:---------:|:---:|:--------:|
| none | Empty Task | Skip directly<sup>1</sup><br>`"A": {"next": ["#none", "T1"]}` is interpreted as `"A": {"next": ["T1"]}`<br>`"A#none + T1"` is interpreted as `"T1"` |
| self | Current Task Name | `"A": {"next": "#self"}` in `"#self"` is interpreted as `"A"`<br>`"B": {"next": "A@B@C#self"}` in `"A@B@C#self"` is interpreted as `"B"`.<sup>2</sup> |
| back | # Preceding task name | `"A@B#back"` is interpreted as `"A@B"`<br>`"#back"` will be skipped if it appears directly<sup>3</sup> |
| next, sub, etc. | # The field corresponding to the previous task name | Take `next` for example:<br>`"A#next"` is interpreted as `Task.get("A")->next`<br>`"#next"` will be skipped if it appears directly |

*Note<sup>1</sup>: `"#none"` generally used in conjunction with the template task to add prefixes, or used in the `baseTask` field to avoid unnecessary fields in multi-file inheritance.*

*Note<sup>2</sup>: `"XXX#self"` has the same meaning as `"#self"`.*

*Note<sup>3</sup>: When several tasks have `"next": [ "#back" ]`, `"T1@T2@T3"` represents executing `T3`, `T2`, and `T1` in sequence.*

### Multi-File Task

If a task defined in a later loaded task file (e.g. `tasks.json` for foreign services; hereinafter called File 2) also has a task of the same name defined in a earlier loaded task file (e.g. `tasks.json` for official services; hereinafter called File 1), then.

- if the task in File 2 does not have a `baseTask` field, then it inherits the fields of the task with the same name in File 1 directly.
- If the task in File 2 has a `baseTask` field, then it does not inherit the fields of the task with the same name in File 1, but overwrites them. In particular, when there is no template task, you can use `"baseTask": "#none"` to avoid inheriting unnecessary fields.

### Simple example

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

    The result is:

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

## Runtime task modification

- `Task.lazy_parse()` loads the json task configuration file at runtime. The lazy_parse rules are the same as for [multi-file task](#multi-file-task).
- `Task.set_task_base()` modifies the `baseTask` field of a task.

### Usage Example

Suppose you have the following task configuration file:

```json
{
    "A": {
        "baseTask": "A_default"
    },
    "A_default": {
        "next": [ "xxx" ]
    },
    "A_mode1": {
        "next": [ "yyy" ]
    },
    "A_mode2": {
        "next": [ "zzz" ]
    }
}
```

The following code enables changing task "A" based on the value of `mode`, and will also change other tasks that depend on task "A", e.g. "B@A":

```cpp
switch (mode) {
case 1:
    Task.set_task_base("A", "A_mode1");  // This is basically the same as replacing A with the contents of A_mode1, as follows
    break;
case 2:
    Task.set_task_base("A", "A_mode2");
    break;
default:
    Task.set_task_base("A", "A_default");
    break;
}
```

## Schema Check

This project configures a json schema check for `tasks.json`, the schema file is `docs/maa_tasks_schema.json`.

### Visual Studio

It is configured in `MaaCore.vcxporj` and works right out of the box. The hint effect is more obscure and some information is missing.

### Visual Studio Code

It is configured in `.vscode/settings.json`, open that **project folder** with vscode and you are ready to use it. The hint works better.
