// <copyright file="DemoShotData.cs" company="MaaAssistantArknights">
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
using System.IO;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Main.DemoShot;

/// <summary>
/// README 截图演示模式的数据文件模型。
/// 所有字段均可缺省，加载失败仅在数据文件损坏时发生，字段缺失时使用默认值。
/// </summary>
public class DemoShotData
{
    private static readonly ILogger _logger = Log.ForContext<DemoShotData>();

    /// <summary>
    /// Gets or sets 各界面语言使用的客户端类型名（key 形如 zh-cn，value 为 ClientType 枚举名，忽略大小写）。
    /// </summary>
    [JsonProperty("clientType")]
    public Dictionary<string, string> ClientType { get; set; } = [];

    /// <summary>
    /// Gets or sets 仓库识别数据，原样透传
    /// <see cref="MaaWpfGui.ViewModels.UI.ToolboxViewModel.DepotParse(Newtonsoft.Json.Linq.JObject, bool, int)"/>，
    /// 结构为 {"done":true,"data":{"itemId":count,...},"syncTime":"ISO8601"}。
    /// </summary>
    [JsonProperty("depot")]
    public JObject? Depot { get; set; }

    /// <summary>
    /// Gets or sets 干员识别数据，原样透传
    /// <see cref="MaaWpfGui.ViewModels.UI.ToolboxViewModel.OperBoxParse(Newtonsoft.Json.Linq.JObject, bool, int)"/>，
    /// 结构为 {"done":true,"own_opers":[{"id","name","own","elite","level","potential","rarity"},...],"syncTime":"ISO8601"}；
    /// 缺省时由 <see cref="DemoShotService"/> 从 battle_data 干员全集生成全干员满练度数据注入。
    /// </summary>
    [JsonProperty("operBox")]
    public JObject? OperBox { get; set; }

    /// <summary>
    /// Gets or sets 窗口标题版本段覆盖，null 表示字段缺省走原行为（显示真实构建版本）。
    /// </summary>
    [JsonProperty("windowTitle")]
    public DemoWindowTitleData? WindowTitle { get; set; }

    /// <summary>
    /// Gets or sets 长草页任务勾选与候选关卡配置。
    /// </summary>
    [JsonProperty("taskQueue")]
    public DemoTaskQueueData TaskQueue { get; set; } = new();

    /// <summary>
    /// Gets or sets 长草页日志条目。
    /// </summary>
    [JsonProperty("taskQueueLogs")]
    public List<DemoLogEntry> TaskQueueLogs { get; set; } = [];

    /// <summary>
    /// Gets or sets 日志卡片缩略图路径列表（相对演示数据文件所在目录解析），供
    /// <see cref="DemoLogEntry.Thumbnail"/> 按索引引用；5 语言共用同一组图片。
    /// </summary>
    [JsonProperty("thumbnails")]
    public List<string> Thumbnails { get; set; } = [];

    /// <summary>
    /// Gets or sets 自动战斗页配置与日志。
    /// </summary>
    [JsonProperty("copilot")]
    public DemoCopilotData Copilot { get; set; } = new();

    /// <summary>
    /// 从文件加载演示数据；文件不存在或 JSON 损坏时记日志并返回 null。
    /// </summary>
    /// <param name="path">演示数据 JSON 路径。</param>
    /// <returns>演示数据；加载失败时为 null。</returns>
    public static DemoShotData? LoadFromFile(string path)
    {
        if (!File.Exists(path))
        {
            _logger.Error("Demo data file not found: {Path}", path);
            return null;
        }

        try
        {
            var data = JsonConvert.DeserializeObject<DemoShotData>(File.ReadAllText(path));
            if (data == null)
            {
                _logger.Error("Demo data file is empty: {Path}", path);
                return null;
            }

            _logger.Information(
                "Demo data loaded: {TaskLogs} task logs, {CopilotLogs} copilot logs, {Depot} depot, {OperBox} operBox",
                data.TaskQueueLogs.Count,
                data.Copilot.Logs.Count,
                data.Depot != null,
                data.OperBox != null);
            return data;
        }
        catch (JsonException e)
        {
            _logger.Error(e, "Failed to parse demo data file: {Path}", path);
            return null;
        }
    }
}

#pragma warning disable SA1402 // File may only contain a single type

/// <summary>
/// 窗口标题版本段覆盖。
/// </summary>
public class DemoWindowTitleData
{
    /// <summary>
    /// Gets or sets UI 版本段文本；null 或 <c>latest</c> 表示动态取 GitHub 最新 Release 的 tag_name
    /// （联网失败静默回退原行为，显示真实构建版本），其他非空值原样替换（消除 DEBUG_VERSION 等）。
    /// </summary>
    [JsonProperty("version")]
    public string? Version { get; set; }

    /// <summary>
    /// Gets or sets 资源版本段文本；null 走原行为，空串隐藏整段，非空则原样替换（消除 culture 日期格式差异）。
    /// </summary>
    [JsonProperty("resourceVersion")]
    public string? ResourceVersion { get; set; }
}

/// <summary>
/// 长草页任务勾选与候选关卡配置。
/// </summary>
public class DemoTaskQueueData
{
    /// <summary>
    /// Gets or sets 任务列表的完整有序序列，顺序即长草页任务列的显示顺序；
    /// 任务列整体替换为该序列，未列出的任务类型不会出现。
    /// </summary>
    [JsonProperty("tasks")]
    public List<DemoTaskEntry> Tasks { get; set; } = [];

    /// <summary>
    /// Gets or sets 候选关卡显示字符串（如 PA-8），全语言共享（关卡代号语言无关，干员/材料名才由 GUI 查表本地化）。
    /// </summary>
    [JsonProperty("stages")]
    public List<string> Stages { get; set; } = [];
}

/// <summary>
/// 任务列表单个条目。
/// </summary>
public class DemoTaskEntry
{
    /// <summary>
    /// Gets or sets 任务类型 key（与 <see cref="Main.AsstProxy.TaskType"/> 枚举名一致，忽略大小写，如 StartUp）。
    /// </summary>
    [JsonProperty("type")]
    public string Type { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 是否勾选该任务。
    /// </summary>
    [JsonProperty("enabled")]
    public bool Enabled { get; set; } = true;

    /// <summary>
    /// Gets or sets 条目状态展示（<c>idle</c> 未开始 / <c>inProgress</c> 进行中 / <c>completed</c> 已完成），
    /// 缺省 idle；仅影响截图观感，演示模式无真实任务回调。
    /// </summary>
    [JsonProperty("status")]
    public string Status { get; set; } = "idle";
}

/// <summary>
/// 长草页日志条目。
/// </summary>
public class DemoLogEntry
{
    /// <summary>
    /// Gets or sets 展示时间（如 "20:21:47"），空串表示保留注入时的真实时间。
    /// </summary>
    [JsonProperty("time")]
    public string Time { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 颜色，直接使用 UiLogColor 常量名（如 Trace/Message/Success）。
    /// </summary>
    [JsonProperty("color")]
    public string Color { get; set; } = nameof(MaaWpfGui.Constants.UiLogColor.Trace);

    /// <summary>
    /// Gets or sets 字重（Regular/Bold）。
    /// </summary>
    [JsonProperty("weight")]
    public string Weight { get; set; } = "Regular";

    /// <summary>
    /// Gets or sets 日志卡片拆分方式（None/Before/After/Both）。
    /// </summary>
    [JsonProperty("split")]
    public string Split { get; set; } = "None";

    /// <summary>
    /// Gets or sets 该条目所在日志卡片引用的缩略图索引（指向
    /// <see cref="DemoShotData.Thumbnails"/>，0-based）；null 表示无缩略图。
    /// 卡片按首条日志条目的索引取图，同卡片后续条目的该字段无效。
    /// </summary>
    [JsonProperty("thumbnail")]
    public int? Thumbnail { get; set; }

    /// <summary>
    /// Gets or sets 各语言文案，缺语言时回退 zh-cn。
    /// </summary>
    [JsonProperty("text")]
    public Dictionary<string, string> Text { get; set; } = [];
}

/// <summary>
/// 自动战斗页配置与日志。
/// </summary>
public class DemoCopilotData
{
    /// <summary>
    /// Gets or sets 作业类型 Tab 索引。
    /// </summary>
    [JsonProperty("tabIndex")]
    public int TabIndex { get; set; }

    /// <summary>
    /// Gets or sets 作业站 ID 列表（非空时作业列表逐项展示 maa://{id}，顶部输入框展示首个，演示模式不发起网络请求）。
    /// </summary>
    [JsonProperty("copilotIds")]
    public List<int> CopilotIds { get; set; } = [];

    /// <summary>
    /// Gets or sets 无时间戳的头部行（干员组等）。
    /// </summary>
    [JsonProperty("headerLines")]
    public List<DemoCopilotLine> HeaderLines { get; set; } = [];

    /// <summary>
    /// Gets or sets 带时间戳的日志条目。
    /// </summary>
    [JsonProperty("logs")]
    public List<DemoCopilotLog> Logs { get; set; } = [];
}

/// <summary>
/// 自动战斗页无时间戳头部行。
/// </summary>
public class DemoCopilotLine
{
    /// <summary>
    /// Gets or sets 颜色，直接使用 UiLogColor 常量名。
    /// </summary>
    [JsonProperty("color")]
    public string Color { get; set; } = nameof(MaaWpfGui.Constants.UiLogColor.Info);

    /// <summary>
    /// Gets or sets 字重（Regular/Bold）。
    /// </summary>
    [JsonProperty("weight")]
    public string Weight { get; set; } = "Bold";

    /// <summary>
    /// Gets or sets 各语言文案，缺语言时回退 zh-cn。
    /// </summary>
    [JsonProperty("text")]
    public Dictionary<string, string> Text { get; set; } = [];
}

/// <summary>
/// 自动战斗页带时间戳日志条目。
/// </summary>
public class DemoCopilotLog
{
    /// <summary>
    /// Gets or sets 展示时间（如 "20:45:13"），空串表示保留注入时的真实时间。
    /// </summary>
    [JsonProperty("time")]
    public string Time { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets 颜色，直接使用 UiLogColor 常量名。
    /// </summary>
    [JsonProperty("color")]
    public string Color { get; set; } = nameof(MaaWpfGui.Constants.UiLogColor.Trace);

    /// <summary>
    /// Gets or sets 字重（Regular/Bold）。
    /// </summary>
    [JsonProperty("weight")]
    public string Weight { get; set; } = "Regular";

    /// <summary>
    /// Gets or sets 各语言文案，缺语言时回退 zh-cn。
    /// </summary>
    [JsonProperty("text")]
    public Dictionary<string, string> Text { get; set; } = [];
}
