// <copyright file="Performance.cs" company="MaaAssistantArknights">
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
/// 性能设置
/// </summary>
[AddINotifyPropertyChangedInterface]
public partial class Performance
{
    public bool UseGpu { get; set; }

    public string GpuDescription { get; set; } = string.Empty;

    public string GpuInstancePath { get; set; } = string.Empty;

    public bool AllowDeprecatedGpu { get; set; }
}
