// <copyright file="SettingKey.cs" company="MaaAssistantArknights">
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
namespace MaaWpfGui.Constants.Enums;

/// <summary>
/// 设置页标识，用作排列顺序和折叠状态的字典 key。
/// </summary>
public enum SettingKey
{
    /// <summary>
    /// 切换配置。
    /// </summary>
    SwitchConfiguration,

    /// <summary>
    /// 定时设置。
    /// </summary>
    ScheduleSettings,

    /// <summary>
    /// 性能设置。
    /// </summary>
    PerformanceSettings,

    /// <summary>
    /// 游戏设置。
    /// </summary>
    GameSettings,

    /// <summary>
    /// 连接设置。
    /// </summary>
    ConnectionSettings,

    /// <summary>
    /// 启动设置。
    /// </summary>
    StartupSettings,

    /// <summary>
    /// 远程控制设置。
    /// </summary>
    RemoteControlSettings,

    /// <summary>
    /// 界面设置。
    /// </summary>
    UiSettings,

    /// <summary>
    /// 背景设置。
    /// </summary>
    BackgroundSettings,

    /// <summary>
    /// 外部通知设置。
    /// </summary>
    ExternalNotificationSettings,

    /// <summary>
    /// 快捷键设置。
    /// </summary>
    HotKeySettings,

    /// <summary>
    /// 成就设置。
    /// </summary>
    AchievementSettings,

    /// <summary>
    /// 软件更新设置。
    /// </summary>
    UpdateSettings,

    /// <summary>
    /// 问题反馈。
    /// </summary>
    IssueReport,

    /// <summary>
    /// 关于。
    /// </summary>
    AboutUs,
}
