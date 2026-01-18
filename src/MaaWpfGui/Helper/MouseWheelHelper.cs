// <copyright file="MouseWheelHelper.cs" company="MaaAssistantArknights">
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

using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;

namespace MaaWpfGui.Helper;

public static class MouseWheelHelper
{
    public static void RouteMouseWheelToParent(object sender, MouseWheelEventArgs e)
    {
        if (e.Handled)
        {
            return;
        }

        e.Handled = true;
        var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta)
        {
            RoutedEvent = UIElement.MouseWheelEvent,
        };
        var parent = ((Control)sender).Parent as UIElement;
        parent?.RaiseEvent(eventArg);
    }

    /// <summary>
    /// 将鼠标滚轮事件路由到父元素，但如果事件源在可滚动的子控件内部，则不路由
    /// </summary>
    /// <param name="sender">要检查的元素</param>
    /// <param name="e">滚动事件参数</param>
    public static void RouteMouseWheelToParentExceptScrollable(object sender, MouseWheelEventArgs e)
    {
        if (e.Handled)
        {
            return;
        }

        // 如果在可滚动控件内部，不路由事件，让子控件自己处理滚动
        if (IsInsideScrollableControl(e.OriginalSource as DependencyObject, sender as DependencyObject))
        {
            return;
        }

        e.Handled = true;
        var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta)
        {
            RoutedEvent = UIElement.MouseWheelEvent,
        };
        var parent = ((Control)sender).Parent as UIElement;
        parent?.RaiseEvent(eventArg);
    }

    /// <summary>
    /// 检查元素是否在可滚动的控件内部（如 ComboBox 的 Popup、ScrollViewer 等）
    /// </summary>
    /// <param name="element">要检查的元素</param>
    /// <param name="stopAt">停止查找的元素（通常是 sender），不包括此元素本身</param>
    private static bool IsInsideScrollableControl(DependencyObject element, DependencyObject stopAt)
    {
        while (element != null && element != stopAt)
        {
            // 如果找到 Popup，说明在弹出层内部（如 ComboBox 的下拉框）
            if (element is Popup)
            {
                return true;
            }

            // 如果找到 ScrollViewer，检查是否可滚动
            if (element is ScrollViewer scrollViewer)
            {
                if (scrollViewer.ScrollableHeight > 0 || scrollViewer.ScrollableWidth > 0)
                {
                    return true;
                }
            }

            // 向上遍历可视树
            element = VisualTreeHelper.GetParent(element);
        }

        return false;
    }

    public static void DisableComboBoxScrollWhenClosed(object sender, MouseWheelEventArgs e)
    {
        if (sender is ComboBox { IsDropDownOpen: false })
        {
            e.Handled = true; // 阻止滚动
        }
    }
}
