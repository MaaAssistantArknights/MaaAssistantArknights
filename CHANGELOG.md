## v6.0.0-beta.2

### Highlights

#### .NET 10 升级

MAA 已升级至 .NET 10，带来性能与开发体验改进；取消单文件发布同时将第三方依赖移至子文件夹，在保持目录清爽的同时有效减少更新包的体积。 (PR #14971, #14984) @SherkeyXD @ABA2396

#### 关卡导航与小游戏优化

重构关卡导航 API，小游戏列表可通过 API 动态获取并支持配置小游戏开放时间，增强灵活性与可配置性。 (PR #14997) @ABA2396

----

#### .NET 10 Upgrade

MAA has been upgraded to .NET 10, improving performance and the developer experience. Cancel the single file release and move third-party dependencies to subfolders, effectively reducing update package size while keeping the directory clean. (PR #14971) @SherkeyXD @ABA2396

#### Stage Navigation and Mini-game Optimization

Refactored the stage navigation API. Mini-game lists can be retrieved via API and opening times configured, improving flexibility. (PR #14997, #14984) @ABA2396

#### EN (Global) IS Fix (v6.0.1)

Quick Fix for EN (Global) IS in the Stage Trader  
Quick Fix for EN (Global) RA2 post crafting  
Translation for SSS#8 copilot jobs  
Fixes to the missions collection post client update

----

以下是详细内容：

## v6.0.0

### 新增 | New

* `LocalizationHelper` 支持 `TryGetString` 与 `HasTranslation` @ABA2396
* 将第三方依赖移至子文件夹以减小更新包体积 (PR #14984) @ABA2396

### 改进 | Improved

* 重构关卡导航 API，小游戏列表通过 API 获取并支持配置开放时间 (PR #14997) @ABA2396
* Roguelike：StageTrader 与 OCR 兼容性与 UI 优化，改进多区域支持 (PR #15047, #15026) @Manicsteiner @HX3N
* 将部分 P/Invoke 从 `DllImport` 切换为 `LibraryImport`，提高跨平台兼容性 @status102

### 修复 | Fix

* 修复 nightly OTA dotnet 构建问题 (PR #14996) @Manicsteiner
* 修复资源更新时创建 ToolTip 失败的问题 @ABA2396
* 修复自动编队/助战模块中 retry 与标记导致的重复或遗漏问题 @status102
* 修复多活动同时开放时被错误折叠的问题 @ABA2396

### 文档 | Docs

* 在 VS Code 扩展中补充日志查看说明 (PR #14696) @NtskwK
* 在文档中更新 .NET 版本说明为 10 (PR #15023) @wryx166

### 其他 | Other

* 移除废弃的右键添加作业集行为 @status102
* 移除不再使用的 `report` 与相关引用 @status102
* 若干 OCR 与文本替换的局部改进 YostarKR / YostarJP @HX3N @Manicsteiner

----

## v6.0.1

### 新增 | New

* 为 MuMu/LDPlayer 添加路径/库检查并补充 i18n 提示 @ABA2396

### 修复 | Fix

* collect rewards for EN updated templates @Constrat
* OCR for RA2 @Constrat
* Update StageTrader for EN IS @Constrat

### 其他 | Other

* 移除不再使用的 report @ABA2396
* YostarKR CharsNameOcrReplace OCR 编辑 @HX3N

----

## v6.0.0-beta.2

### 修复 | Fix

* 资源更新创建 ToolTip 失败 @ABA2396
* 自动编队助战切换职业 retry @status102
* 多活动同时开放时提示可能被错误折叠 @ABA2396

### 文档 | Docs

* 更新文档中的 .NET 版本至 10 (#15023) @wryx166
* 更新安全策略版本 @SherkeyXD
