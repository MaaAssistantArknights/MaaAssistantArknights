// <copyright file="TimerSettingsUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Windows.Documents;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 定时设置
/// </summary>
public class TimerSettingsUserControlModel : PropertyChangedBase
{
    static TimerSettingsUserControlModel()
    {
        Instance = new();
    }

    public static TimerSettingsUserControlModel Instance { get; }

    /// <summary>
    /// Gets or sets a value indicating whether to force scheduled start.
    /// </summary>
    public bool ForceScheduledStart
    {
        get; set {
            ConfigFactory.Root.TimerSettings.ForceScheduledStart = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.TimerSettings.ForceScheduledStart;

    /// <summary>
    /// Gets or sets a value indicating whether show window before force scheduled start.
    /// </summary>
    public bool ShowWindowBeforeForceScheduledStart
    {
        get; set {
            ConfigFactory.Root.TimerSettings.ShowWindowBeforeForceScheduledStart = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.TimerSettings.ShowWindowBeforeForceScheduledStart;

    /// <summary>
    /// Gets or sets a value indicating whether to use custom config.
    /// </summary>
    public bool CustomConfig
    {
        get; set {
            ConfigFactory.Root.TimerSettings.CustomConfig = value;
            SetAndNotify(ref field, value);
        }
    } = ConfigFactory.Root.TimerSettings.CustomConfig;

    public ObservableCollection<Timer> TimerList => ConfigFactory.Root.TimerSettings.List;
}
