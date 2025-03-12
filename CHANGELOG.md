## v5.14.0

### 有味道的版本号 | Highlight

在这个版本，我们继续对基建、萨卡兹肉鸽等部分的问题进行修正，并且开始探讨新的更新模式，敬请期待！

#### 加解密失败通知

从以前的版本更新到最近几个版本的用户，可能会遇到以下情况：

> 每次打开 MAA，都会收到系统通知，提示“加解密失败，请重新配置相关设置，否则加密内容将以明文形式存储在配置文件中。”

这是因为 v5.10.0 版本引入的“配置文件保护”机制（在 v5.13.0 的更新日志里说明过），只能解密已经加密过的内容。而如果配置文件里的密钥等是在以前就添加的，则会保持明文形式保留在配置文件中。

我们建议用户在设置里：

* 【远程控制】部分，重新生成【设备标识符（只读）】；
* 【外部通知】部分，检查所有通知配置，并**剪切粘贴**一次所有密码、密钥。

#### 基建部分

上次基建更新，yj 为会客室添加了一个可交互家具【信息板】，该家具每周会收集来访好友带来的信用点，上限10次。

这个版本开始，牛牛会尝试收取这部分信用。

本次更新，我们也修复了大量基建相关问题，例如无法进入会客室、无法切换贸易站订单等，同时也优化了各部分流程，降低卡住的概率。

#### 萨卡兹肉鸽部分

本次更新，恰逢萨卡兹肉鸽深入调查模式的实装，我们也跟进并优化了部分事件和分队的交互逻辑。

但请注意，牛牛只是一个自动机，还不能很好地应对一些肉鸽关卡，我们仍然在继续优化中。

#### 其他部分

这个版本，我们对配置文件的格式进行了改动，增强可读性与可（lǔ）靠（bàng）性。

同时，“外部通知”功能支持详细输出，和错误时额外通知功能，希望能让你注意到牛牛呀。

### 新增 | New

* 适配「滋味」界面主题（YoStarEN） (#12135) @gui-ying233
* SideStory「挽歌燃烧殆尽」导航 (#12094) @SherkeyXD
* 基建收取会客室周限300信用 (#12014) @status102 @pre-commit-ci[bot] @Constrat
* ExternalNotification 可选输出详细信息 (#12020) @ABA2396
* 更新 243 极限效率一天四换排班表（20250310 修订） (#12132) @bodayw @Daydreamer114
* 萨卡兹肉鸽深入调查实装 (#12098) @BxFS
* 萨卡兹肉鸽 extend2 事件 转机、似是而非 (#11971) @Daydreamer114
* 萨卡兹肉鸽 extend2 专业人士分队 (#11938) @Daydreamer114
* YostarEN DT navigation @Constrat
* YostarEN I.S. modes (#11956) @Constrat
* YoStarEN Sarkaz roguelike (#11921) @Constrat @Daydreamer114
* YostarJP Sarkaz roguelike (#11914) @Manicsteiner @Daydreamer114
* YostarKR Sarkaz roguelike (#11920) @HX3N @Daydreamer114 @pre-commit-ci[bot]

### 改进 | Improved

* 基建信息板收取信用增加开关 (#12050) @status102 @pre-commit-ci[bot]
* 热键支持 Windows 键 @ABA2396
* 添加强制使用 Github 进行版本更新 @ABA2396
* Wpf自动战斗重构 (#11977) @status102
* 填写完cdk后自动检查一次更新 @ABA2396
* 刷理智企鹅物流上报失败-不支持的关卡，输出降级为警告，移除提示中的更新提醒 @status102
* 自动战斗在作业地图不支持时，自动检测资源更新 @status102
* 水月肉鸽 精酿杀手 防爆桩 @Daydreamer114
* 萨米肉鸽 度假村冤魂 防爆桩 @Daydreamer114
* 调整萨卡兹肉鸽作战、招募 @Daydreamer114
* 肉鸽难度选项降到18个 (#11934) @Daydreamer114
* 萨卡兹肉鸽调整公害ew部署 (#11935) @Daydreamer114

### 修复 | Fix

* 枯朽祭坛 维什戴尔 逻各斯 OCR辨識 (#12096) @XuQingTW @pre-commit-ci[bot]
* 更新 243 极限效率一天四换排班表 (#12137) @bodayw
* 自动战斗费用识别在部分分辨率下概率出错 @status102
* 修复自动战斗-战斗列表批量导入失效 @status102
* 尝试修复肉鸽不期而遇退出后仍在尝试点击事件 @BxFS @Constra
* 傀影肉鸽可以选择在第五层 BOSS 前暂停 @Daydreamer114
* 外服重复检查资源 (#11927) @ABA2396
* 支持肉鸽特定模式下跳过选队直接选人 (#11915) @BxFS
* 3星招募逻辑修复 (#11913) @Roland125
* 「滋味」主题概率无法识别 @ABA2396
* 萨米第二次调查装备获取+萨米暂时撤退适配 (#12030) @BxFS
* retake Mizuki IS recruit templates try to fix triple recruit issue @Constrat
* 月度小队/深入调查未启用自动切换会以当前游戏模式开始探索 (#12032) @BxFS
* 肉鸽深入调查没有指挥分队可选时，随机选择分队跳过干员招募 @status102
* fast forward text in encounter (#12021) @Constrat
* IS2, IS3 支援起重机 缺失 @Daydreamer114
* Wpf信用任务-OF-1战斗 修复未勾选刷理智时，仍判断刷理智关卡是否为`当前/上次` @status102
* 肉鸽放弃战斗奖励后仍能识别到掉落 (#12000) @Daydreamer114
* Monthly Squad for Global (#11993) @BxFS @Constrat
* 会客室进入失败 (#11950) @ABA2396 @pre-commit-ci[bot] @Daydreamer114
* add an missing @ for MonthlySquadCommsBackTwice (#11991) @BxFS
* 狭路相逢遇到构想卡死 (#11970) @Daydreamer114
* IS4 绝境？抉择？ocr (#11969) @Daydreamer114
* wpf自动战斗-追加自定干员失效 @status102
* sarkaz MS count 1->8 (#12085) @BxFS
* 修复截图速度过快导致不能切换贸易站订单的问题 (#12090) @Roland125
* 基建设置UI绑定失效 @status102
* burden/toil tasks threshold 0.9 -> 0.8 (#12087) @BxFS @Constrat
* wpf自动战斗-自动编队追加自定干员意外激活 @status102
* log rotate (#12025) @status102 @pre-commit-ci[bot]
* Sarkaz MS unable to progress from post StartExplore (#12039) @BxFS @HX3N @Constrat
* stuck on StageEnterBattleAgain while in team selection screen (#12059) @BxFS
* 「滋味」主题在有多个 badge 时无法识别基建入口 (#12057) @Alan-Charred
* 修复公招的几个问题 (#12056) @Roland125 @pre-commit-ci[bot]
* 修复不自动招募4星干员时，会执行空招募的问题 (#12054) @Roland125 @pre-commit-ci[bot]
* 自动战斗追加自定义干员序列化 @status102
* add custom text and roi for SideStory DT @Constratt
* EN remove all spaces from encounter and ignore spaces @Constrat
* EN Sarkaz SelectTheme @Constrat
* YoStarEN 肉鸽开局分队界面检测ROI扩大 @status102
* EN服萨卡兹肉鸽负荷干员icon ROI @status102
* YostarKR updated the StartUpConnectingFlag template (#11960) @HX3N
* Fix hidden floor Sarkaz EN @Constrat
* EN 服 IS2 导航失效 (#11955) @Daydreamer114
* YostarJP IS4 计划耕种 (#11990) @Manicsteiner
* YostarKR Sarkaz CR Node recognition (#11986) @HX3N
* YostarKR missing tasks (#11981) @HX3N
* Missing EN Sarkaz tasks 2 @Constrat
* EN Sarkaz missing NextLevel task @Constrat
* EN replace Sguad with Squad @Constrat
* Mac 日服 IS4 识别阈值 (#11979) @Daydreamer114

### 文档 | Docs

* Wpf选中战斗列表作业时，作业内容重复显示 @status102
* StartupUpdateCheck 不生效 @ABA2396
* 调整肉鸽文档 skill_usage 描述，尝试 json5 @Daydreamer114
* 此AI非彼AI (#12005) @Rbqwow
* 作业协议补充难度字段 (#11980) @status102
* 更新文档 @Daydreamer114
* update english schema @Constrat
* 更新开发前须知 (#12043) @Rbqwow @pre-commit-ci[bot]

### 其他 | Other

* wpf领取奖励任务序列化错误 @status102
* 自动战斗移除点赞时判断作业是否来自云端 @status102
* Log Rotate 临时性检查措施 (#11835) @status102
* 自动战斗保全作业地图存在性判断 @status102
* missing tasks for EN @Constrat
* 生息演算Mode序列化类型 @status102
* 移除肉鸽结算插件中的Matcher复用 @status102
* 修复BattleHelper中的意外报错 @status102
* 尝试修复启动时不显示ui @ABA2396
* wpf开始唤醒任务在未选择官服/B服时，不再提供账号切换输入 @status102
* Wpf地图信息MapInfo字段名更正 @status102
* typo in xaml themes for Idea Filter + i18n EN @Constrat
* wpf自动战斗战斗列表序列化 @status102
* 萨卡兹肉鸽负荷干员编入入口ROI拆分 (#11966) @status102
* remove duplicate strings (#12065) @Constrat
* smtp 格式 @ABA2396
* 自动战斗列表非即时添加作业时，使用已读取的缓存替代读取文件 @status102
* 优化日志输出 @ABA2396
* 肉鸽为识别错误的事件也添加 callback (#11946) @Daydreamer114 @HX3N
* Wpf重构自动公招任务序列化 (#11951) @status102
* Wpf重构生息演算任务参数序列化 (#11916) @status102
* 修改初次设置引导中的项目，移除账号切换相关 @status102
* 调整干员识别及仓库识别开始流程 @status102
* 基建会客室信息板无新访客时，不再进入领取页 @status102
* 移除无用的彩蛋控件 @status102
* 肉鸽投资ocr封装 @status102
* 手动更新下活动名称 @ABA2396
* 修改彩蛋gif默认路径为string.Empty @status102
* remove spaces for all ocrReplaces for KR + various optimizations (#11926) @Constrat
* parse release note for resource updater (#12092) @MistEO @ABA2396 @Constrat
* 猫猫 (#12017) @ABA2396
* 肉鸽任务参数禁止运行期修改 @status102
* 合并视频任务判断 @status102
* SimpleEncryptionHelper 支持默认字符串 @ABA2396
* Wpf地图查询 @status102
* 读取地图数据 (#11973) @status102
* Wpf旧Config增加int读取 (#11987) @status102
* deprecate more Intel integrated GPUs @dantmnf
* give priority to flint item compared to flint operator @Constrat
* YostarKR Theme Delicious (#12131) @HX3N
* YostarJP Theme Delicious (#12130) @Manicsteiner
* YostarJP DT stage (#12129) @Manicsteiner
* YostarKR DT stage roi and text (#12127) @HX3N
* wpf招募任务序列化 (#12080) @status102
* Revert "chore: Auto Update Game Resources - 2025-02-18" @Daydreamer114
* revert c3d98ec8822fc41892c07f6b6caef5fd435fbc43 @status102
* EN tasks [skip changelo] [skip ci] @Constrat
* 加解密失败时通知 @ABA2396
* TaskData增加OcrTask的ocrReplace合法性检查 (#11878) @status102
* 使用RecruitData代替ocrReplace的外服适配 (#11879) @status102 @momomochi987
* tools: ImageCoordinate cursor tweak @Constrat
* 7a9c5a9f2db1360939e804e716d07937efafee7d for YoStarEN @Constrat
* MirrorChyanCdk 添加 Placeholder @ABA2396
* Wpf自动战斗作业序列化调整 @status102
* Wpf自动战斗任务序列化 @status102
* 移动截图耗时显示位置以避免产生GPU加速截图的误导 @status102
* 拆分 OF-1 与 访问好友回调显示 @ABA2396
* clarify different originium types and unify deep investigation in EN (#11995) @dragonheart107 @Constrat
* fix typo @MistEO
* mirrorc with new tab @MistEO
* 删除官网早就不能用了的镜像 @MistEO
* 官网添加 MirrorChyan 下载链接 @ABA2396
* add missing tasks and template for txwy (#11985) @momomochi987
* ResourceUpdater to remove spaces from EN I.S. encounter @Constrat
* Revert "ci: 临时措施" (#12033) @MistEO
* wpf基建任务序列化默认值补全 @status102
* 删除多余判断 @ABA2396
* YostarKR DT navigation (#12105) @HX3N
* wpf信用任务序列化 @status102
* YostarJP DT テラ飯 preload (#12101) @Manicsteiner
* 提取 ResourceReload 方法 @ABA2396
* wpf领取奖励任务序列化 @status102
* 倒计时添加日志 @ABA2396
* 添加 spid @ABA2396
* 调整参数名 @ABA2396
* Wpf基建任务序列化调整 (#12053) @status102
* Wpf唤醒任务序列化 @status102
* Wpf关闭明日方舟任务序列化 @status102
* 外部通知邮件 html 支持颜色输出 @ABA2396
* wpf生息演算任务序列化 @status102
* smtp 通知使用 html @ABA2396
* add sami investigation equipment text and temporary retreat template (#12046) @momomochi987 @pre-commit-ci[bot] @HX3N
* 猫猫改为全局 @ABA2396
