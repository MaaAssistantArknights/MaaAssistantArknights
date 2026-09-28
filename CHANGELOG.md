## v6.19.0-beta.1

### Highlights

#### 从指定任务处继续运行

任务列表新增 ｢从此处运行｣ 右键菜单项，可从指定任务处开始执行任务队列，跳过其之前的任务，任务失败后无需从头重跑；指定任务未启用时从其后首个已启用的任务开始。

#### 任务栏进度按 Core 任务链计数

任务栏进度条改为按 Core 任务链计数，执行一轮任务时按本轮产生的任务链逐条推进，更准确反映整体执行进度。

<details>
<summary><b>English</b></summary>

#### Resume the Queue from a Selected Task

A "Run from Here" option is added to the task list context menu, letting you start the queue from any task and skip the ones before it, so a failed run no longer has to restart from scratch. If the selected task is disabled, the queue starts from the first enabled task after it.

#### Taskbar Progress by Core Task Chains

The taskbar progress bar now counts Core task chains: within a run, progress advances as each chain completes, giving a more accurate view of overall execution.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.19.0-beta.1 (2026-09-28)</b></summary>

### 新增 | New

* 任务列表支持右键 ｢从此处运行｣ ，从指定任务处开始执行任务队列，跳过其之前的任务；指定任务未启用时从其后首个已启用的任务开始 ([#18317](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18317)) @H2O-MERO

### 改进 | Improved

* 优化黑流树海肉鸽事件选择，按事件策略规则选择选项，界面日志显示实际选择的选项与策略理由 ([#18339](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18339)) @ZiyinLin
* 任务栏进度改为按 Core 任务链计数 ([#18338](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18338)) @ABA2396

### 修复 | Fix

* 修复黑流树海肉鸽 ｢失与得｣ ｢先行一步｣ 等事件卡住或报任务出错的问题 ([#18339](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18339)) @ZiyinLin
* 修复基建常规换班将本轮已安排的干员重复分配到其他设施的问题 ([#18346](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18346)) @youzibigg
* 修复基建换班评分误将 ｢泰拉的方舟｣ （人力办公室技能）按加工站技能评分的问题 @Lancarus
* YostarJP add the StageEncounterOptionUnknown template for the Sami roguelike ([#18277](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18277)) @LinYanxi0
* YostarKR adjust the notification dot roi of AutoRaisePotential @HX3N

### 文档 | Docs

* 集成协议文档补充自动提升潜能任务（AutoRaisePotential）的参数说明 @ABA2396

</details>
