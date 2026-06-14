---
name: changelog
description: 根据提交记录、PR、diff、现有 CHANGELOG 与历史 tag 内容，整理符合 MAA 发布规范的 changelog Markdown。用于修正工具自动生成的 changelog、合并同类改动、清理 bot 或 release 噪音，并产出可直接提交的最终版本。
---

# MAA Changelog Skill

## Goal

- 读取待发布范围内的 commit、PR、diff、现有 CHANGELOG 与对应 tag 内容，输出可直接写入 CHANGELOG.md 的最终 Markdown 片段。
- 只输出最终 Markdown，不输出分析过程、分类理由、筛选记录、额外说明或 Markdown 代码围栏。
- 以“最终用户看得懂、历史版本不断裂、同类改动不重复”为第一目标，不以保留原始 commit 标题为目标。

## Scope

- 适用于正式版、测试版、补丁版的 changelog 整理与重写。
- 正式版的补丁版本不应该修改 Highlights 中的内容，除非确实有用户可感知的重要变化。
- 当工具已经生成初稿时，初稿只可作为原始素材，不能直接信任其分类、版本归属、标题质量、去重结果与排序结果。
- 如果 commit 标题含糊、口语化、玩梗、只写 review、typo、warning、日志顺序、调整坐标等，必须查看 diff 后改写为专业、完整、可独立理解的用户向描述。

## Required Inputs

- 目标版本号。
- 待发布的提交范围，或可推导该范围的 PR、tag、分支信息。
- 当前 CHANGELOG 内容。
- 如果目标版本与已有正式版属于同一非 patch 版号，必须先读取对应正式版 tag 下的 changelog 内容，再决定如何合并与追加。

## Non-Negotiable Rules

### 1. 先看净变更，再写条目

- 对同一功能、同一问题或逻辑相关的多条 commit，应合并为单条 changelog 项。
- 合并后的描述必须简洁、专业、面向最终用户，避免堆实现细节。
- 若 commit 标题不足以表达改动价值，必须结合 diff 重写标题。
- Revert 不是 changelog 项。遇到 Revert 时，必须结合最终 diff 判断净效果：
- 若原改动被完整撤销，则 Revert 与原始项都删除。
- 若最终仍保留部分语义，则把原始项与 Revert 合并为一条准确描述最终结果的 changelog 项。
- 不要把“review”“日志顺序”“调整坐标”“typo”“build warning”这类缺乏用户语义的提交原样保留为条目，除非 diff 证明它确实修复了用户可感知问题。

### 2. 分类按用户价值，不按 commit 前缀

- 改动必须放入正确模块：Highlights、新增 | New、改进 | Improved、修复 | Fix、文档 | Docs、其他 | Other。
- 新功能、新支持、新入口、新导出能力、新兼容性，放“新增 | New”。
- 现有能力增强、性能提升、稳定性提升、体验优化、识别优化、重构后带来的用户收益，放“改进 | Improved”。
- 缺陷修正、兼容性修复、异常处理、回归修复，放“修复 | Fix”。
- 纯文档变更放“文档 | Docs”。
- 仅内部维护、CI、脚本、杂项且不适合省略时，才放“其他 | Other”。
- 如果自动生成结果分类错误，必须移动到更合适的模块并同步调整描述。
- 子仓库（如 MaaMacGui）的更新应作为独立的 `### MaaMacGui` 子项，放在主 changelog 的 `### 其他 | Other` 之后。该子项内部使用与主 changelog 相同的分类结构（新增 | New、改进 | Improved、修复 | Fix 等），PR 引用格式为 `([#数字](https://github.com/MaaAssistantArknights/MaaMacGui/pull/数字))`。

### 3. 模块内排序与文案规范

- 中文条目放在前面，纯英文条目排在该模块最后。
- 同一模块内按逻辑相关性或重要性排序：功能与接口变更优先，其次是兼容性或实现优化，最后是次要修复或杂项。
- 列表统一使用 * 作为项目前缀。
- 中英文数字混排时，在英文词与数字之间保留空格，例如：修复 3 个 bug、支持 3D 功能。
- 统一常见术语大小写与写法，例如 WPF、Json、Markdown、CSV、Info。
- 保留作者、PR、commit reference，例如 (#12345) @author；若多条相关提交被合并，可把相近引用合并到同一项后括注。

### 4. 正式版与 patch 版的历史连续性

- 正式版严禁只留下单独的 patch 版本内容。
- 如果目标正式版与已有正式版属于同一非 patch 版号，必须对比对应 tag 下的 changelog，并把当前新增改动追加到原有内容之后，保持版本历史连续。
- patch 版详细区块只能写“自上一发布版本之后新增的变化”，不能把更早正式版已经出现过的条目整段复制到当前 patch 版本下面。
- 例如生成 v6.10.4 时，v6.10.4 区块只能写 v6.10.3 之后的新变化；v6.10.0、v6.10.1、v6.10.2、v6.10.3 的既有内容应保留在历史区块，而不是重新抄进 v6.10.4。
- 若非 patch 版本不同，则直接根据现有内容组织该版本及其历史区块。

### 5. 正式版合并测试版内容

- 正式版发布时，应把所有前置测试版（beta.1、beta.2 等）的条目按模块（新增、改进、修复等）合并到正式版的单一详细区块中，去重后统一展示。
- 不应按测试版小版本分别折叠。正式版用户不关心测试版之间的增量差异，只关心"这个正式版相比上一个正式版有什么变化"。
- 测试版之间重复或被后续修改覆盖的条目只保留最终有效版本。例如 beta.1 修复了某个问题但 beta.2 又对其做了改进，正式版中只保留合并后的最终描述。
- 正式版的详细区块使用 `<details open>` 默认展开，其后不再保留 beta 版本的历史折叠块（除非之前还有更早的正式版）。

### 6. patch / 测试版的 Highlights 复用规则

- patch 版本（例如 v6.10.4 相对于 v6.10.3）和测试版（例如 v6.11.0-beta.2 相对于 v6.11.0-beta.1）如果没有用户可感知的重要新功能或重大变化，必须直接复用其父版本的 Highlights 内容，不得自行重写或另起一套。
- 复用 Highlights 时，只改顶部版本号标题和发版日期（例如 `## v6.11.0-beta.1 (2026-05-27)` → `## v6.11.0-beta.2 (2026-05-29)`），Highlights 正文原样保留。
- 当 patch 版本或测试版确实包含用户可感知的重要新变化时（例如新增了重大功能、改变了核心交互），可以为 Highlights 追加新段落，但必须保留原有 Highlights 内容，新段落追加在末尾。

### 7. patch / 测试版编辑的完整结构

- 输出文件的结构必须严格遵循以下层次，不得把 patch 版本或测试版的详细内容插入到父版本的 Highlights 与详细内容之间：
  1. 顶部：`## vX.Y.Z (YYYY-MM-DD)`（patch / 测试版标题，含发版日期）
  2. `### Highlights`（复用父版本内容，或在有必要时追加新段落）
  3. 英文 Highlights 折叠块
  4. `----`
  5. `以下是详细内容：`
  6. `<details open><summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>`（当前版本详细内容，默认展开）
  7. `<details><summary><b>vX.Y.Z-1 (YYYY-MM-DD)</b></summary>`（上一 patch 版本，默认收起）
  8. 更早版本各自独立折叠块...
  9. `<details><summary><b>vX.Y.0 (YYYY-MM-DD)</b></summary>`（正式版，默认收起）
- 每个版本的详细内容各自放入独立的 `<details>` 折叠块，`<summary>` 内格式为 `<b>vX.Y.Z (YYYY-MM-DD)</b>`（版本号 + 发版日期），当前目标版本使用 `<details open>` 默认展开，其余默认收起。
- 折叠块内只保留详细内容（改进、修复等），不重复 Highlights，不写 `## vX.Y.Z` 子标题（`<summary>` 已提供版本标识）。

### 8. Highlights 必须中英双语且先中后英

- 输出顶部必须包含当前目标版本和发版日期，例如 `## vX.Y.Z (2026-05-29)`。
- 必须包含 ### Highlights。
- 中文 Highlights 直接展示，不折叠。
- 英文 Highlights 放入折叠块：`<details><summary><b>English</b></summary>` ... `</details>`。
- 中文与英文都应按主题分段，标题简洁明确，正文面向最终用户，不是 commit 列表翻译。
- Highlights 只总结本次版本中最值得强调的变化，不要把所有条目机械搬进去。

### 9. 必须过滤的噪音项

- 删除或忽略纯 bot 自动生成的 changelog、update、release 条目（如 github-actions[bot] 的 Auto Update Game Resources）。
- 删除显式的 Release 发布记录，例如 Release vX.Y.Z。
- 删除或忽略 Generate、Auto Update、Auto Generate、Update CHANGELOG、Bump version 之类自动维护条目。
- 删除带有 `[skip changelog]` 标记的提交。
- 删除 commit 消息仅为 "Update CHANGELOG" "docs: Update CHANGELOG for vX.Y.Z release" 之类纯 changelog 内容维护的提交。
- 除此之外的提交都不应过滤，即使标题是 chore、perf 或看起来像内部优化——只要有用户可感知的效果（包括启动体验、操作窗口、性能、稳定性等），都应保留并放入合适的模块。

### 10. 查询 git 历史时的编码处理

- 在 Windows PowerShell 环境下，git log 输出的中文默认会乱码。查询 git 历史时必须指定编码参数：
  ```
  [Console]::OutputEncoding = [System.Text.Encoding]::UTF8; git -c core.quotepath=false -c i18n.logoutputencoding=utf-8 -c i18n.commitencoding=utf-8 log --encoding=utf-8 --format="%H %s" RANGE | ForEach-Object { [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::Default.GetBytes($_)) }
  ```
- 简化写法（仅在当前终端已执行过 `[Console]::OutputEncoding = [System.Text.Encoding]::UTF8` 后有效）：`git -c core.quotepath=false log --encoding=utf-8 --format="..." | ForEach-Object { [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::Default.GetBytes($_)) }`
- 如果输出仍然乱码，可将结果写入临时文件（`| Out-File -Encoding utf8 -FilePath "$env:TEMP\commits.txt"`）再用 read_file 工具读取。

## Workflow

1. 先确定本次发布边界：目标版本、上一版本、对应 tag、待发布 commit 范围。
2. 读取现有 CHANGELOG 与目标范围内的 diff，不要只根据 commit 标题下结论。
3. 先过滤 bot、release、generate、update changelog、revert、重复历史条目等噪音。
4. 按“净变更”合并同类提交，必要时从 diff 改写标题。
5. 按用户价值重新分类到正确模块，而不是沿用自动生成结果。
6. 在每个模块内完成排序、术语统一与中英文条目整理。
7. 编写中英双语 Highlights，先中文，后英文，中间用 ---- 分隔。
8. 输出完整 Markdown 片段，包含顶部版本、Highlights、以下是详细内容：、当前版本区块与历史版本区块。

## Common Failure Patterns To Correct

- 把旧版本已有条目整段复制到当前 patch 版本。
- 把 Revert 原样保留成单独 changelog 项。
- 把 Release vX.Y.Z、Auto Update Changelogs、Update CHANGELOG、Bump version 之类自动提交写进文档或其他模块。
- 把同一功能拆成多条重复表述，例如同一个生息演算功能拆成多个相近新增或改进条目。
- 保留玩梗、口语化、半成品标题，例如不会现在还有人选沙中遗火吧、特意删的 PNS 怎么又给加回来了。
- 机械沿用 commit type 导致分类错误，例如把用户能感知的修复放进其他，把兼容性提升放进新增。
- patch 版本或测试版没有用户可感知的重要新变化，却自行重写了独立的 Highlights，而非复用父版本内容。
- 把 patch 版本的详细内容插入到父版本的 Highlights 与详细内容之间，破坏了文件结构。
- patch / 测试版的历史区块中重复保留了 Highlights 和"以下是详细内容："引导语，这些应只在顶部出现一次。
- 正式版不应保留各 beta 版本的独立折叠块，应将所有测试版条目按模块合并到正式版的单一详细区块中。
- 查询 git 历史时未指定编码，导致中文 commit 消息乱码，无法正确理解变更内容。
- 把 chore/perf 标题的提交默认当作噪音过滤，而不是判断其是否有用户可感知的效果。

## Output Requirements

- 输出完整 Markdown 文件片段。
- 顶部必须包含当前版本标题，例如 ## vX.Y.Z。
- 顶部必须包含 ### Highlights，并满足先中文、后英文、用 ---- 分隔的格式。
- 英文 Highlights 折叠块结束后，接“----”分隔线，然后接“以下是详细内容：”引导语。
- 每个版本的详细内容各自放入独立的折叠块：`<details><summary><b>vX.Y.Z</b></summary>` ... `</details>`。
- 当前目标版本的折叠块使用 `<details open>` 默认展开，历史版本使用 `<details>` 默认收起。
- 详细内容中的模块标题统一使用以下格式：
- ### 新增 | New
- ### 改进 | Improved
- ### 修复 | Fix
- ### 文档 | Docs
- ### 其他 | Other
- ### MaaMacGui（子仓库独立区块，放在 `### 其他 | Other` 之后，内部再按相同分类结构组织）
- 列表项统一使用 *。
- 仅保留有内容的模块；空模块省略。

## Output Template

### 正式版模板（合并测试版内容）

```
## vX.Y.Z

### Highlights

#### 中文小结标题 A

中文小结正文。

#### 中文小结标题 B

中文小结正文。

<details>
<summary><b>English</b></summary>

#### English Summary Title A

English summary paragraph.

#### English Summary Title B

English summary paragraph.

</details>

----

以下是详细内容：

<details open>
<summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>

### 新增 | New

* 条目 A @author

### 改进 | Improved

* 条目 B (#12345) @author

### 修复 | Fix

* 条目 C @author

### 文档 | Docs

* 条目 D @author

### MaaMacGui

#### 新增 | New

* 子仓库新增条目 ([#85](https://github.com/MaaAssistantArknights/MaaMacGui/pull/85)) @author

#### 修复 | Fix

* 子仓库修复条目 ([#88](https://github.com/MaaAssistantArknights/MaaMacGui/pull/88)) @author

</details>

<details>
<summary><b>vX.Y-1.Z (YYYY-MM-DD)</b></summary>

### 修复 | Fix

* 上一个正式版的条目 @author

</details>
```

### patch / 测试版模板

### Highlights

#### 中文小结标题 A

中文小结正文。

#### 中文小结标题 B

中文小结正文。

<details>
<summary><b>English</b></summary>

#### English Summary Title A

English summary paragraph.

#### English Summary Title B

English summary paragraph.

</details>

----

以下是详细内容：

<details open>
<summary><b>vX.Y.Z (YYYY-MM-DD)</b></summary>

### 改进 | Improved

* 条目 A (#12345) @author

### 修复 | Fix

* 条目 B @author

### 文档 | Docs

* 条目 C @author

### MaaMacGui

#### 新增 | New

* 子仓库新增条目 ([#85](https://github.com/MaaAssistantArknights/MaaMacGui/pull/85)) @author

#### 修复 | Fix

* 子仓库修复条目 ([#88](https://github.com/MaaAssistantArknights/MaaMacGui/pull/88)) @author

</details>

<details>
<summary><b>vX.Y.1 (YYYY-MM-DD)</b></summary>

### 改进 | Improved

* 历史版本条目 @author

### 修复 | Fix

* 历史版本条目 @author

</details>

<details>
<summary><b>vX.Y.0 (YYYY-MM-DD)</b></summary>

### 新增 | New

* 正式版条目 @author

### 改进 | Improved

* 正式版条目 @author

### 修复 | Fix

* 正式版条目 @author

</details>
```

## Final Checklist

- 是否只保留最终有效的净变更，而不是机械罗列 commit？
- 是否已经删除 bot、Release、Generate、Update CHANGELOG、Revert 等噪音项？
- 是否避免把旧版本已发布内容重复抄进当前 patch 版本？
- 是否所有条目都能被最终用户独立理解？
- 是否已经按模块正确分类、排序，并保持中文在前、英文在后？
- 是否已经输出完整 Markdown，而不是说明文字或代码块？
- 如果是 patch 版本且没有用户可感知的重要新变化，是否复用了父版本的 Highlights 而非自行重写？
- patch / 测试版的详细内容是否紧跟在"以下是详细内容："之后，而非插入到父版本的 Highlights 下方？
- 历史版本区块中是否只保留详细内容，没有重复 Highlights 和引导语？
- 英文 Highlights 是否放入 `<details>` 折叠块（中文不折叠）？
- 每个版本的详细内容是否各自放入独立的 `<details>` 折叠块？
- 当前版本是否使用 `<details open>` 默认展开，历史版本是否默认收起？
- 如果有子仓库（如 MaaMacGui）更新，是否作为 `### MaaMacGui` 独立子项放在 `### 其他 | Other` 之后，且内部使用与主 changelog 相同的分类结构？
- 正式版是否已将所有测试版条目合并到单一详细区块中，而非按 beta 小版本分别折叠？
- 查询 git 历史时是否已正确指定编码参数，避免中文 commit 消息乱码？