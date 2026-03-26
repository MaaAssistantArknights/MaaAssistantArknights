## v6.7.0

### 正式接入 DeepSleep | Highlight

从 v6.7.0 开始，MAA 正式接入 DeepSleep。

#### 成就 DLC #3 同步上线

除了 DeepSleep，本次更新也带来了成就 DLC #3。

* 成就现已支持按 DLC 期数搜索；
* 补充了一批新的成就触发与描述；
* 多语言文本与部分触发细节也一并完成调整。

#### 多项细节问题一并修复

除了 DeepSleep，本次更新也处理了一批影响体验的细节问题。

* 修复任务开始/完成时无法显示任务名的问题；
* 修复在非 UI 线程清空库存数据可能导致任务添加失败的问题；
* 修复多项剿灭相关识别与战斗完成 ROI 问题。

----

Starting with v6.7.0, MAA officially introduces DeepSleep.

#### Achievement DLC #3 Included

Alongside DeepSleep, this update also brings Achievement DLC #3.

* Achievements are now organized by DLC release phase;
* A new batch of achievement triggers and descriptions has been added;
* Multilingual texts and several trigger details have also been refined.

#### Various UX Fixes Included

Besides DeepSleep, this update also fixes several small but noticeable issues.

* Fix missing task names when tasks start or finish;
* Fix task creation failures caused by clearing depot data off the UI thread;
* Fix several issues related to Annihilation recognition and completion ROI.

----

以下是详细内容：

### 新增 | New

* 正式接入 DeepSleep @ABA2396
* 新增成就 DLC#3，支持按 DLC 期数搜索并补充新的成就触发与描述 @ABA2396 @Constrat @HX3N @Manicsteiner

### 改进 | Improved

* 使用 DateTimeOffset 替代 DateTime @status102
* Log 头时间从 UTC 时间改为 Local 时间 @status102
* 不再默认启用 `在下拉框中隐藏当日未开放关卡` @status102
* Ocr 内部 log 在 without_det 时也对 rect 输出进行基于 base_roi 的偏移 @status102

### 修复 | Fix

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

* DateTimeOffset 基础支持 @status102
* 修复错误描述：`过期关卡重置` @status102
* 补充遗漏图片资源 @status102
* Revise linking guidelines and AI suggestions @MistEO
