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
using System.Collections.Generic;
using System.Linq;
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

    /// <summary>
    /// 干员职业类型枚举。
    /// </summary>
    private enum Role
    {
        /// <summary>
        /// 近卫/Guard
        /// </summary>
        Warrior,

        /// <summary>
        /// 先锋/Vanguard
        /// </summary>
        Pioneer,

        /// <summary>
        /// 医疗/Medic
        /// </summary>
        Medic,

        /// <summary>
        /// 重装/Defender/坦克
        /// </summary>
        Tank,

        /// <summary>
        /// 狙击/Sniper
        /// </summary>
        Sniper,

        /// <summary>
        /// 术师/术士/法师/Caster
        /// </summary>
        Caster,

        /// <summary>
        /// 辅助/支援/Supporter
        /// </summary>
        Support,

        /// <summary>
        /// 特种/Specialist
        /// </summary>
        Special,

        /// <summary>
        /// 无人机/召唤物/Drone/Summon
        /// </summary>
        Drone,
    }

    // 在 SSSCopilotModel 类中嵌入 NameToRole 映射
    private static readonly Dictionary<string, Role> NameToRole = new()
    {
        // Warrior/Guard/近卫
        { "WARRIOR", Role.Warrior },    { "Warrior", Role.Warrior },    { "warrior", Role.Warrior },
        { "GUARD", Role.Warrior },      { "Guard", Role.Warrior },      { "guard", Role.Warrior },
        { "近卫", Role.Warrior },

        // Pioneer/Vanguard/先锋
        { "PIONEER", Role.Pioneer },    { "Pioneer", Role.Pioneer },    { "pioneer", Role.Pioneer },
        { "VANGUARD", Role.Pioneer },   { "Vanguard", Role.Pioneer },   { "vanguard", Role.Pioneer },
        { "先锋", Role.Pioneer },

        // Medic/医疗
        { "MEDIC", Role.Medic },        { "Medic", Role.Medic },        { "medic", Role.Medic },
        { "医疗", Role.Medic },

        // Tank/Defender/重装/坦克
        { "TANK", Role.Tank },          { "Tank", Role.Tank },          { "tank", Role.Tank },
        { "DEFENDER", Role.Tank },      { "Defender", Role.Tank },      { "defender", Role.Tank },
        { "重装", Role.Tank },          { "坦克", Role.Tank },

        // Sniper/狙击
        { "SNIPER", Role.Sniper },      { "Sniper", Role.Sniper },      { "sniper", Role.Sniper },
        { "狙击", Role.Sniper },

        // Caster/术师/术士/法师
        { "CASTER", Role.Caster },      { "Caster", Role.Caster },      { "caster", Role.Caster },
        { "术师", Role.Caster },        { "术士", Role.Caster },        { "法师", Role.Caster },

        // Support/Supporter/辅助/支援
        { "SUPPORT", Role.Support },    { "Support", Role.Support },    { "support", Role.Support },
        { "SUPPORTER", Role.Support },  { "Supporter", Role.Support },  { "supporter", Role.Support },
        { "辅助", Role.Support },       { "支援", Role.Support },

        // Special/Specialist/特种
        { "SPECIAL", Role.Special },    { "Special", Role.Special },    { "special", Role.Special },
        { "SPECIALIST", Role.Special }, { "Specialist", Role.Special }, { "specialist", Role.Special },
        { "特种", Role.Special },

        // Drone/Summon/无人机/召唤物
        { "DRONE", Role.Drone },        { "Drone", Role.Drone },        { "drone", Role.Drone },
        { "SUMMON", Role.Drone },       { "Summon", Role.Drone },       { "summon", Role.Drone },
        { "无人机", Role.Drone },       { "召唤物", Role.Drone },
    };

    private static string GetLocalizedToolmenName(string key)
    {
        if (NameToRole.TryGetValue(key, out var role))
        {
            // 枚举转字符串后用于本地化查找
            return LocalizationHelper.GetString(role.ToString());
        }

        // 未匹配到时返回原始 key
        return key;
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

        output.Add((string.Format(LocalizationHelper.GetString("TotalOperatorsCount"), count), null));

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
        }
    }
}
