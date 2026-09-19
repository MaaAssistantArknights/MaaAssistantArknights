// <copyright file="MaterialCraftViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using ObservableCollections;
using Serilog;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl;

public class MaterialCraftViewModel : PropertyChangedBase
{
    private static readonly ILogger _logger = Log.ForContext<MaterialCraftViewModel>();
    private readonly ToolboxViewModel _toolbox;
    private readonly RunningState _runningState = RunningState.Instance;
    private MaterialCraftExecution? _materialCraftExecution;
    private CancellationTokenSource? _materialCraftCancellation;
    private int _requirementTaskId;
    private bool _materialCraftStopRequested;
    private string _manufacturingFailure = string.Empty;

    public MaterialCraftViewModel(ToolboxViewModel toolbox)
    {
        _toolbox = toolbox;
        _runningState.StateChanged += (_, e) => {
            NotifyOfPropertyChange(nameof(Idle));
            NotifyOfPropertyChange(nameof(Inited));
            NotifyOfPropertyChange(nameof(Stopping));
            NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
            if (e.NewState.Stopping)
            {
                CancelMaterialCraftPlan();
            }
            if (e.NewState.Idle)
            {
                CompleteMaterialCraftStop();
            }
        };

        // The toolbox and its child have the same application-wide lifetime.
        LocalizationHelper.LanguageChanged += () => Execute.OnUIThread(LoadMaterialRecipes);
        LoadMaterialRecipes();
    }

    public bool Idle => _runningState.Idle;

    public bool Inited => _runningState.Inited;

    public bool Stopping => _runningState.Stopping;

    public bool CanToggleMaterialCraft => Inited && !Stopping &&
        (Idle || _materialCraftExecution is not null || _materialCraftCancellation is not null || _requirementTaskId != 0);

    public bool CanEditMaterialCraftPlan => _materialCraftExecution is null && _materialCraftCancellation is null;

    public bool MaterialCraftStationOperators
    {
        get; set {
            SetAndNotify(ref field, value);
            ConfigFactory.CurrentConfig.Toolbox.MaterialCraftStationOperators = value;
        }
    } = ConfigFactory.CurrentConfig.Toolbox.MaterialCraftStationOperators;

    private ObservableList<ToolboxViewModel.DepotResultDate> DepotResult => _toolbox.DepotResult;

    public class MaterialCraftTarget
    {
        public string Id { get; init; } = string.Empty;

        public string Name { get; init; } = string.Empty;

        public int SortId { get; init; }

        public int QualityRank { get; init; }

        public string QualityName { get; init; } = string.Empty;

        public BitmapSource? Image { get; init; }
    }

    public class MaterialCraftTargetGroup
    {
        public string Name { get; init; } = string.Empty;

        public int QualityRank { get; init; }

        public ObservableCollection<MaterialCraftTarget> Targets { get; init; } = [];
    }

    public class MaterialCraftPlanItem : PropertyChangedBase
    {
        public string Id { get; init; } = string.Empty;

        public string Name { get => field; set => SetAndNotify(ref field, value); } = string.Empty;

        public BitmapSource? Image { get; init; }

        public int Count
        {
            get => field;
            set => SetAndNotify(ref field, Math.Max(1, value));
        } = 1;
    }

    public class MaterialCraftChange
    {
        public BitmapSource? Image { get; init; }

        public string Text { get; init; } = string.Empty;

        public bool ShowByproductHeader { get; init; }
    }

    public ObservableCollection<MaterialCraftTarget> MaterialCraftTargetList { get; } = [];

    public ObservableCollection<MaterialCraftTargetGroup> MaterialCraftTargetGroups { get; } = [];

    public ObservableCollection<MaterialCraftPlanItem> MaterialCraftPlanItems { get; } = [];

    public ObservableCollection<MaterialCraftChange> MaterialCraftChanges { get; } = [];

    public string MaterialCraftResultInfo { get => field; set => SetAndNotify(ref field, value); } = string.Empty;

    private void LoadMaterialRecipes()
    {
        try
        {
            var filename = Path.Combine(PathsHelper.ResourceDir, "material_recipes.json");
            var recipes = JObject.Parse(File.ReadAllText(filename));
            var targets = recipes.Properties().Select(property => property.Value).OfType<JObject>()
                .Where(recipe => !string.IsNullOrEmpty(recipe.Value<string>("itemId")))
                .GroupBy(recipe => recipe.Value<string>("itemId")!)
                .Select(group => {
                    int rank = group.First().Value<string>("facility") == "Mfg" ? -1 :
                        group.Key is "3302" or "3303" ? 0 : GetWorkshopQualityRank(group.First().Value<int>("goldCost"));
                    return new MaterialCraftTarget {
                        Id = group.Key,
                        Name = GetItemNameOrId(group.Key),
                        SortId = GetItemSortId(group.Key),
                        QualityRank = rank,
                        QualityName = GetWorkshopQualityName(rank),
                        Image = ItemListHelper.GetItemImage(group.Key),
                    };
                }).OrderBy(target => target.SortId).ToList();
            MaterialCraftTargetList.Clear();
            MaterialCraftTargetGroups.Clear();
            foreach (var target in targets)
            {
                MaterialCraftTargetList.Add(target);
            }
            foreach (var group in targets.GroupBy(target => target.QualityRank).OrderByDescending(group => group.Key))
            {
                MaterialCraftTargetGroups.Add(new() {
                    Name = GetWorkshopQualityName(group.Key),
                    QualityRank = group.Key,
                    Targets = new(group),
                });
            }
            foreach (var item in MaterialCraftPlanItems)
            {
                item.Name = GetItemNameOrId(item.Id);
            }
            InvalidateMaterialCraftPreview();
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to load material craft catalog");
        }
    }

    private static int GetWorkshopQualityRank(int goldCost)
    {
        if (goldCost <= 100)
        {
            return 1;
        }

        if (goldCost <= 200)
        {
            return 2;
        }

        if (goldCost <= 300)
        {
            return 3;
        }

        return 4;
    }

    private static string GetWorkshopQualityName(int qualityRank)
    {
        return qualityRank switch {
            -1 => LocalizationHelper.GetString("MaterialCraftManufacturingChips"),
            0 => LocalizationHelper.GetString("MaterialCraftSkillSummary"),
            1 => LocalizationHelper.GetString("MaterialCraftQualityNormal"),
            2 => LocalizationHelper.GetString("MaterialCraftQualityRare"),
            3 => LocalizationHelper.GetString("MaterialCraftQualityExcellent"),
            4 => LocalizationHelper.GetString("MaterialCraftQualitySuperior"),
            _ => LocalizationHelper.GetString("MaterialCraftTarget"),
        };
    }

    internal void InvalidateMaterialCraftPreview()
    {
        if (!CanEditMaterialCraftPlan)
        {
            return;
        }
        MaterialCraftResultInfo = string.Empty;
        MaterialCraftChanges.Clear();
    }

    [UsedImplicitly]
    public void AddMaterialCraftTarget(MaterialCraftTarget target)
    {
        AddMaterialCraftPlanItem(target, 1);
    }

    private void AddMaterialCraftPlanItem(MaterialCraftTarget target, int count)
    {
        if (!CanEditMaterialCraftPlan)
        {
            return;
        }

        var existing = MaterialCraftPlanItems.FirstOrDefault(item => item.Id == target.Id);
        if (existing is not null)
        {
            existing.Count += count;
        }
        else
        {
            var item = new MaterialCraftPlanItem {
                Id = target.Id,
                Name = target.Name,
                Image = target.Image,
                Count = count,
            };
            item.PropertyChanged += MaterialCraftPlanItemPropertyChanged;
            MaterialCraftPlanItems.Add(item);
        }

        InvalidateMaterialCraftPreview();
    }

    private bool TryAddMaterialCraftPlanItem(string itemId, int count, out string displayName)
    {
        displayName = GetItemNameOrId(itemId);
        if (count <= 0)
        {
            return false;
        }

        var target = MaterialCraftTargetList.FirstOrDefault(target => target.Id == itemId);
        if (target is null)
        {
            return false;
        }

        displayName = target.Name;
        AddMaterialCraftPlanItem(target, count);
        return true;
    }

    public void RemoveMaterialCraftPlanItem(MaterialCraftPlanItem item)
    {
        if (!CanEditMaterialCraftPlan)
        {
            return;
        }

        item.PropertyChanged -= MaterialCraftPlanItemPropertyChanged;
        MaterialCraftPlanItems.Remove(item);
        InvalidateMaterialCraftPreview();
    }

    [UsedImplicitly]
    public void DecreaseMaterialCraftPlanItem(MaterialCraftPlanItem item)
    {
        if (!CanEditMaterialCraftPlan)
        {
            return;
        }

        if (item.Count <= 1)
        {
            RemoveMaterialCraftPlanItem(item);
            return;
        }

        item.Count--;
    }

    [UsedImplicitly]
    public void IncreaseMaterialCraftPlanItem(MaterialCraftPlanItem item)
    {
        if (!CanEditMaterialCraftPlan)
        {
            return;
        }

        item.Count++;
    }

    public void ClearMaterialCraftPlan()
    {
        if (!CanEditMaterialCraftPlan)
        {
            return;
        }

        foreach (var item in MaterialCraftPlanItems)
        {
            item.PropertyChanged -= MaterialCraftPlanItemPropertyChanged;
        }

        MaterialCraftPlanItems.Clear();
        InvalidateMaterialCraftPreview();
    }

    private void MaterialCraftPlanItemPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(MaterialCraftPlanItem.Count))
        {
            InvalidateMaterialCraftPreview();
        }
    }

    [UsedImplicitly]
    public async Task RecognizeMaterialRequirement()
    {
        if (!Idle || Stopping || !CanEditMaterialCraftPlan)
        {
            return;
        }
        using var cancellation = new CancellationTokenSource();
        _materialCraftCancellation = cancellation;
        NotifyOfPropertyChange(nameof(CanEditMaterialCraftPlan));
        NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
        _runningState.BeginRun(RunOwner.Toolbox);
        MaterialCraftResultInfo = LocalizationHelper.GetString("ConnectingToEmulator");
        bool taskStarted = false;
        try
        {
            var (connected, error) = await Task.Run(() => {
                string message = string.Empty;
                return (Instances.AsstProxy.AsstConnect(ref message), message);
            });
            if (cancellation.IsCancellationRequested)
            {
                return;
            }
            if (!connected)
            {
                MaterialCraftResultInfo = error;
                _runningState.SetIdle(true);
                return;
            }
            var (appended, taskId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(
                AsstProxy.TaskType.MaterialRequirement, (Services.AsstTaskType.MaterialRequirement, null));
            _requirementTaskId = taskId;
            MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialRequirementStarted");
            if (!appended || !(taskStarted = Instances.AsstProxy.AsstStart()))
            {
                Instances.AsstProxy.AsstStop();
                _requirementTaskId = 0;
                MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialRequirementStartFailed");
                _runningState.SetIdle(true);
            }
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to start material requirement recognition");
            _requirementTaskId = 0;
            MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialRequirementStartFailed");
            _runningState.SetIdle(true);
        }
        finally
        {
            _materialCraftCancellation = null;
            if (cancellation.IsCancellationRequested && !taskStarted)
            {
                // No task chain exists to send TaskChainStopped during connection cancellation.
                MaterialCraftResultInfo = LocalizationHelper.GetString("Stopped");
                Instances.TaskQueueViewModel.SetStopped();
            }
            NotifyOfPropertyChange(nameof(CanEditMaterialCraftPlan));
            NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
        }
    }

    public bool MaterialRequirementParse(int taskId, JObject? details)
    {
        if (taskId != _requirementTaskId || Stopping)
        {
            return false;
        }
        _requirementTaskId = 0;
        NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
        if (details?.Value<string>("status") == "failed" || details?["items"] is not JArray items)
        {
            MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialRequirementFailed");
            return false;
        }

        var added = new List<string>();
        var noRecipe = new List<string>();
        foreach (var item in items.OfType<JObject>())
        {
            var itemId = item.Value<string>("item_id") ?? item.Value<string>("itemId") ?? string.Empty;
            int shortage = item.Value<int?>("shortage") ?? item.Value<int?>("count") ?? 0;
            if (string.IsNullOrEmpty(itemId) || shortage <= 0)
            {
                continue;
            }

            if (TryAddMaterialCraftPlanItem(itemId, shortage, out var displayName))
            {
                added.Add($"{displayName} x{shortage}");
            }
            else
            {
                noRecipe.Add($"{displayName} x{shortage}");
            }
        }

        var separator = LocalizationHelper.GetString("MaterialCraftListSeparator");
        var messages = new List<string>();
        if (details.Value<string>("status") == "partial")
        {
            messages.Add(LocalizationHelper.GetString("MaterialRequirementPartial"));
        }
        if (added.Count > 0)
        {
            messages.Add(string.Format(LocalizationHelper.GetString("MaterialRequirementAdded"), string.Join(separator, added)));
        }
        if (noRecipe.Count > 0)
        {
            messages.Add(string.Format(LocalizationHelper.GetString("MaterialRequirementNoRecipe"), string.Join(separator, noRecipe)));
        }

        MaterialCraftResultInfo = messages.Count == 0
            ? LocalizationHelper.GetString("MaterialRequirementNoMissing")
            : string.Join(Environment.NewLine, messages);
        return added.Count > 0;
    }

    public void FinishMaterialRequirement(int taskId)
    {
        if (_requirementTaskId != taskId)
        {
            return;
        }
        _requirementTaskId = 0;
        NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
        MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialRequirementFailed");
    }

    [UsedImplicitly]
    public async Task StartMaterialCraft()
    {
        if (!Idle || Stopping || !CanEditMaterialCraftPlan || !CheckMaterialCraftPlanCore())
        {
            return;
        }

        using var cancellation = new CancellationTokenSource();
        _materialCraftCancellation = cancellation;
        NotifyOfPropertyChange(nameof(CanEditMaterialCraftPlan));
        NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
        var taskParams = BuildMaterialCraftTaskParams();
        var targets = MaterialCraftPlanItems.ToDictionary(item => item.Id, item => item.Count);
        _manufacturingFailure = string.Empty;
        _runningState.BeginRun(RunOwner.Toolbox);
        MaterialCraftResultInfo = LocalizationHelper.GetString("ConnectingToEmulator");
        bool taskStarted = false;
        try
        {
            var (connected, error) = await Task.Run(() => {
                string message = string.Empty;
                return (Instances.AsstProxy.AsstConnect(ref message), message);
            });
            if (cancellation.IsCancellationRequested)
            {
                return;
            }
            if (!connected)
            {
                MaterialCraftResultInfo = error;
                _runningState.SetIdle(true);
                return;
            }

            var (appended, taskId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(
                AsstProxy.TaskType.MaterialCraft, (Services.AsstTaskType.MaterialCraft, taskParams));
            if (!appended)
            {
                MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialCraftStartFailed");
                _runningState.SetIdle(true);
                return;
            }

            // Register the snapshot before Core can send its first callback.
            var execution = new MaterialCraftExecution(taskId, targets) {
                InventoryUncertain = _toolbox.DepotInventoryNeedsRecognition,
            };
            _materialCraftExecution = execution;
            MaterialCraftChanges.Clear();

            // Core callbacks are asynchronous; persist before starting any game action.
            _toolbox.DepotInventoryNeedsRecognition = true;
            _toolbox.SaveDepotDetails();
            MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialCraftStarted");
            taskStarted = await Task.Run(() => {
                if (cancellation.IsCancellationRequested)
                {
                    return false;
                }
                bool result = Instances.AsstProxy.AsstStart();
                if (cancellation.IsCancellationRequested)
                {
                    Instances.AsstProxy.AsstStop();
                }
                return result;
            });
            if (!taskStarted && ReferenceEquals(_materialCraftExecution, execution))
            {
                Instances.AsstProxy.AsstStop();
                FinishMaterialCraft(taskId, completed: false);
                if (!cancellation.IsCancellationRequested)
                {
                    MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialCraftStartFailed");
                    _runningState.SetIdle(true);
                }
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to start material crafting");
            if (_materialCraftExecution is { } execution)
            {
                FinishMaterialCraft(execution.TaskId, completed: false);
            }
            if (!cancellation.IsCancellationRequested)
            {
                MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialCraftStartFailed");
                _runningState.SetIdle(true);
            }
        }
        finally
        {
            _materialCraftCancellation = null;
            if (cancellation.IsCancellationRequested && !taskStarted)
            {
                MaterialCraftResultInfo = LocalizationHelper.GetString("Stopped");
                Instances.TaskQueueViewModel.SetStopped();
            }
            NotifyOfPropertyChange(nameof(CanEditMaterialCraftPlan));
            NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));
        }
    }

    [UsedImplicitly]
    public async Task ToggleMaterialCraft()
    {
        if (Idle)
        {
            await StartMaterialCraft();
            return;
        }

        StopMaterialCraft();
    }

    private void StopMaterialCraft()
    {
        if (Idle || Stopping || (_materialCraftExecution is null && _materialCraftCancellation is null && _requirementTaskId == 0))
        {
            return;
        }

        MaterialCraftResultInfo = LocalizationHelper.GetString("Stopping");
        CancelMaterialCraftPlan();
        if (_materialCraftCancellation is not null && _materialCraftExecution is null)
        {
            // Let connection finish before allowing another task to start. There is no
            // Core task to stop yet; a background AsstStop could otherwise clear the next task.
            _runningState.SetStopping(true);
            return;
        }
        Instances.TaskQueueViewModel.ManualStop();
    }

    public void CancelMaterialCraftPlan()
    {
        _materialCraftStopRequested |= _materialCraftExecution is not null ||
            _materialCraftCancellation is not null || _requirementTaskId != 0;
        _materialCraftCancellation?.Cancel();
        _requirementTaskId = 0;
    }

    private void CompleteMaterialCraftStop()
    {
        if (!_materialCraftStopRequested || !Idle)
        {
            return;
        }
        _materialCraftStopRequested = false;
        MaterialCraftResultInfo = string.IsNullOrEmpty(_manufacturingFailure)
            ? LocalizationHelper.GetString("Stopped") : _manufacturingFailure;
    }

    private JObject BuildMaterialCraftTaskParams()
    {
        var targets = new JArray(MaterialCraftPlanItems
            .Where(item => item.Count > 0)
            .Select(item => new JObject {
                ["itemId"] = item.Id,
                ["count"] = item.Count,
            }));
        var inventory = new JObject();
        foreach (var group in DepotResult.Where(item => item.Count >= 0).GroupBy(item => item.Id))
        {
            inventory[group.Key] = group.First().Count;
        }

        return new JObject {
            ["items"] = targets,
            ["inventory"] = inventory,
            ["station_operators"] = MaterialCraftStationOperators,
            ["replenish"] = ConfigFactory.CurrentConfig.TaskQueue.OfType<InfrastTask>()
                .FirstOrDefault()?.OriginiumShardAutoReplenishment ?? false,
        };
    }

    private bool CheckMaterialCraftPlanCore()
    {
        if (MaterialCraftPlanItems.Count == 0)
        {
            MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialCraftEmptyPlan");
            return false;
        }

        // Core validates recipes and reads current ingredient quantities after connecting.
        if (MaterialCraftPlanItems.Any(item => item.Count <= 0))
        {
            MaterialCraftResultInfo = LocalizationHelper.GetString("MaterialCraftNoRecipe");
            return false;
        }

        return true;
    }

    public void MaterialCraftProgress(int taskId, string what, JObject? details)
    {
        if (_materialCraftExecution is not { } execution || execution.TaskId != taskId || details is null)
        {
            return;
        }
        int operation = details.Value<int?>("operation_id") ?? -1;
        if (what == "MaterialCraftManufacturingRestoreRequired")
        {
            _manufacturingFailure = string.Format(LocalizationHelper.GetString(what),
                GetItemNameOrId(details.Value<string>("item_id") ?? string.Empty),
                details.Value<int>("batches"), details.Value<int>("index") + 1);
            MaterialCraftResultInfo = _manufacturingFailure;
        }
        else if (what == "MaterialCraftManufacturingFailed")
        {
            _manufacturingFailure = LocalizationHelper.GetString(what);
            MaterialCraftResultInfo = _manufacturingFailure;
        }
        else if (what == "MaterialCraftRecipeFailed")
        {
            _logger.Warning("Material recipe execution failed: {Error}", details.Value<string>("error"));
            if (string.IsNullOrEmpty(_manufacturingFailure))
            {
                _manufacturingFailure = string.Format(LocalizationHelper.GetString(what),
                    GetItemNameOrId(details.Value<string>("item_id") ?? string.Empty));
                MaterialCraftResultInfo = _manufacturingFailure;
            }
        }
        else if (what == "MaterialCraftTargetCompleted" &&
            details.Value<string>("item_id") is { } targetId &&
            execution.ConfirmTarget(targetId, details.Value<int>("count")))
        {
            var completedItem = MaterialCraftPlanItems.FirstOrDefault(item => item.Id == targetId);
            if (completedItem is not null)
            {
                completedItem.PropertyChanged -= MaterialCraftPlanItemPropertyChanged;
                MaterialCraftPlanItems.Remove(completedItem);
            }
        }
        else if (what == "MaterialCraftOperationStarted")
        {
            execution.BeginOperation(operation);
        }
        else if (what == "MaterialCraftOperationCompleted" && execution.CompleteOperation(operation))
        {
            execution.InventoryUncertain |= details.Value<bool?>("inventory_complete") != true ||
                details["inventory_changes"] is not JArray { Count: > 0 };
            var netChanges = new Dictionary<string, long>();
            var byproducts = new Dictionary<string, long>();
            foreach (var item in (details["byproducts"] as JArray ?? []).OfType<JObject>())
            {
                string id = item.Value<string>("item_id") ?? string.Empty;
                long count = item.Value<long?>("count") ?? 0;
                if (!string.IsNullOrEmpty(id) && count > 0)
                {
                    byproducts[id] = byproducts.GetValueOrDefault(id) + count;
                }
            }
            var changes = new List<MaterialCraftInventoryChange>();
            foreach (var item in (details["inventory_changes"] as JArray ?? []).OfType<JObject>())
            {
                string id = item.Value<string>("item_id") ?? string.Empty;
                long delta = item.Value<long?>("count") ?? 0;
                if (string.IsNullOrEmpty(id))
                {
                    continue;
                }
                netChanges[id] = netChanges.GetValueOrDefault(id) + delta;
            }
            foreach (var (id, delta) in netChanges)
            {
                if (delta == 0)
                {
                    continue;
                }
                int before = DepotResult.FirstOrDefault(item => item.Id == id)?.Count ?? 0;
                long after = before + delta;
                execution.InventoryUncertain |= after < 0 || after > int.MaxValue;
                changes.Add(new() { Id = id, OldCount = before, NewCount = (int)Math.Clamp(after, 0, int.MaxValue) });
            }
            _toolbox.ApplyMaterialCraftInventoryChanges(changes);
            execution.RecordInventoryChanges(netChanges, byproducts);
        }
    }

    public void FinishMaterialCraft(int taskId, bool completed)
    {
        if (_materialCraftExecution is not { } execution || execution.TaskId != taskId)
        {
            return;
        }
        _materialCraftExecution = null;
        NotifyOfPropertyChange(nameof(CanEditMaterialCraftPlan));
        NotifyOfPropertyChange(nameof(CanToggleMaterialCraft));

        // Keep normal production/consumption separate from byproducts, including interrupted rounds.
        RenderMaterialCraftChanges(execution);
        if (completed && execution.HasConfirmedCompletion)
        {
            _toolbox.DepotInventoryNeedsRecognition = execution.InventoryUncertain;
            MaterialCraftResultInfo = LocalizationHelper.GetString(execution.InventoryUncertain ? "MaterialCraftInventoryUncertain" : "MaterialCraftCompleted");
        }
        else
        {
            // No started operation means Core has not attempted a crafting action.
            if (execution.CompletedOperations == 0 && !execution.PendingOperation.HasValue)
            {
                _toolbox.DepotInventoryNeedsRecognition = execution.InventoryUncertain;
            }
            MaterialCraftResultInfo = LocalizationHelper.GetString(
                _toolbox.DepotInventoryNeedsRecognition ? "MaterialCraftInventoryUncertain" : "MaterialCraftStartFailed");
        }
        if (!string.IsNullOrEmpty(_manufacturingFailure))
        {
            MaterialCraftResultInfo = _manufacturingFailure;
        }
        _toolbox.SaveDepotDetails();
    }

    private void RenderMaterialCraftChanges(MaterialCraftExecution execution)
    {
        MaterialCraftChanges.Clear();
        AppendChanges(execution.RegularChanges, byproducts: false);
        AppendChanges(execution.Byproducts, byproducts: true);

        void AppendChanges(IReadOnlyDictionary<string, long> changes, bool byproducts)
        {
            bool first = true;
            foreach (var (id, delta) in changes
                .Where(change => change.Value != 0)
                .OrderBy(change => GetItemSortId(change.Key))
                .ThenBy(change => GetItemNameOrId(change.Key), StringComparer.CurrentCulture))
            {
                MaterialCraftChanges.Add(new() {
                    Image = ItemListHelper.GetItemImage(id),
                    Text = $"{GetItemNameOrId(id)}: {delta:+#;-#;0}",
                    ShowByproductHeader = byproducts && first,
                });
                first = false;
            }
        }
    }

    private static string GetItemNameOrId(string itemId)
    {
        return ItemListHelper.GetItemName(itemId) ?? itemId;
    }

    private static int GetItemSortId(string itemId)
    {
        return ItemListHelper.ArkItems.TryGetValue(itemId, out var item) ? item.SortId : int.MaxValue;
    }
}
