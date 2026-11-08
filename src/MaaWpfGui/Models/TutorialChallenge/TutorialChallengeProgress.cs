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
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;

namespace MaaWpfGui.Models.TutorialChallenge;

/// <summary>
/// 教程关卡进度存档，经 <see cref="JsonDataHelper"/> 存于 data/TutorialChallengeProgress.json，
/// 记录每关通关标记；牛牛币数由已通关关卡的难度值求和推导，不单独落盘。
/// </summary>
public static class TutorialChallengeProgress
{
    private static HashSet<string> _clearedLevels = [];

    /// <summary>
    /// 通关标记变更时触发，用于列表页与关卡窗口同步刷新。
    /// </summary>
    public static event EventHandler? Changed;

    /// <summary>
    /// Gets 牛牛币当前总数，等于已通关关卡的难度值之和。
    /// </summary>
    public static int CoinCount => TutorialChallengeLevels.All
        .Where(level => _clearedLevels.Contains(level.Id))
        .Sum(level => (int)level.Difficulty);

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

        var data = JsonDataHelper.Get<ProgressData>(JsonDataKey.TutorialChallengeProgress);
        if (data?.ClearedLevels is { } list)
        {
            // 只认已注册的关卡，关卡下线或文件被手改时牛牛币不会多于关卡总数
            var registered = TutorialChallengeLevels.All.Select(level => level.Id).ToHashSet();
            _clearedLevels = [.. list.Where(registered.Contains)];
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
        var data = new ProgressData { ClearedLevels = [.. _clearedLevels] };
        JsonDataHelper.Set(JsonDataKey.TutorialChallengeProgress, data);
    }

    private class ProgressData
    {
        [JsonProperty("clearedLevels")]
        public List<string> ClearedLevels { get; set; } = [];
    }
}
