# 设施预设换班 PR 审阅包

> 对应主仓库 PR：[#17164](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17164)  
> 源分支：`xjbsteven:dev-v2` → 目标：`MaaAssistantArknights/dev-v2`  
> 本地 HEAD（rebase + squash，**未 push**）：`2e184b76f5`  
> **审阅结论**：[infrast-station-preset-pr-17164-review-result.md](./infrast-station-preset-pr-17164-review-result.md)（第 1 轮：`changes-needed`）

---

## 处理表（多轮审阅，开发维护）

| 轮次 | 审阅项 | 状态 | 说明 |
|------|--------|------|------|
| 1 | B1 `MaaMacGui@711daf6` 未进 upstream | **已修** | 主 PR 已移除 bump；companion [MaaMacGui#98](https://github.com/MaaAssistantArknights/MaaMacGui/pull/98) |
| 1 | B2 未 push / PR 冲突 | **已修（本地）** | rebase 完成 `2e184b76f5`；**待你 force-push** |
| 1 | B3 PR 描述 | **已起草** | 见 §10；push 后 `gh pr edit 17164` |
| 1 | Core 内联 preset + 会客室分层 | **已通过** | 见 result §3.1 |
| 1 | MacGui Rotation UI | **已通过**（实现） | companion：`711daf6` on `xjbsteven/MaaMacGui` |
| 1 | WPF 本地化重构 | **已回退** | 恢复 `LocalizedObservableList` + `LanguageChanged`；保留 `auto_advance_plan_index` |

---

## 1. 功能概述

在 **进驻总览（Rotation）** 与 **Custom `strategy: facility_preset`** 下，按 JSON 预设或 **内联 `preset` 对象** 点击各设施预设切换按钮完成换班；支持无人机、源石碎片补货、会客室分层行为、干员休整等辅助子任务。

**主 PR 范围**（`ab7ea823fc`）：

- Core + 资源 + 协议 + WPF（`auto_advance_plan_index` only）
- **不含** `MaaMacGui` / `MAAUnified` 子模块 bump（与 upstream `dev-v2` 指针一致）

**Companion（独立 PR）**：

- MaaMacGui `711daf6`：Rotation 进驻总览预设 UI、内联 `preset`/`drones` 序列化

---

## 2. 变更分层与 PR 范围

| 层级 | 仓库 | 是否在本 PR diff | 说明 |
|------|------|------------------|------|
| Core + 资源 | 主仓库 | 是 | `InfrastPresetTask` / `InfrastReceptionPresetTask` / `InfrastTask` |
| 协议文档 | 主仓库 | 是 | `integration.md`、`base-scheduling-schema.md` |
| WPF | 主仓库 | 是 | `auto_advance_plan_index` UI + 守卫；无 station_preset UI |
| MaaMacGui | 子模块 | **否** | companion PR；主 PR 指针 = upstream `2bc3b7b` |
| MAAUnified | 子模块 | **否** | 指针 = upstream `1a20c0f` |

---

## 3. 合并与发布惯例

1. 合并 **MaaMacGui** companion（`711daf6`）→ upstream `master`
2. **force-push** 主 PR `ab7ea823fc` → 更新 #17164
3. 更新 PR body（§10）
4. 合并 #17164
5. 单独 bump `MaaMacGui` 指针（若步骤 1 已完成）

---

## 4. 主仓库变更清单

| 区域 | 变更 |
|------|------|
| `InfrastPresetTask` / `InfrastReceptionPresetTask` | 设施预设换班 + 会客室预设流 |
| `InfrastTask.cpp` | 内联 `preset`/`drones`；会客室条件挂载；`reception_receive_clue` |
| `integration.md` / `base-scheduling-schema.md` | 协议补充 |
| WPF | `CustomInfrastAutoAdvancePlanIndex` + CheckBox；`IncreaseCustomInfrastPlanIndex` 守卫 |
| 资源 | 预设按钮灰/蓝模板；`facility_preset_3_shifts_daily.json` |

---

## 5. Companion GUI

| 仓库 | 分支 | commit | 状态 |
|------|------|--------|------|
| MaaMacGui | `feat/infrast-auto-advance-plan-index` | `711daf6` | [PR #98](https://github.com/MaaAssistantArknights/MaaMacGui/pull/98) 已开 |
| MAAUnified | — | — | 本 PR 不 bump |

---

## 6. 关键设计决策（有意为之）

| 决策 | 理由 |
|------|------|
| 主 PR 不 bump MacGui | B1：指针须先经 companion 进 upstream |
| 多班次 = 多个基建任务 | 单任务单 plan |
| 内联 `preset` 优先于 `filename` | GUI 不暴露 JSON；兼容旧 JSON |
| 会客室分层 | 访达页 ≠ 线索板 Tab |
| MacGui 默认布局 423 | 4 制造 / 2 贸易 / 3 发电；UserDefaults 记上次 |

---

## 7. 测试证据

| 项 | 状态 |
|----|------|
| Mac Core + MacGui 编译 | ✅ |
| Mac station_preset UI 实测 | ✅ 用户自述 |
| WPF | 未测（仅 auto_advance 小改） |
| rebase `upstream/dev-v2` | ✅ 本地 `ab7ea823fc` |

---

## 8. 审阅检查清单

- [x] 内联 preset 与 filename 优先级
- [x] 会客室挂载与线索板逻辑
- [x] 主 PR 子模块指针可解析（与 upstream 一致）
- [x] WPF 本地化未破坏
- [ ] force-push 后远程 PR 无冲突
- [ ] companion PR 合并

---

## 9. 本地复现审阅命令

```bash
git fetch upstream dev-v2
git diff upstream/dev-v2...2e184b76f5 --stat
```

---

## 10. PR 描述（#17164 body 草案）

```markdown
## Summary

基建 **进驻总览设施点预设换班**（`mode=20000` + `rotation_style=station_preset`）及 Custom `strategy: facility_preset`：

- 新增 `InfrastPresetTask` / `InfrastReceptionPresetTask`，按 JSON 或 **内联 `preset`** 点击各设施预设按钮换班
- `station_preset` 支持内联 `preset` / `drones` 对象，无需 `filename`
- 新增 `reception_receive_clue`；会客室四项全关时不进入会客室；仅信息板时不切线索板 Tab
- `replenish`：制造站源石碎片补货；`drones`（station_preset）：`{ enable, room, index, order }`
- WPF：Custom 计划 `auto_advance_plan_index`（任务链完成后自动切下一班次）

**Companion**：MaaMacGui Rotation 进驻总览预设 UI — [MaaMacGui#98](https://github.com/MaaAssistantArknights/MaaMacGui/pull/98)

## Test plan

- [x] Mac：Rotation station_preset UI + Core 内联 preset（开发版 MaaMacGui）
- [ ] Windows WPF：Custom auto_advance_plan_index
- [ ] JSON 示例 `facility_preset_3_shifts_daily.json`
```
