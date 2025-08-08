---
order: 9
icon: ph:sword-bold
---

# Auto Combat

Welcome to use and share operation files on [prts.plus](https://prts.plus).

::: warning
All features involving automatic combat require a stable game frame rate of at least 60 FPS, including but not limited to Auto Combat and Integrated Strategy.
:::

## Using Operations

Supports automatic combat for any `Squad Formation Stage` and `Stationary Security Service` mode.

- This feature should be started from the squad selection screen where the `Start Operation` button is visible.  
  Then, import an operation by either `Import Local JSON Operation File` or `Enter Operation Code` in the upper left box of MAA.
- The `Auto Squad` feature will **clear the current squad** and automatically form a squad based on the operators required by the operation.
  - You need to unmark any specially focused operators that will be used in the auto squad.
  - You can add `custom operators` and `low-trust operators` to the auto squad as needed.
  - You can disable `Auto Squad` and manually form the squad before starting if needed (for example, when using `Friend Support`).
  - For "Paradox Simulation" stages, you must disable `Auto Squad`, manually select skills, and start automatic combat from the screen with the **Start Simulation** button.
  - For "Stationary Security Service" stages, `Auto Squad` is ineffective. You must manually complete the **initial** task preparation until the screen with the **Start Deployment** button appears before starting automatic combat.
- You can set `Loop Times`, such as for Stationary Security Service. However, MAA will not borrow operators, so don't use this if you need to borrow operators.
- You can use the `Battle List` feature for automatic continuous combat across stages in the same area.
  - The three buttons below the Battle List from left to right are `Batch Import`, `Add Stage`, and `Clear Stages`.  
    Right-clicking `Add Stage` adds a challenge mode stage; right-clicking `Clear Stages` clears all unchecked stages.
  - After importing an operation, the stage name will appear below the battle list. Confirm it's correct before adding the stage. You can drag stages to reorder them and check/uncheck to control execution.
  - When using this feature, start automatic combat from the **map screen where the stages are located**. The automatic combat queue will stop if sanity is insufficient, combat fails, or you don't achieve three stars.
  - Ensure all stages in the list are in the same area (navigable by swiping the map screen left or right).
- **Please remember to like high-quality operations to boost their ratings and encourage their creators.**  
  ![image](/images/zh-cn/copilot-click-like.png)

## Creating Operations

- Please use the [Operation Editor](https://prts.plus/create) to create operations. You can refer to the [Combat Operation Protocol](../../protocol/copilot-schema.md) for guidance.
- Getting map coordinates:
  - After entering the stage name in the Operation Editor, a draggable and zoomable coordinate map will automatically load in the lower left corner, where you can click to set operator positions.
  - If you export the JSON after entering the stage name and then start an operation, a map image with coordinate information will be generated in the `debug\map` folder in your MAA directory.
  - Use [PRTS.Map](https://map.ark-nights.com/areas) and change the `Coordinate Display` to `MAA` in settings.
- Practice plans are supported.
- We recommend including your name (as author), reference video links, and other helpful information in the operation description.
- You're welcome to join the QQ group [1169188429](https://jq.qq.com/?_wv=1027&k=QZcGcJ9G) to discuss operation creation and related topics.
