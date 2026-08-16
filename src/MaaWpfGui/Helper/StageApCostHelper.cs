// <copyright file="StageApCostHelper.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Threading;
using Newtonsoft.Json.Linq;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// 关卡理智消耗（apCost）查询，数据来自 resource/stages.json。
/// 资源更新会直接覆盖该文件且进程不重启，故按文件时间戳检测变化并自动重建缓存。
/// </summary>
public static class StageApCostHelper
{
    private static readonly ILogger _logger = Log.ForContext(typeof(StageApCostHelper));

    /// <summary>
    /// 保护 <see cref="_apCosts"/> 与 <see cref="_stamp"/> 这对相关字段的检查与重建；
    /// 调用频率为每个战斗任务一次，加锁开销可忽略。
    /// </summary>
    private static readonly Lock _lock = new();

    private static Dictionary<string, int>? _apCosts;

    /// <summary>
    /// 已加载数据对应的 stages.json 时间戳，用于检测资源更新并重建缓存。
    /// </summary>
    private static DateTime _stamp;

    /// <summary>
    /// 获取关卡单次进图的理智消耗；关卡不在数据中（如手输活动关、复刻前缀）时返回 null。
    /// </summary>
    /// <param name="stage">关卡编号。</param>
    /// <returns>理智消耗，未知返回 null。</returns>
    public static int? GetApCost(string stage)
    {
        if (string.IsNullOrEmpty(stage))
        {
            return null;
        }

        lock (_lock)
        {
            var path = Path.Combine(PathsHelper.ResourceDir, "stages.json");
            var stamp = File.Exists(path) ? File.GetLastWriteTimeUtc(path) : DateTime.MinValue;
            if (_apCosts == null || stamp != _stamp)
            {
                _stamp = stamp;
                _apCosts = Load(path);
            }

            return _apCosts.TryGetValue(stage, out var cost) ? cost : null;
        }
    }

    private static Dictionary<string, int> Load(string path)
    {
        var result = new Dictionary<string, int>();
        try
        {
            if (!File.Exists(path))
            {
                _logger.Warning("StageApCostHelper: stages.json not found at {Path}", path);
                return result;
            }

            foreach (var item in JArray.Parse(File.ReadAllText(path)))
            {
                var code = item["code"]?.ToString();
                var apCost = item["apCost"]?.ToObject<int>();
                if (!string.IsNullOrEmpty(code) && apCost is > 0)
                {
                    result[code] = apCost.Value;
                }
            }

            _logger.Information("StageApCostHelper: loaded apCost of {Count} stages", result.Count);
        }
        catch (Exception e)
        {
            _logger.Error(e, "StageApCostHelper: failed to load stages.json");
        }

        return result;
    }
}
