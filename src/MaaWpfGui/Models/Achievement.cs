// <copyright file="AchievementTrackerHelper.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using Newtonsoft.Json;

namespace MaaWpfGui.Models
{
    public class Achievement
    {
        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public string Id { get; set; } = string.Empty;

        [JsonIgnore]
        public string Title => IsUnlocked ? LocalizationHelper.GetString($"Achievement.{Id}.Title") : "???";

        [JsonIgnore]
        public string Description => IsUnlocked ? LocalizationHelper.GetString($"Achievement.{Id}.Description") : "???";

        [JsonIgnore]
        public string Conditions => CanShow ? LocalizationHelper.GetString($"Achievement.{Id}.Conditions") : "???";

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool IsUnlocked { get; set; } = false;

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public DateTime? UnlockedTime { get; set; } = null;

        [JsonIgnore]
        public DateTime? UnlockedTimeLocal => UnlockedTime?.ToLocalTime();

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public bool IsHidden { get; set; } = false;

        [JsonIgnore]
        public bool CanShow => !IsHidden || IsUnlocked;

        [JsonIgnore]
        public bool IsProgressive => Target != null;

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public int? Target { get; set; } = null; // 可选目标值

        [JsonProperty(DefaultValueHandling = DefaultValueHandling.Ignore)]
        public int Progress { get; set; } = 0; // 用于累计型成就
    }
}
