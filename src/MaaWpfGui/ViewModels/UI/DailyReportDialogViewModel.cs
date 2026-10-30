// <copyright file="DailyReportDialogViewModel.cs" company="MaaAssistantArknights">
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
using System.Globalization;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Extensions;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.DailyReport;
using MaaWpfGui.Utilities.ValueType;
using MaaWpfGui.ViewModels.Items;
using Stylet;

namespace MaaWpfGui.ViewModels.UI;

/// <summary>
/// 作战日报弹窗模型：按自然日/周/月/年查看作战记录，日文件是唯一事实源，周/月/年由现场聚合。
/// </summary>
public class DailyReportDialogViewModel : PropertyChangedBase
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportDialogViewModel"/> class.
    /// </summary>
    protected DailyReportDialogViewModel()
    {
        _anchor = DateOnly.FromDateTime(DateTime.UtcNow.ToYjDate());
        RetentionOptions = new(
            (DailyReportRetention.Day, "TaskQueue.DailyReport.Retention.Day"),
            (DailyReportRetention.Week, "TaskQueue.DailyReport.Retention.Week"),
            (DailyReportRetention.Month, "TaskQueue.DailyReport.Retention.Month"),
            (DailyReportRetention.Year, "TaskQueue.DailyReport.Retention.Year"),
            (DailyReportRetention.Unlimited, "TaskQueue.DailyReport.Retention.Unlimited"));

        LocalizationHelper.LanguageChanged += RefreshLocalization;
    }

    public static DailyReportDialogViewModel Instance { get; } = new();

    /// <summary>
    /// 查看维度。
    /// </summary>
    protected enum DimensionKind
    {
        /// <summary>自然日。</summary>
        Day,

        /// <summary>自然周（周一为始）。</summary>
        Week,

        /// <summary>自然月。</summary>
        Month,

        /// <summary>自然年。</summary>
        Year,

        /// <summary>全部记录（最早一条记录所在游戏日至今日）。</summary>
        All,
    }

    private DimensionKind _dimension = DimensionKind.Day;

    /// <summary>
    /// Gets or sets a value indicating whether 当前维度为自然日（RadioButton 绑定）。
    /// </summary>
    public bool IsDayDimension
    {
        get => _dimension == DimensionKind.Day;
        set
        {
            if (value)
            {
                SetDimension(DimensionKind.Day);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 当前维度为自然周。
    /// </summary>
    public bool IsWeekDimension
    {
        get => _dimension == DimensionKind.Week;
        set
        {
            if (value)
            {
                SetDimension(DimensionKind.Week);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 当前维度为自然月。
    /// </summary>
    public bool IsMonthDimension
    {
        get => _dimension == DimensionKind.Month;
        set
        {
            if (value)
            {
                SetDimension(DimensionKind.Month);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 当前维度为自然年。
    /// </summary>
    public bool IsYearDimension
    {
        get => _dimension == DimensionKind.Year;
        set
        {
            if (value)
            {
                SetDimension(DimensionKind.Year);
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 当前维度为全部记录。
    /// </summary>
    public bool IsAllDimension
    {
        get => _dimension == DimensionKind.All;
        set
        {
            if (value)
            {
                SetDimension(DimensionKind.All);
            }
        }
    }

    /// <summary>
    /// Gets a value indicating whether 当前维度可以翻页与选取日期（全部记录维度没有可翻的时段）。
    /// </summary>
    public bool IsPageable => _dimension != DimensionKind.All;

    private void SetDimension(DimensionKind kind)
    {
        if (_dimension == kind)
        {
            return;
        }

        _dimension = kind;
        NotifyOfPropertyChange(nameof(IsDayDimension));
        NotifyOfPropertyChange(nameof(IsWeekDimension));
        NotifyOfPropertyChange(nameof(IsMonthDimension));
        NotifyOfPropertyChange(nameof(IsYearDimension));
        NotifyOfPropertyChange(nameof(IsAllDimension));
        NotifyOfPropertyChange(nameof(IsPageable));
        NotifyOfPropertyChange(nameof(CalendarPickMode));
        NotifyOfPropertyChange(nameof(IsYearView));
        RefreshAll();
    }

    private DateOnly _anchor;

    /// <summary>
    /// Gets 当前查看时段的显示文本。
    /// </summary>
    public string PeriodLabel { get; private set; } = string.Empty;

    /// <summary>
    /// Gets a value indicating whether 是否可向后翻页（时段起点不超过今日）。
    /// </summary>
    public bool CanGoNext { get; private set; } = true;

    /// <summary>
    /// Gets 汇总统计块。
    /// </summary>
    public IReadOnlyList<DailyReportStatViewModel> Stats { get; private set; } = [];

    /// <summary>
    /// Gets 作战记录行（日维度为逐条 session，周/月/年为按关卡聚合）。
    /// </summary>
    public ObservableCollection<DailyReportRowViewModel> Rows { get; } = [];

    /// <summary>
    /// Gets 保留档位选项。
    /// </summary>
    public LocalizedObservableList<DailyReportRetention> RetentionOptions { get; }

    private DailyReportRetention _selectedRetention = ConfigFactory.Root.Gui.DailyReportRetention;

    /// <summary>
    /// Gets or sets 当前保留档位；已有记录超出新档位时先弹窗确认，确认后写全局配置并立即裁剪，取消则回退原档位。
    /// </summary>
    public DailyReportRetention SelectedRetention
    {
        get => _selectedRetention;
        set
        {
            if (_selectedRetention == value)
            {
                return;
            }

            if (DailyReportStore.ListPrunableDays(value).Count == 0)
            {
                ApplyRetention(value);
                return;
            }

            // 绑定回写期间弹模态框会被绑定引擎吞掉异常，回退通知也不会被 ComboBox 响应，须等回写结束再确认
            Execute.PostToUIThread(() => ConfirmRetention(value));
        }
    }

    private void ConfirmRetention(DailyReportRetention value)
    {
        var prunable = DailyReportStore.ListPrunableDays(value);
        if (prunable.Count > 0)
        {
            var result = MessageBoxHelper.Show(
                LocalizationHelper.GetStringFormat(
                    "TaskQueue.DailyReport.Retention.ConfirmPruneMessage",
                    prunable[0].ToString("yyyy-MM-dd", CultureInfo.InvariantCulture),
                    prunable[^1].ToString("yyyy-MM-dd", CultureInfo.InvariantCulture),
                    prunable.Count),
                LocalizationHelper.GetString("Common.Warning"),
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning);
            if (result != MessageBoxResult.Yes)
            {
                NotifyOfPropertyChange(nameof(SelectedRetention));
                return;
            }
        }

        ApplyRetention(value);
        LoadHistory();
        RefreshAll();
    }

    private void ApplyRetention(DailyReportRetention value)
    {
        SetAndNotify(ref _selectedRetention, value, nameof(SelectedRetention));
        ConfigFactory.Root.Gui.DailyReportRetention = value;
        DailyReportCollector.PruneNow();
    }

    /// <summary>
    /// 向前翻一个当前维度单位（Command 绑定入口）。
    /// </summary>
    public void GoPrevious()
    {
        _anchor = ShiftAnchor(-1);
        RefreshAll();
    }

    /// <summary>
    /// 向后翻一个当前维度单位（Command 绑定入口）。
    /// </summary>
    public void GoNext()
    {
        _anchor = ShiftAnchor(1);
        RefreshAll();
    }

    /// <summary>
    /// 回到包含今日的时段（Command 绑定入口）。
    /// </summary>
    public void GoToday()
    {
        IsCalendarOpen = false;
        _anchor = Today;
        RefreshAll();
    }

    private DateOnly ShiftAnchor(int direction)
    {
        var shifted = _dimension switch
        {
            DimensionKind.Week => _anchor.AddDays(7 * direction),
            DimensionKind.Month => new DateOnly(_anchor.Year, _anchor.Month, 1).AddMonths(direction),
            DimensionKind.Year => new DateOnly(_anchor.Year + direction, 1, 1),
            _ => _anchor.AddDays(direction),
        };
        return shifted > Today ? Today : shifted;
    }

    private static DateOnly Today => DateOnly.FromDateTime(DateTime.UtcNow.ToYjDate());

    /// <summary>
    /// Gets or sets a value indicating whether 日期选择弹窗是否打开。
    /// </summary>
    public bool IsCalendarOpen { get; set => SetAndNotify(ref field, value); }

    /// <summary>
    /// Gets 日历的选取粒度：日/周维度选日，月维度选月，年维度选年。
    /// </summary>
    public CalendarMode CalendarPickMode => _dimension switch
    {
        DimensionKind.Month => CalendarMode.Year,
        DimensionKind.Year => CalendarMode.Decade,
        _ => CalendarMode.Month,
    };

    /// <summary>
    /// Gets 日历可选的最晚日期（游戏日的今日）。
    /// </summary>
    public static DateTime CalendarMaxDate => Today.ToDateTime(TimeOnly.MinValue);

    /// <summary>
    /// Gets 日历当前选中的日期（当前锚点）。
    /// </summary>
    public DateTime CalendarDate => _anchor.ToDateTime(TimeOnly.MinValue);

    /// <summary>
    /// 生成日历日期格的渲染快照：当前查看时段、游戏日的今日与有记录日期。
    /// </summary>
    /// <returns>日历状态。</returns>
    public DailyReportCalendarState BuildCalendarState()
    {
        // 今日之后的日期格不可选，色带终点截到今日，才能在今日收圆角而不是在可见范围内断成直角
        var (from, to) = GetRange();
        if (to > Today)
        {
            to = Today;
        }

        return new(
            from.ToDateTime(TimeOnly.MinValue),
            to.ToDateTime(TimeOnly.MinValue),
            CalendarMaxDate,
            DailyReportStore.ListDays().Select(d => d.ToDateTime(TimeOnly.MinValue)).ToHashSet());
    }

    /// <summary>
    /// 日历选取完成后跳转到所选日期所在时段，并关闭弹窗。
    /// </summary>
    /// <param name="date">所选日期；月/年维度只取到对应月份或年份。</param>
    public void JumpTo(DateTime date)
    {
        var picked = DateOnly.FromDateTime(date);
        _anchor = picked > Today ? Today : picked;
        IsCalendarOpen = false;
        RefreshAll();
    }

    /// <summary>
    /// 语言切换后重算展示文本。
    /// </summary>
    public void RefreshLocalization()
    {
        RetentionOptions.RefreshLocalization();
        RefreshAll();
    }

    /// <summary>
    /// Gets or sets a value indicating whether 列表按材料汇总（否则按关卡）；仅在本次运行期间保持。
    /// </summary>
    public bool IsMaterialView
    {
        get;
        set
        {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(IsStageView));

                // 选中物品只在按材料视图有入口可以取消，回到按关卡视图时图表回到理智消耗
                if (!value && _selectedItemId is not null)
                {
                    SelectItem(null, null);
                }
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 列表按关卡展示（RadioButton 绑定）。
    /// </summary>
    public bool IsStageView
    {
        get => !IsMaterialView;
        set => IsMaterialView = !value;
    }

    /// <summary>
    /// Gets 按材料视图的物品行（按游戏物品顺序）。
    /// </summary>
    public IReadOnlyList<DailyReportMaterialViewModel> Materials { get; private set; } = [];

    private string? _selectedItemId;

    /// <summary>
    /// 物品列表点击：切换柱状图展示的物品，点击已选中的物品取消选中、回到理智消耗（事件绑定入口）。
    /// </summary>
    /// <param name="sender">物品列表。</param>
    /// <param name="e">鼠标事件。</param>
    public void OnMaterialClick(object sender, MouseButtonEventArgs e)
    {
        if ((e.OriginalSource as FrameworkElement)?.DataContext is not DailyReportMaterialViewModel material)
        {
            return;
        }

        SelectItem(_selectedItemId == material.ItemId ? null : material.ItemId, null);
    }

    /// <summary>
    /// 切换图表展示的物品并同步物品格的选中态。
    /// </summary>
    /// <param name="itemId">物品 id；为 null 时回到理智消耗。</param>
    /// <param name="highlightStage">排名中高亮的关卡（从按关卡浮层跳转时的来源关卡），不高亮时为 null。</param>
    private void SelectItem(string? itemId, string? highlightStage)
    {
        _selectedItemId = itemId;
        _highlightStage = highlightStage;
        foreach (var row in Materials)
        {
            row.IsSelected = row.ItemId == _selectedItemId;
        }

        if (!CanRank)
        {
            IsRankMode = false;
        }

        NotifyOfPropertyChange(nameof(HasSelectedItem));
        NotifyOfPropertyChange(nameof(CanRank));
        RefreshChart();
    }

    private string? _highlightStage;

    /// <summary>
    /// Gets a value indicating whether 图表当前展示某种物品（否则为理智消耗）。
    /// </summary>
    public bool HasSelectedItem => _selectedItemId is not null;

    /// <summary>
    /// Gets a value indicating whether 当前选中的物品可以做关卡排名（仅养成材料），决定 ｢趋势 / 关卡排名｣ 切换是否出现。
    /// </summary>
    public bool CanRank => _selectedItemId is not null && IsRankable(_selectedItemId);

    /// <summary>
    /// 从按关卡视图的掉率浮层跳转：切到按材料视图、选中该物品并显示关卡排名，高亮来源关卡。
    /// </summary>
    /// <param name="drop">浮层对应的掉落物。</param>
    public void ShowRanking(DailyReportDropItemViewModel drop)
    {
        IsMaterialView = true;
        _chartExpandedOnce = true;
        NotifyOfPropertyChange(nameof(IsChartCollapsed));
        SelectItem(drop.ItemId, drop.Stage);
        IsRankMode = true;
    }

    // 从浮层跳转时临时展开图表（本次打开内有效），不覆盖用户持久化的折叠选择
    private bool _chartExpandedOnce;

    /// <summary>
    /// 计算某种物品的单件理智排名：数据取全部历史记录（时段内样本太少，排名误差大到没有参考意义），
    /// 只列出当前时段刷过的关卡；可靠样本在前、按单件理智升序，样本不足的排到末尾。
    /// </summary>
    /// <param name="itemId">物品 id。</param>
    /// <returns>排名行；没有任何列出的关卡在历史上掉落过该物品时为空。</returns>
    public List<DailyReportRankRowViewModel> BuildRanking(string itemId)
    {
        // 只对养成材料排名；龙门币、家具零件等通用产出几乎每关都掉，按理智比较没有意义
        if (!IsRankable(itemId))
        {
            return [];
        }

        var stages = _sessions.Select(s => s.Session.Stage).ToHashSet();
        var rows = HistoryRows(itemId)
            .Where(r => stages.Contains(r.Stage))
            .OrderByDescending(r => r.IsReliable)
            .ThenBy(r => r.SanityPerItem)
            .ToList();

        // 横条比例为最划算关卡的单件理智与本关之比，越费理智越短；满长取各行误差带上界的最大值，让误差带完整显示。
        // 基准与满长只看可靠样本：样本不足的偶然低值或超宽误差带会让其他横条整体变短，这些行超出满长的部分截断
        var reliable = rows.Where(r => r.IsReliable).ToList();
        var basis = reliable.Count > 0 ? reliable : rows;
        var best = basis.Select(r => r.SanityPerItem).DefaultIfEmpty(1).Min();
        var scale = basis.Select(r => best / r.SanityPerItem * r.UpperBoundFactor).Where(double.IsFinite).DefaultIfEmpty(1).Max();
        foreach (var row in rows)
        {
            row.SetBarScale(best, Math.Max(1, scale));
        }

        return rows;
    }

    // 全部历史记录按关卡分组的逐条记录（场次与掉落），排名时按物品现算累计量；窗口打开与裁剪记录后重读。
    // 保留逐条记录而非只存合计：误差按记录间的实际波动估算，未掉落该物品的记录也是样本
    private Dictionary<string, (int Sanity, List<CombatSession> Sessions)> _history = [];

    // 最早一条记录所在的游戏日，没有记录时为 null
    private DateOnly? _historyStart;

    /// <summary>
    /// 读取全部历史记录并按关卡分组，供单件理智排名使用。
    /// </summary>
    public void LoadHistory()
    {
        var days = DailyReportStore.LoadRange(DateOnly.MinValue, DateOnly.MaxValue);
        _historyStart = days.Count > 0 ? days.Min(d => d.Day) : null;
        _history = days
            .SelectMany(d => d.Data.Sessions)
            .Where(s => s.Times > 0)
            .GroupBy(s => s.Stage)
            .ToDictionary(g => g.Key, g => (g.Sum(s => s.Sanity), g.ToList()));
    }

    private IEnumerable<DailyReportRankRowViewModel> HistoryRows(string itemId)
    {
        foreach (var (stage, (sanity, sessions)) in _history)
        {
            var stats = default(DailyReportRankRowViewModel.DropStats);
            foreach (var session in sessions)
            {
                stats.Add(session.Times, session.Drops.GetValueOrDefault(itemId));
            }

            if (stats.Count > 0 && sanity > 0)
            {
                yield return new DailyReportRankRowViewModel(stage, stats, sanity);
            }
        }
    }

    /// <summary>
    /// Gets 排名卡片标题旁的口径说明：列出的关卡范围与数据来源。
    /// </summary>
    public string RankScopeText => _historyStart is { } start
        ? LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Rank.ScopeFormat", start.ToString("yyyy-MM-dd", LocalizationHelper.CustomCultureInfo))
        : string.Empty;

    /// <summary>
    /// Gets 历史最优关卡提示：历史上单件理智最低的可靠关卡不在列表里（当前时段没刷过）时给出，否则为空。
    /// </summary>
    public string RankHistoryBestText { get; private set; } = string.Empty;

    /// <summary>
    /// Gets a value indicating whether 显示历史最优关卡提示。
    /// </summary>
    public bool HasRankHistoryBest => RankHistoryBestText.Length > 0;

    private string BuildHistoryBestText(string itemId, IReadOnlyList<DailyReportRankRowViewModel> rows)
    {
        var best = HistoryRows(itemId).Where(r => r.IsReliable).MinBy(r => r.SanityPerItem);
        if (best is null || rows.Any(r => r.Stage == best.Stage))
        {
            return string.Empty;
        }

        return LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Rank.HistoryBestFormat", best.Stage, best.ValueText, best.ErrorText);
    }

    /// <summary>
    /// 物品是否参与单件理智排名：仅 item_index 中归类为 MATERIAL 的养成材料。
    /// </summary>
    private static bool IsRankable(string itemId) =>
        ItemListHelper.ArkItems.TryGetValue(itemId, out var item) && item.ClassifyType == "MATERIAL";

    /// <summary>
    /// 生成掉率浮层里的排名摘要：该物品在本关卡的名次、参与排名的关卡数与误差。
    /// </summary>
    /// <param name="drop">浮层对应的掉落物。</param>
    /// <returns>摘要文本；只有一个关卡掉落或本关卡无从计算时为空。</returns>
    public string BuildRankSummary(DailyReportDropItemViewModel drop)
    {
        var rows = BuildRanking(drop.ItemId);
        var index = rows.FindIndex(r => r.Stage == drop.Stage);
        if (rows.Count < 2 || index < 0)
        {
            return string.Empty;
        }

        var row = rows[index];
        var key = row.IsReliable ? "TaskQueue.DailyReport.Rank.SummaryFormat" : "TaskQueue.DailyReport.Rank.SummaryLowSampleFormat";
        return LocalizationHelper.GetStringFormat(key, index + 1, rows.Count, row.ErrorText);
    }

    /// <summary>
    /// Gets or sets a value indicating whether 图表卡片显示关卡排名（否则显示趋势）；仅在选中物品时可切换。
    /// </summary>
    public bool IsRankMode
    {
        get;
        set
        {
            if (SetAndNotify(ref field, value))
            {
                NotifyOfPropertyChange(nameof(IsTrendMode));
            }
        }
    }

    /// <summary>
    /// Gets or sets a value indicating whether 图表卡片显示趋势（RadioButton 绑定）。
    /// </summary>
    public bool IsTrendMode
    {
        get => !IsRankMode;
        set => IsRankMode = !value;
    }

    /// <summary>
    /// Gets 当前选中物品的单件理智排名。
    /// </summary>
    public IReadOnlyList<DailyReportRankRowViewModel> RankRows { get; private set; } = [];

    /// <summary>
    /// Gets or sets a value indicating whether 图表卡片已折叠；折叠状态写入全局配置，下次打开保持。
    /// </summary>
    public bool IsChartCollapsed
    {
        get => ConfigFactory.Root.Gui.DailyReportChartCollapsed && !_chartExpandedOnce;
        set
        {
            _chartExpandedOnce = false;
            if (ConfigFactory.Root.Gui.DailyReportChartCollapsed == value)
            {
                NotifyOfPropertyChange();
                return;
            }

            ConfigFactory.Root.Gui.DailyReportChartCollapsed = value;
            NotifyOfPropertyChange();
        }
    }

    /// <summary>
    /// 折叠或展开图表卡片（Command 绑定入口）。
    /// </summary>
    public void ToggleChartCollapsed()
    {
        IsChartCollapsed = !IsChartCollapsed;
    }

    /// <summary>
    /// Gets a value indicating whether 当前为年维度（仅年视图提供热力图）。
    /// </summary>
    public bool IsYearView => _dimension == DimensionKind.Year;

    /// <summary>
    /// Gets or sets a value indicating whether 年视图显示热力图（否则显示柱状图）；默认热力图，仅在本次运行期间保持。
    /// </summary>
    public bool IsHeatmapMode
    {
        get => _isHeatmapMode;
        set
        {
            if (SetAndNotify(ref _isHeatmapMode, value))
            {
                NotifyOfPropertyChange(nameof(IsBarMode));
                NotifyOfPropertyChange(nameof(ShowMedicineLegend));
                NotifyOfPropertyChange(nameof(ShowStoneLegend));
            }
        }
    }

    private bool _isHeatmapMode = true;

    /// <summary>
    /// Gets or sets a value indicating whether 年视图显示柱状图（RadioButton 绑定）。
    /// </summary>
    public bool IsBarMode
    {
        get => !IsHeatmapMode;
        set => IsHeatmapMode = !value;
    }

    /// <summary>
    /// Gets 年度热力图的日期格，按行优先排列供 7 行的 UniformGrid 填充：第 r 行依次是各周的星期 r+1，形成每列一周、周一在上的布局。
    /// </summary>
    public IReadOnlyList<DailyReportHeatCellViewModel> HeatCells { get; private set; } = [];

    /// <summary>
    /// Gets 热力图列数（周数）。
    /// </summary>
    public int HeatColumns { get; private set; } = 53;

    /// <summary>
    /// Gets 热力图上方的月份标签及其所在列。
    /// </summary>
    public IReadOnlyList<HeatMonthLabel> HeatMonthLabels { get; private set; } = [];

    /// <summary>
    /// 热力图月份标签（属性形式，WPF 绑定不能读取 ValueTuple 字段）。
    /// </summary>
    /// <param name="Column">该月 1 日所在的列。</param>
    /// <param name="Label">月份文本。</param>
    public record HeatMonthLabel(int Column, string Label);

    /// <summary>
    /// 热力图格子点击：跳到当天的日视图（事件绑定入口）。
    /// </summary>
    /// <param name="sender">热力图。</param>
    /// <param name="e">鼠标事件。</param>
    public void OnHeatCellClick(object sender, MouseButtonEventArgs e)
    {
        if ((e.OriginalSource as FrameworkElement)?.DataContext is not DailyReportHeatCellViewModel { Date: { } date })
        {
            return;
        }

        _anchor = date > Today ? Today : date;
        SetDimension(DimensionKind.Day);
    }

    /// <summary>
    /// Gets 药剂折线；不画时 <see cref="DailyReportOverlayLine.IsVisible"/> 为 false。
    /// </summary>
    public DailyReportOverlayLine? MedicineLine { get; private set; }

    /// <summary>
    /// Gets 源石折线。
    /// </summary>
    public DailyReportOverlayLine? StoneLine { get; private set; }

    /// <summary>
    /// Gets a value indicating whether 柱状图叠加了折线（显示理智消耗且药剂或源石不全为 0）。
    /// </summary>
    public bool HasOverlay => MedicineLine?.IsVisible == true || StoneLine?.IsVisible == true;

    // 年视图显示热力图时不画折线，图例一并隐藏
    private bool IsShowingBars => !(IsYearView && IsHeatmapMode);

    /// <summary>
    /// Gets a value indicating whether 显示药剂折线图例。
    /// </summary>
    public bool ShowMedicineLegend => IsShowingBars && MedicineLine?.IsVisible == true;

    /// <summary>
    /// Gets a value indicating whether 显示源石折线图例。
    /// </summary>
    public bool ShowStoneLegend => IsShowingBars && StoneLine?.IsVisible == true;

    /// <summary>
    /// Gets 折线右轴上限文本。
    /// </summary>
    public string OverlayAxisMaxText { get; private set; } = string.Empty;

    /// <summary>
    /// Gets 柱状图标题（当前展示的指标）。
    /// </summary>
    public string ChartTitle { get; private set; } = string.Empty;

    /// <summary>
    /// Gets 柱状图各柱。
    /// </summary>
    public IReadOnlyList<DailyReportChartBarViewModel> ChartBars { get; private set; } = [];

    private List<(DateOnly Day, CombatSession Session)> _sessions = [];

    /// <summary>
    /// 按当前维度与锚点重算时段范围、聚合记录、物品汇总、柱状图与统计。
    /// </summary>
    public void RefreshAll()
    {
        var (from, to) = GetRange();
        PeriodLabel = BuildPeriodLabel(from, to);
        NotifyOfPropertyChange(nameof(PeriodLabel));
        NotifyOfPropertyChange(nameof(CalendarDate));

        CanGoNext = to < Today;
        NotifyOfPropertyChange(nameof(CanGoNext));

        _sessions = DailyReportStore.LoadRange(from, to)
            .SelectMany(d => d.Data.Sessions.Select(s => (d.Day, Session: s)))
            .ToList();
        var sessions = _sessions.Select(s => s.Session).ToList();

        Rows.Clear();
        var rows = _dimension == DimensionKind.Day
            ? sessions.OrderBy(s => s.StartTime).Select(BuildRow)
            : sessions.GroupBy(s => s.Stage)
                .OrderByDescending(g => g.Sum(s => s.Sanity))
                .ThenBy(g => g.Key, StringComparer.Ordinal)
                .Select(g => BuildRow(g.Key, g.ToList()));
        foreach (var row in rows)
        {
            Rows.Add(row);
        }

        Materials = sessions
            .SelectMany(s => s.Drops.Select(d => (ItemId: d.Key, s.Stage, Count: d.Value)))
            .GroupBy(d => d.ItemId)
            .Select(g => (ItemId: g.Key, Sources: g.GroupBy(d => d.Stage).ToDictionary(s => s.Key, s => s.Sum(d => d.Count))))
            .OrderBy(m => ItemListHelper.ArkItems.TryGetValue(m.ItemId, out var item) ? item.SortId : int.MaxValue)
            .ThenBy(m => m.ItemId, StringComparer.Ordinal)
            .Select(m => new DailyReportMaterialViewModel(m.ItemId, m.Sources, m.ItemId == _selectedItemId))
            .ToList();
        NotifyOfPropertyChange(nameof(Materials));

        var dropKinds = Materials.Count;
        var culture = LocalizationHelper.CustomCultureInfo;
        Stats =
        [
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sessions"), sessions.Sum(s => s.Times).ToString("N0", culture)),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sanity"), sessions.Sum(s => s.Sanity).ToString("N0", culture)),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Medicines"), sessions.Sum(s => s.MedicinesUsed + s.ExpiringMedicinesUsed).ToString("N0", culture)),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Stones"), sessions.Sum(s => s.StonesUsed).ToString("N0", culture)),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.DropKinds"), dropKinds.ToString("N0", culture)),
        ];
        NotifyOfPropertyChange(nameof(Stats));

        RefreshChart();
    }

    private void RefreshChart()
    {
        var (from, to) = GetRange();
        var culture = LocalizationHelper.CustomCultureInfo;

        // 日维度按小时分桶（游戏日从 4:00 起），周/月维度按天，年维度按月，全部记录维度按跨度自适应（见 BuildAllBuckets）
        var allBuckets = _dimension == DimensionKind.All ? BuildAllBuckets(from, to) : null;
        var bucketCount = _dimension switch
        {
            DimensionKind.Day => 24,
            DimensionKind.Year => 12,
            DimensionKind.All => allBuckets!.Count,
            _ => to.DayNumber - from.DayNumber + 1,
        };

        int BucketOf(DateOnly day, CombatSession session) => _dimension switch
        {
            DimensionKind.Day => session.StartTime.ToYjDateTime().Hour,
            DimensionKind.Year => day.Month - 1,
            DimensionKind.All => allBuckets!.FindLastIndex(b => b.From <= day),
            _ => day.DayNumber - from.DayNumber,
        };

        var values = new long[bucketCount];
        var medicines = new long[bucketCount];
        var stones = new long[bucketCount];
        foreach (var (day, session) in _sessions)
        {
            var bucket = BucketOf(day, session);
            if (bucket < 0 || bucket >= bucketCount)
            {
                continue;
            }

            medicines[bucket] += session.MedicinesUsed + session.ExpiringMedicinesUsed;
            stones[bucket] += session.StonesUsed;
            values[bucket] += _selectedItemId is null
                ? session.Sanity
                : session.Drops.GetValueOrDefault(_selectedItemId);
        }

        string BucketLabel(int i) => _dimension switch
        {
            DimensionKind.Day => $"{(i + 4) % 24:00}:00",
            DimensionKind.Year => new DateOnly(from.Year, i + 1, 1).ToString("MMM", culture),
            DimensionKind.All => allBuckets![i].ToolTip,
            _ => from.AddDays(i).ToString("MM-dd ddd", culture),
        };

        // 每根柱子都给出刻度，放不下时由视图按间隔稀疏：日维度为小时，周维度为星期，月维度为日期，年维度为月份
        string TickLabel(int i) => _dimension switch
        {
            DimensionKind.Day => $"{(i + 4) % 24}",
            DimensionKind.Week => from.AddDays(i).ToString("ddd", culture),
            DimensionKind.All => allBuckets![i].Tick,
            _ => $"{i + 1}",
        };

        // 药剂与源石只在显示理智消耗时叠加，两条线共用右轴；全为 0 的那条不画
        var overlayMax = Math.Max(medicines.Max(), stones.Max());
        var showOverlay = _selectedItemId is null && overlayMax > 0;
        var axisMax = NiceAxisMax(overlayMax);

        // 时段尚未过完时（本周、本月、本年），今日之后的桶没有数据而非为 0，折线只画到今日
        var elapsed = _dimension switch
        {
            DimensionKind.Day => bucketCount,
            DimensionKind.Year => to < Today ? bucketCount : from.Year == Today.Year ? Today.Month : 0,
            DimensionKind.All => bucketCount,
            _ => Math.Clamp(Today.DayNumber - from.DayNumber + 1, 0, bucketCount),
        };
        MedicineLine = showOverlay ? new DailyReportOverlayLine(medicines, axisMax, elapsed) : null;
        StoneLine = showOverlay ? new DailyReportOverlayLine(stones, axisMax, elapsed) : null;
        OverlayAxisMaxText = showOverlay ? axisMax.ToString("N0", culture) : string.Empty;

        string BarToolTip(int i)
        {
            var text = $"{BucketLabel(i)}  {values[i].ToString("N0", culture)}";
            if (showOverlay)
            {
                text += "\n" + LocalizationHelper.GetStringFormat(
                    "TaskQueue.DailyReport.Chart.OverlayTipFormat",
                    medicines[i].ToString("N0", culture),
                    stones[i].ToString("N0", culture));
            }

            return text;
        }

        var max = values.Max();
        ChartBars = values
            .Select((v, i) => new DailyReportChartBarViewModel(
                i,
                max == 0 ? 0 : (double)v / max,
                BarToolTip(i),
                TickLabel(i),
                allBuckets?[i].IsMajor ?? false,
                v == 0 || showOverlay ? string.Empty : v.FormatNumber()))
            .ToList();
        ChartTitle = _selectedItemId is null
            ? LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sanity")
            : ItemListHelper.GetItemName(_selectedItemId) ?? _selectedItemId;
        NotifyOfPropertyChange(nameof(ChartBars));
        NotifyOfPropertyChange(nameof(ChartTitle));
        NotifyOfPropertyChange(nameof(MedicineLine));
        NotifyOfPropertyChange(nameof(StoneLine));
        NotifyOfPropertyChange(nameof(HasOverlay));
        NotifyOfPropertyChange(nameof(ShowMedicineLegend));
        NotifyOfPropertyChange(nameof(ShowStoneLegend));
        NotifyOfPropertyChange(nameof(OverlayAxisMaxText));

        RankRows = _selectedItemId is null ? [] : BuildRanking(_selectedItemId);
        foreach (var row in RankRows)
        {
            row.IsHighlighted = row.Stage == _highlightStage;
        }

        RankHistoryBestText = _selectedItemId is null ? string.Empty : BuildHistoryBestText(_selectedItemId, RankRows);
        NotifyOfPropertyChange(nameof(RankRows));
        NotifyOfPropertyChange(nameof(RankHistoryBestText));
        NotifyOfPropertyChange(nameof(HasRankHistoryBest));
        NotifyOfPropertyChange(nameof(RankScopeText));

        if (_dimension == DimensionKind.Year)
        {
            RefreshHeatmap(from, to);
        }
    }

    // 全部记录维度的柱子上限：跨度不超过 30 个月按月分，不超过 30 个季度按季分，再长就把整个跨度均分为 30 段
    private const int MaxAllBuckets = 30;

    private record AllBucket(DateOnly From, string Tick, bool IsMajor, string ToolTip);

    /// <summary>
    /// 生成全部记录维度的分桶。刻度标月份或季度，每年第一个月或第一个季度改标年份并作为主刻度；
    /// 均分时只在跨年的段标年份。
    /// </summary>
    private static List<AllBucket> BuildAllBuckets(DateOnly from, DateOnly to)
    {
        var culture = LocalizationHelper.CustomCultureInfo;
        var buckets = new List<AllBucket>();
        var months = ((to.Year - from.Year) * 12) + to.Month - from.Month + 1;
        if (months <= MaxAllBuckets)
        {
            for (var m = new DateOnly(from.Year, from.Month, 1); m <= to; m = m.AddMonths(1))
            {
                var isMajor = m.Month == 1;
                buckets.Add(new(m, isMajor ? $"{m.Year}" : $"{m.Month}", isMajor, m.ToString("yyyy-MM", culture)));
            }

            return buckets;
        }

        var firstQuarter = new DateOnly(from.Year, ((from.Month - 1) / 3 * 3) + 1, 1);
        var quarters = ((((to.Year - firstQuarter.Year) * 12) + to.Month - firstQuarter.Month) / 3) + 1;
        if (quarters <= MaxAllBuckets)
        {
            for (var q = firstQuarter; q <= to; q = q.AddMonths(3))
            {
                var quarter = ((q.Month - 1) / 3) + 1;
                var isMajor = quarter == 1;
                buckets.Add(new(q, isMajor ? $"{q.Year}" : $"Q{quarter}", isMajor, $"{q.Year} Q{quarter}"));
            }

            return buckets;
        }

        var days = to.DayNumber - from.DayNumber + 1;
        for (var i = 0; i < MaxAllBuckets; i++)
        {
            var start = from.AddDays(days * i / MaxAllBuckets);
            var end = from.AddDays((days * (i + 1) / MaxAllBuckets) - 1);
            var crossesYear = i > 0 && start.Year != buckets[^1].From.Year;
            var tip = start.ToString("yyyy-MM-dd", culture) + " ~ " + end.ToString("yyyy-MM-dd", culture);
            buckets.Add(new(start, crossesYear ? $"{start.Year}" : string.Empty, crossesYear, tip));
        }

        return buckets;
    }

    /// <summary>
    /// 折线右轴上限取不小于最大值的 1/2/5×10ⁿ 整数，刻度好读；最大值本身落在整数时即为上限，点贴顶而不是悬在半空。
    /// </summary>
    private static long NiceAxisMax(long max)
    {
        if (max <= 1)
        {
            return 1;
        }

        long magnitude = 1;
        while (magnitude * 10 < max)
        {
            magnitude *= 10;
        }

        foreach (var step in new long[] { 1, 2, 5, 10 })
        {
            if (step * magnitude >= max)
            {
                return step * magnitude;
            }
        }

        return 10 * magnitude;
    }

    private void RefreshHeatmap(DateOnly from, DateOnly to)
    {
        var culture = LocalizationHelper.CustomCultureInfo;
        var daily = new Dictionary<DateOnly, long>();
        foreach (var (day, session) in _sessions)
        {
            var value = _selectedItemId is null ? session.Sanity : session.Drops.GetValueOrDefault(_selectedItemId);
            daily[day] = daily.GetValueOrDefault(day) + value;
        }

        // 按当年非零值的四分位分 4 档，单日极端值不会把其他格子的颜色都压淡
        var sorted = daily.Values.Where(v => v > 0).Order().ToList();
        long Quantile(double q) => sorted.Count == 0 ? 0 : sorted[(int)((sorted.Count - 1) * q)];
        var cuts = new[] { Quantile(0.25), Quantile(0.5), Quantile(0.75) };
        int Level(long v) => v <= 0 ? 0 : v <= cuts[0] ? 1 : v <= cuts[1] ? 2 : v <= cuts[2] ? 3 : 4;

        // 先按时间顺序排成每列一周、周一在上，年初不足一周的部分用占位格补齐
        var lead = ((int)from.DayOfWeek + 6) % 7;
        var cells = new List<DailyReportHeatCellViewModel>();
        for (var i = 0; i < lead; i++)
        {
            cells.Add(new DailyReportHeatCellViewModel(null, 0, null));
        }

        var months = new List<HeatMonthLabel>();
        for (var day = from; day <= to; day = day.AddDays(1))
        {
            var value = daily.GetValueOrDefault(day);
            var tip = day.ToString("yyyy-MM-dd ddd", culture) + "  " + (value > 0 ? value.ToString("N0", culture) : LocalizationHelper.GetString("TaskQueue.DailyReport.Chart.NoBattle"));
            if (day.Day == 1)
            {
                months.Add(new(cells.Count / 7, day.ToString("MMM", culture)));
            }

            cells.Add(new DailyReportHeatCellViewModel(day > Today ? null : day, day > Today ? 0 : Level(value), day > Today ? null : tip));
        }

        // cells 按时间顺序（列优先）生成，UniformGrid 按行填充，转置后每列才是一周
        var columns = (cells.Count + 6) / 7;
        while (cells.Count < columns * 7)
        {
            cells.Add(new DailyReportHeatCellViewModel(null, 0, null));
        }

        var transposed = new List<DailyReportHeatCellViewModel>(cells.Count);
        for (var row = 0; row < 7; row++)
        {
            for (var column = 0; column < columns; column++)
            {
                transposed.Add(cells[(column * 7) + row]);
            }
        }

        HeatColumns = columns;
        HeatCells = transposed;
        HeatMonthLabels = months;
        NotifyOfPropertyChange(nameof(HeatColumns));
        NotifyOfPropertyChange(nameof(HeatCells));
        NotifyOfPropertyChange(nameof(HeatMonthLabels));
    }

    private (DateOnly From, DateOnly To) GetRange()
    {
        switch (_dimension)
        {
            case DimensionKind.Week:
                {
                    var offsetFromMonday = ((int)_anchor.DayOfWeek + 6) % 7;
                    var monday = _anchor.AddDays(-offsetFromMonday);
                    return (monday, monday.AddDays(6));
                }

            case DimensionKind.Month:
                {
                    var first = new DateOnly(_anchor.Year, _anchor.Month, 1);
                    return (first, first.AddMonths(1).AddDays(-1));
                }

            case DimensionKind.Year:
                return (new DateOnly(_anchor.Year, 1, 1), new DateOnly(_anchor.Year, 12, 31));

            case DimensionKind.All:
                return (_historyStart ?? Today, Today);

            default:
                return (_anchor, _anchor);
        }
    }

    private string BuildPeriodLabel(DateOnly from, DateOnly to)
    {
        var culture = LocalizationHelper.CustomCultureInfo;
        return _dimension switch
        {
            DimensionKind.Week => from.ToString("MM-dd", culture) + " ~ " + to.ToString("MM-dd", culture),
            DimensionKind.Month => _anchor.ToString("yyyy-MM", culture),
            DimensionKind.Year => _anchor.ToString("yyyy", culture),
            DimensionKind.All => _historyStart is null
                ? LocalizationHelper.GetString("TaskQueue.DailyReport.NoData")
                : from.ToString("yyyy-MM-dd", culture) + " ~ " + to.ToString("yyyy-MM-dd", culture),
            _ => _anchor.ToString("yyyy-MM-dd ddd", culture),
        };
    }

    private static DailyReportRowViewModel BuildRow(CombatSession session)
    {
        var timeText = session.StartTime.ToLocalTime().ToString("HH:mm", LocalizationHelper.CustomCultureInfo)
            + " – " + (session.EndTime?.ToLocalTime().ToString("HH:mm", LocalizationHelper.CustomCultureInfo) ?? "?");
        return new DailyReportRowViewModel(
            session.Stage,
            timeText,
            BuildMetaText(session.Times, session.Sanity, session.MedicinesUsed + session.ExpiringMedicinesUsed, session.StonesUsed, AveragePerBattle([session])),
            session.Drops,
            session.Times,
            session.Sanity);
    }

    private static DailyReportRowViewModel BuildRow(string stage, List<CombatSession> sessions)
    {
        var times = sessions.Sum(s => s.Times);
        var sanity = sessions.Sum(s => s.Sanity);
        var drops = new Dictionary<string, int>();
        foreach (var drop in sessions.SelectMany(s => s.Drops))
        {
            drops[drop.Key] = drops.GetValueOrDefault(drop.Key) + drop.Value;
        }

        return new DailyReportRowViewModel(
            stage,
            string.Empty,
            BuildMetaText(times, sanity, sessions.Sum(s => s.MedicinesUsed + s.ExpiringMedicinesUsed), sessions.Sum(s => s.StonesUsed), AveragePerBattle(sessions)),
            drops,
            times,
            sanity);
    }

    private static TimeSpan Duration(CombatSession session) =>
        session.EndTime is { } end && end > session.StartTime ? end - session.StartTime : TimeSpan.Zero;

    /// <summary>
    /// 每场平均耗时；只统计有起止时刻的记录，缺结束时刻的记录场次不计入分母，避免拉低平均值。
    /// </summary>
    private static TimeSpan AveragePerBattle(IEnumerable<CombatSession> sessions)
    {
        var timed = sessions.Where(s => s.Times > 0 && Duration(s) > TimeSpan.Zero).ToList();
        var times = timed.Sum(s => s.Times);
        return times > 0 ? TimeSpan.FromSeconds(Math.Round(timed.Sum(s => Duration(s).TotalSeconds) / times)) : TimeSpan.Zero;
    }

    /// <summary>
    /// 按数量选单数或复数文案 key（各语言在 key 后加 <c>.One</c> 区分单数，不区分单复数的语言两者同文）。
    /// </summary>
    private static string Plural(string key, int count) =>
        LocalizationHelper.GetStringFormat(count == 1 ? key + ".One" : key, count.ToString("N0", LocalizationHelper.CustomCultureInfo));

    private static string BuildMetaText(int times, int sanity, int medicines, int stones, TimeSpan perBattle)
    {
        var parts = new List<string>
        {
            Plural("TaskQueue.DailyReport.Meta.SessionsFormat", times),
            LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.SanityFormat", sanity.ToString("N0", LocalizationHelper.CustomCultureInfo)),
        };
        if (medicines > 0)
        {
            parts.Add(Plural("TaskQueue.DailyReport.Meta.MedicinesFormat", medicines));
        }

        if (stones > 0)
        {
            parts.Add(LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.StonesFormat", stones.ToString("N0", LocalizationHelper.CustomCultureInfo)));
        }

        // 平均耗时含用药、碎石与界面跳转的等待，用于关卡间相对比较，不等于单局实际战斗时长
        if (perBattle > TimeSpan.Zero)
        {
            var text = $"{(int)perBattle.TotalMinutes}:{perBattle.Seconds:00}";
            parts.Add(LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.DurationFormat", text));
        }

        return string.Join(" · ", parts);
    }
}
