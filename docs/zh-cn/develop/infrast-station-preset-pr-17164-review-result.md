# infrast station_preset PR #17164 审阅结论

> **审阅对象**
>
> | 范围 | 引用 |
> |------|------|
> | 主仓库 PR | [#17164](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17164) |
> | 本地 HEAD（squash，**未 push**） | `bcb128a202` — `feat(infrast): 设施预设换班与进驻总览队列轮换` |
> | diff | `git diff upstream/dev-v2...bcb128a202` |
> | 审阅 brief | [infrast-station-preset-pr-review.md](./infrast-station-preset-pr-review.md) |
>
> **Companion（子模块指针已写入主 PR diff）**
>
> | 仓库 | commit | 上游存在 | 说明 |
> |------|--------|----------|------|
> | MaaMacGui | `711daf6` | ❌ | `feat(infrast): Rotation 进驻总览预设 UI 与内联 preset 参数` |
> | MAAUnified | `b55a798` | ✅ | 仅 `auto_advance_plan_index`，**无** station_preset UI |
>
> 审阅日期：2026-06-29 · 第 1 轮

---

## 1. 总体结论

**需修改后再审（`changes-needed`）** — Core + 协议 + MacGui 设计方向正确，Mac 实测有说服力；但 **主 PR 不能携带未进上游的 `MaaMacGui@711daf6` 指针**，且远程 PR 仍冲突/未同步 squash，PR 描述待更新。

---

## 2. Blocking

### B1 — 子模块 `MaaMacGui` 指向未合并 commit

| 项 | 内容 |
|----|------|
| **位置** | `src/MaaMacGui` → `711daf6482` |
| **问题** | `gh api …/MaaMacGui/commits/711daf6` 返回 **404**；commit 仅存在于本地/fork，**不在** `MaaAssistantArknights/MaaMacGui` 默认分支。主仓合并后 CI/他人 `git submodule update` 将无法解析该指针。 |
| **改法** | **二选一**：(a) 主 PR **不 bump** `MaaMacGui`，先开并合并 companion PR 到 `MaaMacGui/master`，再单独 bump；或 (b) companion 已合并 upstream 后，将指针改为 **upstream 上真实存在的 commit**。 |
| **审阅** | `711daf6` diff 含 `StationPresetSupport.swift`、Rotation UI、内联 `preset`/`drones` 序列化，**内容可合并**；阻塞在 **流程/指针**，非实现质量。 |

### B2 — 远程 PR 与本地 squash 未对齐

| 项 | 内容 |
|----|------|
| **问题** | handoff 写明 **未 push**；`gh pr view 17164` 远程仍为 `b38bf14151` + 2× `chore: auto-commit`；brief 称 **CONFLICTING / DIRTY**。审阅 diff 以本地 `bcb128a202` 为准，**不能代表** GitHub PR 当前可合并状态。 |
| **改法** | rebase/merge `upstream/dev-v2` 解决冲突 → force-push（或新分支）**单 commit** `bcb128a202` 等价物 → 确认 PR diff 与 handoff `head` 一致。 |

### B3 — PR 描述未覆盖本轮叠改

| 项 | 内容 |
|----|------|
| **问题** | 远程 body 仍为旧版 facility_preset 叙述；未体现：内联 `preset`/`drones`、`reception_receive_clue`、会客室四项全关跳过、Mac Rotation UI、多任务多班次策略。 |
| **改法** | push 后按 brief §10 更新 #17164 body。 |

---

## 3. Suggestion

### 3.1 Core / 资源

| 文件 | 结论 |
|------|------|
| `InfrastTask.cpp` | ✅ 内联 `preset` 优先于 `filename`；`station_preset` 下 `drones` 对象与 `make_facility_preset_drones_task` 合理 |
| `InfrastTask.cpp` | ✅ `needs_reception` 四项 OR；全关则不挂载 `InfrastReceptionPresetTask` |
| `InfrastReceptionPresetTask.cpp` | ✅ `needs_clue_board` 为 false 时跳过线索板 Tab；仅信息板路径清晰 |
| `reception_receive_clue` | ✅ 仅预设链使用；经典 `InfrastReceptionTask` 未改行为（参数解析但不影响经典子任务编排） |
| `InfrastPresetTask.cpp` | ✅ `on_run_fails` 返回键回退；`rooms` 空时 warn 并 skip（与上轮结论一致） |
| `tasks.json` | ✅ `InfrastPresetSwitchButton` 灰/蓝双模板，无自定义降阈；信息板节点无 diff |

### 3.2 WPF

| 文件 | 问题 |
|------|------|
| `InfrastSettingsUserControlModel.cs` | ⚠️ `LocalizedObservableList` → 静态 `List` + 删除 `LanguageChanged` 刷新：疑似 **无关重构**，切换界面语言后下拉文案可能不更新；作者未测 WPF，建议 **回退本地化改动** 或补语言切换验证 |
| WPF 整体 | ❌ 仍无 `station_preset` / 内联 preset UI（已知；Mac 为参考实现） |
| `IncreaseCustomInfrastPlanIndex` | ✅ 增加 `AutoAdvancePlanIndex` 守卫，与属性一致 |

### 3.3 Companion

| 平台 | 结论 |
|------|------|
| **MaaMacGui `711daf6`** | ✅ Rotation `station_preset` UI、`StationPresetSupport`、API payload 与 `integration.md` 字段对齐 |
| **MAAUnified `b55a798`** | ⚠️ 指针合法但 **无** station_preset；与 Mac 能力差距维持 |

---

## 4. Nit

| 项 | 说明 |
|----|------|
| brief §5 | 写「MacGui 本地未 commit」，与 squash 内 `711daf6` 指针 **矛盾**；应更新 brief |
| `integration.md` | `replenish` 文案已改为制造站源石碎片，与实现一致 ✅ |
| 审阅文档进主 PR | `infrast-station-preset-pr-*.md` 是否长期留 `docs/zh-cn/develop/` 由作者决定，不阻塞 |

---

## 5. GUI 三端行为对照

| 能力 | WPF | MaaMacGui `711daf6` | MAAUnified `b55a798` |
|------|-----|---------------------|----------------------|
| `rotation_style=station_preset` | ❌ | ✅ | ❌ |
| 内联 `preset` / `drones` GUI | ❌ | ✅ | ❌ |
| `reception_receive_clue` 等会客室分层 | API only | ✅ | ❌ |
| `auto_advance_plan_index` | ✅ Custom | ✅（多任务策略下弱化） | ✅ Custom |
| 子模块指针在主 PR | — | ⚠️ 未进 upstream | ✅ |

---

## 6. §8 检查清单对照

| 项 | 结果 |
|----|------|
| 内联 preset 与 filename 优先级 | ✅ |
| drones 对象与 Rotation 字符串不冲突 | ✅ |
| 会客室挂载与线索板逻辑 | ✅ |
| `reception_receive_clue=false` 经典流 | ✅ 经典 Reception 未改 |
| 预设按钮失败回退 | ✅ |
| squash 无 auto-commit 噪音 | ✅ 本地 `bcb128a202` 单 commit |
| 远程无冲突、已 push | ❌ B2 |
| 子模块指针可解析 | ❌ B1（MacGui） |

---

## 7. 测试与 CI

| 项 | 状态 |
|----|------|
| Mac Core + MacGui 编译 | ✅ brief §7 |
| Mac station_preset UI 实测 | ✅ 用户自述 |
| WPF | ❌ 未测 |
| Windows CI | ⏳ push 后待 Actions |
| 国际服 OCR | ❌ 文档已声明仅国服 |

---

## 8. 建议合并顺序

1. **MaaMacGui**：向 `MaaAssistantArknights/MaaMacGui` 提 companion PR（`711daf6`），合并到 `master`
2. **主仓**：rebase + push squash；**或** 首版主 PR **去掉** `MaaMacGui` bump，仅保留 Core+协议+`MAAUnified`（若坚持分步）
3. 更新 **B3** PR 描述
4. 合并 #17164
5. bump `MaaMacGui`（若步骤 2 未含）
6. Unified station_preset UI follow-up

---

## 9. 变更文件复核摘要

| 区域 | 结论 |
|------|------|
| `InfrastPresetTask` / `InfrastReceptionPresetTask` | ✅ |
| `InfrastTask` 路由 + 内联 preset | ✅ |
| `integration.md` / `base-scheduling-schema.md` | ✅ |
| 资源模板 | ✅ |
| WPF | ⚠️ auto_advance 修复 + 可疑本地化重构 |
| `src/MaaMacGui` | ✅ 实现；❌ 指针未进 upstream |
| `src/MAAUnified` | ⚠️ bump 合法但能力不全 |

---

## 10. 第 1 轮闭环

| 审阅项 | 状态 | 说明 |
|--------|------|------|
| B1 MacGui 指针 | **待处理** | companion 先合 upstream |
| B2 push/冲突 | **待处理** | |
| B3 PR 描述 | **待处理** | |
| Core 内联 preset | **通过** | |
| MacGui UI | **通过**（实现） | |
