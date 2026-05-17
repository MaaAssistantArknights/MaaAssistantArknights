// <copyright file="ReclamationTask.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 生息演算任务
/// </summary>
public class ReclamationTask : BaseTask
{
    public ReclamationTask() => TaskType = TaskType.Reclamation;

    public ReclamationTheme Theme { get; set; } = ReclamationTheme.Tales;

    /// <summary>
    /// Gets or sets 生息演算模式（含义由 Theme 决定）
    /// <list type="bullet">
    ///     <item>
    ///         <term><c>0</c></term>
    ///         <description>Tales: 无存档刷繁荣点数 / RelaunchAnchor: RA-1</description>
    ///     </item>
    ///     <item>
    ///         <term><c>1</c></term>
    ///         <description>Tales: 有存档刷繁荣点数 / RelaunchAnchor: RA-15</description>
    ///     </item>
    /// </list>
    /// </summary>
    public int Mode { get; set; } = 0;

    /// <summary>
    /// Gets or sets 要组装的支援道具
    /// </summary>
    public string ToolToCraft { get; set; } = string.Empty;

    public int IncrementMode { get; set; }

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int MaxCraftCountPerRound { get; set; } = 16;

    /// <summary>
    /// Gets or sets a value indicating whether 商店购物
    /// </summary>
    public bool ClearStore { get; set; } = true;
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

    /// <summary>
    /// 重启锚点（RELAUNCH ANCHOR）
    /// </summary>
    RelaunchAnchor,
}

public enum TalesMode
{
    /// <summary>
    /// 无存档，通过进出关卡刷生息点数
    /// </summary>
    ProsperityNoSave = 0,

    /// <summary>
    /// 有存档，通过组装支援道具刷生息点数
    /// </summary>
    ProsperityInSave = 1,
}

public enum RelaunchAnchorMode
{
    /// <summary>
    /// RA-1
    /// </summary>
    RA1 = 0,

    /// <summary>
    /// RA-15
    /// </summary>
    RA15 = 1,
}
