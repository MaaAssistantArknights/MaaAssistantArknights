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
using System.IO;
using System.Threading.Tasks;
using MaaWpfGui.Configuration;
using MaaWpfGui.Constants;
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
        private static readonly string _configurationOldFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json.old");
        private static readonly string _configurationFile = Path.Combine(Environment.CurrentDirectory, "config/gui.json");
        private static readonly ILogger _logger = Log.ForContext<ConfigurationHelper>();

        public static bool ConvertConfig()
        {
            if (Directory.Exists("config") is false)
            {
                Directory.CreateDirectory("config");
            }

            // Load configuration file
            var parsed = ParseJsonFile(_configurationFile);
            if (parsed is null)
            {
                return false;
            }

            if (parsed["ConfigVersion"] is null)
            {
                // v4 to v5
                try
                {
                    File.Copy(_configurationFile, _configurationOldFile, true);
                }
                catch (Exception e)
                {
                    _logger.Error(e, "Failed to backup old configuration file.");
                }

                var result = ConvertV4ToV5();

                try
                {
                    File.Delete(_configurationFile);
                }
                catch (Exception e)
                {
                    _logger.Error(e, "Failed to remove " + _configurationFile);
                }

                return result;
            }
            else if (parsed["ConfigVersion"]?.Type == JTokenType.Integer)
            {
                // 暂无意义
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

            var currentConfigName = ConfigurationHelper.GetCurrentConfiguration();

            // VersionUpdate部分
            {
                ConfigFactory.Root.VersionUpdate.Name = ConfigurationHelper.GetValue(ConfigurationKeys.VersionName, string.Empty);
                ConfigFactory.Root.VersionUpdate.Body = ConfigurationHelper.GetValue(ConfigurationKeys.VersionUpdateBody, string.Empty);
                ConfigFactory.Root.VersionUpdate.IsFirstBoot = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.VersionUpdateIsFirstBoot, bool.FalseString));
                ConfigFactory.Root.VersionUpdate.Package = ConfigurationHelper.GetValue(ConfigurationKeys.VersionUpdatePackage, string.Empty);
                ConfigFactory.Root.VersionUpdate.VersionType = Enum.Parse<UpdateVersionType>(ConfigurationHelper.GetValue(ConfigurationKeys.VersionType, UpdateVersionType.Stable.ToString()));

                // 不完全迁移，缺少Proxy更新后监听
                ConfigFactory.Root.VersionUpdate.Proxy = ConfigurationHelper.GetValue(ConfigurationKeys.UpdateProxy, string.Empty);
                ConfigFactory.Root.VersionUpdate.UpdateCheck = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UpdateCheck, bool.TrueString));
                ConfigFactory.Root.VersionUpdate.UpdateAutoCheck = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.UpdateAutoCheck, bool.FalseString));
                ConfigFactory.Root.VersionUpdate.ResourceApi = ConfigurationHelper.GetValue(ConfigurationKeys.ResourceApi, MaaUrls.MaaResourceApi);
                ConfigFactory.Root.VersionUpdate.AutoInstallUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoInstallUpdatePackage, bool.FalseString));
                ConfigFactory.Root.VersionUpdate.AutoDownloadUpdatePackage = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoDownloadUpdatePackage, bool.TrueString));
                ConfigFactory.Root.VersionUpdate.DoNotShowUpdate = Convert.ToBoolean(ConfigurationHelper.GetGlobalValue(ConfigurationKeys.VersionUpdateDoNotShowUpdate, bool.FalseString));
            }

            foreach (var configName in ConfigurationHelper.GetConfigurationList())
            {
                ConfigurationHelper.SwitchConfiguration(configName);
                ConfigFactory.AddConfiguration(configName);

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
                    var fightTask2 = new FightTask();
                    var awardTask = new AwardTask();
                    var mallTask = new MallTask();
                    var infrastTask = new InfrastTask();
                    var recruitTask = new RecruitTask();
                    var roguelikeTask = new RoguelikeTask();
                    var reclamationTask = new ReclamationTask();

                    awardTask.Award = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveAward, bool.TrueString));
                    awardTask.Mail = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMail, bool.FalseString));
                    awardTask.FreeRecruit = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveFreeRecruit, bool.FalseString));
                    awardTask.Orundum = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveOrundum, bool.FalseString));
                    awardTask.Mining = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveMining, bool.FalseString));
                    awardTask.SpecialAccess = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ReceiveSpecialAccess, bool.FalseString));

                    mallTask.Shopping = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CreditShopping, true.ToString()));
                    mallTask.WhiteList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditFirstListNew, LocalizationHelper.GetString("HighPriorityDefault"));
                    mallTask.BlackList = ConfigurationHelper.GetValue(ConfigurationKeys.CreditBlackListNew, LocalizationHelper.GetString("BlacklistDefault"));
                    mallTask.ShoppingWhenCreditFull = bool.Parse(ConfigurationHelper.GetValue(ConfigurationKeys.CreditForceShoppingIfCreditFull, false.ToString()));

                    roguelikeTask.Theme = ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeTheme, "Sami");
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
