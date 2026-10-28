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
using System.Windows.Media;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
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

    private DateOnly ShiftAnchor(int direction)
    {
        return _dimension switch
        {
            DimensionKind.Week => _anchor.AddDays(7 * direction),
            DimensionKind.Month => new DateOnly(_anchor.Year, _anchor.Month, 1).AddMonths(direction),
            DimensionKind.Year => new DateOnly(_anchor.Year + direction, 1, 1),
            _ => _anchor.AddDays(direction),
        };
    }

    /// <summary>
    /// 语言切换后重算展示文本。
    /// </summary>
    public void RefreshLocalization()
    {
        RefreshAll();
    }

    /// <summary>
    /// 按当前维度与锚点重算时段范围、聚合记录与汇总。
    /// </summary>
    public void RefreshAll()
    {
        var (from, to) = GetRange();
        PeriodLabel = BuildPeriodLabel(from, to);
        NotifyOfPropertyChange(nameof(PeriodLabel));

        var today = DateOnly.FromDateTime(DateTime.UtcNow.ToYjDate());
        CanGoNext = from <= today;
        NotifyOfPropertyChange(nameof(CanGoNext));

        var sessions = DailyReportStore.LoadRange(from, to).SelectMany(d => d.Sessions).ToList();
        Rows.Clear();

        var rows = _dimension == DimensionKind.Day
            ? sessions.Select(BuildRow)
            : sessions.GroupBy(s => s.Stage).Select(g => BuildRow(g.Key, g.ToList()));
        foreach (var row in rows)
        {
            Rows.Add(row);
        }

        var dropKinds = sessions.SelectMany(s => s.Drops.Keys).Distinct().Count();
        Stats =
        [
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Records"), sessions.Count.ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sessions"), sessions.Sum(s => s.Times).ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Sanity"), sessions.Sum(s => s.Times * s.SanityCost).ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.Medicines"), sessions.Sum(s => s.MedicinesUsed + s.ExpiringMedicinesUsed).ToString()),
            new(LocalizationHelper.GetString("TaskQueue.DailyReport.Stat.DropKinds"), dropKinds.ToString()),
        ];
        NotifyOfPropertyChange(nameof(Stats));
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
        var culture = CultureInfo.CurrentUICulture;
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
        var timeText = session.StartTime.ToLocalTime().ToString("HH:mm", CultureInfo.CurrentUICulture)
            + " – " + (session.EndTime?.ToLocalTime().ToString("HH:mm", CultureInfo.CurrentUICulture) ?? "?");
        return new DailyReportRowViewModel(
            session.Stage,
            timeText,
            BuildMetaText(session.Times, session.Times * session.SanityCost, session.MedicinesUsed + session.ExpiringMedicinesUsed, session.StonesUsed),
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
            BuildMetaText(times, sessions.Sum(s => s.Times * s.SanityCost), sessions.Sum(s => s.MedicinesUsed + s.ExpiringMedicinesUsed), sessions.Sum(s => s.StonesUsed)),
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
