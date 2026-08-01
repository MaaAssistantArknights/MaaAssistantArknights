---
name: side-story-navigation
description: 新增/更新 SideStory 活动关卡导航。涵盖 tasks/Stages 导航任务、StageActivityV2.json 活动配置、MaaRelease 仓库同步三个文件的联动修改。用于用户要求添加活动导航、更新活动关卡、同步 MaaRelease 时使用。
---

# SideStory 活动导航 Skill

新增或更新一个 SideStory 活动的关卡导航，涉及多个文件的联动修改。

## 必需信息清单

开始前必须向用户收集以下信息；用户提供了部分后，列出仍缺少的项目。

| # | 信息项 | 说明 | 示例 |
|---|--------|------|------|
| 1 | 活动前缀代号 XX | 两字母关卡前缀 | `TO` |
| 2 | 活动全名 | 填入 StageActivityV2 的 Tip/StageName | `直到大地变成一颗酸橙` |
| 3 | OpenOcr 识别文字 | 活动入口横幅文字，**会随活动进度变化**：第一期是活动名，后续 EX/新章节开放后变为对应章节内容，需要提前写好所有阶段 | `直到大地变成一颗酸橙, 踏上归家长途, 眺望待行之路` |
| 4 | 关卡编号列表 | 通常后三关 + 可选搓玉关 | `7, 8, 9` 或 `4, 7, 8, 9` |
| 5 | 各关卡掉落物名称（非搓玉关） | 用于查 `item_index.json` 取 ID | `糖组、凝胶、晶体元件` |
| 6 | 搓玉关掉落描述（如有搓玉关） | 直接填入 Drop 字段，不查表 | `搓玉效率0.98` |
| 7 | 关卡开放时间 | 起止时间 | `8.1 12:00 — 8.22 03:59` |
| 8 | 选章方式 | OCR 文字 / 模板图 | OCR「驶向大地尽头」或模板图 |
| 9 | 选章识别内容 | OCR 文字或模板图文件名 | `驶向大地尽头` |
| 10 | 选章 roi 坐标 | 截图中的位置，用户可能用 `[x, y, w, h]` 或 `x+w y+h` 两种格式之一 | `[1040, 420, 150, 50]` 或 `x1040+150 y420+50` |
| 11 | 进活动后等待时间 | postDelay 毫秒数 | `5000`（等 5 秒） |

> 若选章方式为模板图，还需额外提醒用户提供模板图片，并准备修改 `MinimumRequired` 版本号。

## 0. 术语

| 简称 | 含义 |
|------|------|
| **XX** | 活动前缀代号，如 `TD`、`TO`、`AT`，取自关卡名 `XX-7` 的前半 |
| **tasks 导航文件** | `resource/tasks/Stages/XX.json` |
| **StageActivityV2** | `build/bin/Debug/cache/gui/StageActivityV2.json`（主仓库的缓存副本） |
| **MaaRelease** | 上级目录 `../MaaRelease/MaaAssistantArknights/api/` 下的镜像文件 |

## 1. 涉及的文件

| # | 文件路径 | 必需性 | 作用 |
|---|----------|--------|------|
| 1 | `resource/tasks/Stages/XX.json` | ✅ 必须 | 关卡导航任务定义（OCR 识别活动入口、选章、找关） |
| 2 | `build/bin/Debug/cache/gui/StageActivityV2.json` | ✅ 本地测试 | 本地构建缓存，客户端运行时从远程 API（`gui/StageActivityV2.json`）拉取，改此文件用于本地测试 |
| 3 | `../MaaRelease/MaaAssistantArknights/api/gui/StageActivityV2.json` | ✅ 必须 | MaaRelease 远程源文件，StageActivityV2 的真正来源 |
| 4 | `../MaaRelease/MaaAssistantArknights/api/resource/tasks.json` | 🔶 可选 | MaaRelease 精简 tasks 覆盖层（只放活动导航，依赖项由客户端本地资源提供） |

## 2. tasks 导航文件结构（`resource/tasks/Stages/XX.json`）

参考往期活动（如 `AT.json`、`ME.json`、`AveMujica.json`）。

### 2.1 关卡范围

- 通常是活动的**后三关**（如 7/8/9、6/7/8、8/9/10），但具体关卡编号由用户提供。
- 可能有额外的低关卡用于**搓玉**（如第 4 关、第 5 关），搓玉关与后三关一并加入导航。

### 2.2 任务定义模板

```jsonc
{
    // —— 关卡入口（按编号从小到大）——
    "XX-4": {
        "algorithm": "JustReturn",
        "sub": ["XX-OpenOpt"],
        "next": ["XX-4@SideStoryStage", "XX-4@SwipeToStage"]
    },
    "XX-7": { ... },
    "XX-8": { ... },
    "XX-9": { ... },

    // —— 关卡识别（按编号从小到大）——
    "XX-4@SideStoryStage": { "text": ["XX-4"] },
    "XX-7@SideStoryStage": { "text": ["XX-7"] },
    "XX-8@SideStoryStage": { "text": ["XX-8"] },
    "XX-9@SideStoryStage": { "text": ["XX-9"] },

    // —— 活动入口 ——
    "XX-OpenOpt": {
        "algorithm": "JustReturn",
        "next": ["XX-OpenOcr", "XX-Open"]
    },
    "XX-Open": {
        "baseTask": "SS-Open",
        "postDelay": 5000,              // 进活动后等待时间，按需调整
        "template": ["StageSideStory.png", "StageActivity.png"],
        "next": ["XXChapterToXX"]
    },
    "XX-OpenOcr": {
        "baseTask": "SS-OpenOcr",
        "text": [...],                   // 见下方 OCR 文本规则
        "postDelay": 5000,
        "next": ["XXChapterToXX"]
    },

    // —— 选章（进入活动后选择关卡列表）——
    "XXChapterToXX": {
        // 方式 A：OCR 识别（章节名可被 OCR 时优先使用）
        "algorithm": "OcrDetect",
        "action": "ClickSelf",
        "text": ["章节名"],
        "roi": [x, y, w, h],
        "postDelay": 2000,
        "next": ["#self", "SideStoryStage", "ChapterSwipeToTheRight"]
    }
}
```

### 2.3 `XXChapterToXX` 选章任务的两种方式

| 方式 | 适用场景 | algorithm |
|------|----------|------------|
| **A. OCR** | 章节名是可识别的文字 | `OcrDetect` |
| **B. 模板图** | 章节名不可 OCR（特殊字体/图标） | `MatchTemplate` |

方式 B 示例：
```jsonc
"XXChapterToXX": {
    "action": "ClickSelf",
    "template": "XXChapterToXX.png",
    "roi": [x, y, w, h],
    "postDelay": 2000,
    "next": ["#self", "SideStoryStage", "ChapterSwipeToTheRight"]
}
```

> ⚠️ **使用模板图时的联动操作**：需要新增 `XXChapterToXX.png` 模板图片到 `resource/template/StageNavigation/SideStory/XX/` 目录，并且 **必须修改 StageActivityV2.json 中的 `MinimumRequired`**（详见 §3.3）。

### 2.4 OCR 文本规则

- **长活动名**切成多段 4 字短词，同时保留原文，混排在 `text` 数组中。
  - 原文放前面，短词兜底放后面。
  - 示例：`"text": ["直到大地变成一颗酸橙", "踏上归家长途", "眺望待行之路", "直到大地", "一颗酸橙", "踏上归家", "归家长途", "眺望待行", "待行之路"]`
  - 原因：全字匹配任一错字就失败；短词提高容错，但不可太短（≥4 字）避免误匹配。

### 2.5 排列顺序规则

> **tasks 导航文件中关卡按编号从小到大排列。**

先放所有入口任务（`XX-4`/`XX-7`/`XX-8`/`XX-9`），再放所有 `@SideStoryStage`（`XX-4@SideStoryStage`/`XX-7@SideStoryStage`/…），最后放 Open/Chapter 相关。

### 2.6 roi 坐标

由用户提供截图坐标。格式为 `[x, y, width, height]`。
- `XXChapterToXX` 的 roi 通常由用户指定形如 "x1040+150 y420+50"，即 `[1040, 420, 150, 50]`。

## 3. StageActivityV2.json

### 3.1 位置

- 本地测试缓存：`build/bin/Debug/cache/gui/StageActivityV2.json`（客户端运行时从远程 API 拉取到本地缓存，修改此文件用于本地测试）
- 远程源文件：`../MaaRelease/MaaAssistantArknights/api/gui/StageActivityV2.json`（StageActivityV2 的真正来源，需 commit 推送）
- 两者内容必须一致。

### 3.2 替换规则

将旧活动条目整体替换为新活动。键名为活动前缀（如 `"TO"`），结构如下：

```jsonc
"TO": {
    "MinimumRequired": "v6.11.0",       // 见 §3.3 版本号规则
    "Activity": {
        "Tip": "SideStory「活动全名」",
        "StageName": "活动全名",
        "UtcStartTime": "2026/08/01 12:00:00",
        "UtcExpireTime": "2026/08/22 03:59:59",
        "TimeZone": 8
    },
    "Stages": [
        { "Display": "TO-9", "Value": "TO-9", "Drop": "31033" },
        { "Display": "TO-8", "Value": "TO-8", "Drop": "31013" },
        { "Display": "TO-7", "Value": "TO-7", "Drop": "30023" },
        { "Display": "TO-4", "Value": "TO-4", "Drop": "搓玉用" }
    ]
}
```

### 3.3 MinimumRequired 版本号规则

| `XXChapterToXX` 方式 | 版本号处理 |
|----------------------|------------|
| **A. OCR**（已有任务可复用） | 沿用旧活动的值，通常无需改动 |
| **B. 模板图**（需新增模板） | 必须改为**当前最新 tag 的下一个版本** |

模板图方式的具体操作：
1. 查看当前最新版本号（如 `git describe --tags` 得到 `v6.15.1`）。
2. 将 `MinimumRequired` 改为下一个 patch 版本（如 `v6.15.2`）。
3. **通知用户复查**该版本号是否正确，因为新模板图片需要随新版本客户端分发。

### 3.4 排列顺序规则

> **StageActivityV2 的 Stages 数组按编号从大到小排列。**

高难关卡（高掉落价值）排在前面，搓玉等低关卡排在后面。

### 3.5 Drop 字段

| 关卡类型 | Drop 取值 | 获取方式 |
|----------|------------|----------|
| 后三关（常规掉落） | item_index ID（如 `"31033"`） | 在 `resource/item_index.json` 中按材料名查表取 key |
| 搓玉关等特殊关 | 用户提供的文字描述（如 `"搓玉效率0.98"`） | **直接使用用户提供的描述，不查表** |

> 💡 **知识**：个别关卡可能有独立的 `MinimumRequired`（如某关卡需要更新版本才能解锁），可单独加到该关卡的条目中：`{ "Display": "XX-4", "Value": "XX-4", "Drop": "搓玉用", "MinimumRequired": "v6.16.0" }`。但用户通常不会指定，不需要主动询问。

## 4. MaaRelease 仓库同步（可选）

> MaaRelease 仓库的同步是**可选操作**。用户本地可能没有拉取该仓库，或者仓库不在上级目录。**操作前先检查 `../MaaRelease/` 是否存在，不存在则跳过此步骤。**

### 4.1 前置检查

```powershell
# 检查上级目录是否有 MaaRelease
Test-Path ..\MaaRelease
```

若存在则 `git pull` 更新到最新。

### 4.2 需要同步的文件

| 文件 | 操作 |
|------|------|
| `api/gui/StageActivityV2.json` | 与主仓库缓存保持一致 |
| `api/resource/tasks.json` | 将新活动导航任务追加到文件**开头**（`BlackFlowTemporary` 之前） |

### 4.3 tasks.json 追加规则

- MaaRelease 的 `tasks.json` 是精简覆盖层，**只放活动导航任务本身**，不放依赖项（`SS-Open`、`SideStoryStage`、`SwipeToStage`、`ChapterSwipeToTheRight` 等基础任务由客户端本地资源提供）。
- 新内容放**文件开头**（`{` 之后、第一个已有 key 之前），不是末尾。
- 内容与主仓库 `resource/tasks/Stages/XX.json` 完全一致。

## 5. 排列顺序速查

| 文件 | 顺序 |
|------|------|
| `resource/tasks/Stages/XX.json` | 关卡编号**从小到大** |
| MaaRelease `api/resource/tasks.json` | 同上，**从小到大** |
| `StageActivityV2.json` 的 `Stages` 数组 | 关卡编号**从大到小** |

## 6. 完整操作清单

1. ✅ 创建/修改 `resource/tasks/Stages/XX.json`（导航任务，关卡从小到大）
2. ✅ 修改 `build/bin/Debug/cache/gui/StageActivityV2.json`（本地测试缓存，替换活动，关卡从大到小，Drop 查表或用用户描述）
   - 若 `XXChapterToXX` 用模板图：修改 `MinimumRequired` 为下一个版本，通知用户复查
3. 🔶 检查 `../MaaRelease/` 是否存在；存在则：
   - `cd ../MaaRelease; git pull`
   - 修改 `api/gui/StageActivityV2.json`（远程源文件，同步骤 2）
   - 修改 `api/resource/tasks.json`（导航任务追加到开头，关卡从小到大）
