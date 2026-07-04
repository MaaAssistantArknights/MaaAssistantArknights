## v6.13.1-alpha.1 (2026-07-04)

### Highlights

#### 模拟器兼容性与主题体验改进

新增模拟器帧率检测、MuMu 后台保活检测与更多截图增强指引，帮助排查截图慢、操作异常和低帧率问题；同时新增莫奈取色，可根据背景图自动生成界面配色，并补充更新提示与进度窗口相关选项。

<details>
<summary><b>English</b></summary>

#### Better Emulator Compatibility and Theme Experience

Added emulator FPS detection, MuMu keep-alive detection, and more screenshot-enhancement guidance to help diagnose slow screencaps, unstable actions, and low frame rate issues. Also introduced Monet color extraction for background-based theme colors, plus clearer update prompts and progress window options.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.13.1-alpha.1 (2026-07-04)</b></summary>

### 新增 | New

* 新增莫奈取色，可根据背景图自动提取主色生成主题配色，并支持自定义颜色 ([#17242](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17242) [#17243](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17243) [#17249](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17249)) @ABA2396
* 新增模拟器帧率检测与提示，帮助识别后台降帧、低帧率设置和插帧问题 ([#17219](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17219)) @ABA2396
* 新增 MuMu 后台保活检测提示，帮助排查截图失败和操作异常 ([#17241](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17241)) @ABA2396
* 设置指引与开始唤醒中新增截图增强和截图测试相关选项 ([#17247](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17247)) @ABA2396
* 新增隐藏更新进度提示框选项，并补充自动下载更新包的提示说明 ([#17254](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17254) [#17232](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17232)) @ABA2396 @Rin @status102
* 新增同时使用雷电模拟器与 MaaTouch 的警告提示 ([#17238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17238)) @ABA2396
* 繁中服新增「衛戍協議：盟約」小游戏资源支持 ([#17257](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17257)) @momomochi987

### 改进 | Improved

* MuMu 截图增强支持 `emulator-5xxx` 格式端口连接，兼容更多连接方式 ([#17255](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17255)) @ABA2396
* 禁用 Xe-LPG+ Arrow Lake Arc 140T 的 GPU 推理选项，避免缺字等识别异常 ([#17246](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17246) [#17245](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17245)) @ABA2396
* YostarEN/JP/KR/txwy add MASS choice encounters support for Roguelike @Constrat

### 修复 | Fix

* 修复 NCNN OCR 在部分文本上的检测框偏移问题，提升小字识别稳定性 ([#17227](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17227)) @Aliothmoon
* 修复 MuMu 模拟器第 32 个及以后多开实例的编号计算错误 ([#17112](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17112)) @dikxingmengya @ABA2396
* 修复未开启系统通知时仍执行通知检查的问题 @ABA2396
* 修复自定义基建配置列表显示异常 ([#17254](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17254)) @ABA2396
* 修复基建产物收取时被 Loading 遮挡而跳过的问题 ([#17232](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17232)) @Rin @status102
* 修复萨米肉鸽「特里蒙旅行社特派团」识别错误 @萨拉托加
* 修复多服肉鸽事件与选项识别错误，统一问号等文本匹配，并清理重复或过期条目 ([#17256](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17256)) @uye @Constrat

</details>

<details>
<summary><b>v6.13.0 (2026-06-28)</b></summary>

### 新增 | New

* Android OCR 推理引擎切换为 NCNN ([#17133](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17133)) @Aliothmoon
* 切换界面语言不再需要重启，干员识别、仓库识别等界面同步支持动态切换，并可单独设置干员名称显示语言 ([#17183](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17183)) @ABA2396
* 新增可通过手动修改配置文件开启 6 星自动公招，将「是否招募」和「招募时间」选项移入常规设置，优化 ToolTip 提示 ([#17154](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17154)) @ABA2396
* Custom Webhook 新增预置模板功能 ([#17081](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17081)) @pboymt
* 新增常驻关卡备选提示，当选中的关卡为常驻关卡或当前/上次时提示其后关卡不会被选中执行 @ABA2396
* 增加 DWM 被禁用与重复拖动异常的解决方案提示 @ABA2396
* 添加落叶逐火复刻关卡入口任务 @SherkeyXD
* Yostar 服新增 SSS#10 极寒自动战斗作业 ([#17137](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17137)) @Manicsteiner

### 改进 | Improved

* 统一任务队列与自动战斗的停止逻辑 ([#17087](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17087)) @ABA2396
* 无法显示系统通知时自动回退到软件内通知，启动时通知不可用提示改为软件内弹窗 ([#17165](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17165)) @ABA2396
* 优化成就 DLC 标识显示效果与提示，增加对应 DLC 上线时间提示 @ABA2396
* 调整繁中服 CharNameOcrReplace 替换规则 ([#17113](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17113)) @momomochi987
* 怪猎一期复刻 CF-EX-8、CF-S-1 关卡参数调整 @status102
* 移除依赖库安装脚本的提权操作 @ABA2396
* 调整日志超时提示 @ABA2396
* YostarKR crop JieGarden@Roguelike@StartAction.png to improve match score @HX3N

### 修复 | Fix

* 修复 NCNN OCR 引擎在小 ROI 下识别异常，对齐 fastdeploy 的 det 缩放与 rec 预处理 ([#17182](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17182)) @Aliothmoon
* 修复部分设备界园树洞"是非境"识别错误 ([#17181](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17181)) @Aliothmoon
* 修复自动战斗多作业模式无法进行关卡导航 @status102
* 修复自动战斗导航 retry 异常导致跳过作业的问题 @status102
* 修复肉鸽战斗结束后招募误入 StartExplore 的问题 @Saratoga-Official
* 修复水月肉鸽"大海的遗产"和"狗眼婆娑"事件名识别错误 @Saratoga-Official
* 修复肉鸽 GetDrop 未等待 LoadingText 导致的问题 @Saratoga-Official
* 修复 CloseEvent 和 CloseCollection 同时出现导致的异常 @Saratoga-Official
* 修复 MAA 更新检查 API 请求未遵循代理设置的问题 @status102
* 修正落叶逐火入口 OCR 文本 @SherkeyXD
* 修复复制任务时未保留原任务启用状态的问题 @ABA2396
* YostarEN fix future alter operators recognition and EnterEpisodeNew-Click @Constrat
* YostarKR correct ClickChapterNewDefaultProgress roi, expand training roi for wrapped operator names @HX3N

### 其他 | Other

* 修复 MAAUnified CI 选错 ref 的问题 ([#17143](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17143)) @Halo
* 优化 Avalonia 构建工作流触发条件 ([#17139](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17139)) @Manicsteiner
* YostarEN/JP/KR update LoneTrail and JieGarden themes ([#17175](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17175)) @Constrat @Manicsteiner @HX3N
* YostarJP add episode new navigation and JieGarden theme recruit @Manicsteiner
* Update GitHub push action version comment ([#17124](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17124)) @AnnAngela

</details>
