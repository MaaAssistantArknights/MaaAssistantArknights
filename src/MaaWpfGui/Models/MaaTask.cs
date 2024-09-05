// <copyright file="MaaTask.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Text.Json.Serialization;
using MaaWpfGui.Configuration;
using MaaWpfGui.Configuration.MaaTask;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Models
{
    public class MaaTask
    {
        public class StartUpTask : BaseTask
        {
            public StartUpTask()
            {
                TaskType = TaskType.StartUp;
            }
        }

        public class CloseDownTask : BaseTask
        {
            public CloseDownTask()
            {
                TaskType = TaskType.CloseDown;
            }
        }

        public class FightTask : BaseTask
        {
            public FightTask()
            {
                TaskType = TaskType.Fight;
            }
        }

        public class InfrastTask : BaseTask
        {
            public InfrastTask()
            {
                TaskType = TaskType.Infrast;
            }
        }

        public class RecruitTask : BaseTask
        {
            public RecruitTask()
            {
                TaskType = TaskType.Recruit;
            }
        }

        public class RoguelikeTask : BaseTask
        {
            public RoguelikeTask()
            {
                TaskType = TaskType.Roguelike;
            }

            public string Theme { get; set; } = "Sami";
        }

        public class CopilotTask : BaseTask
        {
            public CopilotTask()
            {
                TaskType = TaskType.Copilot;
            }
        }

        public class SSSCopilotTask : BaseTask
        {
            public SSSCopilotTask()
            {
                TaskType = TaskType.SSSCopilot;
            }
        }

        public class SingleStepTask : BaseTask
        {
            public SingleStepTask()
            {
                TaskType = TaskType.SingleStep;
            }
        }

        public class VideoRecognition : BaseTask
        {
            public VideoRecognition()
            {
                TaskType = TaskType.VideoRecognition;
            }
        }

        public class DepotTask : BaseTask
        {
            public DepotTask()
            {
                TaskType = TaskType.Depot;
            }
        }

        public class OperBoxTask : BaseTask
        {
            public OperBoxTask()
            {
                TaskType = TaskType.OperBox;
            }
        }

        public class ReclamationTask : BaseTask
        {
            public ReclamationTask()
            {
                TaskType = TaskType.Reclamation;
            }
        }

        public class CustomTask : BaseTask
        {
            public CustomTask()
            {
                TaskType = TaskType.Custom;
            }
        }

        [JsonDerivedType(typeof(StartUpTask), typeDiscriminator: "StartUpTask")]
        [JsonDerivedType(typeof(CloseDownTask), typeDiscriminator: "CloseDownTask")]
        [JsonDerivedType(typeof(FightTask), typeDiscriminator: "FightTask")]
        [JsonDerivedType(typeof(AwardTask), typeDiscriminator: "AwardTask")]
        [JsonDerivedType(typeof(MallTask), typeDiscriminator: "MallTask")]
        [JsonDerivedType(typeof(InfrastTask), typeDiscriminator: "InfrastTask")]
        [JsonDerivedType(typeof(RecruitTask), typeDiscriminator: "RecruitTask")]
        [JsonDerivedType(typeof(RoguelikeTask), typeDiscriminator: "RoguelikeTask")]
        [JsonDerivedType(typeof(CopilotTask), typeDiscriminator: "CopilotTask")]
        [JsonDerivedType(typeof(SSSCopilotTask), typeDiscriminator: "SSSCopilotTask")]
        [JsonDerivedType(typeof(SingleStepTask), typeDiscriminator: "SingleStepTask")]
        [JsonDerivedType(typeof(VideoRecognition), typeDiscriminator: "VideoRecognition")]
        [JsonDerivedType(typeof(DepotTask), typeDiscriminator: "DepotTask")]
        [JsonDerivedType(typeof(OperBoxTask), typeDiscriminator: "OperBoxTask")]
        [JsonDerivedType(typeof(ReclamationTask), typeDiscriminator: "ReclamationTask")]
        [JsonDerivedType(typeof(CustomTask), typeDiscriminator: "CustomTask")]
        public class BaseTask : INotifyPropertyChanged
        {
            public event PropertyChangedEventHandler? PropertyChanged;

            public string Name { get; set; } = string.Empty;

            public bool? IsEnable { get; set; } = false;

            public TaskType TaskType { get; set; }

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
}
