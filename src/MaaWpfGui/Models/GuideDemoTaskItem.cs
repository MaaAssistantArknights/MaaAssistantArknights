// <copyright file="GuideDemoTaskItem.cs" company="MaaAssistantArknights">
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

using System;
using MaaWpfGui.Helper;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;

namespace MaaWpfGui.Models;

/// <summary>
/// 设置指引 ｢任务设置｣ 步互动演示用的任务项，纯内存演示数据，不与配置的任务队列持久化交互。
/// </summary>
public class GuideDemoTaskItem : PropertyChangedBase, IDisposable
{
    private string _customName = string.Empty;

    private bool _isChecked;

    private bool _enableSetting;

    public GuideDemoTaskItem()
    {
        PropertyDependsOnUtility.InitializePropertyDependencies(this);
    }

    /// <summary>
    /// Gets the localization key of the task type（如 Fight）；重命名不改变本值（名称走自定义文本），复制产生的新项不携带本值（为空串）。
    /// </summary>
    public string LocalizationKey { get; init; } = string.Empty;

    /// <summary>
    /// Gets the display name：重命名/复制产生的自定义名，否则按 <see cref="LocalizationKey"/> 实时取本地化名；
    /// 语言切换经 <see cref="PropertyDependsOnAttribute"/> 依赖语言设置属性自动重新求值。
    /// </summary>
    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    public string Name
    {
        get => _customName.Length > 0
            ? _customName
            : (LocalizationKey.Length > 0 ? LocalizationHelper.GetString(LocalizationKey) : string.Empty);
        set => SetAndNotify(ref _customName, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the demo task is checked.
    /// </summary>
    public bool IsChecked
    {
        get => _isChecked;
        set => SetAndNotify(ref _isChecked, value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether the demo task's gear button is toggled on.
    /// </summary>
    public bool EnableSetting
    {
        get => _enableSetting;
        set => SetAndNotify(ref _enableSetting, value);
    }

    /// <summary>
    /// 清理依赖注册，避免被静态依赖表强引用持有（内存泄漏与语言切换时的僵尸通知）。
    /// </summary>
    void IDisposable.Dispose()
    {
        PropertyDependsOnUtility.UnInitializePropertyDependencies(this);
        GC.SuppressFinalize(this);
    }
}
