---
order: 10
icon: mdi:account-sync
---

# Update Doctor Data

::: info UI-Only Feature
This page covers UI-layer features. See [Getting Started](../newbie.md#about-this-documentation) for details.
:::

The Update Doctor Data task is used to **sync operator roster and depot inventory** cache data, providing accurate inventory baselines for features like [Depot Maintain](./depot-maintain.md).

## Features

This task includes two sub-items that can be independently toggled:

### Operator Recognition

- Recognizes owned and unowned operator rosters, and saves potential data.
- Results are used by [Recruitment Recognition](./tools.md#recruitment-recognition) to display operator potentials.
- Last sync time is displayed after recognition.

### Depot Recognition

- Recognizes in-game depot inventory and updates material quantity cache.
- Results are used by [Depot Maintain](./depot-maintain.md) to calculate shortfalls.
- Shares the same data as [Tools - Depot Recognition](./tools.md#depot-recognition). Last sync time is displayed after recognition.

::: tip
The difference between this task and [Tools - Depot Recognition](./tools.md#depot-recognition): the Tools version requires manually navigating to the depot screen; this task automatically navigates to the depot and can be combined with trigger intervals for automation.
:::

## Trigger Interval

Set the execution frequency of the task:

| Option     | Description                                                                              |
| :--------- | :--------------------------------------------------------------------------------------- |
| Every time | Execute the update every time the task queue runs.                                       |
| Daily      | Execute at most once per day; automatically skipped on subsequent triggers the same day. |
| Weekly     | Execute at most once per week.                                                           |

Recommended to set to "Daily" for automatic daily data sync on first run, avoiding frequent OCR that impacts efficiency.
