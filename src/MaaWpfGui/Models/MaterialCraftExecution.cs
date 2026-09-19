// <copyright file="MaterialCraftExecution.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;

namespace MaaWpfGui.Models;

/// <summary>
/// A submitted crafting request. UI previews and edits must not change this snapshot.
/// </summary>
public sealed class MaterialCraftExecution(
    int taskId,
    IReadOnlyDictionary<string, int> targets)
{
    private readonly Dictionary<string, long> _regularChanges = [];
    private readonly Dictionary<string, long> _byproducts = [];
    private readonly HashSet<string> _completedTargets = [];

    public int TaskId { get; } = taskId;

    public IReadOnlyDictionary<string, int> Targets { get; } = new ReadOnlyDictionary<string, int>(new Dictionary<string, int>(targets));

    public int? PendingOperation { get; private set; }

    public int CompletedOperations { get; private set; }

    public bool InventoryUncertain { get; set; }

    public bool HasConfirmedCompletion => CompletedOperations > 0 && !PendingOperation.HasValue;

    public IReadOnlyDictionary<string, long> RegularChanges => _regularChanges;

    public IReadOnlyDictionary<string, long> Byproducts => _byproducts;

    public void RecordInventoryChanges(
        IReadOnlyDictionary<string, long> netChanges,
        IReadOnlyDictionary<string, long> byproducts)
    {
        foreach (var (id, count) in netChanges)
        {
            _regularChanges[id] = _regularChanges.GetValueOrDefault(id) + count;
        }
        foreach (var (id, count) in byproducts)
        {
            // Core's net deltas already include byproducts. Split the display without applying stock twice.
            _regularChanges[id] = _regularChanges.GetValueOrDefault(id) - count;
            _byproducts[id] = _byproducts.GetValueOrDefault(id) + count;
        }
    }

    public bool ConfirmTarget(string itemId, int count)
    {
        return PendingOperation is null && CompletedOperations > 0 &&
            Targets.TryGetValue(itemId, out int requested) && count == requested && _completedTargets.Add(itemId);
    }

    public bool BeginOperation(int operation)
    {
        if (operation != CompletedOperations || PendingOperation.HasValue)
        {
            return false;
        }

        PendingOperation = operation;
        return true;
    }

    public bool CompleteOperation(int operation)
    {
        if (PendingOperation != operation)
        {
            return false;
        }

        PendingOperation = null;
        CompletedOperations++;
        return true;
    }
}
