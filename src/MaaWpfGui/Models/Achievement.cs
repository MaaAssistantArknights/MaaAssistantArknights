// <copyright file="Achievement.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Windows;
using System.Windows.Media;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using static MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Models
{
    public class Achievement
    {
        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public string Id { get; set; } = string.Empty;

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool IsUnlocked { get; set; } = false;

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public DateTime? UnlockedTime { get; set; } = null;

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public int Progress { get; set; } = 0; // 用于累计型成就

        // 用法示例
        // 记录上次打开时间
        // achievement.CustomData["LastLaunchTime"] = DateTime.UtcNow;
        // 读取时
        // if (achievement.CustomData.TryGetValue("LastLaunchTime", out var token))
        // {
        //     var lastTime = token.ToObject<DateTime>();
        // }
        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public Dictionary<string, JToken>? CustomData { get; set; } = null;

        [JsonIgnore]
        public string Title => IsUnlocked ? LocalizationHelper.GetString($"Achievement.{Id}.Title") : "???";

        [JsonIgnore]
        public string Description => IsUnlocked ? LocalizationHelper.GetString($"Achievement.{Id}.Description") : "???";

        [JsonIgnore]
        public string Conditions => CanShow ? LocalizationHelper.GetString($"Achievement.{Id}.Conditions") : "???";

        [JsonIgnore]
        public DateTime? UnlockedTimeLocal => UnlockedTime?.ToLocalTime();

        [JsonIgnore]
        public bool IsHidden { get; set; } = false;

        [JsonIgnore]
        public bool CanShow => !IsHidden || IsUnlocked;

        [JsonIgnore]
        public bool IsProgressive => Target != default;

        [JsonIgnore]
        public int Target { get; set; } = default; // 可选目标值

        [JsonIgnore]
        public AchievementCategory Category { get; set; } // 分组

        [JsonIgnore]
        public bool IsRare { get; set; } = false;

        [JsonIgnore]
        public string MedalBrushKey
        {
            get
            {
                if (!IsUnlocked)
                {
                    return "LockedMedalBrush";
                }

                if (IsRare)
                {
                    return "AchievementBrush.Rare.LinearGradientBrush";
                }

                if (IsHidden)
                {
                    return "HiddenMedalBrush";
                }

                return $"AchievementBrush.{Category}";
            }
        }

        [JsonIgnore]
        public Brush MedalBrush
        {
            get
            {
                if (Application.Current.Resources[MedalBrushKey] is Brush brush)
                {
                    return brush;
                }

                return new SolidColorBrush(Colors.Transparent);
            }
        }

        // 新解锁的成就放前面，仅本次关闭前生效
        [JsonIgnore]
        public bool IsNewUnlock { get; set; } = false;
    }
}
