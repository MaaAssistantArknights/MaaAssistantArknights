// <copyright file="AsstPixelPaintTask.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using MaaWpfGui.Services;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models.AsstTasks;

/// <summary>
/// 像素画自动填色：短 task 入口 + params.pixel_paint 分组点列（不拼进 task 名）。
/// </summary>
public class AsstPixelPaintTask : AsstBaseTask
{
    public override AsstTaskType TaskType => AsstTaskType.Custom;

    [JsonProperty("task_names")]
    public List<string> CustomTasks { get; set; } = ["MiniGame@PixelPaint@Begin"];

    [JsonProperty("params")]
    public PixelPaintParams Params { get; set; } = new();

    public override (AsstTaskType TaskType, JObject Params) Serialize() => (TaskType, JObject.FromObject(this));

    public sealed class PixelPaintParams
    {
        [JsonProperty("pixel_paint")]
        public PixelPaintPayload PixelPaint { get; set; } = new();
    }

    public sealed class PixelPaintPayload
    {
        /// <summary>按色分组：color 为 0~39，points 为 [x,y]。</summary>
        [JsonProperty("groups")]
        public List<PixelPaintGroup> Groups { get; set; } = [];
    }

    public sealed class PixelPaintGroup
    {
        [JsonProperty("color")]
        public int Color { get; set; }

        [JsonProperty("points")]
        public List<int[]> Points { get; set; } = [];
    }
}
