// <copyright file="SideStoryActivity.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Runtime.Serialization;
using Newtonsoft.Json;
using Semver;

namespace MaaWpfGui.Models.MaaApi;

public class SideStoryActivity
{
    [JsonProperty("MinimumRequired")]
    private string VersionRequiredStr { get; set; } = string.Empty;

    [JsonIgnore]
    public SemVersion? MinVersionRequired { get; private set; }

    [JsonProperty("Activity")]
    public ActivityInfo Info { get; set; } = new ActivityInfo();

    [JsonProperty("Stages")]
    public List<Stage> StageList { get; set; } = [];

    public class ActivityInfo
    {
        [JsonProperty(nameof(StageName))]
        public string StageName { get; set; } = string.Empty;

        [JsonProperty(nameof(Tip))]
        public string Tip { get; set; } = string.Empty;

        [JsonProperty("UtcStartTime")]
        private string StartTimeStr { get; set; } = string.Empty; // 写作UTC, 实际为TimeZone时区的本地时间

        [JsonProperty("UtcExpireTime")]
        private string ExpireTimeStr { get; set; } = string.Empty; // 写作UTC, 实际为TimeZone时区的本地时间

        public DateTimeOffset StartTimeUtc { get; set; } = DateTimeOffset.MinValue;

        public DateTimeOffset ExpireTimeUtc { get; set; } = DateTimeOffset.MinValue;

        [JsonProperty(nameof(TimeZone))]
        public int TimeZone { get; set; }

        [OnDeserialized]
        internal void OnDeserializedMethod(StreamingContext context)
        {
            if (DateTime.TryParseExact(StartTimeStr, "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture, DateTimeStyles.None, out var startTime))
            {
                StartTimeUtc = new DateTimeOffset(startTime, TimeSpan.FromHours(TimeZone)).ToLocalTime();
            }
            if (DateTime.TryParseExact(ExpireTimeStr, "yyyy/MM/dd HH:mm:ss", CultureInfo.InvariantCulture, DateTimeStyles.None, out var expireTime))
            {
                ExpireTimeUtc = new DateTimeOffset(expireTime, TimeSpan.FromHours(TimeZone)).ToLocalTime();
            }
        }
    }

    public record class Stage([property: JsonProperty("Display")] string Display, [property: JsonProperty("Value")] string Value, [property: JsonProperty("Drop")] string Drop);

    [OnDeserialized]
    internal void OnDeserializedMethod(StreamingContext context)
    {
        if (SemVersion.TryParse(VersionRequiredStr, SemVersionStyles.AllowLowerV | SemVersionStyles.OptionalPatch, out var verOut))
        {
            MinVersionRequired = verOut;
        }
    }
}
