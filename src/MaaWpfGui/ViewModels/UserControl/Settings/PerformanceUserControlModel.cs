// <copyright file="PerformanceUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

/// <summary>
/// 性能设置
/// </summary>
public class PerformanceUserControlModel : PropertyChangedBase
{
    static PerformanceUserControlModel()
    {
        Instance = new();
        LocalizationHelper.LanguageChanged += Instance.RefreshGpuOptions;
    }

    public static PerformanceUserControlModel Instance { get; }

    private List<GpuOptionItem>? _gpuOptions;

    public List<GpuOptionItem> GpuOptions => _gpuOptions ??= [.. GpuOption.GetGpuOptions().Select(o => new GpuOptionItem(o))];

    private GpuOptionItem? _activeGpuOption;

    public GpuOptionItem? ActiveGpuOption
    {
        get => _activeGpuOption ??= GpuOptions.FirstOrDefault(x => x.Value.Equals(GpuOption.GetCurrent()));
        set
        {
            if (!SetAndNotify(ref _activeGpuOption, value) || value is null)
            {
                return;
            }

            GpuOption.SetCurrent(value.Value);
            SettingsViewModel.AskRestartToApplySettings();
        }
    }

    /// <summary>
    /// 刷新 GPU 选项的显示文本（语言切换时调用）。
    /// 只更新每个 item 的 Display 字符串，选中项引用保持稳定，避免 ComboBox 选中框刷新问题。
    /// </summary>
    private void RefreshGpuOptions()
    {
        foreach (var item in GpuOptions)
        {
            item.RefreshDisplay();
        }
    }

    public bool AllowDeprecatedGpu
    {
        get => GpuOption.AllowDeprecatedGpu;
        set {
            GpuOption.AllowDeprecatedGpu = value;
            NotifyOfPropertyChange();
        }
    }
}
