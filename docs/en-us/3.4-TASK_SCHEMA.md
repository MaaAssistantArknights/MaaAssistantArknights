# Task Schema

Usage of `resource/tasks.json` and description of each field

## Overview

```jsonc
{
    "TaskName" : {                          // Task name

        "algorithm": "MatchTemplate",       // Recognition algorithm, optional
                                            // MatchTemplate by default
                                            //      - JustReturn:       Proceed action without recognition
                                            //      - MatchTemplate:    Matching template image
                                            //      - OcrDetect:        OCR detection
                                            //      - Hash:             Hash calculation

        "template": "xxx.png",              // Available only when `algorithm` is `MatchTemplate`; Image file for matching, optional
                                            // "任务名.png" by default

        "text": [ "接管作战", "代理指挥" ],  // Available only when `algorithm` is `OcrDetect`; text to recognize, required, matched when any of the texts are found

        "action": "ClickSelf",              // Action after recognition, optional
                                            // DoNothing by default
                                            //      - ClickSelf:        Clicking the position recognized (random position within the region)
                                            //      - ClickRand:        Clicking any position in the screen
                                            //      - DoNothing:        Doing nothing
                                            //      - Stop:             Stopping current task
                                            //      - ClickRect:        Clicking in the specified rectangle. See the specificRect field, not recommended
                                            //      - SwipeToTheLeft:   Swiping to the left
                                            //      - SwipeToTheRight:  Swiping to the right
                                            //      - SlowlySwipeToTheLeft:   Slowly swiping to the left
                                            //      - SlowlySwipeToTheRight:  Slowly swiping to the right

        "sub": [ "SubTaskName1", "SubTaskName2" ],
                                            // Subtasks, optional. Will be executed in the order of the list after execution of the current task.
                                            // Subtasks can contain other subtasks. Pay attention to the infinite loop.

        "subErrorIgnored": true,            // Whether to ignore the error of the subtask, optional
                                            // `false` by default
                                            // `false` for not executing proceeding subtasks when one of the subtasks fails (considered as a failure of the current task).
                                            // `true` for ignoring subtask errors.

        "next": [ "OtherTaskName1", "OtherTaskName2"],
                                            // Next tasks to be executed after the current task is finished, optional
                                            // Will be recognized from the first item of the list and execute the first matched one.
                                            // Empty list means to stop the task after the current task is finished.

        "maxTimes": 10,                     // Maximum execution times, optional
                                            // Infinity by default
                                            // Proceeds with `exceedNext` if it exists; otherwise stops.

        "exceededNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Tasks to be executed after reaching `maxTimes`, optional
                                            // Stops the task if empty; otherwise, proceeds with the tasks here in the list instead of `next`.
        "onErrorNext": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Proceeding tasks when there are errors, optional

        "preDelay": 1000,                   // Pre-delay of action in ms after recognition, optional; 0 by default
        "rearDelay": 1000,                  // Post-delay of action in ms after recognition, optional; 0 by default

        "roi": [ 0, 0, 1280, 720],          // Recognition region with the format of [ x, y, width, height ], optional
                                            // Auto-scaling based on 1280 * 720 resolution；[ 0, 0, 1280, 720 ] by default
                                            // Reducing the region has the effect of reducing performance consumption and speeding up the recognition.

        "cache": true,                      // Whether the task uses cache, optional; `true` by default 
                                            // After a successful recognition, subsequent recognition will be done only in the region cached, which has a significant performance enhancement.
                                            // Only for targets that do not change position. Please set it to `false` for targets with changing position.

        "rectMove": [0, 0, 0, 0],           // Target movements after recognition, optional, not recommended. Auto-scaling based on 1280 * 720 resolution.
                                            // E.g. Icon A is recognized but the target is some position beside A. You can set it to click the place beside it.
                                            // It is suggested that it should recognize the target to click instead of using this field.
        
        "reduceOtherTimes": [ "OtherTaskName1", "OtherTaskName2" ],
                                            // Reducing execution counter of other tasks after execution, optional
                                            // E.g. Using sanity potion means clicking the blue start button did not take effect so the counter of the starting task should be reduced by 1.

        "specificRect": [ 100, 100, 50, 50],
                                            // Available only when `action` is `ClickRect`, indicating the position to click (random position within the region), required.
                                            // Auto-scaled based on 1280 * 720 resolution.
        
        /* Available only when `algorithm` is `MatchTemplate` */

        "templThreshold": 0.8,              // Threshold of image recognition, optional. Image is matched only when the similarity is greater than the threshold.
                                            // 0.8 by default. You can check it in the logs.

        "maskRange": [ 1, 255 ],            // Grayscale mask range, optional. E.g. fill in the region that does not require recognition with black colour,
                                            // and set it to [ 1, 255 ] so that the black region is ignored.


        /* Available only when `algorithm` is `OcrDetect` */

        "fullMatch": false,                 // Whether the text recognition is full match, optional. `false` by default.
                                            // `false` means substring matches as well. E.g.: `text: [ "开始" ]` matches "开始行动";
                                            // `true` means, e.g. it must match "开始" without other characters.

        "ocrReplace": [                     // Text replacement for common wrong recognition of OCR (supporting RegExp)
            [ "千员", "干员" ],
            [ ".+击干员", "狙击干员" ]
        ]

        /* Available only when `algorithm` is `Hash` */
        // Development in progress, only used in some special cases, not recommended
        // Todo
    }
}
```

## Tips

- Put the name of the task itself in `next` for looping, for example:

```jsonc
{
    "Waiting": {
        "algorithm": "OcrDetect",
        "text": [ "正在加载" ],
        "next": [
            "Succeed",  // Matches "加载完成" first
            "Waiting"   // Otherwise matches "正在加载"
        ]
    },
    "Succeed": {
        "algorithm": "OcrDetect",
        "text": [ "加载完成" ],
        "next": [
            "OtherTask"
        ]
    }
}
```

- The `JustReturn` field of `algorithm` is not recommended to set as a loop. Otherwise recognition will be always successful in some cases like adb disconnection.

- Tasks that are required to be executed N times can be copied N times with different names. For example:

```jsonc
{
    "DoIt": {
        "next": [
            "DoItAgain"
        ]
    },
    "DoItAgain": {
        "next": [
            "OtherTask"
        ]
    }
}
```

## Schema Check

This project provides JSON schema check for `tasks.json`. The schema file can be found at `docs/maa_tasks_schema.json`.

### Visual Studio

Visual Studio has been configured with `MeoAssistant.vcxporj`. Developers should be able to use it directly after loading the project.

### Visual Studio Code

Visual Studio Code has been configured with `.vscode/settings.json`. Developeres can open the **project folder** to use it, with better experience than Visual Studio.
