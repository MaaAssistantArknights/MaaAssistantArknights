## v6.19.0-beta.2

### Highlights

#### 新增 ｢干员培养｣ 任务（测试功能）

为干员设定精英化、技能等级与专精目标后，任务将自动逐步完成培养流程，材料即缺即合自动合成。该功能为测试功能，可能存在较多未发现的 bug，遇到问题欢迎通过 ｢设置 - 问题反馈｣ 反馈。

#### 从指定任务处继续运行

任务列表新增 ｢从此处运行｣ 右键菜单项，可从指定任务处开始执行任务队列，跳过其之前的任务，任务失败后无需从头重跑；指定任务未启用时从其后首个已启用的任务开始。

#### 界面显示干员头像与物品图片

干员识别、自动战斗作业预览、干员培养与肉鸽开局设置等界面新增干员头像与职业图标显示，干员识别卡新增 ｢头像展示｣ 样式选项；理智作战与库存保持的指定材料下拉、牛杂 ｢活动商店｣ 的黑名单多选框同步支持显示物品图片。

#### 自动战斗一图流数据预检

自动战斗可使用一图流的干员练度数据预检作业编队，提前校验编队中干员的练度是否满足作业要求；编队结果按干员实际数据匹配优化，需在设置中配置一图流 Token。

<details>
<summary><b>English</b></summary>

#### New Task: Operator Progression (Testing)

Set elite promotion, skill level, and mastery targets for an operator, and the task completes the training flow step by step with on-demand material synthesis. This feature is still in testing and may contain undiscovered bugs; feedback is welcome via Settings - Issue Report.

#### Resume the Queue from a Selected Task

A "Run from Here" option is added to the task list context menu, letting you start the queue from any task and skip the ones before it, so a failed run no longer has to restart from scratch. If the selected task is disabled, the queue starts from the first enabled task after it.

#### Operator Avatars and Item Images in the UI

Operator avatars and class icons are now shown in the operator recognition view, copilot formation preview, operator progression, and roguelike start-up settings; the recognition card gains an "Avatar View" style option. The material dropdowns in Combat and Depot Maintain, as well as the event shop blacklist checkboxes, now display item images as well.

#### Copilot Formation Pre-check with Yituliu Data

The copilot feature can pre-check the formation against operator data from Yituliu, verifying in advance whether the operators in a formation meet the operation's requirements; formation assignment is optimized with real operator data. A Yituliu token needs to be configured in Settings.

</details>

----

以下是详细内容：

<details open>
<summary><b>v6.19.0-beta.2 (2026-10-10)</b></summary>

### 新增 | New

* 定时设置新增 ｢定时启动前尝试唤醒计算机并启动 MAA｣，通过 Windows 任务计划程序在每个启用的定时项前 5 分钟唤醒计算机并拉起 MAA（部分设备仅支持从睡眠唤醒，不支持从休眠唤醒） ([#18505](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18505)) @ABA2396
* 自动战斗新增一图流数据预检：作业编队按一图流干员练度数据校验可行性，编队结果改用最小费用流匹配优化；设置中新增一图流 Token 配置与获取链接 ([#17414](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/17414)) @yali-hzy @ABA2396
* 基建换班新增周计划，换班方案支持按星期几勾选启用，当日未勾选时跳过基建任务 ([#18440](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18440)) @H2O-MERO
* 新增界面主题 ｢幽脉｣ ｢剧目｣ ([#18002](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18002)) @SherkeyXD
* 连接设置新增 ｢MuMu 模拟器（Windows ARM）｣ 连接预设，支持自动检测并测试 ([#18380](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18380)) @jisanjin41
* 干员培养支持拖拽调整培养顺序 ([#18443](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18443)) @youzibigg
* PC 端启动设置新增 PC 客户端配置：可设置客户端路径与启动等待时间，支持启动 MAA 后自动开启客户端、连接失败时尝试启动客户端并重连 ([#18026](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18026)) @moranfanhua
* PC 端窗口绑定新增 AnchoredTouch 鼠标输入方式（全后台、不抢占鼠标，需 Win10 1809+，被遮挡时窗口短暂闪烁） ([#18341](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18341)) @wkrs15

### 改进 | Improved

* 外部通知设置重组：新增独立 ｢通知设置｣ 区块（日志有效时间、日志更新停滞检测），发送条件扩展为任务完成、任务出错、定时执行前、日志输出停滞、新增日志条数与日志内容匹配（白名单正则），支持附带历史日志并按数量、有效时间与黑名单过滤，解决 Bark/Discord 因日志过长推送失败的问题 ([#18445](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18445) [#18459](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18459)) @H2O-MERO @ABA2396
* 黑流树海肉鸽商店按策略选择购买与出售列表，完善商店交互流程 ([#18501](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18501) [#18506](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18506)) @ZiyinLin
* 重组设置页区块，性能配置迁移为全局设置，不再跟随配置档案切换 ([#18551](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18551)) @ABA2396 @H2O-MERO
* 任务日志输出各任务用时 ([#18433](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18433)) @H2O-MERO
* 设置指引新增添加任务引导：主界面任务按钮与添加任务菜单显示红点，任务设置步骤需先添加任务才能进入下一步 @ABA2396
* 调整会客室与控制中枢的基建入驻优先级 ([#18449](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18449) [#18450](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18450)) @Reverse0xCC
* 基建生产状态识别兼容活动主题（换肤）设施 ([#18495](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18495)) @panda361
* 干员培养支持导入多语言干员名，任务运行中锁定参数编辑 @status102
* 减少干员培养正常流程中检查的重试次数 ([#18476](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18476)) @youzibigg
* 自动战斗 ｢使用编队｣ 选项状态会被记忆 ([#18523](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18523)) @H2O-MERO
* 作业集预览显示干员头像并标注关卡和组名 ([#18536](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18536)) @youzibigg
* 库存保持增加预检语义提示与识别后缓存翻转警告，理智作战与库存保持的指定材料未选择时显示圆环占位图标 @ABA2396
* 调整黑流树海事件默认选择，固定选项改用编号 ([#18455](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18455)) @H2O-MERO
* 大幅优化彩虹文字动画的资源占用，并优化显示效果 ([#18432](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18432)) @H2O-MERO
* 内核初始化完成前开始任务时弹出警告 ([#18436](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18436)) @H2O-MERO
* 任务链失败时统一保存现场截图至 debug/interface ([#18383](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18383)) @jinghero
* Python 集成接口更新时清理新版本已移除的旧资源文件 ([#18520](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18520)) @djx-trans-n
* PC 端任务运行中允许点击 ｢窗口恢复｣ ([#18278](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18278)) @H2O-MERO
* YostarKR add the Reconstruct UI theme @HX3N

### 修复 | Fix

* 修复黑流树海肉鸽前往预览未按步行与加工品分别读取行动力消耗的问题 ([#18539](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18539)) @ZiyinLin
* 修复黑流树海肉鸽事件 OCR 识别与 ｢出发前往｣ 按钮模板匹配失败的问题 ([#18516](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18516) [#18529](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18529)) @ZiyinLin
* 修复界园肉鸽事件选项无限滑动的问题 ([#18477](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18477)) @ZiyinLin @Constrat
* 修复干员培养技能专精等级识别与回报错误，区分前置专精状态并上报训练中的实际专精等级 ([#18482](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18482)) @status102 @SteveWang92
* 修复干员培养芯片助剂不足识别的问题 ([#18480](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18480)) @youzibigg
* 修复自动编队在无法补充低信赖干员时误执行干员培养的问题 @status102
* 修复战斗结束时等级提升界面点击卡住后无法继续结算的问题 ([#18361](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18361)) @youzibigg
* 修复作业集模式错误地向作业追加 SkillDaemon 动作的问题 @status102
* 修复 SSS 保全派驻部分策略作业战斗等待期间不推进的问题 @ABA2396
* 适配新版登录页账号管理与设置页用户中心入口位置，账号管理识别区域同时兼容新旧登录页布局 ([#18524](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18524) [#18538](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18538)) @Aliothmoon
* 修复肉鸽开局干员手动输入缺少校验的问题 ([#18379](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18379)) @youzibigg
* 修复生息演算 RA4 关卡名括号误识别与完成等待的问题 @Saratoga-Official
* 修复芯片助剂补购返回流程异常并调整商店滑动起点 ([#18498](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18498)) @youzibigg
* 修复基建常规模式宿舍入驻的问题 ([#18447](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18447)) @Reverse0xCC
* 修复物品模板无效时仓库识别继续运行的问题 ([#18521](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18521)) @Arthur031221 @wzacolemak
* 修复自动战斗切换作业页签时循环执行开关状态不同步的问题 ([#18567](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18567)) @yali-hzy @status102
* 修复自定义基建计划包含未适配的设施或产物时报错失败的问题，现改为跳过该计划 @ABA2396
* 修复 Python 集成接口截图返回尺寸与实际图像不一致的问题 ([#18434](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18434)) @Arthur031221
* 修复 PC 端窗口恢复位置错误的问题，以及所有任务全部跳过时窗口停留在屏幕外的问题 ([#18430](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18430)) @H2O-MERO
* 修复 PC 端牛牛监控预览与任务截图相互干扰的问题，预览与任务截图改用独立缓存与采集流程 ([#18462](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18462)) @H2O-MERO
* 繁中服修复烏啾的识别 ([#18519](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18519)) @vonnoq
* YostarKR add ocrReplace for SpecialAccessActivities @HX3N

### 文档 | Docs

* 基建排班协议文档完善，新增排班表扩充协议说明 ([#18511](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18511)) @Zirun-wang
* 集成文档补充活动商店兑换黑名单参数 @ABA2396
* winget 安装命令改为指定包 ID 与 winget 源，修正部分网络环境下 msstore 源证书校验失败导致安装中止的问题 ([#18513](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18513)) @djx-trans-n
* Android 实体设备文档新增 MAA 安卓版使用说明 ([#18503](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18503)) @Aliothmoon
* 修正日语文档中 4 处失效链接 ([#18549](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18549)) @chanshengbinying
* 更新 README 界面截图至干员识别头像显示的新版界面 @ABA2396

### 其他 | Other

* 新增 --skip-core-init 启动参数，跳过内核加载用于界面预览 ([#18492](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18492)) @ABA2396
* 修复资源同步工具：基建模板按房间映射过滤、Recycle 设施校验与头像误判重写导致的资源更新失败 @ABA2396
* ImageCropper 新增无头 CLI 并改造为线坐标语义与两轴窄带网格定位，MaskRangeTool 支持无头调参，移除被取代的 ImageCoordinate 工具 @ABA2396
* 更新仓库内 AI 开发技能（活动导航、模板制作、README 截图演示） @ABA2396
* CI 维护：合并 issue 检查与 AI 分析 workflow、PR 标题检查中英文间空格、调整日志上传失败提醒 @ABA2396 @Saratoga-Official

</details>

<details>
<summary><b>v6.19.0-beta.1 (2026-10-02)</b></summary>

### 新增 | New

* 新增 ｢干员培养｣ 任务：为干员设定精英化、技能等级与专精目标后自动逐步完成培养流程，材料即缺即合自动合成，训练室专精自动选择协助者 ([#18137](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18137) [#18329](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18329) [#18413](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18413) [#18425](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18425)) @Lancarus @youzibigg @HX3N @Constrat @status102 @Manicsteiner @momomochi987
* 任务列表支持右键 ｢从此处运行｣ ，从指定任务处开始执行任务队列，跳过其之前的任务；指定任务未启用时从其后首个已启用的任务开始 ([#18317](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18317)) @H2O-MERO
* 干员识别、自动战斗作业预览与肉鸽开局设置等界面新增干员头像与职业图标显示，干员识别卡新增 ｢头像展示｣ 样式选项 ([#18352](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18352)) @Lancarus @youzibigg @ABA2396
* 干员名、关卡名等 OCR 纠错规则拆分至独立热修复资源，此类修复随资源更新即时分发，无需等待完整版本更新 ([#18406](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18406)) @ABA2396
* 完成后动作新增 ｢锁屏｣（Windows），与休眠、关机、睡眠选项互斥 ([#18412](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18412)) @H2O-MERO
* 牛杂 ｢活动商店｣ 新增商品黑名单，可按商品名称或物品 ID 排除自动购买，选项列表显示材料图标 ([#18270](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18270)) @H2O-MERO @ABA2396
* PC 端连接设置新增 ｢最小化游戏窗口｣ 选项（连接设置 - Win32 附加选项），修复任务运行过程中无法最小化游戏窗口的问题 ([#18419](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18419)) @H2O-MERO

### 改进 | Improved

* 优化黑流树海肉鸽事件选择，按事件策略规则选择选项，界面日志显示实际选择的选项与策略理由 ([#18339](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18339)) @ZiyinLin
* 黑流树海肉鸽新增地图拓扑模板识别，节点预览揭示节点实际身份后立即重新规划路线 ([#18368](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18368)) @ZiyinLin
* 任务栏进度改为按 Core 任务链计数 ([#18338](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18338)) @ABA2396
* 库存保持列表启用虚拟化、像素滚动与惯性滚动，操作按钮区固定显示 @ABA2396
* 干员培养的干员选择框、理智作战与库存保持的指定材料下拉、肉鸽开局干员下拉在收起状态显示已选物品图标或干员头像 @ABA2396
* PC 端跳过掉落上报的提示改为指明具体三方平台，避免重复输出相同内容 ([#18311](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18311)) @H2O-MERO
* PC 端截图测试结束后恢复游戏窗口位置 ([#18427](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18427)) @H2O-MERO
* Rename the roguelike start-up item option from "Thought" to "Plan" in the English UI @Constrat

### 修复 | Fix

* 修复黑流树海肉鸽 ｢失与得｣ ｢先行一步｣ 等事件卡住或报任务出错的问题 ([#18339](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18339)) @ZiyinLin
* 修复黑流树海肉鸽首次进层时楼层名识别失败导致地图重建缺少楼层记录的问题，现在优先在缩放后的画面上识别，持续失败时报错停止 ([#18390](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18390)) @ZiyinLin
* 修复基建常规换班将本轮已安排的干员重复分配到其他设施的问题 ([#18346](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18346)) @youzibigg
* 修复任务出错跳过完成后动作时日志重复输出出错任务名的问题 @status102
* 修复基建换班评分误将 ｢泰拉的方舟｣ （人力办公室技能）按加工站技能评分的问题 @Lancarus
* 修正基建训练室 ｢专精完成｣ 的识别模板与识别区域 @Lancarus
* 修正黑流树海肉鸽 ｢秘境行商｣ 事件名称的错别字（必境→秘境） ([#18420](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18420)) @youzibigg
* 修复界园肉鸽铜币交换完成后偶发识别失败导致任务异常的问题 ([#18423](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18423)) @Manicsteiner
* 补齐六个肉鸽主题中升变阿米娅术师与近卫形态的职业配置，修复使用对应形态时的配置缺失异常 ([#18414](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18414)) @ZiyinLin
* 修复生息演算 RA4 完成识别在部分分辨率下失败的问题 @Saratoga-Official
* 我是小猪（修正识别替换规则中半角括号的正则转义） @Saratoga-Official
* 修复主窗口标题栏文本滚动启用后高度未恢复为单行的问题 ([#18422](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18422)) @H2O-MERO
* 修复 PC 端任务结束后游戏静音未正常恢复的问题 ([#18411](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18411)) @H2O-MERO
* YostarJP add the StageEncounterOptionUnknown template for the Sami roguelike ([#18277](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18277)) @LinYanxi0
* YostarKR adjust the notification dot roi of AutoRaisePotential @HX3N

### 文档 | Docs

* 集成协议文档补充自动提升潜能任务（AutoRaisePotential）的参数说明 @ABA2396

### 其他 | Other

* MaaWpfGui 开启 TreatWarningsAsErrors 并清理 StyleCop 抑制 ([#18340](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18340)) @ABA2396
* CI 升级 pnpm/setup 至 v3 ([#18358](https://github.com/MaaAssistantArknights/MaaAssistantArknights/pull/18358)) @dependabot[bot]
* 资源同步脚本支持 hotfix 变更自动 bump version.json 与单跳同步 @ABA2396
* 统一 XAML 格式化并约定 XAML Styler 工具链 @ABA2396
* 更新开发规范：WPF 配置存储不保留原始 Json、MaaCore 不手动拼接业务任务流 @status102
* 统一计划列表注释与绑定属性格式 @ABA2396
* 新增 ItemImageConverter 物品图标转换器 @ABA2396
* 补充与调整界面层代码注释 @ABA2396
* 版权年份更新至 2026 @ABA2396

</details>
