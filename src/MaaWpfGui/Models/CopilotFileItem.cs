// <copyright file="CopilotFileItem.cs" company="MaaAssistantArknights">
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

using System.Collections.ObjectModel;

namespace MaaWpfGui.Models;

/// <summary>
/// 文件项数据模型，用于 TreeView 显示
/// </summary>
public class CopilotFileItem
{
    public string Name { get; set; } = string.Empty;

    public string? FullPath { get; set; }

    public string? RelativePath { get; set; }

    public bool IsFolder { get; set; }

    public ObservableCollection<CopilotFileItem> Children { get; set; } = [];
}
