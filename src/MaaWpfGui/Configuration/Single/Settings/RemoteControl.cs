// <copyright file="RemoteControl.cs" company="MaaAssistantArknights">
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
/// 远程控制设置
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class RemoteControl
{
    public string RemoteControlGetTaskEndpointUri { get; set; } = string.Empty;

    public string RemoteControlReportStatusUri { get; set; } = string.Empty;

    public string RemoteControlUserIdentity { get; set; } = string.Empty;

    public string RemoteControlDeviceIdentity { get; set; } = string.Empty;

    public int RemoteControlPollIntervalMs { get; set; } = 1000;
}
