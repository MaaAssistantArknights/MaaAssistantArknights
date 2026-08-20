// <copyright file="Gui.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.ComponentModel;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Services.HotKeys;
using static MaaWpfGui.ViewModels.UI.OverlayViewModel;

namespace MaaWpfGui.Configuration.Global;

public class Gui : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public DarkModeType DarkMode { get; set; } = DarkModeType.SyncWithOs;

    public bool UseNotify { get; set; } = true;

    public bool MainTasksInvertNullFunction { get; set; }

    public string Localization { get; set; } = LocalizationHelper.DefaultLanguage;

    public string OperNameLanguage { get; set; } = "OperNameLanguageMAA";

    public bool UseTray { get; set; } = true;

    public bool MinimizeToTray { get; set; }

    public bool MinimizeOnStartup { get; set; }

    public bool HideCloseButton { get; set; }

    public string WindowTitleSelectShowList { get; set; } = "2 3 4";

    public bool WindowTitleScrollable { get; set; }

    public string LogItemDateFormat { get; set; } = "HH:mm:ss";

    public WindowPlacement? WindowPlacement { get; set; } = null;

    public bool LoadWindowPlacement { get; set; } = true;

    public bool SaveWindowPlacement { get; set; } = true;

    public InverseClearType InverseClearMode { get; set; } = InverseClearType.Clear;

    public TransitionSpeedType TransitionSpeed { get; set; } = TransitionSpeedType.Normal;

    public bool TaskQueueInverseMode { get; set; }

    public bool UseCardLog { get; set; } = true;

    public int MaxNumberOfLogThumbnails { get; set; } = 100;

    public string SoberLanguage { get; set; } = LocalizationHelper.DefaultLanguage;

    public bool Hangover { get; set; }

    public DateTimeOffset LastBuyWineTime { get; set; } = DateTimeOffset.MinValue;

    public string CustomCulture { get; set; } = string.Empty;

    public bool IgnoreBadModulesAndUseSoftwareRendering { get; set; }

    public OverlayTargetInfo OverlayTarget { get; set; } = new();

    /// <summary>
    /// 设置页排列顺序。
    /// </summary>
    public List<SettingKey> SettingOrders { get; set; } = [];

    /// <summary>
    /// 设置页折叠框展开状态。
    /// </summary>
    public ObservableCollection<SettingKey> CollapesStates { get; set; } = [];

    /// <summary>
    /// 全局热键配置。
    /// </summary>
    public Dictionary<MaaHotKeyAction, MaaHotKey?> HotKeys { get; set; } = [];

    public int GuideStep { get; set; } = 0;

    public Background Background { get; set; } = new();

    // ===== 背景设置（莫奈取色） =====
    public bool BackgroundMonetEnabled { get; set; } = false;

    public MonetModeType BackgroundMonetMode { get; set; } = MonetModeType.Auto;

    public string BackgroundMonetCustomColor { get; set; } = "#326CF3";

    /// <summary>
    /// 自动取色模式上次提取到的主色（HEX），用于下次启动时同步恢复调色板，避免闪烁。
    /// 自定义模式的颜色也写入此缓存，使启动时不论何种模式都能即时恢复。
    /// </summary>
    public string BackgroundMonetCachedColor { get; set; } = string.Empty;

    [UsedImplicitly]
    public void OnPropertyChanged(string propertyName, object before, object after)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
    }

    /// <summary>
    /// 表示深色模式的类型。
    /// </summary>
    public enum DarkModeType
    {
        /// <summary>
        /// 与操作系统的深色模式同步。
        /// </summary>
        SyncWithOs = 0,

        /// <summary>
        /// 明亮的主题。
        /// </summary>
        Light,

        /// <summary>
        /// 暗黑的主题。
        /// </summary>
        Dark,
    }

    public enum InverseClearType
    {
        /// <summary>
        /// 清空
        /// </summary>
        Clear = 0,

        /// <summary>
        /// 反转
        /// </summary>
        Inverse,

        /// <summary>
        /// 下拉框，可选清空/反转
        /// </summary>
        ClearInverse,
    }

    /// <summary>
    /// 莫奈取色的模式。
    /// </summary>
    public enum MonetModeType
    {
        /// <summary>
        /// 从背景图自动提取主色。
        /// </summary>
        Auto = 0,

        /// <summary>
        /// 用户手动选择颜色。
        /// </summary>
        Custom,
    }

    /// <summary>
    /// 表示界面过渡动画的速度档位。
    /// </summary>
    public enum TransitionSpeedType
    {
        /// <summary>
        /// 原速。
        /// </summary>
        Normal = 0,

        /// <summary>
        /// 快速。
        /// </summary>
        Fast,

        /// <summary>
        /// 无动画。
        /// </summary>
        None,
    }
}
