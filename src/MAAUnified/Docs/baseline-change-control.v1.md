# MAAUnified 基线冻结变更控制 v1

## 适用范围
- 机读基线: `src/MAAUnified/Compat/Mapping/Baseline/baseline.freeze.v1.json`
- 机读验收模板: `src/MAAUnified/Compat/Mapping/Baseline/acceptance.template.v1.json`
- 评审文档: `baseline.freeze.v1.md`, `acceptance.checklist.template.v1.md`

## 冻结规则
1. Package A 冻结后，所有条目默认 `priority=P0`，禁止私自降级。
2. `parity_status` 仅允许: `Aligned`, `Gap`, `Waived`。
3. 每条记录必须包含 `evidence` 四元组:
   - `ui_path`
   - `log_path`
   - `scope`
   - `case_id`
4. 任意失败路径必须满足:
   - 应用进程不崩溃
   - UI 有可见错误反馈
   - `debug/` 下有可定位日志

## 变更类型
- `Data-only`: 仅修改基线 JSON 的状态、备注、证据、案例矩阵。
- `Schema`: 修改契约字段或约束。
- `Policy`: 修改 P0 口径、矩阵策略、Waiver 规则。

## 审批流程
1. 提交 PR 时标注变更类型（Data-only / Schema / Policy）。
2. Data-only 需至少 1 名前端维护者 + 1 名测试维护者审批。
3. Schema/Policy 需额外 1 名技术负责人审批。
4. 合并前必须通过 `MAAUnified.Tests`（包含基线门禁测试）。

## Waiver 规则
- 仅当存在真实阻塞且无法在当前包完成时允许 `Waived`。
- 必填字段:
  - `owner`
  - `reason`
  - `expires_on`（ISO 日期）
  - `alternative_validation`
- `expires_on` 超期后，CI 视为失败（需移除或续签并说明）。

## 文档同步
- `baseline.freeze.v1.md` 与 `acceptance.checklist.template.v1.md` 视为基线 JSON 的投影。
- 若 JSON 变更导致文档未更新，`GeneratedDocs_ShouldMatchMachineReadableSource` 必须失败。

## 版本策略
- 破坏性变更（字段删除/重命名）必须升主版本（`v2`）。
- 向后兼容增强（新增字段/案例）升次版本（`v1.x`）。
- 修正文案或证据路径升补丁版本（`v1.x.y`）。
