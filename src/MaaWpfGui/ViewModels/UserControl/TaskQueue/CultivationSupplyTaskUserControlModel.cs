// <copyright file="CultivationSupplyTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Linq;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;
using Newtonsoft.Json.Linq;
using Serilog;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;
using static MaaWpfGui.ViewModels.UserControl.TaskQueue.FightTaskUserControlModel;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class CultivationSupplyTaskUserControlModel : TaskSettingsViewModel
{
    private static readonly ILogger _logger = Log.ForContext<CultivationSupplyTaskUserControlModel>();
    private readonly List<SupplyRun> _runs = [];

    public static CultivationSupplyTaskUserControlModel Instance { get; } = new();

    public CultivationSupplyTaskUserControlModel()
    {
        Instances.AsstProxy.AsstSubTaskMsgEvent += OnSubTaskMessage;
        RunningState.Instance.StateChanged += OnRunningStateChanged;
        LocalizationHelper.LanguageChanged += RefreshRows;
    }

    public ObservableCollection<CultivationSupplyPlanItemViewModel> PlanItems { get; } = [];

    public bool HasPlans => PlanItems.Count > 0;

    public bool CanEdit => RunningState.Instance.GetIdle();

    public bool IsStale => GetTaskConfig<CultivationSupplyTask>().IsStale;

    public bool UseAutoSeries
    {
        get => GetTaskConfig<CultivationSupplyTask>().UseAutoSeries;
        set => SetTaskConfig<CultivationSupplyTask>(t => t.UseAutoSeries == value, t => t.UseAutoSeries = value);
    }

    public bool UseMedicine
    {
        get => GetTaskConfig<CultivationSupplyTask>().UseMedicine;
        set => SetTaskConfig<CultivationSupplyTask>(t => t.UseMedicine == value, t => t.UseMedicine = value);
    }

    public int MedicineCount
    {
        get => GetTaskConfig<CultivationSupplyTask>().MedicineCount;
        set {
            value = Math.Clamp(value, 0, 999);
            SetTaskConfig<CultivationSupplyTask>(t => t.MedicineCount == value, t => t.MedicineCount = value);
        }
    }

    public bool UseExpiringMedicine
    {
        get => GetTaskConfig<CultivationSupplyTask>().UseExpiringMedicine;
        set => SetTaskConfig<CultivationSupplyTask>(t => t.UseExpiringMedicine == value, t => t.UseExpiringMedicine = value);
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not CultivationSupplyTask task)
        {
            return;
        }

        task.IsStale = task.Plans.Count > 0 && !IsSourceCurrent(task);
        PlanItems.Clear();
        foreach (var plan in task.Plans)
        {
            PlanItems.Add(new(plan));
        }

        NotifyOfPropertyChange(nameof(HasPlans));
        NotifyOfPropertyChange(nameof(CanEdit));
        NotifyOfPropertyChange(nameof(IsStale));
        NotifyOfPropertyChange(nameof(UseAutoSeries));
        NotifyOfPropertyChange(nameof(UseMedicine));
        NotifyOfPropertyChange(nameof(MedicineCount));
        NotifyOfPropertyChange(nameof(UseExpiringMedicine));
    }

    public bool ReplacePlans(
        CultivationSupplyTask task,
        IReadOnlyCollection<CultivationSupplyTask.Plan> plans,
        string sourceTaskId,
        string sourceFingerprint)
    {
        if (!CanEdit || string.IsNullOrWhiteSpace(sourceTaskId) || string.IsNullOrWhiteSpace(sourceFingerprint) ||
            !ArePlansValid(plans))
        {
            return false;
        }

        task.Plans = [.. plans];
        task.SourceTaskId = sourceTaskId;
        task.SourceFingerprint = sourceFingerprint;
        task.IsStale = false;
        if (ReferenceEquals(TaskSettingVisibilityInfo.CurrentTask, task))
        {
            RefreshUI(task);
        }

        return true;
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null)
    {
        if (baseTask is not CultivationSupplyTask task || taskId > 0 || task.Plans.Count == 0)
        {
            return (null, []);
        }

        if (!IsSourceCurrent(task))
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CultivationSupplyStale"), UiLogColor.Warning);
            return (null, []);
        }

        if (!ArePlansValid(task.Plans))
        {
            _logger.Warning("Invalid cultivation supply plan");
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CultivationSupplyStale"), UiLogColor.Warning);
            return (false, []);
        }

        var (success, depotTaskId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Depot, (AsstTaskType.Depot, null));
        if (!success)
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DepotPlanUpdateDepotFailed"), UiLogColor.Error);
            return (false, []);
        }

        var run = new SupplyRun(task, depotTaskId);
        _runs.Add(run);
        Instances.ToolboxViewModel.MarkDepotRecognitionSyncTimeForReset(depotTaskId);
        var ids = new List<int> { depotTaskId };

        // Register every open candidate before scanning. A stale cache must never omit a Fight.
        // The Core barrier prevents either running or skipping until its parameters are ready.
        foreach (var (plan, index) in run.Plans.Select((plan, index) => (plan, index)))
        {
            if (string.IsNullOrEmpty(GetFightStage([plan.Stage])))
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanStageNotOpen", index + 1, plan.Stage));
                continue;
            }

            var fight = CreateFight(run, plan, false);
            var (added, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, fight);
            if (!added)
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanAddTaskFailed", index + 1), UiLogColor.Error);
                return (false, ids);
            }

            run.Fights.Add(id, plan);
            ids.Add(id);
        }

        return (true, ids);
    }

    private static bool IsSourceCurrent(CultivationSupplyTask task)
    {
        var sources = ConfigFactory.CurrentConfig.TaskQueue.OfType<OperProgressTask>()
            .Where(candidate => candidate.SupplySourceId == task.SourceTaskId).Take(2).ToList();
        var source = sources.Count == 1 ? sources[0] : null;
        if (source is null || string.IsNullOrWhiteSpace(task.SourceFingerprint) ||
            OperProgressTaskUserControlModel.GetSupplySourceFingerprint(source) != task.SourceFingerprint)
        {
            task.IsStale = true;
            return false;
        }

        return !task.IsStale;
    }

    private static bool ArePlansValid(IEnumerable<CultivationSupplyTask.Plan> plans)
    {
        var ids = new HashSet<string>();
        return plans.All(plan => plan.TargetCount > 0 &&
            CultivationSupplyItemHelper.GetStage(plan.ItemId) is { } stage &&
            stage == plan.Stage && ids.Add(plan.ItemId));
    }

    private static AsstFightTask CreateFight(SupplyRun run, CultivationSupplyTask.Plan plan, bool ready) => new() {
        Stage = plan.Stage,
        MaxTimes = 0,
        Medicine = ready ? run.RemainingMedicine : 0,
        MedicineExpireDays = ready && run.UseExpiringMedicine ? DepotMaintainTask.ExpiringMedicineDays : 0,
        Stone = 0,
        Series = run.UseAutoSeries ? 0 : 1,
        WaitForParams = true,
        ParamsReady = ready,
        ReportToPenguin = SettingsViewModel.ThirdPartyServiceSettings.EnablePenguin,
        ReportToYituliu = SettingsViewModel.ThirdPartyServiceSettings.EnableYituliu,
        PenguinId = SettingsViewModel.ThirdPartyServiceSettings.PenguinId,
        ServerType = Instances.SettingsViewModel.ServerType,
        ClientType = SettingsViewModel.GameSettings.ClientType,
    };

    private void OnSubTaskMessage(AsstMsg type, AsstSubTaskMsg? msg)
    {
        if (type != AsstMsg.SubTaskExtraInfo || msg is null)
        {
            return;
        }

        var depotRun = _runs.FirstOrDefault(run => run.DepotTaskId == msg.TaskId);
        if (depotRun is not null)
        {
            if (msg.What == "DepotInfo" && msg.Details is { } details && details.Value<bool?>("done") == true)
            {
                depotRun.Inventory = CultivationSupplyItemHelper.ReadInventory(details);
                MarkCompletedPlans(depotRun);
                RefreshRows();
            }
            return;
        }

        var run = _runs.FirstOrDefault(candidate => candidate.Fights.ContainsKey(msg.TaskId));
        if (run is null)
        {
            return;
        }

        switch (msg.What)
        {
            case "FightAwaitingParameters":
                ReleaseFight(run, msg.TaskId, run.Fights[msg.TaskId]);
                break;

            case "UseMedicine":
                // Core processes regular allowance first; expiring-mode medicine is extra.
                if (msg.Details?.Value<bool?>("is_expiring") == false && msg.Details.Value<int?>("count") is > 0 and var medicineCount)
                {
                    run.RemainingMedicine = Math.Max(0, run.RemainingMedicine - medicineCount);
                }
                break;

            case "StageDrops":
                if (run.Inventory is not null && msg.Details?["stats"] is JArray stats)
                {
                    foreach (var item in stats)
                    {
                        var itemId = item.Value<string>("itemId");
                        var total = item.Value<int?>("quantity") ?? 0;
                        if (!string.IsNullOrEmpty(itemId) && total > 0)
                        {
                            var key = (msg.TaskId, itemId);
                            var previous = run.DropTotals.GetValueOrDefault(key);
                            if (total <= previous)
                            {
                                continue;
                            }

                            run.DropTotals[key] = total;
                            run.Inventory.TryGetValue(itemId, out var oldCount);
                            run.Inventory[itemId] = (int)Math.Min(int.MaxValue, (long)oldCount + total - previous);
                        }
                    }

                    MarkCompletedPlans(run);
                    RefreshRows();
                }
                break;
        }
    }

    private static void ReleaseFight(SupplyRun run, int taskId, CultivationSupplyTask.Plan plan)
    {
        if (!run.ReleasedFights.Add(taskId))
        {
            return;
        }

        var fight = CreateFight(run, plan, true);
        if (run.Inventory is null)
        {
            Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("CultivationSupplyInventoryUnavailable"), UiLogColor.Warning);
        }
        else if (IsSourceCurrent(run.Task) && run.Task.SourceTaskId == run.SourceTaskId &&
            !string.IsNullOrEmpty(GetFightStage([plan.Stage])))
        {
            var current = CultivationSupplyItemHelper.GetEffectiveCount(plan.ItemId, run.Inventory);
            var need = Math.Max(0, plan.TargetCount - current);
            if (need > 0)
            {
                fight.MaxTimes = int.MaxValue;
                fight.Drops = new() { [plan.ItemId] = need };
            }
        }

        if (!Instances.AsstProxy.AsstSetTaskParamsEncoded(taskId, fight))
        {
            _logger.Error("Could not release cultivation supply Fight {TaskId}", taskId);
        }
    }

    private static void MarkCompletedPlans(SupplyRun run)
    {
        if (run.Inventory is null)
        {
            return;
        }

        foreach (var plan in run.Plans)
        {
            if (CultivationSupplyItemHelper.GetEffectiveCount(plan.ItemId, run.Inventory) >= plan.TargetCount)
            {
                run.Completed.Add(plan);
            }
        }
    }

    private void OnRunningStateChanged(object? sender, RunningState.RunningStateChangedEventArgs args)
    {
        Execute.OnUIThread(() => {
            NotifyOfPropertyChange(nameof(CanEdit));
            if (!args.OldState.Idle && args.NewState.Idle)
            {
                FinishRun();
            }
        });
    }

    public void FinishRun()
    {
        // Completion evidence belongs to this run, before subsequent cultivation can consume it.
        // A later inventory decrease never reconstructs a cleared plan.
        foreach (var run in _runs)
        {
            if (run.Task.SourceTaskId == run.SourceTaskId && run.Completed.Count > 0)
            {
                run.Task.Plans = [.. run.Task.Plans.Where(plan => !run.Completed.Contains(plan))];
                if (run.Task.Plans.Count == 0)
                {
                    run.Task.IsStale = false;
                }
                if (ReferenceEquals(TaskSettingVisibilityInfo.CurrentTask, run.Task))
                {
                    RefreshUI(run.Task);
                }
            }
        }

        _runs.Clear();
    }

    private void RefreshRows()
    {
        foreach (var row in PlanItems)
        {
            row.RefreshDisplay();
        }
    }

    private sealed class SupplyRun(CultivationSupplyTask task, int depotTaskId)
    {
        public CultivationSupplyTask Task { get; } = task;

        public string SourceTaskId { get; } = task.SourceTaskId;

        public int DepotTaskId { get; } = depotTaskId;

        public List<CultivationSupplyTask.Plan> Plans { get; } = [.. task.Plans];

        public Dictionary<int, CultivationSupplyTask.Plan> Fights { get; } = [];

        public HashSet<int> ReleasedFights { get; } = [];

        public Dictionary<string, int>? Inventory { get; set; }

        public Dictionary<(int TaskId, string ItemId), int> DropTotals { get; } = [];

        public HashSet<CultivationSupplyTask.Plan> Completed { get; } = [];

        public int RemainingMedicine { get; set; } = task.UseMedicine ? Math.Clamp(task.MedicineCount, 0, 999) : 0;

        public bool UseExpiringMedicine { get; } = task.UseExpiringMedicine;

        public bool UseAutoSeries { get; } = task.UseAutoSeries;
    }
}
