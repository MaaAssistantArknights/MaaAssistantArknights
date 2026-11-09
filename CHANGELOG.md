## v6.22.0

### Highlights

#### OCR 引擎升级至 PP-OCRv6，新增推理后端选择（实验性 WebGPU）

桌面端中英文与繁体、日服识别模型统一切换至 PP-OCRv6 多语言模型，韩文升级至 PP-OCRv5，识别精度与覆盖率提升。新增 ｢推理后端｣ 设置项，可在 ONNX Runtime（默认）与实验性 WebGPU 之间切换；WebGPU 后端为测试功能，如遇问题请通过 ｢设置 - 问题反馈｣ 提交反馈。

#### 新增 ｢作战日报｣，按天汇总战斗掉落

任务队列新增 ｢作战日报｣ 入口，按日历与柱状图汇总每日作战场次、理智消耗、材料掉落与源石消耗，支持按材料筛选并查看每场掉落。

#### 卫戍协议三期适配

卫戍协议三期活动已适配，可使用 ｢小工具-牛杂｣ 中的 ｢卫戍协议｣ 刷取等级奖励。

#### 设置页结构调整

WPF 设置页分 ｢常规设置｣ 与 ｢高级设置｣ 两区，低频项收纳到 ｢高级设置｣。

<details>
<summary><b>English</b></summary>

#### OCR Engine Upgraded to PP-OCRv6 with Inference Backend Selection (Experimental WebGPU)

The desktop OCR model for Chinese (Simplified/Traditional), English, and Japanese now uses the unified PP-OCRv6 multilingual model; Korean moves to PP-OCRv5, improving accuracy and coverage. A new "Inference Backend" setting lets you switch between ONNX Runtime (default) and an experimental WebGPU backend. WebGPU is a testing feature — if you run into issues, please report via Settings - Issue Report.

#### New "Combat Daily Report" Aggregates Battles by Day

The task queue gains a "Combat Daily Report" entry that summarizes daily battle count, sanity used, material drops, and originite consumption in a calendar and bar chart; supports filtering by material and per-run drop details.

#### Stronghold Protocol Season 3 Support

Stronghold Protocol Season 3 is now supported — use "Stronghold Protocol" under Toolbox - Useful Tasks to farm level rewards.

#### Settings Reorganization: Low-Frequency Items Grouped Under "Advanced"

Low-frequency WPF settings are grouped under an "Advanced" section while achievements stay in the regular area.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.22.0 (2026-11-09)</b></summary>

### 新增 | New

* 任务队列新增 ｢作战日报｣，按日历与柱状图汇总每日作战场次、理智、掉落与源石消耗 ([#18918](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18918)) @ABA2396
* 适配卫戍协议三期，刷取需使用默认皮肤 @ABA2396
* 新增推理后端选择，可在 ONNX Runtime（默认）与实验性 WebGPU 之间切换，WebGPU 推理会话在资源加载后预热 @SherkeyXD @hguandl

### 改进 | Improved

* OCR 引擎升级至 PP-OCRv6（中英繁日统一多语言模型）与 PP-OCRv5（韩文），并接入 v6 ASCII 专用识别模型 ([#18246](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18246), [#18812](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18812)) @SherkeyXD @Plumess
* WPF 设置页分 ｢常规设置｣ 与 ｢高级设置｣ 两区，低频项收纳至 ｢高级设置｣ ([#19095](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19095)) @MistEO @ABA2396
* 设置指引导航移至顶部横排并优化红点指引与长文案适配 ([#19134](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19134)) @ABA2396
* 部署操作统一在控制器支持暂停时先暂停再部署，移除原外服的单独开关 ([#19135](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19135)) @momomochi987
* 干员培养清空计划增加二次确认 ([#19140](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19140)) @Appler-D
* 库存保持计划列表支持滚轮边缘接力 ([#19137](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19137)) @Appler-D
* 剿灭入口找不到、拜访好友失败等静默略过时在界面日志显示提示 ([#19098](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19098)) @momomochi987
* 优化全屏下设置指引提示位置 @ABA2396

### 修复 | Fix

* 修复资源收集、肉鸽在卫戍协议常驻后无法导航的问题 ([#19179](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19179)) @Radekyspec
* 修复“狂躁铁皮”被识别为干员的问题 @ABA2396
* 修复从指定任务开始执行时，其余任务状态可能未被标记为跳过的问题 @status102
* 修复库存保持 EN 长文案导致提示按钮被裁掉不可达的问题 ([#19169](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/19169)) @ABA2396
* 修复肉鸽确认放弃时序，确认后再记录日志和成就 ([#19130](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19130)) @zhulongqihan @ZiyinLin
* 修复代理失败超限后未退出结算页直接结束任务的问题 ([#19131](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19131)) @zhulongqihan
* 修复黑流树海开局奖励选卡未生效时卡死的问题，未生效时自动重试 @ZiyinLin
* 修复已是日间主题但识别不到入口时无限循环更换主题的问题；返回后找不到入口改为正常结束 ([#19111](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19111)) @momomochi987
* 修复干员 ｢孑｣ OCR 替换规则（国服、繁中服）([#19193](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/19193)) @Saratoga-Official
* 繁中服剿灭入口更新模板，基建弹窗补充繁中文字 ([#19097](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19097)) @momomochi987
* 繁中服更新剿灭图标模板 ([#19190](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19190)) @Daydreamer114
* 修复环圣骏堡电车 OCR @Daydreamer114
* 修复战时新闻委员会基尔萨宁乡土报相关 OCR 误识别 ([#19119](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19119)) @hanyuwei707
* YostarJP 放宽遥（ハルカ）干员名 OCR 替换规则 @Manicsteiner
* YostarJP 放宽魔王（シヴィライト・エテルナ）干员名 OCR 替换规则 ([#19167](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19167)) @amasoiD
* YostarJP 修复生息演算 3、肉鸽基础任务与 ocrReplace ([#19108](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19108)) @Manicsteiner
* YostarKR 适配新 OCR 模型 ([#19159](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19159)) @HX3N

### 文档 | Docs

* 补充推理后端选择与实验性 WebGPU 的说明 @SherkeyXD

### 其他 | Other

* 模板优化脚本改用参数列表调用子进程，消除文件名 shell 命令注入 @ABA2396
* Android ncnn OCR 适配 PP-OCRv6，对齐 FastDeploy 行为；OcrPackNcnn 文本行裁切与 rec 分批补边逐项对齐 ([#19117](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19117)) @Aliothmoon
* 干员养成改用内部存储代替 Status 通信 @status102
* 移除部分过早的配置迁移器（FightTaskStageResetMode 系列） @status102
* 修复 clang-formatter 路径解析与失败状态丢失的问题，补集成测试 ([#19129](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19129)) @zhulongqihan
* TilePack 内部字段 has_multi_stages 更名为 has_multi_phases @status102
* 更新 MaaDeps @SherkeyXD
* 修复 smoke-testing 工作流问题 @SherkeyXD

</details>
