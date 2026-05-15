// <copyright file="SharedCardWidthBehavior.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace MaaWpfGui.Extensions.UIBehaviors;

/// <summary>
/// 为按行虚拟化的卡片列表维护统一的共享宽度，并根据父容器可用宽度动态约束单卡上限。
/// </summary>
public static class SharedCardWidthBehavior
{
    #region ColumnCount

    /// <summary>
    /// 指定共享宽度作用域当前使用的列数，用于按父容器宽度推导单卡的最大允许宽度。
    /// </summary>
    public static readonly DependencyProperty ColumnCountProperty =
        DependencyProperty.RegisterAttached(
            "ColumnCount",
            typeof(int),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(1, OnScopeLayoutParameterChanged));

    /// <summary>
    /// 设置共享宽度作用域的列数。
    /// </summary>
    /// <param name="obj">承载共享宽度作用域的元素。</param>
    /// <param name="value">当前布局的列数。</param>
    public static void SetColumnCount(DependencyObject obj, int value) => obj.SetValue(ColumnCountProperty, value);

    /// <summary>
    /// 获取共享宽度作用域的列数。
    /// </summary>
    /// <param name="obj">承载共享宽度作用域的元素。</param>
    /// <returns>当前布局的列数。</returns>
    public static int GetColumnCount(DependencyObject obj) => (int)obj.GetValue(ColumnCountProperty);

    #endregion

    #region WidthPadding

    /// <summary>
    /// 指定单卡宽度计算时需要预留的水平占位，用来抵消外边距、边框等额外宽度。
    /// </summary>
    public static readonly DependencyProperty WidthPaddingProperty =
        DependencyProperty.RegisterAttached(
            "WidthPadding",
            typeof(double),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(10d, OnScopeLayoutParameterChanged));

    /// <summary>
    /// 设置单卡宽度计算时的水平预留值。
    /// </summary>
    /// <param name="obj">承载共享宽度作用域的元素。</param>
    /// <param name="value">需要从每列可用宽度中扣除的水平占位。</param>
    public static void SetWidthPadding(DependencyObject obj, double value) => obj.SetValue(WidthPaddingProperty, value);

    /// <summary>
    /// 获取单卡宽度计算时的水平预留值。
    /// </summary>
    /// <param name="obj">承载共享宽度作用域的元素。</param>
    /// <returns>每列需要扣除的水平占位。</returns>
    public static double GetWidthPadding(DependencyObject obj) => (double)obj.GetValue(WidthPaddingProperty);

    #endregion

    #region EnableScope

    /// <summary>
    /// 标记某个 ItemsControl 是否作为共享宽度作用域的根节点。
    /// </summary>
    public static readonly DependencyProperty EnableScopeProperty =
        DependencyProperty.RegisterAttached(
            "EnableScope",
            typeof(bool),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(false, OnEnableScopeChanged));

    /// <summary>
    /// 设置元素是否启用共享宽度作用域。
    /// </summary>
    /// <param name="obj">候选作用域根节点。</param>
    /// <param name="value">为 true 时启用共享宽度管理。</param>
    public static void SetEnableScope(DependencyObject obj, bool value) => obj.SetValue(EnableScopeProperty, value);

    /// <summary>
    /// 获取元素是否启用了共享宽度作用域。
    /// </summary>
    /// <param name="obj">候选作用域根节点。</param>
    /// <returns>为 true 表示该元素负责维护整组卡片的共享宽度。</returns>
    public static bool GetEnableScope(DependencyObject obj) => (bool)obj.GetValue(EnableScopeProperty);

    #endregion

    #region TrackWidth

    /// <summary>
    /// 标记某个卡片元素是否参与共享宽度测量。
    /// </summary>
    public static readonly DependencyProperty TrackWidthProperty =
        DependencyProperty.RegisterAttached(
            "TrackWidth",
            typeof(bool),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(false, OnTrackWidthChanged));

    /// <summary>
    /// 设置元素是否参与共享宽度跟踪。
    /// </summary>
    /// <param name="obj">需要参与测量的卡片元素。</param>
    /// <param name="value">为 true 时会把该元素的实际宽度纳入共享宽度计算。</param>
    public static void SetTrackWidth(DependencyObject obj, bool value) => obj.SetValue(TrackWidthProperty, value);

    /// <summary>
    /// 获取元素是否参与共享宽度跟踪。
    /// </summary>
    /// <param name="obj">需要参与测量的卡片元素。</param>
    /// <returns>为 true 表示该元素会推动共享宽度增大。</returns>
    public static bool GetTrackWidth(DependencyObject obj) => (bool)obj.GetValue(TrackWidthProperty);

    #endregion

    #region SharedMinWidth

    /// <summary>
    /// 保存当前作用域内所有卡片共享的最小宽度，供每个卡片绑定统一值。
    /// </summary>
    public static readonly DependencyProperty SharedMinWidthProperty =
        DependencyProperty.RegisterAttached(
            "SharedMinWidth",
            typeof(double),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(0d));

    /// <summary>
    /// 设置当前作用域的共享最小宽度。
    /// </summary>
    /// <param name="obj">共享宽度作用域根节点。</param>
    /// <param name="value">所有卡片共同使用的最小宽度。</param>
    public static void SetSharedMinWidth(DependencyObject obj, double value) => obj.SetValue(SharedMinWidthProperty, value);

    /// <summary>
    /// 获取当前作用域的共享最小宽度。
    /// </summary>
    /// <param name="obj">共享宽度作用域根节点。</param>
    /// <returns>当前缓存的统一卡片宽度。</returns>
    public static double GetSharedMinWidth(DependencyObject obj) => (double)obj.GetValue(SharedMinWidthProperty);

    #endregion

    #region InitialWidth

    /// <summary>
    /// 指定作用域初始化或整体重算时使用的基准宽度，避免首次布局时过窄。
    /// </summary>
    public static readonly DependencyProperty InitialWidthProperty =
        DependencyProperty.RegisterAttached(
            "InitialWidth",
            typeof(double),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(0d));

    /// <summary>
    /// 设置共享宽度的初始基准值。
    /// </summary>
    /// <param name="obj">共享宽度作用域根节点。</param>
    /// <param name="value">整体重算前先采用的基准宽度。</param>
    public static void SetInitialWidth(DependencyObject obj, double value) => obj.SetValue(InitialWidthProperty, value);

    /// <summary>
    /// 获取共享宽度的初始基准值。
    /// </summary>
    /// <param name="obj">共享宽度作用域根节点。</param>
    /// <returns>初始化或重算时使用的基准宽度。</returns>
    public static double GetInitialWidth(DependencyObject obj) => (double)obj.GetValue(InitialWidthProperty);

    #endregion

    #region WidthLimit

    /// <summary>
    /// 保存当前作用域内单个卡片允许使用的硬上限宽度。
    /// </summary>
    public static readonly DependencyProperty WidthLimitProperty =
        DependencyProperty.RegisterAttached(
            "WidthLimit",
            typeof(double),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(double.PositiveInfinity, OnWidthLimitChanged));

    /// <summary>
    /// 设置当前作用域内单卡的宽度上限。
    /// </summary>
    /// <param name="obj">共享宽度作用域根节点。</param>
    /// <param name="value">单卡允许使用的最大宽度。</param>
    public static void SetWidthLimit(DependencyObject obj, double value) => obj.SetValue(WidthLimitProperty, value);

    /// <summary>
    /// 获取当前作用域内单卡的宽度上限。
    /// </summary>
    /// <param name="obj">共享宽度作用域根节点。</param>
    /// <returns>单卡允许使用的最大宽度。</returns>
    public static double GetWidthLimit(DependencyObject obj) => (double)obj.GetValue(WidthLimitProperty);

    #endregion

    // 监听 ItemsSource 变更，确保整表数据切换后重新扫描一次共享宽度。
    private static readonly DependencyPropertyDescriptor ItemsSourceDescriptor =
        DependencyPropertyDescriptor.FromProperty(ItemsControl.ItemsSourceProperty, typeof(ItemsControl));

    // 标记当前作用域是否已经排队等待重算，避免同一帧重复投递刷新请求。
    private static readonly DependencyProperty IsRecalculationQueuedProperty =
        DependencyProperty.RegisterAttached(
            "IsRecalculationQueued",
            typeof(bool),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(false));

    // 标记当前作用域是否正在执行整体重算，避免测量过程中的回调再次干扰共享宽度。
    private static readonly DependencyProperty IsRecalculatingProperty =
        DependencyProperty.RegisterAttached(
            "IsRecalculating",
            typeof(bool),
            typeof(SharedCardWidthBehavior),
            new PropertyMetadata(false));

    private static void OnScopeLayoutParameterChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is ItemsControl itemsControl && GetEnableScope(itemsControl))
        {
            // 列数或间距变化后，需要重新按父容器宽度计算单卡上限。
            QueueScopeRecalculation(itemsControl);
        }
    }

    private static void OnWidthLimitChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is ItemsControl itemsControl)
        {
            CoerceSharedMinWidth(itemsControl);
        }
    }

    private static void OnEnableScopeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not ItemsControl itemsControl)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            itemsControl.Loaded += OnScopeLoaded;
            itemsControl.SizeChanged += OnScopeSizeChanged;
            ItemsSourceDescriptor?.AddValueChanged(itemsControl, OnScopeItemsSourceChanged);
            QueueScopeRecalculation(itemsControl);
        }
        else
        {
            itemsControl.Loaded -= OnScopeLoaded;
            itemsControl.SizeChanged -= OnScopeSizeChanged;
            ItemsSourceDescriptor?.RemoveValueChanged(itemsControl, OnScopeItemsSourceChanged);
            ResetScope(itemsControl);
        }
    }

    private static void OnTrackWidthChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not FrameworkElement element)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            element.Loaded += OnTrackedElementLoaded;
            element.SizeChanged += OnTrackedElementSizeChanged;
            UpdateSharedMinWidth(element);
        }
        else
        {
            element.Loaded -= OnTrackedElementLoaded;
            element.SizeChanged -= OnTrackedElementSizeChanged;
        }
    }

    private static void OnScopeLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is ItemsControl itemsControl)
        {
            QueueScopeRecalculation(itemsControl);
        }
    }

    private static void OnScopeItemsSourceChanged(object? sender, EventArgs e)
    {
        if (sender is ItemsControl itemsControl)
        {
            QueueScopeRecalculation(itemsControl);
        }
    }

    private static void OnScopeSizeChanged(object sender, SizeChangedEventArgs e)
    {
        if (!e.WidthChanged)
        {
            return;
        }

        if (sender is ItemsControl itemsControl)
        {
            // 共享宽度只在父容器宽度变化时允许收缩，卡片自身变化只允许把它撑大。
            QueueScopeRecalculation(itemsControl);
        }
    }

    private static void OnTrackedElementLoaded(object sender, RoutedEventArgs e)
    {
        if (sender is FrameworkElement element)
        {
            UpdateSharedMinWidth(element);
        }
    }

    private static void OnTrackedElementSizeChanged(object sender, SizeChangedEventArgs e)
    {
        if (sender is FrameworkElement element)
        {
            UpdateSharedMinWidth(element);
        }
    }

    private static void UpdateSharedMinWidth(FrameworkElement element)
    {
        var trackedWidth = GetTrackedWidth(element);
        if (trackedWidth <= 0)
        {
            return;
        }

        var scope = FindScope(element);
        if (scope == null)
        {
            return;
        }

        if ((bool)scope.GetValue(IsRecalculatingProperty))
        {
            return;
        }

        // 运行时上推共享宽度时，仍然必须先服从当前的单卡硬上限。
        trackedWidth = ClampWidth(scope, trackedWidth);
        if (trackedWidth <= 0)
        {
            return;
        }

        var currentWidth = GetSharedMinWidth(scope);
        if (trackedWidth > currentWidth + 0.5)
        {
            SetSharedMinWidth(scope, trackedWidth);
        }
    }

    private static void ResetScope(ItemsControl itemsControl)
    {
        SetSharedMinWidth(itemsControl, ClampWidth(itemsControl, GetInitialWidth(itemsControl)));
    }

    private static void QueueScopeRecalculation(ItemsControl itemsControl)
    {
        if ((bool)itemsControl.GetValue(IsRecalculationQueuedProperty) || itemsControl.Dispatcher.HasShutdownStarted)
        {
            return;
        }

        itemsControl.SetValue(IsRecalculationQueuedProperty, true);

        _ = itemsControl.Dispatcher.BeginInvoke(DispatcherPriority.Render, new Action(() =>
        {
            itemsControl.SetValue(IsRecalculationQueuedProperty, false);

            if (!GetEnableScope(itemsControl))
            {
                return;
            }

            RecalculateScopeWidth(itemsControl);
        }));
    }

    private static void RecalculateScopeWidth(ItemsControl itemsControl)
    {
        itemsControl.SetValue(IsRecalculatingProperty, true);

        try
        {
            // 布局前后各刷新一次上限，确保依赖可视区的值都是最新的。
            UpdateWidthLimit(itemsControl);
            ResetScope(itemsControl);
            itemsControl.UpdateLayout();
            UpdateWidthLimit(itemsControl);

            var maxWidth = GetInitialWidth(itemsControl);
            var widthLimit = GetWidthLimit(itemsControl);
            foreach (var trackedElement in EnumerateTrackedElements(itemsControl))
            {
                var trackedWidth = GetTrackedWidth(trackedElement);
                if (trackedWidth > maxWidth + 0.5)
                {
                    maxWidth = trackedWidth;
                }
            }

            SetSharedMinWidth(itemsControl, ClampWidth(itemsControl, maxWidth));
        }
        finally
        {
            itemsControl.SetValue(IsRecalculatingProperty, false);
        }
    }

    private static ItemsControl? FindScope(DependencyObject? current)
    {
        while (current != null)
        {
            if (current is ItemsControl itemsControl && GetEnableScope(itemsControl))
            {
                return itemsControl;
            }

            current = VisualTreeHelper.GetParent(current);
        }

        return null;
    }

    private static void UpdateWidthLimit(ItemsControl itemsControl)
    {
        var availableWidth = GetAvailableWidth(itemsControl);
        var columnCount = Math.Max(1, GetColumnCount(itemsControl));
        var widthPadding = GetWidthPadding(itemsControl);

        if (availableWidth <= 0)
        {
            SetWidthLimit(itemsControl, double.PositiveInfinity);
            return;
        }

        var widthLimit = Math.Max(0, (availableWidth / columnCount) - widthPadding);
        SetWidthLimit(itemsControl, widthLimit);
    }

    private static void CoerceSharedMinWidth(ItemsControl itemsControl)
    {
        // SharedMinWidth 是缓存值，所以硬上限变化后要再做一次钳制。
        var currentWidth = GetSharedMinWidth(itemsControl);
        var clampedWidth = ClampWidth(itemsControl, currentWidth);
        if (Math.Abs(clampedWidth - currentWidth) > 0.5)
        {
            SetSharedMinWidth(itemsControl, clampedWidth);
        }
    }

    private static double GetAvailableWidth(ItemsControl itemsControl)
    {
        var scrollViewer = FindDescendant<ScrollViewer>(itemsControl);
        if (scrollViewer != null)
        {
            // 出现滚动条时，ViewportWidth 比 ActualWidth 更接近真实可用宽度。
            var viewportWidth = scrollViewer.ViewportWidth;
            if (!double.IsNaN(viewportWidth) && !double.IsInfinity(viewportWidth) && viewportWidth > 0)
            {
                return viewportWidth;
            }
        }

        return itemsControl.ActualWidth;
    }

    private static IEnumerable<FrameworkElement> EnumerateTrackedElements(DependencyObject root)
    {
        var childCount = VisualTreeHelper.GetChildrenCount(root);
        for (var index = 0; index < childCount; index++)
        {
            var child = VisualTreeHelper.GetChild(root, index);
            if (child is FrameworkElement element && GetTrackWidth(element))
            {
                yield return element;
            }

            foreach (var descendant in EnumerateTrackedElements(child))
            {
                yield return descendant;
            }
        }
    }

    private static double GetTrackedWidth(FrameworkElement element)
    {
        var width = element.ActualWidth;
        if (width <= 0)
        {
            return 0;
        }

        var maxWidth = element.MaxWidth;
        if (!double.IsNaN(maxWidth) && !double.IsInfinity(maxWidth) && maxWidth > 0)
        {
            width = Math.Min(width, maxWidth);
        }

        return width;
    }

    private static double ClampWidth(ItemsControl itemsControl, double width)
    {
        if (width <= 0)
        {
            return 0;
        }

        // 共享宽度任何时候都不允许超过当前的单卡上限。
        var widthLimit = GetWidthLimit(itemsControl);
        if (!double.IsNaN(widthLimit) && !double.IsInfinity(widthLimit) && widthLimit > 0)
        {
            width = Math.Min(width, widthLimit);
        }

        return Math.Max(0, width);
    }

    private static T? FindDescendant<T>(DependencyObject root)
        where T : DependencyObject
    {
        var childCount = VisualTreeHelper.GetChildrenCount(root);
        for (var index = 0; index < childCount; index++)
        {
            var child = VisualTreeHelper.GetChild(root, index);
            if (child is T result)
            {
                return result;
            }

            var descendant = FindDescendant<T>(child);
            if (descendant != null)
            {
                return descendant;
            }
        }

        return null;
    }
}
