---
order: 12
icon: ph:sword-bold
---

# Auto Combat

Welcome to use and share operation files on [prts.plus](https://prts.plus).

::: warning
All features involving Copilot require the following prerequisites, including but not limited to Copilot and Auto I. S.

- At least 60 frames of **stable** framerate
- Touch mode set to Minitouch, MaaTouch, or MuMu Touch Enhancement

:::

## Using Operations

Supports automatic combat for any `Squad Formation Stage` and `Stationary Security Service` mode.

- This feature should be started from the squad selection screen where the `Start Operation` button is visible.  
  Then, import an operation by either `Import Local JSON Operation File` or `Enter Operation Code` in the upper left box of MAA.
- The `Auto Squad` feature will **clear the current squad** and automatically form a squad based on the operators required by the operation.
  - You need to unmark any specially focused operators that will be used in the auto squad.
  - You can add `custom operators` and `low-trust operators` to the auto squad as needed.
  - You can disable `Auto Squad` and manually form the squad before starting if needed (for example, when using `Friend Support`).
  - In single-strategy mode, for "Paradox Simulation" stages, you must disable `Auto Squad`, manually select skills, and start automatic combat from the screen with the **Start Simulation** button.
  - For "Stationary Security Service" stages, `Auto Squad` is ineffective. You must manually complete the **initial** task preparation until the screen with the **Start Deployment** button appears before starting automatic combat.
- You can set `Loop Times`, such as for Stationary Security Service. However, MAA will not borrow operators, so don't use this if you need to borrow operators.
- You can use the `Multi-Job mode` feature for automatic continuous combat across stages in the same area.
  - The three buttons below the Job List from left to right are `Import task files`, `Add`, and `Clear`.  
    Left-clicking `Add` adds a normal difficulty stage, and right-clicking it adds a challenge difficulty stage; left-clicking `Clear` clears all stages, and right-clicking it clears unchecked stages.
  - After importing an operation, the stage name will appear below the job list. Confirm it's correct before adding the stage. You can drag stages to reorder them and check/uncheck to control execution.
  - When using this feature, start automatic combat from the **map screen where the stages are located**. The automatic combat queue will stop if sanity is insufficient, combat fails, or you don't achieve three stars.
  - Ensure all stages in the list are in the same area (navigable by swiping the map screen left or right).
- **Please remember to like high-quality operations to boost their ratings and encourage their creators.**  
  ![image](/images/zh-cn/copilot-click-like.png)

Enable `Automatically select strategies` in **Auto Combat → Paradox Simulation** and start from the in-game operator list without importing strategies. MAA searches PRTS Plus, selects up to 3 candidates per stage by rating and popularity, reads the uncleared list, verifies operator details, and tries backups on failure. Cleared, locked, and unsupported operators are skipped. Strategies are cached for 24 hours; completion is read from the game. Currently only Chinese Official and Bilibili servers are supported. Search depends on authors including “悖论模拟” in the title or description, so coverage and successful clears are not guaranteed. Recognition errors stop the task; operators whose candidates all fail are not retried again in the same run.

## Creating Operations

- Please use the [Operation Editor](https://prts.plus/create) to create operations. You can refer to the [Combat Operation Protocol](../../protocol/copilot-schema.md) for guidance.
- Getting map coordinates:
  - After entering the stage name in the Operation Editor, a draggable and zoomable coordinate map will automatically load in the lower left corner, where you can click to set operator positions.
  - If you export the JSON after entering the stage name and then start an operation, a map image with coordinate information will be generated in the `debug\map` folder in your MAA directory.
  - Use [PRTS.Map](https://map.ark-nights.com/areas) and change the `Coordinate Display` to `MAA` in settings.
- Practice plans are supported.
- We recommend including your name (as author), reference video links, and other helpful information in the operation description.
- You're welcome to join the QQ group [1169188429](https://jq.qq.com/?_wv=1027&k=QZcGcJ9G) to discuss operation creation and related topics.
