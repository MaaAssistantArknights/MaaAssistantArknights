## v6.13.0-beta.2

### Highlights

#### 6 星自动公招支持

新增可通过手动修改配置文件开启的 6 星自动公招功能，「是否招募」和「招募时间」选项移入常规设置，并优化了相关提示信息。

#### 通知不可用时自动回退

系统通知无法显示时自动回退到软件内通知，避免用户错过重要提醒；启动时的通知检测提示也改为软件内弹窗，不再仅写入日志。

<details>
<summary><b>English</b></summary>

#### 6-Star Auto Recruitment Support

Added a 6-star auto recruitment feature that can be enabled through manual configuration file editing. The "Recruit" and "Recruitment Time" options have been moved to general settings, with improved tooltip hints.

#### Notification Fallback

Automatically falls back to in-app notifications when system notifications are unavailable, ensuring users never miss important alerts. The startup notification availability check now shows an in-app Growl message instead of only logging to file.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.13.0-beta.2 (2026-06-22)</b></summary>

### 修复 | Fix

* 修复自动战斗多作业模式无法进行关卡导航 @status102

</details>

<details>
<summary><b>v6.13.0-beta.1 (2026-06-21)</b></summary>

### 新增 | New

* Android OCR 推理引擎切换为 NCNN (#17133) @Aliothmoon
* Custom Webhook 新增预置模板功能 (#17081) @pboymt
* 新增可通过手动修改配置文件开启 6 星自动公招，将「是否招募」和「招募时间」选项移入常规设置，优化 ToolTip 提示 (#17154) @ABA2396
* 添加落叶逐火复刻关卡入口任务 @SherkeyXD
* Yostar 服新增 SSS#10 极寒自动战斗作业 (#17137) @Manicsteiner

### 改进 | Improved

* 统一任务队列与自动战斗的停止逻辑 (#17087) @ABA2396
* 无法显示系统通知时自动回退到软件内通知 (#17165) @ABA2396
* 移除依赖库安装脚本的提权操作 @ABA2396
* 调整日志超时提示 @ABA2396
* 怪猎一期复刻 CF-EX-8、CF-S-1 关卡参数调整 @status102
* 调整繁中服 CharNameOcrReplace 替换规则 (#17113) @momomochi987
* 裁剪 YostarKR 界园肉鸽模板以提升匹配分数 @HX3N

### 修复 | Fix

* 修复自动战斗导航 retry 异常导致跳过作业的问题 @status102
* 修复多作业模式导航逻辑遗漏 @status102
* 修复肉鸽战斗结束后招募误入 StartExplore 的问题 @Saratoga-Official
* 修复水月肉鸽"大海的遗产"和"狗眼婆娑"事件名识别错误 @Saratoga-Official
* 修复肉鸽 GetDrop 未等待 LoadingText 导致的问题 @Saratoga-Official
* 修复 CloseEvent 和 CloseCollection 同时出现导致的异常 @Saratoga-Official
* 修正落叶逐火入口 OCR 文本 @SherkeyXD
* 修复 MAA 更新检查 API 请求未遵循代理设置的问题 @status102
* YostarKR 修正章节导航 ROI @HX3N

### 其他 | Other

* 修复 MAAUnified CI 选错 ref 的问题 (#17143) @Halo
* Update GitHub push action version comment (#17124) @AnnAngela
* 优化 Avalonia 构建工作流触发条件 (#17139) @Manicsteiner

</details>
