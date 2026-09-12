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
    IReadOnlyDictionary<string, int> targets,
    IReadOnlyDictionary<string, long>? plannedOutputs = null)
{
    private readonly Dictionary<string, long> _plannedOutputs = plannedOutputs is null ? [] : new(plannedOutputs);
    private readonly Dictionary<string, long> _craftedOutputs = [];
    private readonly Dictionary<string, MaterialCraftInventoryChange> _inventoryChanges = [];

    public int TaskId { get; } = taskId;

    public IReadOnlyDictionary<string, int> Targets { get; } = new ReadOnlyDictionary<string, int>(new Dictionary<string, int>(targets));

    public int? PendingOperation { get; private set; }

    public int CompletedOperations { get; private set; }

    public bool InventoryUncertain { get; set; }

    public bool HasConfirmedCompletion => CompletedOperations > 0 && !PendingOperation.HasValue;

    public IReadOnlyCollection<MaterialCraftInventoryChange> InventoryChanges => _inventoryChanges.Values;

    public void RecordInventoryChanges(IEnumerable<MaterialCraftInventoryChange> changes)
    {
        foreach (var change in changes)
        {
            _inventoryChanges[change.Id] = new() {
                Id = change.Id,
                OldCount = _inventoryChanges.TryGetValue(change.Id, out var previous) ? previous.OldCount : change.OldCount,
                NewCount = change.NewCount,
            };
        }
    }

    public bool RecordCraftedOutput(string itemId, long count)
    {
        if (count <= 0 || !Targets.ContainsKey(itemId))
        {
            return false;
        }
        _craftedOutputs[itemId] = _craftedOutputs.GetValueOrDefault(itemId) + count;
        return _craftedOutputs[itemId] >= _plannedOutputs.GetValueOrDefault(itemId, Targets[itemId]);
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
