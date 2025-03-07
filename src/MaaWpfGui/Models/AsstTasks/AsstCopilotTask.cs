// <copyright file="AsstCopilotTask.cs" company="MaaAssistantArknights">
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
using System.Linq;
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
    /// Gets or sets 作业 JSON 的文件路径，绝对、相对路径均可
    /// </summary>
    public string FileName { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets a value indicating whether 自动编队
    /// </summary>
    public bool Formation { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 追加信赖干员
    /// </summary>
    public bool AddTrust { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 导航至关卡（启用自动战斗序列、取消代理）
    /// </summary>
    public bool NeedNavigate { get; set; }

    /// <summary>
    /// Gets or sets 关卡名，仅导航用，Wpf会自动读取地图对应的关卡名
    /// </summary>
    public string? StageName { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 突袭难度
    /// </summary>
    public bool IsRaid { get; set; }

    /// <summary>
    /// Gets or sets 重复次数
    /// </summary>
    public int LoopTimes { get; set; }

    /// <summary>
    /// Gets or sets a value indicating whether 使用理智药
    /// </summary>
    public bool UseSanityPotion { get; set; }

    /// <summary>
    /// Gets or sets 自定干员列表
    /// </summary>
    public List<UserAdditional>? UserAdditionals { get; set; }

    public override (AsstTaskType TaskType, JObject Params) Serialize()
    {
        var taskParams = new JObject
        {
            ["filename"] = FileName,
            ["formation"] = Formation,
            ["add_trust"] = AddTrust,
            ["need_navigate"] = NeedNavigate,
            ["is_raid"] = IsRaid,
            ["loop_times"] = LoopTimes,
            ["use_sanity_potion"] = UseSanityPotion,
        };

        if (UserAdditionals?.Count > 0)
        {
            taskParams["user_additional"] = JArray.FromObject(UserAdditionals);
        }

        if (NeedNavigate)
        {
            taskParams["navigate_name"] = StageName;
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

        [JsonProperty("skill")]
        public int Skill { get; set; }
    }
}
