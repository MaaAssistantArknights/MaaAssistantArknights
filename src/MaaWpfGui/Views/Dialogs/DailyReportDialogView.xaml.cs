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
using System.Windows.Media;
using System.Windows.Threading;
using MaaWpfGui.Helper;
using MaaWpfGui.Styles.Properties;
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
        DailyReportDialogViewModel.Instance.LoadHistory();
        DailyReportDialogViewModel.Instance.RefreshAll();

        // 浮层开合与 ComboBox 一致：点外部或窗口失活关闭，关闭浮层的那一击不作用于其他元素；
        // 锚点取按关卡列表，用于窗口失活订阅与卸载时解绑；连击判定只认当前打开的掉落格，
        // 判定窗口内双击该格保持打开，点列表其他位置照常关闭
        _dropRateDismiss = new PopupDismissController(
            DropRatePopup,
            StageList,
            repeatClickTarget: () => DropRatePopup.PlacementTarget as FrameworkElement);
    }

    // 持有引用使控制器存活不依赖 Popup 事件订阅的隐式引用
    private readonly PopupDismissController _dropRateDismiss;

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

    // 鼠标按下时所在的掉落格；抬起时仍在同一格才算点击，拖动、滚动后松开不弹浮层
    private FrameworkElement _pressedDropCell;

    // 处理挂在外层列表上：鼠标按下时列表捕获鼠标，抬起事件的源变成列表本身，掉落格自己的处理收不到；
    // 因此按坐标重新命中测试，再上行找到所在的掉落格
    private static FrameworkElement FindDropCell(UIElement list, Point position)
    {
        for (DependencyObject node = list.InputHitTest(position) as DependencyObject; node is not null;)
        {
            if (node is ContentPresenter { Content: DailyReportDropItemViewModel } presenter)
            {
                return presenter;
            }

            // 文本里的 Run 等内容元素不在可视树上，先沿逻辑树回到所属的可视元素
            node = node is Visual or System.Windows.Media.Media3D.Visual3D ? VisualTreeHelper.GetParent(node) : LogicalTreeHelper.GetParent(node);
        }

        return null;
    }

    private void OnDropPress(object sender, MouseButtonEventArgs e)
    {
        _pressedDropCell = sender is UIElement list ? FindDropCell(list, e.GetPosition(list)) : null;
    }

    // 掉落格点击：在被点的格子下方弹出该物品在本行的掉率；Popup 不在可视树内，直接把物品设为其 DataContext
    private void OnDropClick(object sender, MouseButtonEventArgs e)
    {
        var pressed = _pressedDropCell;
        _pressedDropCell = null;
        if (sender is not UIElement list || FindDropCell(list, e.GetPosition(list)) is not { DataContext: DailyReportDropItemViewModel drop } cell || cell != pressed)
        {
            return;
        }

        // 延后到本次鼠标抬起处理完再打开：外层 ListBox 在按下时捕获了鼠标、抬起时才释放，
        // 同步打开时控制器挂到浮层上的捕获会被随后的释放冲掉，浮层就收不到外部点击而无法关闭
        Dispatcher.BeginInvoke(
            () =>
            {
                drop.RankText = ViewModel.BuildRankSummary(drop);
                DropRatePopup.DataContext = null;
                DropRatePopup.DataContext = drop;
                DropRatePopup.PlacementTarget = cell;
                DropRatePopup.IsOpen = true;
            },
            DispatcherPriority.Input);
    }

    // 浮层排名摘要点击：跳到按材料视图的关卡排名，高亮当前关卡
    private void OnRankSummaryClick(object sender, MouseButtonEventArgs e)
    {
        if (DropRatePopup.DataContext is not DailyReportDropItemViewModel drop)
        {
            return;
        }

        DropRatePopup.IsOpen = false;
        ViewModel.ShowRanking(drop);
    }
}
