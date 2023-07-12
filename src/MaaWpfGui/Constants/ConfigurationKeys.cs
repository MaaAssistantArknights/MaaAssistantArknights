// <copyright file="ConfigurationKeys.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

namespace MaaWpfGui.Constants
{
    /// <summary>
    /// MaaWpfGui configuration keys
    /// </summary>
    public static class ConfigurationKeys
    {
        public const string CurrentConfiguration = "Current";
        public const string DefaultConfiguration = "Default";
        public const string GlobalConfiguration = "Global";
        public const string ConfigurationMap = "Configurations";
        public const string ConfigurationData = "Data";
        public const string ConfigurationCron = "Cron";

        public const string Localization = "GUI.Localization";
        public const string MinimizeToTray = "GUI.MinimizeToTray";
        public const string HideCloseButton = "GUI.HideCloseButton";
        public const string UseNotify = "GUI.UseNotify";
        public const string WindowPlacement = "GUI.Placement";
        public const string LoadWindowPlacement = "GUI.Placement.Load";
        public const string SaveWindowPlacement = "GUI.Placement.SaveOnClosing";
        public const string UseAlternateStage = "GUI.UseAlternateStage";
        public const string HideUnavailableStage = "GUI.HideUnavailableStage";
        public const string CustomStageCode = "GUI.CustomStageCode";
        public const string InverseClearMode = "GUI.InverseClearMode";
        public const string WindowTitlePrefix = "GUI.WindowTitlePrefix";
        public const string DarkMode = "GUI.DarkMode";

        public const string MonitorNumber = "GUI.Monitor.Number";
        public const string MonitorWidth = "GUI.Monitor.Width";
        public const string MonitorHeight = "GUI.Monitor.Height";
        public const string PositionLeft = "GUI.Position.Left";
        public const string PositionTop = "GUI.Position.Top";
        public const string WindowWidth = "GUI.Size.Width";
        public const string WindowHeight = "GUI.Size.Height";
        public const string LoadPositionAndSize = "GUI.PositionAndSize.Load";
        public const string SavePositionAndSize = "GUI.PositionAndSize.SaveOnClosing";

        public const string AddressHistory = "Connect.AddressHistory";
        public const string AutoDetect = "Connect.AutoDetect";
        public const string AlwaysAutoDetect = "Connect.AlwaysAutoDetect";
        public const string ConnectAddress = "Connect.Address";
        public const string AdbPath = "Connect.AdbPath";
        public const string ConnectConfig = "Connect.ConnectConfig";
        public const string RetryOnAdbDisconnected = "Connect.RetryOnDisconnected";
        public const string AllowADBRestart = "Connect.AllowADBRestart";
        public const string AdbLiteEnabled = "Connect.AdbLiteEnabled";
        public const string KillAdbOnExit = "Connect.KillAdbOnExit";
        public const string TouchMode = "Connect.TouchMode";
        public const string AdbReplaced = "Connect.AdbReplaced";

        public const string ClientType = "Start.ClientType";
        public const string RunDirectly = "Start.RunDirectly";
        public const string MinimizeDirectly = "Start.MinimizeDirectly";
        public const string StartEmulator = "Start.StartEmulator";
        public const string MinimizingStartup = "Start.MinimizingStartup";
        public const string EmulatorPath = "Start.EmulatorPath";
        public const string EmulatorAddCommand = "Start.EmulatorAddCommand";
        public const string EmulatorWaitSeconds = "Start.EmulatorWaitSeconds";
        public const string AutoRestartOnDrop = "Start.AutoRestartOnDrop";
        public const string StartsWithScript = "Start.StartsWithScript";
        public const string EndsWithScript = "Start.EndsWithScript";

        public const string ChooseLevel3 = "Recruit.ChooseLevel3";
        public const string ChooseLevel4 = "Recruit.ChooseLevel4";
        public const string ChooseLevel5 = "Recruit.ChooseLevel5";
        public const string ChooseLevel6 = "Recruit.ChooseLevel6";
        public const string AutoSetTime = "Recruit.AutoSetTime";
        public const string Level3UseShortTime = "Recruit.IsLevel3UseShortTime";
        public const string Level3UseShortTime2 = "Recruit.IsLevel3UseShortTime2";
        public const string RecruitmentShowPotential = "Recruit.ShowPotential";

        public const string DormThreshold = "Infrast.DormThreshold";
        public const string UsesOfDrones = "Infrast.UsesOfDrones";
        public const string DefaultInfrast = "Infrast.DefaultInfrast";
        public const string IsCustomInfrastFileReadOnly = "Infrast.IsCustomInfrastFileReadOnly";
        public const string DormFilterNotStationedEnabled = "Infrast.DormFilterNotStationedEnabled";
        public const string DormTrustEnabled = "Infrast.DormTrustEnabled";
        public const string OriginiumShardAutoReplenishment = "Infrast.OriginiumShardAutoReplenishment";
        public const string CustomInfrastEnabled = "Infrast.CustomInfrastEnabled";
        public const string CustomInfrastFile = "Infrast.CustomInfrastFile";
        public const string CustomInfrastPlanIndex = "Infrast.CustomInfrastPlanIndex";
        public const string CustomInfrastPlanShowInFightSettings = "Infrast.CustomInfrastPlanShowInFightSettings";

        public const string UseRemainingSanityStage = "Fight.UseRemainingSanityStage";
        public const string UseExpiringMedicine = "Fight.UseExpiringMedicine";
        public const string RemainingSanityStage = "Fight.RemainingSanityStage";

        public const string RoguelikeTheme = "Roguelike.RoguelikeTheme";
        public const string RoguelikeMode = "Roguelike.Mode";
        public const string RoguelikeSquad = "Roguelike.Squad";
        public const string RoguelikeRoles = "Roguelike.Roles";
        public const string RoguelikeCoreChar = "Roguelike.CoreChar";
        public const string RoguelikeUseSupportUnit = "Roguelike.RoguelikeUseSupportUnit";
        public const string RoguelikeEnableNonfriendSupport = "Roguelike.RoguelikeEnableNonfriendSupport";
        public const string RoguelikeStartsCount = "Roguelike.StartsCount";
        public const string RoguelikeInvestmentEnabled = "Roguelike.InvestmentEnabled";
        public const string RoguelikeRefreshTraderWithDice = "Roguelike.RefreshTraderWithDice";
        public const string RoguelikeInvestsCount = "Roguelike.InvestsCount";
        public const string RoguelikeStopWhenInvestmentFull = "Roguelike.StopWhenInvestmentFull";
        public const string RoguelikeDeploymentWithPause = "Roguelike.DeploymentWithPause";

        public const string LastCreditFightTaskTime = "Visit.LastCreditFightTaskTime";
        public const string CreditFightTaskEnabled = "Visit.CreditFightTaskEnabled";

        public const string RecruitMaxTimes = "AutoRecruit.MaxTimes";
        public const string RefreshLevel3 = "AutoRecruit.RefreshLevel3";
        public const string IsLevel3UseShortTime = "AutoRecruit.IsLevel3UseShortTime";
        public const string IsLevel3UseShortTime2 = "AutoRecruit.IsLevel3UseShortTime2";
        public const string NotChooseLevel1 = "AutoRecruit.NotChooseLevel1";
        public const string RecruitChooseLevel3 = "AutoRecruit.ChooseLevel3";
        public const string RecruitChooseLevel4 = "AutoRecruit.ChooseLevel4";
        public const string RecruitChooseLevel5 = "AutoRecruit.ChooseLevel5";

        public const string CreditShopping = "Mall.CreditShopping";
        public const string CreditFirstListNew = "Mall.CreditFirstListNew";
        public const string CreditBlackListNew = "Mall.CreditBlackListNew";
        public const string CreditForceShoppingIfCreditFull = "Mall.CreditForceShoppingIfCreditFull";

        public const string CopilotLoopTimes = "Copilot.LoopTimes";

        public const string UpdateProxy = "VersionUpdate.Proxy";
        public const string VersionType = "VersionUpdate.VersionType";
        public const string UpdateCheck = "VersionUpdate.UpdateCheck";
        public const string UpdatAutoCheck = "VersionUpdate.ScheduledUpdateCheck";
        public const string UseAria2 = "VersionUpdate.UseAria2";
        public const string AutoDownloadUpdatePackage = "VersionUpdate.AutoDownloadUpdatePackage";
        public const string AutoInstallUpdatePackage = "VersionUpdate.AutoInstallUpdatePackage";

        public const string PenguinId = "Penguin.Id";
        public const string IsDrGrandet = "Penguin.IsDrGrandet";

        public const string BluestacksConfigPath = "Bluestacks.Config.Path";
        public const string BluestacksConfigKeyword = "Bluestacks.Config.Keyword";
        public const string BluestacksConfigError = "Bluestacks.Config.Error";

        public const string ActionAfterCompleted = "MainFunction.ActionAfterCompleted";
        public const string MainFunctionInverseMode = "MainFunction.InverseMode";
        public const string Stage1 = "MainFunction.Stage1";
        public const string Stage2 = "MainFunction.Stage2";
        public const string Stage3 = "MainFunction.Stage3";
        public const string UseMedicine = "MainFunction.UseMedicine";
        public const string UseMedicineQuantity = "MainFunction.UseMedicine.Quantity";
        public const string UseStoneQuantity = "MainFunction.UseStone.Quantity";
        public const string TimesLimited = "MainFunction.TimesLimited";
        public const string TimesLimitedQuantity = "MainFunction.TimesLimited.Quantity";
        public const string DropsEnable = "MainFunction.Drops.Enable";
        public const string DropsItemId = "MainFunction.Drops.ItemId";
        public const string DropsItemName = "MainFunction.Drops.ItemName";
        public const string DropsQuantity = "MainFunction.Drops.Quantity";

        // The following should not be modified manually
        public const string VersionName = "VersionUpdate.name";

        public const string VersionUpdateBody = "VersionUpdate.body";
        public const string VersionUpdateIsFirstBoot = "VersionUpdate.isfirstboot";
        public const string VersionUpdatePackage = "VersionUpdate.package";

        public const string OperBoxData = "OperBox.Data";

        public const string GachaShowDisclaimerNoMore = "Gacha.ShowDisclaimerNoMore";

        public const string GuideStepIndex = "Guide.StepIndex";

        public const string ForceScheduledStart = "Timer.ForceScheduledStart";
        public const string CustomConfig = "Timer.CustomConfig";
    }
}
