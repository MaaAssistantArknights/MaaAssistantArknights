## v6.16.0

### Highlights

#### 适配新代理倍率

游戏代理倍率上限已提升至 10 倍。本版本全面适配新的倍率列表界面与识别逻辑，支持最高 10 倍连战，并移除此前因未适配而临时锁定的限制，AUTO 与手动倍率切换可正常使用。

#### 背景选择器增强

背景设置支持树形结构选择与缩略图预览，扩展支持 jpeg / bmp / gif / webp 等更多图片格式，并内置「牛牛表情包」壁纸包，自定义界面更方便。

#### 更新后自动运行可控

新增「更新后立即重启时不自动运行」选项；启动自动运行任务或模拟器前增加 10 秒倒计时确认，避免更新重启后非预期地直接开跑。

<details>
<summary><b>English</b></summary>

#### New Series Multiplier Support

In-game series (proxy) multiplier cap is now up to 10x. This version fully adapts to the new series list UI and recognition logic, supports up to 10x consecutive battles, and removes the temporary lock used before adaptation so AUTO and manual multiplier switching work normally again.

#### Enhanced Background Picker

Background settings now support a tree-style picker with thumbnail previews, more image formats (jpeg / bmp / gif / webp), and a built-in "Niuniu sticker" wallpaper pack for easier customization.

#### Controllable Auto-Run After Update

Added an option to skip auto-run after an immediate post-update restart, plus a 10-second countdown confirmation before automatically starting tasks or the emulator, preventing unexpected runs after update restarts.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.16.0 (2026-08-01)</b></summary>

### 新增 | New

* 适配游戏新代理倍率设置与列表界面，支持最高 10 倍连战，并移除临时锁定限制 ([#17500](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17500)) @status102 @ABA2396
* 背景设置支持树形选择器与缩略图预览，扩展 jpeg / bmp / gif / webp 等图片格式，并内置「牛牛表情包」壁纸包 @ABA2396
* 新增「更新后立即重启时不自动运行」选项，启动自动运行前增加 10 秒倒计时确认 ([#17483](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17483)) @ABA2396
* 新增「直到大地变成一颗酸橙」活动关卡导航 @ABA2396
* 幸运墙 OCR 新增「零号公路」关键词 ([#17494](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17494)) @Copilot

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
