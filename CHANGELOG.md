## v6.1.1

### Highlights

#### 新增隐秘战线（Hidden Front）玩法支持

本次版本新增隐秘战线（Hidden Front）功能。支持完整流程控制与识别逻辑，为相关玩法提供稳定、可用的自动化支持。  
v6.1.1 新增系列任务支持。

#### 外服肉鸽（界园）适配

完善了 Yostar JP / KR / EN 服务器的肉鸽（界园）玩法支持。

#### 日志悬浮窗正式上线

新增日志悬浮窗功能，可实时展示自动战斗相关日志，并支持通过托盘菜单快速切换显示状态，便于运行观测与问题排查。

#### 更新提示支持点击直接触发更新

主界面左上角的新版本提示现已支持点击，用户可直接触发更新流程，减少操作步骤。

----

#### New Hidden Front Support Across All Servers

This release introduces full support for the Hidden Front feature across all servers, with complete flow handling and recognition logic to ensure stable and reliable automation.  
v6.1.1 adds support for a series of new tasks.

#### Overseas Roguelike (JieGarden) Support

Roguelike (JieGarden) support has been improved for Yostar JP / KR / EN servers.

#### Floating Log Window Introduced

A new floating log window is now available, providing real-time visibility into auto-battle related logs. It can be toggled via the system tray for easier monitoring and debugging.

#### Clickable Update Notification in Main Window

The new version indicator in the top-left corner of the main window is now clickable, allowing users to directly trigger the update process for a smoother upgrade experience.

----

以下是详细内容：

### 新增 | New

* 隐秘战线支持系列任务 (#15249) @ABA2396 @HX3N @Constrat @Manicsteiner @momomochi987
* 新增模板路径同步工具 (#15254) @ABA2396 @Constrat @HX3N @momomochi987
* 添加孤星主题配置与模板图 @SherkeyXD
* 繁中服「紅絲絨」活動導航 (#15234) @momomochi987
* YostarEN base templates overhaul @Constrat

### 改进 | Improved

* 优化 ImageCropper 输出 @SherkeyXD
* optimize LoneTrail templates @Constrat

### 修复 | Fix

* 修复自动战斗击杀数识别错误 (#15266) @status102
* 修复大地图技能、撤离按钮及相机偏移问题，连带修复引航者 S6 地图 TN-2 ~ TN-4 view[1] (#15252) @status102
* 修复盲点撤退兜底逻辑 @status102
* 修复自动战斗在未存在 debug/map 路径时无法生成地图截图的问题 @status102
* 修复满线索再放入逻辑 @ABA2396
* EN Sami floor detection regex @Constrat
* Copper regex for Sui Garden EN @Constrat
* bad reference @MistEO

### 其他 | Other

* YostarJP copper regex (#15257) @eyjafjallascomb @Manicsteiner
* YostarJP roguelike JieGarden OCR 优化 @Manicsteiner
* KR tweak NightlyWarning and CheckBeforeReportingIssue @HX3N
* 减少 string_view 滥用 @MistEO
* 统一 `DEBUG_VERSION` @MistEO

----

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

* 有猪把切换主题改炸了 @ABA2396
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
* ClueGiveTo1stConfirm use delay for all client @Constrat

### 其他 | Other

* 调整倍战超过限制提示 @ABA2396
* 更新 macos.cmake 构建配置 (#15173) @Alan-Charred
* 更新 C# EditorConfig 以支持 C# 13 / 14 (#15146) @status102
* 更新 pre-commit 与开发工具链配置 @SherkeyXD
* 移除过时的代码检查与格式化配置，简化开发流程 @SherkeyXD
* 悬浮窗移动时不再自动隐藏，避免与部分窗口（如 QQ）交互闪烁的问题 @ABA2396
* continue on error required for blame ignore\npost @Constrat
