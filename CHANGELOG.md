## v6.1.0

### Highlights

本次版本更新，我们新增了多项功能，敬请体验：

（别忘了，v6.0.0 以后的版本需要 [.NET 10.0 桌面运行时](https://dotnet.microsoft.com/zh-cn/download/dotnet/thank-you/runtime-desktop-10.0.1-windows-x64-installer?cid=getdotnetcore)方可运行）

#### 新增“隐秘战线”自动游玩支持

我们为牛牛新增了“隐秘战线”的自动游玩功能，支持完整流程控制与识别逻辑，可为你自动刷取“战线任务”奖励。

你可以在【小工具】-【小游戏】处找到相关功能。

#### 外服界园肉鸽适配

这个版本我们完善了 Yostar JP / KR / EN 服务器的界园肉鸽玩法支持。

#### 日志悬浮窗正式上线

我们新增了日志悬浮窗功能，在【一键长草】界面右上角新设按钮、托盘菜单均可快捷切换显示状态，可在指定窗口内实时展示日志，便于运行观测与问题排查。

你也可以右键【一键长草】界面右上角的新设按钮手动指定窗口，这样可以把模拟器和 MAA 最小化的同时，在浏览器、聊天工具等窗口内实时查看运行状态。

#### 更新提示支持点击直接触发更新

标题栏的新版本提示（`检测到新版本`）现已支持点击，可直接触发更新流程，减少操作步骤。

----

In this update, we've added several new features. Please experience them:

(Please remember that versions 6.0.0 and later require the [.NET 10.0 Desktop Runtime](https://dotnet.microsoft.com/en-us/download/dotnet/thank-you/runtime-desktop-10.0.1-windows-x64-installer?cid=getdotnetcore) to run.)  

#### Added Support for farming *Hidden Front*

We've added support for "Hidden Front" in MAA, supporting complete process control and recognition logic, allowing you to automatically farm *Frontline Missions* rewards.

You can find this function under *Toolbox* - *Mini-Games*.

#### Overseas Server *Sui's Garden of Grotesqueries* Compatibility

This version improves support for *Sui's Garden of Grotesqueries* gameplay on Yostar JP/KR/EN servers.

#### Log Floating Window Officially Launched

We've added a log floating window function. The display status can be quickly switched between the new button in the upper right corner of the *Farming* interface and the tray menu. Logs can be displayed in real-time in a designated window for easy monitoring and troubleshooting.

You can also manually specify a window by right-clicking the new button in the upper right corner of the *Farming* interface. This allows you to minimize the emulator and MAA while viewing the running status in real-time in browsers, chat tools, etc.

#### Update Notification Now Clickable to Trigger Update

The new version notification (`New Version Found`) in the title bar is now clickable, allowing you to directly trigger the update process and reduce steps.

----

以下是详细内容：

## v6.1.0

### 新增 | New

* 隐秘战线（Hidden Front）支持 @ABA2396 @Manicsteiner @HX3N @Constrat
* Yostar JP/KR/EN 界园肉鸽支持 @Manicsteiner @HX3N @Constrat
* 支持点击标题栏左上角“检测到新版本”提示直接触发更新 @ABA2396
* 新增极寒保全派驻作业与相关自动战斗支持 @Saratoga-Official @ABA2396
* 新增日志悬浮窗，支持自动战斗日志源与布局优化 (#15185) @ABA2396
* 右键托盘菜单新增日志悬浮窗显示切换 @ABA2396
* WPF 日志系统支持按级别输出日志文件 @status102
* 满线索状态下一键置入线索 @ABA2396
* 移除未使用的模板资源 (#15207) @Constrat
* 更新期间退出应用时增加二次确认提示 (#14964) @Hakuin123 @ABA2396

### 改进 | Improved

* move_camera 允许微调镜头 (#15220) @Daydreamer114
* 任务运行期间，即使组件被禁用也可正常查看 Tooltip (#15186) @yali-hzy
* 模板资源整体重构与体积优化，提升加载与识别效率 @Constrat
* TemplResource 图片查找流程预构建索引，加快匹配速度 (#15092) @status102
* ProcessTask 任务支持命中结果缓存，减少重复计算 (#12651) @status102
* StableHash 算法优化，提升一致性与性能 @ABA2396
* 截图工具统一将来自 src 的截图缩放至目标分辨率 @DavidWang19
* RefreshCustomInfrastPlanIndexByPeriod 支持传入当前时间 @ABA2396
* 悬浮窗整体布局与交互体验优化 @ABA2396
* 使用 boost::regex 替换 std::regex，提高正则性能 (#15126) @MistEO
* Update ko-kr.xaml (#15213) @178619

### 修复 | Fix

* 引航者试炼借助战 @ABA2396
* 修复资源更新错误复制 cache 文件夹的问题 @ABA2396
* 修复更新后未成功下载的 OTA 包未被清理的问题 @status102
* 修复更新界面在无可用 OTA 增量包时提示不正确的问题 @ABA2396
* 修复 MacOS 下 asst.log 被意外自动清空的问题 (#15122) @Alan-Charred
* 修复路径字符串首尾空格与控制字符导致的问题 (#15082) @mayuri0v0
* 修复窗口更新检测按钮在更新期间未禁用的问题 @ABA2396
* 修复日志与 UI 延迟变化导致的误匹配问题 (#15198) @status102
* 修复 ROI 越界与更新工具异常 (#15204) @178619
* 修复自动战斗中借用非好友助战后卡在加好友界面的问题 @ABA2396
* 修复快速编队与模拟器卡顿导致的误点击与异常进入界面问题 @Saratoga-Official
* 修复保全派驻在网络波动下可能无法确认阵容的问题 @ABA2396
* 修复通宝与铜钱识别失败时的异常流程与卡死问题 (#15167 #15180 #15197) @travellerse @HX3N
* 修复多服 OCR / 正则匹配问题，提升识别准确率（EN / JP / KR / 繁中） @Constrat @HX3N @Manicsteiner @momomochi987
* 修复关卡名、掉落、通宝、铜钱、助战、招募等多处模板或识别错误 @ABA2396 @Constrat
* Fixed multiple OCR regex issues for EN servers, improving drop, currency, and stage name recognition @Constrat

### 其他 | Other

* 更新 macos.cmake 构建配置 (#15173) @Alan-Charred
* 更新 C# EditorConfig 以支持 C# 13 / 14 (#15146) @status102
* 更新 pre-commit 与开发工具链配置 @SherkeyXD
* 移除过时的代码检查与格式化配置，简化开发流程 @SherkeyXD
* 悬浮窗移动时不再自动隐藏，避免与部分窗口（如 QQ）交互闪烁的问题 @ABA2396
* continue on error required for blame ignore\npost @Constrat
