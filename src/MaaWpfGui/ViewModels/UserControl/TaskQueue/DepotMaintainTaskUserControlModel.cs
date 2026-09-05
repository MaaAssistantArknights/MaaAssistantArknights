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
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using JetBrains.Annotations;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using MaaWpfGui.Utilities;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using ObservableCollections;
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
        PlanList.CollectionChanged += PlanList_CollectionChanged;

        // 仓库数据变化时刷新计划显示（当前库存数量）
        if (Instances.ToolboxViewModel is { } toolbox)
        {
            toolbox.DepotResult.CollectionChanged += (in NotifyCollectionChangedEventArgs<ToolboxViewModel.DepotResultDate> _) => {
                NotifyOfPropertyChange(nameof(PlanInfo));
            };
        }

        // 任务开始时用最新库存重算该 plan 的缺口；任务正常结束但未达标时记录临期药耗尽证明
        Instances.AsstProxy.OnTaskStatusChanged += OnTaskStatusChanged;

        // 语言切换时由 FightSettings.RebuildDropsList 显式调用 OnLanguageChanged
    }

    public void OnLanguageChanged()
    {
        // 刷新预设列表的本地化显示
        PresetList.RefreshLocalization();

        // DropsList 已由 RebuildDropsList 原地更新 Display（不增删项），ComboBox SelectedValue 不会丢失
        // 刷新各 plan 的 DropName（包括未选择时的占位文本，也需随语言切换）
        foreach (var plan in PlanList)
        {
            var newName = !string.IsNullOrEmpty(plan.DropId)
                ? ItemListHelper.GetItemName(plan.DropId) ?? LocalizationHelper.GetString("NotSelected")
                : LocalizationHelper.GetString("NotSelected");
            plan.DropName = newName;
        }

        // 刷新关卡列表（关卡名随语言变化）
        RefreshStageList();
    }

    private void OnTaskStatusChanged(int taskId, TaskItemStatus status)
    {
        if (taskId <= 0)
        {
            return;
        }

        var task = ConfigFactory.CurrentConfig.TaskQueue.OfType<DepotMaintainTask>().FirstOrDefault(t => t.PlanList.Any(p => p.TaskId == taskId));
        if (task == null || task.PlanList.FirstOrDefault(plan => plan.TaskId == taskId) is not { } plan)
        {
            return;
        }

        if (string.IsNullOrEmpty(plan.DropId) || plan.DropCount <= 0)
        {
            return;
        }

        if (status == TaskItemStatus.Completed)
        {
            // 库存任务无次数上限，未达库存目标即正常结束说明理智打不动了，记录其临期药窗口为已耗尽
            UpdateProvenExhaustedMedicineDays(
                task.UseExpiringMedicine ? DepotMaintainTask.ExpiringMedicineDays : 0,
                plan.DropId,
                plan.DropCount);
            return;
        }

        if (status != TaskItemStatus.InProgress)
        {
            return;
        }

        // 复用 FightSettings 的公共方法，用最新库存重算缺口
        var stage = GetFightStage([plan.Stage]);
        if (!string.IsNullOrEmpty(stage))
        {
            var fight = new AsstFightTask() {
                Stage = stage,
                Medicine = task.UseMedicine && plan.UseMedicine ? plan.MedicineCount : 0,
                Stone = task.UseStone && plan.UseStone ? plan.StoneCount : 0,
                MedicineExpireDays = task.UseExpiringMedicine ? DepotMaintainTask.ExpiringMedicineDays : 0,
                Series = task.UseAutoSeries ? 0 : 1,
                MaxTimes = int.MaxValue,
                ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
                ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
                PenguinId = SettingsViewModel.GameSettings.PenguinId,
                YituliuId = SettingsViewModel.GameSettings.PenguinId,
                ServerType = Instances.SettingsViewModel.ServerType,
                ClientType = SettingsViewModel.GameSettings.ClientType,
            };
            RefreshFightTaskDrops(taskId, plan.DropId, plan.DropCount,
                $"{task.PlanList.IndexOf(plan) + 1}",
                fight);
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

    /// <summary>
    /// Gets or sets a value indicating whether 使用 AUTO 代理倍率（Series = 0）。
    /// 默认关闭（按 1 倍刷取）；开启后单次进入可能因高倍率超过目标库存上限。
    /// </summary>
    public bool UseAutoSeries
    {
        get => GetTaskConfig<DepotMaintainTask>().UseAutoSeries;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UseAutoSeries == value, t => t.UseAutoSeries = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 启用「使用药剂」勾选框。
    /// 默认开启；关闭后各 Plan 不显示药剂行，序列化时强制传 0。
    /// </summary>
    public bool UseMedicine
    {
        get => GetTaskConfig<DepotMaintainTask>().UseMedicine;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UseMedicine == value, t => t.UseMedicine = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 启用「使用源石」勾选框。
    /// 默认开启；关闭后各 Plan 不显示源石行，序列化时强制传 0。
    /// </summary>
    public bool UseStone
    {
        get => GetTaskConfig<DepotMaintainTask>().UseStone;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UseStone == value, t => t.UseStone = value);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 使用 48 小时内过期的理智药。
    /// 默认关闭；开启后所有 Plan 均使用临期药（固定 2 天阈值）。
    /// </summary>
    public bool UseExpiringMedicine
    {
        get => GetTaskConfig<DepotMaintainTask>().UseExpiringMedicine;
        set => SetTaskConfig<DepotMaintainTask>(t => t.UseExpiringMedicine == value, t => t.UseExpiringMedicine = value);
    }

    public ObservableCollection<DepotPlanItemViewModel> PlanList { get; private set => SetAndNotify(ref field, value); } = [];

    public void AddPlan()
    {
        PlanList.Add(new() { Index = PlanList.Count, });
    }

    public void RemovePlan(DepotPlanItemViewModel plan)
    {
        PlanList.Remove(plan);
    }

    public void ClearPlans()
    {
        if (PlanList.Count == 0)
        {
            return;
        }

        var result = MessageBoxHelper.Show(
            LocalizationHelper.GetString("ClearPlansConfirm"),
            LocalizationHelper.GetString("Warning"),
            MessageBoxButton.YesNo,
            MessageBoxImage.Warning);
        if (result != MessageBoxResult.Yes)
        {
            return;
        }

        PlanList.Clear();
    }

    /// <summary>
    /// 预设列表。
    /// </summary>
    public static LocalizedObservableList<string> PresetList { get; } = new(
        ("Chip1", "DepotPresetChip1"),
        ("Chip2", "DepotPresetChip2"),
        ("CE6", "DepotPresetLmd"),
        ("AP5", "DepotPresetCertificate"),
        ("CA5", "DepotPresetSkillSummary"));

    /// <summary>
    /// 预设数据：关卡 → [(掉落物 itemId, 掉落物名称), ...]。
    /// </summary>
    private static readonly Dictionary<string, (string Stage, string[] Drops, int DefaultCount)[]> PresetData = new() {
        ["Chip1"] = [
            ("PR-A-1", ["3261", "3231"], 20),  // 医疗芯片、重装芯片
            ("PR-B-1", ["3251", "3241"], 20),  // 术师芯片、狙击芯片
            ("PR-C-1", ["3211", "3271"], 20),  // 先锋芯片、辅助芯片
            ("PR-D-1", ["3221", "3281"], 20),  // 近卫芯片、特种芯片
        ],
        ["Chip2"] = [
            ("PR-A-2", ["3262", "3232"], 20),  // 医疗芯片组、重装芯片组
            ("PR-B-2", ["3252", "3242"], 20),  // 术师芯片组、狙击芯片组
            ("PR-C-2", ["3212", "3272"], 20),  // 先锋芯片组、辅助芯片组
            ("PR-D-2", ["3222", "3282"], 20),  // 近卫芯片组、特种芯片组
        ],
        ["CE6"] = [("CE-6", ["4001"], 2000000)],    // 龙门币
        ["AP5"] = [("AP-5", ["4006"], 5000)],       // 采购凭证（红票）
        ["CA5"] = [("CA-5", ["3303"], 200)],        // 技巧概要·卷3
    };

    public void AddPresetPlan(string presetValue)
    {
        if (!PresetData.TryGetValue(presetValue, out var stages))
        {
            return;
        }

        var list = PlanList.ToList();
        foreach (var (stage, drops, defaultCount) in stages)
        {
            foreach (var dropId in drops)
            {
                var dropName = ItemListHelper.GetItemName(dropId) ?? LocalizationHelper.GetString("NotSelected");
                var plan = new DepotPlanItemViewModel(stage, dropId, dropName, defaultCount);
                plan.PropertyChanged += PlanItem_PropertyChanged;
                list.Add(plan);
            }
        }

        PlanList = new(list);
        PlanList.CollectionChanged += PlanList_CollectionChanged;

        // new(list) 的 Reset 发生在构造阶段，此时 CollectionChanged 尚未挂载，需手动重排一次
        ReindexPlans();
        SyncPlanListToTaskConfig();
        NotifyOfPropertyChange(nameof(PlanInfo));
    }

    /// <summary>
    /// Menu 的 MenuItem.Click 事件处理，从点击的菜单项 DataContext 提取预设 Value。
    /// </summary>
    /// <param name="sender">事件发送者。</param>
    /// <param name="e">路由事件参数。</param>
    [UsedImplicitly]
    public void PresetMenuClick(object sender, RoutedEventArgs e)
    {
        if (e.OriginalSource is MenuItem { DataContext: GenericCombinedData<string> item })
        {
            AddPresetPlan(item.Value);
        }
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

    [PropertyDependsOn(typeof(GuiSettingsUserControlModel), nameof(GuiSettingsUserControlModel.Language))]
    public string PlanInfo => string.Join("\n", PlanList.Select((t, i) => $"{i + 1}: {StageListSource.FirstOrDefault(i => i.Value == t.Stage)?.Display ?? t.Stage} - {t.DropName} {GetCurrentInventoryCount(t.DropId).FormatNumber(false)}/{t.DropCount.FormatNumber(false)}"));

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
            try
            {
                RefreshStageList();
            }
            finally
            {
                TaskQueueViewModel.TaskQueueSerializingLock.Release();
            }
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

    public override void RefreshUI(BaseTask baseTask)
    {
        if (baseTask is not DepotMaintainTask task)
        {
            return;
        }

        using var refresh = new UiRefreshingScope();
        var list = new List<DepotPlanItemViewModel>();
        int index = 0;
        foreach (var plan in task.PlanList)
        {
            // 根据 DropId 从掉落列表恢复 DropName，避免初始化显示为"不选择"
            var dropName = FightSettingsUserControlModel.Instance.DropsList.FirstOrDefault(i => i.Value == plan.DropId)?.Display ?? LocalizationHelper.GetString("NotSelected");

            var uiPlan = new DepotPlanItemViewModel(plan.Stage, plan.DropId, dropName, plan.DropCount, plan.UseMedicine, plan.MedicineCount, plan.UseStone, plan.StoneCount, plan.TaskId) {
                Index = index,
            };
            list.Add(uiPlan);
            uiPlan.PropertyChanged += PlanItem_PropertyChanged;
            ++index;
        }
        foreach (var plan in PlanList)
        {
            plan.PropertyChanged -= PlanItem_PropertyChanged;
        }
        PlanList = [.. list];
        PlanList.CollectionChanged += PlanList_CollectionChanged;
        NotifyOfPropertyChange(nameof(PlanInfo));
        RefreshStageList();
        Refresh();
    }

    private void PlanItem_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (!IsRefreshingUI)
        {
            if (e.PropertyName is not nameof(DepotPlanItemViewModel.IsExpanded) and not nameof(DepotPlanItemViewModel.Title) and not nameof(DepotPlanItemViewModel.Index) && sender is DepotPlanItemViewModel plan)
            {
                var list = GetTaskConfig<DepotMaintainTask>().PlanList.ToList();
                if (plan.Index < 0 || plan.Index >= list.Count)
                {
                    _logger.Warning("PlanItem_PropertyChanged: index {Index} out of range (Count={Count}), skip and resync", plan.Index, list.Count);
                    SyncPlanListToTaskConfig();
                }
                else
                {
                    list[plan.Index] = new DepotMaintainTask.Plan(plan.Stage, plan.DropId, plan.DropCount, plan.UseMedicine, plan.MedicineCount, plan.UseStone, plan.StoneCount, plan.TaskId);
                    SetTaskConfig<DepotMaintainTask>(t => t.PlanList.SequenceEqual(list), t => t.PlanList = list);
                }
            }
        }
        if (e.PropertyName is nameof(DepotPlanItemViewModel.Title))
        {
            NotifyOfPropertyChange(nameof(PlanInfo));
        }
    }

    private void PlanList_CollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        NotifyOfPropertyChange(nameof(PlanInfo));
        if (e.Action is NotifyCollectionChangedAction.Remove or NotifyCollectionChangedAction.Replace)
        {
            e.OldItems?.OfType<DepotPlanItemViewModel>().ToList().ForEach(plan => {
                plan.PropertyChanged -= PlanItem_PropertyChanged;
            });
        }
        if (e.Action is NotifyCollectionChangedAction.Add or NotifyCollectionChangedAction.Replace)
        {
            e.NewItems?.OfType<DepotPlanItemViewModel>().ToList().ForEach(plan => {
                plan.PropertyChanged += PlanItem_PropertyChanged;
            });
        }
        ReindexPlans();
        SyncPlanListToTaskConfig();
    }

    /// <summary>
    /// 按当前 <see cref="PlanList"/> 顺序重排每项 <see cref="DepotPlanItemViewModel.Index"/>。
    /// Title 依赖 Index，Fody 已在 Index setter 织入 Title 的通知，赋值即自动刷新显示。
    /// </summary>
    private void ReindexPlans()
    {
        foreach (var (plan, index) in PlanList.Select((plan, index) => (plan, index)))
        {
            plan.Index = index;
        }
    }

    /// <summary>
    /// 将 <see cref="PlanList"/> 同步回 <see cref="DepotMaintainTask.PlanList"/>。
    /// 供 <see cref="PlanList_CollectionChanged"/>、<see cref="ClearPlans"/>、<see cref="AddPresetPlan"/>
    /// 等挂起 CollectionChanged 的批量操作复用，替代已删除的 SavePlan。
    /// </summary>
    private void SyncPlanListToTaskConfig()
    {
        var list = PlanList.Select(plan => new DepotMaintainTask.Plan(plan.Stage, plan.DropId, plan.DropCount, plan.UseMedicine, plan.MedicineCount, plan.UseStone, plan.StoneCount, plan.TaskId)).ToList();
        SetTaskConfig<DepotMaintainTask>(t => t.PlanList.SequenceEqual(list), t => t.PlanList = list);
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
                var (result, depotTaskId) = Instances.AsstProxy.AsstAppendTaskWithEncoding(TaskType.Depot, (AsstTaskType.Depot, null));
                if (!result)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetString("DepotPlanUpdateDepotFailed"), UiLogColor.Error);
                    return (false, []);
                }
                Instances.ToolboxViewModel.MarkDepotRecognitionSyncTimeForReset(depotTaskId);
                taskIds.Add(depotTaskId);
            }

            Instances.TaskQueueViewModel.AddLogSection(depot.NameOrTaskType);

            var depotList = Instances.ToolboxViewModel?.DepotResult.Where(item => item.Count >= 0).ToDictionary(item => item.Id, item => item.Count) ?? [];
            for (int i = 0; i < depot.PlanList.Count; i++)
            {
                var plan = depot.PlanList[i];
                depot.PlanList[i] = plan with { TaskId = 0 }; // 主动重置 TaskId
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

                var currentCount = depotList.TryGetValue(plan.DropId, out var value) ? value : 0;
                var need = plan.DropCount - currentCount;
                var dropName = ItemListHelper.GetItemName(plan.DropId) ?? plan.DropId;
                if (need <= 0)
                {
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanInventoryEnough", i + 1, dropName, currentCount.ToString("N0"), plan.DropCount.ToString("N0")));
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

                var fight = new AsstFightTask() {
                    Stage = stage,
                    Drops = new() { { plan.DropId, need } },
                    MaxTimes = need > 0 ? int.MaxValue : 0,
                    Medicine = depot.UseMedicine && plan.UseMedicine ? plan.MedicineCount : 0,
                    Stone = depot.UseStone && plan.UseStone ? plan.StoneCount : 0,
                    MedicineExpireDays = depot.UseExpiringMedicine ? DepotMaintainTask.ExpiringMedicineDays : 0,
                    Series = depot.UseAutoSeries ? 0 : 1,
                    ReportToPenguin = SettingsViewModel.GameSettings.EnablePenguin,
                    ReportToYituliu = SettingsViewModel.GameSettings.EnableYituliu,
                    PenguinId = SettingsViewModel.GameSettings.PenguinId,
                    YituliuId = SettingsViewModel.GameSettings.PenguinId,
                    ServerType = Instances.SettingsViewModel.ServerType,
                    ClientType = SettingsViewModel.GameSettings.ClientType,
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
                    depot.PlanList[i] = plan with { TaskId = id };
                    Instances.TaskQueueViewModel.AddLog(LocalizationHelper.GetStringFormat("DepotPlanInventoryInsufficient", i + 1, dropName, currentCount.ToString("N0"), plan.DropCount.ToString("N0"), need.ToString("N0")));
                }
            }

            Instances.TaskQueueViewModel.AddLog(string.Empty, splitMode: TaskQueueViewModel.LogCardSplitMode.Before);

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
