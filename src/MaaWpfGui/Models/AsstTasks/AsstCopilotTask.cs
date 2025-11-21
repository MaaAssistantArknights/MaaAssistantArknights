// <copyright file="AsstCopilotTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 自动抄作业。
/// </summary>
public class AsstCopilotTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Copilot;

    /// <summary>
    /// Gets or sets 作业 JSON 的文件路径，绝对、相对路径均可，不得与MultiTasks同时使用
    /// </summary>
    public string FileName { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 多作业列表, 导航至关卡 (启用自动战斗序列、取消代理), 不得与FileName同时使用
    /// </summary>
    public List<MultiTask> MultiTasks { get; set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 自动编队
    /// </summary>
    public bool Formation { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 自动编队
    /// </summary>
    public int SupportUnitUsage { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 追加信赖干员
    /// </summary>
    public bool AddTrust { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 跳过未满足的干员属性要求
    /// </summary>
    public bool IgnoreRequirements { get; set; }

    /// <summary>
    /// Gets or sets 重复次数
    /// </summary>
    public int LoopTimes { get; set; } = 1;

    /// <summary>
    /// Gets or sets a value indicating whether 使用理智药
    /// </summary>
    public bool UseSanityPotion { get; set; }

    /// <summary>
    /// Gets or sets 自定干员列表
    /// </summary>
    public List<UserAdditional>? UserAdditionals { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 信用战斗选择编队
    /// </summary>
    public int FormationIndex { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var taskParams = new JObject
        {
            ["formation"] = Formation,
            ["support_unit_usage"] = SupportUnitUsage,
            ["add_trust"] = AddTrust,
            ["ignore_requirements"] = IgnoreRequirements,
            ["loop_times"] = LoopTimes,
            ["use_sanity_potion"] = UseSanityPotion,
        };

        if (!string.IsNullOrEmpty(FileName) && MultiTasks.Count > 0)
        {
            throw new ArgumentException("FileName和MultiTasks不能同时使用");
        }
        else if (MultiTasks.Count > 0)
        {
            taskParams["copilot_list"] = JArray.FromObject(MultiTasks);
        }
        else if (!string.IsNullOrEmpty(FileName))
        {
            taskParams["filename"] = FileName;
        }
        else
        {
            throw new ArgumentException("FileName和MultiTasks必须使用其一");
        }

        if (FormationIndex > 0)
        {
            taskParams["formation_index"] = FormationIndex;
        }

        if (UserAdditionals?.Count > 0)
        {
            taskParams["user_additional"] = JArray.FromObject(UserAdditionals);
        }

        return (TaskType, taskParams);
    }

    public class UserAdditional
    {
        /// <summary>
        /// Gets or sets 干员名，ocr文本，跟随客户端语言
        /// </summary>
        [JsonProperty("name")]
        public string Name { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 技能序号，可选，默认为 1，取值范围 [1, 3]
        /// </summary>
        [JsonProperty("skill")]
        public int Skill { get; set; } = 1;

        /// <summary>
        /// Gets or sets 模组编号，可选，默认为 0
        /// -1: 不切换模组 / 无要求, 0: 不使用模组, 1: 模组 χ, 2: 模组 γ, 3: 模组 α, 4: 模组 Δ
        /// 当前核心仅使用 name 和 skill 字段，module 作为预留字段
        /// </summary>
        [JsonProperty("module")]
        public int Module { get; set; }
    }

    public class MultiTask
    {
        [JsonProperty("filename")]
        public string FileName { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets 关卡名，仅导航用，Wpf 会自动读取地图对应的关卡名
        /// </summary>
        [JsonProperty("stage_name")]
        public string StageName { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets a value indicating whether 突袭难度
        /// </summary>
        [JsonProperty("is_raid")]
        public bool IsRaid { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether 悖论模拟
        /// </summary>
        [JsonProperty("is_paradox")]
        public bool IsParadox { get; set; }
    }
}
