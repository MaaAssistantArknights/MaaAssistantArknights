// <copyright file="SSSCopilotModel.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;

namespace MaaWpfGui.Models;

public class SSSCopilotModel
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
    /// Gets or sets 最低要求 maa 版本号，必选
    /// </summary>
    [JsonProperty("minimum_required")]
    public string MinimumRequired { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 描述，可选
    /// </summary>
    [JsonProperty("doc")]
    public Doc? Documentation { get; set; }

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

    public class Doc
    {
        [JsonProperty("title")]
        public string? Title { get; set; }

        [JsonProperty("title_color")]
        public string? TitleColor { get; set; }

        [JsonProperty("details")]
        public string? Details { get; set; }

        [JsonProperty("details_color")]
        public string? DetailsColor { get; set; }
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
        }
    }
}
