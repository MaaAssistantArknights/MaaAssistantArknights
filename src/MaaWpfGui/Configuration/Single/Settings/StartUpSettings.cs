// <copyright file="StartUpSettings.cs" company="MaaAssistantArknights">
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
using PropertyChanged;

namespace MaaWpfGui.Configuration.Single.Settings;

/// <summary>
/// 模拟器启动设置
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class StartUpSettings
{
    public bool RunDirectly { get; set; }

    /// <summary>
    /// 更新后「立即重启」时是否跳过「启动后直接运行 / 启动模拟器」。
    /// 默认开启：｢自动安装更新包｣ 或更新提示中选择立即重启时，在该重启链写入 <c>--skip-startup-auto-run</c>。
    /// 选择「稍后」再手动启动不属于该链，仍按正常启动流程执行。
    /// </summary>
    public bool SkipStartupAutoRunAfterUpdate { get; set; } = true;

    public bool StartEmulator { get; set; }

    public bool RestartEmulatorWhenAdbFailed { get; set; }

    public string EmulatorPath { get; set; } = string.Empty;

    public string EmulatorAddCommand { get; set; } = string.Empty;

    public int EmulatorWaitSeconds { get; set; } = 60;

    public string GameExePath { get; set; } = string.Empty;
}
