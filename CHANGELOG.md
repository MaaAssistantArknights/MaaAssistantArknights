## v6.17.3

### Highlights

#### 随机主题与更换主题任务

购买了大量主题但不知道选哪个？新增随机主题功能，可从所有已安装主题中随机选取；同时新增更换主题任务与额外滑动方向（上/下/左/右）拓展。

#### 库存保持优化与任务链日志区分

库存保持任务新增仅执行第一个库存不足计划的选项；更新用户数据与库存保持的任务链日志现按序号或识别类型区分显示，便于排查。

#### MuMu 触控增强状态修复

修复 MuMu 触控增强在多种场景下的状态残留与互锁失同步问题：连接成功时重置触控增强状态、勾选框改为触控模式投影、启动时归一截图增强关闭后清理残留触控模式。

<details>
<summary><b>English</b></summary>

#### Random Theme and Switch Theme Task

Not sure which theme to pick from your large collection? A random theme feature has been added to select randomly from all installed themes; additionally, a switch-theme task and extra swipe directions (up/down/left/right) are now available.

#### Depot Maintain Optimization and Task-Chain Log Distinction

Depot maintain now supports an option to execute only the first plan that lacks stock; task-chain logs for depot maintain and user-data updates are now distinguished by serial number or recognition type for easier troubleshooting.

#### MuMu Touch Enhancement State Fixes

Fixed residual state and interlock desynchronization issues with MuMu touch enhancement in multiple scenarios: touch enhancement state is reset on successful connection, the checkbox now uses touch-mode projection, and residual touch mode is cleaned up when normalized screenshot enhancement is disabled at startup.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.17.3 (2026-09-08)</b></summary>

### 新增 | New

* 新增了随机主题功能（从所有已安装主题中随机选取），并新增更换主题任务与额外滑动方向（上/下/左/右）拓展 ([#18099](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18099)) @ABA2396
* 库存保持任务新增仅执行第一个库存不足计划的选项 @ABA2396
* 库存保持与更新用户数据的任务链日志按序号或识别类型区分显示 @ABA2396
* 设置指引第4步任务设置改为可交互任务演示 ([#18130](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18130)) @ABA2396

### 改进 | Improved

* 任务排序器（TaskSorter）降低复杂度并引入 jsonc 库支持 ([#18094](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18094)) @Constrat
* OperBox 相关 Analyzer 迁移 @status102
* 修改 Swipe 行为下 SpecialParams 的 Slope-in 和 Slope-out 参数改为除以 10 读取，并优化部分滑动 ([#18112](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18112)) @status102
* 基建默认排序调整，避免切换自定义基建后未调整顺序时先换宿舍 @ABA2396
* 设置指引第3步连接设置复用 ConnectSettingsUserControl ([#18047](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18047)) @satgo1546

### 修复 | Fix

* 修正干员名识别替换正则，将 `埃.*斯` 精确为 `^埃[癸类奖突]?斯$`，修复误替换问题 ([#18102](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/18102)) @ABA2396
* 降低黑流树海事件选项确认阈值 ([#18066](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18066)) @ZiyinLin
* 新增黑流树海重开等待逻辑 ([#18126](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18126)) @ZiyinLin
* 修复繁中服自动编队不会切换技能页的问题 ([#18118](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18118)) @vonnoq
* 修复 MuMu 触控增强在连接成功时残留旧值、勾选框互锁失同步、启动时归一截图增强关闭后残留触控模式等问题 @ABA2396
* 修复 YostarEN 萨米 IS4 温度识别：缩小 ROI，并将温度正则简化为非贪婪匹配 ([#18088](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/18088)) @Constrat
* 修复 YostarEN Sami 错误策略问题 @Constrat
* 修复 KR 服掌灯与引烛的 ocrReplace 正则 @HX3N
* 干员缺失提示增加属性未达标说明 ([#18046](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18046)) @status102
* KR 服缩短资源修理对话框字符串以适配 UI @HX3N
* 修复日志分隔线标题过长时换行显示 @ABA2396

### 文档 | Docs

* 格式化文档 @ABA2396

### 其他 | Other

* 移除 CharsNameOcrReplace 中过时的正则字节处理说明 @ABA2396

### MaaMacGui

#### 修复 | Fix

* 修复切换触控模式后无需重启即可在下次连接生效 ([#110](https://github.com/MaaAssistantArknights/MaaMacGui/pull/110)) @VinciJL

</details>

## v6.17.2

### Highlights

#### 黑流树海肉鸽适配

新增适配黑流树海肉鸽，支持刷等级、刷源石锭、刷襁褓动物三个策略。

#### 界面过渡动画与操作体验优化

主界面页签、任务链等内容切换加入方向性过渡动画，设置页导航改为平滑滚动；自定义下拉交互统一（点外部关闭、连击保持）。

#### 移除掉线重连，通宵挂机改用定时启动

掉线重连的恢复链状态复杂且维护成本高，已移除；通宵挂机场景请改用定时启动与强制定时启动。

#### 基建效率算法重写

重写基建效率算法，常规（默认）模式支持跨设施组合，新增跨设施组合设置与菲亚梅塔恢复目标设置，并修复一批基建选人与识别问题；宿舍换班调整为第一轮仅执行菲亚梅塔配对，新增恢复开关。

<details>
<summary><b>English</b></summary>

#### BlackFlow Roguelike

Added support for the BlackFlow (黑流树海) roguelike theme, with the level-farming, Originium Ingot investment, and cultivation strategies.

#### UI Transition Animations and Interaction Polish

Main tabs and task-chain switching now animate with directional transitions, and settings-page navigation scrolls smoothly; custom dropdown interactions are unified (click outside to close, repeated clicks keep open).

#### Reconnect Removed in Favor of Scheduled Startup

The reconnect-after-disconnect logic has been removed due to the complexity and maintenance cost of restoring the chain state; for overnight sessions, please switch to scheduled startup and forced scheduled startup instead.

#### Infrast Efficiency Algorithm Rewrite

The infrast efficiency algorithm has been rewritten; the default mode now supports cross-facility combinations, with new settings for cross-facility combinations and Fiammetta recovery targets, along with a batch of infrast operator selection and recognition fixes. Dormitory shifting now performs only Fiammetta pairing in the first round, with a new recovery toggle.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.17.2 (2026-09-06)</b></summary>

### 新增 | New

* 检测到资源文件损坏时，新增尝试下载完整包进行修复 @ABA2396

### 修复 | Fix

* 修复月行水上设定次数代理连战时，刚好在最后一轮结算后弹出的塔罗牌弹窗无人关闭、阻挡后续任务的问题；弹窗改为经 PreStartCheck 等待界面稳定后关闭 ([#18071](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/18071)) @ABA2396
* 修复窗口初始化时点击最大化按钮闪退 ([#12034](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/12034)) @ABA2396
* 恢复 YostarEN/JP/KR 与繁中服的仓库页签模板任务，修复仓库识别失败 @HX3N
* 修复繁中服仓库辨识，更新材料页签模板 ([#18081](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18081)) @vonnoq
* 修正干员名识别替换 埃类斯 -> 埃癸斯 @ABA2396

### 文档 | Docs

* 自动战斗与自动肉鸽的触控模式文档补充 MuMu 触控增强说明 ([#18052](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18052)) @youzibigg

</details>

<details>
<summary><b>v6.17.1 (2026-09-04)</b></summary>

### 新增 | New

* 新增 SideStory「月行水上」活动关卡导航，支持 SR-6~8 与搓玉关 SR-5 @Daydreamer114 @ABA2396

### 修复 | Fix

* 修复游戏新增 ｢特别纪念｣ 页签后仓库识别报错，更新仓库页签模板 @ABA2396
* 修复月行水上活动代理连战中，结算退出后概率弹出的塔罗牌弹窗遮挡界面导致任务停止的问题 @ABA2396
* 修复 PC 端（Win32）连接下滑动操作被判定为点击的问题：滑动插值步间补充等待，Seize 等异步输入方式下不再瞬移 @ABA2396
* 修复自动战斗指定编队选择第二、三队时有低概率触发编队改名 @ABA2396
* 修复自动战斗部分早期别传（SS）活动关卡代码匹配失败 @status102
* 修复黑流树海的一批识别问题：补充楼层识别正则、调整点击 ｢出发前往｣ 的识别范围 ([#18023](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18023) [#18037](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18037)) @ZiyinLin
* 修复 MuMu/雷电连接与系统通知的提示文案中转义符显示异常 @ABA2396
* 修复界面文案中含 `.` 或 `@` 的 `{key=}` 引用未被替换的问题 @ABA2396

### 改进 | Improved

* 补齐一批设置项与界面的提示说明，修正歧义命名（｢烧水使用分队｣ 改为 ｢刷开局使用分队｣ 等） @ABA2396
* 统一界面提示宽度，默认上限改为 500 @ABA2396

### 文档 | Docs

* 对照代码全面修正文档 ([#18030](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18030)) @ABA2396
* 各语言文档补充 RIIC.Autos 链接 ([#18022](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18022)) @BrKDDD

</details>

<details>
<summary><b>v6.17.0 (2026-09-02)</b></summary>

### 新增 | New

* 新增黑流树海肉鸽，支持刷等级、刷源石锭、刷襁褓动物三个策略 @ZiyinLin @ABA2396
* 新增侧边栏快捷入口：仓库、基建、采购中心、信用站、商城、贸易点 @ABA2396
* 新增任务队列支持暂停/继续功能 @ABA2396
* 设置页面新增任务队列管理入口 @ABA2396
* 自动战斗新增保全作业浏览功能 @status102
* 保全作业新增日达诺夫园区与荒废灯塔两个保全作业 @Saratoga-Official
* 肉鸽新增干员数据子职业解析，找不到干员与多稀有度干员跳过重复检查，召唤物职业解析临时兼容 @status102 @yali-hzy
* 查找干员在职业未知时回退到按名称匹配，避免检索失败 ([#17735](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17735)) @yali-hzy
* 刷理智作战高级设置倍率识别改用 RGB 颜色匹配，提升识别稳定性 ([#17719](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17719)) @status102
* 任务因内存不足停止时给出专门提示，建议关闭部分程序或重启 MAA 后重试 @ABA2396
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

### 修复 | Fix

* 更新器等待主程序退出增加超时强制结束，主程序意外滞留时更新不再无限等待；主程序启动中止时改为立即退出进程；更新器获取父进程句柄权限不足时不再跳过等待 ([#17930](https://github.com/MaaAssistantArknights/MaaAssistantArknights/issues/17930)) @ABA2396
* 修复作业解析同名干员时可能取错条目的问题，改为取稀有度最高条目，阿米娅技能 3 判断不再受哈希顺序影响，并在技能 3 的支持条件中允许阿米娅 ([#17893](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17893)) @ABA2396 @yali-hzy
* 修复自动战斗与肉鸽作业 `role` 字段的职业解析，未指定或未知职业名不再被误判为无人机职业，并兼容职业大小写；修复以 Unknown 职业注册的技能用法无法正确查找的问题，工具人技能用法改为部署时写入 @status102 @ABA2396
* 修复自动战斗部署时未能正确移除目标地块的过往干员 @status102
* 修复自动战斗作业保存时输出多余超时参数、编队反复切换职业的问题 @status102
* 修复黑流树海肉鸽出发前往流程与不投资源石锭状态机，襁褓羽蛇与三头犬改为进入第三层后结算，规划器单拍间隔调整（白模鸟系列留给 boss），策略完成后无法正确停止并上报、未完成时无法重开下一局 ([#17916](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17916) [#17870](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17870) [#17862](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17862)) @ZiyinLin
* 重构黑流肉鸽节点路线（routes）读取，配置解析错误时正确报错 ([#17820](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17820)) @status102
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
* 修复成就 ｢不务正业｣ 达成条件文案未跟随术语显示 ([#17834](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17834)) @H2O-MERO
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
* 更新仓库分支名 dev → dev-v2，修复文档站 ｢编辑此页｣ 链接 404 ([#17920](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17920)) @satgo1546
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
