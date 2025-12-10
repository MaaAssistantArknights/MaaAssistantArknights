## v6.0.0-beta.1

### Highlights

#### .NET 10 升级

MAA 已全面升级至 .NET 10，带来更好的性能和更现代化的开发体验。并将第三方依赖移至子文件夹，在保持目录清爽的同时有效减少更新包的体积。

#### 关卡导航与小游戏优化

重构了关卡导航 API，现在小游戏列表可以通过 API 动态获取，支持配置小游戏开放时间，使小游戏功能更加灵活易用。

----

#### .NET 10 Upgrade

MAA has been fully upgraded to .NET 10, bringing better performance and a more modern development experience. Third-party dependencies have been moved to a subfolder, effectively reducing update package size while keeping the directory clean.

#### Stage Navigation and Mini-game Optimization

Refactored the stage navigation API. Mini-game lists can now be dynamically retrieved through the API, with support for configuring mini-game opening times, making the mini-game feature more flexible and user-friendly.

----

以下是详细内容：

## v6.0.0-beta.1

### 新增 | New

* LocalizationHelper 支持 TryGetString 与 HasTranslation @hguandl
* 小游戏列表通过 API 获取，支持配置开放时间 (#14997) @ABA2396

### 改进 | Improved

* 重构简化关卡导航 API (#14997) @ABA2396
* 优化自动编队助战页面继承 @ABA2396
* 优化自动编队缺失干员查找 @ABA2396
* 优化自动编队编入助战后缺失干员查找 @ABA2396

### 修复 | Fix

* 在已开启更新日志的情况下点击更新日志窗口不会移至前台 @hguandl
* 自动编队当干员存在于多个作业组且已经被编入后可能被误标记为未持有 @ABA2396
* 自动编队编入助战后未添加至干员-干员组映射 @ABA2396
* 因延迟进入线索传递界面导致任务出错 (#14992) @ABA2396
* 夜间 OTA dotnet 构建错误 (#14996) @hguandl

### 构建 | Build

* 升级至 .NET 10 (#14971) @hguandl @ABA2396
* 取消单文件发布，第三方依赖移至子文件夹，减少更新包体积 (#14984) @hguandl

### 其他 | Other

* CI: 更新 github-actions 依赖 (#14978) @dependabot
* 移除不再使用的 report @hguandl
* YostarKR CharsNameOcrReplace OCR 编辑 @ABA2396
