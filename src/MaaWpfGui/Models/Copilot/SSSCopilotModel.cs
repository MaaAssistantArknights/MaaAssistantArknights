// <copyright file="SSSCopilotModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using Newtonsoft.Json;

namespace MaaWpfGui.Models.Copilot;

public class SSSCopilotModel : CopilotBase
{
    /// <summary>
    /// Gets 协议类型，SSS 表示保全派驻，必选，不可修改
    /// </summary>
    [JsonProperty("type")]
    public string Type { get; } = "SSS";

    /// <summary>
    /// Gets or sets 保全派驻地图名，必选，例：多索雷斯在建地块
    /// </summary>
    [JsonProperty("stage_name")]
    public string StageName { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 开局导能元件选择，可选
    /// </summary>
    [JsonProperty("buff")]
    public string? Buff { get; set; }

    /// <summary>
    /// Gets or sets 开局装备选择，横着数，可选。当前版本暂未实现，只会在界面上显示一下
    /// </summary>
    [JsonProperty("equipment")]
    public List<string>? Equipment { get; set; }

    /// <summary>
    /// Gets or sets 优选策略，可选。当前版本暂未实现，只会在界面上显示一下
    /// </summary>
    [JsonProperty("strategy")]
    public string? Strategy { get; set; }

    /// <summary>
    /// Gets or sets 指定干员，可选
    /// </summary>
    [JsonProperty("opers")]
    public List<Oper>? Opers { get; set; }

    /// <summary>
    /// Gets or sets 剩余所需各职业人数，按费用排序随便拿，可选
    /// </summary>
    [JsonProperty("tool_men")]
    public Dictionary<string, int>? ToolMen { get; set; }

    /// <summary>
    /// Gets or sets 战斗开始时和战斗中途，招募干员、获取装备优先级
    /// </summary>
    [JsonProperty("drops")]
    public List<string>? Drops { get; set; }

    /// <summary>
    /// Gets or sets 黑名单，可选。在 drops 里不会选这些人。
    /// </summary>
    [JsonProperty("blacklist")]
    public List<string>? Blacklist { get; set; }

    /// <summary>
    /// Gets or sets 关卡信息
    /// </summary>
    [JsonProperty("stages")]
    public List<Stage>? Stages { get; set; }

    private static readonly Dictionary<OperatorRole, string[]> _typeAliases = new()
    {
        { OperatorRole.Warrior, ["Warrior", "Guard", "近卫"] },
        { OperatorRole.Pioneer, ["Pioneer", "Vanguard", "先锋"] },
        { OperatorRole.Medic, ["Medic", "医疗"] },
        { OperatorRole.Tank, ["Tank", "Defender", "重装", "坦克"] },
        { OperatorRole.Sniper, ["Sniper", "狙击"] },
        { OperatorRole.Caster, ["Caster", "术师", "术士", "法师"] },
        { OperatorRole.Support, ["Support", "Supporter", "辅助", "支援"] },
        { OperatorRole.Special, ["Special", "Specialist", "特种"] },
        { OperatorRole.Drone, ["Drone", "Summon", "无人机", "召唤物"] },
    };

    private static readonly Dictionary<string, OperatorRole> _nameToOperType =
        _typeAliases.SelectMany(kv => kv.Value.Select(v => new { v, kv.Key }))
            .ToDictionary(x => x.v, x => x.Key, StringComparer.OrdinalIgnoreCase);

    private static string GetLocalizedToolmenName(string key)
    {
        return _nameToOperType.TryGetValue(key, out var operType)
            ? LocalizationHelper.GetString(operType.ToString())
            : key;
    }

    public List<(string Output, string? Color)> Output()
    {
        var output = new List<(string, string?)>();

        if (Documentation is not null)
        {
            var title = Documentation.Title;
            if (!string.IsNullOrEmpty(title))
            {
                output.Add((title, Documentation.TitleColor));
            }

            var details = Documentation.Details;
            if (!string.IsNullOrEmpty(details))
            {
                output.Add((details, Documentation.DetailsColor));
            }
        }

        output.Add((string.Empty, null));
        output.Add(("------------------------------------------------", null));
        output.Add((string.Empty, null));

        var count = 0;
        foreach (var oper in Opers ?? [])
        {
            count++;
            var localizedName = DataHelper.GetLocalizedCharacterName(oper.Name);
            output.Add(($"{localizedName}, {LocalizationHelper.GetString("CopilotSkill")} {oper.Skill}", null));
        }

        output.Add((LocalizationHelper.GetStringFormat("TotalOperatorsCount", count), null));

        if (ToolMen is not null)
        {
            var toolMenLog = LocalizationHelper.GetString("OtherOperators");
            var toolMenString = string.Join("\n", ToolMen.Where(kv => kv.Value > 0).Select(kv => $"{GetLocalizedToolmenName(kv.Key)}: {kv.Value}"));
            output.Add((toolMenLog + "\n" + toolMenString, null));
        }

        if (Buff is not null)
        {
            var buffLog = LocalizationHelper.GetString("DirectiveECTerm");
            var localizedBuffName = DataHelper.GetLocalizedCharacterName(Buff);
            output.Add((buffLog + (string.IsNullOrEmpty(localizedBuffName) ? Buff : localizedBuffName), null));
        }

        if (Equipment is not null)
        {
            var equipmentLog = LocalizationHelper.GetString("InitialEquipmentHorizontal") + '\n';
            output.Add((equipmentLog + string.Join('\n', Equipment.Chunk(4).Select(i => string.Join(",", i))), null));
        }

        if (Strategy is not null)
        {
            var strategyLog = LocalizationHelper.GetString("InitialStrategy");
            output.Add((strategyLog + Strategy, null));
        }

        return output;
    }

    public class Oper
    {
        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        [JsonProperty("skill")]
        public int Skill { get; set; }

        [JsonProperty("skill_usage")]
        public int SkillUsage { get; set; }
    }

    public class Stage
    {
        /// <summary>
        /// Gets or sets 关卡名，必选，Core在识别时会转为识别LT-1~6
        /// </summary>
        [JsonProperty("stage_name")]
        public string StageName { get; set; } = string.Empty;

        [JsonProperty("strategies")]
        public List<Strategy>? Strategies { get; set; }

        [JsonProperty("draw_as_possible")]
        public bool? DrawAsPossible { get; set; }

        [JsonProperty("actions")]
        public List<Action>? Actions { get; set; }

        [JsonProperty("retry_times")]
        public int? RetryTimes { get; set; }

        public class Strategy
        {
            [JsonProperty("core")]
            public string? Core { get; set; }

            [JsonProperty("tool_men")]
            public Dictionary<string, int>? ToolMen { get; set; }

            [JsonProperty("location")]
            public List<int>? Location { get; set; }

            [JsonProperty("direction")]
            public string? Direction { get; set; }
        }

        public class Action
        {
            [JsonProperty("type")]
            public string Type { get; set; } = string.Empty;

            [JsonProperty("name")]
            public string? Name { get; set; }

            [JsonProperty("location")]
            public List<int>? Location { get; set; }

            [JsonProperty("kills")]
            public int? Kills { get; set; }

            // Click 的点击区域，与 location 至少填一个，同时填写时优先使用 rect；begin/end 为 Swipe 起终点区域；均为 720p 基准像素矩形 [x, y, w, h]
            [JsonProperty("rect")]
            public List<int>? Rect { get; set; }

            [JsonProperty("begin")]
            public List<int>? Begin { get; set; }

            [JsonProperty("end")]
            public List<int>? End { get; set; }

            [JsonProperty("duration")]
            public int? Duration { get; set; }

            [JsonProperty("extra_swipe")]
            public int? ExtraSwipe { get; set; }

            // slope 为 ×10 整数，10 即 1.0；null（未填）时 core 按 10 处理，显式 0 合法（即斜率 0.0）
            [JsonProperty("slope_in")]
            public int? SlopeIn { get; set; }

            [JsonProperty("slope_out")]
            public int? SlopeOut { get; set; }

            [JsonProperty("with_pause")]
            public bool? WithPause { get; set; }

            [JsonProperty("high_resolution_swipe_fix")]
            public bool? HighResolutionSwipeFix { get; set; }

            // MoveCamera 专用：true 时不等待当前波次结束、击杀数不清零，用于同一波次内移动镜头；null（未填）时 core 按 false 处理
            [JsonProperty("keep_kills")]
            public bool? KeepKills { get; set; }

            // MoveCamera 专用：镜头移动量 [x 格数, y 格数]，可小数可负
            [JsonProperty("distance")]
            public List<double>? Distance { get; set; }
        }
    }
}
