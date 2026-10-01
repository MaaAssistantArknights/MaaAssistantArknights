---
name: changelog
description: 依据 git 提交、diff、现有 CHANGELOG 与 tag，生成符合 MAA 规范、可直接写入 CHANGELOG.md 的最终 Markdown。用户提到发版、整理/生成 changelog、写更新日志、发布正式版或 beta 时均应使用本 skill，即使用户没有明确说出 "changelog" 一词。
---

# MAA Changelog Skill

读取待发布范围内的 commit、diff、现有 CHANGELOG 与 tag，输出可直接写入 `CHANGELOG.md` 的最终 Markdown。**只输出最终 Markdown**，不输出分析过程或代码围栏。

**必要输入**：目标版本号、提交范围（由 tag/PR/分支推导）、当前 CHANGELOG 内容。

> ⚠️ **版本号必须从用户/CI 提供的来源获取，禁止从 git tag 自行推测。** 若获取新版本号失败，必须通过 PR URL（如 `https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17248`）直接访问网页提取标题中的版本号，**不得 fallback 到 git tag 推测**。推测版本号会导致版本号全错（如 v6.14.0-beta.1 被错推为 v6.13.1-alpha.1），进而连锁触发折叠块处理错误。

## 1. 净变更优先

- 同一功能/问题的多条相关 commit **合并为单条**，描述最终效果。
- **首次对用户可见的功能只留功能本体条目**：功能在本版本第一次开放/出现时，其开发过程中的全部中间条目（内部修复、打磨、布局调整、参数默认值变化等）一律并入功能本体，只描述最终形态。用户没见过旧版本，｢修复 XX 功能的问题｣ 对他们无从谈起；逐条罗列只会稀释本体条目。注意区分：对**已发布功能**的修复仍照常写修复条目。
- commit 标题含糊/口语化时**必须查看 diff 后改写**为专业描述。作者刻意整活/玩梗的标题保留原文，仅在含糊到读者无法理解实际内容时括注补充（如 `我是小猪（修正识别替换规则中半角括号的正则转义）`）；若玩梗提交与其余净变更同链、合并会洗掉原文，则该提交单列保留。
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
| **其他 \| Other** | 内部维护、CI、脚本、工具链约定、开发规范、工程化开关等开发者向内容 |
| **MaaMacGui** | 子仓库独立区块，放在 `### 其他 \| Other` 之后，内部复用相同分类，PR 格式 `([#数字](https://github.com/MaaAssistantArknights/MaaMacGui/pull/数字))` |

仅保留有内容的模块，空模块省略。

**MaaMacGui 区块的判定**：本地子模块目录 `src/MaaMacGui` 通常未 clone（空目录，`git -C src/MaaMacGui log` 不可用），且主仓库的 submodule 指针更新提交都带 `[skip changelog]`，看不出实际内容。判断版本范围内 MaaMacGui 是否有合并 PR，用 `gh pr list --repo MaaAssistantArknights/MaaMacGui --state merged` 按 `mergedAt` 对照版本 tag 日期，不要依赖本地子模块历史。

## 3. 排序与文案

- **中文在前，纯英文条目排最后**；按重要性排序：功能/接口变更 > 兼容性/优化 > 次要修复/杂项。
- **PC 端条目统一放所在分类的中文区末尾、其他语言条目之前**（带 ｢PC 端｣ 前缀的直连/Win32 专属功能）：主流用户使用模拟器连接，PC 直连功能对他们不可用，靠后集中便于快速跳过；中文在前、其他语言在最后的规则优先级更高。
- 列表前缀统一 `*`；中英文与数字间留空格（如"修复 3 个 bug"）。
- 术语统一大小写：WPF、Json、Markdown、CSV、Info。
- 保留作者与 PR 引用，格式 `([#12345](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/12345)) @author`；多条合并时引用合并括注。
- **作者归属**：从 commit 的 `%an`（author）字段获取，**不要**用 `%cn`（committer）——squash merge 的 committer 通常是 GitHub 或执行合并的人，而非贡献者。对于多贡献者 PR（squash 后协作者信息丢失），需访问 PR 页面确认发起人与协作者，全部列出（用空格分隔，如 `@author1 @author2`）。
- **显示名 → GitHub login 映射**：条目里的 `@作者` 必须是 GitHub login，而 `%an` 给出的常是显示名。已确认的映射：墨染 → @moranfanhua、Ziyi Huang → @yali-hzy、uye → @ABA2396、status102 / Status102 → @status102、Rin → @ZiyinLin、HY → @momomochi987、H2O_MERO → @H2O-MERO、Zian Wen → @wzacolemak；Constrat、youzibigg 与 login 同名。表外名字用 `gh pr view <N> --json author` 核实；`gh` 间歇性 TLS handshake timeout 时，用 commit 的 `%ae` 邮箱前缀、`Co-authored-by` trailer、以及历史 CHANGELOG 中同一 PR 的既有归属相互印证。
- **条件生效的功能把限定写进条目**：功能若依赖输入方式、截图方式、连接类型、服务器等条件才生效（如仅在 PC 端 win32 连接下生效、仅特定截图方式下行为不同），条目必须写明限定，不能只写笼统效果；拿不准时查 squash commit 信息与最终代码里的实际分支逻辑再落笔。
- **功能与机制名称用 UI 正式名称，不用开发者别称**：条目里出现的功能名、界面项、游戏机制名，以本地化资源 key 的实际值或游戏内 UI 术语为准（如任务 `OperProgress` 的 UI 名是 ｢干员培养｣ 就不写 ｢培养计划｣；训练室协助专精的干员游戏内叫 ｢协助者｣，就不沿用代码注释/PR 描述里的 ｢导师｣｢助教｣）。拿不准时查 `src/MaaWpfGui/Res/Localizations/zh-cn.xaml` 或问作者，不从 Sourcery 总结、代码注释脑补。
- **Sourcery 总结仅作线索，不作事实依据**：PR body 里的 ｢## Sourcery 摘要 / Summary by Sourcery｣ 是低思考等级的 AI 生成内容，常见术语俗称、界面归属错乱、行为概括失真。它只用于快速定位 PR 改动范围；条目事实必须落到 diff、作者手写的 PR 正文、issue 标题或作者口述上，Sourcery 与 diff 冲突时一律以 diff 为准。

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
- **测试/实验性功能条目注明性质并引导反馈入口**：标注 ｢测试功能｣ 并可能存在未发现问题时，附上反馈路径（中文 ｢设置 - 问题反馈｣、英文 `Settings - Issue Report`，以界面实际文案为准），避免用户遇到问题后到处询问。

## 6. 噪音过滤与内部内容收录

**删除**：bot 自动生成（`Auto Update Game Resources`、`Auto Templates Optimization` 及其附带的产物重生成）、`Release vX.Y.Z`、`Auto Update/Generate Changelog`、`Update CHANGELOG`、`Bump version`、带 `[skip changelog]` 标记的提交。

**收录进 ｢其他 | Other｣**：其余内部维护改动——工具链约定、工程化开关（如 TreatWarningsAsErrors）、开发规范更新、CI 依赖升级、构建/资源同步脚本机制、格式统一、版权年份等。两个受众分层：**Highlights 是面向用户的精选层**，从用户价值出发筛选；**详细内容的读者也包括开发者与集成者**，内部维护改动对他们有参考价值，按 Other 收录而不是按 ｢用户可感知性｣ 扔掉。仅真正的噪音（上述删除项）不入文档。

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

## 9. Squash PR 作者归属

仓库默认使用 squash merge，squash 后的 commit 会丢失原始分支上的多提交者信息：

- **`%an`（author）= PR 发起人**，是默认归属依据。**不要**用 `%cn`（committer），squash 的 committer 通常是 GitHub 或执行合并维护者。
- **逐 PR 核对协作者，不得默认单作者**：`%an` 只反映发起人，分支上有他人 commit 的 PR 标发起人会漏标。主判据是本地 squash commit 的 `Co-authored-by` trailer（GitHub squash 自动保留、不依赖网络），对范围内全部 PR 批量执行：

  ```bash
  git log -1 --format=%b <dev-v2上的squash-commit> | grep -i "Co-authored-by"
  ```

  trailer 非空 → 按 trailer 列出协作者；trailer 为空且存疑时，用 `gh api repos/.../pulls/<N>/commits` 看分支 commit 作者列表佐证；仍无法确认才访问 PR 页面核对 contributor 列表。本地 trailer 判据优先于 gh 网络请求——`gh` 有间歇性 TLS 超时，批量核对时本地先行能省掉大量重试。
- **作者顺序**：发起人在前，协作者按 trailer 顺序随后，如 `@author1 @author2 @author3`。单作者 `@author`。显示名 → GitHub login 的已确认映射与 `gh` 超时回退办法见 §3 作者归属。

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

### 测试版发布后的增量补充（常见循环）

beta 的 CHANGELOG 已写入并推送、但版本尚未发布期间，dev-v2 会持续有新提交合并。用户常说 ｢补充一下之后新提交的 changelog｣ ｢再拉一下新提交｣，此时**不做全量重分析**，按增量循环：

1. **确定增量范围**：以 CHANGELOG 已覆盖的最后一个提交为界（即上次写入/推送 changelog 时的 dev-v2 顶点），`git fetch` 后分析 `上界..origin/dev-v2`。
2. 新提交照常走 §6 过滤/收录、§1 合并、§9 作者核对（trailer 逐 PR 查）。
3. 条目叠加进当前版本的对应分类区块，统一排序；新增亮点按 §5 累计规则处理，是否进 Highlights 交用户裁定。
4. 用户过程中的逐条裁定（如某功能入口未开放暂不写、某条目定性）在后续增量轮次中继续沿用，不得悄悄回退；功能状态发生变化（如入口开放）时按新裁定重新定性，并按 §1 ｢首次可见功能｣ 规则合并。
5. 推送若被拒（远端又进了新提交，多为 bot），`git pull --rebase` 后重推。

## 常见错误

- ❌ 旧版本条目整段复制到当前版本 / Revert 原样保留
- ❌ bot/release/auto generate/update changelog 写入文档
- ❌ 同一功能拆成多条重复 / 保留玩梗或半成品标题 / 机械沿用 commit type
- ❌ 无重大变化却重写 Highlights / 有重大变化时整体丢弃前驱版本 curation 仅保留当前增量 / 正式版全量重分析而非合并测试版
- ❌ 详细内容插入到 Highlights 与历史区块之间，破坏结构 / 历史区块重复 Highlights 或引导语
- ❌ chore/perf 默认当噪音过滤（应判断用户可感知效果）
- ❌ 从 git tag 自行推测版本号（应从 PR 标题/用户输入获取；`gh` 失败时用 URL 访问 PR 页面）
- ❌ 外服专有条目整条译成中文 / git 历史未指定编码导致乱码
- ❌ squash PR 作者取 `%cn`（committer/合并者）而非 `%an`（author/发起人）；多贡献者 PR 未核对 contributor（默认单作者直接落笔——应逐 PR 查 trailer）
- ❌ 条件生效的功能写成无条件生效（应查代码把输入/截图/连接方式等限定写进条目）
- ❌ 首次可见功能的中间打磨/修复条目单列罗列（应并入功能本体条目）
- ❌ 内部维护提交按 ｢用户不可感知｣ 一律过滤（应收录进 Other——详细内容面向开发者；Highlights 才是用户精选层）
- ❌ 条目沿用开发者俗称而非游戏/UI 正式术语（如 ｢导师｣ 应为 ｢协助者｣、｢培养计划｣ 应为 ｢干员培养｣）
- ❌ 把 Sourcery 摘要直接当条目事实来源（低思考等级 AI 输出，仅作线索，事实须落到 diff/作者正文/issue）
- ❌ 测试功能条目不注明测试性质与反馈入口（应写 ｢设置 - 问题反馈｣）

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
- [ ] squash PR 作者取 `%an`（发起人），非 `%cn`（合并者）？多贡献者 PR 已核对 contributor 列表？`@作者` 均为 GitHub login 而非显示名？
- [ ] 首次可见功能已合并为功能本体条目，无中间打磨条目残留？
- [ ] 内部维护改动已收录 Other（bot/Release/[skip changelog] 除外）？
- [ ] 功能与机制名称为 UI 正式名称（本地化 key 值/游戏内术语），无开发者别称？
- [ ] 测试功能条目注明性质并附 ｢设置 - 问题反馈｣ 引导？
- [ ] 外服专有条目保留英文原文？git 历史已指定编码？
