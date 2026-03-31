## v6.7.0

### 新增 | New

* DeepSleep @ABA2396
* 成就 DLC #3 (#16093) @ABA2396 @Constrat @HX3N
* 自动战斗滑块动画效果 @ABA2396
* 添加 local-install.bat 将构建产物打包到 install 目录用于本地生成类似正式版的测试安装包 @ABA2396
* 任务设置按钮右键新增单次运行功能 @ABA2396

### 改进 | Improved

* 使用DateTimeOffset替代DateTime @status102
* Log头时间从UTC时间改为Local时间 @status102
* 不再默认启用`在下拉框中隐藏当日未开放关卡` @status102
* Ocr内部log在without_det时也对rect输出进行基于base_roi的偏移 @status102
* TaskNameDisplay @status102
* 统一样式 @ABA2396
* UserDataUpdate任务 时间存取优化 @status102
* 仓库数据存储优化 @status102
* Revert "rft: 重构battle_data" @status102
* 重构battle_data @status102
* 仓库信息存储避免将 JsonObject 以 string 格式存放在上层 JsonObject中 @status102
* 优化日志展示效果 @ABA2396
* 在收到对应识别结果时再重置库存数据 @ABA2396
* 优化自动战斗 tab 样式，避免切页重置状态 @ABA2396
* Revert "perf: 优化自动战斗 tab 样式，避免切页重置状态" @ABA2396
* 简化自动战斗tab绑定 @status102
* 优化自动战斗 tab 样式，避免切页重置状态 @ABA2396
* colorful sleep @ABA2396

### 修复 | Fix

* 前景色 @ABA2396
* KR update Dorm template @HX3N
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
* 亮色模式下开始按钮样式缺失 @ABA2396
* 在更新时报错无法显示本地化语言 @ABA2396
* 粘贴作业集不会自动勾选多作业模式 @ABA2396
* 添加任务按钮半透明效果 @ABA2396

### 文档 | Docs

* Auto Update Changelogs of v5.7.0-beta.1 (#16107) @github-actions[bot]
* 版本号错了 @ABA2396
* v6.7.0-beta.2 @ABA2396
* Update CHANGELOG for v6.7.0-beta.2 release @ABA2396
* Update CHANGELOG for v6.7.0 with DeepSleep integration @AnnAngela
* changelog 成就 DLC @ABA2396
* v6.7.0 changelog @ABA2396
* Update CHANGELOG with recent changes @ABA2396
* Auto Update Changelogs of v6.7.0-beta.3 (#16121) @github-actions[bot]

### 其他 | Other

* deps update @SherkeyXD
* Release v6.7.0-beta.3 (#16120) @ABA2396
* Release v6.7.0-beta.2 (#16108) @ABA2396
* Release v6.7.0-beta.1 (#16110) @ABA2396
* Release v5.7.0-beta.1 (#16106) @ABA2396
* 缓存结果 @ABA2396
* DateTimeOffset基础支持 @status102
* Revise linking guidelines and AI suggestions @MistEO
* EN @Constrat
* 修复错误描述: `过期关卡重置` @status102
* 图图漏了 @status102
* revert resource @status102
* 手动输入关卡支持 OF-1 与 OF-F3 @ABA2396
* Revert "perf: 简化自动战斗tab绑定" @ABA2396
* 更新数据任务禁用高级设置 @ABA2396
* 将是否启动多作业模式逻辑放到 ParseCopilotAsync 中 @ABA2396
