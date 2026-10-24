// <copyright file="OperProgressTaskUserControlModel.Supply.cs" company="MaaAssistantArknights">
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
using System.Security.Cryptography;
using System.Text;
using System.Windows.Media;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.States;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public partial class OperProgressTaskUserControlModel
{
    private readonly Dictionary<int, SupplyRun> _supplyRuns = [];
    private readonly Dictionary<string, SupplyReport> _supplyReports = [];

    public bool EnableSupplyReport
    {
        get => GetTaskConfig<OperProgressTask>().EnableSupplyReport;
        set {
            if (SetTaskConfig<OperProgressTask>(task => task.EnableSupplyReport == value, task => task.EnableSupplyReport = value))
            {
                RefreshSupplyReport();
            }
        }
    }

    public bool SupplyReportVisible { get; private set => SetAndNotify(ref field, value); }

    public bool IsSupplyReportExpanded { get; set => SetAndNotify(ref field, value); }

    public bool CanUpdateSupply { get; private set => SetAndNotify(ref field, value); }

    public string SupplyReportAction { get; private set => SetAndNotify(ref field, value); } = string.Empty;

    public string SupplyReportMessage { get; private set => SetAndNotify(ref field, value); } = string.Empty;

    public IReadOnlyList<SupplyMaterialRow> SupplyMaterialRows { get; private set => SetAndNotify(ref field, value); } = [];

    public record SupplyMaterialRow(string ItemId, int Missing)
    {
        public string Name => ItemListHelper.GetItemName(ItemId) ?? ItemId;

        public ImageSource? Image => ItemListHelper.GetItemImage(ItemId);

        public string Quantity => LocalizationHelper.GetStringFormat("OperProgressSupplyMissingCount", Missing);
    }

    /// <summary>
    /// Gets the identity of the current Elite 2 goals without including unrelated skill or display settings.
    /// </summary>
    /// <param name="task">The source cultivation task.</param>
    /// <returns>A stable signature for the current chip-pack requirements.</returns>
    public static string GetSupplySourceFingerprint(OperProgressTask task)
    {
        var goals = task.Plans.Where(plan => plan.Elite == 2)
            .Select(plan => (Role: plan.Role.ToString(), plan.Name))
            .Distinct().OrderBy(plan => plan.Role, StringComparer.Ordinal).ThenBy(plan => plan.Name, StringComparer.Ordinal);
        var payload = new JObject {
            ["source"] = task.SupplySourceId,
            ["goals"] = new JArray(goals.Select(plan => new JArray(plan.Role, plan.Name))),
        };
        return Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(payload.ToString(Formatting.None))));
    }

    private void InitializeSupplyReports()
    {
        Instances.AsstProxy.OnTaskStatusChanged += SupplyTaskStatusChanged;
        RunningState.Instance.StateChanged += (_, e) => Execute.OnUIThread(() => {
            if (e.OldState.Idle && !e.NewState.Idle)
            {
                _supplyReports.Clear();
            }
            RefreshSupplyReport();
        });
    }

    private (bool? IsSuccess, IEnumerable<int> TaskId) SerializeWithSupplyReport(OperProgressTask source, int? taskId)
    {
        var reportMaterials = source.EnableSupplyReport && source.Plans.Any(plan => plan.Elite == 2);
        var task = new AsstOperProgressTask { Plans = source.Plans, ReportMaterials = reportMaterials };
        if (taskId is > 0)
        {
            return (Instances.AsstProxy.AsstSetTaskParamsEncoded(taskId.Value, task), [taskId.Value]);
        }
        if (taskId is not null)
        {
            return (null, []);
        }

        var (success, coreId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.OperProgress, task);
        if (success != true || coreId <= 0)
        {
            return (success, []);
        }
        if (!reportMaterials)
        {
            return (true, [coreId]);
        }

        foreach (var oldId in _supplyRuns.Where(pair => ReferenceEquals(pair.Value.Source, source)).Select(pair => pair.Key).ToList())
        {
            _supplyRuns.Remove(oldId);
        }
        _supplyReports.Remove(source.SupplySourceId);
        var (depotSuccess, depotId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Depot, (AsstTaskType.Depot, null));
        if (depotSuccess != true || depotId <= 0)
        {
            _logger.Warning("Could not append cultivation material inventory recognition");
            return (false, [coreId]);
        }
        Instances.ToolboxViewModel.MarkDepotRecognitionSyncTimeForReset(depotId);
        _supplyRuns[coreId] = new SupplyRun(source, source.Plans.ToList(), depotId);
        return (true, [coreId, depotId]);
    }

    private void ProcSupplyReportMsg(AsstMsg type, AsstSubTaskMsg? msg)
    {
        if (type != AsstMsg.SubTaskExtraInfo || msg is null)
        {
            return;
        }
        if (msg.TaskChain == nameof(TaskType.Depot) && msg.What == "DepotInfo" && msg.Details?.Value<bool?>("done") == true)
        {
            var run = _supplyRuns.Values.FirstOrDefault(item => item.DepotTaskId == msg.TaskId);
            if (run is null)
            {
                return;
            }

            // Read this completed callback itself; an older cache is not evidence of successful recognition.
            run.Inventory = CultivationSupplyItemHelper.ReadInventory(msg.Details);
            return;
        }
        if (msg.What != "OperProgressMaterialReport" || !_supplyRuns.TryGetValue(msg.TaskId, out var context) || msg.Details is not { } details)
        {
            return;
        }

        if (details.Value<string>("scope") != "elite_chip_pack" || details["report_id"]?.Type != JTokenType.Integer)
        {
            return;
        }
        if (details.Value<string>("phase") == "begin")
        {
            context.ReportId = details.Value<int>("report_id");
            context.TargetCount = details.Value<int?>("target_count");
            return;
        }
        if (details.Value<string>("phase") == "end" && context.ReportId == details.Value<int>("report_id"))
        {
            context.End = (JObject)details.DeepClone();
        }
    }

    private void SupplyTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        if (status != TaskItemStatus.Completed && status != TaskItemStatus.Error)
        {
            return;
        }
        var context = _supplyRuns.Values.FirstOrDefault(run => run.DepotTaskId == taskId);
        if (context is null)
        {
            return;
        }
        Execute.OnUIThread(() => {
            if (!ConfigFactory.CurrentConfig.TaskQueue.Contains(context.Source) || !context.Source.EnableSupplyReport || !context.Valid ||
                !context.ExpectedPlans.SequenceEqual(context.Source.Plans))
            {
                return;
            }
            var plans = status == TaskItemStatus.Completed ? ReadSupplyPlans(context) : null;
            var report = new SupplyReport(context.Source, GetSupplySourceFingerprint(context.Source), plans, context.Inventory);
            _supplyReports[context.Source.SupplySourceId] = report;
            RefreshSupplyReport();
        });
    }

    private static List<CultivationSupplyTask.Plan>? ReadSupplyPlans(SupplyRun run)
    {
        if (run.Inventory is null || run.End is not { } end || end.Value<bool?>("complete") != true ||
            end.Value<bool?>("stopped") != false || end["entries"] is not JArray entries ||
            run.TargetCount is not int count || count != entries.Count || end.Value<int?>("target_count") != count)
        {
            return null;
        }

        var indices = new HashSet<int>();
        var demands = new Dictionary<(OperatorRole Role, string Name), (string ItemId, int Required, string Stage)>();
        foreach (var entry in entries.OfType<JObject>())
        {
            if (entry.Value<int?>("plan_index") is not int index || index < 0 || index >= run.OriginalPlans.Count || !indices.Add(index) ||
                !Enum.TryParse<OperatorRole>(entry.Value<string>("role"), true, out var role))
            {
                return null;
            }
            var source = run.OriginalPlans[index];
            if (source.Role != role || source.Name != entry.Value<string>("name") || source.Elite != 2 ||
                entry.Value<string>("action") != "Elite" || entry.Value<int?>("elite") != 2)
            {
                return null;
            }
            var state = entry.Value<string>("status");
            if (state == "satisfied")
            {
                continue;
            }
            if (state != "observed" || entry.Value<string>("source") != "elite_promotion" ||
                entry.Value<int?>("chip_packs_per_dual_chip") != 2 || entry.Value<int?>("dual_chip_required") is not (3 or 4))
            {
                return null;
            }
            if (!run.Source.Plans.Any(plan => plan.Role == role && plan.Name == source.Name && plan.Elite == 2))
            {
                return null;
            }
            var itemId = entry.Value<string>("chip_pack_item_id") ?? string.Empty;
            var expectedItemId = CultivationSupplyItemHelper.GetChipPackId(role);
            if (expectedItemId is null || itemId != expectedItemId ||
                entry.Value<string>("dual_chip_item_id") != CultivationSupplyItemHelper.GetDualChipId(itemId) ||
                entry.Value<int?>("dual_chip_owned") is not >= 0)
            {
                return null;
            }
            var stage = CultivationSupplyItemHelper.GetStage(itemId);
            if (stage is null)
            {
                return null;
            }

            // Identical operator goals describe one promotion, not multiple uses of the same shared stock.
            demands[(role, source.Name)] = (itemId, entry.Value<int>("dual_chip_required") * 2, stage);
        }
        if (indices.Count != count)
        {
            return null;
        }
        return demands.Values.GroupBy(demand => (demand.ItemId, demand.Stage))
            .Select(group => new CultivationSupplyTask.Plan(group.Key.ItemId, group.Sum(demand => demand.Required), group.Key.Stage))
            .OrderBy(plan => plan.ItemId, StringComparer.Ordinal).ToList();
    }

    private void AdvanceSupplyReportPlans(OperProgressTask source, IReadOnlyList<OperProgressTask.Plan> previous, IReadOnlyList<OperProgressTask.Plan> current)
    {
        foreach (var run in _supplyRuns.Values.Where(run => ReferenceEquals(run.Source, source)))
        {
            if (!run.ExpectedPlans.SequenceEqual(previous))
            {
                run.Valid = false;
            }
            run.ExpectedPlans = current.ToList();
        }
    }

    private void InvalidateChangedSupplySource(OperProgressTask source)
    {
        var fingerprint = GetSupplySourceFingerprint(source);
        if (_supplyReports.TryGetValue(source.SupplySourceId, out var report) && report.Fingerprint != fingerprint)
        {
            _supplyReports.Remove(source.SupplySourceId);
        }
        foreach (var run in _supplyRuns.Values.Where(run => ReferenceEquals(run.Source, source)))
        {
            if (!run.ExpectedPlans.SequenceEqual(source.Plans))
            {
                run.Valid = false;
            }
        }
        foreach (var supply in ConfigFactory.CurrentConfig.TaskQueue.OfType<CultivationSupplyTask>().Where(task => task.SourceTaskId == source.SupplySourceId))
        {
            if (supply.SourceFingerprint != fingerprint)
            {
                supply.IsStale = true;
            }
        }
        RefreshSupplyReport();
    }

    private void RefreshSupplyReport()
    {
        var source = TaskSettingVisibilityInfo.CurrentTask as OperProgressTask;
        var report = source is not null ? _supplyReports.GetValueOrDefault(source.SupplySourceId) : null;
        var valid = source is not null && source.EnableSupplyReport && report is not null && ReferenceEquals(report.Source, source) && !report.Dismissed &&
            report.Fingerprint == GetSupplySourceFingerprint(source);
        SupplyReportVisible = valid && RunningState.Instance.GetIdle();
        SupplyMaterialRows = [];
        CanUpdateSupply = false;
        SupplyReportAction = LocalizationHelper.GetString("OperProgressSupplySet");
        SupplyReportMessage = LocalizationHelper.GetString("OperProgressSupplyUnverifiedTip");
        if (!valid || report!.Plans is null || report.Inventory is null)
        {
            return;
        }
        var missing = report.Plans.Select(plan => new SupplyMaterialRow(plan.ItemId, Math.Max(0, plan.TargetCount -
            CultivationSupplyItemHelper.GetEffectiveCount(plan.ItemId, report.Inventory))))
            .Where(item => item.Missing > 0).ToList();
        SupplyMaterialRows = missing;
        SupplyReportMessage = LocalizationHelper.GetString(missing.Count > 0 ? "OperProgressSupplyScopeTip" : "OperProgressSupplySatisfiedTip");
        var existing = ConfigFactory.CurrentConfig.TaskQueue.OfType<CultivationSupplyTask>().FirstOrDefault();
        var proposed = report.Plans.Where(plan => missing.Any(item => item.ItemId == plan.ItemId)).ToList();
        if (existing is null && proposed.Count == 0)
        {
            SupplyReportVisible = false;
            return;
        }
        var same = existing is not null && existing.SourceTaskId == source!.SupplySourceId && existing.SourceFingerprint == report.Fingerprint &&
            !existing.IsStale && existing.Plans.OrderBy(plan => plan.ItemId).SequenceEqual(proposed.OrderBy(plan => plan.ItemId));
        var replaceSource = existing is not null && existing.SourceTaskId != source!.SupplySourceId;
        if (replaceSource)
        {
            SupplyReportMessage += Environment.NewLine + LocalizationHelper.GetString("OperProgressSupplyReplaceSourceTip");
        }
        SupplyReportAction = LocalizationHelper.GetString(same ? "OperProgressSupplyConfigured" :
            existing is null ? "OperProgressSupplySet" : replaceSource ? "OperProgressSupplyReplace" : "OperProgressSupplyUpdate");
        CanUpdateSupply = !same && RunningState.Instance.GetIdle();
    }

    public void DismissSupplyReport()
    {
        if (TaskSettingVisibilityInfo.CurrentTask is OperProgressTask source && _supplyReports.TryGetValue(source.SupplySourceId, out var report))
        {
            report.Dismissed = true;
            RefreshSupplyReport();
        }
    }

    public void UpdateSupply()
    {
        RefreshSupplyReport();
        if (!CanUpdateSupply || TaskSettingVisibilityInfo.CurrentTask is not OperProgressTask source ||
            !_supplyReports.TryGetValue(source.SupplySourceId, out var report) || report.Plans is null)
        {
            return;
        }
        var proposed = report.Plans.Where(plan => SupplyMaterialRows.Any(row => row.ItemId == plan.ItemId)).ToList();
        if (report.Fingerprint != GetSupplySourceFingerprint(source) || !RunningState.Instance.GetIdle())
        {
            RefreshSupplyReport();
            return;
        }
        Instances.TaskQueueViewModel.UpsertCultivationSupply(source, proposed, report.Fingerprint);
        RefreshSupplyReport();
    }

    private sealed class SupplyRun(OperProgressTask source, List<OperProgressTask.Plan> originalPlans, int depotTaskId)
    {
        public OperProgressTask Source { get; } = source;

        public List<OperProgressTask.Plan> OriginalPlans { get; } = originalPlans;

        public List<OperProgressTask.Plan> ExpectedPlans { get; set; } = originalPlans.ToList();

        public int DepotTaskId { get; } = depotTaskId;

        public int? ReportId { get; set; }

        public int? TargetCount { get; set; }

        public bool Valid { get; set; } = true;

        public JObject? End { get; set; }

        public Dictionary<string, int>? Inventory { get; set; }
    }

    private sealed class SupplyReport(OperProgressTask source, string fingerprint, List<CultivationSupplyTask.Plan>? plans, Dictionary<string, int>? inventory)
    {
        public OperProgressTask Source { get; } = source;

        public string Fingerprint { get; } = fingerprint;

        public List<CultivationSupplyTask.Plan>? Plans { get; } = plans;

        public Dictionary<string, int>? Inventory { get; } = inventory;

        public bool Dismissed { get; set; }
    }
}
