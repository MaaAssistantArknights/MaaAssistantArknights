## v6.14.0-beta.2

### Highlights

#### 仓库识别支持更多基础资源

仓库识别现已新增对源石、合成玉、龙门币、赤金与采购凭证的识别支持，导出与库存核对更完整。

#### 关卡导航与肉鸽识别继续增强

自动战斗多作业模式现已支持异体字关卡导航，并为部分活动/支线关卡补充模板导航；同时继续补强繁中服、YostarEN 与 YostarJP 的关卡、肉鸽事件与选项识别。

#### 启动与界面体验优化

新增账号切换启用勾选框，避免不需要时仍执行账号切换；同时优化帧率检查异步流程、主题色持久化与相关界面细节，减少截图返回阻塞、启动闪烁与主题设置异常。

<details>
<summary><b>English</b></summary>

#### More Base Resources Supported in Depot Recognition

Depot recognition now supports Originium, Orundum, LMD, Gold, and Purchase Certificates, making inventory checks and exports more complete.

#### Better Stage Navigation and Roguelike Recognition

Multi-copilot auto-battle now supports template-based navigation for variant stage names, with additional template coverage for some event and side-story stages. Recognition has also been improved further for Traditional Chinese, YostarEN, and YostarJP stages, roguelike events, and options.

#### Startup and UI Experience Improvements

Added an explicit account-switch toggle so account switching no longer runs when unnecessary, while also improving asynchronous frame-rate checks, persisted theme colors, and related UI details to reduce screenshot blocking, startup flicker, and theme-setting issues.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.14.0-beta.2 (2026-07-06)</b></summary>

### 新增 | New

* 自动战斗多作业模式新增支持异体字关卡导航；部分活动与支线关卡新增模板导航，在有模板时可优先使用模板识别、无模板时回退 OCR ([#16984](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16984)) @ABA2396
* 仓库识别新增支持源石、合成玉、龙门币、赤金与采购凭证 ([#17287](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17287)) @ABA2396
* 账号切换新增启用勾选框，可按需关闭账号切换 ([#17280](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17280)) @ABA2396
* 繁中服新增「未許之地」关卡导航支持 ([#17285](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17285)) @momomochi987
* YostarJP adds JieGarden DLC2 roguelike support, including new squad names and a large batch of OCR/recognition mappings ([#17286](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17286)) @Manicsteiner

### 改进 | Improved

* 持久化保存主题色，减少启动时主题闪烁 ([#17263](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17263)) @ABA2396
* 帧率检查改为异步执行，减少截图返回阻塞 ([#17277](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17277)) @ABA2396
* 优化被注入提示文案，提示信息更清晰 ([#17272](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17272)) @ABA2396
* 确认招募时同步更新 UI 日志 Card 图片，界面展示更及时 ([#17268](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17268)) @ABA2396
* YostarEN/JP improve JieGarden and Sami roguelike event/option recognition; YostarEN additionally improves MASS encounter option mappings ([#17261](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17261)) @Manicsteiner @Constrat

### 修复 | Fix

* 修复使用莫奈取色吸管工具后二次打开页面时崩溃的问题 ([#17270](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17270)) @ABA2396
* 修复背景填充模式仍可编辑的问题 @ABA2396
* YostarEN fixes a Varkaris text/accent recognition issue @Constrat

### 文档 | Docs

* 补充截图相关回调文档 @ABA2396

</details>

<details>
<summary><b>v6.14.0-beta.1 (2026-07-04)</b></summary>

### 新增 | New

* 新增背景莫奈取色，支持根据背景图自动生成主题色，也支持手动选择自定义颜色；优化主题色板生成与对比度表现 ([#17242](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17242), [#17243](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17243), [#17249](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17249)) @ABA2396
* 新增更新器进度窗口显示开关，并补充自动下载更新包提示文本 @ABA2396
* 新增当前模拟器帧率检测与提示，可识别过低、非 60 FPS 与异常高帧率设置 ([#17219](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17219)) @ABA2396
* 新增 MuMu 后台保活检测，连接后可提示可能导致截图与操作异常的后台保活设置 ([#17241](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17241)) @ABA2396
* 新增同时使用雷电模拟器 + MaaTouch 组合的警告 ([#17238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17238)) @ABA2396
* 设置指引与开始唤醒中新增截图增强与截图测试相关选项 ([#17247](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17247)) @ABA2396
* MuMu 截图增强新增支持 `emulator-5xxx` 格式端口 ([#17255](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17255)) @ABA2396
* 繁中服新增「衛戍協議：盟約」小玩法模板支持 ([#17257](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17257)) @momomochi987

### 改进 | Improved

* 禁用 Xe-LPG+ Arrow Lake Arc 140T 的 GPU 推理选项，避免相关机型出现识别异常 @ABA2396
* 修复未开启系统通知时仍执行系统通知检查的问题 @ABA2396

### 修复 | Fix

* 修复 MuMu 模拟器下第 32 个及以后多开实例的编号计算错误 ([#17112](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17112)) @ABA2396
* 修复 DBNet UnClip 多边形偏移实现，提升 NCNN OCR 结果与 fastdeploy 的一致性 ([#17227](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17227)) @Aliothmoon
* 修复基建产物收取时因 loading 遮挡导致跳过的问题 ([#17232](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17232)) @ZiyinLin @status102
* 修复自定义基建配置列表显示异常 ([#17254](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17254)) @ABA2396
* 修复萨米肉鸽「特里蒙旅行社特派团」识别错误 @Saratoga-Official
* 修复肉鸽事件与选项中的问号、空格、重复项及相似项锚定问题，统一多项事件名识别 ([#17256](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17256)) @ABA2396 @Constrat
* 繁中服补充与修正萨米肉鸽大量事件/选项识别 @Constrat
* YostarEN/JP/KR: add and fix a large batch of Sami roguelike event/option recognition; YostarEN additionally adds a set of MASS event option mappings @Constrat
* YostarEN: fix `MissionFailedFlag2` template mismatch recognition issue @Constrat

### 其他 | Other

* 优化 MAAUnified 构建流程，复用 MaaCore 构建产物 ([#17233](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17233)) @GhostKiller127

</details>
