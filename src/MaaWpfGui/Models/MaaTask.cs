// <copyright file="MaaTask.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System.Text.Json.Serialization;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Models
{
    public class MaaTask
    {
        public class StartUpTask : BaseTask
        {
            public StartUpTask()
            {
                TaskType = TaskTypeEnum.StartUp;
            }
        }

        public class CloseDownTask : BaseTask
        {
            public CloseDownTask()
            {
                TaskType = TaskTypeEnum.CloseDown;
            }
        }

        public class FightTask : BaseTask
        {
            public FightTask()
            {
                TaskType = TaskTypeEnum.Fight;
            }
        }

        public class AwardTask : BaseTask
        {
            public AwardTask()
            {
                TaskType = TaskTypeEnum.Award;
            }

            public bool Award { get; set; } = true;

            public bool Mail { get; set; }
        }

        public class MallTask : BaseTask
        {
            public MallTask()
            {
                TaskType = TaskTypeEnum.Mall;
            }

            public bool Shopping { get; set; } = true;

            public bool ShoppingWhenCreditFull { get; set; }

            public string WhiteList { get; set; } = LocalizationHelper.GetString("HighPriorityDefault");

            public string BlackList { get; set; } = LocalizationHelper.GetString("BlacklistDefault");
        }

        public class InfrastTask : BaseTask
        {
            public InfrastTask()
            {
                TaskType = TaskTypeEnum.Infrast;
            }
        }

        public class RecruitTask : BaseTask
        {
            public RecruitTask()
            {
                TaskType = TaskTypeEnum.Recruit;
            }
        }

        public class RoguelikeTask : BaseTask
        {
            public RoguelikeTask()
            {
                TaskType = TaskTypeEnum.Roguelike;
            }

            public string Theme { get; set; } = "Sami";
        }

        public class CopilotTask : BaseTask
        {
            public CopilotTask()
            {
                TaskType = TaskTypeEnum.Copilot;
            }
        }

        public class SSSCopilotTask : BaseTask
        {
            public SSSCopilotTask()
            {
                TaskType = TaskTypeEnum.SSSCopilot;
            }
        }

        public class SingleStepTask : BaseTask
        {
            public SingleStepTask()
            {
                TaskType = TaskTypeEnum.SingleStep;
            }
        }

        public class VideoRecognition : BaseTask
        {
            public VideoRecognition()
            {
                TaskType = TaskTypeEnum.VideoRecognition;
            }
        }

        public class DepotTask : BaseTask
        {
            public DepotTask()
            {
                TaskType = TaskTypeEnum.Depot;
            }
        }

        public class OperBoxTask : BaseTask
        {
            public OperBoxTask()
            {
                TaskType = TaskTypeEnum.OperBox;
            }
        }

        public class ReclamationTask : BaseTask
        {
            public ReclamationTask()
            {
                TaskType = TaskTypeEnum.Reclamation;
            }
        }

        public class CustomTask : BaseTask
        {
            public CustomTask()
            {
                TaskType = TaskTypeEnum.Custom;
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
        public class BaseTask
        {
            public string Name { get; set; } = string.Empty;

            [JsonInclude]
            public bool IsChecked { get; set; } = false;

            [JsonInclude]
            public TaskTypeEnum TaskType { get; set; }
        }

        public enum TaskTypeEnum
        {
            /// <summary>
            /// 开始唤醒。
            /// </summary>
            StartUp = 0,

            CloseDown,

            /// <summary>
            /// 刷理智。
            /// </summary>
            Fight,
            Award,
            Mall,
            Infrast,
            Recruit,
            Roguelike,
            Copilot,
            SSSCopilot,
            SingleStep,
            VideoRecognition,
            Depot,
            OperBox,
            Reclamation,
            Custom,
        }
    }
}
