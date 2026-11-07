// <copyright file="TutorialChallengeLevelViewModel.cs" company="MaaAssistantArknights">
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
using System.Collections.Specialized;
using System.ComponentModel;
using System.Linq;
using System.Text.Json;
using System.Windows;
using System.Windows.Input;
using HandyControl.Controls;
using HandyControl.Data;
using HandyControl.Tools.Command;
using MaaWpfGui.Configuration.Single.MaaTask;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models;
using MaaWpfGui.Models.TutorialChallenge;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using MaaWpfGui.ViewModels.UserControl.Toolbox;
using MaaWpfGui.Views.Dialogs;
using Stylet;

namespace MaaWpfGui.ViewModels.Dialogs;

/// <summary>
/// 教程关卡操作场的视图模型。持有纯内存沙盒任务队列（不进入真实任务队列、不持久化），
/// 沙盒任何变更即时重算终态判定，达成即记录通关并发放牛牛币。
/// 条目与面板的形状对齐真实任务队列（TaskItemViewModel / FightTaskUserControlModel），仅数据源隔离。
/// </summary>
public class TutorialChallengeLevelViewModel : PropertyChangedBase
{
    private readonly TutorialChallengeLevel _level;
    private readonly PropertyChangedEventHandler _taskPropertyChangedHandler;
    private readonly Random _random = new();
    private readonly Dictionary<string, int> _fakeInventories = [];
    private bool _isCompleted;
    private TutorialChallengeSandboxItem? _selectedItem;
    private string _stagePlanTip = string.Empty;
    private string _dropsItemName = string.Empty;

    /// <summary>
    /// Initializes a new instance of the <see cref="TutorialChallengeLevelViewModel"/> class.
    /// </summary>
    /// <param name="level">关卡定义。</param>
    public TutorialChallengeLevelViewModel(TutorialChallengeLevel level)
    {
        _level = level;
        _taskPropertyChangedHandler = (sender, e) =>
        {
            switch (e.PropertyName)
            {
                case nameof(FightTask.UseOptionalStage):
                    RefreshPanelState();
                    NotifyOfPropertyChange(nameof(SandboxUseOptionalStage));
                    break;
                case nameof(FightTask.StagePlan):
                    // 行内编辑写回的计划与现有行一致时不重建，否则正在输入的关卡框随行容器销毁而丢焦点
                    if (sender is FightTask fight && !StagePlanRows.Select(row => row.Stage).SequenceEqual(fight.StagePlan))
                    {
                        RefreshPanelState();
                    }

                    break;
                case nameof(FightTask.DropId):
                    SyncDropsItemName();
                    NotifySpecifiedDropsStateChanged();
                    RefreshInventoryDisplay();
                    break;
                case nameof(FightTask.EnableTargetDrop):
                case nameof(FightTask.IsInventoryTarget):
                case nameof(FightTask.DropCount):
                    NotifySpecifiedDropsStateChanged();
                    RefreshInventoryDisplay();
                    break;
                case nameof(FightTask.UseStone):
                    NotifyOfPropertyChange(nameof(SandboxUseStoneDisplay));
                    break;
            }

            RefreshStagePlanTip();
            Evaluate();
        };

        // 添加任务/预设导入菜单的条目命令经 ItemContainerStyle 统一下发，s:Action 无法进入 Style Setter，故走 ICommand
        AddTaskCommand = new RelayCommand<Type>(AddTask);
        ImportPresetCommand = new RelayCommand<string>(ImportPreset);

        SandboxItems.CollectionChanged += OnSandboxItemsChanged;
        StagePlanRows.CollectionChanged += OnStagePlanRowsChanged;
        PlanRows.CollectionChanged += OnPlanRowsChanged;
        foreach (var item in WeeklyScheduleSource)
        {
            item.PropertyChanged += OnWeeklyScheduleItemPropertyChanged;
        }

        LocalizationHelper.LanguageChanged += OnLanguageChanged;
        Instances.StageManager.StageDataChanged += OnStageDataChanged;
        BuildStageListSource();
        BuildSandbox();
    }

    /// <summary>
    /// Gets 添加任务菜单命令。
    /// </summary>
    public ICommand AddTaskCommand { get; }

    /// <summary>
    /// Gets 预设导入菜单命令。
    /// </summary>
    public ICommand ImportPresetCommand { get; }

    /// <summary>
    /// Gets 当前选中理智作战任务指定掉落材料的伪造库存文本。
    /// </summary>
    public string CurrentDropsInventoryText =>
        SelectedFightTask is { DropId.Length: > 0 } fight ? FakeInventoryFor(fight.DropId).ToString() : "--";

    /// <summary>
    /// Gets 当前选中理智作战任务在库存目标模式下的有效数量文本（还需刷多少个）。
    /// </summary>
    public string EffectiveDropsQuantityText =>
        SelectedFightTask is { } fight
            ? Math.Max(0, fight.DropCount - FakeInventoryFor(fight.DropId)).ToString()
            : string.Empty;

    /// <summary>
    /// Gets 或设置 a value indicating whether 备选关卡行拖拽进行中（拖拽期间行内 ComboBox 停用，避免误触）。
    /// </summary>
    public bool IsStageItemDragging
    {
        get;
        set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets 沙盒任务条目列表，操作与真实任务队列同构，数据仅存在于本关卡窗口生命周期内。
    /// </summary>
    public ObservableCollection<TutorialChallengeSandboxItem> SandboxItems { get; } = [];

    /// <summary>
    /// Gets 或设置当前通过齿轮选中的沙盒任务条目，右侧设置面板随其切换。
    /// </summary>
    public TutorialChallengeSandboxItem? SelectedItem
    {
        get => _selectedItem;
        set {
            if (ReferenceEquals(_selectedItem, value))
            {
                return;
            }

            _selectedItem = value;
            foreach (var item in SandboxItems)
            {
                item.EnableSetting = ReferenceEquals(item, value);
            }

            EnableAdvancedSettings = false;
            NotifySelectedChanged();
            RefreshPanelState();
        }
    }

    /// <summary>
    /// Gets 选中的沙盒任务实例。
    /// </summary>
    public BaseTask? SelectedTask => SelectedItem?.Task;

    /// <summary>
    /// Gets 选中的理智作战沙盒任务，非理智作战时为 null。
    /// </summary>
    public FightTask? SelectedFightTask => SelectedTask as FightTask;

    /// <summary>
    /// Gets 选中的库存保持沙盒任务，非库存保持时为 null。
    /// </summary>
    public DepotMaintainTask? SelectedDepotMaintainTask => SelectedTask as DepotMaintainTask;

    /// <summary>
    /// Gets a value indicating whether 本关已达成。已达成后判定停算，重进或重置沙盒可重玩，但不再重复发放牛牛币。
    /// </summary>
    public bool IsCompleted
    {
        get => _isCompleted;
        private set => SetAndNotify(ref _isCompleted, value);
    }

    /// <summary>
    /// Gets 或设置 a value indicating whether 设置面板切到高级区（沙盒局部态，对齐真实面板的 EnableAdvancedSettings 切换行为）。
    /// </summary>
    public bool EnableAdvancedSettings
    {
        get;
        set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets 连战次数候选列表。只读展示数据，直接共用真实面板的实例，语言切换由其刷新。
    /// </summary>
    public LocalizedObservableList<int> SeriesList => FightTaskUserControlModel.Instance.SeriesList;

    /// <summary>
    /// Gets 剿灭模式候选列表，共用真实面板的实例。
    /// </summary>
    public LocalizedObservableList<string> AnnihilationModeList => FightTaskUserControlModel.Instance.AnnihilationModeList;

    /// <summary>
    /// Gets 关卡重置模式候选列表，共用真实面板的实例。
    /// </summary>
    public LocalizedObservableList<FightStageResetMode> StageResetModeList => FightTaskUserControlModel.Instance.StageResetModeList;

    /// <summary>
    /// Gets 关卡名。
    /// </summary>
    public string Title => LocalizationHelper.GetString(_level.TitleKey);

    /// <summary>
    /// Gets 关卡目标描述。
    /// </summary>
    public string Goal => LocalizationHelper.GetString(_level.GoalKey);

    /// <summary>
    /// Gets 操作场窗口标题（模块名 - 关卡名）。
    /// </summary>
    public string WindowTitle => $"{LocalizationHelper.GetString("Toolbox.TutorialChallenge.Title")} - {Title}";

    /// <summary>
    /// Gets 操作场右侧设置面板的种类。
    /// </summary>
    public TutorialChallengePanelKind PanelKind => _level.PanelKind;

    /// <summary>
    /// Gets a value indicating whether 显示理智作战沙盒面板。
    /// </summary>
    public bool ShowFightPanel => PanelKind == TutorialChallengePanelKind.Fight && SelectedFightTask != null;

    /// <summary>
    /// Gets a value indicating whether 显示库存保持沙盒面板。
    /// </summary>
    public bool ShowDepotMaintainPanel => PanelKind == TutorialChallengePanelKind.DepotMaintain && SelectedDepotMaintainTask != null;

    /// <summary>
    /// Gets ｢+｣ 添加任务菜单的条目，按 <see cref="TutorialChallengeLevel.AddableTaskTypes"/> 从全量任务类型过滤。
    /// </summary>
    public IEnumerable<TaskTypeItem> TaskTypeList => TaskQueueViewModel.TaskTypeList.Where(item => _level.AddableTaskTypes.Contains(item.Value));

    /// <summary>
    /// Gets 关卡候选列表，自建自 StageManager 的全量关卡（真实面板的 StageListSource 依赖真实队列当前选中任务，此处不可复用）。
    /// </summary>
    public ObservableCollection<FightTaskUserControlModel.StageSourceItem> StageListSource { get; } = [];

    /// <summary>
    /// Gets 指定掉落材料候选列表，共用真实面板的实例；该实例在语言切换时原地更新显示名，不会清掉 ComboBox 的选中项。
    /// </summary>
    public ObservableCollection<CombinedData> DropsList => FightTaskUserControlModel.Instance.DropsList;

    /// <summary>
    /// Gets 备选关卡列表行。
    /// </summary>
    public ObservableCollection<TutorialChallengeStagePlanRow> StagePlanRows { get; } = [];

    /// <summary>
    /// Gets 库存保持计划展示行，形状对齐真实面板的计划项（展开/编辑/删除）。
    /// </summary>
    public ObservableCollection<TutorialChallengePlanRow> PlanRows { get; } = [];

    /// <summary>
    /// Gets 周计划条目列表，形状对齐真实面板的 WeeklyScheduleSource。
    /// </summary>
    public ObservableCollection<WeeklyScheduleItem> WeeklyScheduleSource { get; } =
        [.. Enum.GetValues<DayOfWeek>().Select(day => new WeeklyScheduleItem(day))];

    /// <summary>
    /// Gets 或设置 关卡备选提示（当前会刷哪关），鼠标悬浮刷新。
    /// </summary>
    public string StagePlanTip
    {
        get => _stagePlanTip;
        private set => SetAndNotify(ref _stagePlanTip, value);
    }

    /// <summary>
    /// Gets 或设置 选中理智作战任务是否使用备选关卡（对 XAML 暴露的直接路径，等价 SelectedFightTask.UseOptionalStage）。
    /// </summary>
    public bool SandboxUseOptionalStage
    {
        get => SelectedFightTask?.UseOptionalStage ?? false;
        set {
            if (SelectedFightTask is not { } fight || fight.UseOptionalStage == value)
            {
                return;
            }

            fight.UseOptionalStage = value;
            if (value)
            {
                fight.HideUnavailableStage = false;
                fight.StageResetMode = FightStageResetMode.Ignore;
            }
            else
            {
                // 取消备选时按计划首行回填主关卡，其余备选行丢弃；无行则补一空行
                var first = StagePlanRows.Count > 0 ? StagePlanRows[0].Stage : string.Empty;
                StagePlanRows.Clear();
                StagePlanRows.Add(new TutorialChallengeStagePlanRow(this, first));
                SyncStagePlanFromRow();
            }
        }
    }

    /// <summary>
    /// Gets 或设置 选中理智作战任务是否启用指定掉落（对 XAML 暴露的直接路径，等价 SelectedFightTask.EnableTargetDrop）。
    /// </summary>
    public bool? SandboxEnableTargetDrop
    {
        get => SelectedFightTask?.EnableTargetDrop;
        set {
            if (SelectedFightTask is { } fight)
            {
                fight.EnableTargetDrop = value;
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 选中理智作战任务启用了指定掉落、为库存目标模式且已选定材料（对应真面板 IsInventoryTargetDropEnabled 的显示条件）。
    /// </summary>
    public bool SandboxIsInventoryTarget => SelectedFightTask is { EnableTargetDrop: true, IsInventoryTarget: true } fight
        && !string.IsNullOrEmpty(fight.DropId);

    /// <summary>
    /// Gets a value indicating whether 选中理智作战任务的源石勾选框不为 ｢未勾选｣（含右键切出的 null 态），用于停用理智药相关控件。
    /// </summary>
    public bool SandboxUseStoneDisplay => SelectedFightTask?.UseStone != false;

    /// <summary>
    /// Gets 指定掉落 ｢数量｣ 模式（已启用指定材料且非库存目标），语义同真实面板的同名包装属性。
    /// </summary>
    public bool UseDropQuantityMode
    {
        get => SelectedFightTask is { EnableTargetDrop: true, IsInventoryTarget: false };
        set {
            // 同组 RadioButton 取消选中时也会回写 false，只响应选中
            if (!value || SelectedFightTask is not { } fight)
            {
                return;
            }

            fight.EnableTargetDrop = true;
            fight.IsInventoryTarget = false;
        }
    }

    /// <summary>
    /// Gets 指定掉落 ｢目标库存｣ 模式（已启用指定材料且为库存目标），语义同真实面板的同名包装属性。
    /// </summary>
    public bool UseTargetInventoryMode
    {
        get => SelectedFightTask is { EnableTargetDrop: true, IsInventoryTarget: true };
        set {
            if (!value || SelectedFightTask is not { } fight)
            {
                return;
            }

            fight.EnableTargetDrop = true;
            fight.IsInventoryTarget = true;
        }
    }

    /// <summary>
    /// Gets or sets 当前选中理智作战任务的掉落材料名称（可编辑 ComboBox 的 Text 绑定）。
    /// 输入过程中只缓存文本，下拉关闭时由 <see cref="DropsListDropDownClosed"/> 反查回 DropId，避免搜索时逐字清空选中材料。
    /// </summary>
    public string DropsItemName
    {
        get => _dropsItemName;
        set => SetAndNotify(ref _dropsItemName, value);
    }

    /// <summary>
    /// 掉落材料下拉关闭时按输入文本反查材料，查不到则回到 ｢不选择｣。
    /// </summary>
    public void DropsListDropDownClosed()
    {
        if (SelectedFightTask is not { } fight)
        {
            return;
        }

        var match = DropsList.FirstOrDefault(d => string.Equals(d.Display, DropsItemName, StringComparison.Ordinal));
        fight.DropId = match?.Value ?? string.Empty;
        SyncDropsItemName();
    }

    /// <summary>
    /// 从添加任务菜单加入一个沙盒任务。
    /// </summary>
    /// <param name="taskType">任务类型。</param>
    public void AddTask(Type taskType)
    {
        // 开始唤醒任务至多一个，入口按钮禁用之外的行为兜底
        if (taskType == typeof(StartUpTask) && StartUpTaskCount >= 1)
        {
            return;
        }

        if (Activator.CreateInstance(taskType) is not BaseTask task)
        {
            return;
        }

        SandboxItems.Add(new TutorialChallengeSandboxItem(task));
    }

    /// <summary>
    /// 移除一个沙盒任务条目。
    /// </summary>
    /// <param name="item">条目。</param>
    public void RemoveTask(TutorialChallengeSandboxItem item)
    {
        SandboxItems.Remove(item);
        if (ReferenceEquals(SelectedItem, item))
        {
            SelectedItem = null;
        }
    }

    /// <summary>
    /// 勾选全部沙盒任务。
    /// </summary>
    public void SelectedAll()
    {
        foreach (var item in SandboxItems)
        {
            item.Task.IsEnable = true;
        }
    }

    /// <summary>
    /// 取消全部沙盒任务的勾选（对应真面板的 ｢全不选｣）。
    /// </summary>
    public void ClearAll()
    {
        foreach (var item in SandboxItems)
        {
            item.Task.IsEnable = false;
        }
    }

    /// <summary>
    /// 上移一个沙盒任务条目。
    /// </summary>
    /// <param name="item">条目。</param>
    public void MoveTaskUp(TutorialChallengeSandboxItem item)
    {
        var index = SandboxItems.IndexOf(item);
        if (index <= 0)
        {
            return;
        }

        SandboxItems.Move(index, index - 1);
    }

    /// <summary>
    /// 下移一个沙盒任务条目。
    /// </summary>
    /// <param name="item">条目。</param>
    public void MoveTaskDown(TutorialChallengeSandboxItem item)
    {
        var index = SandboxItems.IndexOf(item);
        if (index < 0 || index >= SandboxItems.Count - 1)
        {
            return;
        }

        SandboxItems.Move(index, index + 1);
    }

    /// <summary>
    /// 复制一个沙盒任务条目（完整克隆任务配置，插到原条目之后）。
    /// </summary>
    /// <param name="item">条目。</param>
    public void CopyTask(TutorialChallengeSandboxItem item)
    {
        var task = item.Task;
        var json = JsonSerializer.Serialize(task, task.GetType());
        if (JsonSerializer.Deserialize(json, task.GetType()) is not BaseTask copy)
        {
            return;
        }

        SandboxItems.Insert(SandboxItems.IndexOf(item) + 1, new TutorialChallengeSandboxItem(copy));
    }

    /// <summary>
    /// 重命名一个沙盒任务条目。
    /// </summary>
    /// <param name="item">条目。</param>
    public void RenameTask(TutorialChallengeSandboxItem item)
    {
        var dialog = new TextDialogView(
            LocalizationHelper.GetString("TaskQueue.RenameTask"),
            LocalizationHelper.GetString("TaskQueue.RenameTaskPrompt"),
            item.Task.Name) {
            Owner = Application.Current.Windows.OfType<TutorialChallengeLevelView>().FirstOrDefault(),
        };

        if (dialog.ShowDialog() != true || string.IsNullOrWhiteSpace(dialog.InputText))
        {
            return;
        }

        item.Task.Name = dialog.InputText.Trim().Replace("\r", string.Empty).Replace("\n", string.Empty);
        item.RefreshDisplayName();
    }

    /// <summary>
    /// 重置沙盒，等价重新进入关卡：任务、完成态、伪造库存、设置页签全部回到进关初始状态。
    /// </summary>
    public void Reset()
    {
        EnableAdvancedSettings = false;
        IsCompleted = false;
        _fakeInventories.Clear();
        BuildSandbox();
    }

    /// <summary>
    /// 向当前理智作战任务的备选关卡列表末尾添加一个空关卡行。
    /// </summary>
    public void AddStageToPlan()
    {
        if (SelectedFightTask is null)
        {
            return;
        }

        StagePlanRows.Add(new TutorialChallengeStagePlanRow(this, string.Empty));
        SyncStagePlanFromRow();
    }

    /// <summary>
    /// 从备选关卡列表移除一行；最后一行不移除，与真实面板保持一致。
    /// </summary>
    /// <param name="row">行。</param>
    public void RemoveStageFromPlan(TutorialChallengeStagePlanRow row)
    {
        if (StagePlanRows.Count <= 1)
        {
            return;
        }

        StagePlanRows.Remove(row);
        SyncStagePlanFromRow();
    }

    /// <summary>
    /// 鼠标悬浮刷新关卡备选提示。
    /// </summary>
    public void StagePlanTipRefresh() => RefreshStagePlanTip();

    /// <summary>
    /// 向当前库存保持任务导入一份预设的全部计划；静态预设未命中时按进行中活动的 key 导入。
    /// </summary>
    /// <param name="presetKey">预设 key（见 <see cref="DepotMaintainTaskUserControlModel.PresetData"/>）或活动 key。</param>
    public void ImportPreset(string presetKey)
    {
        if (SelectedDepotMaintainTask is not { } task)
        {
            return;
        }

        if (!DepotMaintainTaskUserControlModel.PresetData.TryGetValue(presetKey, out var presets))
        {
            AddActivityPresetPlan(task, presetKey);
            return;
        }

        foreach (var (stage, drops, defaultCount) in presets)
        {
            foreach (var dropId in drops)
            {
                task.PlanList.Add(new DepotMaintainTask.Plan(stage, dropId, defaultCount));
            }
        }

        RefreshPanelState();
        Evaluate();
    }

    /// <summary>
    /// 预设下拉打开时重估活动时间窗口（活动开始/结束不触发关卡数据事件，以打开时刻墙钟重查）。
    /// </summary>
    /// <param name="sender">事件发送者。</param>
    /// <param name="e">可见性变化参数。</param>
    public void PresetDropDownVisibilityChanged(object sender, DependencyPropertyChangedEventArgs e)
        => DepotMaintainTaskUserControlModel.Instance.PresetDropDownVisibilityChanged(sender, e);

    /// <summary>
    /// 清空当前库存保持任务的全部计划。
    /// </summary>
    public void ClearPlans()
    {
        if (SelectedDepotMaintainTask is not { } task)
        {
            return;
        }

        task.PlanList.Clear();
        RefreshPanelState();
        Evaluate();
    }

    /// <summary>
    /// 折叠所有计划行。
    /// </summary>
    public void CollapseAll()
    {
        foreach (var row in PlanRows)
        {
            row.IsExpanded = false;
        }
    }

    /// <summary>
    /// 追加一条空计划。
    /// </summary>
    public void AddPlan()
    {
        if (SelectedDepotMaintainTask is not { } task)
        {
            return;
        }

        task.PlanList.Add(new DepotMaintainTask.Plan());
        RefreshPanelState();
        Evaluate();
    }

    /// <summary>
    /// Gets 当前库存保持任务的计划概览（每行 ｢索引: 关卡 - 材料 xN｣ 拼接），对齐真实面板的 PlanInfo 显示形态。
    /// </summary>
    public string PlanInfo
    {
        get {
            if (SelectedDepotMaintainTask is not { } task || task.PlanList.Count == 0)
            {
                return string.Empty;
            }

            return string.Join("\n", task.PlanList.Select((plan, i) =>
            {
                var stageName = StageListSource.FirstOrDefault(item => item.Value == plan.Stage)?.Display ?? plan.Stage;
                var dropName = ItemListHelper.GetItemName(plan.DropId) ?? LocalizationHelper.GetString("Common.NotSelected");
                return $"{i + 1}: {stageName} - {dropName} {FakeInventoryFor(plan.DropId)}/{plan.DropCount.FormatNumber(false)}";
            }));
        }
    }

    /// <summary>
    /// 移除当前库存保持任务的一行计划。
    /// </summary>
    /// <param name="row">行。</param>
    public void RemovePlan(TutorialChallengePlanRow row)
    {
        // Plan 是 record，按值比较会命中第一条等值计划，用行下标定位
        var index = PlanRows.IndexOf(row);
        if (SelectedDepotMaintainTask is not { } task || index < 0)
        {
            return;
        }

        task.PlanList.RemoveAt(index);
        PlanRows.RemoveAt(index);
        NotifyOfPropertyChange(nameof(PlanInfo));
        Evaluate();
    }

    /// <summary>
    /// 库存保持行内编辑后写回任务实例并触发判定。Plan 为 init-only record，用 with 替换同索引项。
    /// </summary>
    /// <param name="row">行。</param>
    /// <param name="updated">更新后的 Plan。</param>
    internal void ReplacePlan(TutorialChallengePlanRow row, DepotMaintainTask.Plan updated)
    {
        var index = PlanRows.IndexOf(row);
        if (SelectedDepotMaintainTask is not { } task || index < 0)
        {
            return;
        }

        task.PlanList[index] = updated;
        row.Plan = updated;
        row.RefreshTitle();
        NotifyOfPropertyChange(nameof(PlanInfo));
        Evaluate();
    }

    private void AddActivityPresetPlan(DepotMaintainTask task, string activityKey)
    {
        if (!Instances.StageManager.ActivityList.TryGetValue(activityKey, out var activity))
        {
            return;
        }

        foreach (var stage in activity.StageList)
        {
            if (!string.IsNullOrEmpty(stage.Drop) && ItemListHelper.ArkItems.ContainsKey(stage.Drop))
            {
                task.PlanList.Add(new DepotMaintainTask.Plan(stage.Value, stage.Drop, DepotMaintainTaskUserControlModel.ActivityPresetDefaultCount));
            }
        }

        if (task.SkipDuringActivity)
        {
            GrowlHelper.Warning(LocalizationHelper.GetString("TaskQueue.DepotMaintain.PresetActivitySkipConflict"));
        }

        RefreshPanelState();
        Evaluate();
    }

    // 拖拽排序只产生 Move；RefreshPanelState 重建行时的 Reset/Add 不需要写回
    private void OnStagePlanRowsChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action == NotifyCollectionChangedAction.Move)
        {
            SyncStagePlanFromRow();
        }
    }

    private void OnPlanRowsChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.Action != NotifyCollectionChangedAction.Move || SelectedDepotMaintainTask is not { } task)
        {
            return;
        }

        task.PlanList = [.. PlanRows.Select(row => row.Plan)];
        NotifyOfPropertyChange(nameof(PlanInfo));
        Evaluate();
    }

    private void BuildSandbox()
    {
        SelectedItem = null;

        // 逐条 Remove/Add 让 CollectionChanged 处理器统一挂摘条目订阅，避免 Clear 的 Reset 事件丢失 OldItems 造成重复订阅
        for (var i = SandboxItems.Count - 1; i >= 0; i--)
        {
            SandboxItems.RemoveAt(i);
        }

        foreach (var task in _level.SandboxFactory())
        {
            SandboxItems.Add(new TutorialChallengeSandboxItem(task));
        }
    }

    private void OnSandboxItemsChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        if (e.NewItems is { } newItems)
        {
            foreach (TutorialChallengeSandboxItem item in newItems)
            {
                item.Task.PropertyChanged += _taskPropertyChangedHandler;
                item.PropertyChanged += OnSandboxItemPropertyChanged;
            }
        }

        if (e.OldItems is { } oldItems)
        {
            foreach (TutorialChallengeSandboxItem item in oldItems)
            {
                item.Task.PropertyChanged -= _taskPropertyChangedHandler;
                item.PropertyChanged -= OnSandboxItemPropertyChanged;
            }
        }

        NotifyOfPropertyChange(nameof(StartUpTaskCount));
        RefreshMoveFlags();
        Evaluate();
    }

    /// <summary>
    /// Gets 沙盒队列中开始唤醒任务的个数（添加菜单 ｢开始唤醒至多一个｣ 限制的判定源）。
    /// </summary>
    public int StartUpTaskCount => SandboxItems.Count(item => item.Task is StartUpTask);

    private void OnSandboxItemPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        // 齿轮由条目 EnableSetting 双向绑定驱动，其余条目的取消选中由 RadioButton 的 GroupName 完成，这里只同步 SelectedItem
        if (e.PropertyName == nameof(TutorialChallengeSandboxItem.EnableSetting)
            && sender is TutorialChallengeSandboxItem { EnableSetting: true } item)
        {
            _selectedItem = item;
            EnableAdvancedSettings = false;
            NotifySelectedChanged();
            RefreshPanelState();
        }
    }

    private void NotifySelectedChanged()
    {
        NotifyOfPropertyChange(nameof(SelectedItem));
        NotifyOfPropertyChange(nameof(SelectedTask));
        NotifyOfPropertyChange(nameof(SelectedFightTask));
        NotifyOfPropertyChange(nameof(SelectedDepotMaintainTask));
        NotifyOfPropertyChange(nameof(ShowFightPanel));
        NotifyOfPropertyChange(nameof(ShowDepotMaintainPanel));
        NotifyOfPropertyChange(nameof(SandboxUseStoneDisplay));
        SyncDropsItemName();
        NotifySpecifiedDropsStateChanged();
        RefreshInventoryDisplay();
    }

    private void NotifySpecifiedDropsStateChanged()
    {
        NotifyOfPropertyChange(nameof(UseDropQuantityMode));
        NotifyOfPropertyChange(nameof(UseTargetInventoryMode));
        NotifyOfPropertyChange(nameof(SandboxUseOptionalStage));
        NotifyOfPropertyChange(nameof(SandboxEnableTargetDrop));
        NotifyOfPropertyChange(nameof(SandboxIsInventoryTarget));
    }

    private void SyncDropsItemName()
    {
        var dropId = SelectedFightTask?.DropId ?? string.Empty;
        DropsItemName = DropsList.FirstOrDefault(d => d.Value == dropId)?.Display
            ?? ItemListHelper.GetItemName(dropId)
            ?? dropId;
    }

    private void RefreshInventoryDisplay()
    {
        NotifyOfPropertyChange(nameof(CurrentDropsInventoryText));
        NotifyOfPropertyChange(nameof(EffectiveDropsQuantityText));
    }

    /// <summary>
    /// 取指定材料的伪造库存，未生成过则现造并缓存；同一材料在本关卡窗口内固定，重置后重新生成。
    /// </summary>
    /// <param name="dropId">材料 id，空值返回 0。</param>
    /// <returns>该材料的伪造库存数。</returns>
    private int FakeInventoryFor(string? dropId)
    {
        if (string.IsNullOrEmpty(dropId))
        {
            return 0;
        }

        if (!_fakeInventories.TryGetValue(dropId, out var count))
        {
            count = _random.Next(0, 30);
            _fakeInventories[dropId] = count;
        }

        return count;
    }

    private void RefreshMoveFlags()
    {
        // 右键菜单上移/下移可用性与真实队列一致：首项不可上移，末项不可下移
        for (var i = 0; i < SandboxItems.Count; i++)
        {
            SandboxItems[i].CanMoveUp = i > 0;
            SandboxItems[i].CanMoveDown = i < SandboxItems.Count - 1;
        }
    }

    private void RefreshPanelState()
    {
        StagePlanRows.Clear();
        if (SelectedFightTask is { } fight)
        {
            foreach (var stage in fight.StagePlan)
            {
                StagePlanRows.Add(new TutorialChallengeStagePlanRow(this, stage));
            }

            RefreshWeeklySchedule();
        }

        PlanRows.Clear();
        if (SelectedDepotMaintainTask is { } depotTask)
        {
            foreach (var plan in depotTask.PlanList)
            {
                PlanRows.Add(new TutorialChallengePlanRow(this, plan));
            }
        }

        NotifyOfPropertyChange(nameof(PlanInfo));
    }

    /// <summary>
    /// 备选关卡行变更后写回任务实例并重算判定；行内直接改选关卡也走这里。
    /// </summary>
    internal void SyncStagePlanFromRow()
    {
        if (SelectedFightTask is not { } fight)
        {
            return;
        }

        var newPlan = StagePlanRows.Select(row => row.Stage).ToList();
        if (!fight.StagePlan.SequenceEqual(newPlan))
        {
            fight.StagePlan = newPlan;
        }
        else
        {
            Evaluate();
        }
    }

    /// <summary>
    /// 把当前选中理智作战任务的周计划回填到展示行；行内勾选变更经 <see cref="OnWeeklyScheduleItemPropertyChanged"/> 写回。
    /// </summary>
    private void RefreshWeeklySchedule()
    {
        if (SelectedFightTask is not { } fight)
        {
            return;
        }

        foreach (var item in WeeklyScheduleSource)
        {
            item.Value = !fight.WeeklySchedule.TryGetValue(item.DayOfWeek, out var value) || value;
        }
    }

    /// <summary>
    /// 展示行勾选变更后写回当前选中理智作战任务并重算判定。
    /// </summary>
    /// <param name="sender">变更的周计划行。</param>
    /// <param name="e">属性变更参数。</param>
    private void OnWeeklyScheduleItemPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName != nameof(WeeklyScheduleItem.Value)
            || sender is not WeeklyScheduleItem item
            || SelectedFightTask is not { } fight)
        {
            return;
        }

        fight.WeeklySchedule[item.DayOfWeek] = item.Value;
        Evaluate();
    }

    private void RefreshStagePlanTip()
    {
        var fight = SelectedFightTask;
        if (fight is null)
        {
            StagePlanTip = string.Empty;
            return;
        }

        var stage = GetEffectiveStage(fight) ?? "--";
        StagePlanTip = LocalizationHelper.GetStringFormat("TaskQueue.Fight.StagePlan.Tip", stage);
    }

    private static string? GetEffectiveStage(FightTask fight)
    {
        var day = Instances.TaskQueueViewModel.CurDayOfWeek;
        foreach (var code in fight.StagePlan)
        {
            if (string.IsNullOrEmpty(code))
            {
                continue;
            }

            var info = Instances.StageManager.GetStageInfo(code);
            if (info.IsStageOpen(day))
            {
                return info.Display;
            }
        }

        return null;
    }

    private void Evaluate()
    {
        if (IsCompleted)
        {
            return;
        }

        // 只计入真实队列会执行的任务，IsEnable 为 null 时是否执行跟随 ｢反转主任务右键单击效果｣ 设置
        var tasks = SandboxItems.Select(item => item.Task).Where(TaskQueueViewModel.IsTaskEnable).ToList();
        if (!_level.GoalPredicate(tasks))
        {
            return;
        }

        Complete(tasks);
    }

    private void Complete(IReadOnlyList<BaseTask> tasks)
    {
        IsCompleted = true;

        if (_level.SmartPredicate is { } smart && !smart(tasks))
        {
            var msg = LocalizationHelper.GetString("TutorialChallenge.SmartSolutionHint");
            GrowlHelper.Show(InfoType.Info, msg, () => Growl.Info(msg, "TutorialChallenge"));
        }

        TutorialChallengeProgress.MarkCleared(_level.Id);
    }

    /// <summary>
    /// 构建关卡候选列表，映射逻辑同真实面板的 <c>RefreshStageList</c>。
    /// </summary>
    private void BuildStageListSource()
    {
        var day = Instances.TaskQueueViewModel.CurDayOfWeek;
        StageListSource.Clear();
        foreach (var stage in Instances.StageManager.GetStageList())
        {
            StageListSource.Add(new FightTaskUserControlModel.StageSourceItem() {
                Display = stage.Display,
                Value = stage.Value,
                IsVisible = true,
                IsOpen = stage.IsStageOpen(day),
            });
        }
    }

    private void OnStageDataChanged(object? sender, EventArgs e)
        => _ = Execute.OnUIThreadAsync(BuildStageListSource);

    /// <summary>
    /// 窗口关闭或切换到其他关卡时退订外部事件，否则静态的语言切换事件会一直持有本实例。
    /// </summary>
    public void Detach()
    {
        LocalizationHelper.LanguageChanged -= OnLanguageChanged;
        Instances.StageManager.StageDataChanged -= OnStageDataChanged;
        foreach (var item in SandboxItems)
        {
            item.Task.PropertyChanged -= _taskPropertyChangedHandler;
            item.PropertyChanged -= OnSandboxItemPropertyChanged;
        }

        foreach (var item in WeeklyScheduleSource)
        {
            item.PropertyChanged -= OnWeeklyScheduleItemPropertyChanged;
        }
    }

    private void OnLanguageChanged()
    {
        NotifyOfPropertyChange(nameof(Title));
        NotifyOfPropertyChange(nameof(Goal));
        NotifyOfPropertyChange(nameof(WindowTitle));
        RefreshStagePlanTip();

        // DropsList 由真实面板原地刷新，选中项不变，只需按新显示名回填输入框
        SyncDropsItemName();

        foreach (var stage in StageListSource)
        {
            stage.Display = Instances.StageManager.GetStageInfo(stage.Value).Display;
        }

        foreach (var item in WeeklyScheduleSource)
        {
            item.RefreshLocalization();
        }

        foreach (var item in SandboxItems)
        {
            item.RefreshDisplayName();
        }

        foreach (var row in PlanRows)
        {
            row.RefreshTitle();
        }

        NotifyOfPropertyChange(nameof(PlanInfo));
    }
}

#pragma warning disable SA1402 // 关卡沙盒 UI 态小类集中放本文件
/// <summary>
/// 沙盒任务条目。仅承载选中态等 UI 状态，配置字段直接读写所包装的任务实例，形状对齐真实队列的条目视图模型。
/// </summary>
public class TutorialChallengeSandboxItem(BaseTask task) : PropertyChangedBase
{
    private bool _enableSetting;
    private bool _canMoveUp;
    private bool _canMoveDown;

    /// <summary>
    /// Gets 包装的沙盒任务实例。
    /// </summary>
    public BaseTask Task => task;

    /// <summary>
    /// Gets 条目显示名，自定义名优先，未命名时回退任务类型本地化名。
    /// </summary>
    public string DisplayName => Task.NameOrTaskType;

    /// <summary>
    /// Gets 或设置 齿轮选中态（左键打开任务设置）。
    /// </summary>
    public bool EnableSetting
    {
        get => _enableSetting;
        set => SetAndNotify(ref _enableSetting, value);
    }

    /// <summary>
    /// Gets 或设置 a value indicating whether 可上移。
    /// </summary>
    public bool CanMoveUp
    {
        get => _canMoveUp;
        set => SetAndNotify(ref _canMoveUp, value);
    }

    /// <summary>
    /// Gets 或设置 a value indicating whether 可下移。
    /// </summary>
    public bool CanMoveDown
    {
        get => _canMoveDown;
        set => SetAndNotify(ref _canMoveDown, value);
    }

    /// <summary>
    /// Gets a value indicating whether 可复制。开始唤醒至多一个，不可复制。
    /// </summary>
    public bool CanCopy => Task is not StartUpTask;

    /// <summary>
    /// 任务名变更后刷新条目显示名（显示名由任务实例字段计算，无独立变更通知）。
    /// </summary>
    public void RefreshDisplayName() => NotifyOfPropertyChange(nameof(DisplayName));
}

/// <summary>
/// 备选关卡列表行。行内 ComboBox 直接改选关卡并写回任务实例，排序走拖拽。
/// </summary>
public class TutorialChallengeStagePlanRow(TutorialChallengeLevelViewModel owner, string stage) : PropertyChangedBase
{
    private string _stage = stage;

    /// <summary>
    /// Gets 或设置 关卡 code，变更即写回所属理智作战任务。
    /// </summary>
    public string Stage
    {
        get => _stage;
        set {
            if (SetAndNotify(ref _stage, value))
            {
                IsOpen = Instances.TaskQueueViewModel.IsStageOpen(value);
                owner.SyncStagePlanFromRow();
            }
        }
    }

    /// <summary>
    /// Gets 或设置 a value indicating whether 该关卡当日开放（ComboBox 与手写框半透明提示用）。
    /// </summary>
    public bool IsOpen { get; set => SetAndNotify(ref field, value); } = Instances.TaskQueueViewModel.IsStageOpen(stage);
}

/// <summary>
/// 库存保持计划行。Plan 为 init-only record，行内编辑经 owner.ReplacePlan 用 with 替换同索引项写回任务实例。
/// </summary>
public class TutorialChallengePlanRow(TutorialChallengeLevelViewModel owner, DepotMaintainTask.Plan plan) : PropertyChangedBase
{
    /// <summary>
    /// Gets or sets 包装的库存保持计划记录。替换后由 <see cref="RefreshTitle"/> 补齐依赖属性通知，本属性本身不带通知。
    /// </summary>
    public DepotMaintainTask.Plan Plan { get; set; } = plan;

    /// <summary>
    /// Gets 或设置 a value indicating whether 行展开编辑态。
    /// </summary>
    public bool IsExpanded
    {
        get;
        set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets 行标题（关卡 + 材料名），形状对齐真实计划项的 Title。
    /// </summary>
    public string Title => $"{Plan.Stage} - {DropName} x{Plan.DropCount}";

    /// <summary>
    /// Gets 或设置 关卡 code。
    /// </summary>
    public string Stage
    {
        get => Plan.Stage;
        set => owner.ReplacePlan(this, Plan with { Stage = value });
    }

    /// <summary>
    /// Gets 或设置 掉落物 id。
    /// </summary>
    public string DropId
    {
        get => Plan.DropId;
        set => owner.ReplacePlan(this, Plan with { DropId = value });
    }

    /// <summary>
    /// Gets or sets 掉落物显示名（可编辑 ComboBox 的 Text 绑定；set 时按 Display 反查回 DropId）。
    /// </summary>
    public string DropName
    {
        get => ItemListHelper.GetItemName(Plan.DropId) ?? Plan.DropId;
        set {
            var match = owner.DropsList.FirstOrDefault(d => string.Equals(d.Display, value, StringComparison.Ordinal));
            if (match is not null)
            {
                DropId = match.Value ?? string.Empty;
            }
        }
    }

    /// <summary>
    /// 材料下拉关闭时按输入文本反查材料，查不到则回退 ｢不选择｣。
    /// </summary>
    public void DropsListDropDownClosed()
    {
        if (owner.DropsList.FirstOrDefault(d => string.Equals(d.Display, DropName, StringComparison.Ordinal)) is { } item)
        {
            DropId = item.Value ?? string.Empty;
        }
        else
        {
            DropId = string.Empty;
            NotifyOfPropertyChange(nameof(DropName));
        }
    }

    /// <summary>
    /// Gets 或设置 目标数量。
    /// </summary>
    public int DropCount
    {
        get => Plan.DropCount;
        set => owner.ReplacePlan(this, Plan with { DropCount = value });
    }

    /// <summary>
    /// Gets 或设置 本计划是否使用药剂。
    /// </summary>
    public bool UseMedicine
    {
        get => Plan.UseMedicine;
        set => owner.ReplacePlan(this, Plan with { UseMedicine = value });
    }

    /// <summary>
    /// Gets 或设置 本计划用药剂数。
    /// </summary>
    public int MedicineCount
    {
        get => Plan.MedicineCount;
        set => owner.ReplacePlan(this, Plan with { MedicineCount = value });
    }

    /// <summary>
    /// Gets 或设置 本计划是否使用源石。
    /// </summary>
    public bool UseStone
    {
        get => Plan.UseStone;
        set => owner.ReplacePlan(this, Plan with { UseStone = value });
    }

    /// <summary>
    /// Gets 或设置 本计划用源石数。
    /// </summary>
    public int StoneCount
    {
        get => Plan.StoneCount;
        set => owner.ReplacePlan(this, Plan with { StoneCount = value });
    }

    /// <summary>
    /// Plan 记录被替换后刷新依赖显示。
    /// </summary>
    public void RefreshTitle()
    {
        NotifyOfPropertyChange(nameof(Stage));
        NotifyOfPropertyChange(nameof(DropId));
        NotifyOfPropertyChange(nameof(DropName));
        NotifyOfPropertyChange(nameof(DropCount));
        NotifyOfPropertyChange(nameof(Title));
    }
}
#pragma warning restore SA1402
