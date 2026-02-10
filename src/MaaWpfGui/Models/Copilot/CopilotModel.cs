// <copyright file="CopilotModel.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;

namespace MaaWpfGui.Models.Copilot;

public class CopilotModel : CopilotBase
{
    /// <summary>
    /// Gets or sets 关卡名，必选。关卡中文名、code、stageId、levelId等，只要能保证唯一均可。
    /// </summary>
    [JsonProperty("stage_name")]
    public string StageName { get; set; } = string.Empty;

    [JsonProperty("opers")]
    public List<Oper> Opers { get; set; } = [];

    [JsonProperty("groups")]
    public List<Group> Groups { get; set; } = [];

    /// <summary>
    /// Gets or sets 战斗中的操作列表。有序，执行完前一个才会去执行下一个。必选
    /// </summary>
    [JsonProperty("actions")]
    public List<Action> Actions { get; set; } = [];

    /// <summary>
    /// Gets or sets 难度。可选，默认为 0
    /// </summary>
    [JsonProperty("difficulty")]
    public DifficultyFlags Difficulty { get; set; }

    public List<(string Output, string? Color)> Output()
    {
        var output = new List<(string, string?)>();
        if (Documentation is not null)
        {
            var title = Documentation.Title;
            if (!string.IsNullOrEmpty(title))
            {
                output.Add((title, Documentation.TitleColor ?? UiLogColor.Message));
            }

            var details = Documentation.Details;
            if (!string.IsNullOrEmpty(details))
            {
                output.Add((details, Documentation.DetailsColor ?? UiLogColor.Message));
            }
        }

        output.Add((string.Empty, null));
        output.Add(("------------------------------------------------", null));
        output.Add((string.Empty, null));
        var count = 0;
        foreach (var oper in Opers)
        {
            count++;
            var localizedName = DataHelper.GetLocalizedCharacterName(oper.Name);
            var log = $"{localizedName}{PrintLevelInfo(oper)} {LocalizationHelper.GetString("CopilotSkill")} {oper.Skill}{PrintSkillLevel(oper)} {GetModuleInfo(oper.Requirements)}".Trim();
            output.Add((log, UiLogColor.Message));
        }

        foreach (var group in Groups)
        {
            count++;
            var groupName = group.Name + ": ";
            var operInfos = group.Opers
                .Select(oper => $"{DataHelper.GetLocalizedCharacterName(oper.Name)} {oper.Skill}{PrintSkillLevel(oper)} {GetModuleInfo(oper.Requirements)}".Trim())
                .ToList();

            output.Add((groupName + string.Join(" / ", operInfos), UiLogColor.Message));
        }

        output.Add((LocalizationHelper.GetStringFormat("TotalOperatorsCount", count), UiLogColor.Message));
        return output;
    }

    private static string PrintLevelInfo(Oper oper)
    {
        if (oper.Requirements is not { } req || (req.Elite == 0 && req.Level <= 0))
        {
            return string.Empty;
        }
        return $" [{LocalizationHelper.GetStringFormat("CopilotFormationRequirement.Level", req.Elite, req.Level)}]";
    }

    private static string PrintSkillLevel(Oper oper)
    {
        if (oper.Requirements is not { } req || req.SkillLevel <= 0 || req.SkillLevel > 10)
        {
            return string.Empty;
        }
        return $" [Lv.{req.SkillLevel}]";
    }

    private static string GetModuleInfo(Requirements? req)
    {
        if (req is null || req.Module < 0)
        {
            return string.Empty;
        }
        string[] moduleName = [string.Empty, "χ", "γ", "α", "Δ"];

        // var moduleLevel = req.ModuleLevel > 0 ? $" [Lv.{req.ModuleLevel}]" : string.Empty;

        // 模组编号 -1: 不切换模组 / 无要求, 0: 不使用模组, 1: 模组χ, 2: 模组γ, 3: 模组α, 4: 模组Δ
        return req.Module switch {
            0 => $"{LocalizationHelper.GetString("CopilotWithoutModule")}",
            1 or 2 or 3 or 4 => $"{LocalizationHelper.GetString("CopilotModule")} {moduleName[req.Module]}",
            _ => string.Empty,
        };
    }

    public class Oper
    {
        /// <summary>
        /// Gets or sets 干员名，必选。
        /// </summary>
        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 技能序号，可选，默认为1，取值范围 [1, 3]
        /// </summary>
        [JsonProperty("skill")]
        public int Skill { get; set; } = 1;

        /// <summary>
        /// Gets or sets 技能用法。可选，默认为 0
        /// <list type="bullet">
        /// <item>0 - 不自动使用（依赖 "actions" 字段）</item>
        /// <item>1 - 好了就用，有多少次用多少次（例如干员 棘刺 3 技能、桃金娘 1 技能等）</item>
        /// <item>2 - 使用 X 次（例如干员 山 2 技能用 1 次、重岳 3 技能用 5 次，通过 "skill_times" 字段设置）</item>
        /// <item>3 - 自动判断使用时机（画饼.jpg）</item>
        /// </list>
        /// </summary>
        [JsonProperty("skill_usage")]
        public int SkillUsage { get; set; }

        /// <summary>
        /// Gets or sets 技能使用次数。可选，默认为 1
        /// </summary>
        [JsonProperty("skill_times")]
        public int SkillTimes { get; set; } = 1;

        /// <summary>
        /// Gets or sets 练度要求。保留接口，暂未实现。可选，默认为空
        /// </summary>
        [JsonProperty("requirements")]
        public Requirements? Requirements { get; set; }
    }

    public class Group
    {
        /// <summary>
        /// Gets or sets 群组名，必选
        /// </summary>
        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 干员列表。
        /// </summary>
        [JsonProperty("opers")]
        public List<Oper> Opers { get; set; } = [];
    }

    public class Action
    {
        /// <summary>
        /// Gets or sets 操作类型，可选，默认为 "Deploy"。
        /// <list type="bullet">
        /// <item>"Deploy" - 部署</item>
        /// <item>"Skill" - 技能</item>
        /// <item>"Retreat" - 撤退</item>
        /// <item>"SpeedUp" - 二倍速</item>
        /// <item>"BulletTime" - 子弹时间</item>
        /// <item>"SkillUsage" - 技能用法</item>
        /// <item>"Output" - 打印</item>
        /// <item>"SkillDaemon" - 摆完挂机</item>
        /// <item>"MoveCamera" - 移动镜头</item>
        /// </list>
        /// </summary>
        [JsonProperty("type")]
        public string Type { get; set; } = "Deploy";

        /// <summary>
        /// Gets or sets 击杀数条件，如果没达到就一直等待。可选，默认为 0，直接执行。
        /// </summary>
        [JsonProperty("kills")]
        public int Kills { get; set; }

        /// <summary>
        /// Gets or sets 费用条件，如果没达到就一直等待。可选，默认为 0，直接执行。
        /// </summary>
        [JsonProperty("costs")]
        public int Costs { get; set; }

        /// <summary>
        /// Gets or sets 费用变化量条件，如果没达到就一直等待。可选，默认为 0，直接执行。
        /// </summary>
        [JsonProperty("cost_changes")]
        public int CostChanges { get; set; }

        /// <summary>
        /// Gets or sets CD 中干员数量条件，如果没达到就一直等待。可选，默认为 -1，不识别。
        /// </summary>
        [JsonProperty("cooling")]
        public int Cooling { get; set; } = -1;

        /// <summary>
        /// Gets or sets 干员名 或 群组名， type 为 "部署" 时必选，为 "技能" | "撤退" 时可选。
        /// </summary>
        [JsonProperty("name")]
        public string? Name { get; set; }

        /// <summary>
        /// Gets or sets 部署干员的位置。type 为 "部署" 时必选。type 为 "技能" | "撤退" 时可选。
        /// </summary>
        [JsonProperty("location")]
        public List<int>? Location { get; set; }

        /// <summary>
        /// Gets or sets 部署干员的干员朝向。 type 为 "部署" 时必选。
        /// <list type="bullet">
        /// <item>"Left" - 左</item>
        /// <item>"Right" - 右</item>
        /// <item>"Up" - 上</item>
        /// <item>"Down" - 下</item>
        /// <item>"None" - 无</item>
        /// </list>
        /// </summary>
        [JsonProperty("direction")]
        public string? Direction { get; set; }

        /// <summary>
        /// Gets or sets 修改技能用法。当 type 为 "技能用法" 时必选。
        /// </summary>
        [JsonProperty("skill_usage")]
        public int SkillUsage { get; set; }

        /// <summary>
        /// Gets or sets 技能使用次数。可选，默认为 1。
        /// </summary>
        [JsonProperty("skill_times")]
        public int SkillTimes { get; set; } = 1;

        /// <summary>
        /// Gets or sets 前置延时。可选，默认为 0, 单位毫秒。
        /// </summary>
        [JsonProperty("pre_delay")]
        public int PreDelay { get; set; }

        /// <summary>
        /// Gets or sets 后置延时。可选，默认为 0, 单位毫秒。
        /// </summary>
        [JsonProperty("post_delay")]
        public int PostDelay { get; set; }

        /// <summary>
        /// Gets or sets 移动镜头的距离。type 为 "移动镜头" 时必选。
        /// </summary>
        [JsonProperty("distance")]
        public List<double>? Distance { get; set; }

        /// <summary>
        /// Gets or sets 描述，可选。会显示在界面上，没有实际作用
        /// </summary>
        [JsonProperty("doc")]
        public string? Doc { get; set; }

        /// <summary>
        /// Gets or sets 描述颜色，可选，默认灰色。会显示在界面上，没有实际作用
        /// </summary>
        [JsonProperty("doc_color")]
        public string? DocColor { get; set; }
    }

    public class Requirements
    {
        /// <summary>
        /// Gets or sets 精英化等级。可选，默认为 0, 不要求精英化等级。
        /// </summary>
        [JsonProperty("elite")]
        public int Elite { get; set; }

        /// <summary>
        /// Gets or sets 干员等级。可选，默认为 0。
        /// </summary>
        [JsonProperty("level")]
        public int Level { get; set; }

        /// <summary>
        /// Gets or sets 技能等级。可选，默认为 0。
        /// </summary>
        [JsonProperty("skill_level")]
        public int SkillLevel { get; set; }

        /// <summary>
        /// Gets or sets 模组编号。可选，默认为 -1。
        /// </summary>
        [JsonProperty("module")]
        public int Module { get; set; } = -1;
        /*
        /// <summary>
        /// Gets or sets 模组编号。可选，默认为 -1。
        /// </summary>
        [JsonProperty("module_level")]
        public int ModuleLevel { get; set; } = 0;
        */

        /// <summary>
        /// Gets or sets 潜能要求。可选，默认为 0。
        /// </summary>
        [JsonProperty("potentiality")]
        public int Potentiality { get; set; }
    }

    [Flags]
    public enum DifficultyFlags
    {
        /// <summary>
        /// 缺省，未设置
        /// </summary>
        None = 0,

        /// <summary>
        /// 普通
        /// </summary>
        Normal = 1,

        /// <summary>
        /// 突袭
        /// </summary>
        Raid = 2,
    }
}
