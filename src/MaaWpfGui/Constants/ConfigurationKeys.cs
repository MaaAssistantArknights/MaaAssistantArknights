// <copyright file="ConfigurationKeys.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2025 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using JetBrains.Annotations;

namespace MaaWpfGui.Constants;

/// <summary>
/// MaaWpfGui configuration keys
/// </summary>
public static class ConfigurationKeys
{
    public const string CurrentConfiguration = "Current";
    public const string DefaultConfiguration = "Default";
    public const string GlobalConfiguration = "Global";
    public const string ConfigurationMap = "Configurations";

    [UsedImplicitly]
    public const string ConfigurationData = "Data";

    [UsedImplicitly]
    public const string ConfigurationCron = "Cron";

    public const string Localization = "GUI.Localization";
    public const string OperNameLanguage = "GUI.OperNameLanguage";
    public const string UseTray = "GUI.UseTray";
    public const string MinimizeToTray = "GUI.MinimizeToTray";
    public const string HideCloseButton = "GUI.HideCloseButton";
    public const string WindowTitleScrollable = "GUI.WindowTitleScrollable";

    // public const string UseLogItemDateFormat = "GUI.UseLogItemDateFormat"; 是否使用自定义的时间格式
    public const string LogItemDateFormat = "GUI.LogItemDateFormatString";
    public const string WindowPlacement = "GUI.Placement";
    public const string LoadWindowPlacement = "GUI.Placement.Load";
    public const string SaveWindowPlacement = "GUI.Placement.SaveOnClosing";
    public const string UseAlternateStage = "GUI.UseAlternateStage"; // 已废弃
    public const string AllowUseStoneSave = "GUI.AllowUseStoneSave"; // 已迁移
    public const string HideUnavailableStage = "GUI.HideUnavailableStage";
    public const string HideSeries = "GUI.HideSeries"; // 已迁移
    public const string CustomStageCode = "GUI.CustomStageCode"; // 已迁移
    public const string InverseClearMode = "GUI.InverseClearMode";
    public const string WindowTitlePrefix = "GUI.WindowTitlePrefix";
    public const string MaxNumberOfLogThumbnails = "GUI.MaxNumberOfLogThumbnails";
    public const string UseCardLog = "GUI.UseCardLog";
    public const string WindowTitleSelectShowList = "GUI.WindowTitleSelectShowList";
    public const string SoberLanguage = "GUI.SoberLanguage";
    public const string Cheers = "GUI.Cheers";
    public const string Hangover = "GUI.Hangover";
    public const string LastBuyWineTime = "GUI.LastBuyWineTime";
    public const string CustomCulture = "GUI.CustomCulture";

    public const string BackgroundImagePath = "GUI.Background.ImagePath";
    public const string BackgroundImageStretchMode = "GUI.Background.StretchMode";
    public const string BackgroundOpacity = "GUI.Background.Opacity";
    public const string BackgroundBlurEffectRadius = "GUI.Background.BlurEffectRadius";

    public const string AddressHistory = "Connect.AddressHistory";
    public const string AutoDetect = "Connect.AutoDetect";
    public const string AlwaysAutoDetect = "Connect.AlwaysAutoDetect";
    public const string MumuBridgeConnection = "Connect.MumuBridgeConnection";
    public const string ConnectAddress = "Connect.Address";
    public const string AdbPath = "Connect.AdbPath";
    public const string ConnectConfig = "Connect.ConnectConfig";
    public const string MuMu12ExtrasEnabled = "Connect.MuMu12Extras.Enabled";
    public const string MuMu12EmulatorPath = "Connect.MuMu12EmulatorPath";
    public const string MuMu12Index = "Connect.MuMu12Index";
    public const string MuMu12Display = "Connect.MuMu12Display";
    public const string LdPlayerExtrasEnabled = "Connect.LdPlayerExtras.Enabled";
    public const string LdPlayerEmulatorPath = "Connect.LdPlayerEmulatorPath";
    public const string LdPlayerManualSetIndex = "Connect.LdPlayerManualSetIndex";
    public const string LdPlayerIndex = "Connect.LdPlayerIndex";
    public const string RetryOnAdbDisconnected = "Connect.RetryOnDisconnected";
    public const string AllowAdbRestart = "Connect.AllowADBRestart";
    public const string AllowAdbHardRestart = "Connect.AllowADBHardRestart";
    public const string AdbLiteEnabled = "Connect.AdbLiteEnabled";
    public const string KillAdbOnExit = "Connect.KillAdbOnExit";
    public const string TouchMode = "Connect.TouchMode";
    public const string AdbReplaced = "Connect.AdbReplaced";

    // AttachWindow (Win32窗口绑定) 配置
    public const string UseAttachWindow = "Connect.UseAttachWindow";
    public const string AttachWindowScreencapMethod = "Connect.AttachWindow.ScreencapMethod";
    public const string AttachWindowMouseMethod = "Connect.AttachWindow.MouseMethod";
    public const string AttachWindowKeyboardMethod = "Connect.AttachWindow.KeyboardMethod";

    public const string StartGame = "Start.StartGame";
    public const string ClientType = "Start.ClientType";
    public const string AccountName = "Start.AccountName"; // 已迁移
    public const string RunDirectly = "Start.RunDirectly";
    public const string MinimizeDirectly = "Start.MinimizeDirectly";
    public const string StartEmulator = "Start.OpenEmulatorAfterLaunch";
    public const string EmulatorPath = "Start.EmulatorPath";
    public const string EmulatorAddCommand = "Start.EmulatorAddCommand";
    public const string EmulatorWaitSeconds = "Start.EmulatorWaitSeconds";

    // 仅在理智作战的任务中会生效，前缀需要修改
    public const string AutoRestartOnDrop = "Start.AutoRestartOnDrop";
    public const string StartsWithScript = "Start.StartsWithScript";
    public const string EndsWithScript = "Start.EndsWithScript";
    public const string CopilotWithScript = "Start.CopilotWithScript";
    public const string ManualStopWithScript = "Start.ManualStopWithScript";
    public const string BlockSleep = "Start.BlockSleep";
    public const string BlockSleepWithScreenOn = "Start.BlockSleepWithScreenOn";

    public const string ChooseLevel3 = "Recruit.ChooseLevel3";
    public const string ToolBoxChooseLevel3Time = "Toolbox.Recruit.ChooseLevel3.Time";
    public const string ChooseLevel4 = "Recruit.ChooseLevel4";
    public const string ToolBoxChooseLevel4Time = "Toolbox.Recruit.ChooseLevel4.Time";
    public const string ChooseLevel5 = "Recruit.ChooseLevel5";
    public const string ToolBoxChooseLevel5Time = "Toolbox.Recruit.ChooseLevel5.Time";
    public const string ChooseLevel6 = "Recruit.ChooseLevel6";
    public const string AutoSetTime = "Recruit.AutoSetTime";
    public const string RecruitmentShowPotential = "Recruit.ShowPotential";

    public const string DepotResult = "Depot.DepotResult";

    public const string InfrastMode = "Infrast.InfrastMode"; // 已迁移
    public const string DormThreshold = "Infrast.DormThreshold"; // 已迁移
    public const string UsesOfDrones = "Infrast.UsesOfDrones"; // 已迁移
    public const string InfrastReceptionMessageBoardReceive = "Infrast.ReceptionMessageBoardReceive"; // 已迁移
    public const string InfrastReceptionClueExchange = "Infrast.ReceptionClueExchange"; // 已迁移
    public const string InfrastReceptionSendClue = "Infrast.ReceptionSendClue"; // 已迁移
    public const string ContinueTraining = "Infrast.ContinueTraining";  // 已迁移
    public const string DefaultInfrast = "Infrast.DefaultInfrast";
    public const string IsCustomInfrastFileReadOnly = "Infrast.IsCustomInfrastFileReadOnly"; // 已废弃
    public const string DormFilterNotStationedEnabled = "Infrast.DormFilterNotStationedEnabled"; // 已迁移
    public const string DormTrustEnabled = "Infrast.DormTrustEnabled"; // 已迁移
    public const string OriginiumShardAutoReplenishment = "Infrast.OriginiumShardAutoReplenishment"; // 已迁移
    public const string CustomInfrastEnabled = "Infrast.CustomInfrastEnabled";
    public const string CustomInfrastFile = "Infrast.CustomInfrastFile"; // 已迁移
    public const string CustomInfrastPlanIndex = "Infrast.CustomInfrastPlanIndex"; // 已废弃
    public const string CustomInfrastPlanSelect = "Infrast.CustomInfrastPlanSelect"; // 已迁移
    public const string CustomInfrastPlanShowInFightSettings = "Infrast.CustomInfrastPlanShowInFightSettings"; // 已废弃

    public const string UseRemainingSanityStage = "Fight.UseRemainingSanityStage"; // 已废弃
    public const string UseExpiringMedicine = "Fight.UseExpiringMedicine"; // 已迁移
    public const string RemainingSanityStage = "Fight.RemainingSanityStage"; // 已迁移

    public const string RoguelikeTheme = "Roguelike.RoguelikeTheme"; // 已迁移
    public const string RoguelikeDifficulty = "Roguelike.Difficulty"; // 已迁移
    public const string RoguelikeMode = "Roguelike.Mode"; // 已迁移
    public const string RoguelikeSquad = "Roguelike.Squad"; // 已迁移
    public const string RoguelikeCollectibleModeSquad = "Roguelike.CollectibleModeSquad"; // 已迁移
    public const string RoguelikeRoles = "Roguelike.Roles"; // 已迁移
    public const string RoguelikeCoreChar = "Roguelike.CoreChar"; // 已迁移
    public const string RoguelikeStartWithEliteTwo = "Roguelike.RoguelikeStartWithEliteTwo"; // 已迁移
    public const string RoguelikeOnlyStartWithEliteTwo = "Roguelike.RoguelikeOnlyStartWithEliteTwo"; // 已迁移
    public const string RoguelikeStartWithSelectList = "Roguelike.RoguelikeStartWithSelectList"; // 已迁移
    public const string Roguelike3FirstFloorFoldartal = "Roguelike.Roguelike3FirstFloorFoldartal"; // 已迁移
    public const string Roguelike3FirstFloorFoldartals = "Roguelike.Roguelike3StartFloorFoldartal"; // 已迁移
    public const string Roguelike3NewSquad2StartingFoldartal = "Roguelike.Roguelike3NewSquad2StartingFoldartal"; // 已迁移
    public const string Roguelike3NewSquad2StartingFoldartals = "Roguelike.Roguelike3NewSquad2StartingFoldartals"; // 已迁移
    public const string RoguelikeExpectedCollapsalParadigms = "Roguelike.RoguelikeExpectedCollapsalParadigms"; // 已迁移
    public const string RoguelikeUseSupportUnit = "Roguelike.RoguelikeUseSupportUnit"; // 已迁移
    public const string RoguelikeEnableNonfriendSupport = "Roguelike.RoguelikeEnableNonfriendSupport"; // 已迁移
    public const string RoguelikeDelayAbortUntilCombatComplete = "Roguelike.RoguelikeDelayAbortUntilCombatComplete";
    public const string RoguelikeStartsCount = "Roguelike.StartsCount"; // 已迁移
    public const string RoguelikeInvestmentEnabled = "Roguelike.InvestmentEnabled"; // 已迁移
    public const string RoguelikeInvestmentEnterSecondFloor = "Roguelike.InvestmentEnterSecondFloor"; // 已迁移
    public const string RoguelikeCollectibleModeShopping = "Roguelike.CollectibleModeShopping"; // 已迁移
    public const string RoguelikeRefreshTraderWithDice = "Roguelike.RefreshTraderWithDice"; // 已迁移
    public const string RoguelikeInvestsCount = "Roguelike.InvestsCount"; // 已迁移
    public const string RoguelikeStopWhenInvestmentFull = "Roguelike.StopWhenInvestmentFull"; // 已迁移
    public const string RoguelikeDeploymentWithPause = "Roguelike.DeploymentWithPause";
    public const string RoguelikeStopAtFinalBoss = "Roguelike.ExitAtFinalBoss"; // 已迁移
    public const string RoguelikeStopAtMaxLevel = "Roguelike.StopAtMaxLevel"; // 已迁移
    public const string RoguelikeStartWithSeed = "Roguelike.StartWithSeed";
    public const string RoguelikeMonthlySquadAutoIterate = "Roguelike.MonthlySquadAutoIterate"; // 已迁移
    public const string RoguelikeMonthlySquadCheckComms = "Roguelike.MonthlySquadCheckComms"; // 已迁移
    public const string RoguelikeDeepExplorationAutoIterate = "Roguelike.DeepExplorationAutoIterate"; // 已迁移
    public const string RoguelikeFindPlaytimeTarget = "Roguelike.FindPlaytimeTarget"; // 已迁移
    public const string ReclamationTheme = "Reclamation.Theme"; // 已迁移
    public const string ReclamationMode = "Reclamation.Mode"; // 已迁移
    public const string ReclamationToolToCraft = "Reclamation.ToolToCraft"; // 已迁移
    public const string ReclamationIncrementMode = "Reclamation.ReclamationIncrementMode"; // 已迁移
    public const string ReclamationMaxCraftCountPerRound = "Reclamation.ReclamationMaxCraftCountPerRound"; // 已迁移
    public const string ReclamationClearStore = "Reclamation.ReclamationClearStore"; // 已迁移

    public const string RecruitMaxTimes = "AutoRecruit.MaxTimes"; // 已迁移
    public const string AutoRecruitFirstList = "AutoRecruit.AutoRecruitFirstList"; // 已迁移
    public const string RefreshLevel3 = "AutoRecruit.RefreshLevel3"; // 已迁移
    public const string ForceRefresh = "AutoRecruit.ForceRefresh"; // 已迁移
    public const string SelectExtraTags = "AutoRecruit.SelectExtraTags"; // 已迁移
    public const string NotChooseLevel1 = "AutoRecruit.NotChooseLevel1"; // 已迁移
    public const string RecruitChooseLevel3 = "AutoRecruit.ChooseLevel3"; // 已迁移
    public const string ChooseLevel3Time = "AutoRecruit.ChooseLevel3.Time"; // 已迁移
    public const string RecruitChooseLevel4 = "AutoRecruit.ChooseLevel4"; // 已迁移
    public const string ChooseLevel4Time = "AutoRecruit.ChooseLevel4.Time"; // 已迁移
    public const string RecruitChooseLevel5 = "AutoRecruit.ChooseLevel5"; // 已迁移
    public const string ChooseLevel5Time = "AutoRecruit.ChooseLevel5.Time"; // 已迁移

    public const string LastCreditFightTaskTime = "Visit.LastCreditFightTaskTime"; // 已迁移
    public const string CreditFightTaskEnabled = "Visit.CreditFightTaskEnabled"; // 已迁移
    public const string CreditFightOnceADay = "Visit.CreditFightOnceADay"; // 已迁移
    public const string CreditFightSelectFormation = "Visit.CreditFightSelectFormation"; // 已迁移

    public const string LastCreditVisitFriendsTime = "Mall.LastCreditVisitFriendsTime"; // 已迁移
    public const string CreditVisitOnceADay = "Mall.CreditVisitOnceADay"; // 已迁移
    public const string CreditVisitFriendsEnabled = "Mall.CreditVisitFriendsEnabled"; // 已迁移
    public const string CreditShopping = "Mall.CreditShopping"; // 已迁移
    public const string CreditFirstListNew = "Mall.CreditFirstListNew"; // 已迁移
    public const string CreditBlackListNew = "Mall.CreditBlackListNew"; // 已迁移
    public const string CreditForceShoppingIfCreditFull = "Mall.CreditForceShoppingIfCreditFull"; // 已迁移
    public const string CreditOnlyBuyDiscount = "Mall.CreditOnlyBuyDiscount"; // 已迁移
    public const string CreditReserveMaxCredit = "Mall.CreidtReserveMaxCredit"; // 已迁移

    public const string ReceiveAward = "Mission.ReceiveAward"; // 已迁移
    public const string ReceiveMail = "Mission.ReceiveMail"; // 已迁移
    public const string ReceiveFreeGacha = "Mission.ReceiveFreeRecruit"; // 已迁移
    public const string ReceiveOrundum = "Mission.ReceiveOrundum"; // 已迁移
    public const string ReceiveMining = "Mission.ReceiveMining"; // 已迁移
    public const string ReceiveSpecialAccess = "Mission.ReceiveSpecialAccess"; // 已迁移

    public const string CopilotAddUserAdditional = "Copilot.AddUserAdditional";
    public const string CopilotUserAdditional = "Copilot.UserAdditional";
    public const string CopilotSelectFormation = "Copilot.SelectFormation";
    public const string CopilotSupportUnitUsage = "Copilot.SupportUnitUsage";
    public const string CopilotLoopTimes = "Copilot.LoopTimes";
    public const string CopilotTaskList = "Copilot.CopilotTaskList";
    public const string UpdateProxy = "VersionUpdate.Proxy";
    public const string ProxyType = "VersionUpdate.ProxyType";
    public const string VersionType = "VersionUpdate.VersionType";

    // MirrorChyanCdk 迁到新配置的话就直接叫 UpdateSource，现在本体更新也加上了 MirrorChyan
    public const string UpdateSource = "VersionUpdate.ResourceUpdateSource";
    public const string ForceGithubGlobalSource = "VersionUpdate.UpdateSource.ForceGithubGlobalSource";
    public const string MirrorChyanCdk = "VersionUpdate.ResourceUpdateSource.MirrorChyanCdk";
    public const string MirrorChyanCdkExpiredTime = "VersionUpdate.UpdateSource.MirrorChyanCdkExpired";
    public const string StartupUpdateCheck = "VersionUpdate.StartupUpdateCheck";
    public const string UpdateAutoCheck = "VersionUpdate.ScheduledUpdateCheck";
    public const string ResourceApi = "VersionUpdate.ResourceApi";
    public const string AllowNightlyUpdates = "VersionUpdate.AllowNightlyUpdates";
    public const string HasAcknowledgedNightlyWarning = "VersionUpdate.HasAcknowledgedNightlyWarning";

    // 这个已经废弃了，还要留着吗？
    [UsedImplicitly]
    public const string UseAria2 = "VersionUpdate.UseAria2";

    public const string AutoDownloadUpdatePackage = "VersionUpdate.AutoDownloadUpdatePackage";
    public const string AutoInstallUpdatePackage = "VersionUpdate.AutoInstallUpdatePackage";

    public const string PenguinId = "Penguin.Id";
    public const string IsDrGrandet = "Penguin.IsDrGrandet"; // 已迁移
    public const string EnablePenguin = "Penguin.EnablePenguin";

    public const string EnableYituliu = "Yituliu.EnableYituliu";

    public const string TaskTimeoutMinutes = "TimeOut.Timer.TaskTimeoutMinutes";
    public const string ReminderIntervalMinutes = "TimeOut.Timer.ReminderIntervalMinutes";

    public const string BluestacksConfigPath = "Bluestacks.Config.Path";
    public const string BluestacksConfigKeyword = "Bluestacks.Config.Keyword";
    public const string BluestacksConfigError = "Bluestacks.Config.Error";

    public const string PostActions = "MainFunction.PostActions";
    public const string MainFunctionInverseMode = "MainFunction.InverseMode";
    public const string Stage1 = "MainFunction.Stage1"; // 已迁移
    public const string Stage2 = "MainFunction.Stage2"; // 已迁移
    public const string Stage3 = "MainFunction.Stage3"; // 已迁移
    public const string Stage4 = "MainFunction.Stage4"; // 已迁移
    public const string UseCustomAnnihilation = "MainFunction.Annihilation.UseCustom"; // 已迁移
    public const string AnnihilationStage = "MainFunction.Annihilation.Stage"; // 已迁移
    public const string UseMedicine = "MainFunction.UseMedicine"; // 已迁移
    public const string UseMedicineQuantity = "MainFunction.UseMedicine.Quantity"; // 已迁移
    public const string UseStone = "MainFunction.UseStone"; // 已迁移
    public const string UseStoneQuantity = "MainFunction.UseStone.Quantity"; // 已迁移
    public const string TimesLimited = "MainFunction.TimesLimited"; // 已迁移
    public const string TimesLimitedQuantity = "MainFunction.TimesLimited.Quantity"; // 已迁移
    public const string SeriesQuantity = "MainFunction.Series.Quantity"; // 已迁移
    public const string DropsEnable = "MainFunction.Drops.Enable"; // 已迁移
    public const string DropsItemId = "MainFunction.Drops.ItemId"; // 已迁移
    public const string DropsItemName = "MainFunction.Drops.ItemName"; // 已废弃
    public const string DropsQuantity = "MainFunction.Drops.Quantity"; // 已迁移

    public const string RemoteControlGetTaskEndpointUri = "RemoteControl.RemoteControlGetTaskEndpointUri";
    public const string RemoteControlReportStatusUri = "RemoteControl.RemoteControlReportStatusUri";
    public const string RemoteControlUserIdentity = "RemoteControl.RemoteControlUserIdentity";
    public const string RemoteControlDeviceIdentity = "RemoteControl.RemoteControlDeviceIdentity";
    public const string RemoteControlPollIntervalMs = "RemoteControl.RemoteControlPollIntervalMs";

    public const string ExternalNotificationEnabled = "ExternalNotification.Enabled";
    public const string ExternalNotificationSendWhenComplete = "ExternalNotification.SendWhenComplete";
    public const string ExternalNotificationEnableDetails = "ExternalNotification.EnableDetails";
    public const string ExternalNotificationSendWhenError = "ExternalNotification.SendWhenError";
    public const string ExternalNotificationSendWhenTimeout = "ExternalNotification.SendWhenTimeout";
    public const string ExternalNotificationSmtpServer = "ExternalNotification.Smtp.Server";
    public const string ExternalNotificationSmtpPort = "ExternalNotification.Smtp.Port";
    public const string ExternalNotificationSmtpUser = "ExternalNotification.Smtp.User";
    public const string ExternalNotificationSmtpPassword = "ExternalNotification.Smtp.Password";
    public const string ExternalNotificationSmtpUseSsl = "ExternalNotification.Smtp.UseSsl";
    public const string ExternalNotificationSmtpRequiresAuthentication = "ExternalNotification.Smtp.RequiresAuthentication";
    public const string ExternalNotificationSmtpFrom = "ExternalNotification.Smtp.From";
    public const string ExternalNotificationSmtpTo = "ExternalNotification.Smtp.To";
    public const string ExternalNotificationServerChanSendKey = "ExternalNotification.ServerChan.SendKey";
    public const string ExternalNotificationDiscordBotToken = "ExternalNotification.Discord.BotToken";
    public const string ExternalNotificationDiscordUserId = "ExternalNotification.Discord.UserId";
    public const string ExternalNotificationDiscordWebhookUrl = "ExternalNotification.Discord.WebhookUrl";
    public const string ExternalNotificationDingTalkAccessToken = "ExternalNotification.DingTalk.AccessToken";
    public const string ExternalNotificationDingTalkSecret = "ExternalNotification.DingTalk.Secret";
    public const string ExternalNotificationTelegramBotToken = "ExternalNotification.Telegram.BotToken";
    public const string ExternalNotificationTelegramChatId = "ExternalNotification.Telegram.ChatId";
    public const string ExternalNotificationTelegramTopicId = "ExternalNotification.TelegramTopicId";
    public const string ExternalNotificationBarkSendKey = "ExternalNotification.Bark.SendKey";
    public const string ExternalNotificationBarkServer = "ExternalNotification.Bark.Server";
    public const string ExternalNotificationQmsgServer = "ExternalNotification.Qmsg.Server";
    public const string ExternalNotificationQmsgKey = "ExternalNotification.Qmsg.Key";
    public const string ExternalNotificationQmsgUser = "ExternalNotification.Qmsg.User";
    public const string ExternalNotificationQmsgBot = "ExternalNotification.Qmsg.Bot";
    public const string ExternalNotificationGotifyServer = "ExternalNotification.Gotify.Server";
    public const string ExternalNotificationGotifyToken = "ExternalNotification.Gotify.Token";
    public const string ExternalNotificationCustomWebhookUrl = "ExternalNotification.CustomWebhook.Url";
    public const string ExternalNotificationCustomWebhookBody = "ExternalNotification.CustomWebhook.Body";

    public const string PerformanceUseGpu = "Performance.UseGpu";
    public const string PerformancePreferredGpuDescription = "Performance.PreferredGpuDescription";
    public const string PerformancePreferredGpuInstancePath = "Performance.PreferredGpuInstancePath";
    public const string PerformanceAllowDeprecatedGpu = "Performance.AllowDeprecatedGpu";

    // The following should not be modified manually
    public const string VersionName = "VersionUpdate.name";

    public const string VersionUpdateBody = "VersionUpdate.body";
    public const string VersionUpdateIsFirstBoot = "VersionUpdate.isfirstboot";
    public const string VersionUpdatePackage = "VersionUpdate.package";
    public const string VersionUpdateDoNotShowUpdate = "VersionUpdate.doNotShowUpdate";

    public const string OperBoxData = "OperBox.Data";

    public const string GachaShowDisclaimerNoMore = "Gacha.ShowDisclaimerNoMore";

    public const string PeepTargetFps = "Peep.TargetFps";

    public const string GuideStepIndex = "Guide.StepIndex";

    public const string ForceScheduledStart = "Timer.ForceScheduledStart";
    public const string ShowWindowBeforeForceScheduledStart = "Timer.ShowWindowBeforeForceScheduledStart";
    public const string CustomConfig = "Timer.CustomConfig";

    public const string DebugTaskName = "Debug.TaskName";

    public const string MiniGameTaskName = "MiniGame.TaskName";
    public const string MiniGameSecretFrontEnding = "MiniGame.SecretFrontEnding";
    public const string MiniGameSecretFrontEvent = "MiniGame.SecretFrontEvent";

    public const string RecastConditions = "MiniGame.RecastConditions";

    public const string AchievementPopupDisabled = "Achievement.PopupDisabled";
    public const string AchievementPopupAutoClose = "Achievement.PopupAutoClose";

    public const string OverlayTarget = "Overlay.Target";

    public const string AnnouncementInfo = "Announcement.AnnouncementInfo"; // 已迁移
    public const string DoNotRemindThisAnnouncementAgain = "Announcement.DoNotRemindThisAnnouncementAgain"; // 已迁移
    public const string DoNotShowAnnouncement = "Announcement.DoNotShowAnnouncement"; // 已迁移
}
