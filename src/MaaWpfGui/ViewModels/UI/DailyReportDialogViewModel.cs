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
        NotifyOfPropertyChange(nameof(CalendarPickMode));
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

        _selectedItemId = _selectedItemId == material.ItemId ? null : material.ItemId;
        foreach (var row in Materials)
        {
            row.IsSelected = row.ItemId == _selectedItemId;
        }

        RefreshChart();
    }

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
        Stats =
        [
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Records"), sessions.Count.ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sessions"), sessions.Sum(s => s.Times).ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sanity"), sessions.Sum(s => s.Sanity).ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Medicines"), sessions.Sum(s => s.MedicinesUsed + s.ExpiringMedicinesUsed).ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.DropKinds"), dropKinds.ToString()),
        ];
        NotifyOfPropertyChange(nameof(Stats));

        RefreshChart();
    }

    private void RefreshChart()
    {
        var (from, to) = GetRange();
        var culture = LocalizationHelper.CustomCultureInfo;

        // 日维度按小时分桶（游戏日从 4:00 起），周/月维度按天，年维度按月
        var bucketCount = _dimension switch
        {
            DimensionKind.Day => 24,
            DimensionKind.Year => 12,
            _ => to.DayNumber - from.DayNumber + 1,
        };
        var values = new long[bucketCount];
        foreach (var (day, session) in _sessions)
        {
            var value = _selectedItemId is null
                ? session.Sanity
                : session.Drops.GetValueOrDefault(_selectedItemId);
            if (value == 0)
            {
                continue;
            }

            var index = _dimension switch
            {
                DimensionKind.Day => session.StartTime.ToYjDateTime().Hour,
                DimensionKind.Year => day.Month - 1,
                _ => day.DayNumber - from.DayNumber,
            };
            if (index >= 0 && index < bucketCount)
            {
                values[index] += value;
            }
        }

        string BucketLabel(int i) => _dimension switch
        {
            DimensionKind.Day => $"{(i + 4) % 24:00}:00",
            DimensionKind.Year => new DateOnly(from.Year, i + 1, 1).ToString("MMM", culture),
            _ => from.AddDays(i).ToString("MM-dd ddd", culture),
        };

        // 刻度标签只标在对应柱子正下方：日维度每 6 小时，周维度每天（星期），月维度逢 1/5/10/… 号，年维度每月
        string TickLabel(int i) => _dimension switch
        {
            DimensionKind.Day => i % 6 == 0 ? $"{(i + 4) % 24}" : string.Empty,
            DimensionKind.Week => from.AddDays(i).ToString("ddd", culture),
            DimensionKind.Month => (i + 1) is 1 or 5 or 10 or 15 or 20 or 25 or 30 ? $"{i + 1}" : string.Empty,
            _ => $"{i + 1}",
        };

        var max = values.Max();
        ChartBars = values
            .Select((v, i) => new DailyReportChartBarViewModel(max == 0 ? 0 : (double)v / max, $"{BucketLabel(i)}  {v:N0}", TickLabel(i), v == 0 ? string.Empty : v.FormatNumber()))
            .ToList();
        ChartTitle = _selectedItemId is null
            ? LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sanity")
            : ItemListHelper.GetItemName(_selectedItemId) ?? _selectedItemId;
        NotifyOfPropertyChange(nameof(ChartBars));
        NotifyOfPropertyChange(nameof(ChartTitle));
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
            BuildMetaText(session.Times, session.Sanity, session.MedicinesUsed + session.ExpiringMedicinesUsed, session.StonesUsed),
            session.Drops);
    }

    private static DailyReportRowViewModel BuildRow(string stage, List<CombatSession> sessions)
    {
        var times = sessions.Sum(s => s.Times);
        var drops = new Dictionary<string, int>();
        foreach (var drop in sessions.SelectMany(s => s.Drops))
        {
            drops[drop.Key] = drops.GetValueOrDefault(drop.Key) + drop.Value;
        }

        return new DailyReportRowViewModel(
            stage,
            string.Empty,
            BuildMetaText(times, sessions.Sum(s => s.Sanity), sessions.Sum(s => s.MedicinesUsed + s.ExpiringMedicinesUsed), sessions.Sum(s => s.StonesUsed)),
            drops);
    }

    private static string BuildMetaText(int times, int sanity, int medicines, int stones)
    {
        var parts = new List<string>
        {
            LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.SessionsFormat", times),
            LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.SanityFormat", sanity),
        };
        if (medicines > 0)
        {
            parts.Add(LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.MedicinesFormat", medicines));
        }

        if (stones > 0)
        {
            parts.Add(LocalizationHelper.GetStringFormat("TaskQueue.DailyReport.Meta.StonesFormat", stones));
        }

        return string.Join(" · ", parts);
    }
}
