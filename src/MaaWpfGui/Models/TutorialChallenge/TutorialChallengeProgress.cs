// <copyright file="TutorialChallengeProgress.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Text.Json.Serialization;
using MaaWpfGui.Helper;
using Serilog;
using static MaaWpfGui.Helper.PathsHelper;

namespace MaaWpfGui.Models.TutorialChallenge;

/// <summary>
/// 教程关卡进度存档。独立于主配置单独成文件（config/tutorial_challenge.json），
/// 记录每关通关标记；牛牛币数由已通关关数推导，不单独落盘。
/// </summary>
public static class TutorialChallengeProgress
{
    private static readonly ILogger _logger = Log.ForContext(typeof(TutorialChallengeProgress));
    private static readonly string _filePath = Path.Combine(ConfigDir, "tutorial_challenge.json");
    private static readonly JsonSerializerOptions _options = new() { WriteIndented = true };

    private static HashSet<string> _clearedLevels = [];

    /// <summary>
    /// 通关标记变更时触发，用于列表页与关卡窗口同步刷新。
    /// </summary>
    public static event EventHandler? Changed;

    /// <summary>
    /// Gets 牛牛币当前总数，等于已通关的关卡数。
    /// </summary>
    public static int CoinCount => _clearedLevels.Count;

    /// <summary>
    /// 判断指定关卡是否已通关。
    /// </summary>
    /// <param name="levelId">关卡 Id。</param>
    /// <returns>已通关返回 true。</returns>
    public static bool IsCleared(string levelId) => _clearedLevels.Contains(levelId);

    /// <summary>
    /// 加载进度文件；文件缺失或损坏按空进度处理。
    /// </summary>
    public static void Load()
    {
        _clearedLevels = [];

        if (!File.Exists(_filePath))
        {
            return;
        }

        try
        {
            var data = JsonSerializer.Deserialize<ProgressData>(File.ReadAllText(_filePath), _options);
            if (data?.ClearedLevels is { } list)
            {
                _clearedLevels = [.. list];
            }
        }
        catch (Exception e)
        {
            _logger.Warning(e, "Failed to load tutorial challenge progress, starting fresh");
        }
    }

    /// <summary>
    /// 标记关卡已通关并写盘；已在集合内则跳过。
    /// </summary>
    /// <param name="levelId">关卡 Id。</param>
    public static void MarkCleared(string levelId)
    {
        if (!_clearedLevels.Add(levelId))
        {
            return;
        }

        Save();
        Changed?.Invoke(null, EventArgs.Empty);
    }

    private static void Save()
    {
        try
        {
            Directory.CreateDirectory(ConfigDir);
            var data = new ProgressData { ClearedLevels = [.. _clearedLevels] };
            File.WriteAllText(_filePath, JsonSerializer.Serialize(data, _options));
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to save tutorial challenge progress");
        }
    }

    private class ProgressData
    {
        [JsonPropertyName("clearedLevels")]
        public List<string> ClearedLevels { get; set; } = [];
    }
}
