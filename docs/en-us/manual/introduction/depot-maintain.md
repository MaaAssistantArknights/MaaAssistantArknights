---
order: 12
icon: mdi:package-variant-closed
---

# Depot Maintain

::: info UI-Only Feature
This page covers UI-layer features. See [Getting Started](../newbie.md#about-this-documentation) for details.
:::

Depot Maintain is a task that **automatically farms materials to a target inventory level**. It calculates the shortfall based on the cached depot data from [Depot Recognition](./tools.md#depot-recognition) and farms the specified stage until the target quantity is reached.

## How It Works

1. After clicking ｢Link Start!｣ and before any task actually executes, the shortfall is calculated based on the current cached depot data and each plan's target inventory (hereafter referred to as pre-check). Plans whose cached quantity already meets the target are skipped; only plans with a shortfall have corresponding fight tasks added. If ｢Update inventory before starting｣ is checked, a depot recognition is prepended before this task's fight subtasks.
2. During execution, the prepended depot recognition (if checked) refreshes the depot cache first.
3. Before each plan's fight starts, the shortfall is recalculated using the latest depot cache.
4. After each stage drop, the depot cache is updated in real-time, so materials farmed by earlier plans affect the shortfall calculation of later plans.

::: tip Depot Data Sync
Depot data is cached and may differ from your actual stock after manual farming, crafting, or material use. Sync it with [Update Doctor Data](./user-data-update.md) or [Depot Recognition](./tools.md#depot-recognition).
:::

## Plan Configuration

Each plan contains the following:

| Setting           | Description                                                                                                            |
| :---------------- | :--------------------------------------------------------------------------------------------------------------------- |
| Stage Selection   | Select the stage to farm. Supports regular stages, chip stages, resource stages, etc. ｢Current/Last｣ is not supported. |
| Specified Drop    | Select the target drop material. Each stage may have multiple drops; choose the one you actually need.                 |
| Target Inventory  | Set the target quantity for the material. The plan completes automatically when reached.                               |
| Use Sanity Potion | Check to set the number of sanity potions to use.                                                                      |
| Use Originium     | Check to set the number of Originium to use.                                                                           |

Items in the plan list can be **dragged to reorder**. Plans are executed in order during the task.

### Presets

Built-in presets for quick plan population:

- **Low-tier Chips (all classes)**: PR-A/B/C/D-1, target 20 each
- **High-tier Chip Packs (all classes)**: PR-A/B/C/D-2, target 20 each
- **LMD**: CE-6, target 2,000,000
- **Purchase Certificates (Red Tickets)**: AP-5, target 5,000
- **Skill Summary SP3**: CA-5, target 200

## Advanced Settings

### Update depot data before starting

When checked, a depot recognition is performed at task execution to get the latest inventory. Uncheck to use the last cached data.

Recommended to keep checked unless you are certain the cache is accurate or want to save recognition time.

::: warning Pre-check uses old cache
The pre-check happens before any task actually executes (i.e., after clicking ｢Link Start!｣ and before Core starts running tasks), using the **last cached depot data**, not the result of this run's recognition. Even if this option is checked, depot recognition only executes after the pre-check. Therefore:

- Plans whose cached quantity already meets the target during pre-check are **skipped directly** and will **not** be re-checked after depot recognition refreshes the cache.
- Only plans that were not skipped will have their shortfall recalculated using the latest cache when their respective fights start.

If the cache may be outdated (e.g., after manual farming or crafting), it is recommended to manually sync once via ｢Toolbox → Depot Recognition｣ before starting the task to avoid incorrectly skipping materials that still need farming.
:::

### AUTO Proxy Multiplier

When unchecked, farming is done at single (1×) proxy. When checked, the maximum proxy multiplier available for current sanity is used, which may exceed the target inventory in a single run.

### Enable ｢Use Sanity Potion｣ / ｢Use Originium｣ checkboxes

Controls whether the corresponding potion/Originium rows are shown in each plan. Disabling uniformly prevents all plans from using potions or Originium.

### Use expiring sanity potions within 48 hours

When checked, all plans will prioritize using sanity potions expiring within 48 hours (fixed 2-day threshold).

### Skip during events

When checked, the entire Depot Maintain task is skipped if an event is detected as active.

### Skip during resource collection limited-time full-day open

When checked, the entire Depot Maintain task is skipped if resource collection stages are in limited-time full-day open period.
