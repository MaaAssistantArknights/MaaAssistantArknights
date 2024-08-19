---
order: 7
---

# Integrated Strategy (I.S.)

::: important This page may be outdated.
:::

MAA selects the latest theme by default, and can be changed in `Auto I.S.` - `General`.

::: warning
All features involving auto-battle require a stable game frame rate of at least 60 frames, including but not limited to Copilot and Auto I.S..
:::

- Please pin the corresponding I.S. theme to the terminal of the game. Although automatic navigation is currently available, long-term usability is not guaranteed.
- If there is an exploration of non-target themes (such as if you plan to use MAA to auto Mizuki, but there is still an unfinished exploration of Phantom), please end it manually.
- MAA will not automatically select the difficulty. If the difficulty is not selected, it will get stuck/repeatedly enter and exit the difficulty selection interface.
- In the settings, you can choose the team, starting operator (only one operator name), etc.

## Battle Strategy

MAA does not have AI capabilities; all operations in automatic Integrated Strategy are pre-set strategies, and all stage battles use built-in job files.

For more details, please refer to the [Integrated Strategy Schema](../../protocol/integrated-strategy-schema.md).

- It supports automatic recognition of operators and proficiency and automatically selects better operators and skills.
- It supports identifying store items and prioritizes purchasing more powerful relics.

## Abnormal Detection

- It supports reconnection after disconnection and supports continuation to auto after 4 a.m. server reset.
- Encounters longer than 5 minutes will automatically retreat all ground units, and longer than 6 minutes will automatically abandon the operation to avoid wasting time/getting stuck.
- If the exploration gets stuck, it will automatically abandon and retry.

However, if it often gets stuck in a certain place and then gives up, seriously affecting efficiency, please feel free to submit an issue for feedback.
