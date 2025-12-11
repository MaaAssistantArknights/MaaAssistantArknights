## v6.0.0

### 新增 | New

* LocalizationHelper 支持 TryGetString 与 HasTranslation @ABA2396
* 第三方依赖移至子文件夹 (#14984) @ABA2396

### 改进 | Improved

* 重构简化关卡导航 api，小游戏列表通过 api 获取 (#14997) @ABA2396
* 自动编队助战页面继承优化 @status102
* 自动编队编入助战后缺失干员查找 @status102
* 自动编队缺失干员查找优化 @status102
* 使用LibraryImport替换部分DllImport @status102

### 修复 | Fix

* YostarJP Roguelike StagrTrader (#15047) @Manicsteiner
* 自动编队助战模块retry_times @status102
* 助战干员进入详情页后flag检测retry @status102
* YostarKR Roguelike@StageTrader UI updates @HX3N
* 自动编队助战干员组名存储错误 @status102
* 在已开启更新日志的情况下点击更新日志窗口不会移至前台 @ABA2396
* import @status102
* fix nightly ota dotnet build (#14996) @Manicsteiner
* 自动编队编入助战后未添加至干员-干员组映射 @status102
* 自动编队当干员存在于多个作业组且已经被编入后可能被误标记为未持有 @status102
* 因延迟进入线索传递界面导致任务出错 @ABA2396
* 资源更新创建 ToolTip 失败 @ABA2396
* 自动编队助战切换职业retry @status102
* 多活动同时开放时提示可能被错误折叠 @ABA2396

### 文档 | Docs

* add instruction of log view in vsc-ext (#14696) @NtskwK @pre-commit-ci[bot]
* Generate v6.0.0-beta.1 changelog following project conventions (#15003) @Copilot @ABA2396 @ABA2396 @ABA2396 @ABA2396
* Auto Update Changelogs of v6.0.0-beta.2 (#15022) @github-actions[bot] @ABA2396 @ABA2396 @github-actions[bot] @Copilot @ABA2396
* 更新文档中的 .NET 版本至 10 (#15023) @wryx166
* 更新安全策略版本 @SherkeyXD

### 其他 | Other

* Revert "fix: 助战干员进入详情页后flag检测retry" @status102
* YostarKR ocr edits for FM @HX3N
* YostarJP ocr (#15026) @Manicsteiner
* Release v6.0.0-beta.2 (#15021) @ABA2396
* Release v6.0.0-beta.1 (#15001) @ABA2396
* 移除不再使用的report @status102
* YostarKR CharsNameOcrReplace ocr edit @HX3N
* Revert "chore: 隐藏 .deps.json 与 .runtimeconfig*.json" @ABA2396
* 隐藏 .deps.json 与 .runtimeconfig*.json @ABA2396
