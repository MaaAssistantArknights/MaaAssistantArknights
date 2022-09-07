// <copyright file="StageManager.cs" company="MaaAssistantArknights">
// MeoAsstGui - A part of the MeoAssistantArknights project
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MeoAsstGui
{
    /// <summary>
    /// Stage manager
    /// </summary>
    public class StageManager
    {
        private readonly Dictionary<string, StageInfo> _stages;
        private readonly List<StageActivityInfo> _activities;

        /// <summary>
        /// Initializes a new instance of the <see cref="StageManager"/> class.
        /// </summary>
        public StageManager()
        {
            // 故事集 / SideStory
            var activity = new StageActivityInfo()
            {
                ClientType = "Official",
                Tip = "故事集「日暮寻路」",
                UtcStartTime = new DateTimeOffset(2022, 9, 8, 16, 0, 0, TimeSpan.FromHours(8)),
                UtcExpireTime = new DateTimeOffset(2022, 9, 15, 4, 0, 0, TimeSpan.FromHours(8)),
                IsResourceCollection = false,
            };

            // 资源收集
            /*
            var resourceCollection = new StageActivityInfo()
            {
                ClientType = "Official",
                Tip = "资源收集全天开放",
                UtcStartTime = new DateTimeOffset(2022, 8, 25, 16, 0, 0, TimeSpan.FromHours(8)),
                UtcExpireTime = new DateTimeOffset(2022, 9, 8, 4, 0, 0, TimeSpan.FromHours(8)),
                IsResourceCollection = true,
            };
            */

            _activities = new List<StageActivityInfo>() { activity };

            _stages = new Dictionary<string, StageInfo>
            {
                // SideStory「理想城：长夏狂欢季」活动
                // { "IC-9", new StageInfo { Display = "IC-9", Value = "IC-9", Activity = activity } },
                // { "IC-8", new StageInfo { Display = "IC-8", Value = "IC-8", Activity = activity } },
                // { "IC-7", new StageInfo { Display = "IC-7", Value = "IC-7", Activity = activity } },

                // 「当前/上次」关卡导航
                { string.Empty, new StageInfo { Display = Localization.GetString("DefaultStage"), Value = string.Empty } },

                // 主线关卡
                { "1-7", new StageInfo { Display = "1-7", Value = "1-7" } },

                // 资源本
                { "CE-6", new StageInfo("CE-6", "CETip", new[] { DayOfWeek.Tuesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }) },
                { "AP-5", new StageInfo("AP-5", "APTip", new[] { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }) },
                { "CA-5", new StageInfo("CA-5", "CATip", new[] { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Sunday }) },
                { "LS-6", new StageInfo("LS-6", "LSTip") },

                // 碳本没做导航，只显示关卡提示
                { "SK-5", new StageInfo("SK-5", "SKTip", new[] { DayOfWeek.Monday, DayOfWeek.Wednesday, DayOfWeek.Friday, DayOfWeek.Saturday }) { IsHidden = true } },

                // 剿灭模式
                { "Annihilation", new StageInfo { Display = Localization.GetString("Annihilation"), Value = "Annihilation" } },

                // 芯片本
                { "PR-A-1", new StageInfo("PR-A-1", "PR-ATip", new[] { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday }) },
                { "PR-A-2", new StageInfo("PR-A-2", string.Empty, new[] { DayOfWeek.Monday, DayOfWeek.Thursday, DayOfWeek.Friday, DayOfWeek.Sunday }) },
                { "PR-B-1", new StageInfo("PR-B-1", "PR-BTip", new[] { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday }) },
                { "PR-B-2", new StageInfo("PR-B-2", string.Empty, new[] { DayOfWeek.Monday, DayOfWeek.Tuesday, DayOfWeek.Friday, DayOfWeek.Saturday }) },
                { "PR-C-1", new StageInfo("PR-C-1", "PR-CTip", new[] { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }) },
                { "PR-C-2", new StageInfo("PR-C-2", string.Empty, new[] { DayOfWeek.Wednesday, DayOfWeek.Thursday, DayOfWeek.Saturday, DayOfWeek.Sunday }) },
                { "PR-D-1", new StageInfo("PR-D-1", "PR-DTip", new[] { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday }) },
                { "PR-D-2", new StageInfo("PR-D-2", string.Empty, new[] { DayOfWeek.Tuesday, DayOfWeek.Wednesday, DayOfWeek.Saturday, DayOfWeek.Sunday }) },

                // 周一和周日的关卡提示
                { "Pormpt1", new StageInfo { Tip = Localization.GetString("Pormpt1"), OpenDays = new[] { DayOfWeek.Monday }, IsHidden = true } },
                { "Pormpt2", new StageInfo { Tip = Localization.GetString("Pormpt2"), OpenDays = new[] { DayOfWeek.Sunday }, IsHidden = true } },

                // 老版本「当前/上次」关卡导航
                // new StageInfo { Display = Localization.GetString("CurrentStage"), Value = string.Empty },
                // new StageInfo { Display = Localization.GetString("LastBattle"), Value = "LastBattle" },
            };
        }

        /// <summary>
        /// Gets stage by name
        /// </summary>
        /// <param name="stage">Stage name</param>
        /// <returns>Stage info</returns>
        public StageInfo GetStageInfo(string stage)
        {
            _stages.TryGetValue(stage, out var stageInfo);
            return stageInfo;
        }

        /// <summary>
        /// Determine whether stage is open
        /// </summary>
        /// <param name="stage">Stage name</param>
        /// <param name="dayOfWeek">Current day of week</param>
        /// <returns>Whether stage is open</returns>
        public bool IsStageOpen(string stage, DayOfWeek dayOfWeek)
        {
            return GetStageInfo(stage)?.IsStageOpen(dayOfWeek) == true;
        }

        /// <summary>
        /// Gets open stage tips at specified day of week
        /// </summary>
        /// <param name="dayOfWeek">Day of week</param>
        /// <param name="clientType">Client type</param>
        /// <returns>Open stages</returns>
        public string GetStageTips(DayOfWeek dayOfWeek, string clientType)
        {
            // client type not set, assume official
            if (string.IsNullOrEmpty(clientType))
            {
                clientType = "Official";
            }

            var builder = new StringBuilder();

            bool duringRcActivity = false;

            // show activity period
            foreach (var activity in _activities.Where(act => act.ClientType == clientType))
            {
                if (!duringRcActivity && activity.IsOpen && activity.IsResourceCollection)
                {
                    duringRcActivity = true;
                }

                if (!activity.IsExpired)
                {
                    builder.AppendLine($"{activity.Tip.PadRight(10)} {activity.UtcStartTime:M/d H:00} - {activity.UtcExpireTime:M/d H:00}");
                }
            }

            // show available stages today
            if (!duringRcActivity)
            {
                var stages = _stages.Where(pair => pair.Value.OpenDays?.Any() == true);
                foreach (var item in stages)
                {
                    if (item.Value.OpenDays.Contains(dayOfWeek) && !string.IsNullOrEmpty(item.Value.Tip))
                    {
                        builder.AppendLine(item.Value.Tip);
                    }
                }
            }

            return builder.ToString();
        }

        /// <summary>
        /// Gets open stage list at specified day of week
        /// </summary>
        /// <param name="dayOfWeek">Day of week</param>
        /// <returns>Open stage list</returns>
        public IEnumerable<CombData> GetStageList(DayOfWeek dayOfWeek)
        {
            return _stages.Values.Where(stage => !stage.IsHidden && stage.IsStageOpen(dayOfWeek));
        }

        /// <summary>
        /// Gets all stage list
        /// </summary>
        /// <returns>All stage list</returns>
        public IEnumerable<CombData> GetStageList()
        {
            return _stages.Values.Where(stage => !stage.IsHidden);
        }
    }
}
