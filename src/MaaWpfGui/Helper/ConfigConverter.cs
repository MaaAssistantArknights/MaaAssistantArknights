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
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.Configuration.Single.MaaTask.ReclamationTask;

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

        string[] configKeys = ["Announcement.AnnouncementInfo", "Announcement.DoNotRemindThisAnnouncementAgain", "Announcement.DoNotShowAnnouncement",
            ConfigurationKeys.VersionName, ConfigurationKeys.VersionUpdateBody, ConfigurationKeys.VersionUpdateIsFirstBoot, ConfigurationKeys.VersionUpdatePackage, ConfigurationKeys.VersionUpdateDoNotShowUpdate,
        ];
        var currentConfigName = ConfigurationHelper.GetCurrentConfiguration();
        foreach (var configName in ConfigurationHelper.GetConfigurationList())
        {
            ConfigurationHelper.SwitchConfiguration(configName);
            ConfigFactory.AddConfiguration(configName);
            ConfigFactory.SwitchConfig(configName);

            // 删除旧的配置
            foreach (var key in configKeys)
            {
                ConfigurationHelper.DeleteValue(key);
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

            // TaskQueue部分
            {
                var startUpTask = new StartUpTask();
                var fightTask = new FightTask();
                var fightTask2 = new FightTask(); // 剩余理智
                var awardTask = new AwardTask();
                var mallTask = new MallTask();
                var infrastTask = new InfrastTask();
                var recruitTask = new RecruitTask();
                var roguelikeTask = new RoguelikeTask();
                var reclamationTask = new ReclamationTask();

                startUpTask.AccountName = ConfigurationHelper.GetValue(ConfigurationKeys.AccountName, string.Empty);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.AccountName);

                awardTask.Award = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, bool.TrueString));
                awardTask.Mail = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, bool.FalseString));
                awardTask.FreeGacha = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeGacha, bool.FalseString));
                awardTask.Orundum = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, bool.FalseString));
                awardTask.Mining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, bool.FalseString));
                awardTask.SpecialAccess = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, bool.FalseString));
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveAward);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveMail);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveFreeGacha);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveOrundum);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveMining);
                ConfigurationHelper.DeleteValue(ConfigurationKeys.ReceiveSpecialAccess);

                mallTask.Shopping = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, bool.TrueString));
                mallTask.FirstList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault"));
                mallTask.BlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault"));
                mallTask.ShoppingIgnoreBlackListWhenFull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, bool.FalseString));
                mallTask.OnlyBuyDiscount = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditOnlyBuyDiscount, bool.FalseString));
                mallTask.ReserveMaxCredit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditReserveMaxCredit, bool.FalseString));
                mallTask.CreditFight = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightTaskEnabled, bool.FalseString));
                mallTask.CreditFightFormation = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.CreditFightSelectFormation, "0"));
                mallTask.CreditFightLastTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditFightTaskTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());
                mallTask.VisitFriends = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitFriendsEnabled, bool.TrueString));
                mallTask.VisitFriendsOnceADay = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CreditVisitOnceADay, bool.FalseString));
                mallTask.VisitFriendsLastTime = ConfigurationHelper.GetValue(ConfigurationKeys.LastCreditVisitFriendsTime, DateTime.UtcNow.ToYjDate().AddDays(-1).ToFormattedString());

                roguelikeTask.Theme = Enum.Parse<RoguelikeTheme>(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeTheme, RoguelikeTheme.Sarkaz.ToString()));
                roguelikeTask.Difficulty = Convert.ToInt32(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDifficulty, "MAX").Replace("MAX", int.MaxValue.ToString()));
                roguelikeTask.Mode = Enum.Parse<RoguelikeMode>(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeMode, RoguelikeMode.Exp.ToString()));
                roguelikeTask.CoreChar = EmptyStringToNull(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeCoreChar, string.Empty));
                roguelikeTask.Squad = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeSquad, string.Empty);
                roguelikeTask.Roles = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeRoles, string.Empty);
                roguelikeTask.StartCount = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartsCount, 9999999);
                roguelikeTask.Investment = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnabled, bool.TrueString));
                roguelikeTask.InvestCount = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestsCount, 9999999);
                roguelikeTask.InvestWithMoreScore = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeInvestmentEnterSecondFloor, bool.FalseString));
                roguelikeTask.StopWhenDepositFull = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopWhenInvestmentFull, bool.FalseString));
                roguelikeTask.StopAtFinalBoss = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStopAtFinalBoss, bool.FalseString));
                roguelikeTask.UseSupport = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeUseSupportUnit, bool.FalseString));
                roguelikeTask.UseSupportNonFriend = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeEnableNonfriendSupport, bool.FalseString));
                roguelikeTask.RefreshTraderWithDice = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeRefreshTraderWithDice, bool.FalseString));
                roguelikeTask.StartWithEliteTwo = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeStartWithEliteTwo, bool.FalseString));
                roguelikeTask.StartWithEliteTwoOnly = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeOnlyStartWithEliteTwo, bool.FalseString));
                roguelikeTask.SamiFirstFloorFoldartal = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3FirstFloorFoldartal, bool.FalseString));
                roguelikeTask.SamiFirstFloorFoldartals = EmptyStringToNull(ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3FirstFloorFoldartals, string.Empty));
                roguelikeTask.SamiNewSquad2StartingFoldartal = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartal, bool.FalseString));
                roguelikeTask.SamiNewSquad2StartingFoldartals = EmptyStringToNull(ConfigurationHelper.GetValue(ConfigurationKeys.Roguelike3NewSquad2StartingFoldartals, string.Empty));

                reclamationTask.Theme = Enum.Parse<ReclamationTheme>(ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationTheme, ReclamationTheme.Tales.ToString()));
                reclamationTask.Mode = Enum.Parse<ReclamationMode>(ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMode, ReclamationMode.Archive.ToString()));
                reclamationTask.ToolToCraft = EmptyStringToNull(ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationToolToCraft, string.Empty));
                reclamationTask.IncrementMode = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationIncrementMode, 0);
                reclamationTask.MaxCraftCountPerRound = ConfigurationHelper.GetValue(ConfigurationKeys.ReclamationMaxCraftCountPerRound, 16);

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
                var local = GuiSettingsUserControlModel.Instance.Language;
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
                            fightTask2.IsEnable = task.IsEnable;
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
