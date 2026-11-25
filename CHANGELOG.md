## v6.23.2

### Highlights

#### 卫戍协议极速模拟支持刷角标

MAA 现支持在国服卫戍协议第 3 期 ｢规程｣ 中自动刷取金色或彩色策略角标。在 ｢小工具｣ 的 ｢牛杂｣ 页签选择 ｢卫戍协议｣，将模式设为 ｢刷角标（完整通关）｣。使用活动默认皮肤，手动放弃已有存档后，从有 ｢极速模拟｣ 入口的活动主界面启动。MAA 会选择尚未达标的策略，购买并部署干员，直到达到设定的胜场数，或所有策略均已达标。

<details>
<summary><b>English</b></summary>

#### Farm strategy badges in Stronghold Protocol

On the CN server, MAA can now farm gold or colored strategy badges in Stronghold Protocol Season 3 (Regulation). In Toolbox, open Stronghold Protocol and select “Farm badges (full clear)”. Use the default event skin, abandon any saved run, and start from the event screen that offers Express Simulation. MAA selects strategies that still need clears and plays until it reaches the requested number of wins or every strategy has earned the target badge.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.23.2 (2026-11-25)</b></summary>

### 修复 | Fix

* 修复黑流树海秘境行商买卖时点击商品偶尔不弹出确认框的问题，改为点击商品名称下方的卡片，确认框未出现时自动补点；交易中止时停止任务并提示具体原因 ([#19409](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19409), [#19407](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19407)) @ZiyinLin
* 修复国服刷理智 ｢代理倍率｣ 设为 AUTO 时，使用理智药后所选倍率可能超过 ｢指定次数｣ 剩余次数的问题 @status102
* 修正界园肉鸽自动战斗作业中引用了未定义干员组的问题 ([#19413](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19413)) @temple-dian
* YostarEN/JP/KR add entry templates for the "PF" minigame @Constrat @Manicsteiner @HX3N
* YostarJP/KR update the localized name of the "PF" minigame @Manicsteiner @HX3N
* YostarJP add an OCR replacement for the Japanese name of 凯尔希·思衡托 @Manicsteiner

### 文档 | Docs

* 更新 ｢卫戍协议｣ 与生息演算 ｢重启锚点｣ 的任务说明，｢卫戍协议｣ 需使用卫戍默认主题 UI，PC 端连接时提示关闭 ｢特殊玩法键位｣ 的快捷键显示，｢重启锚点｣ RA-4 需保持 60 帧 ([#19415](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19415)) @Saratoga-Official

### 其他 | Other

* 升级 HandyControls 至 3.7.0 ([#19416](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19416)) @Aliothmoon
* 新增 PC 端 Issue 反馈模板 ([#19379](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19379)) @Saratoga-Official

</details>

<details>
<summary><b>v6.23.1 (2026-11-23)</b></summary>

### 改进 | Improved

* ｢OS-喀兰贸易技术研发部｣ 小游戏在当前关卡已达成时自动进入下一关，领取奖励后自动继续 ([#19360](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19360)) @Daydreamer114 @ABA2396
* 调整各肉鸽主题辅助干员的招募与晋升优先级，令、稀音、冻河和电弧不再作为开局优先招募 @Saratoga-Official

### 修复 | Fix

* 修复肉鸽自动战斗中待部署区域的召唤物查询部署策略错误的问题 @status102
* YostarJP 修复公招标签 ｢範囲攻撃｣ 偶发被识别为 ｢範囲攻擊｣ 导致同页少识别一个标签的问题 ([#19364](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19364)) @Or1a

### MaaMacGui

#### 新增 | New

* 新增 ｢卫戍协议｣ 小游戏 @hguandl

</details>

<details>
<summary><b>v6.23.0 (2026-11-21)</b></summary>

### 新增 | New

* 国服卫戍协议第 3 期 ｢规程｣ 新增 ｢刷角标（完整通关）｣ 模式，每个策略累计通关 3 次可获金色角标，6 次可获彩色角标。`StrongholdProtocol` 任务接口支持通过 `mode` 选择刷分或刷角标 ([#19325](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19325)) @ABA2396

### 改进 | Improved

* 调整各肉鸽主题的招募与晋升优先级，将塔露拉、克莱门莎、冻河和结城理加入开局优先招募名单 ([#19164](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19164)) @Saratoga-Official
* 同一张截图上，使用相同区域、模型和检测模式的候选任务可复用 OCR 结果。区域识别只处理所需范围的掩码 ([#19300](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19300)) @Aliothmoon
* Android 原生 OCR 共用文本检测模型，在两次识别之间释放空闲内存，K70U 实测常驻内存减少约 240 MB。OCR 线程改由系统调度，并缩短了线程忙等时间 ([#19334](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19334), [#19333](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19333)) @Aliothmoon

### 修复 | Fix

* 修复自动战斗关卡名复核偶发不通过的问题 @status102
* 修复保全派驻抽卡按钮在可用时识别不到的问题，改为按调配图标的启用状态识别 ([#19340](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19340)) @Or1a
* 肉鸽战斗前点击 ｢快捷编队｣ 未打开面板时会自动重试，并修正繁中、日、韩服的按钮点击区域 ([#19322](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19322)) @momomochi987
* 修复更换主题时偶尔找不到 ｢梦乡｣ 的问题 ([#19332](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19332)) @noirz607
* 修复 macOS 系统截图在游戏窗口较小时缩放错误的问题 ([#19327](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19327)) @Or1a
* 修复 Windows 下调试截图文件名的编码错误 ([#19324](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19324)) @youzibigg
* 修复繁中服萨米肉鸽在 ｢密靜林地｣ 因区域名识别错误而无法切换策略的问题 ([#19307](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19307)) @momomochi987
* Update Integrated Strategies relic and encounter names on YostarKR, and handle JieGarden encounters that share a Korean name ([#19310](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19310)) @HX3N
* Ignore spaces when recognizing event sign-in pop-ups on YostarKR @HX3N

### 文档 | Docs

* 在贡献指南和 Issue 模板中补充使用 AI 撰写 PR、提交信息和 Issue 的注意事项 @dantmnf
* 修正繁体中文 README 中 MaaX 的 TypeScript 接口链接 ([#19297](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/19297)) @farisbasha

</details>
