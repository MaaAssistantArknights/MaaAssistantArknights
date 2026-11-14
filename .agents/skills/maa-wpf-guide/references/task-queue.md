# 一键长草与任务队列

## 入口和运行规则

｢一键长草｣ 左侧勾选待运行的任务，可拖动排序，齿轮打开单条任务设置。底部 ｢添加新任务｣ 可添加同类的多个实例，分别设关卡、条件和顺序。右键半选是临时覆盖，任务列表的效果可在 ｢设置 → 界面设置｣ 反转。点 ｢Link Start!｣ 后由 WPF 读取当前配置、连接设备并将符合条件的任务转换为 MaaCore 任务。一个 WPF 任务可能产生多个 Core 任务，也可能按条件跳过。

以下是 v6.21.3 添加菜单的可见功能。默认预置清单较短。｢自定任务｣ 在代码中标为调试专用。自动战斗和小工具有独立页面；判断某个老版本是否还把生息演算放在队列时，查该版本的菜单实现。

依据：[新手上路](https://docs.maa.plus/zh-cn/manual/newbie.html) · [任务列表及序列化](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UI/TaskQueueViewModel.cs) · [简体中文界面文案](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Res/Localizations/zh-cn.xaml)。

## 可添加任务

| 任务及搜索词 | 怎么用、关键边界和官方入口 |
| --- | --- |
| 开始唤醒／自动开游戏／切号 | 打开游戏并进入主界面。需自动开模拟器时，在 ｢设置 → 启动设置｣ 填模拟器路径或快捷方式，按需设等待时间，并在 ｢连接设置｣ 启用 ｢ADB 连接失败时尝试启动模拟器｣。切号只选择已登录账号，使用列表内唯一的登录名片段；配合切换配置或定时使用。 [开始唤醒](https://docs.maa.plus/zh-cn/manual/introduction/startup.html) |
| 理智作战／刷图／剿灭 | 见下方关卡和停止条件。 [理智作战](https://docs.maa.plus/zh-cn/manual/introduction/combat.html) |
| 基建换班／收菜／线索 | 常规模式按心情、技能和跨设施组合算班；设施勾选决定做哪些，排序遵循算法固定顺序。队列轮换先在游戏内建预设；自定义模式用排班 JSON，可指定换班顺序。会客室在未凑齐线索时可能取下已放的线索，避免线索上限阻塞。信用收支页若提示部分功能要在基建设置做，按对应子功能设置。 [基建换班](https://docs.maa.plus/zh-cn/manual/introduction/infrastructure.html) · [自定义排班格式](https://docs.maa.plus/zh-cn/protocol/base-scheduling-schema.html) |
| 自动公招／高级资深／保留词条 | 可设最大招募次数、自动确认星级、使用加急许可、保留词条或保留 3 星招募许可。识别到指定保留词条会保留该槽位。只想看标签结果，用 ｢小工具 → 公招识别｣。 [公开招募](https://docs.maa.plus/zh-cn/manual/introduction/recruit.html) |
| 信用收支／逛好友／买东西 | 访问好友、买信用商店物品；优先购买与黑名单是关键词匹配。借助战赚信用需要已解锁火蓝之心 OF-1，如果启用的理智作战在其周计划执行日选到了 ｢当前/上次｣，WPF 会提示并禁用本轮信用借助战。 [信用收支](https://docs.maa.plus/zh-cn/manual/introduction/credit.html) |
| 领取奖励／邮件／签到 | 日常及周常奖励；按齿轮里的开关和活动开放状态领取邮件、免费单抽、幸运墙、挖矿、特殊月卡或限时签到。 [领取奖励](https://docs.maa.plus/zh-cn/manual/introduction/rewards.html) · [奖励开关源码](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Configuration/Single/MaaTask/AwardTask.cs) |
| 干员培养／精英化／技能／专精 | ｢添加新任务 → 干员培养｣，在齿轮里逐项选干员和目标，也可通过 ｢在明日方舟工具箱中在线编辑计划｣ 生成计划后 ｢从剪贴板读取｣。运行前核对目标，会花费游戏资源。 [培养界面源码](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/Views/UserControl/TaskQueue/OperProgressTaskUserControl.xaml) |
| 自动肉鸽／集成战略 | 选主题和策略，先核对目标主题的难度、干员和游戏起始状态。详见 [战斗与小工具](combat-and-toolbox.md)。 |
| 更新数据／同步仓库与干员 | 自动进入相关界面识别并写缓存。仓库与干员可分别选，频率可设每次、每天或每周。启用一图流接口后，干员子项会读取远端练度快照。 [更新数据](https://docs.maa.plus/zh-cn/manual/introduction/user-data-update.html) |
| 库存保持／自动补材料 | 见下方库存计划。 [库存保持](https://docs.maa.plus/zh-cn/manual/introduction/depot-maintain.html) |
| 更换主题／换回夜间 | 填游戏主界面显示的候选主题名，多个候选随机选一个，空列表会跳过。主界面无法识别时有可能触发切回日间主题；可把恢复个人主题的任务放队尾。 [更换主题](https://docs.maa.plus/zh-cn/manual/introduction/switch-theme.html) |

## 理智作战

- ｢当前/上次｣ 优先使用用户已定位的关卡，否则尝试终端首页的上次作战。手动输入关卡名只能导航到对应版本资源已适配的关卡。选到开放的备选关卡后，未解锁代理可能导致任务出错；备选项用于选开放关卡。
- ｢指定材料｣ 以本轮掉落或 ｢目标库存｣ 为完成判据，关卡仍需另外选。理智和补充预算、指定次数、指定材料任一满足停止条件即可结束。启用 ｢使用源石｣ 时 WPF 会把药剂次数锁为 999，先用药，再按设定预算碎石。只刷现有理智就关闭补理智相关选项，也检查临期药开关。
- 固定代理倍率按整组执行，指定次数 12 且倍率 5 时可只刷 10 次；AUTO 可按剩余次数与可用倍率调整。周计划按所选区服的游戏日（当地 04:00 刷新）判断，未勾选的那天会跳过该任务。
- 剿灭从终端入口进入，按手册检查已解锁全权委托、400 杀和扫荡券。导航阶段找不到入口可能按本周已完成结束；达到周上限会停止。可加一条单独的剿灭任务放在普通刷图前，并在其周计划设周一。

依据：[理智作战](https://docs.maa.plus/zh-cn/manual/introduction/combat.html) · [WPF 关卡与参数](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UserControl/TaskQueue/FightTaskUserControlModel.cs)。

## 库存保持

1. 先单独跑完 ｢小工具 → 仓库识别｣ 或 ｢更新数据｣ 同步库存，再开始库存保持。手动消耗材料、合成、换号后重新确认缓存。
2. 添加 ｢库存保持｣，逐条填可导航关卡、掉落物和目标库存。可从预设或当前活动导入，再核对目标、顺序与药剂/源石预算。若开启按达成率排序，运行顺序会按预检结果变化。
3. WPF 先用缓存做预检，只为有缺口的计划排入作战；即使启用 ｢任务开始前更新库存数据｣，后来识别出的缺口也无法把已跳过的计划插回当前队列。同一轮把 ｢更新数据｣ 排前面，仍然是整轮预检先于游戏内识别。同步后重新运行可让新缓存参与预检。
4. 已排入的计划开始前会重算缺口。前一关掉落已补足、活动跳过开关开启，或符合无理智且无可用补充手段的条件，都可能正常跳过。检查日志中的具体原因。当前没有库存数据时也可能直接跳过。
5. 若只想补单一材料，也可在 ｢理智作战 → 指定材料｣ 中用 ｢目标库存｣。若只想刷到本轮掉落数量，使用普通指定材料模式。

依据：[库存保持](https://docs.maa.plus/zh-cn/manual/introduction/depot-maintain.html) · [预检、仓库更新和运行中复核](https://github.com/MaaAssistantArknights/MaaAssistantArknights/blob/v6.21.3/src/MaaWpfGui/ViewModels/UserControl/TaskQueue/DepotMaintainTaskUserControlModel.cs)。
