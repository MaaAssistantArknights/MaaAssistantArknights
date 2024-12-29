// <copyright file="GameSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class GameSettingsUserControlModel : PropertyChangedBase
{
    static GameSettingsUserControlModel()
    {
        Instance = new();
    }

    public static GameSettingsUserControlModel Instance { get; }

    private static readonly ILogger _logger = Log.ForContext<GameSettingsUserControlModel>();

    private static VersionUpdateSettingsUserControlModel VersionUpdateSettings => SettingsViewModel.VersionUpdateSettings;

    /// <summary>
    /// Gets the list of the client types.
    /// </summary>
    public List<CombinedData> ClientTypeList { get; } =
        [
            new() { Display = LocalizationHelper.GetString("NotSelected"), Value = string.Empty },
            new() { Display = LocalizationHelper.GetString("Official"), Value = "Official" },
            new() { Display = LocalizationHelper.GetString("Bilibili"), Value = "Bilibili" },
            new() { Display = LocalizationHelper.GetString("YoStarEN"), Value = "YoStarEN" },
            new() { Display = LocalizationHelper.GetString("YoStarJP"), Value = "YoStarJP" },
            new() { Display = LocalizationHelper.GetString("YoStarKR"), Value = "YoStarKR" },
            new() { Display = LocalizationHelper.GetString("Txwy"), Value = "txwy" },
        ];

    private string _clientType = ConfigurationHelper.GetValue(ConfigurationKeys.ClientType, string.Empty);

    /// <summary>
    /// Gets or sets the client type.
    /// </summary>
    public string ClientType
    {
        get => _clientType;
        set
        {
            SetAndNotify(ref _clientType, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ClientType, value);
            VersionUpdateSettings.ResourceInfo = VersionUpdateSettingsUserControlModel.GetResourceVersionByClientType(_clientType);
            VersionUpdateSettings.ResourceVersion = VersionUpdateSettings.ResourceInfo.VersionName;
            VersionUpdateSettings.ResourceDateTime = VersionUpdateSettings.ResourceInfo.DateTime;
            Instances.SettingsViewModel.UpdateWindowTitle(); // 每次修改客户端时更新WindowTitle
            Instances.TaskQueueViewModel.UpdateStageList();
            Instances.TaskQueueViewModel.UpdateDatePrompt();
            Instances.AsstProxy.LoadResource();
            SettingsViewModel.AskRestartToApplySettings(_clientType is "YoStarEN");
        }
    }

    private bool _deploymentWithPause = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, bool.FalseString));

    public bool DeploymentWithPause
    {
        get => _deploymentWithPause;
        set
        {
            SetAndNotify(ref _deploymentWithPause, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDeploymentWithPause, value.ToString());
            SettingsViewModel.ConnectSettings.UpdateInstanceSettings();
        }
    }

    private bool _autoRestartOnDrop = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.AutoRestartOnDrop, bool.TrueString));

    public bool AutoRestartOnDrop
    {
        get => _autoRestartOnDrop;
        set
        {
            SetAndNotify(ref _autoRestartOnDrop, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.AutoRestartOnDrop, value.ToString());
        }
    }

    private bool _roguelikeDelayAbortUntilCombatComplete = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, bool.FalseString));

    /// <summary>
    /// Gets or sets a value indicating whether delay abort until battle complete
    /// </summary>
    public bool RoguelikeDelayAbortUntilCombatComplete
    {
        get => _roguelikeDelayAbortUntilCombatComplete;
        set
        {
            SetAndNotify(ref _roguelikeDelayAbortUntilCombatComplete, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.RoguelikeDelayAbortUntilCombatComplete, value.ToString());
        }
    }

    private string _startsWithScript = ConfigurationHelper.GetValue(ConfigurationKeys.StartsWithScript, string.Empty);

    public string StartsWithScript
    {
        get => _startsWithScript;
        set
        {
            SetAndNotify(ref _startsWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.StartsWithScript, value);
        }
    }

    private string _endsWithScript = ConfigurationHelper.GetValue(ConfigurationKeys.EndsWithScript, string.Empty);

    public string EndsWithScript
    {
        get => _endsWithScript;
        set
        {
            SetAndNotify(ref _endsWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.EndsWithScript, value);
        }
    }

    private bool _copilotWithScript = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.CopilotWithScript, bool.FalseString));

    public bool CopilotWithScript
    {
        get => _copilotWithScript;
        set
        {
            SetAndNotify(ref _copilotWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.CopilotWithScript, value.ToString());
        }
    }

    private bool _manualStopWithScript = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.ManualStopWithScript, bool.FalseString));

    public bool ManualStopWithScript
    {
        get => _manualStopWithScript;
        set
        {
            SetAndNotify(ref _manualStopWithScript, value);
            ConfigurationHelper.SetValue(ConfigurationKeys.ManualStopWithScript, value.ToString());
        }
    }

    public void RunScript(string str, bool showLog = true)
    {
        bool enable = str switch
        {
            "StartsWithScript" => !string.IsNullOrWhiteSpace(StartsWithScript),
            "EndsWithScript" => !string.IsNullOrWhiteSpace(EndsWithScript),
            _ => false,
        };

        if (!enable)
        {
            return;
        }

        Func<bool> func = str switch
        {
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
            if (string.IsNullOrWhiteSpace(scriptPath))
            {
                return false;
            }

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

            var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
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
        set
        {
            SetAndNotify(ref _blockSleep, value);
            SleepManagement.SetBlockSleep(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.BlockSleep, value.ToString());
        }
    }

    private bool _blockSleepWithScreenOn = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleepWithScreenOn, bool.TrueString));

    public bool BlockSleepWithScreenOn
    {
        get => _blockSleepWithScreenOn;
        set
        {
            SetAndNotify(ref _blockSleepWithScreenOn, value);
            SleepManagement.SetBlockSleepWithScreenOn(value);
            ConfigurationHelper.SetValue(ConfigurationKeys.BlockSleepWithScreenOn, value.ToString());
        }
    }
}
