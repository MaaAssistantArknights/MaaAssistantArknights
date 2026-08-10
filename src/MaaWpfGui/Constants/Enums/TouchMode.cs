// <copyright file="TouchMode.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Constants.Enums;

public enum TouchMode
{
    /// <summary>
    /// MiniTouch 触控模式，适用于大多数设备，默认。
    /// </summary>
    MiniTouch,

    /// <summary>
    /// MaaTouch 触控模式，适用于部分设备，雷电模拟器用可能会有拖动问题。
    /// </summary>
    MaaTouch,

    /// <summary>
    /// Adb 触控模式，适用于部分设备，巨几把慢。
    /// </summary>
    Adb,

    /// <summary>
    /// MaaFwAdb 触控模式。
    /// </summary>
    MaaFwAdb,

    /// <summary>
    /// MuMu external renderer IPC 触控，仅截图增强启用时可选。
    /// </summary>
    MumuExtras,
}
