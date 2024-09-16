// <copyright file="ConfigurationConverter.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Configuration;
using MaaWpfGui.Configuration.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Extensions;
using MaaWpfGui.Models;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;
using static MaaWpfGui.Configuration.GUI;
using static MaaWpfGui.Configuration.VersionUpdate;
using static MaaWpfGui.Models.MaaTask;
using static MaaWpfGui.Models.PostActionSetting;

namespace MaaWpfGui.Helper
{
    public class ConfigurationConverter
    {
        private static readonly string _configurationNewFile = Path.Combine(Environment.CurrentDirectory, "config/gui_v5.json");
        private static readonly string _configurationOldBakFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json.old");
        private static readonly string _configurationOldFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json");
        private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

        public static bool ConvertConfig()
        {
            if (Directory.Exists("config") is false)
            {
                Directory.CreateDirectory("config");
            }

            // Load configuration file
            var parsedOld = ParseJsonFile(_configurationOldFile);
            if (parsedOld is null)
            {
                return false;
            }

            var parsedNew = ParseJsonFile(_configurationNewFile);
            parsedNew?.Remove("ConfigVersion");

            if (parsedNew?.TryGetValue("ConfigVersion", out JToken? configVersion) is not true || configVersion.Type is not JTokenType.Integer)
            {
                // v4 to v5
                try
                {
                    File.Copy(_configurationOldFile, _configurationOldBakFile, true);
                }
                catch (Exception e)
                {
                    _logger.Error(e, "Failed to backup old configuration file.");
                }

                var result = ConvertV4ToV5();

                try
                {
                    File.Delete(_configurationOldFile);
                }
                catch (Exception e)
                {
                    _logger.Error(e, "Failed to remove " + _configurationOldFile);
                }

                return result;
            }
            else if (configVersion.Type is JTokenType.Integer)
            {
                // TODO 后续需要移除，暂时兼容一下未迁移的选项
                ConfigurationHelper.Load();
                return true;
            }
            else
            {
                return false;
            }
        }

        private static bool ConvertV4ToV5()
        {
            ConfigurationHelper.Load();

            // VersionUpdate部分
            {
                ConfigFactory.Root.VersionUpdate.Name = ConfigurationHelper.GetValue(ConfigurationKeys.VersionName, string.Empty);
                ConfigFactory.Root.VersionUpdate.Body = ConfigurationHelper.GetValue(ConfigurationKeys.VersionUpdateBody, string.Empty);
                ConfigFactory.Root.VersionUpdate.IsFirstBoot = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.VersionUpdateIsFirstBoot, bool.FalseString));
                ConfigFactory.Root.VersionUpdate.Package = ConfigurationHelper.GetValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
                ConfigFactory.Root.VersionUpdate.VersionType = Enum.Parse<UpdateVersionType>(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionType, UpdateVersionType.Stable.ToString()));

                // 不完全迁移，缺少Proxy更新后监听
                ConfigFactory.Root.VersionUpdate.Proxy = ConfigurationHelper.GetValue(ConfigurationKeys.UpdateProxy, string.Empty);
                ConfigFactory.Root.VersionUpdate.UpdateCheck = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UpdateCheck, bool.TrueString));
                ConfigFactory.Root.VersionUpdate.UpdateAutoCheck = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UpdateAutoCheck, bool.FalseString));
                ConfigFactory.Root.VersionUpdate.ResourceApi = ConfigurationHelper.GetValue(ConfigurationKeys.ResourceApi, MaaUrls.MaaResourceApi);
                ConfigFactory.Root.VersionUpdate.AutoInstallUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoInstallUpdatePackage, bool.FalseString));
                ConfigFactory.Root.VersionUpdate.AutoDownloadUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoDownloadUpdatePackage, bool.TrueString));
                ConfigFactory.Root.VersionUpdate.DoNotShowUpdate = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdateDoNotShowUpdate, bool.FalseString));
            }

            var currentConfigName = ConfigurationHelper.GetCurrentConfiguration();
            foreach (var configName in ConfigurationHelper.GetConfigurationList())
            {
                ConfigurationHelper.SwitchConfiguration(configName);
                ConfigFactory.AddConfiguration(configName);
                ConfigFactory.SwitchConfig(configName);

                // Connect
                {
                    ConfigFactory.CurrentConfig.Connection.AutoDetect = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoDetect, bool.TrueString));
                    ConfigFactory.CurrentConfig.Connection.AlwaysAutoDetect = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AlwaysAutoDetect, bool.FalseString));
                    ConfigFactory.CurrentConfig.Connection.AdbPath = ConfigurationHelper.GetValue(ConfigurationKeys.AdbPath, string.Empty);
                    ConfigFactory.CurrentConfig.Connection.AdbAddress = ConfigurationHelper.GetValue(ConfigurationKeys.ConnectAddress, string.Empty);

                    ConfigFactory.CurrentConfig.Connection.RetryOnAdbDisconnected = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RetryOnAdbDisconnected, bool.FalseString));
                    ConfigFactory.CurrentConfig.Connection.AllAdbRestart = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AllowAdbRestart, bool.TrueString));
                    ConfigFactory.CurrentConfig.Connection.AllAdbHardRestart = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AllowAdbHardRestart, bool.TrueString));
                    ConfigFactory.CurrentConfig.Connection.AdbLiteEnable = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AdbLiteEnabled, bool.FalseString));
                    ConfigFactory.CurrentConfig.Connection.KillAdbWhenExit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.KillAdbOnExit, bool.FalseString));
                }

                // GUI部分
                {
                    ConfigFactory.CurrentConfig.GUI.Localization = ConfigurationHelper.GetValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);
                    ConfigFactory.CurrentConfig.GUI.LoadWindowPlacement = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.LoadWindowPlacement, bool.TrueString));
                    ConfigFactory.CurrentConfig.GUI.SaveWindowPlacement = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.SaveWindowPlacement, bool.TrueString));
                    ConfigFactory.CurrentConfig.GUI.MinimizeToTray = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.MinimizeToTray, bool.FalseString));
                    ConfigFactory.CurrentConfig.GUI.MinimizeDirectly = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.MinimizeDirectly, bool.FalseString));

                    ConfigFactory.CurrentConfig.GUI.PostActions = JsonConvert.DeserializeObject<PostActions>(ConfigurationHelper.GetValue(ConfigurationKeys.PostActions, "0"));

                    if (Enum.TryParse<InverseClearType>(ConfigurationHelper.GetValue(ConfigurationKeys.InverseClearMode, "Clear"), true, out var inverseClearModeResult))
                    {
                        ConfigFactory.CurrentConfig.GUI.InverseClearMode = inverseClearModeResult;
                    }

                    if (!bool.TryParse(ConfigurationHelper.GetValue(ConfigurationKeys.MainFunctionInverseMode, bool.FalseString), out var result))
                    {
                    }
                    else
                    {
                        ConfigFactory.CurrentConfig.GUI.InverseClearShow = result ? InverseClearType.Inverse : InverseClearType.Clear;
                    }

                    ConfigFactory.CurrentConfig.GUI.HideCloseButton = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.HideCloseButton, bool.FalseString));
                    ConfigFactory.CurrentConfig.GUI.WindowTitlePrefix = ConfigurationHelper.GetValue(ConfigurationKeys.WindowTitlePrefix, string.Empty);
                    if (Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UseLogItemDateFormat, bool.FalseString)))
                    {
                        ConfigFactory.CurrentConfig.GUI.LogItemDateFormat = ConfigurationHelper.GetValue(ConfigurationKeys.LogItemDateFormat, "HH:mm:ss");
                    }
                    else
                    {
                        ConfigFactory.CurrentConfig.GUI.LogItemDateFormat = "HH:mm:ss";
                    }

                    var jsonStr = ConfigurationHelper.GetValue(ConfigurationKeys.WindowPlacement, string.Empty);
                    if (!string.IsNullOrEmpty(jsonStr))
                    {
                        try
                        {
                            ConfigFactory.CurrentConfig.GUI.WindowPlacement = JsonConvert.DeserializeObject<WindowPlacement?>(jsonStr) ?? throw new Exception("Failed to parse json string");
                        }
                        catch (Exception e)
                        {
                            _logger.Error(e, "ConfigurationConverter | Failed to deserialize json string from {Key}", ConfigurationKeys.WindowPlacement);
                        }
                    }
                }

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
                    if (startUpTask.AccountName == string.Empty)
                    {
                        startUpTask.AccountName = null;
                    }

                    awardTask.Award = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, bool.TrueString));
                    awardTask.Mail = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, bool.FalseString));
                    awardTask.FreeGacha = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeRecruit, bool.FalseString));
                    awardTask.Orundum = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, bool.FalseString));
                    awardTask.Mining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, bool.FalseString));
                    awardTask.SpecialAccess = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, bool.FalseString));

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

                    roguelikeTask.Theme = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeTheme, "Sami");

                    // 任务导入排序
                    List<(string OldName, int Index, bool IsEnable)> taskList = [("WakeUp", 0, true), ("Recruiting", 1, true), ("Base", 2, true), ("Combat", 3, true), ("Mall", 4, true), ("Mission", 5, true), ("AutoRoguelike", 6, false)];
                    if (ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty) is not "txwy")
                    {
                        taskList.Add(("Reclamation", 7, false));
                    }

                    for (int i = 0; i != taskList.Count; ++i)
                    {
                        var isEnable = Convert.ToBoolean(ConfigurationHelper.GetValue($"TaskQueue.{taskList[i].OldName}.IsChecked", bool.FalseString));
                        if (int.TryParse(ConfigurationHelper.GetTaskOrder(taskList[i].OldName, "-1"), out var order))
                        {
                            taskList[i] = (taskList[i].OldName, order, isEnable);
                        }
                        else
                        {
                            taskList[i] = (taskList[i].OldName, taskList[i].Index, isEnable);
                        }
                    }

                    ConfigFactory.CurrentConfig.TaskQueue.Clear();
                    var local = ConfigFactory.CurrentConfig.GUI.Localization;
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
    }
}
