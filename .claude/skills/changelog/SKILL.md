---
name: changelog
description: 依据 git 提交、diff、现有 CHANGELOG 与 tag，生成符合 MAA 规范、可直接写入 CHANGELOG.md 的最终 Markdown。
---

# MAA Changelog Skill

读取待发布范围内的 commit、diff、现有 CHANGELOG 与 tag，输出可直接写入 `CHANGELOG.md` 的最终 Markdown。**只输出最终 Markdown**，不输出分析过程或代码围栏。

**必要输入**：目标版本号、提交范围（由 tag/PR/分支推导）、当前 CHANGELOG 内容。

> ⚠️ **版本号必须从用户/CI 提供的来源获取，禁止从 git tag 自行推测。** 若获取新版本号失败，必须通过 PR URL（如 `https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17248`）直接访问网页提取标题中的版本号，**不得 fallback 到 git tag 推测**。推测版本号会导致版本号全错（如 v6.14.0-beta.1 被错推为 v6.13.1-alpha.1），进而连锁触发折叠块处理错误。

## 1. 净变更优先

- 同一功能/问题的多条相关 commit **合并为单条**，描述最终效果。
- commit 标题含糊/口语化/玩梗时**必须查看 diff 后改写**为专业描述。
- **Revert**：完整撤销则删除；部分保留则合并为一条准确描述最终结果的条目。
- "review""typo""日志顺序""调整坐标""build warning" 等不单独保留，除非 diff 证明修复了用户可感知问题。
- **多服合并**：按**单项改动**拆分（而非 commit/PR 整体），每项再跨服合并。服务器名用 `/` 连接（如 `YostarEN/JP/KR`），多位作者依次排列。
  - 示例：某 PR 为 JP 更新主题+新增章节导航，EN/KR 只更新主题 → 拆为 `YostarEN/JP/KR 更新主题` + `YostarJP 新增章节导航`。

## 2. 分类

| 模块 | 适用场景 |
|------|----------|
| **新增 \| New** | 新功能、新支持、新入口、新兼容性 |
| **改进 \| Improved** | 能力增强、性能/稳定性/体验/识别优化、重构收益 |
| **修复 \| Fix** | 缺陷修正、兼容性/异常/回归修复 |
| **文档 \| Docs** | 纯文档变更 |
| **其他 \| Other** | 仅内部维护、CI、脚本等（不适合省略时） |
| **MaaMacGui** | 子仓库独立区块，放在 `### 其他 \| Other` 之后，内部复用相同分类，PR 格式 `([#数字](https://github.com/MaaAssistantArknights/MaaMacGui/pull/数字))` |

仅保留有内容的模块，空模块省略。

## 3. 排序与文案

- **中文在前，纯英文条目排最后**；按重要性排序：功能/接口变更 > 兼容性/优化 > 次要修复/杂项。
- 列表前缀统一 `*`；中英文与数字间留空格（如"修复 3 个 bug"）。
- 术语统一大小写：WPF、Json、Markdown、CSV、Info。
- 保留作者与 PR 引用，格式 `([#12345](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12345)) @author`；多条合并时引用合并括注。

## 4. 版本历史与折叠块

**版本类型**：不带 `-beta`/`-alpha` 后缀为正式版（含首个 `X.Y.0` 与 patch `X.Y.1+`）；带后缀为测试版。

- **跨次版本号**（正式版或新次版本的第一个测试版，如 v6.12.x → v6.13.0、v6.13.0 → v6.14.0-beta.1）：**删除**上一个次版本号的所有历史折叠块。
- **同次版本号内**（patch、beta 之间）：当前版本用 `<details open>`，历史版本各自 `<details>` 收起，紧跟其后。
- 当前版本只写增量变化，不得复制更早版本已发布条目。
- **发布正式版时**：将所有前置测试版条目按模块合并为正式版单一 `<details open>` 区块，跨测试版去重，只保留最终有效版本。其后不再保留 beta 折叠块（除非有更早正式版）。

### 正式版详细内容：合并已有测试版 changelog，禁止全量重分析

**禁止**在发布正式版时从 git 历史全量重新分析提交/阅读 diff。正式版详细内容**直接合并已发布各测试版 changelog 的条目**：

1. 读取同次版本号所有测试版区块（beta.1 … beta.N），按模块汇总去重——同一条目只保留最新表述，被覆盖的保留修正后结果。
2. **唯一增量**：「最后一个测试版 tag → HEAD」的小段提交，逐一检查后补充（有用户可感知效果才补）。
3. 合并后统一排序、统一文案。

## 5. Highlights

- **中英双语，先中后英**。中文直接展示，英文放入 `<details><summary><b>English</b></summary>` 折叠块。
- **累计性质**：同一次版本号（X.Y.0）下，顶层 `### Highlights` 是整个次版本号至今的**累计亮点**——应涵盖所有已发布测试版的亮点 + 当前增量，**而非仅当前测试版的增量亮点**。
- **精简原则**：只保留最值得强调的变化（通常 3-4 条），不要机械搬运所有条目。宁可少不可多——同时出现 4 条以上说明没有做好筛选，需要合并同类项或删减次要条目。
- **新次版本的第一个测试版**（如 v6.14.0-beta.1，跨次版本号）：前驱版本属于旧次版本号，所有旧历史折叠块已被删除，**Highlights 从零编写**（总结本提交范围内最值得强调的变化）。此时不存在可复用的前驱版本 Highlights。
- **同次版本号后续版本**（patch、beta.2+、由测试版晋升的正式版）：相对**直接前驱版本**（patch 的父正式版 / 测试版的上一测试版 / 正式版晋升时的最后一个测试版）：
  - 无用户可感知重大变化 → **直接复用前驱版本 Highlights**，仅改顶部版本号标题，不改写内容。
  - 有重大新变化 → **以前驱版本 Highlights 为基础，补充新亮点后重新审视、合并或替换**。具体规则：
    1. 先完整保留前驱版本的所有段落。
    2. 判断新增内容是否值得进入 Highlights；值得的在末尾追加新段落。
    3. 追加后若总数偏多（通常 >4 条），逐条审视：可合并同类项，也可**将前驱版本中相对凑数/次要的条目直接替换为更重要新亮点**（替换 ≠ 整体丢弃，而是逐条权衡）。
    4. **绝对禁止**整体丢弃前驱版本 curation——即不可仅保留当前版本增量亮点、删除全部前驱亮点。但逐条替换凑数条目是允许的。
- **由测试版晋升的正式版**：判定基准是最后一个测试版。若最后一个测试版到正式版之间只有 CI、chore、内部维护等，Highlights 一字不改复用，只更新顶部版本号标题。

## 6. 噪音过滤

**删除**：bot 自动生成（`Auto Update Game Resources`、`Auto Templates Optimization`）、`Release vX.Y.Z`、`Auto Update/Generate Changelog`、`Update CHANGELOG`、`Bump version`、带 `[skip changelog]` 标记的提交。

**不过滤**：chore、perf 或看似内部的提交——只要有用户可感知效果（启动体验、性能、稳定性等），一律保留。

## 7. 翻译判断

判断依据是「该条目的目标读者是谁」。

**用中文（默认）**：国服（CN）、繁中服（txwy）、跨服/全服通用改动、行为描述（`add support for X` → 「新增支持 X」）、含糊/玩梗的英文 commit 标题改写。

**整条保留英文**——仅针对 YostarEN/JP/KR 等外服且不涉及国服的改动。整条描述（含动词）均保留英文，**不要**只保留专有名词而把描述译成中文。原因：外服条目目标读者是玩该外服的用户，能看懂英文；外服关卡/活动/主题名（如 `lone trail`、`JieGarden`）在中服可能不存在或译名不同，翻译会误导。
- ✅ `YostarEN preload lone trail + fix JieGarden themes`
- ❌ `YostarEN/JP/KR 更新落叶逐火与界园主题`（整条译成中文，且「落叶逐火」是中服译名）

**混合**：同一 commit/PR 同时含国服与外服改动 → 按 §1 拆为多条，国服用中文，外服保留英文。

**代码/技术标识始终保留原文**（不分中外服）：任务名（`Stage`、`Roguelike`、`Depot`）、配置项、接口名、文件名，以及 WPF、Json、Markdown、CSV、onnx、ADB、minitouch 等（大小写见 §3）。

## 8. git 历史编码（Windows PowerShell）

```powershell
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8; git -c core.quotepath=false -c i18n.logoutputencoding=utf-8 -c i18n.commitencoding=utf-8 log --encoding=utf-8 --format="%H %s" RANGE | ForEach-Object { [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::Default.GetBytes($_)) }
```

如仍乱码：`| Out-File -Encoding utf8 -FilePath "$env:TEMP\commits.txt"` 后用 read_file 读取。

## 文件结构（自上而下）

1. `## vX.Y.Z` — 顶部版本标题（**不带日期**）
2. `### Highlights`（中文）→ 英文 Highlights `<details>` 折叠块
3. `----` 分隔线 → `以下是详细内容：`
4. `<details open><summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>` — 当前版本（展开）
5. `<details><summary><b>vX.Y.Z-1 (YYYY-MM-DD)</b></summary>` — 历史版本（收起）
6. 更早版本各自独立折叠块…

折叠块内只保留详细内容，不重复 Highlights、不写 `## vX.Y.Z` 子标题。历史区块不重复 Highlights 和引导语。

## 输出模板

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

### MaaMacGui

#### 新增 | New

* 子仓库条目 ([#85](https://github.com/MaaAssistantArknights/MaaMacGui/pull/85)) @author

</details>

<details>
<summary><b>vX.Y.Z-1 (YYYY-MM-DD)</b></summary>

### 改进 | Improved

* 历史版本条目 @author

</details>
```

- **正式版**（跨次版本号 X.Y.0）：不保留任何历史折叠块，仅 `<details open>` 单一区块。
- **patch 正式版 / 测试版**：保留历史折叠块，如上所示。

## 工作流程

### patch / 测试版

确定发布边界 → 读 CHANGELOG 与 diff → 过滤噪音 → 合并同类提交（必要时从 diff 改写）→ 分类 → 排序/术语/中英文整理 → 处理 Highlights：**新次版本第一个测试版从零编写；后续测试版以前驱版本为基础，无重大变化直接复用、有重大变化补充新亮点后合并精简（禁止丢弃已有 curation 仅保留当前增量）** → 输出完整 Markdown。

### 正式版（由测试版晋升）

不做全量重分析，而是「合并已有测试版 changelog + 补少量增量 + 复用 Highlights」：
1. 读取同次版本号所有测试版区块。
2. 判定「最后一个测试版 tag → HEAD」是否有用户可感知变化（一般仅 CI/chore → 增量为空）。
3. **Highlights**：复用最后一个测试版的（仅改版本号标题）。
4. **详细内容**：各测试版条目按模块汇总去重，补入增量，排序。
5. 跨次版本号不保留历史折叠块；patch 正式版保留。

## 常见错误

- ❌ 旧版本条目整段复制到当前版本 / Revert 原样保留
- ❌ bot/release/auto generate/update changelog 写入文档
- ❌ 同一功能拆成多条重复 / 保留玩梗或半成品标题 / 机械沿用 commit type
- ❌ 无重大变化却重写 Highlights / 有重大变化时整体丢弃前驱版本 curation 仅保留当前增量 / 正式版全量重分析而非合并测试版
- ❌ 详细内容插入到 Highlights 与历史区块之间，破坏结构 / 历史区块重复 Highlights 或引导语
- ❌ chore/perf 默认当噪音过滤（应判断用户可感知效果）
- ❌ 从 git tag 自行推测版本号（应从 PR 标题/用户输入获取；`gh` 失败时用 URL 访问 PR 页面）
- ❌ 外服专有条目整条译成中文 / git 历史未指定编码导致乱码

## 最终检查

- [ ] 净变更合并，非机械罗列 commit？噪音已删除？
- [ ] 未重复旧版本内容？条目用户可独立理解？分类/排序/文案正确？
- [ ] 顶部标题 `## vX.Y.Z` 不带日期，折叠块 summary 带日期？
- [ ] Highlights 处理正确（新次版本第一个测试版从零编写；后续测试版以前驱为基础补充新亮点后合并精简，未丢弃已有 curation；总条数通常 3-4 条）？英文 Highlights 在 `<details>` 内？
- [ ] 当前版本 `<details open>`，历史版本收起？历史区块无重复 Highlights/引导语？
- [ ] 正式版合并所有测试版为单一区块，未全量重分析？
- [ ] 跨次版本号已移除旧版本折叠块（含新次版本第一个测试版）？
- [ ] 子仓库 MaaMacGui 放在 `### 其他 | Other` 之后？
- [ ] 版本号从 PR 标题/用户输入获取，未从 git tag 推测？
- [ ] 外服专有条目保留英文原文？git 历史已指定编码？
