---
name: changelog
description: 根据 git 提交记录、diff、现有 CHANGELOG 与 tag，整理符合 MAA 发布规范的 changelog Markdown。过滤噪音、合并同类改动，输出可直接写入 CHANGELOG.md 的最终版本。
---

# MAA Changelog Skill

## 目标

读取待发布范围内的 commit、diff、现有 CHANGELOG 与历史 tag，输出可直接写入 `CHANGELOG.md` 的最终 Markdown。

- **只输出最终 Markdown**，不输出分析过程、分类理由或代码围栏。
- 第一目标："最终用户看得懂、历史版本不断裂、同类改动不重复"。

## 必要输入

- 目标版本号（如 `v6.13.0-beta.3`）。
- 待发布提交范围（可由 tag、PR、分支信息推导）。
- 当前 CHANGELOG 内容。

## 核心规则

### 1. 净变更优先

- 同一功能/问题的多条相关 commit **合并为单条**，面向用户描述最终效果。
- commit 标题含糊、口语化、玩梗时，**必须查看 diff 后改写**为专业可理解的描述。
- **Revert 处理**：原改动被完整撤销则删除该项；仍保留部分语义则合并为一条准确描述最终结果的条目。
- "review""typo""日志顺序""调整坐标""build warning" 等缺乏用户语义的提交不单独保留，除非 diff 证明修复了用户可感知问题。
- **多服同类改动合并**：按**单项改动**（而非 commit/PR 整体）为粒度拆分，每项改动再跨服合并。同一 commit/PR 中若包含多项不同改动，应先按改动类型拆开，再将每项在实现了它的所有服务器上合并。服务器名用 `/` 连接（如 `YostarEN/JP/KR`），多位作者依次排列（如 `@author1 @author2 @author3`）。
  - 示例：某 PR 为 JP 同时更新了主题、新增了章节导航，而 EN/KR 只更新了主题。应拆为两行：`YostarEN/JP/KR 更新主题`（三服共同）+ `YostarJP 新增章节导航`（仅 JP）。

### 2. 按用户价值分类

| 模块 | 适用场景 |
|------|----------|
| **新增 \| New** | 新功能、新支持、新入口、新兼容性 |
| **改进 \| Improved** | 能力增强、性能/稳定性/体验优化、识别优化、重构收益 |
| **修复 \| Fix** | 缺陷修正、兼容性/异常/回归修复 |
| **文档 \| Docs** | 纯文档变更 |
| **其他 \| Other** | 仅内部维护、CI、脚本等（不适合省略时） |
| **MaaMacGui** | 子仓库独立区块，放在 `### 其他 \| Other` 之后，内部复用相同分类结构，PR 格式 `([#数字](https://github.com/MaaAssistantArknights/MaaMacGui/pull/数字))` |

### 3. 模块内排序与文案

- **中文在前，纯英文条目排最后**。
- 按重要性排序：功能/接口变更 > 兼容性/优化 > 次要修复/杂项。
- 列表前缀统一 `*`。
- 中英文与数字间留空格（如"修复 3 个 bug""支持 3D 功能"）。
- 术语统一大小写：WPF、Json、Markdown、CSV、Info。
- 保留作者与 PR 引用，主仓库格式为 `([#12345](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12345)) @author`；多条合并时引用合并括注。

### 4. 版本历史连续性

**版本类型定义**：不带 `-beta`/`-alpha` 后缀的均为正式版（包含首个正式版 `X.Y.0` 与 patch 正式版 `X.Y.1`、`X.Y.2` 等）；带后缀的为测试版。

- **跨次版本号的正式版**（如 v6.12.x → v6.13.0）：不保留上一个次版本号的任何历史折叠块。
- **同次版本号的 patch 正式版**（如 v6.13.1 相对 v6.13.0）：当前版本只写相对上一版本的增量变化（放在 `<details open>` 展开块），不得复制更早版本已发布条目。更早版本保留为各自独立的 `<details>` 收起块，紧跟在当前版本展开块之后。

### 5. 测试版与 patch 版的折叠块规则

- 同次版本号内的测试版（beta.1、beta.2…）与 patch 版之间**保留历史折叠块**。
- **发布正式版时**：将所有前置测试版条目按模块合并到正式版单一详细区块，去重后统一展示。测试版间被覆盖的条目只保留最终有效版本。正式版使用 `<details open>`，其后不再保留 beta 版本历史折叠块（除非有更早正式版）。

#### 正式版的详细内容来源：基于已有测试版 changelog 合并，而非全量重分析

**禁止**在发布正式版时从 git 历史全量重新分析提交、重新阅读 diff。正式版的详细内容应**直接合并已发布各测试版 changelog 的条目**，工作流如下：

1. 读取现有 CHANGELOG 中同次版本号的所有测试版区块（beta.1、beta.2…直至最新测试版）。
2. 将这些区块的条目按模块（新增/改进/修复/其他…）汇总，跨测试版去重：
   - 同一条目在多个测试版出现 → 只保留最终（最新）版本表述。
   - 被后续测试版修正/覆盖的条目 → 保留修正后的最终结果。
3. **唯一需要新增分析的增量**：仅为「最后一个测试版 tag → HEAD」之间的提交。这部分通常是少量改动，逐一检查后补充进正式版详细区块（有用户可感知效果才补，否则丢弃）。
4. 合并后统一排序、统一文案，不重复、不断裂。

**理由**：测试版发布时已完成对应提交范围的分析与改写，正式版只是对同一发布周期的汇总收尾，全量重分析会引入不一致、重复劳动，且容易把已被测试版 changelog 过滤/改写的噪音重新捞回。

### 6. Highlights 规则

- **中英双语，先中后英**。中文直接展示，英文放入 `<details><summary><b>English</b></summary>` 折叠块。
- Highlights 只总结最值得强调的变化，不要机械搬运所有条目。
- **复用规则（适用于 patch 版、测试版、以及由测试版晋升的正式版）**：
  - 判定标准：相对**直接前驱版本**（patch 的父正式版、测试版的上一测试版、正式版晋升时的最后一个测试版）有无用户可感知的重大变化。
  - 无重大变化 → **直接复用前驱版本 Highlights**，仅改版本号标题和日期，不改写内容。
  - 有重大新变化 → 保留原有内容，新段落追加在末尾。
- 正式版的补丁版本不应修改 Highlights，除非确实有用户可感知的重要变化。
- **由测试版晋升的正式版（X.Y.0）**：判定基准是最后一个测试版（如 beta.3）。若最后一个测试版到正式版之间只有内部维护、CI、通知文案等无用户可感知的变化，则 Highlights 一字不改地复用最后一个测试版的内容，只更新顶部版本号标题与日期。

### 7. 必须过滤的噪音

删除以下类型提交：
- bot 自动生成（`Auto Update Game Resources`、`Auto Templates Optimization` 等）
- `Release vX.Y.Z`、`Auto Update Changelogs`、`Auto Generate Changelog`
- `Update CHANGELOG`、`Bump version` 等 changelog 维护提交
- 带 `[skip changelog]` 标记的提交

**不过滤**：chore、perf 或看似内部优化的提交——只要有用户可感知效果（启动体验、性能、稳定性等），一律保留并放入合适模块。

### 8. git 历史编码处理（Windows PowerShell）

```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; git -c core.quotepath=false -c i18n.logoutputencoding=utf-8 -c i18n.commitencoding=utf-8 log --encoding=utf-8 --format="%H %s" RANGE | ForEach-Object { [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::Default.GetBytes($_)) }
```

如仍乱码，写入临时文件后用 read_file 读取：`| Out-File -Encoding utf8 -FilePath "$env:TEMP\commits.txt"`

## 完整文件结构

patch / 测试版结构（严格自上而下，不得打乱层次）：

1. `## vX.Y.Z (YYYY-MM-DD)` — 顶部版本标题
2. `### Highlights` — 中文直接展示
3. 英文 Highlights 折叠块
4. `----` 分隔线
5. `以下是详细内容：`
6. `<details open><summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>` — 当前版本（展开）
7. `<details><summary><b>vX.Y.Z-1 (YYYY-MM-DD)</b></summary>` — 上一版本（收起）
8. 更早版本各自独立折叠块…
9. `<details><summary><b>vX.Y.0 (YYYY-MM-DD)</b></summary>` — 最早正式版（收起）

**要点**：
- 折叠块内只保留详细内容，不重复 Highlights、不写 `## vX.Y.Z` 子标题。
- 历史区块不重复 Highlights 和"以下是详细内容："引导语——这些只在顶部出现一次。

## 工作流程

### 通用步骤（patch / 测试版）

1. 确定发布边界：目标版本、上一版本 tag、提交范围。
2. 读取现有 CHANGELOG 与范围内的 diff（不只看 commit 标题）。
3. 过滤 bot、release、`[skip changelog]`、revert 等噪音。
4. 按净变更合并同类提交，必要时从 diff 改写标题。
5. 按用户价值分类到正确模块。
6. 模块内排序、术语统一、中英文整理。
7. 编写中英双语 Highlights。
8. 输出完整 Markdown（顶部版本 + Highlights + 详细内容 + 历史折叠块）。

### 正式版（由测试版晋升）专属流程

**关键区别**：正式版**不做全量重分析**，而是「合并已有测试版 changelog + 补少量增量 + 处理 Highlights 复用」。步骤如下：

1. 读取现有 CHANGELOG 中同次版本号的所有测试版区块（beta.1 … beta.N）。
2. 判定「最后一个测试版 tag → HEAD」是否有用户可感知的重大变化：
   - 若有 → 分析这小段增量提交并整理成条目；若足以影响 Highlights 则追加（一般不追加）。
   - 若无（仅 CI、chore、内部维护等）→ 增量条目为空。
3. **Highlights**：直接复用最后一个测试版的 Highlights（仅改顶部版本号标题与日期），不重写。
4. **详细内容**：将各测试版区块条目按模块汇总去重，补入第 2 步的增量条目，按 §3 排序。
5. 输出：跨次版本号（X.Y.0）不保留历史折叠块；同次版本号 patch 正式版保留历史折叠块。

## 模块标题格式

```
### 新增 | New
### 改进 | Improved
### 修复 | Fix
### 文档 | Docs
### 其他 | Other
### MaaMacGui
```

仅保留有内容的模块，空模块省略。列表项统一 `*`。

## 输出模板

### 正式版

```markdown
## vX.Y.Z

### Highlights

#### 中文标题

中文正文。

<details>
<summary><b>English</b></summary>

#### English Title

English paragraph.

</details>

----

以下是详细内容：

<details open>
<summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>

### 新增 | New

* 条目 ([#12345](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12345)) @author

### 改进 | Improved

* 条目 @author

### MaaMacGui

#### 新增 | New

* 子仓库条目 ([#85](https://github.com/MaaAssistantArknights/MaaMacGui/pull/85)) @author

</details>
```

- **跨次版本号正式版**（X.Y.0，如 v6.13.0）：按规则 §4 不保留任何历史折叠块。
- **同次版本号 patch 正式版**（X.Y.Z, Z≥1，如 v6.13.1 相对 v6.13.0）：保留历史折叠块，与下方 patch / 测试版模板结构一致。

### patch / 测试版

```markdown
## vX.Y.Z

### Highlights

#### 中文标题

中文正文。

<details>
<summary><b>English</b></summary>

#### English Title

English paragraph.

</details>

----

以下是详细内容：

<details open>
<summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>

### 改进 | Improved

* 条目 ([#12345](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12345)) @author

### 修复 | Fix

* 条目 @author

</details>

<details>
<summary><b>vX.Y.Z-1 (YYYY-MM-DD)</b></summary>

### 改进 | Improved

* 历史版本条目 @author

</details>

<details>
<summary><b>vX.Y.0 (YYYY-MM-DD)</b></summary>

### 新增 | New

* 正式版条目 @author

</details>
```

## 常见错误

- ❌ 旧版本条目整段复制到当前 patch 版本
- ❌ Revert 原样保留为单独条目
- ❌ bot/release/auto generate/update changelog 提交写入文档
- ❌ 同一功能拆成多条重复表述
- ❌ 保留玩梗/口语化/半成品标题
- ❌ 机械沿用 commit type 导致分类错误
- ❌ patch/测试版无重大变化却重写独立 Highlights
- ❌ 详细内容插入到 Highlights 与历史区块之间，破坏文件结构
- ❌ 历史区块重复 Highlights 或"以下是详细内容："引导语
- ❌ 正式版按 beta 小版本分别折叠而非合并
- ❌ git 历史未指定编码导致中文乱码
- ❌ chore/perf 提交默认当噪音过滤（应判断是否有用户可感知效果）
- ❌ 跨次版本号时仍保留旧版本历史折叠块
- ❌ 正式版晋升时全量重新分析 git 提交，而非合并已有测试版 changelog
- ❌ 正式版晋升时重写 Highlights，而非复用最后一个测试版的 Highlights（无重大变化时）

## 最终检查

- [ ] 只保留最终有效净变更，非机械罗列 commit？
- [ ] 已删除 bot、Release、Generate、Update CHANGELOG、Revert 等噪音？
- [ ] 未把旧版本已发布内容重复抄入当前版本？
- [ ] 所有条目用户可独立理解？
- [ ] 分类正确、排序合理、中文在前英文在后？
- [ ] 输出完整 Markdown（非代码块）？
- [ ] patch/测试版无重大变化时复用了父版本 Highlights？
- [ ] 详细内容紧跟"以下是详细内容："之后？
- [ ] 历史区块无重复 Highlights 和引导语？
- [ ] 英文 Highlights 在 `<details>` 折叠块内？
- [ ] 每个版本各自独立 `<details>` 折叠块？
- [ ] 当前版本 `<details open>`，历史版本默认收起？
- [ ] 子仓库（MaaMacGui）作为独立子项放在 `### 其他 | Other` 之后？
- [ ] 正式版已合并所有测试版条目为单一区块？
- [ ] 正式版是合并已有测试版 changelog，而非全量重分析提交？
- [ ] 正式版 Highlights 无重大变化时复用了最后一个测试版的 Highlights？
- [ ] git 历史查询已指定编码参数？
- [ ] 跨次版本号时已移除旧版本历史折叠块？
