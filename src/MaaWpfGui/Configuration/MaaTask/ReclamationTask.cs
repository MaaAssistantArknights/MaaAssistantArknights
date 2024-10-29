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
using MaaWpfGui.Helper;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Configuration.MaaTask;

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
    public string? ToolToCraft { get; set; }

    public int IncrementMode { get; set; }

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int MaxCraftCountPerRound { get; set; } = 16;

    /// <summary>
    /// 自动生息演算。
    /// </summary>
    /// <param name="theme">生息演算主题["Tales"]</param>
    /// <param name="mode">
    /// 策略。可用值包括：
    /// <list type="bullet">
    ///     <item>
    ///         <term><c>0</c></term>
    ///         <description>无存档时通过进出关卡刷生息点数</description>
    ///     </item>
    ///     <item>
    ///         <term><c>1</c></term>
    ///         <description>有存档时通过合成支援道具刷生息点数</description>
    ///     </item>
    /// </list>
    /// </param>
    /// <param name="toolToCraft">要组装的支援道具。</param>
    /// <param name="incrementMode">点击类型：0 连点；1 长按</param>
    /// <param name="numCraftBatches">单次最大制造轮数</param>
    /// <returns>是否成功。</returns>
    /// string theme = "Tales", int mode = 1, string toolToCraft = "", int incrementMode = 0, int numCraftBatches = 16
    public override JObject SerializeJsonTask()
    {
        var json = new JObject()
        {
            ["theme"] = Theme.ToString(),
            ["mode"] = (int)Mode,
        };

        if (Mode == ReclamationMode.Archive)
        {
            json["tool_to_craft"] = ToolToCraft ?? LocalizationHelper.GetString("ReclamationToolToCraftPlaceholder");
            json["increment_mode"] = IncrementMode;
            json["num_craft_batches"] = MaxCraftCountPerRound;
        }

        return json;
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
}
