// <copyright file="OperBoxAssistHelper.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.IO;
using System.Linq;
using MaaWpfGui.Constants;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 干员识别数据（DataDir/OperBoxData.json）的辅助编队可用性判定与变更通知。
/// 可用条件：数据 source 为 yituliu 且 own_opers 携带技能信息——本地识别与未导入练度的一图流账号数据都不满足。
/// 判定只在落盘完成后的明确事件点求值，不做文件系统监听，数据更新中途不产生中间态。
/// </summary>
public static class OperBoxAssistHelper
{
    private static readonly ILogger _logger = Log.ForContext("SourceContext", "OperBoxAssistHelper");

    private static readonly string _operBoxDataJsonPath =
        Path.Combine(PathsHelper.DataDir, $"{JsonDataKey.OperBoxData}.json");

    /// <summary>
    /// Gets 干员识别数据落盘文件路径，传给 Core 的 operbox_data_path 用同一份文件。
    /// </summary>
    public static string OperBoxDataJsonPath => _operBoxDataJsonPath;

    /// <summary>
    /// 数据落盘内容变化（覆盖写）后触发，订阅方应重新调用 <see cref="CheckData"/> 刷新状态。触发点均在 UI 线程。
    /// </summary>
    public static event Action? StateChanged;

    /// <summary>
    /// 通知订阅方数据已更新。仅在落盘完成后调用，禁止挂在数据写入链路中间。
    /// </summary>
    public static void RaiseStateChanged()
    {
        StateChanged?.Invoke();
    }

    /// <summary>
    /// 读取落盘数据并判定辅助编队可用性。文件缺失或解析失败均视为不可用。
    /// </summary>
    /// <returns>可用性与上次同步时间（无数据或格式无法解析时为 null）。</returns>
    public static (bool Usable, DateTimeOffset? SyncTime) CheckData()
    {
        try
        {
            if (!File.Exists(_operBoxDataJsonPath))
            {
                return (false, null);
            }

            var json = JObject.Parse(File.ReadAllText(_operBoxDataJsonPath));

            DateTimeOffset? syncTime = null;
            if (json["syncTime"]?.Value<string>() is { } syncTimeText
                && DateTimeOffset.TryParse(syncTimeText, out var parsed))
            {
                syncTime = parsed;
            }

            bool usable = json["source"]?.Value<string>() == "yituliu"
                && json["own_opers"] is JArray { Count: > 0 } ownOpers
                && ownOpers.Any(oper => oper["skills"] is JArray { Count: > 0 });
            return (usable, syncTime);
        }
        catch (Exception e)
        {
            _logger.Warning("Failed to check OperBox data usability: {Message}", e.Message);
            return (false, null);
        }
    }
}
