## v5.7.0-beta.1

### 新增 | New

* DeepSleep @ABA2396

### 改进 | Improved

* 使用 DateTimeOffset 替代 DateTime @status102
* Log 头时间从 UTC 时间改为 Local 时间 @status102
* 不再默认启用 `在下拉框中隐藏当日未开放关卡` @status102
* Ocr 内部 log 在 without_det 时也对 rect 输出进行基于 base_roi 的偏移 @status102

### 修复 | Fix

* 修复 build waring，smtp 改用新版本 MailKit @ABA2396
* 剿灭关卡名 OcrReplace @status102
* 在非 UI 线程调用清空库存数据导致任务添加失败 @ABA2396
* 日志记录中使用源石 TaskName @status102
* 任务开始/完成 无法显示任务名 @ABA2396
* 更新 MaaFramework 文件名格式和 .NET SDK 版本 @AnnAngela
* 移除过时的配置迁移兼容逻辑 @status102
* 剿灭战斗完成 roi @status102
* 修复 CN 剿灭后识别, 并统一全客户端类型识别 @status102

### 其他 | Other

* DateTimeOffset 基础支持 @status102
* 修复错误描述: `过期关卡重置` @status102
* 图图漏了 @status102
* Revise linking guidelines and AI suggestions @MistEO
