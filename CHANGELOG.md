## v6.7.0

### 正式接入 DeepSleep | Highlight

我们很高兴地宣布：从 v6.7.0 开始，MAA 将正式接入 DeepSleep。这将意味着我们首次将 AI 引入到游戏助手领域，为你带来更智能、高效的体验。DeepSleep 的 AI 算法能够实时分析游戏运行数据，动态生成运行日志，在保证游戏流畅度的同时有效提升用户体验。这一功能的加入，标志着 MAA 在智能化方向上迈出了关键一步，未来我们也将持续探索 AI 技术与 MAA 的深度融合，为你带来更多创新体验。 ~（绝对不是牛牛喝醉了睡着导致的）~

#### 成就 DLC #3 同步上线

除了 DeepSleep，本次更新也带来了成就 DLC #3。

* 成就现已支持按 DLC 期数搜索；
* 补充了一批新的成就触发与描述；
* 多语言文本与部分触发细节也一并完成调整。

#### 新增支持任务单次运行

右键任务设置图标即可弹出二级菜单，“单次运行”与“重命名”、“删除”并列，方便快速执行单次任务。

#### 多项细节问题一并修复

除了 DeepSleep，本次更新也处理了一批影响体验的细节问题。

* 修复在非 UI 线程清空库存数据可能导致任务添加失败的问题；
* 修复多项剿灭相关识别与战斗完成 ROI 问题。

----

We are pleased to announce that starting from v6.7.0, MAA will officially integrate DeepSleep. This marks the first time we bring AI into the game assistant space, delivering a smarter and more efficient experience. DeepSleep's AI algorithm analyzes game runtime data in real time and dynamically generates operation logs, effectively enhancing user experience while maintaining smooth gameplay. The addition of this feature represents a key milestone in MAA's journey toward intelligence, and we will continue to explore deeper integration of AI technology with MAA to bring you more innovative experiences. ~(Definitely not because Pallas got drunk and fell asleep)~

#### Achievement DLC #3 Included

Alongside DeepSleep, this update also brings Achievement DLC #3.

* Achievements are now organized by DLC release phase;
* A new batch of achievement triggers and descriptions has been added;
* Multilingual texts and several trigger details have also been refined.

### New Support for Run Once of Tasks
Right-click the task settings icon to open a secondary menu, where "Run Once" is now available alongside "Rename" and "Delete," making it easy to quickly execute a task once.

#### Various UX Fixes Included

Besides DeepSleep, this update also fixes several small but noticeable issues.

* Fix task creation failures caused by clearing depot data off the UI thread;
* Fix several issues related to Annihilation recognition and completion ROI.

----

以下是详细内容：

### 新增 | New

* 正式接入 DeepSleep @ABA2396
* 新增成就 DLC#3，支持按 DLC 期数搜索并补充新的成就触发与描述 @ABA2396 @Constrat @HX3N @Manicsteiner
* 新增任务设置按钮右键单次运行功能 @ABA2396

### 改进 | Improved

* 优化自动战斗 tab 样式，新增滑块动画效果，修复切页重置状态 @ABA2396
* data 数据存储优化 @status102
* 优化日志展示效果 @ABA2396
* 在收到对应识别结果时再重置库存数据 @ABA2396
* 使用 DateTimeOffset 替代 DateTime @status102
* Log 头时间从 UTC 时间改为 Local 时间 @status102
* 不再默认启用 `在下拉框中隐藏当日未开放关卡` @status102
* Ocr 内部 log 在 without_det 时也对 rect 输出进行基于 base_roi 的偏移 @status102

### 修复 | Fix

* 在更新时报错无法显示本地化语言 @ABA2396
* 粘贴作业集不会自动勾选多作业模式 @ABA2396
* 亮色模式下 Linkstart 按钮样式缺失 @ABA2396
* 修复 build warning，smtp 改用新版本 MailKit @ABA2396
* 修复剿灭关卡名 OcrReplace @status102
* 修复在非 UI 线程调用清空库存数据导致任务添加失败 @ABA2396
* 修复日志记录中使用源石 TaskName 的问题 @status102
* 修复任务开始/完成时无法显示任务名 @ABA2396
* 修复读取备份成就时不会加载 CustomData 的问题 @ABA2396
* 更新 MaaFramework 文件名格式和 .NET SDK 版本 @AnnAngela
* 移除过时的配置迁移兼容逻辑 @status102
* 修复剿灭战斗完成 roi @status102
* 修复 CN 剿灭后识别，并统一全客户端类型识别 @status102

### 其他 | Other

* 避免生成不必要的 res_updater (#16116) @status102 @Constrat @soundofautumn
* 更新数据任务禁用高级设置 @ABA2396
* DateTimeOffset 基础支持 @status102
* 修复错误描述：`过期关卡重置` @status102
* 补充遗漏图片资源 @status102
* Revise linking guidelines and AI suggestions @MistEO
* resolve warning @status102
