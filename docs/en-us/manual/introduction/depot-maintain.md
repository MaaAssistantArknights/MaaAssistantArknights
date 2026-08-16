---
order: 4
icon: mdi:package-variant-closed
---

# Depot Maintain

::: info UI-Only Feature
This page covers UI-layer features. See [Getting Started](../newbie.md#about-this-documentation) for details.
:::

Depot Maintain is a task that **automatically farms materials to a target inventory level**. It calculates the shortfall based on the cached depot data from [Depot Recognition](./tools.md#depot-recognition) and farms the specified stage until the target quantity is reached.

## How It Works

1. At the start of the task (if "Update depot data before starting" is checked), a depot recognition is performed to get the latest inventory.
2. For each plan, the shortfall is calculated by subtracting the current quantity from the target.
3. Fight tasks are automatically added for materials with shortfalls.
4. After each stage drop, the depot cache is updated in real-time, so materials farmed by earlier plans affect the shortfall calculation of later plans.
5. Right before each plan starts, MAA re-checks whether it needs to run: if the target inventory is already reached, or sanity is below the stage cost with no potion/Originium budget available, the plan is skipped entirely (no terminal, no navigation), with the reason logged.

::: tip Depot Data Sync
Depot data is cached and may differ from your actual stock after manual farming, crafting, or material use. Sync it with [Update Doctor Data](./user-data-update.md) or [Depot Recognition](./tools.md#depot-recognition).
:::

## Plan Configuration

Each plan contains the following:

| Setting           | Description                                                                                                            |
| :---------------- | :--------------------------------------------------------------------------------------------------------------------- |
| Stage Selection   | Select the stage to farm. Supports regular stages, chip stages, resource stages, etc. "Current/Last" is not supported. |
| Specified Drop    | Select the target drop material. Each stage may have multiple drops; choose the one you actually need.                 |
| Target Inventory  | Set the target quantity for the material. The plan completes automatically when reached.                               |
| Use Sanity Potion | Check to set the number of sanity potions to use.                                                                      |
| Use Originium     | Check to set the number of Originium to use.                                                                           |

Items in the plan list can be **dragged to reorder**. Plans are executed in order during the task.

::: tip Skipping when sanity is insufficient
- Plans with a sanity potion or Originium budget are never skipped for insufficient sanity: even when sanity is not enough, they still enter the stage and restore sanity with the budget to keep fighting.
- Plans without a budget are skipped directly (without entering the stage) when the target inventory is already reached, or when sanity is below the stage cost and no expiring potion is usable.
- Decisions always use the latest state: after a middle plan restores sanity and reaches its target, the remaining sanity still flows to later plans.
:::

### Presets

Built-in presets for quick plan population:

- **Low-tier Chips (all classes)**: PR-A/B/C/D-1, target 20 each
- **High-tier Chip Packs (all classes)**: PR-A/B/C/D-2, target 20 each
- **LMD**: CE-6, target 2,000,000
- **Purchase Certificates (Red Tickets)**: AP-5, target 5,000
- **Skill Summary SP3**: CA-5, target 200

## Advanced Settings

### Update depot data before starting

When checked, a depot recognition is performed at task start to get the latest inventory. Uncheck to use the last cached data.

Recommended to keep checked unless you are certain the cache is accurate or want to save recognition time.

### AUTO Proxy Multiplier

When unchecked, farming is done at single (1×) proxy. When checked, the maximum proxy multiplier available for current sanity is used, which may exceed the target inventory in a single run.

### Enable "Use Sanity Potion" / "Use Originium" checkboxes

Controls whether the corresponding potion/Originium rows are shown in each plan. Disabling uniformly prevents all plans from using potions or Originium.

### Use expiring sanity potions within 48 hours

When checked, all plans will prioritize using sanity potions expiring within 48 hours (fixed 2-day threshold). Plans without a potion budget will also try to use expiring potions; such plans are only skipped for insufficient sanity after some fight in this queue run has already ended due to insufficient sanity (i.e., potions within this window are used up).

### Skip during events

When checked, the entire Depot Maintain task is skipped if an event is detected as active.

### Skip during resource collection limited-time full-day open

When checked, the entire Depot Maintain task is skipped if resource collection stages are in limited-time full-day open period.
