// <copyright file="GameSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class GameSettingsUserControlModel : PropertyChangedBase
{
    private static RunningState _runningState => RunningState.Instance;

    static GameSettingsUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshLocalization;
    }

    public static GameSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<GameSettingsUserControlModel>();

    private static VersionUpdateSettingsUserControlModel VersionUpdateSettings => SettingsViewModel.VersionUpdateSettings;

    public bool StartGame
    {
        get => SettingsViewModel.ConnectSettings.IsPCConnectConfig ? false : field;
        set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StartGame = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StartGame;

    /// <summary>
    /// Gets the list of the client types.
    /// </summary>
    public LocalizedObservableList<ClientType> ClientTypeList { get; } = new(
        (ClientType.Official, "Official"),
        (ClientType.Bilibili, "Bilibili"),
        (ClientType.EN, "YoStarEN"),
        (ClientType.JP, "YoStarJP"),
        (ClientType.KR, "YoStarKR"),
        (ClientType.Txwy, "Txwy"));

    /// <summary>
    /// Gets or sets the client type.
    /// </summary>
    public ClientType ClientType
    {
        get; set {
            var oldValue = field;
            if (!SetAndNotify(ref field, value))
            {
                return;
            }

            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ClientType = value;
            VersionUpdateSettings.ResourceInfoUpdate();
            FightSettingsUserControlModel.Instance.UpdateStageList();
            Instances.TaskQueueViewModel.UpdateDatePrompt();

            if (!NeedRestartAfterClientTypeChange(oldValue, value))
            {
                return;
            }

            Task.Run(() => { Instances.AsstProxy.LoadResource(); });

            SettingsViewModel.AskRestartToApplySettings(value is ClientType.EN);
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ClientType;

    private static bool NeedRestartAfterClientTypeChange(ClientType oldType, ClientType newType)
    {
        if (oldType == newType)
        {
            return false;
        }

        // 官服 <-> B服 之间切换不需要重启
        return (oldType != ClientType.Official || newType != ClientType.Bilibili) &&
               (oldType != ClientType.Bilibili || newType != ClientType.Official);
    }

    public bool DeploymentWithPause
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.DeployWithPause = value;
            SettingsViewModel.ConnectSettings.UpdateInstanceSettings();
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.DeployWithPause;

    public string StartsWithScript
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PreRunScript = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PreRunScript;

    public string EndsWithScript
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PostRunScript = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PostRunScript;

    public bool CopilotWithScript
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ExecuteScriptOnCopilot = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ExecuteScriptOnCopilot;

    public bool ManualStopWithScript
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ExecuteScriptOnManualStop = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ExecuteScriptOnManualStop;

    public void RunScript(string str, bool showLog = true)
    {
        bool enable = str switch {
            "StartsWithScript" => !string.IsNullOrWhiteSpace(StartsWithScript),
            "EndsWithScript" => !string.IsNullOrWhiteSpace(EndsWithScript),
            _ => false,
        };

        if (!enable)
        {
            return;
        }

        Func<bool> func = str switch {
            "StartsWithScript" => () => ExecuteScript(StartsWithScript),
            "EndsWithScript" => () => ExecuteScript(EndsWithScript),
            _ => () => false,
        };

        if (!showLog)
        {
            if (!func())
            {
                _logger.Warning("Failed to execute the script.");
            }

            return;
        }

        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("StartTask") + LocalizationHelper.GetString(str));
        if (func())
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CompleteTask") + LocalizationHelper.GetString(str));
        }
        else
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("TaskError") + LocalizationHelper.GetString(str), UiLogColor.Warning);
        }
    }

    private static bool ExecuteScript(string scriptPath)
    {
        try
        {
            scriptPath = scriptPath.Trim();

            if (string.IsNullOrWhiteSpace(scriptPath))
            {
                return false;
            }

            scriptPath = Regex.Replace(scriptPath, @"\p{C}", string.Empty);

            string fileName;
            string arguments;

            if (scriptPath.StartsWith('\"'))
            {
                var parts = scriptPath.Split("\"", 3);
                fileName = parts[1];
                arguments = parts.Length > 2 ? parts[2] : string.Empty;
            }
            else
            {
                fileName = scriptPath;
                arguments = string.Empty;
            }

            bool createNoWindow = arguments.Contains("-noWindow");
            bool minimized = arguments.Contains("-minimized");

            if (createNoWindow)
            {
                arguments = arguments.Replace("-noWindow", string.Empty).Trim();
            }

            if (minimized)
            {
                arguments = arguments.Replace("-minimized", string.Empty).Trim();
            }

            var process = new Process {
                StartInfo = new ProcessStartInfo {
                    FileName = fileName,
                    Arguments = arguments,
                    WindowStyle = minimized ? ProcessWindowStyle.Minimized : ProcessWindowStyle.Normal,
                    CreateNoWindow = createNoWindow,
                    UseShellExecute = !createNoWindow,
                },
            };
            process.Start();
            process.WaitForExit();
            return true;
        }
        catch (Exception)
        {
            return false;
        }
    }

    public bool BlockSleep
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleep = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleep;

    public bool BlockSleepWithScreenOn
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleepWithScreenOn = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleepWithScreenOn;

    #region 企鹅和一图流上报

    /// <summary>
    /// Gets or sets the id of PenguinStats.
    /// </summary>
    public string PenguinId
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PenguinId = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.PenguinId;

    /// <summary>
    /// Gets or sets a value indicating whether to enable penguin upload.
    /// </summary>
    public bool EnablePenguin
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ReportToPenguin = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ReportToPenguin;

    /// <summary>
    /// Gets or sets a value indicating whether to enable yituliu upload.
    /// </summary>
    public bool EnableYituliu
    {
        get;
        set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ReportToYituliu = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.ReportToYituliu;

    #endregion 企鹅和一图流上报

    #region 任务超时

    /// <summary>
    /// Gets or sets a value indicating whether是否启用停滞检测
    /// </summary>
    public bool EnableStallTimeout
    {
        get; set {
            SetAndNotify(ref field, value);
            _runningState.EnableStallTimeout = value;
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.EnableStallTimeout = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.EnableStallTimeout;

    public int StallTimeoutMinutes
    {
        get; set {
            value = value.Clamp(0, TimeoutMaxMinutes);
            SetAndNotify(ref field, value);
            _runningState.StallTimeoutMinutes = value;
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutMinutes = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutMinutes;

    // 防止乘以 60000 毫秒时 int 溢出，int.MaxValue / 60000 ≈ 35791
    public const int TimeoutMaxMinutes = 11451;

    public int ReminderIntervalMinutes
    {
        get; set {
            value = value.Clamp(1, TimeoutMaxMinutes);
            SetAndNotify(ref field, value);
            _runningState.ReminderIntervalMinutes = value;
            ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutReminderIntervalMinutes = value;
        }
    } = ConfigFactory.CurrentConfig.Gui.RuntimeSettings.StallTimeoutReminderIntervalMinutes;

    #endregion 任务超时

    /// <summary>
    /// 刷新构造时缓存的本地化列表文本。
    /// </summary>
    private void RefreshLocalization()
    {
        ClientTypeList.RefreshLocalization();
    }
}
