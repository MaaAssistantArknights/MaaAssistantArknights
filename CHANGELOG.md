## v6.13.0

### Highlights

#### 6 星自动公招支持

新增可通过手动修改配置文件开启的 6 星自动公招功能，「是否招募」和「招募时间」选项移入常规设置，并优化了相关提示信息。

#### 通知不可用时自动回退

系统通知无法显示时自动回退到软件内通知，避免用户错过重要提醒；启动时的通知检测提示也改为软件内弹窗，不再仅写入日志。

#### 切换语言无需重启

切换软件界面语言不再需要重启，干员识别、仓库识别等界面同步支持动态切换。

<details>
<summary><b>English</b></summary>

#### 6-Star Auto Recruitment Support

Added a 6-star auto recruitment feature that can be enabled through manual configuration file editing. The "Recruit" and "Recruitment Time" options have been moved to general settings, with improved tooltip hints.

#### Notification Fallback

Automatically falls back to in-app notifications when system notifications are unavailable, ensuring users never miss important alerts. The startup notification availability check now shows an in-app Growl message instead of only logging to file.

#### Switch Language Without Restart

Switching the UI language no longer requires a restart; operator recognition, depot recognition, and other views now support dynamic switching.

</details>

----

以下是详细内容：

<details open>
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
* 裁剪 YostarKR 界园肉鸽模板以提升匹配分数 @HX3N
* 移除依赖库安装脚本的提权操作 @ABA2396
* 调整日志超时提示 @ABA2396

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
* YostarEN 修复未来异格干员识别与章节导航点击 @Constrat
* YostarKR 修正章节导航 ROI、修复训练 ROI 以适配换行的干员名 @HX3N

### 其他 | Other

* YostarEN/JP/KR 更新落叶逐火与界园主题 ([#17175](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17175)) @Constrat @Manicsteiner @HX3N
* YostarJP 新增章节导航与界园主题招募 @Manicsteiner
* 修复 MAAUnified CI 选错 ref 的问题 ([#17143](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17143)) @Halo
* 优化 Avalonia 构建工作流触发条件 ([#17139](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17139)) @Manicsteiner
* Update GitHub push action version comment ([#17124](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17124)) @AnnAngela

</details>
