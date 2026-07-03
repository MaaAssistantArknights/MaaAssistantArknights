## v6.13.1 (2026-07-03)

### Highlights

#### 模拟器兼容与截图体验增强

新增莫奈取色、MuMu / 雷电截图增强与截图测试入口，并增加模拟器帧率、MuMu 后台保活、雷电模拟器 + MaaTouch 组合等运行环境提示；同时修复 OCR 多边形偏移、MuMu 多开编号及通知检查等问题，提升识别稳定性与连接体验。

<details>
<summary><b>English</b></summary>

#### Better Emulator Compatibility and Screenshot Experience

Added Monet color extraction, enhanced screenshot options and screenshot testing for MuMu / LDPlayer, plus new environment warnings for emulator FPS, MuMu background keep-alive, and the LDPlayer + MaaTouch combination. Also fixed OCR polygon offset issues, MuMu multi-instance indexing, and notification checks to improve recognition stability and connection reliability.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.13.1 (2026-07-03)</b></summary>

### 新增 | New

* 新增莫奈取色，可从背景图自动提取主题色，支持自动取色与自定义颜色模式 ([#17242](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17242)) @ABA2396
* 设置指引与开始唤醒中新增 MuMu / 雷电截图增强相关选项，并提供截图测试入口 ([#17247](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17247)) @ABA2396
* 新增模拟器帧率检测与提示，检测到过低或过高帧率时给出建议 ([#17219](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17219)) @ABA2396
* 新增 MuMu 模拟器后台保活检测与提示，帮助排查截图失败和操作异常 ([#17241](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17241)) @ABA2396
* 新增雷电模拟器与 MaaTouch 同时使用时的兼容性警告 ([#17238](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17238)) @ABA2396
* 更新设置中的自动下载更新包提示文本 @ABA2396
* 新增隐藏更新进度提示框选项 @ABA2396

### 改进 | Improved

* 优化莫奈取色逻辑，改进主题配色生成与亮度计算，提升背景取色后的界面观感与对比度 ([#17243](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17243)) @ABA2396
* 禁用 Xe-LPG+ Arrow Lake Arc 140T 的 GPU 推理选项，避免缺字等显示异常 @ABA2396

### 修复 | Fix

* 修复关闭莫奈取色后切换界面主题异常的问题 ([#17249](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17249)) @ABA2396
* 修复 DBNet UnClip 多边形偏移与 fastdeploy 不一致的问题，提升 NCNN OCR 结果稳定性 ([#17227](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17227)) @Aliothmoon
* 修复 MuMu 模拟器下第 32 个及以后多开实例编号计算错误 ([#17112](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17112)) @dikxingmengya @ABA2396
* 修复未开启系统通知时仍进行系统通知检查的问题 @ABA2396

### 其他 | Other

* 优化 MAAUnified 构建流程，复用 MaaCore 制品以缩短 CI 构建时间 ([#17233](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17233)) @Aliothmoon

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
