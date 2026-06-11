// <copyright file="DepotMaintainTaskUserControlModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Threading.Tasks;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using Serilog;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;
using static MaaWpfGui.ViewModels.UserControl.TaskQueue.FightSettingsUserControlModel;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class DepotMaintainTaskUserControlModel : TaskSettingsViewModel, DepotMaintainTaskUserControlModel.ISerialize
{
    private readonly ILogger _logger = Log.ForContext<DepotMaintainTaskUserControlModel>();

    static DepotMaintainTaskUserControlModel()
    {
        Instance = new();
    }

    public DepotMaintainTaskUserControlModel()
    {
        PlanList.CollectionChanged += (_, __) => SavePlan();
    }

    public static DepotMaintainTaskUserControlModel Instance { get; }

    public bool UpdateDepot
    {
        get => GetTaskConfig<DepotMaintainTask>().UpdateDepot;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UpdateDepot == value, t => t.UpdateDepot = value);
    }

    public ObservableCollection<Plan> PlanList { get; private set => SetAndNotify(ref field, value); } = [];

    public void AddPlan()
    {
        PlanList.Add(new Plan());
    }

    public void RemovePlan(Plan plan)
    {
        PlanList.Remove(plan);
    }

    public bool IsStageManually
    {
        get => GetTaskConfig<DepotMaintainTask>().IsStageManually;
        set {
            bool ret = SetTaskConfig<DepotMaintainTask>(t => t.IsStageManually == value, t => t.IsStageManually = value);
            if (ret && !value)
            {
                for (int i = 0; i < PlanList.Count; i++)
                {
                    var stage = PlanList[i];
                    if (!Instances.StageManager.GetStageList().Any(p => p.Value == stage.Stage))
                    {
                        PlanList[i].Stage = string.Empty;
                    }
                }
            }
        }
    }

    /// <summary>
    /// Gets or private sets a value indicating whether 关卡列表。
    /// </summary>
    public ObservableCollection<StageSourceItem> StageListSource { get; private set => SetAndNotify(ref field, value); } = [];

    public string PlanInfo => string.Join("\n", PlanList.Select((t, i) => $"{i + 1}: {StageListSource.FirstOrDefault(i => i.Value == t.Stage)?.Display ?? t.Stage} - {t.DropName} x{t.DropCount}"));

    private void SavePlan()
    {
        var list = PlanList.Select(i => new DepotMaintainTask.Plan(i.Stage, i.DropId, i.DropCount, i.UseMedicine, i.MedicineCount, i.UseStone, i.StoneCount));
        SetTaskConfig<DepotMaintainTask>(t => t.PlanList.SequenceEqual(list), t => t.PlanList = [.. list]);
    }

    /// <summary>
    /// 更新关卡列表。
    /// 使用手动输入时，只更新关卡列表，不更新关卡选择
    /// 使用隐藏当日不开放时，更新关卡列表，关卡选择为未开放的关卡时清空
    /// 使用备选关卡时，更新关卡列表，关卡选择为未开放的关卡时在关卡列表中添加对应未开放关卡，避免清空导致进入上次关卡
    /// 啥都不选时，更新关卡列表，关卡选择为未开放的关卡时在关卡列表中添加对应未开放关卡，避免清空导致进入上次关卡
    /// 除手动输入外所有情况下，如果剩余理智为未开放的关卡，会被清空
    /// </summary>
    /// <returns>更新任务列表的Task</returns>
    // FIXME：被注入对象只能在 private 函数内使用，只有 Model 显示之后才会被注入。如果 Model 还没有触发 OnInitialActivate 时调用此函数，会导致空引用异常。
    // 这个函数被声明为 public，意味着它可能会在注入对象前被调用。
    public Task UpdateStageList()
    {
        return Execute.OnUIThreadAsync(async () => {
            using var log = new LogScope(_logger);
            var stageList = Instances.StageManager.GetStageList();
            await TaskQueueViewModel.TaskQueueSerializingLock.WaitAsync();

            RefreshStageList();
            TaskQueueViewModel.TaskQueueSerializingLock.Release();
        });
    }

    private void RefreshStageList()
    {
        if (TaskSettingVisibilityInfo.CurrentTask is not DepotMaintainTask current)
        {
            return;
        }
        var stageList = Instances.StageManager.GetStageList().Where(i => i.Value != AnnihilationName).ToList();
        var listCurrent = current.PlanList.ToList();

        var listSource = stageList.Select(i => new StageSourceItem() { Display = i.Display, Value = i.Value, IsVisible = true, IsOpen = Instances.StageManager.GetStageList().FirstOrDefault(p => p.Value == i.Value)?.IsStageOpen(Instances.TaskQueueViewModel.CurDayOfWeek) ?? true }).ToList();

        // 补过期关卡进来
        foreach (var item in listCurrent.Where(i => !listSource.Any(p => p.Value == i.Stage)))
        {
            listSource.Add(new StageSourceItem() { Display = item.Stage, Value = item.Stage, IsOpen = false, IsVisible = true });
        }
        StageListSource = [.. listSource];
        foreach (var (plan, currentPlan) in PlanList.Zip(listCurrent))
        {
            plan.Stage = currentPlan.Stage; // StageListSource更新后, 恢复StagePlan
        }
    }

    // UI 绑定的方法
    [UsedImplicitly]
    public void CollapseAll()
    {
        foreach (var plan in PlanList)
        {
            plan.IsExpanded = false;
        }
    }

    public class Plan : PropertyChangedBase
    {
        public bool IsExpanded { get; set => SetAndNotify(ref field, value); }

        public string Title => $"{Instance.StageListSource.FirstOrDefault(i => i.Value == Stage)?.Display ?? Stage} - {DropName} x{DropCount}";

        public string Stage
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.NotifyOfPropertyChange(nameof(PlanInfo));
                NotifyOfPropertyChange(nameof(Title));
            }
        } = string.Empty;

        /// <summary>
        /// Gets or sets 指定掉落材料 ID。
        /// </summary>
        public string DropId
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.NotifyOfPropertyChange(nameof(PlanInfo));
                NotifyOfPropertyChange(nameof(Title));
            }
        } = string.Empty;

        /// <summary>
        /// Gets or sets 指定掉落材料名称。
        /// </summary>
        public string DropName { get; set => SetAndNotify(ref field, value); } = LocalizationHelper.GetString("NotSelected");

        public int DropCount
        {
            get; set {
                SetAndNotify(ref field, value);
                Instance.NotifyOfPropertyChange(nameof(PlanInfo));
                NotifyOfPropertyChange(nameof(Title));
            }
        }

        public bool UseMedicine { get; set => SetAndNotify(ref field, value); }

        public int MedicineCount { get; set => SetAndNotify(ref field, value); }

        public bool UseStone { get; set => SetAndNotify(ref field, value); }

        public int StoneCount { get; set => SetAndNotify(ref field, value); }

        // UI 绑定的方法
        [UsedImplicitly]
        public void DropsListDropDownClosed()
        {
            if (FightSettingsUserControlModel.Instance.DropsList.FirstOrDefault(i => i.Display == DropName) is { } item)
            {
                DropId = item.Value;
            }
            else
            {
                DropId = string.Empty;
                DropName = LocalizationHelper.GetString("NotSelected");
                NotifyOfPropertyChange(nameof(DropName));
            }
        }
    }

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not DepotMaintainTask task)
        {
            return;
        }
        var list = new List<Plan>();
        foreach (var plan in task.PlanList)
        {
            var uiPlan = new Plan() {
                Stage = plan.Stage,
                DropId = plan.DropId,
                DropCount = plan.DropCount,
                UseMedicine = plan.UseMedicine,
                MedicineCount = plan.MedicineCount,
                UseStone = plan.UseStone,
                StoneCount = plan.StoneCount,
            };
            list.Add(uiPlan);
            uiPlan.PropertyChanged += (_, __) => SavePlan();
        }
        PlanList = [.. list];
        PlanList.CollectionChanged += (_, __) => SavePlan();
        Refresh();
    }

    public override (bool? IsSuccess, IEnumerable<int> TaskId) SerializeTask(BaseTask? baseTask, int? taskId = null) => (this as ISerialize).Serialize(baseTask, taskId);

    private interface ISerialize : ITaskQueueModelSerialize
    {
        (bool? IsSuccess, IEnumerable<int> TaskId) ITaskQueueModelSerialize.Serialize(BaseTask? baseTask, int? taskId)
        {
            if (baseTask is not DepotMaintainTask depot || taskId > 0)
            {
                return (null, []);
            }

            var depotList = Instances.ToolboxViewModel?.DepotResult.Where(item => item.Count >= 0).ToDictionary(item => item.Id, item => item.Count) ?? [];
            var taskIds = new List<int>();
            for (int i = 0; i < depot.PlanList.Count; i++)
            {
                var plan = depot.PlanList[i];
                if (string.IsNullOrEmpty(plan.DropId) || plan.DropCount <= 0)
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: invalid drop item.", UiLogColor.Error);
                    taskIds.Add(0);
                    continue;
                }
                var count = depotList.TryGetValue(plan.DropId, out var value) ? value : 0;
                count = plan.DropCount - count;
                if (count <= 0)
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: Inventory enough.", UiLogColor.Info);
                    taskIds.Add(0);
                    continue;
                }
                var stage = FightSettingsUserControlModel.GetFightStage([plan.Stage]);
                if (string.IsNullOrEmpty(stage))
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: stage '{plan.Stage}' is not open.", UiLogColor.Error);
                    taskIds.Add(0);
                    continue;
                }
                var fight = new AsstFightTask() {
                    Stage = stage,
                    Drops = new() { { plan.DropId, count } },
                    MaxTimes = count > 0 ? int.MaxValue : 0,
                    Medicine = plan.UseMedicine ? plan.MedicineCount : 0,
                    Stone = plan.UseStone ? plan.StoneCount : 0,
                };
                var (ret, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, fight);
                if (!ret)
                {
                    Instances.TaskQueueViewModel.AddLog($"Plan {i + 1}: add task failed.", UiLogColor.Error);
                    taskIds.Add(0);
                }
                else
                {
                    taskIds.Add(id);
                }
            }

            if (taskIds.Any(id => id > 0))
            {
                return (true, taskIds);
            }
            return (null, []);
        }
    }
}
