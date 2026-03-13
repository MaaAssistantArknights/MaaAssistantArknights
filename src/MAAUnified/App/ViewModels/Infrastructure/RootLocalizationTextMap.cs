using System.Text.RegularExpressions;
using MAAUnified.Application.Services.Localization;

namespace MAAUnified.App.ViewModels.Infrastructure;

public sealed class RootLocalizationTextMap : ObservableObject
{
    private static readonly Regex InlineKeyPattern = new(@"\{key=([^}]+)\}", RegexOptions.Compiled);

    private static readonly Dictionary<string, string> ZhCn = new(StringComparer.OrdinalIgnoreCase)
    {
        ["Main.Update.VersionAvailable"] = "版本更新可用，点击设置 > Version Update",
        ["Main.Update.ResourceAvailable"] = "资源更新可用，点击检查资源",
        ["Main.Title.UpdateVersion"] = "版本更新可用",
        ["Main.Title.UpdateResource"] = "资源更新可用",
        ["Main.Tray.Button"] = "托盘菜单",
        ["Main.Blocking.Title"] = "配置校验阻断中（{0}）: Start/LinkStart 已禁用",
        ["Main.Connection.Label"] = "连接:",
        ["Main.Connection.Button"] = "连接",
        ["Main.Import.Button"] = "导入旧配置",
        ["Main.ImportSource.Auto"] = "自动(gui.new + gui)",
        ["Main.ImportSource.GuiNewOnly"] = "仅 gui.new.json",
        ["Main.ImportSource.GuiOnly"] = "仅 gui.json",
        ["Main.Tab.TaskQueue"] = "一键长草",
        ["Main.Tab.Copilot"] = "自动战斗",
        ["Main.Tab.Toolbox"] = "小工具",
        ["Main.Tab.Settings"] = "设置",
        ["Main.Sidebar.GrowlAndCapability"] = "消息与能力状态",
        ["Main.Footer.ImportStatus"] = "导入状态",
        ["Main.Footer.RootLogs"] = "主日志",
        ["Main.Menu.Start"] = "开始",
        ["Main.Menu.Stop"] = "停止",
        ["Main.Menu.SwitchLanguage"] = "切换语言",
        ["Main.Menu.ForceShow"] = "强制显示",
        ["Main.Menu.HideTray"] = "隐藏托盘",
        ["Main.Menu.ToggleOverlay"] = "切换悬浮窗",
        ["Main.Menu.Restart"] = "重启",
        ["Main.Menu.Exit"] = "退出",
        ["Main.Menu.Language.zh-cn"] = "简体中文 (zh-cn)",
        ["Main.Menu.Language.en-us"] = "English (en-us)",
        ["Main.Menu.Language.ja-jp"] = "日本語 (ja-jp)",
        ["Main.Menu.Language.ko-kr"] = "한국어 (ko-kr)",
        ["Main.Menu.Language.zh-tw"] = "繁體中文 (zh-tw)",
        ["Main.Menu.Language.pallas"] = "Pallas (pallas)",
        ["Main.Growl.ManualVersionUpdate"] = "手动更新入口：设置 > Version Update",
        ["Main.Growl.ManualResourceUpdate"] = "手动资源更新入口：设置 > Version Update",

        ["TaskQueue.Root.DailyStageLabel"] = "今日关卡小提示:",
        ["TaskQueue.Root.DailyStageTooltip"] = "可通过 ｢小工具-仓库识别｣ 更新库存数据",
        ["TaskQueue.Root.AdvancedMode"] = "高级模式",
        ["TaskQueue.Root.AutoReload"] = "自动重载",
        ["TaskQueue.Root.TaskListTitle"] = "任务列表",
        ["TaskQueue.Root.Start"] = "开始",
        ["TaskQueue.Root.LinkStart"] = "Link Start!",
        ["TaskQueue.Root.Stop"] = "停止",
        ["TaskQueue.Root.WaitAndStop"] = "等待并停止",
        ["TaskQueue.Root.Waiting"] = "等待中...",
        ["TaskQueue.Root.LogsTitle"] = "日志",
        ["TaskQueue.Root.OverlayButton"] = "Overlay",
        ["TaskQueue.Root.TaskMenuIcon"] = "⚙",
        ["TaskQueue.Root.AddTaskIcon"] = "+",
        ["TaskQueue.Root.PostActionTitle"] = "完成后",
        ["TaskQueue.Root.PostActionTitleWithOnce"] = "{0}（{1}）",
        ["TaskQueue.Root.PostActionNone"] = "无",
        ["TaskQueue.Root.Add"] = "添加",
        ["TaskQueue.Root.NewTaskWatermark"] = "新任务名称（空则用模块名）",
        ["TaskQueue.Root.SelectAll"] = "全选",
        ["TaskQueue.Root.Inverse"] = "反选",
        ["TaskQueue.Root.Clear"] = "清空",
        ["TaskQueue.Root.SwitchBatchMode"] = "切换为{0}",
        ["TaskQueue.Root.MoveUp"] = "上移",
        ["TaskQueue.Root.MoveDown"] = "下移",
        ["TaskQueue.Root.Rename"] = "重命名",
        ["TaskQueue.Root.Delete"] = "删除",
        ["TaskQueue.Root.LeftClick"] = "左键",
        ["TaskQueue.Root.RightClick"] = "右键",
        ["TaskQueue.Root.TaskSettings"] = "任务设置",
        ["TaskQueue.Root.TaskConfigTitle"] = "任务配置",
        ["TaskQueue.Root.RenameWatermark"] = "重命名输入框（选中任务）",
        ["TaskQueue.Root.SaveConfig"] = "保存配置",
        ["TaskQueue.Root.RuntimeTitle"] = "运行与 Overlay",
        ["TaskQueue.Root.PickTarget"] = "选择目标",
        ["TaskQueue.Root.ToggleOverlay"] = "Overlay 开关",
        ["TaskQueue.Root.ReloadTargets"] = "刷新目标",
        ["TaskQueue.Root.GeneralSettings"] = "常规设置",
        ["TaskQueue.Root.AdvancedSettings"] = "高级设置",
        ["TaskQueue.Module.StartUp"] = "开始唤醒",
        ["TaskQueue.Module.Fight"] = "理智作战",
        ["TaskQueue.Module.Recruit"] = "自动公招",
        ["TaskQueue.Module.Infrast"] = "基建换班",
        ["TaskQueue.Module.Mall"] = "信用收支",
        ["TaskQueue.Module.Award"] = "领取奖励",
        ["TaskQueue.Module.Roguelike"] = "自动肉鸽",
        ["TaskQueue.Module.Reclamation"] = "生息演算",
        ["TaskQueue.Module.Custom"] = "自定义任务",
        ["TaskQueue.Module.PostAction"] = "完成后",
        ["TaskQueue.Status.Idle"] = "未开始",
        ["TaskQueue.Status.Running"] = "运行中",
        ["TaskQueue.Status.Success"] = "已完成",
        ["TaskQueue.Status.Error"] = "错误",
        ["TaskQueue.Status.Skipped"] = "已跳过",
        ["TaskQueue.Status.Observed"] = "已记录",
        ["TaskQueue.Log.TaskStart"] = "开始任务: {0}",
        ["TaskQueue.Log.SubTaskRunning"] = "{0}: {1} 进行中",
        ["TaskQueue.Log.SubTaskCompleted"] = "{0}: {1} 已完成",
        ["TaskQueue.Log.TaskCompleted"] = "任务完成: {0}",
        ["TaskQueue.Log.TaskError"] = "任务失败: {0}",
        ["TaskQueue.Log.SubTaskError"] = "{0}: {1} 失败",
        ["TaskQueue.Log.TaskStopped"] = "任务已停止",
        ["TaskQueue.Log.AllCompleted"] = "全部任务完成",
        ["TaskQueue.Log.Observed"] = "{0}: {1}",
        ["Operator"] = "干员",
        ["Series"] = "代理倍率",
        ["TaskError"] = "任务出错: ",
        ["CombatError"] = "战斗出错",
        ["StartTask"] = "开始任务: ",
        ["CompleteTask"] = "完成任务: ",
        ["StartCombat"] = "开始战斗: ",
        ["CompleteCombat"] = "完成战斗",
        ["AllTasksComplete"] = "任务已全部完成！\n(用时 {0})",
        ["SanityReport"] = "理智将在 {DateTime} 回满。({TimeDiff} 后)",
        ["ConnectingToEmulator"] = "正在连接模拟器……",
        ["Running"] = "正在运行中……",
        ["StartsWithScript"] = "开始前脚本",
        ["FailedToOpenClient"] = "打开客户端失败，请检查配置文件",
        ["CloseArknightsFailed"] = "关闭游戏失败",
        ["ErrorOccurred"] = "出现错误",
        ["HasReturned"] = "已返回",
        ["DropRecognitionError"] = "掉落识别错误",
        ["GiveUpUploadingPenguins"] = "放弃上传企鹅物流",
        ["TheEx"] = "无奖励关卡，已停止",
        ["MissingOperators"] = "缺少以下{key=Operator}组中的{key=Operator}：",
        ["MissionStart"] = "已开始行动",
        ["MissionStart.FightTask"] = "开始行动 {0} 次, -{1}理智",
        ["UnitTime"] = "次",
        ["CurrentSanity"] = "理智: {0}/{1}  ",
        ["MedicineUsedTimes"] = "药: {0}  ",
        ["MedicineUsedTimesWithExpiring"] = "药: {0},{1}(临期)  ",
        ["StoneUsedTimes"] = "石: {0}  ",
        ["MedicineUsed"] = "已使用理智药",
        ["ExpiringMedicineUsed"] = "已使用 48 小时内过期的理智药",
        ["StoneUsed"] = "已使用源石",
        ["ActingCommandError"] = "代理指挥失误",
        ["FightMissionFailedAndStop"] = "代理失败次数已达上限，任务已停止",
        ["LabelsRefreshed"] = "已刷新标签",
        ["RecruitConfirm"] = "已确认招募",
        ["InfrastDormDoubleConfirmed"] = "{key=Operator}冲突",
        ["ClueExchangeUnlocked"] = "已开启线索交流",
        ["BegunToExplore"] = "已开始探索",
        ["RoutingRestartTooManyBattles"] = "前方战斗数：{0}，重开路线",
        ["ExplorationAbandoned"] = "已放弃本次探索",
        ["FightCompleted"] = "战斗完成",
        ["FightFailed"] = "战斗失败",
        ["FightTimesUnused"] = "已完成 {0} 次战斗, 下次将执行 {1} 倍率代理, 进入后将完成 {2} 次战斗, 超过 {3} 次限制, 不进入战斗",
        ["UpperLimit"] = "投资达到上限",
        ["GameDrop"] = "游戏掉线，重新连接",
        ["GameDropNoRestart"] = "游戏掉线，不重新连接，停止任务",
        ["RoguelikeGamePass"] = "肉鸽通关啦！",
        ["RoguelikeSpecialItemBought"] = "购买了特殊商品！",
        ["DeepExplorationNotUnlockedComplain"] = "深入调查未解锁",
        ["Trader"] = "节点: 诡意行商",
        ["SafeHouse"] = "节点: 安全的角落",
        ["FilterTruth"] = "节点: 去伪存真",
        ["CombatOps"] = "关卡: 普通作战",
        ["EmergencyOps"] = "关卡: 紧急作战",
        ["DreadfulFoe"] = "关卡: 险路恶敌",
        ["CurTimes"] = "当前次数",
        ["NoDrop"] = "无",
        ["TotalDrop"] = "掉落统计: ",
        ["FurnitureDrop"] = "家具",
        ["ThisFacility"] = "当前设施: ",
        ["RoomGroupsMatch"] = "匹配编组: ",
        ["RoomGroupsMatchFailed"] = "匹配{key=Operator}编组失败，编组列表: ",
        ["RoomOperators"] = "首选{key=Operator}: ",
        ["ProductIncorrect"] = "产物与配置不相符。",
        ["ProductUnknown"] = "无法识别的产物",
        ["ProductChanged"] = "产物已切换",
        ["RecruitingResults"] = "公招识别结果: ",
        ["Choose"] = "选择",
        ["Refreshed"] = "当前槽位已刷新",
        ["ContinueRefresh"] = "无招聘许可，继续尝试刷新 Tags",
        ["NoRecruitmentPermit"] = "无招聘许可，已返回",
        ["NotEnoughStaff"] = "可用{key=Operator}不足",
        ["CreditFullOnlyBuyDiscount"] = "博士！ \n只购买折扣商品让信用点数溢出了！\n小心信用点数浪费哦！\n剩余信用点数：",
        ["StageInfoError"] = "关卡识别错误",
        ["TrainingIdle"] = "训练室空闲中",
        ["TrainingLevel"] = "专精等级",
        ["TrainingCompleted"] = "训练完成",
        ["TrainingTimeLeft"] = "剩余时间",
        ["AlgorithmFinish"] = "演算结束",
        ["AlgorithmBadge"] = "繁荣证章",
        ["AlgorithmConstructionPoint"] = "建造点数",
        ["ReclamationPnsModeError"] = "当前使用无存档任务模式。请手动删除现有存档后再尝试。若您希望保留存档，请考虑切换模式。",
        ["ReclamationPisModeError"] = "当前任务模式需要您拥有可合成道具的存档。请考虑切换模式。",
        ["CreditFight"] = "借助战打 OF-1 赚信用",
        ["Visiting"] = "访问好友",
        ["BattleFormation"] = "开始编队",
        ["BattleFormationParseFailed"] = "编队解析失败",
        ["BattleFormationSelected"] = "选择{key=Operator}: ",
        ["BattleFormationOperUnavailable"] = "{key=Operator}不可用: {0}, 原因: {1}",
        ["BattleFormationOperUnavailable.Elite"] = "精英化不足",
        ["BattleFormationOperUnavailable.Level"] = "等级不足",
        ["BattleFormationOperUnavailable.SkillLevel"] = "技能等级不足",
        ["BattleFormationOperUnavailable.Module"] = "所需模组未解锁",
        ["CopilotUserAdditionalNameInvalid"] = "追加自定{key=Operator}名称无效: {0}，请检查拼写",
        ["CurrentSteps"] = "当前步骤: {0} {1}",
        ["ElapsedTime"] = "当前计时器: {0}ms",
        ["CurrentStage"] = "当前关卡: {0}",
        ["SSSGamePass"] = "保全通关啦！",
        ["UnsupportedLevel"] = "不支持的关卡，请检查关卡名或前往资源更新后重试！",
        ["Deploy"] = "部署",
        ["UseSkill"] = "使用技能",
        ["Retreat"] = "撤退",
        ["SkillDaemon"] = "开始挂机",
        ["SwitchSpeed"] = "切换倍速",
        ["SkillUsage"] = "切换技能用法",
        ["BulletTime"] = "进入子弹时间",

        ["Settings.Root.SectionTitle"] = "设置段落",
        ["Settings.Root.SectionActionTitle"] = "节级动作",
        ["Settings.Root.StatusTitle"] = "执行状态",
        ["Settings.Section.ConfigurationManager"] = "切换配置",
        ["Settings.Section.Timer"] = "定时执行",
        ["Settings.Section.Performance"] = "性能设置",
        ["Settings.Section.Game"] = "运行设置",
        ["Settings.Section.Connect"] = "连接设置",
        ["Settings.Section.Start"] = "启动设置",
        ["Settings.Section.RemoteControl"] = "远程控制",
        ["Settings.Section.GUI"] = "界面设置",
        ["Settings.Section.Background"] = "背景设置",
        ["Settings.Section.ExternalNotification"] = "外部通知",
        ["Settings.Section.HotKey"] = "热键设置",
        ["Settings.Section.Achievement"] = "成就设置",
        ["Settings.Section.VersionUpdate"] = "更新设置",
        ["Settings.Section.IssueReport"] = "问题反馈",
        ["Settings.Section.About"] = "关于我们",
        ["Settings.Action.SaveGui"] = "保存 GUI 设置",
        ["Settings.Action.SaveConnectionGame"] = "保存连接/游戏",
        ["Settings.Action.SaveStartPerformance"] = "保存启动/性能",
        ["Settings.Action.SaveTimer"] = "保存定时",
        ["Settings.Action.SaveRemote"] = "保存远程控制",
        ["Settings.Action.TestRemote"] = "测试远程连通",
        ["Settings.Action.RegisterHotkeys"] = "注册热键",
        ["Settings.Action.ValidateNotification"] = "校验通知参数",
        ["Settings.Action.TestNotification"] = "测试通知发送",
        ["Settings.Action.SaveNotification"] = "保存通知配置",
        ["Settings.Action.SaveVersionUpdate"] = "保存更新设置",
        ["Settings.Action.CheckVersionUpdate"] = "检查更新",
        ["Settings.Action.SaveAchievement"] = "保存成就设置",
        ["Settings.Action.RefreshAchievement"] = "刷新成就策略",
        ["Settings.Action.ShowAchievement"] = "查看成就列表",
        ["Settings.Action.OpenAchievementGuide"] = "打开成就说明",
        ["Settings.Action.BuildIssueReport"] = "生成问题包",
        ["Settings.Action.OpenDebugDirectory"] = "打开 debug 目录",
        ["Settings.Action.ClearImageCache"] = "清理图像缓存",
        ["Settings.Action.CheckAnnouncement"] = "检查公告",
        ["Settings.Action.OpenOfficial"] = "官网",
        ["Settings.Action.OpenCommunity"] = "社区",
        ["Settings.Action.OpenDownload"] = "下载页",
        ["Settings.Action.RefreshProfiles"] = "刷新配置列表",
        ["Settings.State.Saving"] = "正在保存...",
        ["Settings.State.Saved"] = "保存成功",
        ["Settings.Performance.Gpu.SectionTitle"] = "GPU 设置",
        ["Settings.Performance.Gpu.Enable"] = "启用 GPU OCR",
        ["Settings.Performance.Gpu.Select"] = "GPU 选择",
        ["Settings.Performance.Gpu.AllowDeprecated"] = "允许使用已弃用显卡",
        ["Settings.Performance.Gpu.RestartRequired"] = "修改后通常需要重启应用生效。",
        ["Settings.Performance.Gpu.Option.Disabled"] = "不使用",
        ["Settings.Performance.Gpu.Option.SystemDefault"] = "系统默认 GPU",
        ["Settings.Performance.Gpu.Option.Custom"] = "自定义 GPU",
        ["Settings.Performance.Gpu.CustomDescription"] = "GPU 描述",
        ["Settings.Performance.Gpu.CustomInstancePath"] = "GPU 实例路径",
        ["Settings.Performance.Gpu.Status.WindowsReady"] = "当前 Windows 构建已支持 GPU OCR，设置将在重启后生效。",
        ["Settings.Performance.Gpu.Status.Unsupported"] = "当前平台暂不支持 GPU OCR，只有 Windows 构建会应用该设置。",
        ["Settings.Performance.Gpu.Warning.Deprecated"] = "当前所选显卡已被标记为弃用，建议优先使用更新的显卡或 CPU OCR。",
        ["Settings.Performance.Gpu.Warning.OutdatedDriver"] = "当前显卡驱动过旧，DirectML 兼容性可能不足。",
        ["Settings.Performance.Gpu.Warning.Unsupported"] = "当前构建不会在此平台上应用 GPU OCR。",
        ["Settings.Performance.Gpu.Warning.SelectionFallback"] = "已保存的 GPU 选择在当前环境不可用，已自动回退到安全选项。",
    };

    private static readonly Dictionary<string, string> EnUs = new(StringComparer.OrdinalIgnoreCase)
    {
        ["Main.Update.VersionAvailable"] = "Version update available. See Settings > Version Update",
        ["Main.Update.ResourceAvailable"] = "Resource update available. Run resource check",
        ["Main.Title.UpdateVersion"] = "Version update available",
        ["Main.Title.UpdateResource"] = "Resource update available",
        ["Main.Tray.Button"] = "Tray Menu",
        ["Main.Blocking.Title"] = "Config validation blocked ({0}): Start/LinkStart disabled",
        ["Main.Connection.Label"] = "Connect:",
        ["Main.Connection.Button"] = "Connect",
        ["Main.Import.Button"] = "Import Legacy",
        ["Main.ImportSource.Auto"] = "Auto (gui.new + gui)",
        ["Main.ImportSource.GuiNewOnly"] = "gui.new.json only",
        ["Main.ImportSource.GuiOnly"] = "gui.json only",
        ["Main.Tab.TaskQueue"] = "Farming",
        ["Main.Tab.Copilot"] = "Copilot",
        ["Main.Tab.Toolbox"] = "Toolbox",
        ["Main.Tab.Settings"] = "Settings",
        ["Main.Sidebar.GrowlAndCapability"] = "Messages & Capability",
        ["Main.Footer.ImportStatus"] = "Import Status",
        ["Main.Footer.RootLogs"] = "Main Logs",
        ["Main.Menu.Start"] = "Start",
        ["Main.Menu.Stop"] = "Stop",
        ["Main.Menu.SwitchLanguage"] = "Switch Language",
        ["Main.Menu.ForceShow"] = "Force Show",
        ["Main.Menu.HideTray"] = "Hide Tray",
        ["Main.Menu.ToggleOverlay"] = "Toggle Overlay",
        ["Main.Menu.Restart"] = "Restart",
        ["Main.Menu.Exit"] = "Exit",
        ["Main.Menu.Language.zh-cn"] = "简体中文 (zh-cn)",
        ["Main.Menu.Language.en-us"] = "English (en-us)",
        ["Main.Menu.Language.ja-jp"] = "日本語 (ja-jp)",
        ["Main.Menu.Language.ko-kr"] = "한국어 (ko-kr)",
        ["Main.Menu.Language.zh-tw"] = "繁體中文 (zh-tw)",
        ["Main.Menu.Language.pallas"] = "Pallas (pallas)",
        ["Main.Growl.ManualVersionUpdate"] = "Manual update entry: Settings > Version Update",
        ["Main.Growl.ManualResourceUpdate"] = "Manual resource update: Settings > Version Update",

        ["TaskQueue.Root.DailyStageLabel"] = "Today's open stages:",
        ["TaskQueue.Root.DailyStageTooltip"] = "Inventory data can be updated via  ｢Toolbox-Depot Recognition｣",
        ["TaskQueue.Root.AdvancedMode"] = "Advanced",
        ["TaskQueue.Root.AutoReload"] = "Auto reload",
        ["TaskQueue.Root.TaskListTitle"] = "Task List",
        ["TaskQueue.Root.Start"] = "Start",
        ["TaskQueue.Root.LinkStart"] = "Link Start!",
        ["TaskQueue.Root.Stop"] = "Stop",
        ["TaskQueue.Root.WaitAndStop"] = "Wait & Stop",
        ["TaskQueue.Root.Waiting"] = "Waiting...",
        ["TaskQueue.Root.LogsTitle"] = "Logs",
        ["TaskQueue.Root.OverlayButton"] = "Overlay",
        ["TaskQueue.Root.TaskMenuIcon"] = "⚙",
        ["TaskQueue.Root.AddTaskIcon"] = "+",
        ["TaskQueue.Root.PostActionTitle"] = "After Completion",
        ["TaskQueue.Root.PostActionTitleWithOnce"] = "{0} ({1})",
        ["TaskQueue.Root.PostActionNone"] = "Do nothing",
        ["TaskQueue.Root.Add"] = "Add",
        ["TaskQueue.Root.NewTaskWatermark"] = "New task name (fallback to module)",
        ["TaskQueue.Root.SelectAll"] = "Select all",
        ["TaskQueue.Root.Inverse"] = "Inverse",
        ["TaskQueue.Root.Clear"] = "Clear",
        ["TaskQueue.Root.SwitchBatchMode"] = "Switch to {0}",
        ["TaskQueue.Root.MoveUp"] = "Move up",
        ["TaskQueue.Root.MoveDown"] = "Move down",
        ["TaskQueue.Root.Rename"] = "Rename",
        ["TaskQueue.Root.Delete"] = "Delete",
        ["TaskQueue.Root.LeftClick"] = "Left click",
        ["TaskQueue.Root.RightClick"] = "Right click",
        ["TaskQueue.Root.TaskSettings"] = "Task settings",
        ["TaskQueue.Root.TaskConfigTitle"] = "Task Config",
        ["TaskQueue.Root.RenameWatermark"] = "Rename selected task",
        ["TaskQueue.Root.SaveConfig"] = "Save",
        ["TaskQueue.Root.RuntimeTitle"] = "Runtime & Overlay",
        ["TaskQueue.Root.PickTarget"] = "Pick target",
        ["TaskQueue.Root.ToggleOverlay"] = "Toggle overlay",
        ["TaskQueue.Root.ReloadTargets"] = "Reload targets",
        ["TaskQueue.Root.GeneralSettings"] = "General",
        ["TaskQueue.Root.AdvancedSettings"] = "Advanced",
        ["TaskQueue.Module.StartUp"] = "Start Up",
        ["TaskQueue.Module.Fight"] = "Combat",
        ["TaskQueue.Module.Recruit"] = "Recruit",
        ["TaskQueue.Module.Infrast"] = "Infrast",
        ["TaskQueue.Module.Mall"] = "Mall",
        ["TaskQueue.Module.Award"] = "Award",
        ["TaskQueue.Module.Roguelike"] = "Roguelike",
        ["TaskQueue.Module.Reclamation"] = "Reclamation",
        ["TaskQueue.Module.Custom"] = "Custom",
        ["TaskQueue.Module.PostAction"] = "Post Action",
        ["TaskQueue.Status.Idle"] = "Idle",
        ["TaskQueue.Status.Running"] = "Running",
        ["TaskQueue.Status.Success"] = "Completed",
        ["TaskQueue.Status.Error"] = "Error",
        ["TaskQueue.Status.Skipped"] = "Skipped",
        ["TaskQueue.Status.Observed"] = "Observed",
        ["TaskQueue.Log.TaskStart"] = "Task started: {0}",
        ["TaskQueue.Log.SubTaskRunning"] = "{0}: {1} running",
        ["TaskQueue.Log.SubTaskCompleted"] = "{0}: {1} completed",
        ["TaskQueue.Log.TaskCompleted"] = "Task completed: {0}",
        ["TaskQueue.Log.TaskError"] = "Task failed: {0}",
        ["TaskQueue.Log.SubTaskError"] = "{0}: {1} failed",
        ["TaskQueue.Log.TaskStopped"] = "Task stopped",
        ["TaskQueue.Log.AllCompleted"] = "All tasks completed",
        ["TaskQueue.Log.Observed"] = "{0}: {1}",
        ["Operator"] = "Operator",
        ["Series"] = "Series",
        ["TaskError"] = "Task error: ",
        ["CombatError"] = "Combat error",
        ["StartTask"] = "Start task: ",
        ["CompleteTask"] = "Complete task: ",
        ["StartCombat"] = "Start combat: ",
        ["CompleteCombat"] = "Complete combat",
        ["AllTasksComplete"] = "All task(s) completed!\n(in {0})",
        ["SanityReport"] = "Sanity will be full at {DateTime} (in {TimeDiff})",
        ["ConnectingToEmulator"] = "Connecting to emulator……",
        ["Running"] = "Running……",
        ["StartsWithScript"] = "Starts with Script",
        ["FailedToOpenClient"] = "Failed to open the client. Please check the configuration file",
        ["CloseArknightsFailed"] = "Shutdown Arknights failed",
        ["ErrorOccurred"] = "Error occurred",
        ["HasReturned"] = "Has returned",
        ["DropRecognitionError"] = "Drops recognition error",
        ["GiveUpUploadingPenguins"] = "Abort upload to Penguin Statistics",
        ["TheEx"] = "No bonus stage, stopped",
        ["MissingOperators"] = "{key=Operator}s of the following {key=Operator} groups are missing: ",
        ["MissionStart"] = "Mission started",
        ["MissionStart.FightTask"] = "Mission started {0} times (-{1} Sanity)",
        ["UnitTime"] = "times",
        ["CurrentSanity"] = "Sanity: {0}/{1}  ",
        ["MedicineUsedTimes"] = "Medicine: {0}  ",
        ["MedicineUsedTimesWithExpiring"] = "Medicine: {0},{1}(Expiring)  ",
        ["StoneUsedTimes"] = "Stone: {0}  ",
        ["MedicineUsed"] = "Medicine used",
        ["ExpiringMedicineUsed"] = "Expiring medicine used",
        ["StoneUsed"] = "Originite Prime used",
        ["ActingCommandError"] = "PRTS error",
        ["FightMissionFailedAndStop"] = "Proxy failed too many times, task stopped",
        ["LabelsRefreshed"] = "Labels refreshed",
        ["RecruitConfirm"] = "Recruit confirm",
        ["InfrastDormDoubleConfirmed"] = "{key=Operator} conflict",
        ["ClueExchangeUnlocked"] = "Clue Exchange Unlocked",
        ["BegunToExplore"] = "Exploration started",
        ["RoutingRestartTooManyBattles"] = "Too many battles ahead: {0}, restarting route",
        ["ExplorationAbandoned"] = "Abandoned this Exploration",
        ["FightCompleted"] = "Combat completed",
        ["FightFailed"] = "Combat failed",
        ["FightTimesUnused"] = "Completed {0} battles, will execute {1} multiplier proxy next time, will complete {2} battles after entering, exceeds {3} limit, will not enter battle",
        ["UpperLimit"] = "Investment limit reached",
        ["GameDrop"] = "Game disconnected, pending reconnect",
        ["GameDropNoRestart"] = "Game disconnected, not restarting, stopping",
        ["RoguelikeGamePass"] = "Exploration completed! Congratulations!",
        ["RoguelikeSpecialItemBought"] = "Special Item Purchased!",
        ["DeepExplorationNotUnlockedComplain"] = "Deep Investigation not unlocked yet",
        ["Trader"] = "Node: Rogue Trader",
        ["SafeHouse"] = "Node: Safe House",
        ["FilterTruth"] = "Node: Idea Filter",
        ["CombatOps"] = "Node: Combat Operation",
        ["EmergencyOps"] = "Stage: Emergency Operation",
        ["DreadfulFoe"] = "Stage: Dreadful Foe",
        ["CurTimes"] = "Current times",
        ["NoDrop"] = "Nothing",
        ["TotalDrop"] = "Total Drops: ",
        ["FurnitureDrop"] = "Furniture",
        ["ThisFacility"] = "Current Facility: ",
        ["RoomGroupsMatch"] = "Match Group: ",
        ["RoomGroupsMatchFailed"] = "Failed to match {key=Operator} groups, group list: ",
        ["RoomOperators"] = "Preferred {key=Operator}s: ",
        ["ProductIncorrect"] = "Product does NOT match the configuration.",
        ["ProductUnknown"] = "Unknown Product",
        ["ProductChanged"] = "Product has changed",
        ["RecruitingResults"] = "Recruitment Results: ",
        ["Choose"] = "Choose",
        ["Refreshed"] = "Refreshed",
        ["ContinueRefresh"] = "No recruitment permit, trying to refresh Tags",
        ["NoRecruitmentPermit"] = "No recruitment permit, returned",
        ["NotEnoughStaff"] = "Insufficient {key=Operator}s",
        ["CreditFullOnlyBuyDiscount"] = "Doctor! \nPurchasing only discounted items has caused an overflow in credits! \nBe cautious of wasting credits!\nRemaining credits: ",
        ["StageInfoError"] = "Stage recognition error",
        ["TrainingIdle"] = "Training room is vacant",
        ["TrainingLevel"] = "Skill Rank",
        ["TrainingCompleted"] = "Training completed",
        ["TrainingTimeLeft"] = "Remaining Time",
        ["AlgorithmFinish"] = "Algorithm Finish",
        ["AlgorithmBadge"] = "Algorithm Badge",
        ["AlgorithmConstructionPoint"] = "Algorithm Construction Point",
        ["ReclamationPnsModeError"] = "Currently using no-save task mode. Please manually delete existing save and try again. If you wish to keep the save, please consider switching modes.",
        ["ReclamationPisModeError"] = "Current task mode requires you to have a save with craftable items. Please consider switching modes.",
        ["CreditFight"] = "Combat with Support to earn Credits",
        ["Visiting"] = "Visit Friends",
        ["BattleFormation"] = "Start formation",
        ["BattleFormationParseFailed"] = "Formation parse failed",
        ["BattleFormationSelected"] = "Selected: ",
        ["BattleFormationOperUnavailable"] = "{key=Operator} unavailable: {0}, reason: {1}",
        ["BattleFormationOperUnavailable.Elite"] = "Elite too low",
        ["BattleFormationOperUnavailable.Level"] = "Level too low",
        ["BattleFormationOperUnavailable.SkillLevel"] = "Skill level too low",
        ["BattleFormationOperUnavailable.Module"] = "Required module not unlocked",
        ["CopilotUserAdditionalNameInvalid"] = "Additional custom {key=Operator} name invalid: {0}, please check spelling",
        ["CurrentSteps"] = "Step: {0} {1}",
        ["ElapsedTime"] = "Elapsed time: {0}ms",
        ["CurrentStage"] = "Current Stage: {0}",
        ["SSSGamePass"] = "Game cleared! congratulations!",
        ["UnsupportedLevel"] = "Unsupported stage, please check the stage name or update resources and try again!",
        ["Deploy"] = "Deploy",
        ["UseSkill"] = "Use Skill",
        ["Retreat"] = "Retreat",
        ["SkillDaemon"] = "Start Auto",
        ["SwitchSpeed"] = "Switch Speed",
        ["SkillUsage"] = "Switch Skill Usage",
        ["BulletTime"] = "Enter Bullet Time",

        ["Settings.Root.SectionTitle"] = "Settings Sections",
        ["Settings.Root.SectionActionTitle"] = "Section Actions",
        ["Settings.Root.StatusTitle"] = "Status",
        ["Settings.Section.ConfigurationManager"] = "Switch Configuration",
        ["Settings.Section.Timer"] = "Schedule",
        ["Settings.Section.Performance"] = "Performance",
        ["Settings.Section.Game"] = "Game Settings",
        ["Settings.Section.Connect"] = "Connection",
        ["Settings.Section.Start"] = "Startup",
        ["Settings.Section.RemoteControl"] = "Remote Control",
        ["Settings.Section.GUI"] = "GUI",
        ["Settings.Section.Background"] = "Background Setting",
        ["Settings.Section.ExternalNotification"] = "External notifications",
        ["Settings.Section.HotKey"] = "HotKeys",
        ["Settings.Section.Achievement"] = "Achievements",
        ["Settings.Section.VersionUpdate"] = "Update",
        ["Settings.Section.IssueReport"] = "Issue Report",
        ["Settings.Section.About"] = "About us",
        ["Settings.Action.SaveGui"] = "Save GUI",
        ["Settings.Action.SaveConnectionGame"] = "Save Connection/Game",
        ["Settings.Action.SaveStartPerformance"] = "Save Start/Performance",
        ["Settings.Action.SaveTimer"] = "Save Timer",
        ["Settings.Action.SaveRemote"] = "Save Remote",
        ["Settings.Action.TestRemote"] = "Test Connectivity",
        ["Settings.Action.RegisterHotkeys"] = "Register Hotkeys",
        ["Settings.Action.ValidateNotification"] = "Validate Notification",
        ["Settings.Action.TestNotification"] = "Test Notification",
        ["Settings.Action.SaveNotification"] = "Save Notification",
        ["Settings.Action.SaveVersionUpdate"] = "Save Update Settings",
        ["Settings.Action.CheckVersionUpdate"] = "Check Updates",
        ["Settings.Action.SaveAchievement"] = "Save Achievement",
        ["Settings.Action.RefreshAchievement"] = "Refresh Achievement",
        ["Settings.Action.ShowAchievement"] = "Achievement List",
        ["Settings.Action.OpenAchievementGuide"] = "Open Guide",
        ["Settings.Action.BuildIssueReport"] = "Build Issue Bundle",
        ["Settings.Action.OpenDebugDirectory"] = "Open Debug Dir",
        ["Settings.Action.ClearImageCache"] = "Clear Image Cache",
        ["Settings.Action.CheckAnnouncement"] = "Check Announcement",
        ["Settings.Action.OpenOfficial"] = "Official Site",
        ["Settings.Action.OpenCommunity"] = "Community",
        ["Settings.Action.OpenDownload"] = "Downloads",
        ["Settings.Action.RefreshProfiles"] = "Refresh Profiles",
        ["Settings.State.Saving"] = "Saving...",
        ["Settings.State.Saved"] = "Saved",
        ["Settings.Performance.Gpu.SectionTitle"] = "GPU Settings",
        ["Settings.Performance.Gpu.Enable"] = "Enable GPU OCR",
        ["Settings.Performance.Gpu.Select"] = "GPU selection",
        ["Settings.Performance.Gpu.AllowDeprecated"] = "Allow deprecated GPUs",
        ["Settings.Performance.Gpu.RestartRequired"] = "Changes usually take effect after restarting the app.",
        ["Settings.Performance.Gpu.Option.Disabled"] = "Do not use GPU",
        ["Settings.Performance.Gpu.Option.SystemDefault"] = "System default GPU",
        ["Settings.Performance.Gpu.Option.Custom"] = "Custom GPU",
        ["Settings.Performance.Gpu.CustomDescription"] = "GPU description",
        ["Settings.Performance.Gpu.CustomInstancePath"] = "GPU instance path",
        ["Settings.Performance.Gpu.Status.WindowsReady"] = "This Windows build supports GPU OCR and the setting will take effect after restart.",
        ["Settings.Performance.Gpu.Status.Unsupported"] = "GPU OCR is not supported on this platform. Only Windows builds apply this setting.",
        ["Settings.Performance.Gpu.Warning.Deprecated"] = "The selected GPU is marked as deprecated. Prefer a newer GPU or CPU OCR when possible.",
        ["Settings.Performance.Gpu.Warning.OutdatedDriver"] = "The selected GPU driver is outdated and may not be fully compatible with DirectML.",
        ["Settings.Performance.Gpu.Warning.Unsupported"] = "This build does not apply GPU OCR on the current platform.",
        ["Settings.Performance.Gpu.Warning.SelectionFallback"] = "The saved GPU selection is unavailable in the current environment and was reset to a safe fallback.",
    };

    private static readonly Dictionary<string, string> JaJp = new(EnUs, StringComparer.OrdinalIgnoreCase);
    private static readonly Dictionary<string, string> KoKr = new(EnUs, StringComparer.OrdinalIgnoreCase);
    private static readonly Dictionary<string, string> ZhTw = new(EnUs, StringComparer.OrdinalIgnoreCase);
    private static readonly Dictionary<string, string> Pallas = new(EnUs, StringComparer.OrdinalIgnoreCase);

    private readonly string _scope;
    private string _language = UiLanguageCatalog.DefaultLanguage;

    public RootLocalizationTextMap(string scope = "Root.Localization")
    {
        _scope = string.IsNullOrWhiteSpace(scope) ? "Root.Localization" : scope;
    }

    public event Action<LocalizationFallbackInfo>? FallbackReported;

    public string Language
    {
        get => _language;
        set
        {
            var normalized = UiLanguageCatalog.Normalize(value);
            if (!SetProperty(ref _language, normalized))
            {
                return;
            }

            OnPropertyChanged("Item[]");
        }
    }

    public string this[string key]
    {
        get
        {
            var normalizedLanguage = UiLanguageCatalog.Normalize(Language);
            var source = ResolveSource(normalizedLanguage);
            if (source.TryGetValue(key, out var value))
            {
                return ResolveInlineKeys(value, normalizedLanguage, [key]);
            }

            if (EnUs.TryGetValue(key, out value))
            {
                ReportFallback(normalizedLanguage, key, UiLanguageCatalog.FallbackLanguage);
                return ResolveInlineKeys(value, normalizedLanguage, [key]);
            }

            if (ZhCn.TryGetValue(key, out value))
            {
                ReportFallback(normalizedLanguage, key, UiLanguageCatalog.DefaultLanguage);
                return ResolveInlineKeys(value, normalizedLanguage, [key]);
            }

            ReportFallback(normalizedLanguage, key, "key");
            return key;
        }
    }

    public string GetOrDefault(string key, string fallback)
    {
        var value = this[key];
        return string.Equals(value, key, StringComparison.Ordinal) ? fallback : value;
    }

    private static Dictionary<string, string> ResolveSource(string language)
    {
        if (string.Equals(language, "en-us", StringComparison.OrdinalIgnoreCase))
        {
            return EnUs;
        }

        if (string.Equals(language, "ja-jp", StringComparison.OrdinalIgnoreCase))
        {
            return JaJp;
        }

        if (string.Equals(language, "ko-kr", StringComparison.OrdinalIgnoreCase))
        {
            return KoKr;
        }

        if (string.Equals(language, "zh-tw", StringComparison.OrdinalIgnoreCase))
        {
            return ZhTw;
        }

        if (string.Equals(language, "pallas", StringComparison.OrdinalIgnoreCase))
        {
            return Pallas;
        }

        return ZhCn;
    }

    private void ReportFallback(string language, string key, string fallbackSource)
    {
        FallbackReported?.Invoke(
            new LocalizationFallbackInfo(
                Scope: _scope,
                Language: language,
                Key: key,
                FallbackSource: fallbackSource));
    }

    private string ResolveInlineKeys(string value, string language, HashSet<string> resolvingKeys)
    {
        if (string.IsNullOrEmpty(value) || !value.Contains("{key=", StringComparison.Ordinal))
        {
            return value;
        }

        return InlineKeyPattern.Replace(
            value,
            match =>
            {
                var referencedKey = match.Groups[1].Value.Trim();
                if (string.IsNullOrWhiteSpace(referencedKey) || !resolvingKeys.Add(referencedKey))
                {
                    return match.Value;
                }

                try
                {
                    var referencedValue = LookupWithoutFallbackReport(language, referencedKey);
                    if (string.Equals(referencedValue, referencedKey, StringComparison.Ordinal))
                    {
                        return referencedKey;
                    }

                    return ResolveInlineKeys(referencedValue, language, resolvingKeys);
                }
                finally
                {
                    resolvingKeys.Remove(referencedKey);
                }
            });
    }

    private static string LookupWithoutFallbackReport(string language, string key)
    {
        var source = ResolveSource(language);
        if (source.TryGetValue(key, out var value))
        {
            return value;
        }

        if (EnUs.TryGetValue(key, out value))
        {
            return value;
        }

        if (ZhCn.TryGetValue(key, out value))
        {
            return value;
        }

        return key;
    }
}
