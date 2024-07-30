---
order: 9
---

# Copilot

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

Welcome to share your tasks with [prts.plus](https://prts.plus)!

::: warning
All features involving auto-battle require a stable game frame rate of at least 60 frames, including but not limited to Copilot and Auto I.S..
:::

## Load Tasks

支持任意 `可编队关卡` 和 `保全派驻` 模式的自动战斗。

- Please run it on the screen with `Start Operation` button.  
  之后在 MAA 左侧上部的框中 `导入本地 JSON 作业文件` 或 `填写作业站神秘代码` 即可导入作业。
- `Auto Squad` 功能会**清空当前编队**并根据作业需要的干员自动完成编队。
  - 可根据个人需要 (例如需要使用 `好友助战` 时)取消 `Auto Squad`，手动编队后开始。
  - 可根据任务需要为自动编队 `Add custom operators` 和 `Add low-trust operators`。
  - 对于「悖论模拟」关卡，必须关闭 `自动编队`，手动选择技能后，在有**开始模拟**按钮的界面开始自动战斗。
  - 对于「保全派驻」关卡，`自动编队` 无效，必须手动完成**初始**任务准备，直到在关卡详情有**开始部署**按钮的界面才能开始自动战斗。
- 可设置 `Loop Times`，例如保全。但 MAA 不会借干员，如需借干员请勿使用。
- 可使用 `Battle list` 功能进行同一区域关卡的自动连续战斗。
  - Battle list 下方三个按钮从左到右依次为 `批量导入`、`添加关卡`、`清空关卡`。  
    `添加关卡` 右键为添加突袭关卡，`清空关卡` 右键为清空未勾选关卡，
  - 导入作业后，战斗列表下方会出现关卡名，确认正确后再添加该关卡。列表中的关卡可以拖拽调整顺序，勾选是否执行。
  - 开启本功能后改为在**关卡所在的地图界面**开始自动战斗。在理智不足/战斗失败/非三星结算时将停止自动战斗队列。
  - 请确保列表中的关卡在同一区域 (只通过左右滑动地图界面就可以导航到)。
- Remember to like the tasks that you think helpful!
  ![image](/image/zh-cn/copilot-click-like.png)

## Create Tasks

- 请使用 [作业编辑器](https://prts.plus/create) 制作. A tool for creating tasks is provided in the directory of MAA. See also: [Copilot Schema](../../protocol/copilot-schema.md) for help.
- Get map coordinates：
  - 在作业编辑器中填写关卡后，左下角会自动加载可拖动缩放的坐标地图，可点选设置当前干员位置。
  - Start an operation after filling in `stage_name`. A file under `debug\map` named `map.png` will be generated for your reference.
  - Refer to [PRTS.map](https://map.ark-nights.com/), with the `coordinates` set to `MAA` mode.
- Drill plan is available for testing.
- It is recommended to write your own name, video walkthrough URL, or other things that you think helpful in the description.
