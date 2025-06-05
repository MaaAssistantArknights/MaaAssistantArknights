// <copyright file="AsstReclamationTask.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 自动生息演算。
/// </summary>
public class AsstReclamationTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Reclamation;

    /// <summary>
    /// Gets or sets 生息演算主题
    /// </summary>
    public string Theme { get; set; } = "Tales";

    /// <summary>
    /// Gets or sets 生息演算模式
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
    /// </summary>
    public int Mode { get; set; } = 0;

    /// <summary>
    /// Gets or sets 点击类型：0 连点；1 长按
    /// </summary>
    public int IncrementMode { get; set; } = 0;

    /// <summary>
    /// Gets or sets 单次最大制造轮数
    /// </summary>
    public int MaxCraftCountPerRound { get; set; } = 1;

    /// <summary>
    /// Gets or sets 要组装的支援道具。
    /// </summary>
    public List<string> ToolToCraft { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 刷完点数后购买商店
    /// </summary>
    public bool ClearStore { get; set; } = false;

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var data = new JObject
        {
            ["theme"] = Theme,
            ["mode"] = Mode,
            ["increment_mode"] = IncrementMode,
            ["num_craft_batches"] = MaxCraftCountPerRound,
            ["tools_to_craft"] = new JArray(ToolToCraft),
            ["clear_store"] = ClearStore,
        };

        return (TaskType, data);
    }
}
