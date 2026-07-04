## v6.14.0-beta.1

### Highlights

#### 莫奈取色与截图增强

新增背景莫奈取色，可根据背景图自动生成主题色，也支持手动选色；同时在设置指引与开始唤醒中补充截图增强与截图测试入口，MuMu 截图增强也已支持 `emulator-5xxx` 格式端口。

#### 模拟器兼容性提示更完整

新增当前模拟器帧率检测与提示，补充 MuMu 后台保活检测，以及雷电模拟器搭配 MaaTouch 时的组合警告，帮助更快定位截图异常、操作异常与性能设置问题。

#### 多项识别与交互问题修复

修复基建产物收取时被 loading 遮挡导致跳过、自定义基建配置列表显示异常、莫奈取色关闭后切换主题异常，以及多项肉鸽事件、选项与外服模板识别问题。

<details>
<summary><b>English</b></summary>

#### Monet Theming and Screenshot Enhancements

Added background Monet theming with both automatic color extraction and manual color selection. Screenshot enhancement and screenshot test entries are now surfaced in setup guidance and startup wake-up, and MuMu screenshot enhancement now supports `emulator-5xxx` style ports.

#### Better Emulator Compatibility Guidance

Added emulator frame rate detection and warnings, MuMu background keep-alive detection, and a warning for the LDPlayer + MaaTouch combination, making screenshot, input, and performance issues easier to diagnose.

#### Multiple Recognition and UI Fixes

Fixed infrastructure collection being skipped when blocked by loading overlays, abnormal display of custom infrastructure configuration lists, theme switching after disabling Monet theming, and several roguelike event, option, and overseas template recognition issues.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.14.0-beta.1 (2026-07-04)</b></summary>

### 新增 | New

* 新增背景莫奈取色，支持根据背景图自动生成主题色，也支持手动选择自定义颜色 ([#17242](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17242)) @ABA2396
* 新增更新器进度窗口显示开关，并补充自动下载更新包提示文本 @ABA2396
* 新增当前模拟器帧率检测与提示，可识别过低、非 60 FPS 与异常高帧率设置 ([#17219](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17219)) @ABA2396
* 新增 MuMu 后台保活检测，连接后可提示可能导致截图与操作异常的后台保活设置 ([#17241](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17241)) @ABA2396
* 新增同时使用雷电模拟器 + MaaTouch 组合的警告 ([#17238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17238)) @ABA2396
* 设置指引与开始唤醒中新增截图增强与截图测试相关选项 ([#17247](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17247)) @ABA2396
* MuMu 截图增强新增支持 `emulator-5xxx` 格式端口 ([#17255](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17255)) @ABA2396
* 繁中服新增「衛戍協議：盟約」小玩法模板支持 ([#17257](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17257)) @momomochi987

### 改进 | Improved

* 优化莫奈取色逻辑，改进主题色板生成与对比度表现 ([#17243](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17243)) @ABA2396
* 禁用 Xe-LPG+ Arrow Lake Arc 140T 的 GPU 推理选项，避免相关机型出现识别异常 @ABA2396
* 修复未开启系统通知时仍执行系统通知检查的问题 @ABA2396

### 修复 | Fix

* 修复 MuMu 模拟器下第 32 个及以后多开实例的编号计算错误 ([#17112](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17112)) @ABA2396
* 修复 DBNet UnClip 多边形偏移实现，提升 NCNN OCR 结果与 fastdeploy 的一致性 ([#17227](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17227)) @Aliothmoon
* 修复取消勾选莫奈取色后切换界面主题异常 ([#17249](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17249)) @ABA2396
* 修复基建产物收取时因 loading 遮挡导致跳过的问题 ([#17232](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17232)) @ZiyinLin @status102
* 修复自定义基建配置列表显示异常 ([#17254](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17254)) @ABA2396
* 修复萨米肉鸽「特里蒙旅行社特派团」识别错误 @Saratoga-Official
* 修复肉鸽事件与选项中的问号、空格、重复项及相似项锚定问题，统一多项事件名识别 ([#17256](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17256)) @ABA2396 @Constrat
* YostarEN/JP/KR/繁中服补充与修正萨米肉鸽大量事件/选项识别，YostarEN 额外新增一批 MASS 事件选项映射 @Constrat
* YostarEN 修复岁相肉鸽 `MissionFailedFlag2` 模板变更后的识别问题 @Constrat

### 其他 | Other

* 优化 MAAUnified 构建流程，复用 MaaCore 构建产物 ([#17233](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17233)) @GhostKiller127

</details>
