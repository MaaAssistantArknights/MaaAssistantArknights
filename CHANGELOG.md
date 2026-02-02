## v6.3.0-beta.7

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

## v6.3.0-beta.7

### 改进 | Improved

* 备选关卡读取后检查 @status102

### 修复 | Fix

* 切换刷理智任务时读取到错误的关卡列表 @status102

### 文档 | Docs

* Auto Update Changelogs of v6.3.0-beta.6 (#15571) @github-actions[bot] @github-actions[bot]

### 其他 | Other

* 消除部分编译警告 (#15578) @yali-hzy
* 刷理智高级设置使用hc:InfoElement.Title显示设置项名 @status102
* 添加 CMakePresets.json (#15568) @yali-hzy

## v6.3.0-beta.6

### 新增 | New

* 干员识别支持显示精英化等级、等级与潜能，并支持自动编队识别精英化与等级 (#15161) @ABA2396 @status102 @Manicsteiner @Constrat @HX3N
* 界园肉鸽通宝与 DLC2 分队数据更新 @SherkeyXD
* Yostar Dreamland / JieGarden 主题支持（JP / KR / EN）@Constrat @Manicsteiner @HX3N
* 新增注入弹窗“不再提醒”选项，勾选后使用软件渲染 @ABA2396
* 追加自定干员名称非法时的错误处理与本地化支持 (#15556) @yali-hzy @HX3N

### 改进 | Improved

* 优化关卡候选列表与关卡选择下拉刷新逻辑 (#15562) @status102 @HX3N @Constrat @Manicsteiner
* 自动战斗界面布局优化 (#15512) @yali-hzy
* TaskQueue 重命名与删除任务时显示任务序号 @status102
* 提取任务启用状态并统一字段使用 @status102
* 刷理智任务高级设置 UI 选项顺序优化 @status102
* OpenCV `cv::Mat` 使用 const reference 优化 @status102

### 修复 | Fix

* 修复自动战斗-自动编队自定干员名非法字段处理问题 @status102
* 禁用无需技能等级校验时的快速选中，修复外服技能描述过长导致的误选问题 @status102
* 修复追加信赖时助战重复计算问题 @status102
* 修复 OR 关卡掉落界面关卡名识别错误 @ABA2396
* 刷理智任务运行中禁止新增关卡，防止状态异常 @status102
* 修复自动编队干员等级不足的 i18n 问题 @status102
* 修复通宝优先级未定义导致的加载崩溃，回退至默认值 @SherkeyXD
* 修复新增任务时“完成后动作”设置 UI 未正确隐藏的问题 @status102
* 修复切换刷理智时候选掉落物重复加入下拉列表问题 @status102
* 修复信用收取任务好友访问与 OF-1 最后执行时间未保存问题 @status102
* 修复 YostarKR OCR 替换规则导致换行丢失问题 @HX3N
* 日服酒神干员识别修复 @Saratoga-Official
* 修复未勾选“每日仅访问一次”时会访问好友的问题 @status102
* 修复手动输入关卡名时错误移除过期关卡的问题 @status102
* 修复基建计划选择计数变化后 UI 未刷新问题 @status102
* 修复涤火杰西卡干员识别问题 @ABA2396

### 文档 | Docs

* 修正开发文档格式错误与笔误 (#15516) @yali-hzy

### 其他 | Other

* KR 本地化补充与确认弹窗文案优化 @HX3N
* 按钮不透明度与正则匹配规则调整 @ABA2396
* 调整“当前剿灭”键名以避免模式混淆 @status102
* 多语言小游戏与活动文案补充（EN / JP / KR）@Constrat @Manicsteiner @HX3N
* TXWY 数据补充与模板优化 @Constrat
* 补全缺失翻译 @ABA2396

## v6.3.0-beta.5

### 改进 | Improved

* 新 Config 加载时移除旧 Config 中不存在的配置 @status102
* 优化信用战斗检查启用判断 @status102
* 优化干员识别、仓库识别显示 @ABA2396
* 存在 crash.log 时, 尝试获取 dumps 文件 (#15432) @status102
* 配置迁移检查简化 @status102
* 新 Config 字符序列化 @status102
* 剿灭卡使用到上限时不报错停止 @ABA2396
* 剿灭关卡通过 ends_with 判断 @ABA2396
* FightTask 以剿灭为目标关卡时, 在终端界面找不到周剿灭获取进度图标不再以报错退出 @status102

### 修复 | Fix

* 剩余理智启用状态迁移后未能从旧配置移除 @status102
* 剩余理智关卡 关卡选择 迁移后错误使用 @status102
* 萨米肉鸽刷开局功能异常 @ABA2396
* 基建计划选中 Index 超出范围 @status102
* SEH 错误终止运行 @status102
* 自动编队识别技能等级匹配失败 @status102
* 信用收取后刷 OF-1 不会在后续刷理智任务选中 `当前/上次` 时禁用 @status102
* 启动 MAA 后重新读取基建计划 @status102
* 剩余理智勾选且设定关卡为空时, 迁移后禁用剩余理智 @status102
* 配置迁移后切换回原配置 @status102
* `源石恢复` -> `使用源石` 遗漏 @status102
* 刷理智使用源石 CheckBox 勾选后不生效 @status102

### 文档 | Docs

* 繁中文件大更新 (#15480) @momomochi987

### 其他 | Other

* 增加借助战 OF-1 在后续刷理智选择 `当前/上次` 导致禁用时的输出 (#15478) @status102 @ABA2396 @momomochi987 @Constrat @Manicsteiner @HX3N
* 繁中服不上报企鹅物流 @ABA2396
* 基建找不到对应时间的基建计划 (#15468) @status102 @momomochi987
* 移除不再使用的 VirtualizingWrapPanel 与 NoAutomationDataGrid @ABA2396

## v6.3.0-beta.4

### 改进 | Improved

* 自动战斗掉线重连、自动肉鸽在战斗结束前延迟 ｢停止｣ 动作 添加多任务共用提示 @ABA2396

### 修复 | Fix

* 配置迁移后移除 gui.new 中多余的 config @status102
* 任务序列化 Catch @status102
* 基建计划转换期增加检查 @status102
* 多配置用户在删除 Default 配置时迁移异常 @ABA2396
* EX 关符合时 1 被识别为 | @ABA2396
* 修复移动已打开设置的任务后，当前的设置面板无法继续修改的问题 @ABA2396
* add MaaWin32ControlUnit to nightly build (#15447) @Manicsteiner

### 文档 | Docs

* changelog for PC arknights @MistEO

### 其他 | Other

* 调整删旧配置时机 @ABA2396

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
