// <copyright file="Config.cs" company="MaaAssistantArknights">
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

using System;
using System.IO;
using System.Reflection;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui
{
    /// <summary>
    /// 界面设置存储（读写json文件）
    /// </summary>
    public class Config
    {
        private static readonly string _configFilename = Environment.CurrentDirectory + "\\config\\gui.json";
        private static readonly string _configBakFilename = Environment.CurrentDirectory + "\\config\\gui.json.bak";
        private static JObject _viewStatus = new JObject();

        /// <summary>
        /// Gets the value of a key with default value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="default_value">The default value.</param>
        /// <returns>The value, or <paramref name="default_value"/> if <paramref name="key"/> is not found.</returns>
        public static string Get(string key, string default_value)
        {
            if (_viewStatus.ContainsKey(key))
            {
                return _viewStatus[key].ToString();
            }
            else
            {
                return default_value;
            }
        }

        /// <summary>
        /// Sets a key with a value.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="value">The value.</param>
        public static void Set(string key, string value)
        {
            _viewStatus[key] = value;
            Save();
        }

        /// <summary>
        /// Loads configuration.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Load()
        {
            // 2023-1-13 配置文件迁移
            // FIXME: 之后的版本删了这段
            Directory.CreateDirectory("config");
            if (File.Exists("gui.json"))
            {
                if (File.Exists(_configFilename))
                {
                    File.Delete("gui.json");
                }
                else
                {
                    File.Move("gui.json", _configFilename);
                }
            }

            File.Delete("gui.json.bak");
            File.Delete("gui.json.bak1");
            File.Delete("gui.json.bak2");

            if (File.Exists(_configFilename))
            {
                try
                {
                    string jsonStr = File.ReadAllText(_configFilename);

                    if (jsonStr.Length <= 2 && File.Exists(_configBakFilename))
                    {
                        jsonStr = File.ReadAllText(_configBakFilename);
                        try
                        {
                            File.Copy(_configBakFilename, _configFilename, true);
                        }
                        catch (Exception e)
                        {
                            Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                        }
                    }

                    _viewStatus = (JObject)JsonConvert.DeserializeObject(jsonStr) ?? new JObject();
                }
                catch (Exception e)
                {
                    Logger.Error(e.ToString(), MethodBase.GetCurrentMethod().Name);
                    _viewStatus = new JObject();
                    return false;
                }
            }
            else
            {
                _viewStatus = new JObject();
                return false;
            }

            BakeUpDaily();
            return true;
        }

        /// <summary>
        /// Deletes configuration.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Delete(string key)
        {
            try
            {
                _viewStatus.Remove(key);
                Save();
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        /// <summary>
        /// Saves configuration.
        /// </summary>
        /// <returns>Whether the operation is successful.</returns>
        public static bool Save()
        {
            if (_released)
            {
                return false;
            }

            try
            {
                var jsonStr = _viewStatus.ToString();
                if (jsonStr.Length > 2)
                {
                    using (StreamWriter sw = new StreamWriter(_configFilename))
                    {
                        sw.Write(jsonStr);
                    }

                    if (new FileInfo(_configFilename).Length > 2)
                    {
                        File.Copy(_configFilename, _configBakFilename, true);
                    }
                }
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        private static bool _released = false;

        public static void Release()
        {
            Save();
            _released = true;
        }

        /// <summary>
        /// Backs up configuration daily. (#2145)
        /// </summary>
        /// <param name="num">The number of backup files.</param>
        /// <returns>Whether the operation is successful.</returns>
        public static bool BakeUpDaily(int num = 2)
        {
            if (File.Exists(_configBakFilename) && DateTime.Now.Date != new FileInfo(_configBakFilename).LastWriteTime.Date && num > 0)
            {
                try
                {
                    for (; num > 1; num--)
                    {
                        if (File.Exists(string.Concat(_configBakFilename, num - 1)))
                        {
                            File.Copy(string.Concat(_configBakFilename, num - 1), string.Concat(_configBakFilename, num), true);
                        }
                    }

                    File.Copy(_configBakFilename, string.Concat(_configBakFilename, num), true);
                }
                catch (Exception)
                {
                    return false;
                }
            }

            return true;
        }

        public const string Localization = "GUI.Localization";
        public const string MinimizeToTray = "GUI.MinimizeToTray";
        public const string UseNotify = "GUI.UseNotify";
        public const string MonitorNumber = "GUI.Monitor.Number";
        public const string MonitorWidth = "GUI.Monitor.Width";
        public const string MonitorHeight = "GUI.Monitor.Height";
        public const string PositionLeft = "GUI.Position.Left";
        public const string PositionTop = "GUI.Position.Top";
        public const string WindowWidth = "GUI.Size.Width";
        public const string WindowHeight = "GUI.Size.Height";
        public const string LoadPositionAndSize = "GUI.PositionAndSize.Load";
        public const string SavePositionAndSize = "GUI.PositionAndSize.SaveOnClosing";
        public const string UseAlternateStage = "GUI.UseAlternateStage";
        public const string HideUnavailableStage = "GUI.HideUnavailableStage";
        public const string CustomStageCode = "GUI.CustomStageCode";
        public const string InverseClearMode = "GUI.InverseClearMode";
        public const string WindowTitlePrefix = "GUI.WindowTitlePrefix";
        public const string SetColors = "GUI.SetColors";

        public const string AddressHistory = "Connect.AddressHistory";
        public const string AutoDetect = "Connect.AutoDetect";
        public const string AlwaysAutoDetect = "Connect.AlwaysAutoDetect";
        public const string ConnectAddress = "Connect.Address";
        public const string AdbPath = "Connect.AdbPath";
        public const string ConnectConfig = "Connect.ConnectConfig";
        public const string RetryOnAdbDisconnected = "Connect.RetryOnDisconnected";
        public const string AdbLiteEnabled = "Connect.AdbLiteEnabled";
        public const string TouchMode = "Connect.TouchMode";
        public const string AdbReplaced = "Connect.AdbReplaced";

        public const string ClientType = "Start.ClientType";
        public const string RunDirectly = "Start.RunDirectly";
        public const string StartEmulator = "Start.StartEmulator";
        public const string MinimizingStartup = "Start.MinimizingStartup";
        public const string EmulatorPath = "Start.EmulatorPath";
        public const string EmulatorAddCommand = "Start.EmulatorAddCommand";
        public const string EmulatorWaitSeconds = "Start.EmulatorWaitSeconds";
        public const string AutoRestartOnDrop = "Start.AutoRestartOnDrop";

        public const string ChooseLevel3 = "Recruit.ChooseLevel3";
        public const string ChooseLevel4 = "Recruit.ChooseLevel4";
        public const string ChooseLevel5 = "Recruit.ChooseLevel5";
        public const string ChooseLevel6 = "Recruit.ChooseLevel6";
        public const string AutoSetTime = "Recruit.AutoSetTime";
        public const string Level3UseShortTime = "Recruit.IsLevel3UseShortTime";

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
        public const string RoguelikeInvestsCount = "Roguelike.InvestsCount";
        public const string RoguelikeStopWhenInvestmentFull = "Roguelike.StopWhenInvestmentFull";
        public const string RoguelikeDeploymentWithPause = "Roguelike.DeploymentWithPause";

        public const string LastCreditFightTaskTime = "Visit.LastCreditFightTaskTime";
        public const string CreditFightTaskEnabled = "Visit.CreditFightTaskEnabled";

        public const string RecruitMaxTimes = "AutoRecruit.MaxTimes";
        public const string RefreshLevel3 = "AutoRecruit.RefreshLevel3";
        public const string IsLevel3UseShortTime = "AutoRecruit.IsLevel3UseShortTime";
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
        public const string UseAria2 = "VersionUpdate.UseAria2";
        public const string AutoDownloadUpdatePackage = "VersionUpdate.AutoDownloadUpdatePackage";

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

        public static string GetCheckedStorageKey(string storageKey, string name)
        {
            return storageKey + name + ".IsChecked";
        }

        public static string GetFacilityOrderKey(string facility)
        {
            return "Infrast.Order." + facility;
        }

        public static string GetTimerKey(int i)
        {
            return $"Timer.Timer{i + 1}";
        }

        public static string GetTimerHour(int i)
        {
            return $"Timer.Timer{i + 1}Hour";
        }

        public static string GetTimerMin(int i)
        {
            return $"Timer.Timer{i + 1}Min";
        }

        public static string GetTaskOrderKey(string task)
        {
            return "TaskQueue.Order." + task;
        }
    }
}
