## v6.3.0-beta.3

### Highlights

#### WPF 一键长草任务配置重构

本次更新对 WPF 端 ｢一键长草｣ 任务配置进行了重构，显著提升了可用性与表达能力。

现在支持 **添加重复类型任务**，并支持重命名；新增 **理智作战周计划**：可按星期判断任务是否执行；同时任务勾选框会根据运行状态以不同颜色进行区分，直观反馈任务被跳过、已完成、运行中或执行失败等状态。

由于现已支持 ｢理智作战｣ 周计划并可添加多个同类型任务，本次更新**移除了原有 ｢剿灭作战｣ 任务失败后自动尝试下一个已开放的备选逻辑**。如有相关需要，请手动新增一个 ｢理智作战｣ 任务，并**在关卡中选择“剿灭作战”**。

#### PC 端明日方舟

MAA 现已支持 PC 端明日方舟的运行。但由于维护人手有限，PC 端版本可能长期处于不稳定状态，部分功能可能出现异常或暂时无法使用。我们非常欢迎社区开发者协助适配并提交改进，共同完善对 PC 端的支持。

----

#### WPF Farming Task Configuration Reconstruction

This update reconstructs the WPF "Farming" task configuration, significantly improving usability and expressiveness.

Now supports **adding repeating task types** and renaming them; introduces **Combat weekly schedule**: you can determine whether tasks are executed based on the day of the week; task checkboxes will also be distinguished by different colors according to the running status, providing intuitive feedback on tasks being skipped, completed, running, or failed.

Since the "Combat" weekly schedule is now supported and multiple similar tasks can be added, this update **removes the original automatic attempt to use the next available alternate stage after "Annihilation" task failure**. If you need this, please manually add a "Combat" task and **select "Annihilation" in the stage**.

#### PC version of Arknights (CN only)

MAA now supports the PC version of Arknights. However, due to limited maintenance resources, the PC version may remain unstable for an extended period, with some features potentially malfunctioning or temporarily unavailable. We warmly welcome community developers to assist in adaptation and submit improvements to collectively enhance PC support.

----

## v6.3.0-beta.3

### 新增 | New

* 设置指引增加右键重命名 / 删除提示 @ABA2396
* PC 端说明文案调整 @MistEO

### 改进 | Improved

* 优化任务设置按钮悬浮提示 @ABA2396
* 开始唤醒多任务共用参数提示 @status102
* 开始唤醒任务未设置账号切换时，禁用手动切换按钮 @status102
* 优化设置右键菜单布局 @ABA2396
* 更换 Config 迁移检查逻辑 @status102

### 修复 | Fix

* 修复右键“跳过一次”异常行为 @status102
* 修复新 Config 丢失 Default 的问题 @status102
* 修复 Config 仅剩最后一个时移除按钮状态错误 @status102
* 修复删除配置时未清理 `.new` 文件的问题 @ABA2396
* 修复公招加速券相关异常 @status102
* 修复启动客户端绑定失效的问题 @status102
* 修复关卡列表显示不刷新的问题 @status102
* 修复启动客户端绑定流程异常 @status102
* OF-1 跳过条件又有猪改错了 ↓ @ABA2396
* 修复收取信用检查逻辑问题 @status102
* 修复启动 MAA 时无任何任务未自动追加默认任务的问题 @status102
* YostarEN refresh node template @Constrat

### 其他 | Other

* style @status102
* EN fix @Constrat

## v6.3.0-beta.2

### 修复 | Fix

* 在赠送线索时弹出上次线索交流结束的提示时无法返回 @ABA2396
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
