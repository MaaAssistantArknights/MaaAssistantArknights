// <copyright file="CoreTask.cs" company="MaaAssistantArknights">
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
using System;
using System.ComponentModel;
using System.Text.Json.Serialization;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Configuration.MaaTask
{
    public class CloseDownTask : BaseTask
    {
    }

    public class FightTask : BaseTask
    {
    }

    public class CopilotTask : BaseTask
    {
    }

    public class SSSCopilotTask : BaseTask
    {
    }

    public class SingleStepTask : BaseTask
    {
    }

    public class VideoRecognition : BaseTask
    {
    }

    public class DepotTask : BaseTask
    {
    }

    public class OperBoxTask : BaseTask
    {
    }

    public class CustomTask : BaseTask
    {
    }

    [JsonDerivedType(typeof(StartUpTask), typeDiscriminator: nameof(StartUpTask))]
    [JsonDerivedType(typeof(CloseDownTask), typeDiscriminator: nameof(CloseDownTask))]
    [JsonDerivedType(typeof(FightTask), typeDiscriminator: nameof(FightTask))]
    [JsonDerivedType(typeof(AwardTask), typeDiscriminator: nameof(AwardTask))]
    [JsonDerivedType(typeof(MallTask), typeDiscriminator: nameof(MallTask))]
    [JsonDerivedType(typeof(InfrastTask), typeDiscriminator: nameof(InfrastTask))]
    [JsonDerivedType(typeof(RecruitTask), typeDiscriminator: nameof(RecruitTask))]
    [JsonDerivedType(typeof(RoguelikeTask), typeDiscriminator: nameof(RoguelikeTask))]
    [JsonDerivedType(typeof(CopilotTask), typeDiscriminator: nameof(CopilotTask))]
    [JsonDerivedType(typeof(SSSCopilotTask), typeDiscriminator: nameof(SSSCopilotTask))]
    [JsonDerivedType(typeof(SingleStepTask), typeDiscriminator: nameof(SingleStepTask))]
    [JsonDerivedType(typeof(VideoRecognition), typeDiscriminator: nameof(VideoRecognition))]
    [JsonDerivedType(typeof(DepotTask), typeDiscriminator: nameof(DepotTask))]
    [JsonDerivedType(typeof(OperBoxTask), typeDiscriminator: nameof(OperBoxTask))]
    [JsonDerivedType(typeof(ReclamationTask), typeDiscriminator: nameof(ReclamationTask))]
    [JsonDerivedType(typeof(CustomTask), typeDiscriminator: nameof(CustomTask))]
    public class BaseTask : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler? PropertyChanged;

        public string Name { get; set; } = string.Empty;

        public bool? IsEnable { get; set; } = false;

        /// <summary>
        /// Gets or sets 任务id，默认为0，添加后任务id应 > 0；执行后应置为0
        /// </summary>
        public int TaskId { get; set; }

        /// <summary>
        /// Gets 任务类型，用于添加任务时使用
        /// </summary>
        public TaskType TaskType { get; init; }

        public virtual JObject SerializeJsonTask()
        {
            throw new NotImplementedException();
        }

        // ReSharper disable once UnusedMember.Global
        public void OnPropertyChanged(string propertyName, object before, object after)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventDetailArgs(propertyName, before, after));
        }
    }


    public enum TaskType
    {
        /// <summary>
        /// 开始唤醒。
        /// </summary>
        StartUp = 0,

        /// <summary>
        /// 关闭明日方舟
        /// </summary>
        CloseDown,

        /// <summary>
        /// 刷理智
        /// </summary>
        Fight,

        /// <summary>
        /// 领取奖励
        /// </summary>
        Award,

        /// <summary>
        /// 信用商店
        /// </summary>
        Mall,

        /// <summary>
        /// 基建
        /// </summary>
        Infrast,

        /// <summary>
        /// 招募
        /// </summary>
        Recruit,

        /// <summary>
        /// 肉鸽
        /// </summary>
        Roguelike,

        /// <summary>
        /// 自动战斗
        /// </summary>
        Copilot,

        /// <summary>
        /// 自动战斗-保全ver
        /// </summary>
        SSSCopilot,

        SingleStep,

        /// <summary>
        /// 视频识别
        /// </summary>
        VideoRecognition,

        /// <summary>
        /// 仓库识别
        /// </summary>
        Depot,

        /// <summary>
        /// 干员识别
        /// </summary>
        OperBox,

        /// <summary>
        /// 生息演算
        /// </summary>
        Reclamation,

        /// <summary>
        /// 自定义任务
        /// </summary>
        Custom,
    }
}
