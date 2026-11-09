// <copyright file="OperProgressMaterialHelper.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using Serilog;

namespace MaaWpfGui.Helper;

public static class OperProgressMaterialHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(OperProgressMaterialHelper));

    public record OperatorCost(
        [property: JsonPropertyName("elite")] Dictionary<string, int>[] Elite,
        [property: JsonPropertyName("skill")] Dictionary<string, int>[] Skill,
        [property: JsonPropertyName("mastery")] Dictionary<string, int>[][] Mastery);

    public static Dictionary<string, OperatorCost> Load(string filename)
    {
        if (!File.Exists(filename))
        {
            return [];
        }
        try
        {
            return JsonSerializer.Deserialize<Dictionary<string, OperatorCost>>(File.ReadAllText(filename)) ?? [];
        }
        catch (Exception e)
        {
            _logger.Warning(e, "Failed to load operator training costs from {Filename}", filename);
            return [];
        }
    }

    /// <summary>累加尚未完成的等级消耗；未知数据返回 null，避免覆盖已有缺料记录。</summary>
    /// <param name="costs">干员培养消耗表。</param>
    /// <param name="operatorId">干员或专精形态 ID。</param>
    /// <param name="step">培养步骤。</param>
    /// <param name="current">已达到的等级。</param>
    /// <param name="target">目标等级。</param>
    /// <param name="skillIndex">专精技能序号，1–3。</param>
    /// <returns>物品需求合计，无法计算时为 null。</returns>
    public static Dictionary<string, int>? Calculate(IReadOnlyDictionary<string, OperatorCost> costs, string operatorId,
        string? step, int current, int target, int skillIndex)
    {
        if (!costs.TryGetValue(operatorId, out var cost))
        {
            return null;
        }
        var levels = step switch
        {
            "elite" => cost.Elite,
            "skill_level" => cost.Skill,
            "skill_mastery" when skillIndex >= 1 && skillIndex <= cost.Mastery.Length => cost.Mastery[skillIndex - 1],
            _ => null,
        };
        int start = step == "skill_level" ? 1 : 0;
        if (levels is null || current < start || target <= current || target - start > levels.Length)
        {
            return null;
        }
        Dictionary<string, int> materials = [];
        for (int level = current - start; level < target - start; ++level)
        {
            foreach (var (id, count) in levels[level])
            {
                materials[id] = materials.GetValueOrDefault(id) + count;
            }
        }
        return materials;
    }
}
