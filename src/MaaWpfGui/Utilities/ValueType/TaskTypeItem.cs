// <copyright file="TaskTypeItem.cs" company="MaaAssistantArknights">
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

#nullable enable
using System;

namespace MaaWpfGui.Utilities.ValueType;

/// <summary>
/// 任务队列 ｢添加任务｣ 菜单的条目。在 <see cref="GenericCombinedData{TValueType}"/> 基础上
/// 附带 ｢新任务｣ 红点标记所需的信息：红点不是标 ｢功能绝对新旧｣ ，而是标 ｢引入版本晚于用户
/// 上次浏览菜单时的版本｣ （基准由 <c>Root.Gui.AddTaskMenuSeenVersion</c> 记录）；基准为空
/// （首次启动）时按当前运行版本正常比较并特判 v6.19 系列，见
/// <c>TaskQueueViewModel.IsNewerThanBaseline</c>。
/// </summary>

/// <param name="display">显示文本。</param>
/// <param name="value">任务类型。</param>
/// <param name="introducedVersion">该任务类型引入的版本号，须用功能实际上线的首个版本号（如 "6.19.0-beta.2"；登记正式号会让上线前的预发布用户基准永远追不上登记值，红点无法消除）；null 表示早于红点机制的任务，永不标记。</param>
/// <param name="isDebugOnly">是否仅在调试模式（ShowDebugTask）下显示。</param>
public class TaskTypeItem(string display, Type value, string? introducedVersion = null, bool isDebugOnly = false)
    : GenericCombinedData<Type>(display, value)
{
    /// <summary>
    /// Gets 该任务类型引入的版本号。null 表示早于红点机制的任务，不参与标记。
    /// </summary>
    public string? IntroducedVersion { get; } = introducedVersion;

    /// <summary>
    /// Gets or sets a value indicating whether 本条目相对用户上次浏览菜单的版本为新任务。由 TaskQueueViewModel 计算写入。
    /// </summary>
    public bool IsNew
    {
        get => _isNew;
        set => SetAndNotify(ref _isNew, value);
    }

    private bool _isNew;

    /// <summary>
    /// Gets a value indicating whether 本条目仅在调试模式下显示。
    /// </summary>
    public bool IsDebugOnly { get; } = isDebugOnly;
}
