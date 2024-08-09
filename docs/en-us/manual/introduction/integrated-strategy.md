---
order: 7
---

# Integrated Strategy (I.S.)

::: important Translation Required
This page is outdated and maybe still in Simplified Chinese. Translation is needed.
:::

MAA selects the latest theme by default, and can be changed in `Auto I.S.` - `General`.

::: warning
All features involving auto-battle require a stable game frame rate of at least 60 frames, including but not limited to Copilot and Auto I.S..
:::

- Please pin the corresponding I.S. theme to the terminal in the game. 虽然目前也可以自动导航，但不保证长期可用性。
- If there is exploration of non-target themes (such as if you plan to use MAA to brush Mizuki, but there is still an unfinished exploration of Phantom), please end it manually.
- MAA will not automatically select the difficulty. If the difficulty is not selected, it will get stuck/repeatedly enter and exit the difficulty selection interface.
- In the settings, you can choose the team, starting operator (only one operator name), etc.

## Battle Strategy

MAA 没有 AI 功能，自动肉鸽中的一切操作都是预置的策略，所有关卡战斗都是调用内置的作业文件。

详情请查阅 [肉鸽协议](../../protocol/integrated-strategy-schema.md)。

- It supports automatic recognition of operators and proficiency, and automatically selects better operators and skills.
- It supports identifying store items and prioritizes purchasing more powerful collectibles.

## Abnormal Detection

- It supports reconnection after disconnection and supports continuing to return to brush after 4 a.m. update.
- If the scraping cannot be completed during the battle, all ground units will be automatically withdrawn after more than 5 minutes; if it exceeds 6 minutes, the current battle will be automatically abandoned without getting stuck.
- If the task gets stuck, it will automatically abandon the exploration and retry.

However, if it often gets stuck in a certain place and then gives up, seriously affecting efficiency, please feel free to submit an issue for feedback.
