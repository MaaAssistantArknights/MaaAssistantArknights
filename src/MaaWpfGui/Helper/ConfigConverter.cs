// <copyright file="ConfigConverter.cs" company="MaaAssistantArknights">
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
#nullable enable
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Windows.Media;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Models;
using MaaWpfGui.Models.EmulatorConnectionExtra;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.Configuration.Global.Gui;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;
using static MaaWpfGui.Models.AsstTasks.AsstCopilotTask;
using static MaaWpfGui.Models.EmulatorConnectionExtra.Win32Extra;
using static MaaWpfGui.Models.PostActionSetting;
using static MaaWpfGui.ViewModels.UI.CopilotViewModel;
using static MaaWpfGui.ViewModels.UI.OverlayViewModel;
using static MaaWpfGui.ViewModels.UI.ToolboxViewModel;
using static MaaWpfGui.ViewModels.UserControl.Settings.VersionUpdateSettingsUserControlModel;

namespace MaaWpfGui.Helper;

public class ConfigConverter
{
    private static readonly ILogger _logger = Log.ForContext<ConfigConverter>();
    private static readonly string ConfigurationNewFile = ConfigFactory.ConfigFile;
    private static readonly string ConfigurationOldBakFile = ConfigurationHelper.ConfigFile + ".old";
    private static readonly string ConfigurationOldFile = ConfigurationHelper.ConfigFile;
    private static bool HasBackupOldConfig = false;

    public static bool ConvertConfig()
    {
        if (Directory.Exists("config") is false)
        {
            Directory.CreateDirectory("config");
        }

        // Load configuration file
        var parsedOld = ParseJsonFile(ConfigurationOldFile);
        if (parsedOld is null)
        {
            return false;
        }

        var root = ParseJsonFile(ConfigurationNewFile);

        bool ret = true;
        JObject? configurations = root?["Configurations"] as JObject;

        bool needConvert = configurations?.Count == 0 || configurations?["Default"]?["TaskQueueOrder"] is not null;
        if (needConvert)
        {
            ret &= ConvertTaskQueue();
        }

        if (ConfigurationHelper.ContainsKey(ConfigurationKeys.PerformanceUseGpu))
        {
            ret = ret && ConvertWpfSettings();
        }

        return ret;
    }

    // 迁移任务队列，v5.15编写
    private static bool ConvertTaskQueue()
    {
        BackupOldConfig();
        string[] configKeys = [ConfigurationKeys.AnnouncementInfo, ConfigurationKeys.DoNotRemindThisAnnouncementAgain, ConfigurationKeys.DoNotShowAnnouncement,
            ConfigurationKeys.VersionName, ConfigurationKeys.VersionUpdateBody, ConfigurationKeys.VersionUpdateIsFirstBoot, ConfigurationKeys.VersionUpdatePackage,
            ConfigurationKeys.VersionUpdateDoNotShowUpdate, ConfigurationKeys.CustomInfrastEnabled, ConfigurationKeys.CustomInfrastPlanShowInFightSettings,
        ];

        foreach (var name in ConfigFactory.ConfigKeys.ToList())
        {
            ConfigFactory.DeleteConfiguration(name);
        }
        var currentConfigName = ConfigurationHelper.GetCurrentConfiguration();
        foreach (var configName in ConfigurationHelper.GetConfigurationList())
        {
            ConfigurationHelper.SwitchConfiguration(configName);
            if (ConfigFactory.Root.Configurations.ContainsKey(configName))
            {
            }
            else if (ConfigFactory.AddConfiguration(configName) is false)
            {
                _logger.Error("配置迁移失败，无法添加配置: {ConfigName}", configName);
                throw new Exception($"配置迁移失败，无法添加配置{configName}");
            }

            if (!ConfigFactory.SwitchConfig(configName))
            {
                _logger.Error("配置迁移失败，无法切换到配置: {ConfigName}", configName);
                throw new Exception($"配置迁移失败，无法切换到配置{configName}");
            }

            // 删除旧的配置
            foreach (var key in configKeys)
            {
                ConfigurationHelper.DeleteValue(key);
            }

            var local = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);

            // TaskQueue部分
            {
                var startUpTask = new StartUpTask(); // √
                var fightTask = new FightTask();
                var fightTask2 = new FightTask(); // 剩余理智 √
                var awardTask = new AwardTask(); // √
                var mallTask = new MallTask(); // √
                var infrastTask = new InfrastTask();
                var recruitTask = new RecruitTask(); // √
                var roguelikeTask = new RoguelikeTask(); // √
                var reclamationTask = new ReclamationTask(); // √

                startUpTask.AccountName = ConfigurationHelper.GetValue(ConfigurationKeys.AccountName, string.Empty);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AccountName);

                fightTask.UseMedicine = ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicine, false);
                fightTask.MedicineCount = ConfigurationHelper.GetValue(ConfigurationKeys.UseMedicineQuantity, 999);
                fightTask.UseStone = ConfigurationHelper.GetValue(ConfigurationKeys.UseStone, false);
                fightTask.StoneCount = ConfigurationHelper.GetValue(ConfigurationKeys.UseStoneQuantity, 0);
                fightTask.EnableTimesLimit = ConfigurationHelper.GetValue(ConfigurationKeys.TimesLimited, false);
                fightTask.TimesLimit = ConfigurationHelper.GetValue(ConfigurationKeys.TimesLimitedQuantity, 5);
                fightTask.Series = ConfigurationHelper.GetValue(ConfigurationKeys.SeriesQuantity, 0);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseMedicine);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseMedicineQuantity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseStone);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseStoneQuantity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.TimesLimited);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.TimesLimitedQuantity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.SeriesQuantity);

                fightTask.EnableTargetDrop = ConfigurationHelper.GetValue(ConfigurationKeys.DropsEnable, false);
                fightTask.DropId = ConfigurationHelper.GetValue(ConfigurationKeys.DropsItemId, string.Empty);
                fightTask.DropCount = ConfigurationHelper.GetValue(ConfigurationKeys.DropsQuantity, 5);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.DropsEnable);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.DropsItemId);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.DropsQuantity);

                var stage1 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage1, string.Empty) ?? string.Empty;
                var stage2 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage2, string.Empty) ?? string.Empty;
                var stage3 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage3, string.Empty) ?? string.Empty;
                var stage4 = ConfigurationHelper.GetValue(ConfigurationKeys.Stage4, string.Empty) ?? string.Empty;

                fightTask.StagePlan = [stage1];
                if (ConfigurationHelper.GetValue(ConfigurationKeys.UseAlternateStage, false))
                {
                    fightTask.StagePlan.Add(stage2);
                    fightTask.StagePlan.Add(stage3);
                    fightTask.StagePlan.Add(stage4);
                }

                ConfigurationHelper.DeleteValue(ConfigurationKeys.Stage1);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Stage2);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Stage3);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Stage4);
                fightTask.IsDrGrandet = ConfigurationHelper.GetValue(ConfigurationKeys.IsDrGrandet, false);
                fightTask.UseStoneAllowSave = ConfigurationHelper.GetValue(ConfigurationKeys.AllowUseStoneSave, false);
                fightTask.HideSeries = ConfigurationHelper.GetValue(ConfigurationKeys.HideSeries, false);
                fightTask.UseExpiringMedicine = ConfigurationHelper.GetValue(ConfigurationKeys.UseExpiringMedicine, false);
                fightTask.AnnihilationStage = ConfigurationHelper.GetValue(ConfigurationKeys.AnnihilationStage, FightSettingsUserControlModel.AnnihilationName);
                fightTask.UseCustomAnnihilation = ConfigurationHelper.GetValue(ConfigurationKeys.UseCustomAnnihilation, false) && fightTask.AnnihilationStage != FightSettingsUserControlModel.AnnihilationName;
                fightTask.HideUnavailableStage = ConfigurationHelper.GetValue(ConfigurationKeys.HideUnavailableStage, true);
                fightTask.IsStageManually = ConfigurationHelper.GetValue(ConfigurationKeys.CustomStageCode, false);
                fightTask.UseOptionalStage = ConfigurationHelper.GetValue(ConfigurationKeys.UseAlternateStage, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.IsDrGrandet);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AllowUseStoneSave);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.HideSeries);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseExpiringMedicine);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AnnihilationStage);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseCustomAnnihilation);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.HideUnavailableStage);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CustomStageCode);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UseAlternateStage);
                fightTask.StageResetMode = fightTask.HideUnavailableStage ? FightStageResetMode.Current : FightStageResetMode.Ignore;

                if (fightTask.Series > 6)
                {
                    fightTask.Series = 0;
                }

                stage1 = ConfigurationHelper.GetValue(ConfigurationKeys.RemainingSanityStage, string.Empty);
                fightTask2.StagePlan = [stage1];
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemainingSanityStage);

                infrastTask.Mode = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastMode, InfrastMode.Normal);
                infrastTask.UsesOfDrones = ConfigurationHelper.GetValue(ConfigurationKeys.UsesOfDrones, "Money");
                infrastTask.ReceptionMessageBoard = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionMessageBoardReceive, true);
                infrastTask.ReceptionClueExchange = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionClueExchange, true);
                infrastTask.SendClue = ConfigurationHelper.GetValue(ConfigurationKeys.InfrastReceptionSendClue, true);
                infrastTask.ContinueTraining = ConfigurationHelper.GetValue(ConfigurationKeys.ContinueTraining, false);
                infrastTask.DormThreshold = ConfigurationHelper.GetValue(ConfigurationKeys.DormThreshold, 30);
                infrastTask.DormFilterNotStationed = ConfigurationHelper.GetValue(ConfigurationKeys.DormFilterNotStationedEnabled, true);
                infrastTask.DormTrustEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.DormTrustEnabled, false);
                infrastTask.OriginiumShardAutoReplenishment = ConfigurationHelper.GetValue(ConfigurationKeys.OriginiumShardAutoReplenishment, true);
                infrastTask.Filename = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastFile, string.Empty);
                infrastTask.PlanSelect = ConfigurationHelper.GetValue(ConfigurationKeys.CustomInfrastPlanSelect, -1);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.InfrastMode);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.UsesOfDrones);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.InfrastReceptionMessageBoardReceive);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.InfrastReceptionClueExchange);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.InfrastReceptionSendClue);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ContinueTraining);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.DormThreshold);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.DormFilterNotStationedEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.DormTrustEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.OriginiumShardAutoReplenishment);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CustomInfrastFile);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CustomInfrastPlanSelect);

                try
                {
                    if (string.IsNullOrWhiteSpace(infrastTask.Filename) || !File.Exists(infrastTask.Filename))
                    {
                        throw new FileNotFoundException("CustomInfrastFile not found", infrastTask.Filename);
                    }

                    string jsonStr = File.ReadAllText(infrastTask.Filename);
                    if (JsonConvert.DeserializeObject<CustomInfrastConfig>(jsonStr) is not CustomInfrastConfig root)
                    {
                        throw new JsonException("DeserializeObject returned null");
                    }
                    var planList = root.Plans;
                    for (int i = 0; i < planList.Count; ++i)
                    {
                        var plan = planList[i];
                        plan.Index = i;
                        plan.Name ??= "Plan " + ((char)('A' + i));
                        plan.Description ??= string.Empty;
                        plan.DescriptionPost ??= string.Empty;
                        infrastTask.InfrastPlan.Add(plan);
                    }
                }
                catch
                {
                }
                infrastTask.PlanSelect = Math.Clamp(infrastTask.PlanSelect, -1, infrastTask.InfrastPlan.Count - 1);

                infrastTask.RoomList = [];
                var roomTypes = Enum.GetNames<InfrastRoomType>();
                var list = new List<KeyValuePair<string, int>>();
                foreach (var item in roomTypes)
                {
                    var index = ConfigurationHelper.GetValue("Infrast.Order." + item, -1);
                    list.Add(new(item, index));
                    ConfigurationHelper.DeleteValue("Infrast.Order." + item);
                }

                list.Sort((x, y) => x.Value.CompareTo(y.Value));
                var roomList = new List<InfrastTask.RoomInfo>();
                foreach (var (room, isEnable) in list)
                {
                    if (Enum.TryParse<InfrastRoomType>(room, out var result))
                    {
                        roomList.Add(new(result, ConfigurationHelper.GetValue("Infrast." + room + ".IsChecked", true)));
                        ConfigurationHelper.DeleteValue("Infrast." + room + ".IsChecked");
                    }
                    else
                    {
                        _logger.Error("Enum.TryParse<InfrastRoomType> 失败，room: {Room}", room);
                    }
                }

                infrastTask.RoomList = roomList;

                recruitTask.ExtraTagMode = ConfigurationHelper.GetValue(ConfigurationKeys.SelectExtraTags, 0);
                recruitTask.Level3PreferTags =
                [
                    .. ConfigurationHelper.GetValue(ConfigurationKeys.AutoRecruitFirstList, string.Empty)
                        .Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
                ];
                recruitTask.PreferTagEnabled = recruitTask.Level3PreferTags.Count != 0;
                recruitTask.RefreshLevel3 = ConfigurationHelper.GetValue(ConfigurationKeys.RefreshLevel3, true);
                recruitTask.ForceRefresh = ConfigurationHelper.GetValue(ConfigurationKeys.ForceRefresh, true);
                recruitTask.PreserveTagEnabled = ConfigurationHelper.GetValue(ConfigurationKeys.NotChooseLevel1, true);
                recruitTask.PreserveTagList = recruitTask.PreserveTagEnabled ? [RecruitSettingsUserControlModel.LegacyRobotTag] : [];
                recruitTask.MaxTimes = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitMaxTimes, 4);
                recruitTask.Level3Choose = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel3, true);
                recruitTask.Level3Time = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3Time, 540);
                recruitTask.Level4Choose = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel4, true);
                recruitTask.Level4Time = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4Time, 540);
                recruitTask.Level5Choose = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel5, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.SelectExtraTags);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AutoRecruitFirstList);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RefreshLevel3);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ForceRefresh);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.NotChooseLevel1);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitMaxTimes);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitChooseLevel3);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitChooseLevel4);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitChooseLevel5);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel3Time);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel4Time);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel5Time);

                awardTask.Award = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, true);
                awardTask.Mail = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, false);
                awardTask.FreeGacha = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeGacha, false);
                awardTask.Orundum = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, false);
                awardTask.Mining = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, false);
                awardTask.SpecialAccess = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveAward);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveMail);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveFreeGacha);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveOrundum);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveMining);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveSpecialAccess);

                mallTask.Shopping = ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, true);
                mallTask.FirstList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault", local)).Replace("；", ";").Trim();
                mallTask.BlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault", local)).Replace("；", ";").Trim();
                mallTask.ShoppingIgnoreBlackListWhenFull = ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, false);
                mallTask.OnlyBuyDiscount = ConfigurationHelper.GetValue(ConfigurationKeys.CreditOnlyBuyDiscount, false);
                mallTask.ReserveMaxCredit = ConfigurationHelper.GetValue(ConfigurationKeys.CreditReserveMaxCredit, false);
                mallTask.CreditFight = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightTaskEnabled, false);
                mallTask.CreditFightOnceADay = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightOnceADay, true);
                mallTask.CreditFightLastTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());
                mallTask.CreditFightFormation = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightSelectFormation, 0);
                mallTask.VisitFriends = ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitFriendsEnabled, true);
                mallTask.VisitFriendsOnceADay = ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitOnceADay, false);
                mallTask.VisitFriendsLastTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditVisitFriendsTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditShopping);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFirstListNew);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditBlackListNew);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditForceShoppingIfCreditFull);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditOnlyBuyDiscount);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditReserveMaxCredit);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFightTaskEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFightOnceADay);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFightSelectFormation);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LastCreditFightTaskTime);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditVisitFriendsEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditVisitOnceADay);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LastCreditVisitFriendsTime);

                roguelikeTask.Theme = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeTheme, RoguelikeTheme.Sarkaz);
                roguelikeTask.Difficulty = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDifficulty, int.MaxValue);
                roguelikeTask.Mode = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMode, RoguelikeMode.Exp);
                roguelikeTask.CoreChar = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeCoreChar, string.Empty);
                roguelikeTask.Squad = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeSquad, string.Empty);
                roguelikeTask.SquadCollectible = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeCollectibleModeSquad, string.Empty);
                roguelikeTask.Roles = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeRoles, string.Empty);
                roguelikeTask.StartCount = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartsCount, 9999999);
                roguelikeTask.Investment = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnabled, true);
                roguelikeTask.InvestCount = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestsCount, 9999999);
                roguelikeTask.InvestWithMoreScore = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnterSecondFloor, false);
                roguelikeTask.StopWhenDepositFull = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, false);
                roguelikeTask.StopAtFinalBoss = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopAtFinalBoss, false);
                roguelikeTask.UseSupport = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeUseSupportUnit, false);
                roguelikeTask.UseSupportNonFriend = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, false);
                roguelikeTask.RefreshTraderWithDice = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice, false);
                roguelikeTask.StartWithEliteTwo = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartWithEliteTwo, false);
                roguelikeTask.StartWithEliteTwoOnly = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeOnlyStartWithEliteTwo, false);
                roguelikeTask.SamiFirstFloorFoldartal = ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3FirstFloorFoldartal, false);
                roguelikeTask.SamiFirstFloorFoldartals = ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3FirstFloorFoldartals, string.Empty);
                roguelikeTask.SamiNewSquad2StartingFoldartal = ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartal, false);
                roguelikeTask.SamiNewSquad2StartingFoldartals = ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartals, string.Empty).Replace("；", ";").Trim();
                roguelikeTask.CollectibleShopping = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeCollectibleModeShopping, false);
                roguelikeTask.ExpectedCollapsalParadigms = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeExpectedCollapsalParadigms, string.Empty).Replace("；", ";").Trim();
                roguelikeTask.StopWhenLevelMax = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopAtMaxLevel, false);
                roguelikeTask.MonthlySquadAutoIterate = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMonthlySquadAutoIterate, false);
                roguelikeTask.MonthlySquadCheckComms = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMonthlySquadCheckComms, false);
                roguelikeTask.DeepExplorationAutoIterate = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeepExplorationAutoIterate, false);
                roguelikeTask.FindPlaytimeTarget = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeFindPlaytimeTarget, RoguelikeBoskySubNodeType.Ling);

                Dictionary<string, RoguelikeCollectibleAward> dic = new() {
                    { "Roguelike@LastReward", RoguelikeCollectibleAward.HotWater },
                    { "Roguelike@LastReward2", RoguelikeCollectibleAward.Shield },
                    { "Roguelike@LastReward3", RoguelikeCollectibleAward.Ingot },
                    { "Roguelike@LastReward4", RoguelikeCollectibleAward.Hope },
                    { "Roguelike@LastRewardRand", RoguelikeCollectibleAward.Random },
                    { "Mizuki@Roguelike@LastReward5", RoguelikeCollectibleAward.Key },
                    { "Mizuki@Roguelike@LastReward6", RoguelikeCollectibleAward.Dice },
                    { "Sarkaz@Roguelike@LastReward5", RoguelikeCollectibleAward.Idea },
                };
                roguelikeTask.CollectibleStartAwards = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartWithSelectList, string.Empty)
                    .Split(' ').Select(v => dic.TryGetValue(v, out var @out) ? @out : 0).Aggregate((a, b) => a | b);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeTheme);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeDifficulty);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeMode);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeCoreChar);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeSquad);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeRoles);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartsCount);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeInvestmentEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeInvestsCount);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeInvestmentEnterSecondFloor);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStopAtFinalBoss);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeUseSupportUnit);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithEliteTwo);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeOnlyStartWithEliteTwo);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3FirstFloorFoldartal);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3FirstFloorFoldartals);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartal);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartals);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithSelectList);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeCollectibleModeShopping);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeCollectibleModeSquad);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeFindPlaytimeTarget);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeExpectedCollapsalParadigms);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithSeed);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeMonthlySquadAutoIterate);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeMonthlySquadCheckComms);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeDeepExplorationAutoIterate);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStopAtMaxLevel);

                reclamationTask.Theme = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationTheme, ReclamationTheme.Tales);
                reclamationTask.Mode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMode, ReclamationMode.ProsperityInSave);
                reclamationTask.ToolToCraft = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationToolToCraft, string.Empty).Replace('；', ';').Trim();
                reclamationTask.IncrementMode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationIncrementMode, 0);
                reclamationTask.MaxCraftCountPerRound = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, 16);
                reclamationTask.ClearStore = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationClearStore, true);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationTheme);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationMode);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationToolToCraft);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationIncrementMode);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationClearStore);

                ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.DebugTaskName, out var _);

                // 任务导入排序
                List<(string OldName, int Index, bool IsEnable)> taskList = [("WakeUp", 0, true), ("Recruiting", 1, true), ("Base", 2, true), ("Combat", 3, true), ("Mall", 4, true), ("Mission", 5, true), ("AutoRoguelike", 6, false)];
                if (ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty) is not "txwy")
                {
                    taskList.Add(("Reclamation", 7, false));
                }

                for (int i = 0; i != taskList.Count; ++i)
                {
                    var isEnable = ConfigurationHelper.GetValue($"TaskQueue.{taskList[i].OldName}.IsChecked", false);
                    if (int.TryParse(ConfigurationHelper.GetTaskOrder(taskList[i].OldName, "99"), out var order))
                    {
                        taskList[i] = (taskList[i].OldName, order, isEnable);
                    }
                    else
                    {
                        taskList[i] = (taskList[i].OldName, taskList[i].Index, isEnable);
                    }
                    ConfigurationHelper.DeleteValue($"TaskQueue.{taskList[i].OldName}.IsChecked");
                    ConfigurationHelper.DeleteValue("TaskQueue.Order." + taskList[i].OldName);
                }

                ConfigFactory.CurrentConfig.TaskQueue.Clear();
                taskList.OrderBy(x => x.Index).ToList().ForEach(task => {
                    switch (task.OldName)
                    {
                        case "WakeUp":
                            // startUpTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            startUpTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(startUpTask);
                            break;
                        case "Combat":
                            // fightTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            fightTask2.Name = LocalizationHelper.GetString("RemainingSanityStage", local);
                            fightTask.IsEnable = task.IsEnable;
                            fightTask2.IsEnable = task.IsEnable && ConfigurationHelper.GetValue(ConfigurationKeys.UseRemainingSanityStage, true) && fightTask2.StagePlan.FirstOrDefault() != string.Empty;
                            if (fightTask.UseOptionalStage && fightTask.StagePlan.FirstOrDefault() == FightSettingsUserControlModel.AnnihilationName)
                            {
                                ConfigFactory.CurrentConfig.TaskQueue.Add(new FightTask() { Name = LocalizationHelper.GetString("AnnihilationMode"), StagePlan = [FightSettingsUserControlModel.AnnihilationName] });
                                fightTask.StagePlan.RemoveAt(0);
                            }
                            ConfigFactory.CurrentConfig.TaskQueue.Add(fightTask);
                            ConfigFactory.CurrentConfig.TaskQueue.Add(fightTask2);
                            ConfigurationHelper.DeleteValue(ConfigurationKeys.UseRemainingSanityStage);
                            break;
                        case "Mission":
                            // awardTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            awardTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(awardTask);
                            break;
                        case "Mall":
                            // mallTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            mallTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(mallTask);
                            break;
                        case "Base":
                            // infrastTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            infrastTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(infrastTask);
                            break;
                        case "Recruiting":
                            // recruitTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            recruitTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(recruitTask);
                            break;
                        case "AutoRoguelike":
                            // roguelikeTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            roguelikeTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(roguelikeTask);
                            break;
                        case "Reclamation":
                            // reclamationTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            reclamationTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(reclamationTask);
                            break;
                    }
                });
            }
        }

        ConfigurationHelper.SwitchConfiguration(currentConfigName);
        ConfigFactory.SwitchConfig(currentConfigName);
        return true;
    }

    // 迁移Wpf设置, v6.14编写
    private static bool ConvertWpfSettings()
    {
        BackupOldConfig();

        var currentConfigName = ConfigurationHelper.GetCurrentConfiguration();
        foreach (var configName in ConfigurationHelper.GetConfigurationList())
        {
            ConfigurationHelper.SwitchConfiguration(configName);
            if (!ConfigFactory.SwitchConfig(configName))
            {
                _logger.Error("配置迁移失败，无法切换到配置: {ConfigName}", configName);
                throw new Exception($"配置迁移失败，无法切换到配置{configName}");
            }

            // 性能设置
            {
                ConfigFactory.CurrentConfig.Gui.Performance.UseGpu = ConfigurationHelper.GetValue(ConfigurationKeys.PerformanceUseGpu, false);
                ConfigFactory.CurrentConfig.Gui.Performance.GpuDescription = ConfigurationHelper.GetValue(ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty);
                ConfigFactory.CurrentConfig.Gui.Performance.GpuInstancePath = ConfigurationHelper.GetValue(ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty);
                ConfigFactory.CurrentConfig.Gui.Performance.AllowDeprecatedGpu = ConfigurationHelper.GetValue(ConfigurationKeys.PerformanceAllowDeprecatedGpu, false);

                ConfigurationHelper.DeleteValue(ConfigurationKeys.PerformanceUseGpu);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.PerformancePreferredGpuDescription);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.PerformancePreferredGpuInstancePath);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.PerformanceAllowDeprecatedGpu);
            }

            // 外部通知
            {
                IReadOnlyList<string> externalNotificationProviders =
                    [
                        "ServerChan",
                        "Telegram",
                        "Discord",
                        "DingTalk",
                        "Discord Webhook",
                        "SMTP",
                        "Bark",
                        "Qmsg",
                        "Gotify",
                        "Custom Webhook"
                    ];
                var externalList = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationEnabled, string.Empty)
                    .Split(',')
                    .Where(s => externalNotificationProviders.Contains(s.ToString()))
                    .Distinct()
                    .ToArray();
                var serverChanEnabled = externalList.Contains("ServerChan");
                var telegramEnabled = externalList.Contains("Telegram");
                var discordEnabled = externalList.Contains("Discord");
                var discordWebhookEnabled = externalList.Contains("Discord Webhook");
                var dingTalkEnabled = externalList.Contains("DingTalk");
                var smtpEnabled = externalList.Contains("SMTP");
                var barkEnabled = externalList.Contains("Bark");
                var qmsgEnabled = externalList.Contains("Qmsg");
                var gotifyEnabled = externalList.Contains("Gotify");
                var customWebhookEnabled = externalList.Contains("Custom Webhook");
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationEnabled);
                if (serverChanEnabled)
                {
                    var sendKey = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationServerChanSendKey, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new ServerChan(sendKey));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationServerChanSendKey);

                if (telegramEnabled)
                {
                    var botToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramBotToken, string.Empty);
                    var chatId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramChatId, string.Empty);
                    var topicId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramTopicId, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new Telegram(botToken, chatId, topicId));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationTelegramBotToken);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationTelegramChatId);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationTelegramTopicId);

                if (discordEnabled)
                {
                    var botToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordBotToken, string.Empty);
                    var userId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordUserId, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new Discord(botToken, userId));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDiscordBotToken);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDiscordUserId);

                if (discordWebhookEnabled)
                {
                    var webhookUrl = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordWebhookUrl, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new CustomWebhook(webhookUrl, Body: $"{{\"content\": {{content}}}}"));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDiscordWebhookUrl);

                if (dingTalkEnabled)
                {
                    var accessToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDingTalkAccessToken, string.Empty);
                    var secret = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDingTalkSecret, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new DingTalk(accessToken, secret));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDingTalkAccessToken);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDingTalkSecret);

                if (smtpEnabled)
                {
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpServer, string.Empty);
                    var port = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpPort, string.Empty);
                    var user = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpUser, string.Empty);
                    var password = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpPassword, string.Empty);
                    var from = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpFrom, string.Empty);
                    var to = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpTo, string.Empty);
                    var useSsl = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpUseSsl, false);
                    var requiresAuthentication = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSmtpRequiresAuthentication, false);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new Smtp(server, port, user, password, from, to, useSsl, requiresAuthentication));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpServer);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpPort);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpUser);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpPassword);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpFrom);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpTo);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpUseSsl);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpRequiresAuthentication);

                if (barkEnabled)
                {
                    var sendKey = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkSendKey, string.Empty);
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkServer, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new Bark(sendKey, server));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationBarkSendKey);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationBarkServer);

                if (qmsgEnabled)
                {
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgServer, string.Empty);
                    var key = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgKey, string.Empty);
                    var user = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgUser, string.Empty);
                    var bot = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgBot, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new Qmsg(server, key, user, bot));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgServer);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgKey);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgUser);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgBot);

                if (gotifyEnabled)
                {
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationGotifyServer, string.Empty);
                    var token = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationGotifyToken, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new Gotify(server, token));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationGotifyServer);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationGotifyToken);

                if (customWebhookEnabled)
                {
                    var url = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookUrl, string.Empty);
                    var body = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookBody, string.Empty);
                    var headers = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookHeaders, string.Empty);
                    ConfigFactory.CurrentConfig.Gui.ExternalNotification.Configs.Add(new CustomWebhook(url, headers, body));
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationCustomWebhookUrl);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationCustomWebhookBody);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationCustomWebhookHeaders);

                ConfigFactory.CurrentConfig.Gui.ExternalNotification.SendWhenComplete = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSendWhenComplete, true);
                ConfigFactory.CurrentConfig.Gui.ExternalNotification.ShowWhenCompleteWithDetails = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationEnableDetails, false);
                ConfigFactory.CurrentConfig.Gui.ExternalNotification.SendWhenError = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSendWhenError, true);
                ConfigFactory.CurrentConfig.Gui.ExternalNotification.SendWhenStalled = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSendWhenStalled, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSendWhenError);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSendWhenComplete);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSendWhenStalled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationEnableDetails);

                ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlGetTaskEndpointUri = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlReportStatusUri = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlReportStatusUri, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlUserIdentity = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlUserIdentity, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlDeviceIdentity = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlDeviceIdentity, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RemoteControl.RemoteControlPollIntervalMs = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlPollIntervalMs, 1000);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlReportStatusUri);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlUserIdentity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlDeviceIdentity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlPollIntervalMs);
            }

            // 运行设置
            {
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ClientType = ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, ClientType.Official);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.DeployWithPause = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, false);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PreRunScript = ConfigurationHelper.GetValue(ConfigurationKeys.StartsWithScript, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PostRunScript = ConfigurationHelper.GetValue(ConfigurationKeys.EndsWithScript, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ExecuteScriptOnCopilot = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotWithScript, false);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ExecuteScriptOnManualStop = ConfigurationHelper.GetValue(ConfigurationKeys.ManualStopWithScript, false);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleep = ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleep, false);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleepWithScreenOn = ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleepWithScreenOn, true);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ReportToPenguin = ConfigurationHelper.GetValue(ConfigurationKeys.EnablePenguin, true);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ReportToYituliu = ConfigurationHelper.GetValue(ConfigurationKeys.EnableYituliu, true);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PenguinId = ConfigurationHelper.GetValue(ConfigurationKeys.PenguinId, string.Empty);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.EnableStallTimeout = ConfigurationHelper.GetValue(ConfigurationKeys.StallTimeoutEnabled, true);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutMinutes = ConfigurationHelper.GetValue(ConfigurationKeys.StallTimeoutMinutes, 25).Clamp(0, GameSettingsUserControlModel.TimeoutMaxMinutes);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutReminderIntervalMinutes = ConfigurationHelper.GetValue(ConfigurationKeys.ReminderIntervalMinutes, 30).Clamp(1, GameSettingsUserControlModel.TimeoutMaxMinutes);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StartGame = ConfigurationHelper.GetValue(ConfigurationKeys.StartGame, true);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.StartGame);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ClientType);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeDeploymentWithPause);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.StartsWithScript);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.EndsWithScript);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotWithScript);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ManualStopWithScript);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.BlockSleep);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.BlockSleepWithScreenOn);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.EnablePenguin);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.EnableYituliu);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.PenguinId);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.StallTimeoutEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.StallTimeoutMinutes);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReminderIntervalMinutes);

                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.AutoRestartOnDrop = ConfigurationHelper.GetValue(ConfigurationKeys.AutoRestartOnDrop, true);
                ConfigFactory.CurrentConfig.Gui.RuntimeSettings.RoguelikeDelayAbortUntilCombatComplete = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AutoRestartOnDrop);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete);
            }

            // 连接设置
            {
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AutoDetect = ConfigurationHelper.GetValue(ConfigurationKeys.AutoDetect, true);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AlwaysAutoDetect = ConfigurationHelper.GetValue(ConfigurationKeys.AlwaysAutoDetect, false);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AdbPath = ConfigurationHelper.GetValue(ConfigurationKeys.AdbPath, string.Empty);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.Address = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);
                var listStr = ConfigurationHelper.GetValue(ConfigurationKeys.AddressHistory, string.Empty);
                try
                {
                    ConfigFactory.CurrentConfig.Gui.ConnectSettings.AddressHistory = JsonConvert.DeserializeObject<List<string>>(listStr) ?? [];
                }
                catch
                {
                }
                var connectConfig = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectConfig, "General");
                if (Enum.TryParse<ConnectConfig>(connectConfig, true, out var config))
                {
                    ConfigFactory.CurrentConfig.Gui.ConnectSettings.Config = config;
                }
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AllowAdbRestart = ConfigurationHelper.GetValue(ConfigurationKeys.AllowAdbRestart, true);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AllowAdbHardRestart = ConfigurationHelper.GetValue(ConfigurationKeys.AllowAdbHardRestart, true);
                var touch = ConfigurationHelper.GetValue(ConfigurationKeys.TouchMode, "minitouch");
                if (Enum.TryParse<TouchMode>(touch, true, out var touchMode))
                {
                    ConfigFactory.CurrentConfig.Gui.ConnectSettings.TouchMode = touchMode;
                }
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.EnableAdbLite = ConfigurationHelper.GetValue(ConfigurationKeys.AdbLiteEnabled, false);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.KillAdbOnExit = ConfigurationHelper.GetValue(ConfigurationKeys.KillAdbOnExit, false);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.AdbReplaced = ConfigurationHelper.GetValue(ConfigurationKeys.AdbReplaced, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AdbReplaced);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AutoDetect);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AlwaysAutoDetect);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AdbPath);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ConnectAddress);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AddressHistory);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ConnectConfig);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AllowAdbRestart);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AllowAdbHardRestart);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.TouchMode);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AdbLiteEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.KillAdbOnExit);
                {
                    var extra = new MuMu12Extra(
                        ConfigurationHelper.GetValue(ConfigurationKeys.MuMu12ExtrasEnabled, false),
                        ConfigurationHelper.GetValue(ConfigurationKeys.MuMu12EmulatorPath, string.Empty),
                        ConfigurationHelper.GetValue(ConfigurationKeys.MumuBridgeConnection, false),
                        ConfigurationHelper.GetValue(ConfigurationKeys.MuMu12Index, 0));
                    ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.MuMuEmulator12 = extra;
                }
                {
                    var extra = new LDPlayerExtra(
                        ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerExtrasEnabled, false),
                        ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerEmulatorPath, string.Empty),
                        ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerManualSetIndex, false),
                        ConfigurationHelper.GetValue(ConfigurationKeys.LdPlayerIndex, 0));
                    ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.LDPlayer = extra;
                }
                {
                    var extra = new Win32Extra(
                         ConfigurationHelper.GetValue(ConfigurationKeys.AttachWindowScreencapMethod, AsstWin32ScreencapMethod.FramePool),
                         ConfigurationHelper.GetValue(ConfigurationKeys.AttachWindowMouseMethod, AsstWin32InputMethod.SendMessageWithCursorPos),
                         ConfigurationHelper.GetValue(ConfigurationKeys.AttachWindowKeyboardMethod, AsstWin32KeyboardInputMethod.SendMessage));
                    ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.Win32Extra = extra;
                }
                ConfigurationHelper.DeleteValue(ConfigurationKeys.MuMu12ExtrasEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.MuMu12EmulatorPath);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.MumuBridgeConnection);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.MuMu12Index);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LdPlayerExtrasEnabled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LdPlayerEmulatorPath);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LdPlayerManualSetIndex);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LdPlayerIndex);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AttachWindowScreencapMethod);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AttachWindowMouseMethod);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AttachWindowKeyboardMethod);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.BluestacksExtra.ConfigKeyword = ConfigurationHelper.GetValue(ConfigurationKeys.BluestacksConfigKeyword, string.Empty);
                ConfigFactory.CurrentConfig.Gui.ConnectSettings.Extras.BluestacksExtra.ConfigPath = ConfigurationHelper.GetValue(ConfigurationKeys.BluestacksConfigPath, string.Empty);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.BluestacksConfigError);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.BluestacksConfigKeyword);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.BluestacksConfigPath);
            }

            // 启动设置
            {
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.RunDirectly = ConfigurationHelper.GetValue(ConfigurationKeys.RunDirectly, false);
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.StartEmulator = ConfigurationHelper.GetValue(ConfigurationKeys.StartEmulator, false);
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.RestartEmulatorWhenAdbFailed = ConfigurationHelper.GetValue(ConfigurationKeys.RetryOnAdbDisconnected, false);
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorPath = ConfigurationHelper.GetValue(ConfigurationKeys.EmulatorPath, string.Empty);
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorAddCommand = ConfigurationHelper.GetValue(ConfigurationKeys.EmulatorAddCommand, string.Empty);
                ConfigFactory.CurrentConfig.Gui.StartUpSettings.EmulatorWaitSeconds = ConfigurationHelper.GetValue(ConfigurationKeys.EmulatorWaitSeconds, 60);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RunDirectly);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.StartEmulator);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RetryOnAdbDisconnected);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.EmulatorPath);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.EmulatorAddCommand);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.EmulatorWaitSeconds);
            }

            ConfigFactory.CurrentConfig.Gui.WindowTitlePrefix = ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitlePrefix, string.Empty);
            ConfigurationHelper.DeleteValue(ConfigurationKeys.WindowTitlePrefix);
            ConfigFactory.CurrentConfig.Gui.PostActions = JsonConvert.DeserializeObject<PostActions>(ConfigurationHelper.GetValue(ConfigurationKeys.PostActions, "0"));
            ConfigurationHelper.DeleteValue(ConfigurationKeys.PostActions);
            ConfigFactory.CurrentConfig.Gui.AchievementPopupDisabled = ConfigurationHelper.GetValue(ConfigurationKeys.AchievementPopupDisabled, false);
            ConfigFactory.CurrentConfig.Gui.AchievementPopupAutoClose = ConfigurationHelper.GetValue(ConfigurationKeys.AchievementPopupAutoClose, false);
            ConfigurationHelper.DeleteValue(ConfigurationKeys.AchievementPopupDisabled);
            ConfigurationHelper.DeleteValue(ConfigurationKeys.AchievementPopupAutoClose);

            // Copilot
            {
                try
                {
                    var copilotTaskList = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotTaskList, string.Empty);
                    if (!string.IsNullOrEmpty(copilotTaskList))
                    {
                        var list = JsonConvert.DeserializeObject<List<CopilotItemViewModel>>(copilotTaskList) ?? [];
                        ConfigFactory.CurrentConfig.Copilot.TaskList = [.. list.Select((c, i) => {
                            c.Index = i;
                            return c;
                        })];
                    }
                }
                catch
                {
                }

                try
                {
                    var userAdditional = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotUserAdditional, string.Empty);
                    if (!string.IsNullOrEmpty(userAdditional))
                    {
                        var list = JsonConvert.DeserializeObject<List<UserAdditional>>(userAdditional) ?? [];
                        ConfigFactory.CurrentConfig.Copilot.UserAdditional = list;
                    }
                }
                catch
                {
                }

                ConfigFactory.CurrentConfig.Copilot.EnableUserAdditional = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotAddUserAdditional, false);
                ConfigFactory.CurrentConfig.Copilot.SelectFormation = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotSelectFormation, 1);
                ConfigFactory.CurrentConfig.Copilot.LoopTimes = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotLoopTimes, 1);
                ConfigFactory.CurrentConfig.Copilot.SupportMode = (CopilotSupportMode)ConfigurationHelper.GetValue(ConfigurationKeys.CopilotSupportUnitUsage, 1);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotTaskList);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotUserAdditional);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotAddUserAdditional);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotSelectFormation);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotLoopTimes);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CopilotSupportUnitUsage);
            }

            // 小工具
            {
                {
                    var saved = ConfigurationHelper.GetValue(ConfigurationKeys.OperBoxSelectedExportValue, "0");
                    if (int.TryParse(saved, out var val) && Enum.IsDefined(typeof(OperBoxExportFormat), val))
                    {
                        ConfigFactory.CurrentConfig.Toolbox.OperBoxExportFormat = (OperBoxExportFormat)val;
                    }
                    else if (Enum.TryParse<OperBoxExportFormat>(saved, out var fmt))
                    {
                        ConfigFactory.CurrentConfig.Toolbox.OperBoxExportFormat = fmt;
                    }
                }
                ConfigFactory.CurrentConfig.Toolbox.GachaShowDisclaimerNoMore = ConfigurationHelper.GetValue(ConfigurationKeys.GachaShowDisclaimerNoMore, false);
                ConfigFactory.CurrentConfig.Toolbox.PeepTargetFps = ConfigurationHelper.GetValue(ConfigurationKeys.PeepTargetFps, 20);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.OperBoxSelectedExportValue);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.GachaShowDisclaimerNoMore);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.PeepTargetFps);

                ConfigFactory.CurrentConfig.Toolbox.ChooseLevel3 = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3, true);
                ConfigFactory.CurrentConfig.Toolbox.ChooseLevel4 = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4, true);
                ConfigFactory.CurrentConfig.Toolbox.ChooseLevel5 = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5, true);
                ConfigFactory.CurrentConfig.Toolbox.ChooseLevel6 = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel6, true);
                ConfigFactory.CurrentConfig.Toolbox.ChooseLevel3Time = ConfigurationHelper.GetValue(ConfigurationKeys.ToolBoxChooseLevel3Time, 540);
                ConfigFactory.CurrentConfig.Toolbox.ChooseLevel4Time = ConfigurationHelper.GetValue(ConfigurationKeys.ToolBoxChooseLevel4Time, 540);
                ConfigFactory.CurrentConfig.Toolbox.AutoSetTime = ConfigurationHelper.GetValue(ConfigurationKeys.AutoSetTime, true);
                ConfigFactory.CurrentConfig.Toolbox.ShowPotential = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitmentShowPotential, true);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel3);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel4);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel5);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel6);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ToolBoxChooseLevel3Time);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ToolBoxChooseLevel4Time);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AutoSetTime);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitmentShowPotential);
                ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MiniGameTaskName, out var _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ToolBoxChooseLevel5Time);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.MiniGameSecretFrontEnding);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.MiniGameSecretFrontEvent);
            }
        }

        ConfigurationHelper.SwitchConfiguration(currentConfigName);
        ConfigFactory.SwitchConfig(currentConfigName);

        // 更新设置
        {
            ConfigFactory.Root.Update.Name = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionName, string.Empty);
            ConfigFactory.Root.Update.UpdatePackage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
            ConfigFactory.Root.Update.IsFirstBoot = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdateIsFirstBoot, false);
            ConfigFactory.Root.Update.DoNotShowUpdate = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdateDoNotShowUpdate, false);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.VersionName, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.VersionUpdatePackage, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.VersionUpdateIsFirstBoot, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.VersionUpdateDoNotShowUpdate, out var _);

            ConfigFactory.Root.Update.VersionType = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionType, UpdateVersionType.Stable);
            ConfigFactory.Root.Update.AllowNightlyUpdates = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AllowNightlyUpdates, false);
            ConfigFactory.Root.Update.HasAcknowledgedNightlyWarning = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.HasAcknowledgedNightlyWarning, false);
            ConfigFactory.Root.Update.UpdateSource = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UpdateSource, "Github");
            ConfigFactory.Root.Update.ForceGithubGlobalSource = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ForceGithubGlobalSource, false);
            ConfigFactory.Root.Update.MirrorChyanCdk = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MirrorChyanCdk, string.Empty);
            ConfigFactory.Root.Update.MirrorChyanCdkExpiredTime = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MirrorChyanCdkExpiredTime, 0L);
            ConfigFactory.Root.Update.CheckOnStartup = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.StartupUpdateCheck, true);
            ConfigFactory.Root.Update.CheckOnSchedule = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UpdateAutoCheck, false);
            ConfigFactory.Root.Update.Proxy = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UpdateProxy, string.Empty);
            ConfigFactory.Root.Update.ProxyType = CapitalizeFirst(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ProxyType, "Http"));
            ConfigFactory.Root.Update.AutoDownloadUpdatePackage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AutoDownloadUpdatePackage, true);
            ConfigFactory.Root.Update.AutoInstallUpdatePackage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.AutoInstallUpdatePackage, false);
            ConfigFactory.Root.Update.ShowUpdaterProgress = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ShowUpdaterProgress, true);
            ConfigFactory.Root.Update.ShowUpdaterConsole = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ShowUpdaterConsole, false);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.VersionType, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.AllowNightlyUpdates, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.HasAcknowledgedNightlyWarning, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.UpdateSource, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.ForceGithubGlobalSource, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MirrorChyanCdk, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MirrorChyanCdkExpiredTime, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.StartupUpdateCheck, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.UpdateAutoCheck, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.UpdateProxy, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.ProxyType, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.AutoDownloadUpdatePackage, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.AutoInstallUpdatePackage, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.ShowUpdaterProgress, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.ShowUpdaterConsole, out var _);
        }

        // Gui设置
        {
            ConfigFactory.Root.Gui.Localization = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);
            ConfigFactory.Root.Gui.OperNameLanguage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.OperNameLanguage, "OperNameLanguageMAA");
            ConfigFactory.Root.Gui.UseTray = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.UseTray, true);
            ConfigFactory.Root.Gui.MinimizeToTray = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MinimizeToTray, false);
            ConfigFactory.Root.Gui.MinimizeOnStartup = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MinimizeDirectly, false);
            ConfigFactory.Root.Gui.HideCloseButton = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.HideCloseButton, false);
            ConfigFactory.Root.Gui.WindowTitleScrollable = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowTitleScrollable, false);
            ConfigFactory.Root.Gui.LogItemDateFormat = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.LogItemDateFormat, "HH:mm:ss");
            var jsonStr = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowPlacement, string.Empty);
            if (!string.IsNullOrEmpty(jsonStr))
            {
                try
                {
                    ConfigFactory.Root.Gui.WindowPlacement = JsonConvert.DeserializeObject<WindowPlacement?>(jsonStr);
                }
                catch
                {
                }
            }
            ConfigFactory.Root.Gui.LoadWindowPlacement = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.LoadWindowPlacement, true);
            ConfigFactory.Root.Gui.SaveWindowPlacement = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.SaveWindowPlacement, true);
            ConfigFactory.Root.Gui.InverseClearMode = Enum.TryParse(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.InverseClearMode, InverseClearType.Clear.ToString()), out InverseClearType temp) ? temp : InverseClearType.Clear;
            ConfigFactory.Root.Gui.TaskQueueInverseMode = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.MainFunctionInverseMode, false);
            ConfigFactory.Root.Gui.UseCardLog = ConfigurationHelper.GetValue(ConfigurationKeys.UseCardLog, true);
            ConfigFactory.Root.Gui.MaxNumberOfLogThumbnails = ConfigurationHelper.GetValue(ConfigurationKeys.MaxNumberOfLogThumbnails, 100);
            ConfigFactory.Root.Gui.SoberLanguage = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.SoberLanguage, LocalizationHelper.DefaultLanguage);
            ConfigFactory.Root.Gui.Hangover = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Hangover, false);
            var time = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.LastBuyWineTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());
            ConfigFactory.Root.Gui.LastBuyWineTime = new DateTimeOffset(DateTime.ParseExact(time.Replace('-', '/'), "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture));
            ConfigFactory.Root.Gui.CustomCulture = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.CustomCulture, string.Empty);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.Localization, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.OperNameLanguage, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.UseTray, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MinimizeToTray, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MinimizeDirectly, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.HideCloseButton, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.WindowTitleScrollable, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.LogItemDateFormat, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.WindowPlacement, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.LoadWindowPlacement, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.SaveWindowPlacement, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.InverseClearMode, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MainFunctionInverseMode, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.UseCardLog, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.MaxNumberOfLogThumbnails, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.SoberLanguage, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.Hangover, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.LastBuyWineTime, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.CustomCulture, out var _);
            ConfigFactory.Root.Gui.WindowTitleSelectShowList = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.WindowTitleSelectShowList, "2 3 4");
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.WindowTitleSelectShowList, out var _);

            ConfigFactory.Root.Gui.Background.ImagePath = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundImagePath, "background/background.png");
            ConfigFactory.Root.Gui.Background.StretchMode = Enum.Parse<Stretch>(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundImageStretchMode, Stretch.Fill.ToString()));
            ConfigFactory.Root.Gui.Background.Opacity = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundOpacity, 50);
            ConfigFactory.Root.Gui.Background.BlurEffectRadius = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.BackgroundBlurEffectRadius, 5);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.BackgroundImagePath, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.BackgroundImageStretchMode, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.BackgroundOpacity, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.BackgroundBlurEffectRadius, out var _);
            ConfigFactory.Root.Gui.GuideStep = ConfigurationHelper.GetValue(ConfigurationKeys.GuideStepIndex, 0);
            ConfigurationHelper.DeleteValue(ConfigurationKeys.GuideStepIndex);
            ConfigFactory.Root.Gui.HotKeys = ConfigurationHelper.GetGlobalValue("HotKeys", string.Empty);
            ConfigurationHelper.DeleteGlobalValue("HotKeys", out var _);
            try
            {
                var saved = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.OverlayTarget, string.Empty);
                if (!string.IsNullOrWhiteSpace(saved))
                {
                    var info = JsonConvert.DeserializeObject<OverlayTargetInfo>(saved);
                    if (info != null)
                    {
                        ConfigFactory.Root.Gui.OverlayTarget = info;
                    }
                }

                ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.OverlayTarget, out var _);
            }
            catch
            {
            }
        }

        // Timer
        {
            ConfigFactory.Root.TimerSettings.CustomConfig = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.CustomConfig, false);
            ConfigFactory.Root.TimerSettings.ShowWindowBeforeForceScheduledStart = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ShowWindowBeforeForceScheduledStart, false);
            ConfigFactory.Root.TimerSettings.ForceScheduledStart = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.ForceScheduledStart, false);
            ConfigFactory.Root.TimerSettings.List.Clear();
            for (int i = 0; i < 8; i++)
            {
                var timerState = ConfigurationHelper.GetTimer(i, bool.FalseString);
                bool? isOn = bool.TryParse(timerState, out bool parsedBool) ? parsedBool : null;
                var hour = int.Parse(ConfigurationHelper.GetTimerHour(i, $"{i * 3}"));
                var minute = int.Parse(ConfigurationHelper.GetTimerMin(i, "0"));
                var config = ConfigurationHelper.GetTimerConfig(i, ConfigurationHelper.GetCurrentConfiguration());
                var timer = new Timer(i, isOn, config, hour, minute);
                ConfigFactory.Root.TimerSettings.List.Add(timer);
            }
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.CustomConfig, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.ShowWindowBeforeForceScheduledStart, out var _);
            ConfigurationHelper.DeleteGlobalValue(ConfigurationKeys.ForceScheduledStart, out var _);
        }

        return true;

        static string CapitalizeFirst(string? input)
        {
            if (string.IsNullOrEmpty(input))
            {
                return input ?? string.Empty;
            }
            else if (input.Length == 1)
            {
                return char.ToUpperInvariant(input[0]).ToString();
            }

            return char.ToUpperInvariant(input[0]) + input[1..];
        }
    }

    private static JObject? ParseJsonFile(string filePath)
    {
        if (File.Exists(filePath) is false)
        {
            return null;
        }

        var str = File.ReadAllText(filePath);
        try
        {
            var obj = (JObject?)JsonConvert.DeserializeObject(str);
            return obj ?? throw new Exception("Failed to parse json file");
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to deserialize json file: {FilePath}", filePath);
        }

        return null;
    }

    private static void BackupOldConfig()
    {
        if (HasBackupOldConfig)
        {
            return;
        }
        HasBackupOldConfig = true;
        try
        {
            File.Copy(ConfigurationOldFile, ConfigurationOldBakFile, true);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "备份配置失败: {Message}", ex.Message);
        }
    }
}
