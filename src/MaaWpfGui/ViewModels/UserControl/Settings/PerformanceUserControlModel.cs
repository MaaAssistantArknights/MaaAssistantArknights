// <copyright file="PerformanceUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 性能设置
/// </summary>
public class PerformanceUserControlModel : PropertyChangedBase
{
    public static PerformanceUserControlModel Instance { get; } = new();

    public List<GpuOption> GpuOptions => GpuOption.GetGpuOptions();

    public GpuOption ActiveGpuOption
    {
        get => GpuOption.GetCurrent();
        set
        {
            GpuOption.SetCurrent(value);
            SettingsViewModel.AskRestartToApplySettings();
        }
    }

    public bool AllowDeprecatedGpu
    {
        get => GpuOption.AllowDeprecatedGpu;
        set
        {
            GpuOption.AllowDeprecatedGpu = value;
            NotifyOfPropertyChange();
        }
    }

    private string _screencapCost = string.Format(LocalizationHelper.GetString("ScreencapCost"), "---", "---", "---", "---");

    public string ScreencapCost
    {
        get => _screencapCost;
        set => SetAndNotify(ref _screencapCost, value);
    }
}
