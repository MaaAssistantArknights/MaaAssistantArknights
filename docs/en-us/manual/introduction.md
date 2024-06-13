---
icon: mdi:information-outline
---

# MAA User Manual

## Features

### Operations

- If the stage you need is not available in the selection, please choose "Current/Last" in MAA and manually locate the stage in the game.
    Make sure the screen stays on the stage detail page with the **Start** and **Auto-Deploy** buttons available.
- If you are not on this page, `Current/Previous` will automatically navigate to the last stage played according to the record in the lower right corner of the terminal homepage.
- You can also enable `Manually input stage name` in `Task Settings` - `Combat` - `Advanced` and enter the stage number manually. Currently supported stages include:
    - All main theme stages, where `-NORMAL` or `-HARD` can be added at the end to switch between standard and challenge modes.
    - LMD stages and Battle Record stages 5/6. The input should be `CE-6` or `LS-6` even if you have not unlocked it yet. In that case, the program will automatically switch to corresponding stage 5.
    - Skill Summary, Shop Voucher, and Carbon Stages 5. The input also should be `CA-5`, `AP-5`, and `SK-5` respectively.
    - Chip stages. The input should be complete with stage number, such as `PR-A-1`.
    - Annihilation. The input should be `Annihilation`.
    - Certain side story stages. This includes `OF-1`, `OF-F3` and `GT-5`.
    - The last three stages of the current SS event. This is available after downloading updates automatically from the [API](https://ota.maa.plus/MaaAssistantArknights/api/gui/StageActivity.json) when the event is on. Prompt will be shown in the main page when this is available.
    - For the SS event rerun, you can enter `SSReopen-XX` to clear XX-1 ~ XX-9 levels once. Example `SSReopen-IC`.

::: details Example
![Example](https://github.com/MaaAssistantArknights/MaaAssistantArknights/assets/56174894/e94a0842-a42f-449d-9f2e-f2339175cbdd)
:::

- Fight options include `Use Sanity Potion + Use Originium`, `Perform Battles` and `Material`, you can specific any of them. The fight tasks stops once one of the specifications is met.
    - `Use Sanity Potion` specifies the number of sanity potions to use at most. Multiple medicines may be used at a time.
    - `Use Originium` specifies the number of Originium to use at most. It is used one at a time. When using the Origin Stone to restore sanity, if you still have the Sanity Potion, the stone will not be used.
    - `Perform Battles` specifies the number of battles to perform at most.
    - `Material` specifies the number of materials to collect.

Note that `Use Originium` will only be used after `Use Sanity Option`, because MAA will only use Originium to replenish sanity when there are no Sanity Potions left. Therefore, after ticking `Use Originium`, you need to set the number of times `Use Sanity Potion` to a value greater than or equal to the value of the existing Sanity Potion in the storehouse, e.g. 999, in order to avoid skipping `Use Originium`.

::: details Example
| Example | Use Sanity Potion | Use Originium | Perform Battles | Material | Result                                                                                                                                                                                                                                                                                 |
|:-------:|:-----------------:|:-------------:|:---------------:|:--------:|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
|    A    |       999         |      10       |        1        |     x    | The AI will attempt to use sanity potions and/or originium until **one** full run is completed, satisfying the condition `Perform Battles: 1`. If there are not enough sanity potions, originium, or initial sanity to start, the AI will stop without starting the run.               |
|    B    |        x          |       x       |       100       |     x    | The AI will attempt to complete 100 runs, but if all available sanity is used up (which may be less than 100 runs), and the conditions `Use Sanity Potion: No` and `Use Originium: No` are met, the AI will stop without completing the full 100 runs.                                 |
|    C    |        1          |       x       |       100       |     x    | The AI will attempt to complete 100 runs, using at most one sanity potion. If the AI uses a sanity potion and runs out of sanity during the process, and the conditions `Use Sanity Potion: 1` and `Use Originium: No` are met, the AI will stop without completing the full 100 runs. |
|    D    |       999         |       x       |       100       | 3 Orirock| The AI will attempt to complete 100 runs, using up to 999 sanity potions. If during the process, the AI accumulates 3 Orirock cubes, satisfying the condition `Material: 3 Orirock`, the AI will stop without completing the full 100 runs.                                            |
:::


- Note that `Material` and `Stage` are independent options.
    - `Material` is not going to automatically navigate to the the stage for the specified material. You still need to manually configure the stage option.
- `Auto-Deploy` will be automatically selected if not already in case you forget to do so.
- Material drops are automatically recognized and printed to the program log. The data also gets uploaded to [Penguin Stats](https://penguin-stats.io/) and [Yituliu](https://ark.yituliu.cn/). You can manually set your Penguin Stats user ID in the settings.
- After disconnection or flashing at 4 am, it will automatically reconnect and continue to play the last stage selected in the game. If you need to cross the day, please check the last stage selection.
- A level up situation can be automatically handled as well as a failed delegation in which case this time of the operation will be given up.

### Infrastructure

#### Shifting Strategy

- Automatically calculate and choose the **optimal solution within a single facility**. Supports all general and special skill combinations. Supports recognition of Battle Record, Pure Gold, Originium Shard, Chip, etc. for different operators.

#### Morale Threshold for Working in Infrastructure

- Recognizes the percentage of the Morale bar. When Morale is below some threshold, the operator will be moved to the dormitory.

#### Note

- The shifting strategy is based on the optimal solution within a single facility instead of multiple facilities. Combination such as: `Shamare-Tequila`, `Vermeil-Scene` within a single facility can be recognized correctly; while combination like `Rosmontis`, `Pinus Sylvestris` among facilities is not supported yet.
- If `Usage of Drone` is selected with the option `Trading Post-LMD`, it will recognize `Shamare` and reserve it for her.
- Operators of corresponding fraction will be selected when only one Clue is needed Reception Room; otherwise general operators will be chosen.
- Reception Room will send out Clues only when your Clues are full. Three Clues will be send out at most. You can edit `SelectClue` - `maxTimes` field in `resource/tasks.json` to edit number of Clues sent if you want.
- If you do not want operators like `Irene` or someone else to be put into the dormitory when the training room is not in use, you can switch off `Working operator shall not be put into the dormitory` in the settings. Note that this may cause the operators with non-full fatigue not entering the dormitory as well.
- Due to the complexity of Control Center, only `Amiya`, `Swire`, `Kal'tsit`, `Team Rainbow` and other Morale+0.05 operators will be considered. To be improved in future.
- Some alternate operators may have conflicts in Infrastructure. Please notice if there is "Operator conflict" warnings on the UI, and double check the Infrastructure to shift manually (e.g. some facilities may not have any operator).

#### Custom infrastructure shift change (test function)

- The experts of Yituliu with a one-picture flow have helped to create a shift generator, which can be referenced in the documentation.
- Several sets of extremely efficient tasks are built-in under the MAA folder `/resource/custom_infrast/`, which can be used as a reference. (Fully upgraded operators at elite 2 level can try to use them directly.)

### Credit Store

- Automatically visit friends to obtain credit points.
- With the help of Warfarin's credit:
  - Use a support operator to clear the `OF-1` stage in Heart of Surging Flame. If the stage is not unlocked, please do not select this option.
  - You can modify the automatic combat flow of OF-1 by editing `resource\copilot\OF-1_credit_fight.json` (it is generally not recommended to modify).
  - It does not take effect when the stage selection is set to `current/last`.

### Integrated Strategy (I.S.) Support

- Please select the theme you want to explore in the MAA `Task Settings` - `Auto I.S.`, otherwise there may be problems due to the default selection of the latest theme.
  - Please pin the corresponding I.S. theme to the terminal in the game.
  - If there is exploration of non-target themes (such as if you plan to use MAA to brush Mizuki, but there is still an unfinished exploration of Phantom), please end it manually.
  - If MAA gets stuck/repeatedly enters and exits on the difficulty selection interface, please manually select the difficulty before starting this function.
- In the settings, you can choose the team, starting operator (only one operator name), etc.
- It supports automatic recognition of operators and proficiency, and automatically selects better operators and skills.
- It supports identifying store items and prioritizes purchasing more powerful collectibles.
- It supports reconnection after disconnection and supports continuing to return to brush after 4 a.m. update.
- If the scraping cannot be completed during the battle, all ground units will be automatically withdrawn after more than 5 minutes; if it exceeds 6 minutes, the current battle will be automatically abandoned without getting stuck.
- If the task gets stuck, it will automatically abandon the exploration and retry. However, if it often gets stuck in a certain place and then gives up, seriously affecting efficiency, please feel free to submit an issue for feedback~

### Share Your Tasks

- Welcome to share your tasks with [www.抄作业.com](https://www.抄作业.com), or [www.prts.plus](https://www.prts.plus)!

#### Load Tasks

- Please run it on the screen with `Start Operation` button.
- If you need to make your own build manually, turn off `auto build` to do so.
- Remember to like the tasks that you think helpful!

![image](https://github.com/MaaAssistantArknights/MaaAssistantArknights/assets/99072975/39e6dd67-2ff1-4023-a2b8-0018673c08aa)

#### Create Tasks

- A tool for creating tasks is provided in the directory of MAA. See also: [Copilot Schema](3.3-COPILOT_SCHEMA.md) for help.
- How to get map coordinates: start an operation after filling in `stage_name`. A file named `map.png` will be generated for your reference. Or you can refer to [PRTS.map](https://map.ark-nights.com/), with the `coordinates` set to `MAA` mode.
- Drill plan is recommended for testing.
- Welcome to share your tasks with [www.抄作业.com](https://www.抄作业.com), or [www.prts.plus](https://www.prts.plus)!
- It is recommended to write your own name, video walkthrough URL, or other things that you think helpful in the description.

### Task Video Recognition

- Drag and drop the video file to the `Copilot` section and click `Start` to initialize the video recognition.
- Only 16:9 aspect ratio videos with a resolution of 720p or higher are supported. The video content must not contain any black borders, distortion correction, emulator borders or other elements.

### Recruitment Recognition

- Auto-recruitment and recruitment recognition are two different features!
- Auto-recruitment supports using `Expedited Plan` to make it fully automated! Please enable `Auto use Expedited` in `Task Settings` - `Recruit` , and modify `Recruit max times`.
- Pop-up notification when 5★, 6★ operators are recruited.
- Auto-uploading recruitment data to [Penguin Stats](https://penguin-stats.io/) and [Yituliu](https://ark.yituliu.cn/) while auto-recruitment.

### Depot Recognition (test function)

- Please start with the `Upgrade materials` screen. Currently, exporting is only supported to [penguin-stats](https://penguin-stats.io/), [Arknights Toolbox](https://arkntools.app/#/material), and [Arknights | Planner](https://ark-nights.com/table). More useful features may be added in the future.
- Please feel free to contact us if you hope to integrate with our JSON schema.

## Setting Introduction

In addition to the `Settings` tab, there are also `Task Settings` in Windows MAA. Click the `gear` on the right side of the task list of `Farming` to switch between different task settings in the farming interface.

Note that clicking `General` `Advanced` will also switch `Task Settings`.

### Custom Connection

#### Obtain adb path

- Method 1: Download adb and set up connection manually
  - (Only for Windows users) Download [adb](https://dl.google.com/android/repository/platform-tools-latest-windows.zip) and unzip it. It is recommended to unzip to the MAA directory.
  - Go to the "Settings" - "Connection Settings" of the software, select the file path of `adb.exe`, fill in the adb address (IP + port need to be filled in, such as `127.0.0.1:5555`), and select the emulator type.

- Method 2: Find the adb execution port of the emulator and connect
  - For emulators that come with adb, you can find the location of the adb executable file according to the [Confirm adb address section](1.2-FAQ.md#Method-1-Confirm-that-the-adb-and-connection-address-are-correct).
  - Mac Android emulators are generally installed in the `/Application/` directory. Right-click on `[emulator name].app` and select "Show Package Contents". Find the adb executable file in the directory.

#### Get the port number

- Method 1: Use the adb command to view the running port directly

  **Replace adb in the command below with the name of the found adb executable file**, and then execute:

  ```sh
  # mac/linux/windows cmd
  adb devices
  # windows PowerShell
  .\adb devices
  ```

  After most emulator adb execution commands are run, they will be output in the following form, where `[ADBPORT]` is a specific number:

  ```sh
  List of devices attached
        127.0.0.1:[ADBPORT] device
  # may be more output like the above one
  ```

  Use `127.0.0.1:[ADBPORT]` as the connection address (replace `[ADBPORT]` with the actual number).

- Method 2: Use system command to check emulator debugging port

  If the output of the adb command does not display the port information in the form of `127.0.0.1:[port]`, you can use the following method to check:
    1. First, run the adb executable program once (need to start the adb daemon process), and run the Arknights emulator application.
    2. Then use the system command to check the port information of the adb process.

    Windows Command

    You can use `win` + `R` to open the command line by entering `cmd`, and use the following command to check:

    ```sh
    for /F "tokens=1,2" %A in ('"tasklist | findstr "adb""') do ( ^
    netstat -ano | findstr "%B" |)
    ```

    Mac / Linux Command

    Enter the following command in the terminal after starting the emulator:

    ```sh
    tmp=$(sudo ps aux | grep "[a]db" | awk '{print $2}') && \
    sudo netstat -vanp tcp | grep -e "\b$tmp"
    ```

    The output will be in the following format, where `[PID]` is the actual process number of the adb, `[ADBPORT]` is the remote connection port of the target emulator adb, and `[localport]` is the local address of the process, which does not need to be concerned (there will also be other irrelevant indicators in the Mac operation result):

    ```sh
    > ( netstat -ano | findstr "[PID]" )
    TCP  127.0.0.1:[localport]   127.0.0.1:0.0.0.0     LISTENING   [PID]
    TCP  127.0.0.1:[localport]   127.0.0.1:[ADBPORT]   ESTABLISHED [PID]
    # ...
    # maybe more output like the above line
    ```

  Use `127.0.0.1:[ADBPORT]` (replace `[ADBPORT]` with the actual number) in the results as the actual connection address of the emulator adb and fill it in `Settings` - `Connection Settings` - `Connection Address`.

### Automatically Start Multiple Emulators

- If you need to operate multiple emulators simultaneously, you can copy the MAA folder multiple times, and use **different MAAs**, **the same adb.exe**, and **different connection addresses** to connect.
- Taking `BlueStacks International Version` as an example, two ways to start multiple emulators are introduced.

  - Perform multiple operations by attaching commands to `HD-Player.exe`.

    1. Start the corresponding emulator separately.
    2. Open the Task Manager, find the corresponding emulator process, go to the Details tab, right-click the column above, click `Select Columns`, and check `Command Line`.
    3. In the newly added `Command Line` column, find the content after `"...\Bluestacks_nxt\HD-Player.exe"`.
    4. Fill in the found content, similar to `--instance Nougat32`, in `Startup Settings` - `Additional Commands`.

    Note: After the operation is completed, it is recommended to hide the `Command Line` column opened in `Step 2` to prevent freezing.
    - Example:

      ```bash
      Multi-instance 1:
      Emulator Path: C:\Program Files\BlueStacks_nxt\HD-Player.exe
      Additional Commands: --instance Nougat32 --cmd launchApp --package "com.hypergryph.arknights"
      Multi-instance 2:
      Emulator Path: C:\Program Files\BlueStacks_nxt\HD-Player.exe
      Additional Commands: --instance Nougat32_1 --cmd launchApp --package "com.hypergryph.arknights.bilibili"
      ```

      The `--cmd launchApp --package` part starts and automatically runs the specified package name application after startup, which can be changed as needed.

  - Perform multi-instance operation by using the shortcut of emulators or apps.

    1. Open the multi-instance manager and add the corresponding emulator's shortcut.
    2. Fill in the path of the emulator shortcut in `Startup Settings` - `Emulator Path`.

    Note: Some emulators support creating app shortcuts, which can directly launch the emulator and open Arknights with the app shortcut.
    - Example:

      ```bash
      Multi-instance 1:
      Emulator Path: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\Multi-instance 1.lnk
      Multi-instance 2:
      Emulator Path: C:\ProgramData\Microsoft\Windows\Start Menu\Programs\BlueStacks\Multi-instance 2 - Arknights.lnk
      ```

    If using `Emulator Path` for multi-instance operation, it is recommended to leave `Additional Commands` in `Startup Settings` empty.

### Pre/Post-Script

- Starting from v4.13.0, it is possible to set pre/post-script that automatically executes batch files before and after the task.
- You need to provide the path to the batch file, which should have a `.bat` extension.

## Miscellaneous

- Tasks order can be changed on the UI. So can the shifting order in the infrastructure.
- Almost all configuration changes will be saved automatically, except for options containing an *.
- All clicking event is randomized within a region, following Poisson distribution (higher probability at the center, lower probability around).
- Developed in C++, the core algorithm supports multi-level cache, in order to reduce CPU and memory usage as much as possible.
- Our software supports auto-update ✿✿ヽ(°▽°)ノ✿ Beta-testers can try beta versions, which updates faster and less buggy (maybe).
- If auto-update fails, you can download manually and put the ZIP file under the same directory. The update will start automatically.
