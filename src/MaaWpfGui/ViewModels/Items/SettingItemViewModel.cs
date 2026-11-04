// <copyright file="SettingItemViewModel.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License v3.0 only as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Utilities;
using Stylet;

namespace MaaWpfGui.ViewModels.Items;

/// <summary>
/// The view model of setting item.
/// </summary>
public class SettingItemViewModel : PropertyChangedBase
{
    public SettingItemViewModel(string key, string display, int value)
    {
        _key = key;
        _display = display;
        _value = value;
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    private string _key;

    public string Key
    {
        get => _key;
        set => SetAndNotify(ref _key, value);
    }

    private string _display;

    public string Display
    {
        get => _display;
        set => SetAndNotify(ref _display, value);
    }

    private int _value;

    public int Value
    {
        get => _value;
        set => SetAndNotify(ref _value, value);
    }

    [PropertyDependsOn(nameof(Key))]
    public bool IsAdvanced => Key is nameof(SettingKey.PerformanceSettings)
        or nameof(SettingKey.RemoteControlSettings)
        or nameof(SettingKey.ThirdPartyServiceSettings)
        or nameof(SettingKey.HotKeySettings)
        or nameof(SettingKey.AchievementSettings);

    // 高级分区前预留一行放置分割线，Value 仍与导航索引一一对应。
    [PropertyDependsOn(nameof(Value), nameof(IsAdvanced))]
    public int GridRow => Value + (IsAdvanced ? 1 : 0);

    private bool _areAdvancedSettingsVisible;

    public bool AreAdvancedSettingsVisible
    {
        get => _areAdvancedSettingsVisible;
        set => SetAndNotify(ref _areAdvancedSettingsVisible, value);
    }

    [PropertyDependsOn(nameof(IsVisibleInSearch), nameof(IsAdvanced), nameof(AreAdvancedSettingsVisible))]
    public bool IsVisible => IsVisibleInSearch && (!IsAdvanced || AreAdvancedSettingsVisible);

    private bool _isVisibleInSearch = true;

    public bool IsVisibleInSearch
    {
        get => _isVisibleInSearch;
        set => SetAndNotify(ref _isVisibleInSearch, value);
    }
}
