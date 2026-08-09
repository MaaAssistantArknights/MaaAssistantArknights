## v6.16.6

### Highlights

#### 新增像素画自动填色

新增像素画自动填色功能：可将图片转换为 24×24 像素画，通过 Custom 任务在游戏像素画编辑器中自动填色，支持取景、滤镜、抖动与按色分组等转换选项。

#### 适配新代理倍率

游戏代理倍率上限已提升至 10 倍。本版本全面适配新的倍率列表界面与识别逻辑，支持最高 10 倍连战，并移除此前因未适配而临时锁定的限制，AUTO 与手动倍率切换可正常使用。

#### 背景选择器增强

背景设置支持树形结构选择与缩略图预览，自定义界面更方便。

#### 库存保持任务增强

库存保持任务 UI 重构：新增理智药/源石全局开关与临期药支持，并提供芯片、龙门币、采购凭证、技巧概要等刷图预设，计划管理更方便。

<details>
<summary><b>English</b></summary>

#### Auto Pixel Art Filling

MAA can now convert an image into 24×24 pixel art and automatically fill it into the in-game pixel art editor via a Custom task, with options for framing, filters, dithering and color grouping.

#### New Series Multiplier Support

In-game series (proxy) multiplier cap is now up to 10x. This version fully adapts to the new series list UI and recognition logic, supports up to 10x consecutive battles, and removes the temporary lock used before adaptation so AUTO and manual multiplier switching work normally again.

#### Enhanced Background Picker

Background settings now support a tree-style picker with thumbnail previews for easier customization.

#### Enhanced Depot Maintain Task

Refactored the depot maintain task UI with global medicine/originium toggles and expiring-medicine support, plus farming presets for chips, LMD, certificates, and skill summaries for easier plan management.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.16.6 (2026-08-09)</b></summary>

### 新增 | New

* 新增像素画自动填色：支持通过牛杂在游戏像素画编辑器中自动填色，并提供图片转 24×24 像素画的转换管线 ([#17629](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17629)) @ABA2396
* 新增奇象巡展自动探索：自动来回走动寻找未收录生物，遇到已收录生物自动退出战斗继续寻找，遇到未收录生物时停止并交给玩家手动战斗 @ABA2396
* CustomWebhook 预置模板新增 KOOK 频道与私聊选项 ([#17596](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17596)) @墨染

### 改进 | Improved

* 自动战斗额外检查干员技能选择、技能等级与模组所要求的精英化等级，不满足时禁止运行并提示 ([#17448](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17448)) @yali-hzy @status102
* 优化库存保持提示与显示效果：任务日志分段展示库存保持计划与库存充足/不足状态，重算掉落需求时记录库存不足详情 ([#17597](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17597)) @ABA2396
* MuMu 触控增强不可用时自动降级方式由 maatouch 改为 minitouch，提升触控兼容性 @ABA2396
* 更新肉鸽干员招募逻辑，新增予愿安洁莉娜等干员 @Saratoga-Official
* 优化任务日志显示：开始任务日志分段展示，彩虹字动画最多显示 60 秒 @ABA2396
* 统一连接设置与设置指引/开始唤醒中的触控模式提示样式，并优化连接地址与触控模式提示文案 @ABA2396

### 修复 | Fix

* 修复自动战斗期间日志输出未重置停滞计时器导致误报任务卡住的问题 @ABA2396
* 修复 PC 应用宝未处理横屏方向导致显示异常的问题 ([#17343](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17343)) @srdr0p

### 文档 | Docs

* 新增通过 Windows 安全中心阻止 DLL 注入解决方法的 FAQ 文档 @ABA2396
* 补充代理倍率在有回复理智情况下的说明 @ABA2396

</details>

<details>
<summary><b>v6.16.5 (2026-08-06)</b></summary>

### 新增 | New

* 支持拖入资源包更新资源版本，并收窄完整包/OTA 包识别以避免误匹配；拖入检测改为异步避免卡 UI，导入失败时显示提示 ([#17569](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17569)) @ABA2396

### 改进 | Improved

* 更新器进度窗口延后至主窗口退出后再显示，避免与正在退出的 MAA 抢占前台；等待超时后自动弹出窗口并提示等待状态，防止主进程退出卡住时更新器变成不可见进程 @ABA2396

### 修复 | Fix

* 倒计时、关闭模拟器、完成后脚本等非可打断期间阻止自动更新重启 ([#17566](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17566)) @ABA2396
* 修复刷理智未能关闭代理倍率列表的问题 @status102
* 修复悖论自动战斗作业使用本地文件时始终读取临时文件的问题 @status102
* 修复干员 `sortIndex` 缺失时误用默认值 0 导致排序错误的问题 @ABA2396
* 修复中断锁引用计数下溢时钳制操作的竞态，避免误抹合法锁 @ABA2396
* YostarJP fix OCR replace chain mangling operator names (e.g. FEater) and TA single-character rules ([#17516](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17516)) @Ronnoc @Manicsteiner

### 文档 | Docs

* 新增库存保持与更新数据文档，补充手动更新、代理倍率、定时执行与剿灭等说明，并同步各语言文档 ([#17576](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17576)) @ABA2396

</details>

<details>
<summary><b>v6.16.4 (2026-08-04)</b></summary>

### 修复 | Fix

* 修复 Win32IO 竞争条件导致的 `am start` 误判失败，并修正超时路径的异步 I/O 取消与资源释放 ([#17545](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17545)) @ABA2396
* 修复 SplitButton 在亮色模式下显示异常 @ABA2396
* YostarJP fix TA (SS) activity OCR mismatch causing wrong activity navigation @Jason's-Miku

</details>

<details>
<summary><b>v6.16.3 (2026-08-03)</b></summary>

### 改进 | Improved

* 重构库存保持计划项为独立 ViewModel，按索引同步任务配置 ([#17468](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17468)) @status102
* 优化库存保持预设按钮：左侧区域可点击展开下拉，并统一 SplitButton 背景样式 @ABA2396
* 更新进度窗口不再强制置顶，仅在开始更新时前置一次，避免打断全屏游戏或其他操作 ([#17525](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17525)) @ABA2396
* 统一配置与启动设置下拉列表中的删除按钮样式 @ABA2396

### 修复 | Fix

* 修复刷理智选择代理倍率后未关闭次数列表的问题 @status102
* 修复 iOS/PlayCover 基建办公室入口模板阈值过高导致识别失败的问题 ([#17527](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17527)) @Rememorio

</details>

<details>
<summary><b>v6.16.2 (2026-08-02)</b></summary>

### 新增 | New

* 库存保持任务 UI 重构：新增理智药/源石全局开关与临期药支持，并提供芯片、龙门币、采购凭证、技巧概要等刷图预设 ([#17512](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17512)) @ABA2396

### 改进 | Improved

* 优化库存保持界面布局与显示，清空计划时增加确认弹窗 @ABA2396

### 修复 | Fix

* 修复库存保持界面未选择掉落物时切换语言无法正确显示的问题 @ABA2396
* YostarEN fix Mountain OCR regex matching @Constrat

</details>

<details>
<summary><b>v6.16.1 (2026-08-02)</b></summary>

~~MAA不会在周██凌晨更新。如果收到更新提示，请忽略，不要查看更新公告，直到周██。~~

### 改进 | Improved

* 更新后自动运行倒计时弹窗移除关闭按钮，避免误关后仍继续自动运行 @ABA2396

### 修复 | Fix

* 使用 DXGI 适配器 LUID 解析 GPU OCR 设备，避免多显卡环境下绑定错误 GPU；解析失败或执行提供程序不可用时回退 CPU ([#17488](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17488)) @GSY707
* 修复 MuMu display id 在 fallback 时被错误缓存，避免连接时游戏未启动导致截图/触控锁死在错误窗口 @ABA2396
* 修复「直到大地变成一颗酸橙」活动上次战斗关卡未在后三关结束时关卡导航错误 @ABA2396
* 修复 FightTask 在新代理倍率列表下无法指定 7~10 倍的参数校验 @status102

</details>

<details>
<summary><b>v6.16.0 (2026-08-01)</b></summary>

### 新增 | New

* 适配游戏新代理倍率设置与列表界面，支持最高 10 倍连战，并移除临时锁定限制 ([#17500](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17500)) @status102 @ABA2396
* 背景设置支持树形选择器与缩略图预览 @ABA2396
* 新增「更新后立即重启时不自动运行」选项，启动自动运行前增加 10 秒倒计时确认 ([#17483](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17483)) @ABA2396
* 新增「直到大地变成一颗酸橙」活动关卡导航 @ABA2396

### 改进 | Improved

* 增强 MouseWheelHelper：弹层打开时隔离外层页面滚动，避免滚动穿透 @ABA2396
* 库存保持任务先比较库存数量再判断关卡开放状态，已满足目标时直接跳过 @ABA2396
* MuMu / Win32 触控对齐 minitouch 默认延迟，提升点击稳定性 @ABA2396
* 优化自动战斗新活动关卡提示文案 @ABA2396
* 优化库存保持 Item 初始化与作业解析干员属性要求默认值处理 @status102

### 修复 | Fix

* 修复未开启截图增强时 MuMu 后台保活检测失效的问题 @ABA2396
* 修复开启自动检测连接时无法修改 Extra 配置的问题 ([#17480](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17480)) @ABA2396
* 修复更新数据任务仅勾选仓库识别时被跳过的问题 ([#17482](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17482)) @2436238575 @ABA2396
* 修复自动战斗多作业模式超长关卡名显示异常 @ABA2396
* 修复自动战斗添加作业时地图信息不存在的提示错误使用 stageCode 的问题 @status102
* 修复「直到大地变成一颗酸橙」关卡 OCR 可能将 TO 识别为 T0 的问题 @ABA2396
* 修复 BadModules 在注入环境下弹窗崩溃，回退至 Win32 MessageBox @ABA2396
* 修正 PC 端推荐分辨率文案为 1280x720 / 1920x1080 @ABA2396
* YostarKR add BattleQuickFormationClear2 for Vector Breakthrough @HX3N

</details>
