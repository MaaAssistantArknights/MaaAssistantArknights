---
icon: bxs:book
---

# 材料合成

`MaterialCraft` 使用加工站精英材料配方，先展开依赖，再按顺序合成。`items` 表示需要新合成的数量，不是目标库存总量。

启动时若已在加工界面或配方列表，直接继续；若在加工站房间，点击左下角加工入口。其他页面通过 `MaterialCraft@InfrastBegin` 复用基建导航，确认已到基建后立即结束导航，不点击铃铛、不领取产出，也不执行领取前的固定等待。随后通过加工站的正常/缩小视图模板定位设施，当前视图找不到时使用与办公室任务相同的横滑流程。确认进入加工站房间后才点击加工入口；进入后未确认页面的重试会先返回基建。

```json
{
    "items": [{ "itemId": "30135", "count": 1 }],
    "inventory": { "30124": 2, "30134": 1, "4001": 10000 }
}
```

`inventory` 是调用方提供的库存快照。数量为非负整数，目标数量必须大于零。兼容 `items` 字典、`targets` 数组、单个 `itemId` / `count` 和 `depot` 库存字段；新调用方建议使用上面的格式。

## 只计算计划

```cpp
AsstSize AsstGetMaterialCraftPlan(const char* params, char* buff, AsstSize buff_size);
```

先调用 `AsstLoadResource`。此接口不需要创建实例或连接游戏，不会启动任务、点击游戏或修改库存。

返回 UTF-8 JSON 的字节数，不包含结尾 NUL。`buff` 为 null 或容量不足时不写入缓冲区，可按返回大小分配后重试。资源未初始化或参数无法解析时返回 `AsstGetNullSize()`。两次调用之间允许资源更新，调用方应再次检查返回长度。

计划包含：

- `valid` / `error`：是否能构造计划及失败原因。循环依赖、未知目标和数量溢出不能执行。
- `operations`：有序的 `formula_id`、`item_id`、`batches` 和产出 `count`。
- `inventory`：执行完整计划后的估算库存。
- `missing`：缺少的基础材料，包含 `item_id` 和 `count`。
- `gold_cost` / `ap_cost`：64 位整数成本；`mood_cost` 为 `ap_cost / 360000`。

`valid: true` 与 `missing` 非空可以同时出现，保留了根据旧库存快照尝试加工的能力。它不保证游戏内材料充足。规划不计干员副产品；只有输入库存包含 `4001` 时才计算龙门币的库存变化。

## 执行回调

以下均为 `SubTaskExtraInfo`，以外层 `taskid` 关联执行，不能用当前正在编辑的队列代替已提交快照。

- `MaterialCraftPlan`：`details` 与只计算计划的结果相同。
- `MaterialCraftPlanFailed`：计划无效或存在缺料，`details` 包含具体原因。
- `MaterialCraftOperationStarted`：已读回加工份数、即将开始加工。`details` 包含从 0 递增的 `operation_id`、`item_id` 和实际选中的 `batches`。
- `MaterialCraftOperationCompleted`：已识别到领取界面，包含相同操作标识及 `inventory_changes` 数组；数组元素为 `item_id` 和有符号数量 `count`。`inventory_complete: false` 表示无法排除副产品，变化只能作为已知部分。

游戏限制单次份数时，一个计划操作可以拆成多个执行回调。调用方必须按 `(taskid, operation_id)` 去重。完成回调可能早于领取界面的关闭；之后停止任务也不能撤销已经确认的库存变化。

中断时保留已确认的库存变化。开始后未收到完成回调，或库存变化不完整时，保留库存待确认标记，但不拦截下一次合成，允许使用当前库存快照继续规划。每种目标材料的全部计划批次确认完成后，即从待合成区移除；整轮结束时统一显示已确认的库存净变化。回调异步传递，调用方应在 `AsstStart` 前持久化库存待确认状态，覆盖进程意外退出的情况。

## 材料需求识别

`MaterialRequirement` 对当前材料需求界面截图，不自动导航。`MaterialRequirementInfo` 的 `details` 包含 `done`、`status` 和 `items`。

数量识别使用字符模型，仅接受完整的 `已有数量/需求数量`。优先识别原尺寸预处理结果与三倍放大的原色区域；置信度不足时，再通过红色通道去除蓝底，复核原尺寸和两倍放大的数字。必须获得两次置信度均不低于 0.95 的一致结果；高置信度结果冲突、格式异常、数量溢出或复核不足时，该项按识别失败处理，不推测缺口。部分识别失败也会在 `debug/material_requirement` 保存截图。

`status` 为 `success`、`partial` 或 `failed`。`items` 只包含缺料项：`item_id`、`item_name`、`owned`、`required`、`shortage`。`success` 且数组为空时显示“材料已足够”；不能把 `partial` 或 `failed` 的空数组当作没有缺料。

从训练室专精确认页启动合成时，依次返回专精技能选择页、训练室、基建总览，共点击三次左上角返回，随后复用基建导航。该返回分支不适用于其他需求页面。
