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
using System.Collections.Generic;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.States;
using MaaWpfGui.Utilities;
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
    }

    public static GameSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<GameSettingsUserControlModel>();

    private static VersionUpdateSettingsUserControlModel VersionUpdateSettings => SettingsViewModel.VersionUpdateSettings;

    public bool StartGame
    {
        get => SettingsViewModel.ConnectSettings.UseAttachWindow ? false : field;
        set {
            SetAndNotify(ref field, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.StartGame, value.ToString());
        }
    } = ConfigurationHelper.GetValue(ConfigurationKeys.StartGame, true);

    /// <summary>
    /// Gets the list of the client types.
    /// </summary>
    public List<CombinedData> ClientTypeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("Official"), Value = "Official" },
            new() { Display = LocalizationHelper.GetString("Bilibili"), Value = "Bilibili" },
            new() { Display = LocalizationHelper.GetString("YoStarEN"), Value = "YoStarEN" },
            new() { Display = LocalizationHelper.GetString("YoStarJP"), Value = "YoStarJP" },
            new() { Display = LocalizationHelper.GetString("YoStarKR"), Value = "YoStarKR" },
            new() { Display = LocalizationHelper.GetString("Txwy"), Value = "txwy" },
        ];

    private string _clientType = ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, "Official");

    /// <summary>
    /// Gets or sets the client type.
    /// </summary>
    public string ClientType
    {
        get { // v5.19.0-beta.1
            if (!string.IsNullOrEmpty(_clientType))
            {
                return _clientType;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.ClientType, "Official");
            return "Official";
        }

        set {
            var oldValue = _clientType;
            if (!SetAndNotify(ref _clientType, value))
            {
                return;
            }

            ConfigurationHelper.SetValue(ConfigurationKeys.ClientType, value);
            VersionUpdateSettings.ResourceInfoUpdate();
            FightSettingsUserControlModel.Instance.UpdateStageList();
            Instances.TaskQueueViewModel.UpdateDatePrompt();

            if (!NeedRestartAfterClientTypeChange(oldValue, value))
            {
                return;
            }

            Task.Run(() => {
                Instances.AsstProxy.LoadResource();
            });

            SettingsViewModel.AskRestartToApplySettings(value is "YoStarEN");
        }
    }

    private static bool NeedRestartAfterClientTypeChange(string oldType, string newType)
    {
        if (string.IsNullOrEmpty(oldType) || oldType == newType)
        {
            return false;
        }

        // 官服 <-> B服 之间切换不需要重启
        return (oldType != "Official" || newType != "Bilibili") &&
               (oldType != "Bilibili" || newType != "Official");
    }

    private bool _deploymentWithPause = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, bool.FalseString));

    public bool DeploymentWithPause
    {
        get => _deploymentWithPause;
        set {
            SetAndNotify(ref _deploymentWithPause, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, value.ToString());
            SettingsViewModel.ConnectSettings.UpdateInstanceSettings();
        }
    }

    private string _startsWithScript = ConfigurationHelper.GetValue(ConfigurationKeys.StartsWithScript, string.Empty);

    public string StartsWithScript
    {
        get => _startsWithScript;
        set {
            SetAndNotify(ref _startsWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.StartsWithScript, value);
        }
    }

    private string _endsWithScript = ConfigurationHelper.GetValue(ConfigurationKeys.EndsWithScript, string.Empty);

    public string EndsWithScript
    {
        get => _endsWithScript;
        set {
            SetAndNotify(ref _endsWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EndsWithScript, value);
        }
    }

    private bool _copilotWithScript = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CopilotWithScript, bool.FalseString));

    public bool CopilotWithScript
    {
        get => _copilotWithScript;
        set {
            SetAndNotify(ref _copilotWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotWithScript, value.ToString());
        }
    }

    private bool _manualStopWithScript = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ManualStopWithScript, bool.FalseString));

    public bool ManualStopWithScript
    {
        get => _manualStopWithScript;
        set {
            SetAndNotify(ref _manualStopWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ManualStopWithScript, value.ToString());
        }
    }

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

    private bool _blockSleep = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleep, bool.FalseString));

    public bool BlockSleep
    {
        get => _blockSleep;
        set {
            SetAndNotify(ref _blockSleep, value);
            SleepManagement.SetBlockSleep(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.BlockSleep, value.ToString());
        }
    }

    private bool _blockSleepWithScreenOn = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleepWithScreenOn, bool.TrueString));

    public bool BlockSleepWithScreenOn
    {
        get => _blockSleepWithScreenOn;
        set {
            SetAndNotify(ref _blockSleepWithScreenOn, value);
            SleepManagement.SetBlockSleepWithScreenOn(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.BlockSleepWithScreenOn, value.ToString());
        }
    }

    #region 企鹅和一图流上报

    private string _penguinId = ConfigurationHelper.GetValue(ConfigurationKeys.PenguinId, string.Empty);

    /// <summary>
    /// Gets or sets the id of PenguinStats.
    /// </summary>
    public string PenguinId
    {
        get => _penguinId;
        set {
            SetAndNotify(ref _penguinId, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.PenguinId, value);
        }
    }

    private bool _enablePenguin = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.EnablePenguin, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to enable penguin upload.
    /// </summary>
    public bool EnablePenguin
    {
        get => _enablePenguin;
        set {
            SetAndNotify(ref _enablePenguin, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EnablePenguin, value.ToString());
        }
    }

    private bool _enableYituliu = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.EnableYituliu, bool.TrueString));

    /// <summary>
    /// Gets or sets a value indicating whether to enable yituliu upload.
    /// </summary>
    public bool EnableYituliu
    {
        get => _enableYituliu;
        set {
            SetAndNotify(ref _enableYituliu, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EnableYituliu, value.ToString());
        }
    }

    #endregion 企鹅和一图流上报

    #region 任务超时

    private int _taskTimeoutMinutes = ConfigurationHelper.GetValue(ConfigurationKeys.TaskTimeoutMinutes, 60);

    public int TaskTimeoutMinutes
    {
        get => _taskTimeoutMinutes;
        set {
            SetAndNotify(ref _taskTimeoutMinutes, value);
            _runningState.TaskTimeoutMinutes = value;
            ConfigurationHelper.SetValue(ConfigurationKeys.TaskTimeoutMinutes, value.ToString());
        }
    }

    private int _reminderIntervalMinutes = ConfigurationHelper.GetValue(ConfigurationKeys.ReminderIntervalMinutes, 30);

    public int ReminderIntervalMinutes
    {
        get => _reminderIntervalMinutes;
        set {
            SetAndNotify(ref _reminderIntervalMinutes, value);
            _runningState.ReminderIntervalMinutes = value;
            ConfigurationHelper.SetValue(ConfigurationKeys.ReminderIntervalMinutes, value.ToString());
        }
    }

    #endregion 任务超时
}
