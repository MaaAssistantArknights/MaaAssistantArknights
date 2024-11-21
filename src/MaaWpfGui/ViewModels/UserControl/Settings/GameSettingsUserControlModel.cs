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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class GameSettingsUserControlModel : PropertyChangedBase
{
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
}
