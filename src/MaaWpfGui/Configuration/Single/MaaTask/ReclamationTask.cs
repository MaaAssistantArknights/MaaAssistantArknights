// <copyright file="ReclamationTask.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 生息演算任务
/// </summary>
public class ReclamationTask : BaseTask
{
    public ReclamationTask() => TaskType = TaskType.Reclamation;

    public ReclamationTheme Theme { get; set; } = ReclamationTheme.Tales;

    public ReclamationMode Mode { get; set; } = ReclamationMode.Archive;

    /// <summary>
    /// Gets or sets 要组装的支援道具
    /// </summary>
    public string ToolToCraft { get; set; } = string.Empty;

    public int IncrementMode { get; set; }

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int MaxCraftCountPerRound { get; set; } = 16;
}

public enum ReclamationTheme
{
    /// <summary>
    /// 沙中之火
    /// </summary>
    Fire,

    /// <summary>
    /// 沙洲遗闻
    /// </summary>
    Tales,
}

public enum ReclamationMode
{
    /// <summary>
    /// 无存档，通过进出关卡刷生息点数
    /// </summary>
    NoArchive = 0,

    /// <summary>
    /// 有存档，通过组装支援道具刷生息点数
    /// </summary>
    Archive = 1,
}
