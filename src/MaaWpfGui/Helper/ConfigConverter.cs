// <copyright file="ConfigConverter.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Configuration;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Helper;

public class ConfigConverter
{
    private static readonly string ConfigurationNewFile = ConfigFactory.ConfigFileName;
    private static readonly string ConfigurationOldBakFile = ConfigurationHelper.ConfigurationFile + ".old";
    private static readonly string ConfigurationOldFile = ConfigurationHelper.ConfigurationFile;

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

        var parsedNew = ParseJsonFile(ConfigurationNewFile);

        // TODO 测试项, 需移除
        parsedNew?.Remove(nameof(ConfigFactory.Root.ConfigVersion));
        Debug.Assert(Instances.VersionUpdateViewModel.IsDebugVersion(), "测试项请移除");
        try
        {
            File.Copy(ConfigurationOldFile, ConfigurationOldBakFile + $"_{DateTimeOffset.Now:MM-dd_HH-mm-ss}", true);
        }
        catch (Exception e)
        {
            Log.Error(e, "备份旧配置文件失败");
        }

        int curVersion = 0;
        bool ret = true;
        if (parsedNew?.TryGetValue(nameof(ConfigFactory.Root.ConfigVersion), out JToken? configVersion) is true && configVersion is { Type: JTokenType.Integer } version)
        {
            curVersion = version.ToObject<int>();
            if (curVersion == new Root().ConfigVersion)
            {
                return true;
            }
        }

        RemoveTooOldKey(curVersion);
        ret &= ConvertTaskQueue(curVersion);
        return ret;
    }

    private static void RemoveTooOldKey(int curVersion)
    {
        if (curVersion >= 1)
        {
            return;
        }

        string[] configKeys = [ConfigurationKeys.AnnouncementInfo, ConfigurationKeys.DoNotRemindThisAnnouncementAgain, ConfigurationKeys.DoNotShowAnnouncement,
            ConfigurationKeys.VersionName, ConfigurationKeys.VersionUpdateBody, ConfigurationKeys.VersionUpdateIsFirstBoot, ConfigurationKeys.VersionUpdatePackage, ConfigurationKeys.VersionUpdateDoNotShowUpdate,
        ];
        foreach (var configName in ConfigurationHelper.GetConfigurationList())
        {
            ConfigurationHelper.SwitchConfiguration(configName);
            ConfigFactory.AddConfiguration(configName);
            ConfigFactory.SwitchConfig(configName);

            // 删除旧的配置
            foreach (var key in configKeys)
            {
                ConfigurationHelper.DeleteValue(key, out _);
            }
        }
    }

    // 迁移任务队列，v5.15编写
    private static bool ConvertTaskQueue(int curVersion)
    {
        if (curVersion >= 1)
        {
            //return true;
        }

        var currentConfigName = ConfigurationHelper.GetCurrentConfiguration();
        foreach (var configName in ConfigurationHelper.GetConfigurationList())
        {
            ConfigurationHelper.SwitchConfiguration(configName);
            ConfigFactory.SwitchConfig(configName);
            var local = GuiSettingsUserControlModel.Instance.Language;

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
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AccountName, out _);

                fightTask2.Stage1 = ConfigurationHelper.GetValue(ConfigurationKeys.RemainingSanityStage, string.Empty);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RemainingSanityStage, out _);

                recruitTask.ExtraTagMode = ConfigurationHelper.GetValue(ConfigurationKeys.SelectExtraTags, 0);
                recruitTask.Level3PreferTags = [.. ConfigurationHelper.GetValue(ConfigurationKeys.AutoRecruitFirstList, string.Empty).Split(";")];
                recruitTask.RefreshLevel3 = ConfigurationHelper.GetValue(ConfigurationKeys.RefreshLevel3, true);
                recruitTask.ForceRefresh = ConfigurationHelper.GetValue(ConfigurationKeys.ForceRefresh, true);
                recruitTask.Level1NotChoose = ConfigurationHelper.GetValue(ConfigurationKeys.NotChooseLevel1, true);
                recruitTask.MaxTimes = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitMaxTimes, 4);
                recruitTask.Level3Choose = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel3, true);
                recruitTask.Level3Time = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel3Time, 540);
                recruitTask.Level4Choose = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel4, true);
                recruitTask.Level4Time = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel4Time, 540);
                recruitTask.Level5Choose = ConfigurationHelper.GetValue(ConfigurationKeys.RecruitChooseLevel5, false);
                recruitTask.Level5Time = ConfigurationHelper.GetValue(ConfigurationKeys.ChooseLevel5Time, 540);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.SelectExtraTags, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AutoRecruitFirstList, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RefreshLevel3, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ForceRefresh, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.NotChooseLevel1, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitMaxTimes, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitChooseLevel3, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitChooseLevel4, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RecruitChooseLevel5, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel3Time, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel4Time, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ChooseLevel5Time, out _);

                awardTask.Award = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, true);
                awardTask.Mail = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, false);
                awardTask.FreeGacha = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeGacha, false);
                awardTask.Orundum = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, false);
                awardTask.Mining = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, false);
                awardTask.SpecialAccess = ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, false);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveAward, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveMail, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveFreeGacha, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveOrundum, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveMining, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveSpecialAccess, out _);

                mallTask.Shopping = ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, true);
                mallTask.FirstList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault", local)).Replace("；", ";").Trim();
                mallTask.BlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault", local)).Replace("；", ";").Trim();
                mallTask.ShoppingIgnoreBlackListWhenFull = ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, false);
                mallTask.OnlyBuyDiscount = ConfigurationHelper.GetValue(ConfigurationKeys.CreditOnlyBuyDiscount, false);
                mallTask.ReserveMaxCredit = ConfigurationHelper.GetValue(ConfigurationKeys.CreditReserveMaxCredit, false);
                mallTask.CreditFight = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightTaskEnabled, false);
                mallTask.CreditFightFormation = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightSelectFormation, 0);
                mallTask.CreditFightLastTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());
                mallTask.VisitFriends = ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitFriendsEnabled, true);
                mallTask.VisitFriendsOnceADay = ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitOnceADay, false);
                mallTask.VisitFriendsLastTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditVisitFriendsTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditShopping, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFirstListNew, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditBlackListNew, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditOnlyBuyDiscount, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditReserveMaxCredit, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFightTaskEnabled, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditFightSelectFormation, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LastCreditFightTaskTime, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditVisitFriendsEnabled, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.CreditVisitOnceADay, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.LastCreditVisitFriendsTime, out _);

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

                Dictionary<string, RoguelikeCollectibleAward> dic = new()
                {
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
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeTheme, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeDifficulty, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeMode, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeCoreChar, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeSquad, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeRoles, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartsCount, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeInvestmentEnabled, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeInvestsCount, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeInvestmentEnterSecondFloor, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStopAtFinalBoss, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeUseSupportUnit, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithEliteTwo, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeOnlyStartWithEliteTwo, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3FirstFloorFoldartal, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3FirstFloorFoldartals, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartal, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartals, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithTwoIdeas, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithSelectList, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeCollectibleModeShopping, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeExpectedCollapsalParadigms, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStartWithSeed, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeMonthlySquadAutoIterate, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeMonthlySquadCheckComms, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeDeepExplorationAutoIterate, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.RoguelikeStopAtMaxLevel, out _);

                reclamationTask.Theme = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationTheme, ReclamationTheme.Tales);
                reclamationTask.Mode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMode, ReclamationMode.Archive);
                reclamationTask.ToolToCraft = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationToolToCraft, string.Empty).Replace('；', ';').Trim();
                reclamationTask.IncrementMode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationIncrementMode, 0);
                reclamationTask.MaxCraftCountPerRound = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, 16);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationTheme, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationMode, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationToolToCraft, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationIncrementMode, out _);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, out _);

                // 任务导入排序
                List<(string OldName, int Index, bool IsEnable)> taskList = [("WakeUp", 0, true), ("Recruiting", 1, true), ("Base", 2, true), ("Combat", 3, true), ("Mall", 4, true), ("Mission", 5, true), ("AutoRoguelike", 6, false)];
                if (ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty) is not "txwy")
                {
                    taskList.Add(("Reclamation", 7, false));
                }

                for (int i = 0; i != taskList.Count; ++i)
                {
                    var isEnable = Convert.ToBoolean(ConfigurationHelper.GetValue($"TaskQueue.{taskList[i].OldName}.IsChecked", bool.FalseString));
                    if (int.TryParse(ConfigurationHelper.GetTaskOrder(taskList[i].OldName, "99"), out var order))
                    {
                        taskList[i] = (taskList[i].OldName, order, isEnable);
                    }
                    else
                    {
                        taskList[i] = (taskList[i].OldName, taskList[i].Index, isEnable);
                    }
                }

                ConfigFactory.CurrentConfig.TaskQueue.Clear();
                taskList.OrderBy(x => x.Index).ToList().ForEach(task =>
                {
                    switch (task.OldName)
                    {
                        case "WakeUp":
                            startUpTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            startUpTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(startUpTask);
                            break;
                        case "Combat":
                            fightTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            fightTask2.Name = LocalizationHelper.GetString("RemainingSanityStage", local);
                            fightTask.IsEnable = task.IsEnable;
                            fightTask2.IsEnable = task.IsEnable && ConfigurationHelper.GetValue(ConfigurationKeys.UseRemainingSanityStage, true);
                            ConfigFactory.CurrentConfig.TaskQueue.Add(fightTask);
                            ConfigFactory.CurrentConfig.TaskQueue.Add(fightTask2);
                            break;
                        case "Mission":
                            awardTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            awardTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(awardTask);
                            break;
                        case "Mall":
                            mallTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            mallTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(mallTask);
                            break;
                        case "Base":
                            infrastTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            infrastTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(infrastTask);
                            break;
                        case "Recruiting":
                            recruitTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            recruitTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(recruitTask);
                            break;
                        case "AutoRoguelike":
                            roguelikeTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            roguelikeTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(roguelikeTask);
                            break;
                        case "Reclamation":
                            reclamationTask.Name = LocalizationHelper.GetString(task.OldName, local);
                            reclamationTask.IsEnable = task.IsEnable;
                            ConfigFactory.CurrentConfig.TaskQueue.Add(reclamationTask);
                            break;
                    }
                });
            }
        }

        Task.Run(() => ConfigFactory.Save()).Wait();
        return true;
    }

    private static string? EmptyStringToNull(string value)
    {
        return string.IsNullOrEmpty(value) ? null : value;
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
            Log.Error(ex, "Failed to deserialize json file: {FilePath}", filePath);
        }

        return null;
    }
}
