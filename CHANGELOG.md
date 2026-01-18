## v6.3.0-beta.2

### Highlights

#### WPF 一键长草任务配置重构

本次更新对 WPF 端 ｢一键长草｣ 任务配置进行了重构，显著提升了可用性与表达能力。

现在支持 **添加重复类型任务**，并支持重命名；新增 **理智作战周计划**：可按星期判断任务是否执行；同时任务勾选框会根据运行状态以不同颜色进行区分，直观反馈任务被跳过、已完成、运行中或执行失败等状态。

由于现已支持 ｢理智作战｣ 周计划并可添加多个同类型任务，本次更新**移除了原有 ｢剿灭作战｣ 任务失败后自动尝试下一个已开放的备选逻辑**。如有相关需要，请手动新增一个 ｢理智作战｣ 任务，并**在关卡中选择“剿灭作战”**。

#### 支持连接 PC 端

MAA 现已支持连接 PC 端运行，但后续不会对 PC 端运行情况提供维护或适配支持：
- 功能可正常使用的情况下，可自行选择继续使用  
- **若无法运行或存在问题，不受理 PC 端相关的 Issue 与反馈**  
- 欢迎社区自行适配并提交 Pull Request

----

#### WPF Farming Task Configuration Reconstruction

This update reconstructs the WPF "Farming" task configuration, significantly improving usability and expressiveness.

Now supports **adding repeating task types** and renaming them; introduces **Combat weekly schedule**: you can determine whether tasks are executed based on the day of the week; task checkboxes will also be distinguished by different colors according to the running status, providing intuitive feedback on tasks being skipped, completed, running, or failed.

Since the "Combat" weekly schedule is now supported and multiple similar tasks can be added, this update **removes the original automatic attempt to use the next available alternate stage after "Annihilation" task failure**. If you need this, please manually add a "Combat" task and **select "Annihilation" in the stage**.

#### Supports Connection to PC (CN only)

MAA now supports running on PC, but will not provide maintenance or adaptation support for PC-side situations in the future:
- If the functionality works properly, you can choose to continue using it
- **If it cannot run or there are issues, PC-related issues and feedback will not be accepted**
- Community-contributed adaptations and pull requests are welcome

----

## v6.3.0-beta.2

### 修复 | Fix

* 有猪乱写 OF-1 和 当前/上次 的条件 @ABA2396

## v6.3.0-beta.1

### 新增 | New

* WPF 一键长草任务配置重构，支持重复添加任务，支持理智作战周计划 (#15385) @status102 @ABA2396
* 支持 PC 端明日方舟 (#15407) @MistEO @ABA2396

### 改进 | Improved

* 自动战斗编队、技能等级不足、使用理智药及碎石描述优化 (#15435) @status102

### 修复 | Fix

* 修复指定技能等级时三技能无法正确选中的问题 @ABA2396 @status102
* 日服見字祠识别错误 @Saratoga-Official
* 粘贴作业集代码后下方的链接未重置为作业站链接 @ABA2396
* JP AT 小游戏确认按钮识别 (#15427) @Manicsteiner
* 日服界园JieGarden战略变更OCR识别 @Saratoga-Official
* 国服IS投资模板更新 @Constrat
* YostarJP 肉鸽OCR识别 @Manicsteiner
* EN IS6 提示文本 @Constrat

### 文档 | Docs

* 集成文档统一格式，同时显示 field-group 和示例代码 (#15409) @ABA2396 @Manicsteiner @Constrat
* 有猪改漏了 @ABA2396
