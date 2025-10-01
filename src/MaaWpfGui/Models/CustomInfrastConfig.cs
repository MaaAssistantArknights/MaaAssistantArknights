// <copyright file="CustomInfrastConfig.cs" company="MaaAssistantArknights">
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
using System.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models;

/// <summary>
/// 换班方案配置
/// </summary>
public class CustomInfrastConfig
{
    /// <summary>
    /// Gets or sets 作业名
    /// </summary>
    [JsonProperty("title")]
    public string? Title { get; set; }

    /// <summary>
    /// Gets or sets 作业描述
    /// </summary>
    [JsonProperty("description")]
    public string? Description { get; set; }

    /// <summary>
    /// Gets or sets 计划列表
    /// </summary>
    [JsonProperty("plans")]
    public List<Plan> Plans { get; set; } = [];

    /// <summary>
    /// 换班计划
    /// </summary>
    public class Plan
    {
        [JsonProperty("index")]
        public int Index { get; set; }

        /// <summary>
        /// Gets or sets 计划名
        /// </summary>
        [JsonProperty("name")]
        public string? Name { get; set; }

        /// <summary>
        /// Gets or sets 计划描述
        /// </summary>
        [JsonProperty("description")]
        public string? Description { get; set; }

        /// <summary>
        /// Gets or sets 计划执行完时显示的描述
        /// </summary>
        [JsonProperty("description_post")]
        public string? DescriptionPost { get; set; }

        /// <summary>
        /// Gets or sets 换班时间段
        /// </summary>
        [JsonProperty("period")]
        [JsonConverter(typeof(TimeOnlyArrayListConverter))]
        public List<TimeOnly[]> Period { get; set; } = [];
    }

    /// <summary>
    /// TimeOnly[] 列表的自定义转换器
    /// </summary>
    public class TimeOnlyArrayListConverter : JsonConverter<List<TimeOnly[]>>
    {
        public override void WriteJson(JsonWriter writer, List<TimeOnly[]>? value, JsonSerializer serializer)
        {
            if (value == null)
            {
                writer.WriteNull();
                return;
            }

            var stringArrays = value.Select(timeRange => new[] { timeRange[0].ToString("HH:mm"), timeRange[1].ToString("HH:mm") }).ToArray();

            serializer.Serialize(writer, stringArrays);
        }

        public override List<TimeOnly[]> ReadJson(JsonReader reader, Type objectType, List<TimeOnly[]>? existingValue, bool hasExistingValue, JsonSerializer serializer)
        {
            var token = JToken.Load(reader);

            if (token.Type == JTokenType.Null)
            {
                return [];
            }

            var stringArrays = token.ToObject<string[][]>();
            return stringArrays == null
                ? []
                : stringArrays.Select(stringArray => new[] { TimeOnly.Parse(stringArray[0]), TimeOnly.Parse(stringArray[1]) }).ToList();
        }
    }
}
