---
order: 5
icon: ri:earth-fill
---

# Overseas Client Adaptation Tutorial

## Preparation

Before starting this tutorial, make sure you have:

1. Installed and properly configured the required software. There should be relevant information in the CN client or corresponding client's `readme.md` to ensure that supported features work properly.
2. Read the [Task Schema](../protocol/task-schema.md) and have a basic understanding of the meaning and usage of each field, and understand the meaning and usage of `@` and `#` type tasks.
3. Understand that content not mentioned or missing in overseas client `task.json` and template images will use CN client `task.json` and template images as fallbacks. Content in overseas client `task.json` will override and rewrite corresponding fields of CN client tasks.
4. Have sufficient English ability to read English logs and find missing images and other information through logs.
5. It's recommended to modify according to task chains. For example, for the `Award` task, gradually replace `template images` / `text` / `modify roi` step by step according to the `next` order of the `Award` task in CN client `task.json`, ensuring each step works properly after modification or can quickly identify errors. This avoids confusion about which step the program is stuck on when too many changes are made at once.

### Pre-modification Preparation

Before making modifications, there are several preparation steps to note:

1. Refer to CN client task.json and ensure you have prepared template images and text content for overseas clients that differ from CN client.
2. Ensure you can access these images and text content at any time.

## Getting Screenshots

To get high-quality screenshots, please follow these guidelines:

1. Use the emulator's built-in screenshot tool to take and save screenshots.
2. Ensure screenshot dimensions are larger than `1280*720` with aspect ratio `16:9`.
3. Ensure screenshots don't contain any irrelevant content, such as taskbar, status bar, notification bar, etc.
4. Ensure screenshots contain all content that needs to be recognized.

To crop images and get text/image `roi`, you need to install `python` and `opencv`, and download the `MaaAssistantArknights/tools/CropRoi/main.py` file.

Then follow these steps:

1. Create new `src` and `dst` folders in the same directory as `main.py`.
2. Place **complete screenshots** that need resizing or text/image `roi` extraction into the `src` folder.
3. Run `main.py`.
4. Use mouse drag to select target range, try not to include irrelevant content.
5. After determining the range, press `S` to save, press `Q` to exit. Cropped images will be saved in the `dst` folder.

For example, output after completing one crop:

```log
src: Screenshot_xxx.png
dst: Screenshot_xxx.png_426,272,177,201.png
original roi: 476, 322, 77, 101,
amplified roi: 426, 272, 177, 201
```

Where:

`Screenshot_xxx.png` is the name of the complete screenshot placed in the `src` folder. `Screenshot_xxx.png_426,272,177,201.png` is the cropped image.

`original roi` is the mouse-selected area. `amplified roi` is the enlarged area. You need the enlarged range, so fill the `roi` field in `task.json` with this value.

## Modifying Template Images

Before modifying template images, you need to open the corresponding client's template image folder and CN client's template image folder.

For example:

- EN client template image folder location: `MaaAssistantArknights\resource\global\YoStarEN\resource\template`.
- CN client template image folder location: `MaaAssistantArknights\resource\template`.

Refer to template images mentioned in `task.json`, compare CN client and overseas client template images, and find templates missing in overseas client.

Usually, except for logos and other images, templates containing text need to be replaced through screenshots. If image dimensions are significantly larger than corresponding CN client template images, you need to modify `roi` size.

Place captured and renamed template images into the corresponding client's template image folder.

## Modifying Text Content

Before modifying text content, you need to open the corresponding server's `task.json` and CN client's `task.json`.

For example:

- EN client `task.json` location: `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`.
- CN client `task.json` location: `MaaAssistantArknights\resource\tasks.json`.

Find the corresponding task and modify the `text` field to content displayed in the corresponding server. Note that recognized content can be a substring of complete in-game content.

Usually, unless it's pure ASCII character recognition, `text` containing text needs to be replaced. If text length is significantly larger than CN client, you need to modify `roi` size, such as the length difference between `"任务"` and `"Mission"` being too large, requiring modification of overseas client task `roi` size.

If the corresponding overseas client `task.json` doesn't have the task, you need to add the task, only filling in the `text` field.

## Modifying ROI Range

1. Open the corresponding server's `task.json`, such as EN client location `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`
2. Find the corresponding task that needs `roi` range modification, use your prepared overseas game interface screenshots, and adjust `roi` range size according to `amplified roi`.
3. Usually, `roi` doesn't need modification, only when recognition content size difference with CN client is too large.
4. If the corresponding overseas client `task.json` doesn't have the task, add the task and write the `roi` field.

## Save Settings and Restart Software

After modifications are complete, restart the software to reload files and make changes effective. Or create a `DEBUG.txt` file in the software directory before opening the software, so templates and files reload every time you click start, without needing restart.

Check for success:

1. Check software operation to ensure it works normally on overseas clients.
2. If it doesn't work properly, check if modifications are correct or view log output to find error locations.

## Reading Logs

Sometimes after modifying `task.json`, we find the program still can't run correctly. In this case, we should check logs to find error locations and modify corresponding tasks.

Log file location is in the software root directory, filename `asst.log`. If you compiled MAA yourself, it's in `\x64\Release` or `x64\RelWithDebInfo`, depending on your compilation mode selection.

Here's a log example:

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

- `taskchain` represents the currently running task.
- `details` is task content, including fields to be recognized (`to_be_recognized`), current retry count (`cur_retry`), and total retry count (`retry_times`).
- `first` represents task start.
- `taskid` is the task number.
- `class` and `subtask` represent task class and subtask respectively.
- `pre_task` represents the previous task.

Additionally, the log records command execution (like `Call`) and `OCR` information (like `OcrPack::recognize`).

In this log, `"to_be_recognized"`, `"cur_retry":3,"retry_times":20` indicates it has repeatedly tried recognition 3 times, with maximum recognition attempts of 20. After reaching maximum attempts, it will skip the task and report error, continuing to the next task. If previous tasks had no issues, we can basically determine there's a recognition problem here. We need to check tasks mentioned in the log to see if there's a corresponding `template file`, if the corresponding task's `text` is incorrect, if task recognition `roi` range is correct, to find and fix the problem.

By checking the corresponding template image, we found there's a template for this image in the EN client template folder, but it's significantly larger than the CN client image, causing CN client's `roi` to fail recognition on EN client. So we need to modify the corresponding task `roi` in EN client's `task.json` to match EN client image size.

## Submit Your Changes

Please refer to [GitHub Pull Request Guide](./pr-tutorial.md)
