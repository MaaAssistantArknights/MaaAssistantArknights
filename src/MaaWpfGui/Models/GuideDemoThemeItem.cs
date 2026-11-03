// <copyright file="GuideDemoThemeItem.cs" company="MaaAssistantArknights">
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

using Stylet;

namespace MaaWpfGui.Models;

/// <summary>
/// 设置指引演示用的更换主题任务主题名条目，纯内存演示数据。
/// </summary>
public class GuideDemoThemeItem(string name = "") : PropertyChangedBase
{
    /// <summary>
    /// Gets or sets the demo theme name.
    /// </summary>
    public string Name { get => field; set => SetAndNotify(ref field, value); } = name;
}
