## v6.17.0-beta.5

### Highlights

#### 黑流树海肉鸽适配

新增适配黑流树海肉鸽，支持刷等级、刷源石锭、刷襁褓动物三个策略。

#### 启动文件缺失检查

启动时比对安装文件完整性，发现文件缺失时提示，并支持从更新源重新下载完整包自动修复。

#### 移除掉线重连，通宵挂机改用定时启动

掉线重连的恢复链状态复杂且维护成本高，已移除；通宵挂机场景请改用定时启动与强制定时启动。

#### 界面过渡动画与操作体验优化

主界面页签、任务链等内容切换加入方向性过渡动画，设置页导航改为平滑滚动；自定义下拉交互统一（点外部关闭、连击保持）。

<details>
<summary><b>English</b></summary>

#### BlackFlow Roguelike

Added support for the BlackFlow (黑流树海) roguelike theme, with the level-farming, Originium Ingot investment, and cultivation strategies.

#### Startup File Integrity Check

On startup, installed files are now verified against the file list; missing files are reported and can be repaired automatically by re-downloading the full package from the update source.

#### Reconnect Removed in Favor of Scheduled Startup

The reconnect-after-disconnect logic has been removed due to the complexity and maintenance cost of restoring the chain state; for overnight sessions, please switch to scheduled startup and forced scheduled startup instead.

#### UI Transition Animations and Interaction Polish

Main tabs and task-chain switching now animate with directional transitions, and settings-page navigation scrolls smoothly; custom dropdown interactions are unified (click outside to close, repeated clicks keep open).

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.17.0-beta.5 (2026-08-21)</b></summary>

### 新增 | New

* 页签切换过渡动画与设置页导航平滑滚动：主界面页签、任务链、常规/高级设置等内容切换加入方向性过渡动画，设置页导航平滑滚动并跟随过渡动画档位 ([#17799](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17799)) @ABA2396

### 改进 | Improved

* 统一自定义下拉控件的开合交互（点外部关闭、连击保持），｢背景设置｣ ｢神秘代码｣ 的文件选择下拉迁移到新 TreeComboBox 控件，并修复弹层滚动导致设置页跳顶 ([#17813](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17813)) @ABA2396
* YostarEN/JP/KR update MiniGame SPA S2 templates ([#17808](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17808)) @Constrat @Manicsteiner @HX3N

### 修复 | Fix

* 修复从 PC 端切回模拟器需要重启后才能连接 @ABA2396
* 修复自动战斗部署时未能正确移除目标地块的过往干员 @status102
* 调整黑流肉鸽 CloseCollectionContinue 判定 ROI 并下调阈值 ([#17779](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17779)) @ZiyinLin @status102
* 修复像素画英文界面按钮显示不全、切换语言时适配与抖动下拉框内容未实时切换 @ABA2396
* 可搜索 ComboBox 换源统一由扩展接管，修复肉鸽开局干员中间值写回产生的重复日志并本地化提示 @ABA2396
* YostarKR update Roguelike@TraderRandomShoppingConfirm template @HX3N

</details>

<details>
<summary><b>v6.17.0-beta.4 (2026-08-19)</b></summary>

### 改进 | Improved

* 可搜索 ComboBox 启用 UI 虚拟化，减轻肉鸽开局干员等大列表搜索时的卡顿 @ABA2396

### 修复 | Fix

* 修复黑流树海策略完成后无法正确停止并上报、未完成时无法重开下一局的问题 ([#17771](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17771)) @ZiyinLin
* 修复可搜索 ComboBox 初始化时将已绑定值覆盖为列表第一项，导致肉鸽开局干员重启后被重置的问题 @ABA2396
* 调整黑流树海左下角放大镜判定方法与阈值，避免部分设备下遇到流程无法继续 @ABA2396

</details>

<details>
<summary><b>v6.17.0-beta.3 (2026-08-18)</b></summary>

### 新增 | New

* CustomWebhook 预置模板新增企业微信（WeCom）与 ntfy 选项 ([#17695](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17695)) @H2O-MERO
* 繁中服新增「辭歲行」活动关卡导航，并适配 SSS 10 ｢極寒安保派駐｣ ([#17766](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17766)) @momomochi987 @ABA2396

### 改进 | Improved

* 自动战斗自动编队按干员组最低练度跳过浏览低等级干员，缩短编队耗时 ([#17751](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17751)) @status102
* YostarKR normalize BlackFlow roguelike terminology and fix mistranslations @HX3N

### 修复 | Fix

* 修复从公招的选择招募时限界面开始自动公招功能，会触发循环操作的问题 @ABA2396
* 修复可搜索 ComboBox 在语言切换等场景下选项绑定失效、指定材料被清空，以及下拉列表滚轮一次滚到底的问题 ([#17759](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17759)) @status102 @ABA2396
* 修复自动战斗作业保存时输出多余超时参数、编队反复切换职业的问题 @status102
* 修复肉鸽开局干员提示无法随界面语言实时切换的问题 @ABA2396
* YostarKR fix Roguelike recruitment giving up instead of recruiting @HX3N

### 文档 | Docs

* 肉鸽文档补充黑流树海推荐开局 ([#17753](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17753)) @Rbqwow @ABA2396

</details>

<details>
<summary><b>v6.17.0-beta.2 (2026-08-18)</b></summary>

### 新增 | New

* 新增适配黑流树海肉鸽，支持 ｢刷等级，快速飞三层｣ ｢刷源石锭，投资完成后自动退出｣ ｢刷襁褓动物｣ 三个策略 ([#17380](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17380)) @DavidWang19 @ZiyinLin @sakevel @status102 @walkerljy @ABA2396
* 新增启动文件缺失检查，发现安装文件缺失时提示，并支持从更新源重新下载完整包自动修复 ([#17725](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17725)) @ABA2396
* 奇象巡展发现未收录奇象时发送通知，并简化通知文案 ([#17744](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17744)) @LengmoAngel @ABA2396
* 自动战斗支持指定职业以区分同名干员，并兼容职业大小写 ([#17544](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17544)) @status102
* 自动战斗使用技能支持超时参数 ([#17734](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17734)) @status102
* 库存保持任务支持因理智不足跳过后续任务 ([#17741](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17741)) @ABA2396

### 改进 | Improved

* 移除掉线重连逻辑，恢复链状态复杂且维护成本高；通宵挂机请改用定时启动与强制定时启动 ([#17742](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17742)) @ABA2396
* 查找干员在职业未知时回退到按名称匹配，避免检索失败 ([#17735](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17735)) @yali-hzy
* 刷理智代理倍率识别改用 RGB 颜色匹配，提升识别稳定性 ([#17719](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17719)) @status102
* 保全作业浏览时不自动添加到作业列表 @status102
* 截图耗时 100ms 以上且未启用截图增强时，补充截图优化建议，并优化截图增强报错与设置指引 @ABA2396
* 优化启动设置页提示与自动战斗缺少干员时的报错描述 @ABA2396
* 繁中服补全并调整界园肉鸽通宝权重 ([#16306](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16306)) @travellerse
* 繁中服更新界园肉鸽干员管理入口模板 ([#16634](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16634)) @abmcar
* 打包时用原生启动器替换 MAA.exe 的 apphost，MAA.exe 被单独移动等文件不完整的情况能给出明确提示，不再误报未安装 .NET ([#17727](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17727)) @ABA2396
* 切换客户端后断开现有连接 @ABA2396

### 修复 | Fix

* 修复自定义 Webhook 无法通过 Headers 设置 Content-Type 导致通知发送失败的问题，消息体占位符补全 Json 转义 @ABA2396
* 修复 ｢开始任务：｣ 分隔栏在界面最小化时无法正确添加、Rectangle 无法显示的问题 @ABA2396
* 修复自动战斗技能用法设置失效、待部署等待干员就绪检测阻塞、非自动编队下预分配失败后重复添加干员数据、编入干员分组算法无法比对等问题 @status102 @yali-hzy

### 文档 | Docs

* 补充会客室取下线索的说明 @ABA2396

</details>

<details>
<summary><b>v6.17.0-beta.1 (2026-08-14)</b></summary>

### 新增 | New

* 像素画自动填色新增粘贴功能，支持从剪贴板粘贴图片与 4 字以内的文本 ([#17662](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17662) [#17689](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17689)) @H2O-MERO @ABA2396
* 繁中服启用界园肉鸽 DLC 分队并适配相关参数与 OCR 对照 ([#17705](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17705)) @momomochi987
* MaaCore 新增扩展 C 接口 `AsstCallerExtra` ([#17701](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17701)) @hguandl

### 改进 | Improved

* 多作业模式下导航名改由 Core 自动从地图数据读取，并新增 `nav_name_override` 参数支持手动覆盖导航识别名 ([#17687](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17687)) @status102 @hguandl
* PC 端游戏窗口标题按客户端类型解析，连接与结束模拟器支持不同语言的 PC 端，并优化 PC 端相关描述文案 ([#17679](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17679)) @HX3N @ABA2396
* 显卡兼容性提示在每次开始运行时输出，避免被任务日志刷掉后无法看到 @ABA2396
* 调整 core 崩溃后和未知异常的错误提示 @ABA2396
* 为软件更新包下载添加重试 ([#17675](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17675)) @bkzzzz
* 自动战斗自动编队切换职业时切换回全部职业分类，避免游戏未重置 UI 位置 @status102
* 繁中服调整部分干员与关卡名称 OCR ([#17703](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17703)) @momomochi987
* YostarJP OCR fixes ([#17704](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17704)) @Manicsteiner
* YostarKR update localization with official terminology @HX3N

### 修复 | Fix

* 修复 NotifyIcon 双击间隔为 0 时的启动崩溃 ([#17691](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17691)) @bkzzzz
* 修复连接配置下拉框打开和滚动时整个页面位移的问题 @ABA2396
* 修复 MuMu 触控增强等支持划火柴的触控模式下，划火柴模式开关参数未正确传递生效的问题 ([#17652](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17652)) @Rbqwow
* YostarKR handle startup notification during account switch @HX3N

### 文档 | Docs

* 更新新手入门文档，简化下载安装步骤并说明日志包生成方式 @ABA2396
* 修正多语言文档错别字，更新作业协议 `nav_name_override` 字段说明 ([#17708](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17708)) @apricity093 @hguandl

### MaaMacGui

#### 新增 | New

* 支持作业集 @hguandl
* 支持奇象巡展像素画 @hguandl

#### 改进 | Improved

* 调整像素画选项文案 @hguandl

</details>
