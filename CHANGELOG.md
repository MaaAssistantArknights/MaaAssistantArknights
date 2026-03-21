## v6.6.0

### 这次真的开箱即用了 | Highlight

从 `v6.6.0` 开始，Windows 用户无需手动安装 `.NET` 即可直接使用 MAA。由于首次更新会一并下载运行时，更新包体积会暂时增加到约 `75 MB`，后续版本的更新大小将恢复正常。

#### 自动战斗与作业切换更顺手

本次更新继续优化了作业相关体验。在自动战斗中粘贴神秘代码时，MAA 会尝试识别作业类型；若识别成功，则自动切换到对应标签页。

同时，保全派驻现已与其他活动一致，支持作业集粘贴与快速切换。

#### 一键长草新增 ｢更新数据｣ 任务

一键长草中新增了 ｢更新数据｣ 任务。执行后，MAA 会自动进行干员识别和仓库识别；你也可以将其设置为每次执行、每天一次或每周一次，以定期刷新本地数据。

识别结果会同步应用到其他功能中：公招识别可显示干员的潜能与持有情况，主界面的关卡小提示也会展示仓库中的相关库存数据，方便在抄作业或刷图前快速了解当前资源。

#### 外服内容继续适配

针对外服玩家，本次更新补充了肉鸽 DLC 的相关支持。

----

Starting from `v6.6.0`, Windows users no longer need to manually install `.NET` to use MAA. Since the runtime will be bundled during the first update, the package size will temporarily increase to around `75 MB`, but subsequent updates will return to a normal size.

#### Smoother Copilot and Task Switching

This update continues to improve the task-related experience. When pasting a Secret Code in the Copilot input, MAA will attempt to detect the task type; if successful, it will automatically switch to the corresponding tab.

At the same time, Stationary Security Service now supports job set pasting and quick swapping, consistent with other activities.

#### New "Update Doctor Data" Task in Farming

A new "Update Doctor Data" task has been added to the Farming tab. When executed, MAA will automatically perform operator and depot recognition. You can also configure it to run every time, daily, or weekly to keep local data up to date.

The recognition results will be applied across other features: Recruitment recognition can display operator potential and ownership status, while stage tips on the main interface can show relevant inventory data, helping you quickly review your resources before copying strategies or farming stages.

#### Continued Support for Global Servers

For global server players, this update adds support for the **Sui's Garden** Integrated Strategies **DLC 1**!

### 新增 | New

* 自动战斗增加页签自动切换逻辑、保全增加快速切换列表 @ABA2396 @status102
* 增加更新数据任务 (#16026) @ABA2396 @status102
* 同时启动多个模拟器时使用自动检测连接提供弹窗选择 (#16020) @ABA2396
* 运行结束后保留上次运行结果 @ABA2396
* 支持雷电 14 注册表查询 @ABA2396
* AVD 截图增强 (#15608) @satgo1546

### 改进 | Improved

* 减少透明窗口渲染大小，日志悬浮窗支持实时跟随 @ABA2396
* 刷理智任务仅在 stage 为空时检查无掉落关卡，并在使用理智药前检查药品数量 @status102
* 避免下载作业时等待全部完成并减少重复赋值 @status102
* 作业集解析不再输出详细信息以避免刷屏，并优化解析按钮 icon @status102
* 优化 TaskQueueList 列表高度 @status102 @ABA2396
* 更新后首次重启仅进行文件更新不加载多余数据 @ABA2396
* WPF 下载框样式遵循是否使用卡片设置 (#16029) @status102
* 空配置默认任务拆分，并在添加/修改任务设置时返回 taskId @status102
* 移除手动触发切换账号的启动流程逻辑，统一使用 LinkStart @status102
* hoist image-side cvtColor out of template loop in Matcher (#16018) @Aliothmoon

### 修复 | Fix

* 修复 VisitNextBlack 难以触发导致任务循环的问题 (#15767) @sylw114
* 修复自定干员技能范围检查问题 @status102
* 修复勾选手动输入关卡名时无法拖动候选关卡 @ABA2396
* 修复使用空图片匹配时错误 Log 输出 @status102
* 修复地图名查找问题 @status102
* 修复通宝置换/投钱后可能出现的藏品或通宝获得弹窗问题 (#15993) @travellerse
* 修复 LevelKey 空属性导致误匹配 @status102
* 修复刷理智任务返回未开放关卡问题 @status102
* 修复恢复 StagePlan 后 AsstFightTask 的 Stage 问题 @status102
* 修复 Eyjafjalla Alter 正则（EN）@Constrat

### 文档 | Docs

* 修改 git clone 命令使用 --single-branch (#16000) @AnnAngela

### 其他 | Other

* 更新繁中服 "聘用候選人" 截圖 (#16056) @momomochi987
* 使用本地缓存数据时不显示 Growl @ABA2396
* 赠送线索后增加等待以避免弹窗遮挡新线索图标 @ABA2396
* 添加 log 分析与 issue analysis @MistEO
* 多语言内容更新与优化（JP/EN/KR OCR、Roguelike JieGarden DLC1、EP16 等）@Manicsteiner @Constrat @HX3N
* 优化英文输出并为 SKILL.md 添加英文翻译部分 @MistEO
* add discord release notification @Constrat
