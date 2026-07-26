// <copyright file="RuntimeSettings.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using MaaWpfGui.ViewModels.UserControl.Settings;
using PropertyChanged;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// wpf运行时设置
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class RuntimeSettings : IJsonOnDeserialized
{
    public Constants.Enums.ClientType ClientType { get; set; } = Constants.Enums.ClientType.Official;

    public bool StartGame { get; set; } = true;

    public bool DeployWithPause { get; set; }

    public bool RoguelikeDelayAbortUntilCombatComplete { get; set; }

    public string PreRunScript { get; set; } = string.Empty;

    public string PostRunScript { get; set; } = string.Empty;

    public bool ExecuteScriptOnCopilot { get; set; }

    public bool ExecuteScriptOnManualStop { get; set; }

    /// <summary>
    /// 运行时阻止进入睡眠
    /// </summary>
    public bool BlockSleep { get; set; }

    public bool BlockSleepWithScreenOn { get; set; } = true;

    public bool ReportToPenguin { get; set; } = true;

    public string PenguinId { get; set; } = string.Empty;

    public bool ReportToYituliu { get; set; } = true;

    public int BattleStartTimeoutSeconds { get; set; } = GameSettingsUserControlModel.BattleStartTimeoutDefaultSeconds;

    public bool EnableStallTimeout { get; set; } = true;

    public int StallTimeoutReminderIntervalMinutes { get; set; } = 30;

    public int StallTimeoutMinutes { get; set; } = 30;

    public void OnDeserialized()
    {
        BattleStartTimeoutSeconds = Math.Clamp(
            BattleStartTimeoutSeconds,
            GameSettingsUserControlModel.BattleStartTimeoutMinSeconds,
            GameSettingsUserControlModel.BattleStartTimeoutMaxSeconds);
        StallTimeoutMinutes = Math.Clamp(StallTimeoutMinutes, 0, GameSettingsUserControlModel.TimeoutMaxMinutes);
        StallTimeoutReminderIntervalMinutes = Math.Clamp(StallTimeoutReminderIntervalMinutes, 1, GameSettingsUserControlModel.TimeoutMaxMinutes);
    }
}
