---
order: 5
icon: ri:earth-fill
---

# Overseas Clients Adaptation

## Preparation

Before starting this tutorial, make sure you have:

1. Installed and properly configured the required software. There should be information in the `readme.md` of the ZH-CN client or the corresponding client to ensure that the supported features work properly.
2. Read  [Task Schema](../protocol/task-schema.md) and have a basic understanding of the meaning and usage of each field. You should also understand the meaning and usage of `@` and `#` in the code.
3. Understand that the `task.json` file and template image of the ZH-CN client will be used as alternatives when they are unmentioned or missing for the overseas client. The content in the `task.json` for the overseas client should be rewritten according to the fields of the ZH-CN task.
4. Have some English ability, be able to read English logs, and be able to find missing pictures and other information through the logs.
5. Understand that it is recommended to perform the modification individually according to the task chain. For example, to modify the Award task, you should modify the `template image`, `text`, and `roi` step by step in accordance with the order in its `next` field. This will ensure that every task is working normally after modification and errors can be easily spotted. Additionally, this can prevent confusion and forgetting which task the software is stuck on when too much content is modified at once.

### Preparation before making changes

Before making changes, there are several preparations that need to be noted:

1. Refer to the ZH-CN client's `task.json` and make sure you have prepared the template images and text content for the overseas client.
2. Make sure you have ready access to these images and text content.

## Get screenshots

To get high-quality screenshots, please follow these guidelines:

1. Use the screenshot tool provided with the emulator to take and save screenshots.
2. Ensure that the size of the screenshot is at least `1280x720` and the aspect ratio is `16:9`.
3. Make sure the screenshot does not contain any extraneous content, such as the taskbar, status bar, or notification bar.
4. Make sure the screenshot includes all the content that needs to be recognized.

To crop the image and get the text/image region of interest (ROI), you will need to install `Python` and `OpenCV`, and download the `MaaAssistantArknights/tools/CropRoi/main.py` file.

Then, follow these steps:

1. Create new `src` and `dst` folders in the same directory as `main.py`.
2. Place the **complete screenshot** of the text/image that needs to be resized or in need of a new ROI value into the `src` folder.
3. Run `main.py`.
4. Select the target range by dragging the mouse, making sure not to include extraneous content.
5. Once the range is determined, press `S` to save and `Q` to exit. The cropped image will be saved in the dst folder.

For example, the output after completing a crop would be:

``` log
src: Screenshot_xxx.png
dst: Screenshot_xxx.png_426,272,177,201.png
original roi: 476, 322, 77, 101,
amplified roi: 426, 272, 177, 201
```

In this example, `Screenshot_xxx.png` is the name of the full screenshot placed in the `src` folder, and `Screenshot_xxx.png_426,272,177,201.png` is the name of the cropped image.

The `original roi` is the area selected by the mouse, while the `amplified roi` is the enlarged area. You will need the enlarged area, so you should use the `amplified roi` value to fill in the roi field in task.json.

## Modify the template image

Before modifying the template image, you need to open the template image folder of the corresponding client and the template image folder of the ZH-CN client.

For example:

- The location of the template image folder for EN client is `MaaAssistantArknights\resource\global\YoStarEN\resource\template`.
- The location of the template image folder for the ZH-CN client is `MaaAssistantArknights\resource\template`.

Refer to the template images mentioned in the `task.json` file, compare the template images of the ZH-CN client and the overseas client, and identify any missing templates in the overseas client.

Typically, template images containing text need to be replaced with screenshots, with the exception of images such as logos. If the size of the image is significantly larger than the corresponding template image in the ZH-CN client, you may need to modify the size of the `roi`.

Once you have captured and renamed the template image, you can place it in the template image folder of the corresponding client.

## Modify the text content

Before modifying the text content, you will need to open the `task.json` file for both the corresponding server and ZH-CN client.

For example:

- The location of `task.json` in EN client is `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`.
- The location of `task.json` for ZH-CN client is `MaaAssistantArknights\resource\tasks.json`.

To modify the text, locate the corresponding task and change the `text` field to the content displayed in the corresponding server. Keep in mind that the identified content can be a substring of the full content within the game. In general, you should replace any `text` containing text unless it is recognized as a pure ASCII character.

If the text length is significantly longer than the ZH-CN client version, you may need to modify the size of the `roi` field. For example, if the difference in length between the `"任务"`(Mission in chinese)  and the English word `"Mission"` is too large, you may need to modify the size of the `roi` for that mission in the overseas client.

If the task does not exist in the `task.json` file of the corresponding overseas server, you will need to add it. Simply fill in the `text` field with the appropriate content.

## Modify ROI range

To modify the ROI range:

1. Open the `task.json` file for the corresponding server, for example, the location of the EN client's `task.json` file is `MaaAssistantArknights\resource\global\YoStarEN\resource\tasks.json`.
2. Find the task that corresponds to the ROI range you need to modify, and use a screenshot of the overseas game interface to adjust the size of the `roi` range according to the amplified ROI.
3. Normally, you should not need to modify the `roi` range, but you may need to do so if the difference in size between the text and the Chinese client version is too large.
4. If the task does not exist in the `task.json` file of the corresponding overseas client, add the task and write the `roi` field with the appropriate values.

## Save the settings and restart the software

After making the changes, you will need to save the settings and restart the software in order for the changes to take effect. To do this, you can either restart the software and reload the file, or create a new `DEBUG.txt` file in the software directory. This will cause the template and files to be reloaded every time you click the "Link Start" button, so you will not need to restart the software.

To check for success:

1. Test the operation of the software to ensure that it is functioning properly on the overseas server.
2. If it is not working properly, you will need to check if the changes are correct, or check the log output to determine what went wrong.

## Interpreting the logs

If you have modified the `task.json` file but the program is still not functioning correctly, it may be helpful to check the logs to find any errors and modify the corresponding task accordingly. The log file is typically located in the root directory of the software, and is named `asst.log`. If you compiled MAA yourself, it may be located in the `\x64\Release` or `\x64\RelWithDebInfo` directory, depending on the compilation mode you selected when compiling.

Interpreting the logs can be helpful in identifying any issues with the program. Here is an example log:

``` log
[2022-12-18 17:43:17.535][INF][Px7ec][Tx15c8] {"taskchain":"Award","details":{"to_be_recognized":["Award@ReturnTo","Award","ReceiveAward","DailyTask","WeeklyTask","Award@CloseAnno","Award@CloseAnnoTexas","Award@TodaysSupplies","Award@FromStageSN"],"cur_retry":10,"retry_times":20},"first":["AwardBegin"],"taskid":2,"class":"asst::ProcessTask","subtask":"ProcessTask","pre_task":"AwardBegin"}
[2022-12-18 17:43:18.398][INF][Px7ec][Tx15c8] Call ` C:\Program Files\BlueStacks_nxt\. \HD-Adb.exe -s 127.0.0.1:5555 exec-out "screencap | gzip -1" ` ret 0 , cost 862 ms , stdout size: 2074904 , socket size: 0
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

In this log, you can see that:

- `"taskchain"` represents the task currently in progress.
- `"details"` is the content of the task, including the fields to be recognized (`to_be_recognized`) and the current number of retries (`cur_retry`) and the total number of retries (`retry_times`).
- `"first"` represents the start of the task.
- `"taskid"` is the task number.
- `"class"` and `subtask` represent the class and subtask of the task, respectively.
- `"pre_task"` represents the previous task.
In addition, the execution of the command (e.g. `Call`) and the `OCR` information (e.g. `OcrPack::recognize`) are recorded in the log.

For example, in this log, `"to_be_recognized"`,`"cur_retry":3,"retry_times":20` means that the task has been attempted to be recognized 10 times, and the maximum number of times is 20. After the maximum number of times is reached, the task will be skipped and an error will be reported, and the next task will be performed. If there are no issues with the previous task, it is likely that there is a problem with the recognition here. To troubleshoot this issue, you can check the tasks mentioned in the log to see if there is a corresponding template file, if the `text` field of the corresponding task is incorrect, or if the `roi` range for task recognition is incorrect, and make any necessary modifications.

By looking at the corresponding template image, you may find that there is a template for this image in the EN client template folder, but the size is larger than the image in template folder for the ZH-CN client . This could cause the `roi` in ZH-CN client to not be recognized in the EN client, so you may need to modify the corresponding task's `roi` in the EN client's `task.json` file to match the size of the image in the EN client.

## Submit your changes

Check out [Github Pull Request Guide](./pr-tutorial.md)
