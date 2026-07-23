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
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Utilities;
using MaaWpfGui.ViewModels.UI;
using ObservableCollections;
using Serilog;
using Stylet;
using static MaaWpfGui.Main.AsstProxy;
using static MaaWpfGui.ViewModels.UserControl.TaskQueue.FightSettingsUserControlModel;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class DepotMaintainTaskUserControlModel : TaskSettingsViewModel, DepotMaintainTaskUserControlModel.ISerialize
{
    private readonly ILogger _logger = Log.ForContext<DepotMaintainTaskUserControlModel>();

    // plan index → core taskId 的映射，用于运行时重算缺口
    private Dictionary<int, int> _planTaskIdMap = [];

    static DepotMaintainTaskUserControlModel()
    {
        Instance = new();
    }

    public DepotMaintainTaskUserControlModel()
    {
        PlanList.CollectionChanged += (_, __) =>
        {
            SavePlan();
            NotifyOfPropertyChange(nameof(PlanInfo));
        };

        // 仓库数据变化时刷新计划显示（当前库存数量）
        if (Instances.ToolboxViewModel is { } toolbox)
        {
            toolbox.DepotResult.CollectionChanged += (in NotifyCollectionChangedEventArgs<ToolboxViewModel.DepotResultDate> _) =>
            {
                NotifyOfPropertyChange(nameof(PlanInfo));
            };
        }

        // 任务状态变化时，用最新库存重算该 plan 的缺口
        Instances.AsstProxy.OnTaskStatusChanged += OnTaskStatusChanged;
    }

    private void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        if (status != TaskItemStatus.InProgress || taskId <= 0)
        {
            return;
        }

        // 查找该 taskId 对应的 plan index
        var planIndex = _planTaskIdMap.FirstOrDefault(kvp => kvp.Value == taskId).Key;
        if (TaskSettingVisibilityInfo.CurrentTask is not DepotMaintainTask depot || planIndex < 0 || planIndex >= depot.PlanList.Count)
        {
            return;
        }

        var plan = depot.PlanList[planIndex];
        if (string.IsNullOrEmpty(plan.DropId) || plan.DropCount <= 0)
        {
            return;
        }

        // 复用 FightSettings 的公共方法，用最新库存重算缺口
        var stage = GetFightStage([plan.Stage]);
        if (!string.IsNullOrEmpty(stage))
        {
            FightSettingsUserControlModel.RefreshFightTaskDrops(taskId, plan.DropId, plan.DropCount,
                stage,
                plan.UseMedicine ? plan.MedicineCount : 0,
                plan.UseStone ? plan.StoneCount : 0,
                (planIndex + 1).ToString());
        }
    }

    public static DepotMaintainTaskUserControlModel Instance { get; }

    public bool UpdateDepot
    {
        get => GetTaskConfig<DepotMaintainTask>().UpdateDepot;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UpdateDepot == value, t => t.UpdateDepot = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 活动期间跳过整个库存保持任务。
    /// </summary>
    public bool SkipDuringActivity
    {
        get => GetTaskConfig<DepotMaintainTask>().SkipDuringActivity;
        set => SetTaskConfig<DepotMaintainTask>(t => t.SkipDuringActivity == value, t => t.SkipDuringActivity = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 资源全开放期间跳过整个库存保持任务。
    /// </summary>
    public bool SkipDuringResourceCollection
    {
        get => GetTaskConfig<DepotMaintainTask>().SkipDuringResourceCollection;
        set => SetTaskConfig<DepotMaintainTask>(t => t.SkipDuringResourceCollection == value, t => t.SkipDuringResourceCollection = value);
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

    public string PlanInfo => string.Join("\n", PlanList.Select((t, i) => $"{i + 1}: {StageListSource.FirstOrDefault(i => i.Value == t.Stage)?.Display ?? t.Stage} - {t.DropName} {GetCurrentInventoryCount(t.DropId)}/{t.DropCount}"));

    /// <summary>
    /// 获取指定掉落物当前库存数量，无数据时返回 "--"。
    /// </summary>
    private static string GetCurrentInventoryCount(string dropId)
    {
        if (string.IsNullOrEmpty(dropId))
        {
            return "--";
        }

        var depot = Instances.ToolboxViewModel?.DepotResult;
        if (depot == null || depot.Count == 0)
        {
            return "--";
        }

        var item = depot.FirstOrDefault(i => i.Id == dropId);
        return item?.Count >= 0 ? item.Count.ToString() : "--";
    }

    /// <summary>
    /// 单个 Plan 属性变化时，保存配置并通知 UI 刷新。
    /// </summary>
    /// <param name="notifyProperties">需要通知刷新的属性名。</param>
    public void OnPlanChanged(params string[] notifyProperties)
    {
        if (IsRefreshingUI)
        {
            return;
        }

        SavePlan();
        foreach (var prop in notifyProperties)
        {
            NotifyOfPropertyChange(prop);
        }
    }

    private void SavePlan()
    {
        if (IsRefreshingUI)
        {
            return;
        }

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

        using var refresh = new UiRefreshingScope();
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
                NotifyOfPropertyChange(nameof(Title));
                Instance.OnPlanChanged(nameof(PlanInfo));
            }
        } = string.Empty;

        /// <summary>
        /// Gets or sets 指定掉落材料 ID。
        /// </summary>
        public string DropId
        {
            get; set {
                SetAndNotify(ref field, value);
                NotifyOfPropertyChange(nameof(Title));
                Instance.OnPlanChanged(nameof(PlanInfo));
            }
        } = string.Empty;

        /// <summary>
        /// Gets or sets 指定掉落材料名称。
        /// </summary>
        public string DropName
        {
            get => field;
            set
            {
                SetAndNotify(ref field, value);
                NotifyOfPropertyChange(nameof(Title));
                Instance.OnPlanChanged(nameof(PlanInfo));
            }
        } = LocalizationHelper.GetString("NotSelected");

        public int DropCount
        {
            get; set {
                SetAndNotify(ref field, value);
                NotifyOfPropertyChange(nameof(Title));
                Instance.OnPlanChanged(nameof(PlanInfo));
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

        using var refresh = new UiRefreshingScope();
        var list = new List<Plan>();
        foreach (var plan in task.PlanList)
        {
            var uiPlan = new Plan {
                Stage = plan.Stage,
                DropId = plan.DropId,
                DropCount = plan.DropCount,
                UseMedicine = plan.UseMedicine,
                MedicineCount = plan.MedicineCount,
                UseStone = plan.UseStone,
                StoneCount = plan.StoneCount,

                // 根据 DropId 从掉落列表恢复 DropName，避免初始化显示为"不选择"
                DropName = FightSettingsUserControlModel.Instance.DropsList.FirstOrDefault(i => i.Value == plan.DropId)?.Display
                    ?? LocalizationHelper.GetString("NotSelected"),
            };
            list.Add(uiPlan);
        }
        PlanList = [.. list];
        PlanList.CollectionChanged += (_, __) =>
        {
            SavePlan();
            NotifyOfPropertyChange(nameof(PlanInfo));
        };
        NotifyOfPropertyChange(nameof(PlanInfo));
        RefreshStageList();
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

            // 活动期间跳过：当有 SideStory 活动进行中时跳过整个库存保持任务
            if (depot.SkipDuringActivity && Instances.StageManager.IsActivityOpen())
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DepotPlanSkippedActivity"), UiLogColor.Info);
                return (null, []);
            }

            // 资源全开放期间跳过：当资源全开放活动进行中时跳过整个库存保持任务
            if (depot.SkipDuringResourceCollection && Instances.StageManager.IsResourceCollectionOpen())
            {
                Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DepotPlanSkippedResourceCollection"), UiLogColor.Info);
                return (null, []);
            }

            var taskIds = new List<int>();

            // 任务开始前更新库存数据：先追加仓库识别任务，刷新库存后再执行计划
            // 每个 plan 开始时会通过 OnTaskStatusChanged 用最新库存重算缺口
            if (depot.UpdateDepot)
            {
                if (!Instances.ToolboxViewModel.StartDepotRecognitionTask(startImmediately: false))
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DepotPlanUpdateDepotFailed"), UiLogColor.Error);
                    return (false, []);
                }

                int depotTaskId = Instances.AsstProxy.TasksStatus.Last().Key;
                Instances.ToolboxViewModel.MarkDepotRecognitionSyncTimeForReset(depotTaskId);
                taskIds.Add(depotTaskId);
            }

            var depotList = Instances.ToolboxViewModel?.DepotResult.Where(item => item.Count >= 0).ToDictionary(item => item.Id, item => item.Count) ?? [];
            Instance._planTaskIdMap = [];
            for (int i = 0; i < depot.PlanList.Count; i++)
            {
                var plan = depot.PlanList[i];
                if (string.IsNullOrEmpty(plan.DropId))
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanInvalidDropItem", i + 1), UiLogColor.Error);
                    taskIds.Add(0);
                    continue;
                }
                if (plan.DropCount <= 0)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanZeroDropCount", i + 1), UiLogColor.Error);
                    taskIds.Add(0);
                    continue;
                }

                var stage = GetFightStage([plan.Stage]);
                if (string.IsNullOrEmpty(stage))
                {
                    if (string.IsNullOrEmpty(plan.Stage))
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanNoStage", i + 1), UiLogColor.Error);
                    }
                    else
                    {
                        Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanStageNotOpen", i + 1, plan.Stage));
                    }

                    taskIds.Add(0);
                    continue;
                }

                var currentCount = depotList.TryGetValue(plan.DropId, out var value) ? value : 0;
                var need = plan.DropCount - currentCount;
                if (need <= 0)
                {
                    var dropName = ItemListHelper.GetItemName(plan.DropId) ?? plan.DropId;
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanInventoryEnough", i + 1, dropName, currentCount, plan.DropCount));
                    taskIds.Add(0);
                    continue;
                }

                var fight = new AsstFightTask() {
                    Stage = stage,
                    Drops = new() { { plan.DropId, need } },
                    MaxTimes = need > 0 ? int.MaxValue : 0,
                    Medicine = plan.UseMedicine ? plan.MedicineCount : 0,
                    Stone = plan.UseStone ? plan.StoneCount : 0,
                };
                var (ret, id) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Fight, fight);
                if (!ret)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanAddTaskFailed", i + 1), UiLogColor.Error);
                    taskIds.Add(0);
                }
                else
                {
                    taskIds.Add(id);
                    Instance._planTaskIdMap[i] = id;
                }
            }

            if (taskIds.Any(id => id > 0))
            {
                return (true, taskIds);
            }
            return (null, []);
        }
    }

    /// <summary>
    /// UI 刷新作用域，防止刷新期间 ComboBox 等控件的绑定回写覆盖配置。
    /// </summary>
    private struct UiRefreshingScope : IDisposable
    {
        private static int _depth = 0;

        public UiRefreshingScope()
        {
            ++_depth;
            Instance.IsRefreshingUI = true;
        }

        readonly void IDisposable.Dispose()
        {
            --_depth;
            if (_depth == 0)
            {
                Instance.IsRefreshingUI = false;
            }
        }
    }
}
