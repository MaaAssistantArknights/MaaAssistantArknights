## v6.17.4

### Highlights

#### 萨米肉鸽招募后探索入口重试

修复萨米肉鸽初始招募结束后首次点击"探索冰原"未生效时无重试机制导致任务直接失败的问题，恢复与其他主题一致的入口重试行为。

#### Mac 触控模式等实例选项即时生效

Mac 客户端切换触控模式（如 `minitouch` ↔ `maatouch`）后下次连接即生效，无需重启应用。

<details>
<summary><b>English</b></summary>

#### Sami Roguelike Exploration Entry Retry

Fixes the issue where Sami roguelike fails after initial recruitment if the first click on "Explore Tundra" doesn't take effect, by restoring the entry retry behavior consistent with other roguelike themes.

#### Mac Touch Mode Options Apply Immediately

Switching touch mode (e.g. `minitouch` ↔ `maatouch`) in the Mac client now takes effect on the next connection without requiring an app restart.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.17.4 (2026-09-09)</b></summary>

### 改进 | Improved

* Mac 客户端切换触控模式等实例选项后，下次连接即生效，无需重启应用 ([#110](https://github.com/MaaAssistantArknights/MaaMacGui/pull/110)) @VinciJL
* 优化 macOS ScreenCaptureKit 线程同步与资源生命周期管理，帧状态校验更严谨，异步流关闭后资源安全释放 @hguandl

### 修复 | Fix

* 修复萨米肉鸽初始招募结束后，首次点击"探索冰原"未生效时因缺少 `#self` 重试分支导致任务耗尽识别次数报错的问题 ([#18146](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18146)) @Koileo
* 修复任务队列视图中"更换主题"任务入口被意外隐藏、"自定义任务"条目缺失的问题 ([#18147](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18147)) @ABA2396

</details>

<details>
<summary><b>v6.17.3 (2026-09-08)</b></summary>

### 新增 | New

* 任务链新增 ｢更换主题｣ 任务，按游戏内主题名称自动切换主题，可填写多个主题每次运行随机选择一个，主题未解锁或未找到时给出对应提示 ([#18099](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18099)) @ABA2396 @HX3N @momomochi987 @Constrat
* 库存保持新增 ｢仅执行第一个库存不足的计划｣ 选项，第一个计划补满后下次运行自动继续后续计划 @ABA2396

### 改进 | Improved

* Swipe 的 specialParams[1] 额外滑动参数从 0/1 开关拓展为方向选择（0 不启用，1/2/3/4 分别为上/下/左/右）([#18099](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18099)) @ABA2396
* Swipe 的 specialParams[2]/[3] 缓入、缓出斜率参数改为乘 10 输入（默认 10），正常缓入缓出建议 37/1，内置任务参数已同步迁移 ([#18112](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18112)) @status102
* 改进设置指引，连接设置步骤复用实际连接设置界面，修复选择 PC 端时界面残留 ADB 选项且缺少提示，任务设置步骤改为可交互任务演示 ([#18047](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18047) [#18130](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18130)) @satgo1546 @ABA2396
* 设置指引最后一步停留 5 秒后才能点击完成，期间按钮下方显示倒计时 @ABA2396
* 优化黑流树海肉鸽，降低事件选项确认阈值，增加重开等待 ([#18066](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18066) [#18126](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18126)) @ZiyinLin
* 调整基建默认设施顺序，宿舍移至换人设施之后，切换自定义基建后未调整顺序时不再先换宿舍，避免换下的干员送不进宿舍 @ABA2396
* 自动战斗干员缺失提示补充练度未达标情形，不再仅提示未拥有 ([#18046](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18046)) @status102
* 优化界面日志显示，库存保持与更新用户数据的任务链日志按计划序号或识别类型区分，日志分隔线标题过长时换行显示 @ABA2396
* KR shorten resource-repair dialog strings to fit the UI @HX3N

### 修复 | Fix

* 修复 MuMu 触控增强状态与实际触控模式不同步、残留旧值的问题，增强勾选框改为触控模式投影消除互锁失同步 ([#18134](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18134)) @ABA2396
* 修复作业解析同名干员时可能取错条目的问题，改为取稀有度最高条目，阿米娅技能 3 判断不再受哈希顺序影响，并在技能 3 的支持条件中允许阿米娅 ([#17893](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17893)) @ABA2396 @yali-hzy
* 修复自动战斗与肉鸽作业 `role` 字段的职业解析，未指定或未知职业名不再被误判为无人机职业，并兼容职业大小写；修复以 Unknown 职业注册的技能用法无法正确查找的问题，工具人技能用法改为部署时写入 @status102 @ABA2396
* 修复自动战斗部署时未能正确移除目标地块的过往干员 @status102
* 修复自动战斗作业保存时输出多余超时参数、编队反复切换职业的问题 @status102
* 修复自动战斗干员缺省时技能加载器未回退到按名称检索的问题 @yali-hzy
* 修复黑流树海肉鸽出发前往流程与不投资源石锭状态机，襁褓羽蛇与三头犬改为进入第三层后结算，规划器单拍间隔调整（白模鸟系列留给 boss），策略完成后无法正确停止并上报、未完成时无法重开下一局 ([#17916](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17916) [#17870](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17870) [#17862](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17862)) @ZiyinLin
* 重构黑流肉鸽节点路线（routes）读取，配置解析错误时正确报错 ([#17820](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17820)) @status102
* 干员数据增加子职业解析，找不到干员与多稀有度干员跳过重复检查，召唤物职业解析临时兼容 @status102
* 查找干员在职业未知时回退到按名称匹配，避免检索失败 ([#17735](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17735)) @yali-hzy
* 刷理智代理倍率识别改用 RGB 颜色匹配，提升识别稳定性 ([#17719](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17719)) @status102
* 保全作业浏览时不自动添加到作业列表 @status102
* 更新保全作业，新增日达诺夫园区与荒废灯塔保全作业 @Saratoga-Official
* 优化理智作战高级设置界面布局，调整自动战斗缺少干员时的提示与报错描述，明确 ｢特别关注｣ 影响识别时的处理方式 @ABA2396
* 任务因内存不足停止时给出专门提示，建议关闭部分程序或重启 MAA 后重试 @ABA2396
* 调整基建干员冲突提示，检测到干员已进驻其他设施时将自动确认调动，日志不再标红 @ABA2396
* 显卡兼容性提示在每次开始运行时输出，避免被任务日志刷掉后无法看到；调整 core 崩溃后和未知异常的错误提示 @ABA2396
* 为软件更新包下载添加重试 ([#17675](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17675)) @bkzzzz
* 截图耗时 100ms 以上且未启用截图增强时，补充截图优化建议，并优化截图增强报错与设置指引 @ABA2396
* 优化启动设置页提示 @ABA2396
* 打包时用原生启动器替换 MAA.exe 的 apphost，MAA.exe 被单独移动等文件不完整的情况能给出明确提示，不再误报未安装 .NET ([#17727](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17727)) @ABA2396
* 切换客户端后断开现有连接 @ABA2396
* 自动战斗自动编队切换职业时切换回全部职业分类，避免游戏未重置 UI 位置 @status102
* 繁中服调整文件与界面的在地化用词 ([#17856](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17856)) @momomochi987
* 繁中服调整部分干员与关卡名称 OCR 并增加容错 ([#17703](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17703) [#17828](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17828)) @momomochi987
* 繁中服补全并调整界园肉鸽通宝权重，更新界园肉鸽干员管理入口模板 ([#16306](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16306) [#16634](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/16634)) @travellerse @abmcar
* YostarEN/JP/KR update MiniGame SPA S2 templates ([#17808](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17808)) @Constrat @Manicsteiner @HX3N
* YostarKR update localization with official terminology, normalize BlackFlow roguelike terminology, and polish task-log strings and clue terminology @HX3N
* 修复从公招的选择招募时限界面开始自动公招功能，会触发循环操作的问题 @ABA2396
* 修复可搜索 ComboBox 的一批问题：语言切换等场景下选项绑定失效、指定材料被清空、初始化时将已绑定值覆盖为列表第一项（肉鸽开局干员重启后被重置）、下拉列表滚轮一次滚到底、换源统一由扩展接管并修复中间值写回产生的重复日志 ([#17759](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17759)) @status102 @ABA2396
* 修复切换界面语言后部分下拉列表与提示停留旧语言的问题（肉鸽刷通关时长目标、黑流培养目标、自动战斗单位支持用法、隐蔽战线事件、肉鸽开局干员提示）@ABA2396
* 修复从 PC 端切回模拟器需要重启后才能连接 @ABA2396
* 修复 NotifyIcon 双击间隔为 0 时的启动崩溃 ([#17691](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17691)) @bkzzzz
* 修复连接配置下拉框打开和滚动时整个页面位移的问题 @ABA2396
* 修复自定义 Webhook 无法通过 Headers 设置 Content-Type 导致通知发送失败的问题，消息体占位符补全 Json 转义 @ABA2396
* 修复 ｢开始任务：｣ 分隔栏在界面最小化时无法正确添加、Rectangle 无法显示的问题 @ABA2396
* 修复小游戏界面开始任务时连接模拟器失败无任何提示 ([#17887](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17887)) @ABA2396
* 修复无配置文件时首次启动更新源显示为空 @ABA2396
* 修复亮色模式下下拉框高亮文本颜色错误 @ABA2396
* 修复像素画英文界面按钮显示不全、切换语言时适配与抖动下拉框内容未实时切换 @ABA2396
* 修复成就｢不务正业｣达成条件文案未跟随术语显示 ([#17834](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17834)) @H2O-MERO
* 放宽 Miss.Christine 的 OCR 识别容错 @Lancarus
* 繁中服适配新版登录界面 ([#17853](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17853))，调整「辭歲行」OCR 文字 ([#17910](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17910)) @momomochi987
* YostarKR correct CharsNameOcrReplace regex, update SilverAsh ocrReplace, and fix ocrReplace regex for 烛煌 ([#17946](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17946)) @HX3N
* YostarKR loosen SkipThePreBattlePlot match threshold for lighter plot overlays @HX3N
* YostarKR fix Roguelike recruitment giving up instead of recruiting @HX3N
* YostarKR handle startup notification during account switch @HX3N
* YostarKR update Roguelike@TraderRandomShoppingConfirm template @HX3N
* YostarEN fix Eyja alter OCR regex @Constrat
* YostarEN rework Sami IS floor detection to match region names instead of temperature numbers @Constrat

### 文档 | Docs

* 新增 MaaFramework 控制单元自动下载脚本及开发文档说明 ([#17786](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17786)) @Justin-Emils
* 同步更新五语言文档（连接与设备说明、集成协议、copilot 等任务 schema、开发教程与 FAQ）([#17879](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17879)) @ABA2396
* 补充集成协议文档与 schema（nmsDistance 字段、copilot-schema、callback-schema），修正 MirrorChyan 拼写与 config.md 示例 @ABA2396
* 翻译 ja-jp 版 maa-cli 文档并标注机器翻译 @ABA2396
* 添加 Mac GUI 过渡公告 ([#17965](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17965)) @hguandl
* 更新仓库分支名 dev → dev-v2，修复文档站｢编辑此页｣链接 404 ([#17920](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17920)) @satgo1546
* 补充拖入更新文件的管理员权限说明 ([#17842](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17842)) @youzibigg
* CI 教程发布分支由 master 更新为 master-v2 ([#17823](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17823)) @youzibigg
* 更新手入门文档，简化下载安装步骤并说明日志包生成方式 @ABA2396
* 肉鸽文档补充黑流树海推荐开局 ([#17753](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17753)) @Rbqwow @ABA2396
* 修正多语言文档错别字，更新作业协议 `nav_name_override` 字段说明 ([#17708](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17708)) @apricity093 @hguandl
* 补充会客室取下线索的说明 @ABA2396

### 其他 | Other

* 修复 Unix 平台上构建 MaaWpfGui 报错 ([#17958](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17958)) @satgo1546

### MaaMacGui

#### 新增 | New

* 支持作业集 @hguandl
* 支持奇象巡展像素画 @hguandl

#### 改进 | Improved

* 调整像素画选项文案 @hguandl

</details>
