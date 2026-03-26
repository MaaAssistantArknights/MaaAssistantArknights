## v6.7.0-beta.1

### 新增 | New

* 成就 DLC #3 (#16093) @ABA2396 @Constrat
* DeepSleep @ABA2396

### 改进 | Improved

* 使用DateTimeOffset替代DateTime @status102
* Log头时间从UTC时间改为Local时间 @status102
* 不再默认启用`在下拉框中隐藏当日未开放关卡` @status102
* Ocr内部log在without_det时也对rect输出进行基于base_roi的偏移 @status102
* TaskNameDisplay @status102

### 修复 | Fix

* 修复 build waring，smtp 改用新版本 MailKit @ABA2396
* 剿灭关卡名OcrReplace @status102
* 在非 UI 线程调用清空库存数据导致任务添加失败 @ABA2396
* 日志记录中使用源石TaskName @status102
* 任务开始/完成 无法显示任务名 @ABA2396
* 更新 MaaFramework 文件名格式和 .NET SDK 版本 @AnnAngela
* 移除过时的配置迁移兼容逻辑 @status102
* 有猪删多了 @ABA2396
* 剿灭战斗完成roi @status102
* 修复CN剿灭后识别, 并统一全客户端类型识别 @status102

### 文档 | Docs

* changelog 成就 DLC @ABA2396
* v6.7.0 changelog @ABA2396
* Auto Update Changelogs of v5.7.0-beta.1 (#16107) @github-actions[bot]

### 其他 | Other

* 缓存结果 @ABA2396
* DateTimeOffset基础支持 @status102
* Revise linking guidelines and AI suggestions @MistEO
* EN @Constrat
* 修复错误描述: `过期关卡重置` @status102
* 图图漏了 @status102
* revert resource @status102
