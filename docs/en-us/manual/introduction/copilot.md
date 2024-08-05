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

Supports automatic combat for any `Squad Formation Stage` and `Stationary Security Service` mode.

- Please run it on the screen with the `Start Operation` button.  
  Then, import the task by `Import Local JSON Task File` or `Enter Task Station Secret Code` in the upper left box of MAA.
- The `Auto Squad` feature will **clear the current squad** and automatically form a squad based on the operators required for the task.
  - You can disable `Auto Squad` and manually form the squad before starting if you need to use `Friend Support` or for other personal preferences.
  - You can add `custom operators` and `low-trust operators` for auto squad formation as needed.
  - For "Paradox Simulation" stages, you must disable `Auto Squad`, manually select skills, and start automatic combat on the screen with the **Start Simulation** button.
  - For "Stationary Security Service" stages, `Auto Squad` is ineffective. You must manually complete the **initial** task preparation until the **Start Deployment** button appears on the stage details screen to start automatic combat.
- You can set `Loop Times`, such as for Stationary Security Service. However, MAA will not borrow operators. Do not use this feature if you need to borrow operators.
- You can use the `Battle list` feature for automatic continuous combat in the same area stages.
  - The three buttons below the Battle list from left to right are `Batch Import`, `Add Stage`, and `Clear Stages`.  
    Right-click `Add Stage` to add a raid stage, and right-click `Clear Stages` to clear unchecked stages.
  - After importing the task, the stage names will appear below the battle list. Confirm they are correct before adding the stage. You can drag and adjust the order of the stages in the list and check whether to execute them.
  - After enabling this feature, start automatic combat on the **map screen where the stages are located**. The automatic combat queue will stop if sanity is insufficient, combat fails, or the settlement is not three stars.
  - Ensure that the stages in the list are in the same area (can be navigated by swiping the map screen left or right).
- Remember to like the tasks that you think helpful!
  ![image](/image/zh-cn/copilot-click-like.png)

## Create Tasks

- Please use the [Task Editor](https://prts.plus/create) to create tasks. A tool for creating tasks is provided in the directory of MAA. See also: [Copilot Schema](../../protocol/copilot-schema.md) for help.
- Get map coordinates：
  - After filling in the stage in the Task Editor, a draggable and zoomable coordinate map will automatically load in the lower left corner, where you can click to set the current operator positions.
  - Start an operation after filling in `stage_name`. A file under `debug\map` named `map.png` will be generated for your reference.
  - Refer to [PRTS.map](https://map.ark-nights.com/), with the `coordinates` set to `MAA` mode.
- Drill plan is available for testing.
- It is recommended to write your own name, video walkthrough URL, or other things that you think helpful in the description.
