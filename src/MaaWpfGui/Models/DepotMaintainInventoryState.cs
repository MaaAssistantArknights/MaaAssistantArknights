// <copyright file="DepotMaintainInventoryState.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.Models;

/// <summary>
/// Owns the exact display ledger and routes native maintenance callbacks.
/// Registration order prevents a late earlier task from reclaiming a newer run.
/// </summary>
public sealed class DepotMaintainInventoryState
{
    private readonly DepotMaintainInventorySnapshot _snapshot = new();
    private readonly Dictionary<int, long> _generations = [];
    private readonly HashSet<int> _retired = [];
    private long _generation;
    private long _activeGeneration;
    private int _activeTaskId;
    private bool _closed;

    public long? Count(string itemId) => _snapshot.Count(itemId);

    public JObject? Export() => _snapshot.Export();

    public void Register(int taskId)
    {
        if (taskId > 0 && !_generations.ContainsKey(taskId))
        {
            _generations.Add(taskId, ++_generation);
        }
    }

    public bool CanAcceptDepot(int taskId) => taskId > 0 && taskId == _activeTaskId && !_closed;

    public bool LoadCache(JToken? data, bool complete)
    {
        // Language refreshes must never replace a run's current ledger.
        if (_activeGeneration != 0)
        {
            return false;
        }
        _snapshot.Replace(data, complete);
        return true;
    }

    public void Clear()
    {
        if (_activeTaskId > 0)
        {
            _retired.Add(_activeTaskId);
        }
        _closed = true;
        _snapshot.Clear();
    }

    private bool Begin(int taskId)
    {
        if (!_generations.TryGetValue(taskId, out var generation) ||
            generation <= _activeGeneration || _retired.Contains(taskId))
        {
            return false;
        }
        _activeGeneration = generation;
        _activeTaskId = taskId;
        _closed = false;
        _snapshot.Clear();
        return true;
    }

    private bool Finish(int taskId, bool uncertain)
    {
        if (_generations.ContainsKey(taskId))
        {
            _retired.Add(taskId);
        }
        if (!CanAcceptDepot(taskId))
        {
            return false;
        }
        _closed = true;
        if (uncertain)
        {
            _snapshot.Clear();
        }
        return true;
    }

    /// <summary>
    /// Routes the same native message name and payload received by AsstProxy.
    /// </summary>
    /// <param name="message">Native AsstMsg enum name.</param>
    /// <param name="details">Full native callback payload.</param>
    /// <returns>Whether the display ledger changed and must be saved/notified.</returns>
    public bool HandleCallback(string message, JObject details)
    {
        var chain = details["taskchain"]?.ToString();
        var what = details["what"]?.ToString();
        if (chain != "DepotMaintain")
        {
            if (message == "SubTaskExtraInfo" && what is "StageDrops" or "DepotInfo")
            {
                // Only a subsequent external task can invalidate this ledger;
                // a delayed callback cannot interrupt an active maintenance run.
                if (_activeTaskId > 0 && !_closed)
                {
                    return false;
                }
                Clear();
                return true;
            }
            return false;
        }
        if (!int.TryParse(details["taskid"]?.ToString(), out var taskId) || taskId <= 0)
        {
            return false;
        }
        if (message is "TaskChainError" or "TaskChainStopped")
        {
            return Finish(taskId, uncertain: true);
        }
        if (message == "TaskChainCompleted")
        {
            return Finish(taskId, uncertain: false);
        }
        var payload = details["details"] as JObject;
        var status = payload?["status"]?.ToString();
        if (message == "SubTaskExtraInfo" && what == "DepotMaintainInfo" && status == "recognizing_fresh_depot")
        {
            return Begin(taskId);
        }
        if (!CanAcceptDepot(taskId))
        {
            return false;
        }
        if (message == "SubTaskError" && details["subtask"]?.ToString() == "StageDropsTask")
        {
            return Finish(taskId, uncertain: true);
        }
        if (message != "SubTaskExtraInfo")
        {
            return false;
        }
        if (what == "DepotInfo")
        {
            var complete = payload?["done"]?.Type == JTokenType.Boolean && payload["done"]!.Value<bool>();
            _snapshot.Replace(payload?["verified_data"], complete);
            return true;
        }
        if (what == "StageDrops")
        {
            _snapshot.Replace(payload?["verified_inventory"], complete: true);
            return true;
        }
        if (what == "DepotMaintainInfo" && status is
            "depot_unverified_no_farming" or "drops_unverified" or "fallback_drops_unverified" or
            "fight_failed_or_stopped" or "fallback_failed_or_stopped" or
            "incomplete_unverified_stop" or "fallback_incomplete_unverified_stop")
        {
            return Finish(taskId, uncertain: true);
        }

        // A verified natural-budget stop preserves known stock; it does not
        // mean that all goals were met. TaskChainCompleted retires the owner.
        return false;
    }
}
