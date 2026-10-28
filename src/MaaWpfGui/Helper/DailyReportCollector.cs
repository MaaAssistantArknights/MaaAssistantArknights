// <copyright file="DailyReportCollector.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.DailyReport;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Helper;

/// <summary>
/// 作战日报采集器：把 Fight 链的执行期回调（掉落、用药、碎石、次数）聚合为
/// <see cref="CombatSession"/>，回调即写当日文件。
/// 一条 Fight 链（独立理智作战或库存保持的单条计划）通常对应一条 session，别传复刻一条链打多关时按关卡拆成多条；
/// 零场次的链不产生记录，session 在首个执行数据到达时才落盘。
/// 回调均由 AsstProxy 投递至 UI 线程，内部状态无需加锁。
/// </summary>
public static class DailyReportCollector
{
    private static readonly Dictionary<int, CombatSession> _pendingSessions = [];
    private static readonly HashSet<int> _persistedTaskIds = [];

    // 每局开始前 FightTimes 读到的代理倍率与理智消耗（已含倍率），供该局结算时累加
    private static readonly Dictionary<int, (int Series, int SanityCost)> _pendingRounds = [];
    private static DateOnly _currentDay;
    private static DailyReportDayData _currentData = new();

    // 当日文件无法读取且未能备份时置 false，暂停写入以免覆盖原文件；下次切换游戏日时重新判断
    private static bool _currentWritable = true;

    /// <summary>
    /// 订阅 AsstProxy 回调；在 AsstProxy 实例就绪后调用一次。
    /// </summary>
    /// <param name="proxy">AsstProxy 实例。</param>
    public static void Initialize(AsstProxy proxy)
    {
        proxy.AsstSubTaskMsgEvent += OnSubTaskMsg;
        proxy.OnTaskStatusChanged += OnTaskStatusChanged;
        PruneNow();
    }

    /// <summary>
    /// 任务队列停止（手动停止、掉线、强制定时启动统一走 TaskChainStopped）时，
    /// 把所有未定稿 session 补结束时刻定稿。
    /// </summary>
    public static void OnQueueStopped()
    {
        foreach (var taskId in _pendingSessions.Keys.ToList())
        {
            FinalizeSession(taskId, DateTimeOffset.UtcNow);
        }
    }

    /// <summary>
    /// 按当前保留档位裁剪超期日文件。
    /// </summary>
    public static void PruneNow()
    {
        DailyReportStore.Prune(Configuration.Factory.ConfigFactory.Root.Gui.DailyReportRetention);
    }

    private static void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        switch (status)
        {
            case TaskItemStatus.InProgress:
                {
                    // 只采集 Fight 链；仓库识别等其余链不产生作战记录
                    if (Instances.AsstProxy.TasksStatus.TryGetValue(taskId, out var info) && info.Type == AsstProxy.TaskType.Fight)
                    {
                        _pendingSessions[taskId] = new CombatSession { StartTime = DateTimeOffset.UtcNow };
                    }

                    break;
                }

            case TaskItemStatus.Completed:
            case TaskItemStatus.Error:
                FinalizeSession(taskId, DateTimeOffset.UtcNow);
                break;
        }
    }

    private static void OnSubTaskMsg(AsstMsg type, AsstSubTaskMsg? msg)
    {
        if (msg is null || !_pendingSessions.TryGetValue(msg.TaskId, out var session))
        {
            return;
        }

        switch (type)
        {
            case AsstMsg.SubTaskExtraInfo:
                switch (msg.What)
                {
                    case "StageDrops":
                        UpdateFromStageDrops(msg.TaskId, session, msg.Details);
                        break;

                    case "UseMedicine":
                        {
                            var isExpiring = msg.Details?.Value<bool?>("is_expiring") ?? false;
                            var count = msg.Details?.Value<int?>("count") ?? 0;
                            if (isExpiring)
                            {
                                session.ExpiringMedicinesUsed += count;
                            }
                            else
                            {
                                session.MedicinesUsed += count;
                            }

                            PersistIfNeeded(msg.TaskId, session);
                            break;
                        }

                    case "FightTimes":
                        // 只暂存本局倍率与消耗，等该局 StageDrops 结算再计入；达到次数上限时不再开局，不会有对应结算
                        if (msg.Details?.Value<int?>("sanity_cost") is { } sanityCost)
                        {
                            _pendingRounds[msg.TaskId] = (msg.Details.Value<int?>("series") ?? 1, sanityCost);
                        }

                        break;
                }

                break;

            // 碎石确认以通用 ProcessTask 子任务上报，具体任务名在 details.task
            case AsstMsg.SubTaskStart when msg.SubTask == "ProcessTask" && (msg.Details?["task"]?.ToString() == "StoneConfirm"):
                session.StonesUsed++;
                PersistIfNeeded(msg.TaskId, session);
                break;
        }
    }

    private static void UpdateFromStageDrops(int taskId, CombatSession session, JObject? details)
    {
        if (details is null)
        {
            return;
        }

        // 别传复刻一条链依次打多个关卡，关卡变化时结束当前 session 另起一条，各关的场次、理智与掉落分开记录
        var stageCode = details["stage"]?["stageCode"]?.ToString();
        if (!string.IsNullOrEmpty(stageCode) && !string.IsNullOrEmpty(session.Stage) && stageCode != session.Stage)
        {
            session = StartNextStageSession(taskId);
        }

        session.Stage = stageCode ?? session.Stage;

        // cur_times 是结算画面识别的本局代理倍率（识别不到时缺省），不是累计场次；缺省时退回开局读到的倍率
        var hasRound = _pendingRounds.Remove(taskId, out var round);
        var series = details.Value<int?>("cur_times") ?? (hasRound ? round.Series : 1);
        session.Times += series;
        if (hasRound)
        {
            // 开局消耗按开局倍率计；结算倍率与之不同时（如次数上限截断）按单局消耗折算
            session.Sanity += round.Series == series ? round.SanityCost : round.SanityCost / Math.Max(round.Series, 1) * series;
        }

        // stats 的 quantity 是本关卡内的累计值（别传复刻每关从零计），最后一次回调即终值快照
        if (details["stats"] is JArray stats)
        {
            var drops = new Dictionary<string, int>();
            foreach (var item in stats)
            {
                var itemId = item["itemId"]?.ToString();
                if (string.IsNullOrEmpty(itemId))
                {
                    continue;
                }

                // 家具归一为统一 id，与掉落日志的既有处理一致
                if (item["itemName"]?.ToString() == "furni")
                {
                    itemId = "3401";
                }

                drops[itemId] = item.Value<int?>("quantity") ?? 0;
            }

            if (drops.Count > 0)
            {
                session.Drops = drops;
            }
        }

        PersistIfNeeded(taskId, session);
    }

    /// <summary>
    /// 结束任务当前的 session 并以同一任务 id 新开一条，起始时刻接上一条的结束时刻。
    /// 新关卡开局时暂存的倍率与理智属于新 session，不随旧 session 结束而丢弃。
    /// </summary>
    private static CombatSession StartNextStageSession(int taskId)
    {
        var now = DateTimeOffset.UtcNow;
        var hasRound = _pendingRounds.Remove(taskId, out var round);
        FinalizeSession(taskId, now);
        if (hasRound)
        {
            _pendingRounds[taskId] = round;
        }

        var next = new CombatSession { StartTime = now };
        _pendingSessions[taskId] = next;
        return next;
    }

    private static bool IsEmpty(CombatSession session) =>
        session.Times == 0 && session.Drops.Count == 0 && session.MedicinesUsed == 0 && session.ExpiringMedicinesUsed == 0 && session.StonesUsed == 0;

    private static void PersistIfNeeded(int taskId, CombatSession session)
    {
        // 零场次且无掉落、用药、碎石的链没有统计价值，有数据前不落盘
        if (IsEmpty(session))
        {
            return;
        }

        if (!_persistedTaskIds.Add(taskId))
        {
            SaveCurrent();
            return;
        }

        EnsureCurrentDay();
        _currentData.Sessions.Add(session);
        SaveCurrent();
    }

    private static void FinalizeSession(int taskId, DateTimeOffset endTime)
    {
        _pendingRounds.Remove(taskId);
        if (!_pendingSessions.Remove(taskId, out var session))
        {
            return;
        }

        var persisted = _persistedTaskIds.Remove(taskId);
        if (IsEmpty(session))
        {
            // 落盘判断已拦下空记录，这里兜底清理任何已写入的空 session，避免文件残留无结束时刻的空行
            if (persisted && _currentData.Sessions.Remove(session))
            {
                SaveCurrent();
            }

            return;
        }

        session.EndTime = endTime;
        if (persisted)
        {
            SaveCurrent();
        }
    }

    /// <summary>
    /// 保证写入目标是当前游戏日；跨日时（正常不会发生，掉线或定时启动会先停止旧轮）将旧数据留在原文件。
    /// </summary>
    private static void EnsureCurrentDay()
    {
        var today = DateOnly.FromDateTime(DateTime.UtcNow.ToYjDate());
        if (today == _currentDay)
        {
            return;
        }

        _currentDay = today;
        _currentData = DailyReportStore.Load(today, out _currentWritable) ?? new DailyReportDayData();
    }

    private static void SaveCurrent()
    {
        if (_currentDay == default)
        {
            EnsureCurrentDay();
        }

        if (_currentWritable)
        {
            DailyReportStore.Save(_currentDay, _currentData);
        }

        Instances.TaskQueueViewModel.UpdateDailyReportSummary();
    }
}
