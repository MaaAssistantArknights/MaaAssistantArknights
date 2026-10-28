// <copyright file="DailyReportDialogView.xaml.cs" company="MaaAssistantArknights">
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

using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Markup;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;

namespace MaaWpfGui.Views.Dialogs;

/// <summary>
/// DailyReportDialogView.xaml 的交互逻辑
/// </summary>
public partial class DailyReportDialogView
{
    /// <summary>
    /// Initializes a new instance of the <see cref="DailyReportDialogView"/> class.
    /// </summary>
    public DailyReportDialogView()
    {
        InitializeComponent();
        DataContext = DailyReportDialogViewModel.Instance;
        DailyReportDialogViewModel.Instance.RefreshAll();
    }

    private static DailyReportDialogViewModel ViewModel => DailyReportDialogViewModel.Instance;

    // 打开日历时同步语言、选取粒度、可选上限、当前时段与日期格状态，期间置位以屏蔽这些赋值触发的跳转
    private bool _syncingCalendar;

    // Popup 内容不在窗口逻辑树上，DataContext 与绑定在打开时序里不可靠，打开时由代码直接同步日历状态
    private void OnCalendarPopupOpened(object sender, EventArgs e)
    {
        _syncingCalendar = true;
        try
        {
            PeriodCalendar.Language = XmlLanguage.GetLanguage(LocalizationHelper.CustomCultureInfo.IetfLanguageTag);
            PeriodCalendar.Tag = ViewModel.BuildCalendarState();
            PeriodCalendar.DisplayDateEnd = DailyReportDialogViewModel.CalendarMaxDate;
            PeriodCalendar.SelectedDate = ViewModel.CalendarDate;
            PeriodCalendar.DisplayDate = ViewModel.CalendarDate;
            PeriodCalendar.DisplayMode = ViewModel.CalendarPickMode;
        }
        finally
        {
            _syncingCalendar = false;
        }
    }

    // 日/周维度在日视图里点选日期即跳转
    private void OnCalendarSelectedDatesChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_syncingCalendar || ViewModel.CalendarPickMode != CalendarMode.Month || !ViewModel.IsCalendarOpen || PeriodCalendar.SelectedDate is not { } date)
        {
            return;
        }

        ViewModel.JumpTo(date);
    }

    // 月/年维度点选月份或年份时 Calendar 会下钻一级，借下钻事件取所选月份/年份跳转，并把日历拉回选取粒度
    private void OnCalendarDisplayModeChanged(object sender, CalendarModeChangedEventArgs e)
    {
        var pickMode = ViewModel.CalendarPickMode;
        if (_syncingCalendar || pickMode == CalendarMode.Month || !ViewModel.IsCalendarOpen)
        {
            return;
        }

        var drilledDown = (pickMode, e.OldMode, e.NewMode) is (CalendarMode.Year, CalendarMode.Year, CalendarMode.Month)
            or (CalendarMode.Decade, CalendarMode.Decade, CalendarMode.Year);
        if (drilledDown)
        {
            ViewModel.JumpTo(PeriodCalendar.DisplayDate);
        }

        if (e.NewMode != pickMode && (int)e.NewMode < (int)pickMode)
        {
            PeriodCalendar.DisplayMode = pickMode;
        }
    }

    // 掉落格点击：在被点的格子下方弹出该物品在本行的掉率；Popup 不在可视树内，直接把物品设为其 DataContext
    private void OnDropClick(object sender, MouseButtonEventArgs e)
    {
        if (e.OriginalSource is not FrameworkElement { DataContext: DailyReportDropItemViewModel drop } element)
        {
            return;
        }

        var cell = element;
        while (cell.Parent is FrameworkElement parent && parent.DataContext == drop)
        {
            cell = parent;
        }

        DropRatePopup.IsOpen = false;
        DropRatePopup.DataContext = drop;
        DropRatePopup.PlacementTarget = cell;
        DropRatePopup.IsOpen = true;
        e.Handled = true;
    }
}
