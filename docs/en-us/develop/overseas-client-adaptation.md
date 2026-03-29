---
order: 5
icon: 'ri:earth-fill'
---
# Overseas Server Adaptation Tutorial

## Preparations

Before starting this tutorial, please ensure you have:

1. Installed and properly configured the required software. The `readme.md` for the CN server or the corresponding client should contain relevant information. Ensure that the supported features can run normally.
2. Read the [Task Flow Protocol](../protocol/task-schema.md) to have a basic understanding of the meaning and usage of each field, and be able to understand the meaning and usage of `@` and `#` type tasks.
3. Understand that content not mentioned or missing in the overseas server's `task.json` and template images will use the CN server's `task.json` and template images as a fallback. Content in the overseas server's `task.json` will override and rewrite the corresponding fields of the corresponding tasks in the CN server.
4. Possess a certain level of English proficiency, be able to read English logs, and identify missing images and other information through the logs.
5. It is recommended to make modifications according to the task chain. For example, for the `Award` task, follow the `next` order of the `Award` task in the CN server's `task.json` to gradually replace `template images` / `text` / `modify roi`, ensuring that each modified step can run normally or that errors can be quickly identified. This avoids situations where too many changes are made at once, making it unclear at which step the program gets stuck and fails to run.

### Pre-modification Preparation

Before making modifications, there are several preparatory tasks to note:

1. Refer to the CN server's task.json and ensure you have prepared the template images and text content that differ from the CN server for the overseas server.
2. Ensure you can access these images and text content at any time.

## Taking Screenshots

To obtain high-quality screenshots, please follow these guidelines:

1. Use the emulator's built-in screenshot tool to take and save screenshots.
2. Ensure the screenshot dimensions are larger than `1280*720` with an aspect ratio of `16:9`.
3. Ensure the screenshot does not contain any irrelevant content, such as taskbars, status bars, notification bars, etc.
4. Ensure the screenshot includes all content that needs to be recognized.

To crop images and obtain text/image `roi`, you need to use the `MaaAssistantArknights/tools/ImageCropper` tool.

**ImageCropper** is a powerful screenshot tool that supports capturing, saving, and color picking ROI areas from pre-prepared screenshots or by connecting to a device via ADB.

### Environment Configuration

Requires a `python` environment. Recommended version is `3.11`, minimum version is `3.9` or above.

### Installing Dependencies

Windows users are recommended to directly run `install.bat`, or install manually:

```shell
python -m pip install -r requirements.txt
```

### Usage Steps

1. If you have pre-prepared screenshots, save them to the `./src/` path.
2. Run `start.bat` or `python main.py [device serial]` (device address is optional)
   - The tool will automatically search for connected ADB devices. Select a device according to the prompts (press ENTER to skip selection).
   - You can also directly use `python main.py [device serial]` to connect to a specified device.
3. In the pop-up window, use left-click to select the target area, scroll wheel to zoom the image, and right-click to move the image.
4. Use shortcut keys for operations:
   - Press `S` or `ENTER` to save the target area.
   - Press `F` to save a full-screen standardized screenshot.
   - Press `R` to not save, only output the ROI range.
   - Press `C` to not save, output the ROI range and the required fields for ColorMatch.
   - Press `Z`, `DELETE`, or `BACKSPACE` to undo.
   - Press `0` ~ `9` to zoom the window.
   - Press `Q` or `ESC` to exit.
   - Press any key to skip/refresh the current screenshot.
5. Target area screenshots are saved in the `./dst/` path.

For example, the output content after completing a crop is:

```log
src: Screenshot_xxx.png
dst: Screenshot_xxx.png_426,272,177,201.png
original roi: 476, 322, 77, 101,
amplified roi: 426, 272, 177, 201
```

Among these,

`Screenshot_xxx.png` is the name of the full screenshot placed in the `src` folder. `Screenshot_xxx.png_426,272,177,201.png` is the cropped image.

`original roi` is the area selected by the mouse. `amplified roi` is the expanded area. You need the expanded range, so the value entered in the `roi` field in `task.json` is this value.

## Modifying Template Images

Before modifying template images, you need to open the template image folder for the corresponding client and the CN server's template image folder.

For example:

- The location of the EN server's template image folder is `MaaAssistantArknights\resource\global\YoStarEN\resource\template`.
- The location of the CN server's template image folder is `MaaAssistantArknights\resource\template`.

Refer to the template images mentioned in `task.json`, compare the template images between the CN server and the overseas server, and identify the templates missing in the overseas server.

Typically, except for logos and similar images, templates containing text need to be replaced via screenshots. If the image size is significantly larger than the corresponding CN server template image, you need to modify the `roi` size.

Place the cropped and renamed template images into the corresponding client's template image folder.

## Modifying Text Content

Before modifying text content, you need to open the `task.json` for the corresponding server and the CN server's `task.json`.

For example:

- The location of the EN server's `task.json` is `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`.
- The location of the CN server's `task.json` is `MaaAssistantArknights\resource\tasks.json`.

Find the corresponding task and modify the `text` field to the content displayed within the corresponding server. Note that the recognized content can be a substring of the complete content in the game.

Typically, unless it's pure ASCII character recognition, `text` containing characters needs to be replaced. If the text length is significantly longer than the CN server's, you need to modify the `roi` size. For example, the length difference between `"任务"` and `"Mission"` is large, so you need to modify the `roi` size for that task in the overseas server.

If the corresponding overseas server's `task.json` does not have this task, you need to add the task, only filling in the `text` field is sufficient.

## Modifying ROI Range

1. Open the `task.json` for the corresponding server, e.g., the EN server's location is `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`.
2. Find the task that needs the `roi` range modification. Using your prepared overseas server game interface screenshot, adjust the `roi` range size based on the `amplified roi`.
3. Typically, `roi` does not need to be modified. It only needs modification when the size difference from the CN server's recognized content is too large.
4. If the task does not exist in the corresponding overseas server's `task.json`, add the task and write the `roi` field.

## Saving Settings and Restarting the Software

After completing the modifications, restart the software to reload the files and make the modifications take effect. Alternatively, create a new `DEBUG.txt` in the software directory and then open the software. This way, each time you click start, templates and files will be reloaded without needing a restart.

Check for success:

1. Check the software's operation to ensure it can be used normally on the overseas server.
2. If it cannot run normally, you need to check if the modifications are correct or review the log output to find where the error occurred.

## Interpreting Logs

Sometimes, after we modify `task.json`, we find the program still cannot run correctly. In such cases, we consider checking the logs to find where the error occurred and then modify the corresponding task.

The log file is located in the software's root directory, named `asst.log`. If you compiled MAA yourself, it's in `\x64\Release` or `x64\RelWithDebInfo`, specifically in which folder depends on the compilation mode you selected during compilation.

Below is a log example:

```log
[2022-12-18 17:43:17.535][INF][Px7ec][Tx15c8] {"taskchain":"Award","details":{"to_be_recognized":["Award@ReturnTo","Award","ReceiveAward","DailyTask","WeeklyTask","Award@CloseAnno","Award@CloseAnnoTexas","Award@TodaysSupplies","Award@FromStageSN"],"cur_retry":10,"retry_times":20},"first":["AwardBegin"],"taskid":2,"class":"asst::ProcessTask","subtask":"ProcessTask","pre_task":"AwardBegin"}
[2022-12-18 17:43:18.398][INF][Px7ec][Tx15c8] Call ` C:\Program Files\BlueStacks_nxt\.\HD-Adb.exe -s 127.0.0.1:5555 exec-out "screencap | gzip -1" ` ret 0 , cost 862 ms , stdout size: 2074904 , socket size: 0
[2022-12-18 17:43:18.541][TRC][Px7ec][Tx15c8] OcrPack::recognize | roi: [ 500, 50, 300, 150 ]
[2022-12-18 17:43:18.541][TRC][Px7ec][Tx15c8] Ocr Pipeline with asst::WordOcr | enter
[2022-12-18 17:43:18.634][TRC][Px7ec][Tx15c8] Ocr Pipeline with asst::WordOcr | leave, 93 ms
[2022-12-18 17:43:18.634][TRC][Px7ec][Tx15c8] OcrPack::recognize | raw: [{ : [ 0, 0, 300, 150 ], score: 0.000000 }]
[2022-12-18 17:43:18.634][TRC][Px7ec][Tx15c8] OcrPack::recognize | proc: []
[2022-12-18 17:43:18.637][TRC][Px7ec][Tx15c8] asst::ProcessTask::_run | leave, 1101 ms
[2022-12-18 17:43:18.638][TRC][Px7ec][Tx15c8] ready to sleep 500
[2022-12-18 17:43:19.144][TRC][Px7ec][Tx15c8] end of sleep 500
[2022-12-18 17:43:19.144][TRC][Px7ec][Tx15c8] asst::ProcessTask::_run | enter
```

In this log, you can see:

- `taskchain` represents the currently ongoing task.
- `details` is the content of the task, including fields to be recognized (`to_be_recognized`), current retry count (`cur_retry`), and total retry count (`retry_times`).
- `first` represents the start of the task.
- `taskid` is the task number.
- `class` and `subtask` represent the task category and subtask, respectively.
- `pre_task` represents the previous task.
  Additionally, the log records command execution status (e.g., `Call`) and OCR information (e.g., `OcrPack::recognize`).

In this log, `"to_be_recognized"`, `"cur_retry":3,"retry_times":20` indicates that recognition has been attempted 3 times, with a maximum of 20 attempts. After reaching the maximum, the task will be skipped and an error reported, proceeding to the next task. If there are no issues with the preceding tasks, we can basically determine that the recognition here is problematic. We need to check the tasks mentioned in the log, see if there are corresponding `template files`, if the `text` for the corresponding task is wrong, and if the task recognition `roi` range is correct, thereby identifying the problem and making modifications.

By checking the corresponding template image, it was found that the EN server template folder has a template for this image, but its size is significantly larger than the CN server image, causing the CN server's `roi` to fail recognition on the EN server. Therefore, the `roi` for the corresponding task in the EN server's `task.json` needs to be modified to match the size of the EN server image.

## Submitting Your Modifications

Please refer to the [GitHub Pull Request Guide](./pr-tutorial.md)
