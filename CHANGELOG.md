## v6.16.3

### Highlights

#### 适配新代理倍率

游戏代理倍率上限已提升至 10 倍。本版本全面适配新的倍率列表界面与识别逻辑，支持最高 10 倍连战，并移除此前因未适配而临时锁定的限制，AUTO 与手动倍率切换可正常使用。

#### 背景选择器增强

背景设置支持树形结构选择与缩略图预览，自定义界面更方便。

#### 更新后自动运行可控

新增「更新后立即重启时不自动运行」选项；启动自动运行任务或模拟器前增加 10 秒倒计时确认，避免更新重启后非预期地直接开跑。

#### 库存保持任务增强

库存保持任务 UI 重构：新增理智药/源石全局开关与临期药支持，并提供芯片、龙门币、采购凭证、技巧概要等刷图预设，计划管理更方便。

<details>
<summary><b>English</b></summary>

#### New Series Multiplier Support

In-game series (proxy) multiplier cap is now up to 10x. This version fully adapts to the new series list UI and recognition logic, supports up to 10x consecutive battles, and removes the temporary lock used before adaptation so AUTO and manual multiplier switching work normally again.

#### Enhanced Background Picker

Background settings now support a tree-style picker with thumbnail previews for easier customization.

#### Controllable Auto-Run After Update

Added an option to skip auto-run after an immediate post-update restart, plus a 10-second countdown confirmation before automatically starting tasks or the emulator, preventing unexpected runs after update restarts.

#### Enhanced Depot Maintain Task

Refactored the depot maintain task UI with global medicine/originium toggles and expiring-medicine support, plus farming presets for chips, LMD, certificates, and skill summaries for easier plan management.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.16.3 (2026-08-02)</b></summary>

### 改进 | Improved

* 重构库存保持计划项为独立 ViewModel，按索引同步任务配置 ([#17468](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17468)) @status102

### 修复 | Fix

* 修复刷理智选择代理倍率后未关闭次数列表 @status102

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
