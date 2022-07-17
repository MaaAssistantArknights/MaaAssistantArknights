# MAA User Manual

## Introduction

### Operations

- If there is no stage you need, please click **blue start button** manually and select `current stage`.
- When any of the `Sanity Potion`, `Originite Prime`, `Times` conditions are met, the task is considered completed and stops.
  - Example 1：check `Sanity Potion`: `999`, `Originite Prime`: `10`, `Times`: `1`. After **ONE** operation is completed, the task stops because `Times`: `1` is met.
  - Example 2：uncheck `Sanity Potion` and `Originite Prime`, and set `Times`: `100`. After sanity is used up (maybe several times), the condition `Sanity Potion` and `Originite Prime` is met, and the task stops.
- Uploading operation statistics to [Penguin Stats](https://penguin-stats.cn/).
- Custom Penguin Stats ID.
- Recognizing and displaying drops.
- Auto-reconnection after disconnection.
- Auto-reconnection after 4 AM.
- Annihilation support.
- Supporting operator level-up.
- Supporting auto-combat failure by abandoning the operation.

### Infrastructure

#### Shifting Strategy

Auto-calculate and choose the **optimal solution within a single facility**. Supports all general and special skill combination. Supports recognition of Battle Record, Pure Gold, Originium Shard, Chip, etc. for different operators.

#### Morale Threshold for Resting

Recognize the percentage of the Morale progress bar. When Morale decreases beyond some threshold, move the operator to the domitory.

#### Note

- The shifting strategy is based on the optimal solution within a single facility instead of multiple facilities. Combination such as: `Shamare-Tequila`, `Vermeil-Scene` within a single facility can be recognized correctly; while combination like `Rosmontis`, `Pinus Sylvestris` among facilities is not supported yet.
- If `Usage of Drone` is selected with the option `Trading Post-LMD`, it will recognize `Shamare` and reserve it for her.
- Operators of corresponding fraction will be selected when only one Clue is needed Reception Room; otherwise general operators will be chosen.
- Reception Room will send out Clues only when your Clues are full. Three Clues will be send out at most. You can edit `SelectClue` - `maxTimes` field in `resource/tasks.json` to edit number of Clues sent if you want.
- Due to the complexity of Control Center, only `Amiya`, `Swire`, `Kal'tsit`, `Team Rainbow` and other Morale+0.05 operators will be considered. To be improved in future.
- Some alternate operators may have conflicts in Infrastructure. Please notice if there is "Operator conflict" warnings on the UI, and double check the Infrastructure to shift manually (e.g. some facilities may not have any operator).

### Recruitment Recognition

- Auto-recruitment and recruitment recognition are two different features!
- Auto-recruitment supports using `Expedited Plan` to make it fully automated! Please set it in the settings.
- Pop-up window when 5★, 6★ operators are recruited.

### Integrated Strategy (I.S.) Support

- Three options in the settings

  - Plays as much as you can: for getting candles.
  - Exits after first level: the most efficient mode for getting Originium Ingots. Easy mode is recommended.
  - Plays until trading: for getting both items.

- I.S. now supports recognition of operators and levels, and can choose an optimal solution of operators and skills.
- Supports auto-reconnecting after disconnection, and replaying after 4 AM.
- The program will abandon the operation and retry if it is stuck by some bug. If it is stuck frequently at some place and has severe impact on efficiency, please submit an issue to us.

### Share Your Tasks

Welcome to QQ Group: 912322156

#### Load Tasks

- Please run it on the screen with `Start Operation` button.
- If you need to make your own build manually, turn off `auto build` to do so.

#### Create Tasks

- A tool for creating tasks is provided in the directory of EXE file. See also: [Task schema documenatation](TASK_SCHEMA.md) for help.
- How to get map coordinates: start an operation after filling in `stage_name`. A file named `map.png` will be generated for your reference. Features like displaying map on the creator tool will be supported in future.
- Drill plan is recommended for testing.
- If the name of operator is not recognized correctly, check the log `asst.log` to see what is the recognition result, and edit `resource/tasks.json` - `CharsNameOcrReplace` to replace it.
- It is recommended to write your own name, video walkthrough URL, or other things that you think helpful in the description.

### Miscellaneous

- Tasks order can be changed on the UI. So can the shifting order in the infrastructure.
- All clicking event is randomized within a region, following Poisson distribution (higher probability at the center, lower probability around).
- Developed in C++, the core algorithm supports multi-level cache, in order to reduce CPU and memory usage as much as possible.
- Our software supports auto-update ✿✿ヽ(°▽°)ノ✿ Beta-testers can try beta versions, which updates faster and less buggy (maybe).
- If auto-update fails, you can download manually and put the ZIP file under the same directory. The update will start automatically.
