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
using System.IO;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Models;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

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
                ConfigFactory.CurrentConfig.WpfSettings.Performance.UseGpu = ConfigurationHelper.GetValue(ConfigurationKeys.PerformanceUseGpu, false);
                ConfigFactory.CurrentConfig.WpfSettings.Performance.GpuDescription = ConfigurationHelper.GetValue(ConfigurationKeys.PerformancePreferredGpuDescription, string.Empty);
                ConfigFactory.CurrentConfig.WpfSettings.Performance.GpuInstancePath = ConfigurationHelper.GetValue(ConfigurationKeys.PerformancePreferredGpuInstancePath, string.Empty);
                ConfigFactory.CurrentConfig.WpfSettings.Performance.AllowDeprecatedGpu = ConfigurationHelper.GetValue(ConfigurationKeys.PerformanceAllowDeprecatedGpu, false);

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
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationServerChanSendKey);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new ServerChan(sendKey));
                }

                if (telegramEnabled)
                {
                    var botToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramBotToken, string.Empty);
                    var chatId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramChatId, string.Empty);
                    var topicId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationTelegramTopicId, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationTelegramBotToken);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationTelegramChatId);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationTelegramTopicId);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new Telegram(botToken, chatId, topicId));
                }

                if (discordEnabled)
                {
                    var botToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordBotToken, string.Empty);
                    var userId = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordUserId, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDiscordBotToken);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDiscordUserId);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new Discord(botToken, userId));
                }

                if (discordWebhookEnabled)
                {
                    var webhookUrl = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDiscordWebhookUrl, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDiscordWebhookUrl);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new CustomWebhook(webhookUrl, Body: $"{{\"content\": {{content}}}}"));
                }

                if (dingTalkEnabled)
                {
                    var accessToken = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDingTalkAccessToken, string.Empty);
                    var secret = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationDingTalkSecret, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDingTalkAccessToken);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationDingTalkSecret);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new DingTalk(accessToken, secret));
                }

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
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpServer);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpPort);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpUser);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpPassword);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpFrom);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpTo);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpUseSsl);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSmtpRequiresAuthentication);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new Smtp(server, port, user, password, from, to, useSsl, requiresAuthentication));
                }

                if (barkEnabled)
                {
                    var sendKey = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkSendKey, string.Empty);
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationBarkServer, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationBarkSendKey);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationBarkServer);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new Bark(sendKey, server));
                }

                if (qmsgEnabled)
                {
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgServer, string.Empty);
                    var key = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgKey, string.Empty);
                    var user = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgUser, string.Empty);
                    var bot = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationQmsgBot, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgServer);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgKey);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgUser);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationQmsgBot);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new Qmsg(server, key, user, bot));
                }

                if (gotifyEnabled)
                {
                    var server = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationGotifyServer, string.Empty);
                    var token = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationGotifyToken, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationGotifyServer);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationGotifyToken);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new Gotify(server, token));
                }

                if (customWebhookEnabled)
                {
                    var url = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookUrl, string.Empty);
                    var body = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookBody, string.Empty);
                    var headers = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookHeaders, string.Empty);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationCustomWebhookUrl);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationCustomWebhookBody);
                    ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationCustomWebhookHeaders);
                    ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.Configs.Add(new CustomWebhook(url, headers, body));
                }

                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenComplete = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSendWhenComplete, true);
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.ShowWhenCompleteWithDetails = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationEnableDetails, false);
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenError = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSendWhenError, true);
                ConfigFactory.CurrentConfig.WpfSettings.ExternalNotification.SendWhenStalled = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationSendWhenStalled, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSendWhenError);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSendWhenComplete);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationSendWhenStalled);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ExternalNotificationEnableDetails);

                ConfigFactory.CurrentConfig.WpfSettings.RemoteControl.RemoteControlGetTaskEndpointUri = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri, string.Empty));
                ConfigFactory.CurrentConfig.WpfSettings.RemoteControl.RemoteControlReportStatusUri = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlReportStatusUri, string.Empty));
                ConfigFactory.CurrentConfig.WpfSettings.RemoteControl.RemoteControlUserIdentity = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlUserIdentity, string.Empty));
                ConfigFactory.CurrentConfig.WpfSettings.RemoteControl.RemoteControlDeviceIdentity = SimpleEncryptionHelper.Decrypt(ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlDeviceIdentity, string.Empty));
                ConfigFactory.CurrentConfig.WpfSettings.RemoteControl.RemoteControlPollIntervalMs = ConfigurationHelper.GetValue(ConfigurationKeys.RemoteControlPollIntervalMs, 1000);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlGetTaskEndpointUri);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlReportStatusUri);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlUserIdentity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlDeviceIdentity);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemoteControlPollIntervalMs);
            }

            // 运行设置
            {
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.ClientType = ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, ClientType.Official);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.DeployWithPause = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, false);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.PreRunScript = ConfigurationHelper.GetValue(ConfigurationKeys.StartsWithScript, string.Empty);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.PostRunScript = ConfigurationHelper.GetValue(ConfigurationKeys.EndsWithScript, string.Empty);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.ExecuteScriptOnCopilot = ConfigurationHelper.GetValue(ConfigurationKeys.CopilotWithScript, false);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.ExecuteScriptOnManualStop = ConfigurationHelper.GetValue(ConfigurationKeys.ManualStopWithScript, false);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.BlockSleep = ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleep, false);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.BlockSleepWithScreenOn = ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleepWithScreenOn, true);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.ReportToPenguin = ConfigurationHelper.GetValue(ConfigurationKeys.EnablePenguin, true);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.ReportToYituliu = ConfigurationHelper.GetValue(ConfigurationKeys.EnableYituliu, true);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.PenguinId = ConfigurationHelper.GetValue(ConfigurationKeys.PenguinId, string.Empty);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.EnableStallTimeout = ConfigurationHelper.GetValue(ConfigurationKeys.StallTimeoutEnabled, true);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.StallTimeoutMinutes = ConfigurationHelper.GetValue(ConfigurationKeys.StallTimeoutMinutes, 25).Clamp(0, GameSettingsUserControlModel.TimeoutMaxMinutes);
                ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.StallTimeoutReminderIntervalMinutes = ConfigurationHelper.GetValue(ConfigurationKeys.ReminderIntervalMinutes, 30).Clamp(1, GameSettingsUserControlModel.TimeoutMaxMinutes);

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
            }
        }

        ConfigurationHelper.SwitchConfiguration(currentConfigName);
        ConfigFactory.SwitchConfig(currentConfigName);
        return true;
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
