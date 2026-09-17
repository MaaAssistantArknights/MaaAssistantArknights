// <copyright file="TaskGroup.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Models;

namespace MaaWpfGui.Configuration.Single;

public class TaskGroup : NotifyPropertyChangedWithValue
{
    public string Id { get; init; } = Guid.NewGuid().ToString("N");

    public string Name { get; set; } = string.Empty;

    public bool? IsEnable { get; set; } = true;

    public bool IsExpanded { get; set; } = true;

    public int Index { get; set; }
}
