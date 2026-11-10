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
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Threading.Tasks;
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
using MaaWpfGui.Main;
using MaaWpfGui.Models;
using MaaWpfGui.Models.TutorialChallenge;
using MaaWpfGui.Services;
using MaaWpfGui.Styles.Properties;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using MaaWpfGui.ViewModels.UserControl.Toolbox;
using MaaWpfGui.Views.Dialogs;
using Serilog;
using Stylet;
using Timer = System.Timers.Timer;

namespace MaaWpfGui.ViewModels.Dialogs;

/// <summary>
/// 教程关卡操作场的视图模型。持有纯内存沙盒任务队列（不进入真实任务队列、不持久化），
/// 沙盒任何变更即时重算终态判定，达成即记录通关并发放牛牛币。
/// 条目与面板的形状对齐真实任务队列（TaskItemViewModel / FightTaskUserControlModel），仅数据源隔离。
/// </summary>
public class TutorialChallengeLevelViewModel : PropertyChangedBase
{
    private static readonly ILogger _logger = Log.ForContext<TutorialChallengeLevelViewModel>();

    private readonly TutorialChallengeLevel _level;
    private readonly PropertyChangedEventHandler _taskPropertyChangedHandler;
    private readonly Random _random = new();
    private readonly Dictionary<string, int> _fakeInventories = [];
    private bool _isCompleted;
    private TutorialChallengeSandboxItem? _selectedItem;
    private string _stagePlanTip = string.Empty;
    private string _dropsItemName = string.Empty;

    // 设置搜索关卡的干扰项常量：40 个分区、每分区 5 项、每分区恰 1 个目标项
    private const int SettingsSearchSectionCount = 40;
    private const int SettingsSearchItemsPerSection = 5;
    private const string SettingsSearchTargetMarker = "maa";

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
                case nameof(RecruitTask.PreserveTagEnabled):
                    NotifyOfPropertyChange(nameof(SandboxPreserveTagEnabled));
                    break;
                case nameof(RecruitTask.PreserveTagList):
                    NotifyOfPropertyChange(nameof(AutoRecruitPreserveTagList));
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
        IssueReportUserControlModel.Instance.PropertyChanged += OnIssueReportPropertyChanged;
        SandboxSchedule = new TutorialChallengeSandboxSchedule(Evaluate, _level.SandboxTimerSlots);
        BuildSettingSections();

        if (IsDragDropUpdateLevel && Bootstrapper.IsUserAdministrator())
        {
            // 管理员权限下 OLE 拖拽本身无法进入（Windows 限制低权限源向高权限进程投递），进关卡即提示本关无法以拖拽完成
            ShowTutorialGrowl(InfoType.Warning, LocalizationHelper.GetString("TutorialChallenge.Level.DragDropUpdate.AdminBlocked"));
        }

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
    /// Gets 选中的公招沙盒任务，非公招时为 null。
    /// </summary>
    public RecruitTask? SelectedRecruitTask => SelectedTask as RecruitTask;

    /// <summary>
    /// Gets 或设置 当前选中的页签下标（0=任务队列页，1=设置页）。
    /// </summary>
    public int SelectedTab
    {
        get;
        set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets 窗口级定时设置沙盒状态（形状对齐真实设置页的定时执行区，数据仅存于本关卡窗口生命周期内）。
    /// </summary>
    public TutorialChallengeSandboxSchedule SandboxSchedule { get; }

    /// <summary>
    /// Gets 沙盒设置页分区导航列表，形状对齐真实设置页的 Settings 导航项。
    /// </summary>
    public ObservableCollection<TutorialChallengeSettingSection> SandboxSettingSections { get; } = [];

    /// <summary>
    /// Gets 或设置 设置页搜索框文本。分区过滤归口 <see cref="SearchHighlightBehavior"/>（可视树命中写回
    /// <see cref="TutorialChallengeSettingSection.IsVisibleInSearch"/>，对齐真实设置页），本属性变化只联动高级分区可见性。
    /// </summary>
    public string SettingsSearchText
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                ApplySettingsSearch();
            }
        }
    } = string.Empty;

    /// <summary>
    /// Gets 或设置 a value indicating whether 沙盒设置页展开高级分区（对齐真实设置页的 ｢显示高级设置｣ 开关）。
    /// </summary>
    public bool ShowAdvancedSettings
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                UpdateAdvancedSettingsVisibility();
            }
        }
    }

    /// <summary>
    /// Gets 高级分区分隔条在内容 Grid 中的行号，即普通分区总数（普通分区恒占前段行，分隔条插在两类分区边界，对齐真实设置页）。
    /// </summary>
    public int AdvancedSettingsDividerRow => SandboxSettingSections.Count(section => !section.IsAdvanced);

    /// <summary>
    /// Gets a value indicating whether 存在因搜索或高级开关而当前可见的高级分区（控制 ｢高级设置｣ 分隔线显隐）。
    /// </summary>
    public bool HasVisibleAdvancedSettings => SandboxSettingSections.Any(section => section.IsAdvanced && section.IsVisible);

    /// <summary>
    /// 统一回写各分区的高级可见态：搜索框非空时高级分区强制展开（对齐真实设置页
    /// <c>showAdvanced = ShowAdvancedSettings || !string.IsNullOrEmpty(SearchText)</c>，搜索命中的高级项不得被折叠藏住），
    /// 不改动用户的显示偏好本身。
    /// </summary>
    private void UpdateAdvancedSettingsVisibility()
    {
        var showAdvanced = ShowAdvancedSettings || !string.IsNullOrEmpty(SettingsSearchText);
        foreach (var section in SandboxSettingSections)
        {
            section.AreAdvancedSettingsVisible = showAdvanced;
        }
    }

    /// <summary>
    /// 切换高级分区显隐，对齐真实设置页底部按钮行为。
    /// </summary>
    public void ToggleAdvancedSettings() => ShowAdvancedSettings = !ShowAdvancedSettings;

    #region 设置分区导航与滚动视图联动（对齐真实 SettingsViewModel 精简版，无拖拽排序分支）

    private enum NotifyType
    {
        None,
        SelectedIndex,
        ScrollOffset,
    }

    private NotifyType _notifySource = NotifyType.None;

    private Timer? _resetNotifyTimer;

    private void ResetNotifySource()
    {
        if (_resetNotifyTimer != null)
        {
            _resetNotifyTimer.Stop();
            _resetNotifyTimer.Close();
        }

        _resetNotifyTimer = new Timer(20);
        _resetNotifyTimer.Elapsed += (_, _) => {
            _notifySource = NotifyType.None;
        };
        _resetNotifyTimer.AutoReset = false;
        _resetNotifyTimer.Enabled = true;
        _resetNotifyTimer.Start();
    }

    /// <summary>
    /// Gets or sets the height of scroll viewport.
    /// </summary>
    public double ScrollViewportHeight { get; set; }

    /// <summary>
    /// Gets or sets the extent height of scroll.
    /// </summary>
    public double ScrollExtentHeight { get; set; }

    private List<double> _dividerVerticalOffsetList = [];

    /// <summary>
    /// Gets or sets the list of divider vertical offset.
    /// </summary>
    public List<double> DividerVerticalOffsetList
    {
        get => _dividerVerticalOffsetList;
        set {
            if (_dividerVerticalOffsetList == value)
            {
                return;
            }

            SetAndNotify(ref _dividerVerticalOffsetList, value);
        }
    }

    private int _selectedIndex = -1;

    private TutorialChallengeSettingSection? _selectedSetting;

    /// <summary>
    /// Gets or sets the index selected.
    /// </summary>
    public int SelectedIndex
    {
        get => _selectedIndex;
        set {
            if (_selectedIndex == value)
            {
                return;
            }

            if (value == -1)
            {
                _selectedSetting = null;
                SetAndNotify(ref _selectedIndex, value);
                return;
            }

            if (value < 0 || value >= SandboxSettingSections.Count || !SandboxSettingSections[value].IsVisible
                || value >= DividerVerticalOffsetList.Count || double.IsNaN(DividerVerticalOffsetList[value]))
            {
                return;
            }

            switch (_notifySource)
            {
                case NotifyType.None:
                    _notifySource = NotifyType.SelectedIndex;
                    SetAndNotify(ref _selectedIndex, value);

                    ScrollAnimationTarget = DividerVerticalOffsetList[value];

                    ResetNotifySource();
                    break;

                case NotifyType.ScrollOffset:
                    SetAndNotify(ref _selectedIndex, value);
                    break;

                case NotifyType.SelectedIndex:
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }

            if (_selectedIndex == value)
            {
                _selectedSetting = SandboxSettingSections[value];
            }
        }
    }

    private double _scrollOffset;

    /// <summary>
    /// Gets or sets the scroll offset.
    /// </summary>
    public double ScrollOffset
    {
        get => _scrollOffset;
        set {
            // 平滑滚动动画落地的回写（ScrollViewerBinding 在动画结束后把目标值路由回绑定源）：
            // 与动画目标一致说明本次滚动源于导航定位，同步值即可，不反向重算导航高亮——
            // 目标偏移被 ScrollViewer 钳制时（分节下方内容不足一屏，实际停不到目标），
            // 回写会命中下方“滚到底选最后一项”的分支，高亮跳离所点击的分节
            if (Math.Abs(value - ScrollAnimationTarget) < 1)
            {
                SetAndNotify(ref _scrollOffset, value);
                return;
            }

            switch (_notifySource)
            {
                case NotifyType.None:
                    _notifySource = NotifyType.ScrollOffset;
                    SetAndNotify(ref _scrollOffset, value);

                    // 设置 ListBox SelectedIndex 为当前 ScrollOffset 索引
                    var visibleDividers = DividerVerticalOffsetList
                        .Select((offset, index) => (offset, index))
                        .Where(item => !double.IsNaN(item.offset) && item.index < SandboxSettingSections.Count && SandboxSettingSections[item.index].IsVisible)
                        .ToArray();
                    if (visibleDividers.Length > 0)
                    {
                        // 滚动条滚动到底部，返回最后一个 Divider 索引
                        if (value + ScrollViewportHeight >= ScrollExtentHeight)
                        {
                            SelectedIndex = visibleDividers[^1].index;
                            ResetNotifySource();
                            break;
                        }

                        // 根据出当前 ScrollOffset 选出最后一个在可视范围的 Divider 索引
                        var index = visibleDividers.LastOrDefault(item => value >= item.offset, visibleDividers[0]).index;
                        SelectedIndex = index;
                    }

                    ResetNotifySource();
                    break;

                case NotifyType.SelectedIndex:
                    SetAndNotify(ref _scrollOffset, value);
                    break;

                case NotifyType.ScrollOffset:
                    break;

                default:
                    throw new ArgumentOutOfRangeException();
            }
        }
    }

    private double _scrollAnimationTarget = double.NaN;

    /// <summary>
    /// Gets or sets the target offset of the smooth scroll animation,
    /// which is performed by <see cref="Styles.Properties.ScrollViewerBinding"/> on the UI side.
    /// </summary>
    public double ScrollAnimationTarget
    {
        get => _scrollAnimationTarget;
        set => SetAndNotify(ref _scrollAnimationTarget, value);
    }

    /// <summary>
    /// 分区可见性或数量变化后请求 View 重测各分区 header 的纵向偏移（<see cref="Styles.Properties.ScrollViewerBinding.RefreshDividerOffsets"/>）。
    /// </summary>
    public event EventHandler? RefreshDividerOffsetsRequested;

    private bool _isSettingsLayoutRefreshPending;

    /// <summary>
    /// 供 View 在分区容器重建完成后告知布局已变，走与可见性变化相同的刷新纠偏流程
    /// （首次生成容器时左导航尚无选中项，需要以此时已就绪的偏移列表把选中纠偏到首个可见分区）。
    /// </summary>
    public void NotifySettingSectionsLayoutChanged() => OnSettingItemValueChanged();

    /// <summary>
    /// 分区布局待刷新入口：可见性/数量变化后延迟到 Loaded 优先级执行，先请 View 重测偏移，
    /// 再按可见性纠偏选中项并在选中分区 offset 位移超阈值时重发滚动目标。对齐真实
    /// <c>SettingsViewModel.OnSettingItemValueChanged</c> 去掉拖拽排序（重建选中/布局后恢复）分支的精简版。
    /// </summary>
    private void OnSettingItemValueChanged()
    {
        if (_isSettingsLayoutRefreshPending)
        {
            return;
        }

        var oldSelectedOffset = SelectedIndex >= 0 && SelectedIndex < DividerVerticalOffsetList.Count
            ? DividerVerticalOffsetList[SelectedIndex]
            : double.NaN;

        _isSettingsLayoutRefreshPending = true;
        Application.Current.Dispatcher.InvokeAsync(() => {
            _isSettingsLayoutRefreshPending = false;
            RefreshDividerOffsetsRequested?.Invoke(this, EventArgs.Empty);

            var isCurrentSelectionInvalid = SelectedIndex < 0 || SelectedIndex >= SandboxSettingSections.Count || !SandboxSettingSections[SelectedIndex].IsVisible;

            // 折叠高级分区时把选中回退到最后一个可见普通分区，并借 ScrollOffset 通知窗口吸收本次索引改写，
            // 避免把「收起高级」误判成用户点选导航而向不可见分区发起滚动
            var isCollapsingAdvancedSelection = isCurrentSelectionInvalid
                && _selectedSetting is { IsAdvanced: true }
                && string.IsNullOrEmpty(SettingsSearchText);
            if (isCurrentSelectionInvalid)
            {
                var target = isCollapsingAdvancedSelection
                    ? SandboxSettingSections.LastOrDefault(item => !item.IsAdvanced && item.IsVisible)
                    : SandboxSettingSections.FirstOrDefault(item => item.IsVisible);
                _notifySource = isCollapsingAdvancedSelection ? NotifyType.ScrollOffset : NotifyType.None;
                SelectedIndex = target == null ? -1 : SandboxSettingSections.IndexOf(target);
                if (isCollapsingAdvancedSelection)
                {
                    _notifySource = NotifyType.None;
                }
            }

            if (SelectedIndex >= 0 && SelectedIndex < DividerVerticalOffsetList.Count
                && !double.IsNaN(DividerVerticalOffsetList[SelectedIndex]))
            {
                var targetOffset = DividerVerticalOffsetList[SelectedIndex];
                var offsetShifted = !double.IsNaN(oldSelectedOffset) && Math.Abs(targetOffset - oldSelectedOffset) >= 0.5;
                if ((isCurrentSelectionInvalid || offsetShifted) && !isCollapsingAdvancedSelection)
                {
                    if (Math.Abs(ScrollAnimationTarget - targetOffset) < 0.5)
                    {
                        ScrollAnimationTarget = double.NaN;
                    }

                    ScrollAnimationTarget = targetOffset;
                }
            }
        }, System.Windows.Threading.DispatcherPriority.Loaded);
    }

    /// <summary>
    /// 分区项属性变化订阅：IsVisible 变化驱动分隔条显隐与联动偏移重测。
    /// </summary>
    private void OnSettingSectionPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(TutorialChallengeSettingSection.IsVisible))
        {
            NotifyOfPropertyChange(nameof(HasVisibleAdvancedSettings));
            OnSettingItemValueChanged();
        }
    }

    /// <summary>
    /// 重建分区顺序与行号：普通分区在前、高级分区在后（与左导航 CollectionViewSource 分组及真实设置页同序），
    /// 普通分区按序占行 0..N-1，高级分区行号再加 1 跳过分隔条所在行。行号不随可见性变化。
    /// </summary>
    private void AssignSectionGridRows()
    {
        // OrderBy 为稳定排序；集合顺序须与显示顺序一致，否则左侧 SelectedIndex 无法直接索引本集合
        var ordered = SandboxSettingSections.OrderBy(section => section.IsAdvanced).ToList();
        for (var i = 0; i < ordered.Count; i++)
        {
            var currentIndex = SandboxSettingSections.IndexOf(ordered[i]);
            if (currentIndex != i)
            {
                SandboxSettingSections.Move(currentIndex, i);
            }
        }

        var normalCount = SandboxSettingSections.Count(section => !section.IsAdvanced);
        var normalIndex = 0;
        var advancedIndex = 0;
        foreach (var section in SandboxSettingSections)
        {
            section.GridRow = !section.IsAdvanced ? normalIndex++ : normalCount + 1 + advancedIndex++;
        }

        NotifyOfPropertyChange(nameof(AdvancedSettingsDividerRow));
    }

    #endregion 设置分区导航与滚动视图联动

    /// <summary>
    /// Gets 公招词条候选列表，共用真实面板的实例（全量词条，Display 为显示名、Value 为词条 key）。
    /// </summary>
    public List<CombinedData> AutoRecruitSkipTagShowList => RecruitTaskUserControlModel.AutoRecruitSkipTagShowList;

    /// <summary>
    /// Gets 或设置 选中公招任务的保留词条选中项（CheckComboBox 的 SelectedItems 绑定，形状对齐真实面板的 AutoRecruitPreserveTagList）。
    /// </summary>
    public object[] AutoRecruitPreserveTagList
    {
        get {
            var value = SelectedRecruitTask?.PreserveTagList ?? [];
            return value.Select(tag => AutoRecruitSkipTagShowList.FirstOrDefault(i => i.Value == tag))
                .Where(v => v is not null)
                .Cast<CombinedData>()
                .ToArray();
        }

        set {
            if (SelectedRecruitTask is not { } recruit)
            {
                return;
            }

            var config = RecruitTagHelper.NormalizeTagList(value.Cast<CombinedData>().Select(item => item.Value));
            if (!recruit.PreserveTagList.SequenceEqual(config, StringComparer.Ordinal))
            {
                recruit.PreserveTagList = config;
            }

            Evaluate();
        }
    }

    /// <summary>
    /// Gets 或设置 选中公招任务是否启用保留指定词条。
    /// </summary>
    public bool SandboxPreserveTagEnabled
    {
        get => SelectedRecruitTask?.PreserveTagEnabled ?? false;
        set {
            if (SelectedRecruitTask is { } recruit)
            {
                recruit.PreserveTagEnabled = value;
            }
        }
    }

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
    /// Gets 本关开放的沙盒场景列表。
    /// </summary>
    public IReadOnlyList<TutorialChallengePanelKind> PanelKinds => _level.PanelKinds;

    /// <summary>
    /// Gets a value indicating whether 显示理智作战沙盒面板。
    /// </summary>
    public bool ShowFightPanel => _level.HasPanel(TutorialChallengePanelKind.Fight) && SelectedFightTask != null;

    /// <summary>
    /// Gets a value indicating whether 显示库存保持沙盒面板。
    /// </summary>
    public bool ShowDepotMaintainPanel => _level.HasPanel(TutorialChallengePanelKind.DepotMaintain) && SelectedDepotMaintainTask != null;

    /// <summary>
    /// Gets a value indicating whether 显示公招沙盒面板。
    /// </summary>
    public bool ShowRecruitPanel => _level.HasPanel(TutorialChallengePanelKind.Recruit) && SelectedRecruitTask != null;

    /// <summary>
    /// Gets a value indicating whether 任务队列页是否显示：本关声明了任一真任务面板（None/Fight/DepotMaintain/Recruit）时显示。
    /// `Schedule` 已由 settingSectionKinds 接管定时分区，不算任务面板。
    /// </summary>
    public bool ShowTaskQueuePanel => PanelKinds.Any(kind => kind is TutorialChallengePanelKind.None
        or TutorialChallengePanelKind.Fight
        or TutorialChallengePanelKind.DepotMaintain
        or TutorialChallengePanelKind.Recruit);

    /// <summary>
    /// Gets a value indicating whether 本关开放设置页场景。
    /// </summary>
    public bool ShowSettingsPanel => _level.HasPanel(TutorialChallengePanelKind.Settings);

    /// <summary>
    /// Gets a value indicating whether 本关为 ｢设置搜索｣ 关卡（决定沙盒设置页是否投放干扰分区与目标 CheckBox）。
    /// </summary>
    public bool IsSettingsSearchLevel => _level.Id == "SettingsSearch";

    /// <summary>
    /// Gets a value indicating whether 全部含 ｢maa｣ 字样的目标干扰项均已勾选（SettingsSearch 关卡的判定源）。
    /// </summary>
    public bool SandboxDecoyTargetsAllChecked => SandboxSettingSections
        .Where(section => section.Kind == TutorialChallengeSettingSectionKind.Decoy)
        .SelectMany(section => section.Items)
        .Where(item => item.IsTarget)
        .All(item => item.IsChecked);

    /// <summary>
    /// Gets a value indicating whether 本关开放拖拽投放区场景。
    /// </summary>
    public bool ShowDropZonePanel => _level.HasPanel(TutorialChallengePanelKind.DropZone);

    /// <summary>
    /// Gets a value indicating whether 本关为 ｢拖拽更新｣ 关卡（决定本关是否启用拖入更新包识别与管理员权限拦截提示）。
    /// </summary>
    public bool IsDragDropUpdateLevel => _level.Id == "DragDropUpdate";

    /// <summary>
    /// Gets a value indicating whether 已识别出完整更新包。
    /// </summary>
    public bool FullPackageIdentified
    {
        get;
        private set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(DragDropAllPackagesIdentified));
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 已识别出 OTA 增量包。
    /// </summary>
    public bool OtaPackageIdentified
    {
        get;
        private set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(DragDropAllPackagesIdentified));
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 已识别出资源更新包。
    /// </summary>
    public bool ResourcePackageIdentified
    {
        get;
        private set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(DragDropAllPackagesIdentified));
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 完整包与资源包均已识别（DragDropUpdate 关卡的判定源）。
    /// OTA 增量包为可选额外识别，不计入通关判定。
    /// </summary>
    public bool DragDropAllPackagesIdentified =>
        FullPackageIdentified && ResourcePackageIdentified;

    /// <summary>
    /// 投放区拖入事件：仅接受 zip 文件，设置复制拖拽效果。对齐真实 <c>RootViewModel.ManualPackagePreviewDragOver</c>。
    /// </summary>
    /// <param name="sender">事件源。</param>
    /// <param name="e">拖拽参数。</param>
    public void SandboxPackagePreviewDragOver(object sender, DragEventArgs e)
    {
        if (!TryGetDroppedZipFile(e, out _))
        {
            return;
        }

        e.Effects = DragDropEffects.Copy;
        e.Handled = true;
    }

    /// <summary>
    /// 投放区拖放事件：取首个 zip 文件按文件名 / zip 内容识别类别（教学仿真不校验版本方向），
    /// 识别成功置对应标志并弹窗口内 Growl。不真实执行注册 / 解压 / 导入。
    /// </summary>
    /// <param name="sender">事件源。</param>
    /// <param name="e">拖拽参数。</param>
    public void SandboxPackageDrop(object sender, DragEventArgs e)
    {
        if (!TryGetDroppedZipFile(e, out string packagePath))
        {
            return;
        }

        _logger.Information("Tutorial sandbox dropped zip file: {PackagePath}", packagePath);
        e.Handled = true;
        _ = SandboxPackageDropFile(packagePath);
    }

    /// <summary>
    /// 对已落在本机路径的更新包按文件名与 zip 内容识别类别并置对应标志。<see cref="SandboxPackageDrop"/> 抽出文件路径后调用，
    /// 亦供调试 / 自动化验证直接喂路径复现投放。完整包 / OTA 包要求文件名中的版本号等于当前 CoreVersion 且架构匹配，
    /// 资源包要求 zip 内资源版本时间戳等于当前本地资源版本，不匹配时提示版本不匹配或无法识别。
    /// </summary>
    /// <param name="packagePath">zip 包本地路径。</param>
    /// <returns>识别任务。</returns>
    public async Task SandboxPackageDropFile(string packagePath)
    {
        string fileName = Path.GetFileName(packagePath);
        string currentVersion = VersionUpdateSettingsUserControlModel.CoreVersion;
        string normalizedArch = RuntimeInformation.OSArchitecture.ToString().StartsWith("arm", StringComparison.OrdinalIgnoreCase)
            ? "arm64" : "x64";

        if (PendingUpdateApplier.MatchFullPackageFileName(fileName) is { } full)
        {
            if (full.Version.Equals(currentVersion, StringComparison.OrdinalIgnoreCase)
                && full.Arch.Equals(normalizedArch, StringComparison.OrdinalIgnoreCase))
            {
                MarkIdentified(FullPackageIdentified, () => FullPackageIdentified = true, "TutorialChallenge.Level.DragDropUpdate.Identified.FullPackage");
            }
            else
            {
                ShowTutorialGrowl(InfoType.Warning, LocalizationHelper.GetString("TutorialChallenge.Level.DragDropUpdate.WrongVersion"));
            }

            return;
        }

        if (PendingUpdateApplier.MatchOtaPackageFileName(fileName) is { } ota)
        {
            if (ota.To.Equals(currentVersion, StringComparison.OrdinalIgnoreCase)
                && ota.Arch.Equals(normalizedArch, StringComparison.OrdinalIgnoreCase))
            {
                MarkIdentified(OtaPackageIdentified, () => OtaPackageIdentified = true, "TutorialChallenge.Level.DragDropUpdate.Identified.OtaPackage");
            }
            else
            {
                ShowTutorialGrowl(InfoType.Warning, LocalizationHelper.GetString("TutorialChallenge.Level.DragDropUpdate.WrongVersion"));
            }

            return;
        }

        // zip 扫描开销较高（万级 entry），对齐真实 HandleImportedPackageAsync 放后台线程；后续置标志与 Growl 要碰 UI，不摘上下文
        DateTimeOffset dt = DateTimeOffset.MinValue;
        bool isResourcePackage = await Task.Run(() => ResourceUpdater.IsResourcePackage(packagePath, out dt));
        if (isResourcePackage && dt == VersionUpdateSettingsUserControlModel.Instance.ResourceDateTime)
        {
            MarkIdentified(ResourcePackageIdentified, () => ResourcePackageIdentified = true, "TutorialChallenge.Level.DragDropUpdate.Identified.ResourcePackage");
            return;
        }

        ShowTutorialGrowl(InfoType.Warning, LocalizationHelper.GetString(
            isResourcePackage ? "TutorialChallenge.Level.DragDropUpdate.WrongVersion" : "TutorialChallenge.Level.DragDropUpdate.Unknown"));
    }

    /// <summary>
    /// 已识别类别重复拖入时仍按成功提示（幂等，可重玩确认），未识别则置标志并提示，随后重算终态判定。
    /// </summary>
    /// <param name="alreadyIdentified">该类别此前是否已识别。</param>
    /// <param name="mark">首次识别时置标志的动作。</param>
    /// <param name="identifiedKey">首次识别的本地化 key。</param>
    private void MarkIdentified(bool alreadyIdentified, Action mark, string identifiedKey)
    {
        var key = alreadyIdentified ? "TutorialChallenge.Level.DragDropUpdate.AlreadyIdentified" : identifiedKey;
        ShowTutorialGrowl(InfoType.Success, LocalizationHelper.GetString(key));
        if (!alreadyIdentified)
        {
            mark();
            Evaluate();
        }
    }

    private static void ShowTutorialGrowl(InfoType type, string msg)
        => GrowlHelper.Show(type, msg, () => {
            switch (type)
            {
                case InfoType.Success:
                    Growl.Success(msg, "TutorialChallenge");
                    break;
                case InfoType.Warning:
                    Growl.Warning(msg, "TutorialChallenge");
                    break;
                case InfoType.Error:
                    Growl.Error(msg, "TutorialChallenge");
                    break;
                default:
                    Growl.Info(msg, "TutorialChallenge");
                    break;
            }
        });

    private static bool TryGetDroppedZipFile(DragEventArgs e, out string packagePath)
    {
        packagePath = string.Empty;
        if (!e.Data.GetDataPresent(DataFormats.FileDrop))
        {
            return false;
        }

        if (e.Data.GetData(DataFormats.FileDrop) is not string[] files || files.Length == 0)
        {
            return false;
        }

        string candidatePath = files[0];
        if (!File.Exists(candidatePath) || !candidatePath.EndsWith(".zip", StringComparison.OrdinalIgnoreCase))
        {
            return false;
        }

        packagePath = candidatePath;
        return true;
    }

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
    /// 重置沙盒，等价重新进入关卡：任务、完成态、伪造库存、窗口级设置沙盒、页签与搜索全部回到进关初始状态。
    /// </summary>
    public void Reset()
    {
        EnableAdvancedSettings = false;
        SelectedTab = 0;
        IsCompleted = false;
        ExportLogsClicked = false;
        FullPackageIdentified = false;
        OtaPackageIdentified = false;
        ResourcePackageIdentified = false;
        _fakeInventories.Clear();
        SettingsSearchText = string.Empty;
        ShowAdvancedSettings = false;
        SandboxSchedule.Reset();
        if (IsSettingsSearchLevel)
        {
            BuildSettingsSearchDecoySections();
        }

        AssignSectionGridRows();
        OnSettingItemValueChanged();
        BuildSandbox();
    }

    /// <summary>
    /// 构建沙盒设置页分区导航，按本关 <see cref="TutorialChallengeLevel.SettingSectionKinds"/> 白名单投放分区，
    /// 未声明则不构建设置分区（此时 <see cref="ShowSettingsPanel"/> 为 false，设置页整页不显示）。
    /// </summary>
    private void BuildSettingSections()
    {
        foreach (var section in SandboxSettingSections)
        {
            section.PropertyChanged -= OnSettingSectionPropertyChanged;
        }

        SandboxSettingSections.Clear();
        foreach (var kind in _level.SettingSectionKinds ?? [])
        {
            switch (kind)
            {
                case TutorialChallengeSettingSectionKind.Schedule:
                    SandboxSettingSections.Add(new TutorialChallengeSettingSection(
                        TutorialChallengeSettingSectionKind.Schedule,
                        "Settings.Schedule.Title",
                        isAdvanced: false));
                    break;
                case TutorialChallengeSettingSectionKind.IssueReport:
                    SandboxSettingSections.Add(new TutorialChallengeSettingSection(
                        TutorialChallengeSettingSectionKind.IssueReport,
                        "Settings.IssueReport.Title",
                        isAdvanced: false));
                    break;
                case TutorialChallengeSettingSectionKind.Decoy:
                    BuildSettingsSearchDecoySections();
                    break;
            }
        }

        foreach (var section in SandboxSettingSections)
        {
            // Decoy 分区在 BuildSettingsSearchDecoySections 内已挂载，先退订再挂保证幂等
            section.PropertyChanged -= OnSettingSectionPropertyChanged;
            section.PropertyChanged += OnSettingSectionPropertyChanged;
        }

        AssignSectionGridRows();
        OnSettingItemValueChanged();
    }

    /// <summary>
    /// 重建 ｢设置搜索｣ 关卡的干扰分区与 CheckBox 项：40 个分区、每分区 5 项、每分区恰好 1 个目标项
    /// （文本内嵌随机大小写的 ｢maa｣ 字样，可经搜索框按忽略大小写命中），其余 4 项为不含 ｢maa｣ 的
    /// 随机大小写字母串，分区名同样为随机大小写且不含 ｢maa｣ 的字母串。
    /// 每次进关与重置时重新生成，保证观感随机。
    /// </summary>
    private void BuildSettingsSearchDecoySections()
    {
        for (var i = SandboxSettingSections.Count - 1; i >= 0; i--)
        {
            if (SandboxSettingSections[i].Kind == TutorialChallengeSettingSectionKind.Decoy)
            {
                SandboxSettingSections[i].PropertyChanged -= OnSettingSectionPropertyChanged;
                SandboxSettingSections.RemoveAt(i);
            }
        }

        for (var sectionIndex = 0; sectionIndex < SettingsSearchSectionCount; sectionIndex++)
        {
            // 每分区 5 个槽位随机挑一个放目标项，其余填非目标随机串
            var targetSlot = _random.Next(SettingsSearchItemsPerSection);
            var items = new List<TutorialChallengeDecoyItem>(SettingsSearchItemsPerSection);
            for (var slot = 0; slot < SettingsSearchItemsPerSection; slot++)
            {
                var isTarget = slot == targetSlot;
                var text = isTarget ? GenerateTargetItemText() : GenerateDecoyItemText();
                items.Add(new TutorialChallengeDecoyItem(text, isTarget, OnDecoyItemCheckChanged));
            }

            var section = new TutorialChallengeSettingSection(
                TutorialChallengeSettingSectionKind.Decoy,
                "TutorialChallenge.Level.SettingsSearch.DecoySection",
                isAdvanced: false,
                items: items,
                displayText: GenerateDecoyItemText());
            section.PropertyChanged += OnSettingSectionPropertyChanged;
            SandboxSettingSections.Add(section);
        }
    }

    /// <summary>
    /// 干扰项 CheckBox 勾选变化回调：勾上非目标项弹 Info 提示（不清除勾选，由玩家自行取消），
    /// 任何变化都重算终态判定。
    /// </summary>
    /// <param name="item">变化的干扰项。</param>
    private void OnDecoyItemCheckChanged(TutorialChallengeDecoyItem item)
    {
        if (item is { IsChecked: true, IsTarget: false })
        {
            var msg = LocalizationHelper.GetString("TutorialChallenge.Level.SettingsSearch.WrongPick");
            GrowlHelper.Show(InfoType.Info, msg, () => Growl.Info(msg, "TutorialChallenge"));
        }

        NotifyOfPropertyChange(nameof(SandboxDecoyTargetsAllChecked));
        Evaluate();
    }

    /// <summary>
    /// 生成一个干扰项显示文本：随机大小写字母串（6-8 位），保证不偶然含 ｢maa｣（任意大小写形式）。
    /// </summary>
    /// <returns>不含目标标记的随机大小写文本。</returns>
    private string GenerateDecoyItemText()
    {
        string text;
        do
        {
            var length = _random.Next(6, 9);
            text = GenerateRandomLetters(length);
        }
        while (text.Contains(SettingsSearchTargetMarker, StringComparison.OrdinalIgnoreCase));

        return text;
    }

    /// <summary>
    /// 生成一个目标项显示文本：随机大小写字母串（总长 6-8），中间嵌入的 ｢maa｣ 三个字母各自随机大小写。
    /// </summary>
    /// <returns>含随机大小写 ｢maa｣ 字样的随机文本。</returns>
    private string GenerateTargetItemText()
    {
        var totalLength = _random.Next(6, 9);
        var marker = RandomizeCase(SettingsSearchTargetMarker);
        var prefixLength = _random.Next(0, totalLength - marker.Length + 1);
        var suffixLength = totalLength - marker.Length - prefixLength;
        return GenerateRandomLetters(prefixLength) + marker + GenerateRandomLetters(suffixLength);
    }

    /// <summary>
    /// 生成指定长度的随机大小写字母串（每个字母独立随机大小写）。
    /// </summary>
    /// <param name="length">字母个数。</param>
    /// <returns>随机大小写字母串。</returns>
    private string GenerateRandomLetters(int length)
    {
        var chars = new char[length];
        for (var i = 0; i < length; i++)
        {
            chars[i] = RandomCaseChar((char)('a' + _random.Next(26)));
        }

        return new string(chars);
    }

    /// <summary>
    /// 把输入串逐字符随机大小写化（输入应为小写字母串）。
    /// </summary>
    /// <param name="text">小写字母串。</param>
    /// <returns>逐字符随机大小写后的串。</returns>
    private string RandomizeCase(string text)
    {
        var chars = text.ToCharArray();
        for (var i = 0; i < chars.Length; i++)
        {
            chars[i] = RandomCaseChar(chars[i]);
        }

        return new string(chars);
    }

    /// <summary>
    /// 把单个小写字母随机返回大写或小写形式。
    /// </summary>
    /// <param name="c">小写字母。</param>
    /// <returns>随机大小写后的字符。</returns>
    private char RandomCaseChar(char c) => _random.Next(2) == 0 ? c : char.ToUpperInvariant(c);

    /// <summary>
    /// 搜索文本变化后收敛高级分区可见性：分区过滤本身归口 <see cref="SearchHighlightBehavior"/>
    /// （行为层按可视树命中写回各分区 <see cref="TutorialChallengeSettingSection.IsVisibleInSearch"/>，
    /// 搜索清空时亦由行为层全量恢复，对齐真实设置页），VM 不再写该属性，只负责让高级分区
    /// 在非空搜索下保持展开、空搜索时回到用户开关态。
    /// </summary>
    private void ApplySettingsSearch()
    {
        // 搜索非空时高级分区须随搜索态展开（对齐真实语义），否则命中的高级项被折叠看不见
        UpdateAdvancedSettingsVisibility();
        NotifyOfPropertyChange(nameof(HasVisibleAdvancedSettings));
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
    /// Gets a value indicating whether 导出日志按钮已被点击过（ExportLogs 关卡的判定源）。
    /// </summary>
    public bool ExportLogsClicked
    {
        get;
        private set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets a value indicating whether 沙盒 ｢生成日志压缩包｣ 模拟生成中（期间按钮禁用并切换文案）。
    /// </summary>
    public bool IsExportingLogs
    {
        get;
        private set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(CanExportLogs));
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 当前可点击 ｢生成日志压缩包｣（非生成中）。
    /// </summary>
    public bool CanExportLogs => !IsExportingLogs;

    /// <summary>
    /// 沙盒内模拟 ｢生成日志压缩包｣：短暂延迟后弹成功 Growl（token 限定在关卡窗口内），并记录点击态供关卡判定。
    /// </summary>
    /// <returns>Task</returns>
    public async Task ExportLogs()
    {
        if (IsExportingLogs)
        {
            return;
        }

        IsExportingLogs = true;
        try
        {
            // 模拟真实导出耗时；结束后回到 UI 线程弹 Growl 与置判定态，不加 ConfigureAwait(false)
            await Task.Delay(1500);
            var msg = LocalizationHelper.GetString("Error.GenerateSupportPayloadSuccessful");
            GrowlHelper.Show(InfoType.Success, msg, () => Growl.Success(msg, "TutorialChallenge"));
            ExportLogsClicked = true;
            Evaluate();
        }
        finally
        {
            IsExportingLogs = false;
        }
    }

    /// <summary>
    /// 打开 debug 目录（转发真实设置页方法，沙盒与真实共用行为）。
    /// </summary>
    public void OpenDebugFolder() => IssueReportUserControlModel.Instance.OpenDebugFolder();

    /// <summary>
    /// 打开 reports 目录（转发真实设置页方法，沙盒与真实共用行为）。
    /// </summary>
    public void OpenReportsFolder() => IssueReportUserControlModel.Instance.OpenReportsFolder();

    /// <summary>
    /// 清理图片缓存（转发真实设置页方法，沙盒与真实共用行为）。
    /// </summary>
    /// <returns>清理任务。</returns>
    public Task ClearImageCache() => IssueReportUserControlModel.Instance.ClearImageCache();

    /// <summary>
    /// 记录用户点开 issue 反馈页（转发真实设置页方法，解锁对应成就）。
    /// </summary>
    public void SetAcknowledgedNightlyWarning() => IssueReportUserControlModel.Instance.SetAcknowledgedNightlyWarning();

    /// <summary>
    /// Gets a value indicating whether 沙盒生成日志按钮当前可点（转发真实设置页属性）。
    /// </summary>
    public bool CanGenerateSupportPayload => IssueReportUserControlModel.Instance.CanGenerateSupportPayload;

    /// <summary>
    /// Gets a value indicating whether 沙盒清理图片缓存按钮当前可点（转发真实设置页属性）。
    /// </summary>
    public bool CanClearImageCache => IssueReportUserControlModel.Instance.CanClearImageCache;

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
        NotifyOfPropertyChange(nameof(SelectedRecruitTask));
        NotifyOfPropertyChange(nameof(ShowFightPanel));
        NotifyOfPropertyChange(nameof(ShowDepotMaintainPanel));
        NotifyOfPropertyChange(nameof(ShowRecruitPanel));
        NotifyOfPropertyChange(nameof(SandboxUseStoneDisplay));
        NotifyOfPropertyChange(nameof(SandboxPreserveTagEnabled));
        NotifyOfPropertyChange(nameof(AutoRecruitPreserveTagList));
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
        if (!_level.GoalPredicate(tasks, this))
        {
            return;
        }

        Complete(tasks);
    }

    private void Complete(IReadOnlyList<BaseTask> tasks)
    {
        IsCompleted = true;

        if (_level.SmartPredicate is { } smart && !smart(tasks, this))
        {
            var msg = LocalizationHelper.GetString(_level.SmartHintKey);
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

    // 转发属性经静态 Instance 绑定，真实侧状态变化须转成本实例的通知 UI 才刷新
    private void OnIssueReportPropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        switch (e.PropertyName)
        {
            case nameof(IssueReportUserControlModel.CanClearImageCache):
                NotifyOfPropertyChange(nameof(CanClearImageCache));
                break;
            case nameof(IssueReportUserControlModel.CanGenerateSupportPayload):
                NotifyOfPropertyChange(nameof(CanGenerateSupportPayload));
                break;
        }
    }

    /// <summary>
    /// 窗口关闭或切换到其他关卡时退订外部事件，否则静态的语言切换事件会一直持有本实例。
    /// </summary>
    public void Detach()
    {
        LocalizationHelper.LanguageChanged -= OnLanguageChanged;
        Instances.StageManager.StageDataChanged -= OnStageDataChanged;
        IssueReportUserControlModel.Instance.PropertyChanged -= OnIssueReportPropertyChanged;
        foreach (var item in SandboxItems)
        {
            item.Task.PropertyChanged -= _taskPropertyChangedHandler;
            item.PropertyChanged -= OnSandboxItemPropertyChanged;
        }

        foreach (var item in WeeklyScheduleSource)
        {
            item.PropertyChanged -= OnWeeklyScheduleItemPropertyChanged;
        }

        foreach (var section in SandboxSettingSections)
        {
            section.PropertyChanged -= OnSettingSectionPropertyChanged;
        }

        if (_resetNotifyTimer != null)
        {
            _resetNotifyTimer.Stop();
            _resetNotifyTimer.Close();
            _resetNotifyTimer = null;
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

        foreach (var section in SandboxSettingSections)
        {
            section.RefreshDisplay();
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

/// <summary>
/// 窗口级定时设置沙盒状态。形状对齐真实设置页 ｢定时执行｣ 区，仅保留定时项与强制定时启动两个教学要素，
/// 每项变更经 <paramref name="evaluate"/> 回调触发关卡判定。
/// </summary>
/// <param name="evaluate">状态变更后调用的判定回调。</param>
/// <param name="presetSlots">初始定时槽（三态启用/时/分），null 或空表示默认 1 空槽。</param>
public class TutorialChallengeSandboxSchedule(Action evaluate, IReadOnlyList<(bool? IsEnabled, int Hour, int Minute)>? presetSlots = null) : PropertyChangedBase
{
    private readonly IReadOnlyList<(bool? IsEnabled, int Hour, int Minute)> _presetSlots = presetSlots is { Count: > 0 } ? presetSlots : [(false, 0, 0)];

    /// <summary>
    /// Gets 沙盒定时项列表。按关卡预设槽位构造，对齐真实 ｢定时 N｣ 行的交互（三态勾选 + 时/分）。
    /// </summary>
    public ObservableCollection<TutorialChallengeSandboxTimerItem> TimerList { get; } = [.. BuildTimerList(evaluate, presetSlots is { Count: > 0 } ? presetSlots : [(false, 0, 0)])];

    /// <summary>
    /// Gets 或设置 a value indicating whether 强制定时启动。
    /// </summary>
    public bool ForceScheduledStart
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                evaluate();
            }
        }
    }

    /// <summary>
    /// Gets 或设置 a value indicating whether 强制定时启动前显示窗口（仅强制定时启动开启时可见）。
    /// </summary>
    public bool ShowWindowBeforeForceScheduledStart
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                evaluate();
            }
        }
    }

    /// <summary>
    /// 回到进关初始状态：清空勾选并按预设槽重建定时项。
    /// </summary>
    public void Reset()
    {
        ForceScheduledStart = false;
        ShowWindowBeforeForceScheduledStart = false;
        TimerList.Clear();
        foreach (var item in BuildTimerList(evaluate, _presetSlots))
        {
            TimerList.Add(item);
        }
    }

    private static List<TutorialChallengeSandboxTimerItem> BuildTimerList(Action evaluate, IReadOnlyList<(bool? IsEnabled, int Hour, int Minute)> slots)
    {
        var list = new List<TutorialChallengeSandboxTimerItem>(slots.Count);
        for (var i = 0; i < slots.Count; i++)
        {
            list.Add(new TutorialChallengeSandboxTimerItem(i, evaluate, slots[i].IsEnabled, slots[i].Hour, slots[i].Minute));
        }

        return list;
    }
}

/// <summary>
/// 沙盒定时项。形状对齐真实定时设置的单条定时行（三态启用勾选 + 时/分），变更经回调触发判定。
/// </summary>
/// <param name="id">定时项序号（从 0 起，显示为 ｢定时 N+1｣）。</param>
/// <param name="evaluate">状态变更后调用的判定回调。</param>
/// <param name="isEnabled">初始启用态（三态：勾选/未勾/右键 null）。</param>
/// <param name="hour">初始定时小时（0-23）。</param>
/// <param name="minute">初始定时分钟（0-59）。</param>
public class TutorialChallengeSandboxTimerItem(int id, Action evaluate, bool? isEnabled = false, int hour = 0, int minute = 0) : PropertyChangedBase
{
    /// <summary>
    /// Gets 定时项序号。
    /// </summary>
    public int Id { get; } = id;

    /// <summary>
    /// Gets 或设置 a value indicating whether 本定时项启用（三态：勾选/未勾/右键 null）。
    /// </summary>
    public bool? IsEnabled
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                evaluate();
            }
        }
    } = isEnabled;

    /// <summary>
    /// Gets 或设置 定时小时（0-23）。
    /// </summary>
    public int Hour
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                evaluate();
            }
        }
    } = hour;

    /// <summary>
    /// Gets 或设置 定时分钟（0-59）。
    /// </summary>
    public int Minute
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                evaluate();
            }
        }
    } = minute;
}

/// <summary>
/// 沙盒设置页分区标识，决定右栏投放哪种分区内容。
/// </summary>
public enum TutorialChallengeSettingSectionKind
{
    /// <summary>
    /// 定时执行（沙盒定时面板）。
    /// </summary>
    Schedule,

    /// <summary>
    /// 问题反馈分区（沙盒 ｢生成日志压缩包｣ 按钮）。
    /// </summary>
    IssueReport,

    /// <summary>
    /// 干扰分区（｢设置搜索｣ 关卡的随机 CheckBox 群）。
    /// </summary>
    Decoy,
}

/// <summary>
/// 沙盒设置页分区导航项。形状对齐真实设置页的 SettingItemViewModel（Display/IsAdvanced/搜索折叠），
/// 同时携带分区种类供右栏内容选择器投放对应面板。
/// </summary>
public class TutorialChallengeSettingSection(
    TutorialChallengeSettingSectionKind kind,
    string displayKey,
    bool isAdvanced,
    IReadOnlyList<TutorialChallengeDecoyItem>? items = null,
    string? displayText = null) : PropertyChangedBase, ISearchableSettingSection
{
    /// <summary>
    /// Gets 分区标识（种类名的字符串形式）。
    /// </summary>
    public string Key { get; } = kind.ToString();

    /// <summary>
    /// Gets 分区种类。
    /// </summary>
    public TutorialChallengeSettingSectionKind Kind { get; } = kind;

    /// <summary>
    /// Gets 分区标题本地化 key（构造的 displayText 为空时作为取串来源）。
    /// </summary>
    public string DisplayKey { get; } = displayKey;

    /// <summary>
    /// Gets 分区标题：构造的 displayText 非空时原样显示（干扰分区用随机文本，
    /// 不随语言切换），否则回退为 <see cref="DisplayKey"/> 的本地化串（随语言切换刷新）。
    /// </summary>
    public string Display => displayText ?? LocalizationHelper.GetString(DisplayKey);

    /// <summary>
    /// Gets 本分区持有的干扰 CheckBox 项（仅 <see cref="TutorialChallengeSettingSectionKind.Decoy"/> 分区非空）。
    /// </summary>
    public IReadOnlyList<TutorialChallengeDecoyItem> Items { get; } = items ?? [];

    /// <summary>
    /// Gets a value indicating whether 本分区属于高级分区（导航分组与折叠行为对齐真实设置页）。
    /// </summary>
    public bool IsAdvanced { get; } = isAdvanced;

    /// <summary>
    /// Gets 或设置 本分区在右侧内容 Grid 中的行号（普通分区按序占前段行，高级分区 +1 跳过分隔条行；
    /// 由宿主 VM 在构建/重排分区时统一赋值，对齐真实 <c>SettingItemViewModel.GridRow</c> 语义）。
    /// </summary>
    public int GridRow
    {
        get;
        set => SetAndNotify(ref field, value);
    }

    /// <summary>
    /// Gets 或设置 a value indicating whether 搜索过滤后本分区仍可见。
    /// </summary>
    public bool IsVisibleInSearch
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(IsVisible));
            }
        }
    } = true;

    /// <summary>
    /// Gets 或设置 a value indicating whether 高级分区当前展开（由宿主 VM 的 ｢显示高级设置｣ 开关写入）。
    /// </summary>
    public bool AreAdvancedSettingsVisible
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(IsVisible));
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 本分区当前可见（搜索命中且高级分区已展开）。
    /// </summary>
    public bool IsVisible => IsVisibleInSearch && (!IsAdvanced || AreAdvancedSettingsVisible);

    /// <summary>
    /// 语言切换后刷新分区标题显示。
    /// </summary>
    public void RefreshDisplay() => NotifyOfPropertyChange(nameof(Display));
}

/// <summary>
/// ｢设置搜索｣ 关卡干扰分区的单个 CheckBox 项。文本为运行时生成的随机串（不本地化），
/// 勾选变化经 <paramref name="onCheckChanged"/> 回调上报告知宿主 VM 做提示与判定。
/// </summary>
/// <param name="text">显示文本（目标项含随机大小写的 ｢maa｣ 字样）。</param>
/// <param name="isTarget">是否为关卡要求勾选的目标项。</param>
/// <param name="onCheckChanged">勾选状态变化后的回调。</param>
public class TutorialChallengeDecoyItem(string text, bool isTarget, Action<TutorialChallengeDecoyItem> onCheckChanged) : PropertyChangedBase
{
    /// <summary>
    /// Gets 显示文本。
    /// </summary>
    public string Text { get; } = text;

    /// <summary>
    /// Gets a value indicating whether 本项为关卡目标项（文本内嵌随机大小写的 ｢maa｣）。
    /// </summary>
    public bool IsTarget { get; } = isTarget;

    /// <summary>
    /// Gets 或设置 a value indicating whether 本项已勾选（变化经回调通知宿主 VM）。
    /// </summary>
    public bool IsChecked
    {
        get;
        set {
            if (SetAndNotify(ref field, value))
            {
                onCheckChanged(this);
            }
        }
    }
}
#pragma warning restore SA1402
