---
order: 14
icon: mdi:theme-light-dark
---

# Switch Theme

Switches the game's main interface UI theme.

## Usage

1. Add the ｢Switch Theme｣ task to the task queue.
2. Enter candidate theme names in the task settings, as shown in the in-game theme list (e.g. ｢Daytime｣ for Light Theme, ｢Night｣ for Dark Theme on the EN server).
3. With multiple names, one is picked at random each run; with a single name, it switches to it every time.
4. If no theme name is entered, the task is skipped with a log message.

The task returns to the main interface first, then searches the theme list for the target: it checks the current page, then quickly scrolls to the top without stopping and searches downward screen by screen. Theme names are matched via text recognition, and rarely used characters may be misrecognized as similar-looking ones, making the theme not found (EN text detection should be more consistent) (e.g. 凇 misread as 淞; those seen in testing are already corrected built-in). If a theme is still not found, submit the log archive via Issue Report — the log contains what was actually recognized, which helps add a correction.

::: tip Restoring a theme switched away by the fallback
After switching to a newly released theme that MAA has not yet been adapted to, subsequent tasks may fail to recognize its main interface and trigger the fallback that switches back to the day theme. Place this task at the end of the task queue and fill in the theme name to restore it after all tasks finish.
:::
