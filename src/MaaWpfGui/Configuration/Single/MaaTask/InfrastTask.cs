// <copyright file="InfrastTask.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Models;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using Serilog;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

public class InfrastTask : BaseTask, IJsonOnDeserialized
{
    private static readonly ILogger _logger = Log.ForContext<InfrastTask>();

    public InfrastTask() => TaskType = TaskType.Infrast;

    /// <summary>
    /// Gets or sets 基建切换模式
    /// </summary>
    public InfrastMode Mode { get; set; } = InfrastMode.Normal;

    /// <summary>
    /// Gets or sets 基建无人机用法
    /// </summary>
    public string UsesOfDrones { get; set; } = "Money";

    /// <summary>
    /// Gets or sets 基建心情阈值
    /// </summary>
    public int DormThreshold { get; set; } = 30;

    /// <summary>
    /// Gets or sets a value indicating whether 宿舍空位补信赖
    /// </summary>
    public bool DormTrustEnabled { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 制造站搓玉是否补货
    /// </summary>
    public bool OriginiumShardAutoReplenishment { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 不将已进驻干员放入宿舍
    /// </summary>
    public bool DormFilterNotStationed { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 会客室板子领取信用
    /// </summary>
    public bool ReceptionMessageBoard { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 会客室线索交换
    /// </summary>
    public bool ReceptionClueExchange { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 赠送线索
    /// </summary>
    public bool SendClue { get; set; } = true;

    /// <summary>
    /// Gets or sets a value indicating whether 继续专精
    /// </summary>
    public bool ContinueTraining { get; set; } = false;

    public string FiammettaTarget1 { get; set; } = "清流";

    public string FiammettaTarget2 { get; set; } = "可露希尔";

    public string FiammettaTarget3 { get; set; } = "但书";

    /// <summary>
    /// Gets or sets a value indicating whether the Pinus Sylvestris cross-facility team is enabled.
    /// </summary>
    public bool UsePinusSylvestris { get; set; } = false;

    /// <summary>
    /// Gets or sets a value indicating whether the Perception Information cross-facility team is enabled.
    /// </summary>
    public bool UsePerceptionInformation { get; set; } = false;

    /// <summary>
    /// Gets or sets a value indicating whether the Worldly Plight cross-facility team is enabled.
    /// </summary>
    public bool UseWorldlyPlight { get; set; } = false;

    /// <summary>
    /// Gets or sets a value indicating whether the Abyssal Hunter cross-facility team is enabled.
    /// </summary>
    public bool UseAbyssalHunter { get; set; } = false;

    /// <summary>
    /// Gets or sets a value indicating whether efficient manufacturing rooms may keep their current operators.
    /// </summary>
    public bool MfgShortCircuit { get; set; } = false;

    /// <summary>
    /// Gets or sets the average manufacturing efficiency threshold as a percentage.
    /// </summary>
    public int MfgShortCircuitThreshold { get; set; } = 38;

    public string CustomFileType { get; set; } = InfrastSettingsUserControlModel.UserDefined;

    /// <summary>
    /// Gets or sets 自定义配置文件路径
    /// </summary>
    public string Filename { get; set; } = string.Empty;

    [JsonIgnore]
    public List<CustomInfrastConfig.Plan> InfrastPlan { get; set; } = [];

    /// <summary>
    /// Gets or sets 自定义配置计划编号
    /// </summary>
    public int PlanSelect { get; set; } = -1;

    public List<RoomInfo> RoomList { get; set; } =
       [.. typeof(InfrastRoomType).GetEnumValues().OfType<InfrastRoomType>().Select<InfrastRoomType, RoomInfo>(i => new(i, true))];

    public void OnDeserialized()
    {
        if (Mode != InfrastMode.Custom || string.IsNullOrWhiteSpace(Filename) || !File.Exists(Filename))
        {
            return;
        }
        try
        {
            string jsonStr = File.ReadAllText(Filename);
            if (Newtonsoft.Json.JsonConvert.DeserializeObject<CustomInfrastConfig>(jsonStr) is not CustomInfrastConfig root)
            {
                throw new JsonException("DeserializeObject returned null");
            }
            var planList = root.Plans;
            for (int i = 0; i < planList.Count; ++i)
            {
                var plan = planList[i];
                plan.Index = i;
                plan.Name ??= "Plan " + ((char)('A' + i));
                plan.Description ??= string.Empty;
                plan.DescriptionPost ??= string.Empty;
                InfrastPlan.Add(plan);
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "InfrastTask: OnDeserialized failed");
            PlanSelect = 0;
        }
        if (PlanSelect < -1)
        {
            PlanSelect = -1;
            _logger.Warning("InfrastTask: PlanSelect < -1, reset to -1");
        }
        else if (PlanSelect >= InfrastPlan.Count)
        {
            PlanSelect = 0;
            _logger.Warning("InfrastTask: PlanSelect >= InfrastPlan.Count, reset to 0");
        }
    }

    public record RoomInfo(InfrastRoomType Room, [property: JsonPredict("IsEnabled", false)] bool IsEnabled = true);
}
