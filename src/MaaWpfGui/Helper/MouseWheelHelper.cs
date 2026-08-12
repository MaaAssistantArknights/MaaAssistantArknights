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

#nullable enable

using System;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using JetBrains.Annotations;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Helper;

/// <summary>
/// 滚轮与弹层滚动相关的通用处理。
/// </summary>
public static class MouseWheelHelper
{
    private static readonly ConditionalWeakTable<Popup, PopupScrollIsolationState> PopupIsolationStates = [];

    public static void RouteMouseWheelToParent(object sender, MouseWheelEventArgs e)
    {
        if (e.Handled)
        {
            return;
        }

        e.Handled = true;
        var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta) {
            RoutedEvent = UIElement.MouseWheelEvent,
        };
        var parent = ((Control)sender).Parent as UIElement;
        parent?.RaiseEvent(eventArg);
    }

    /// <summary>
    /// 将鼠标滚轮事件路由到父元素，但如果事件源在可滚动的子控件内部，则不路由。
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
        var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta) {
            RoutedEvent = UIElement.MouseWheelEvent,
        };
        var parent = ((Control)sender).Parent as UIElement;
        parent?.RaiseEvent(eventArg);
    }

    /// <summary>
    /// 检查元素是否在可滚动的控件内部（如 ComboBox 的 Popup、ScrollViewer 等）。
    /// </summary>
    /// <param name="element">要检查的元素</param>
    /// <param name="stopAt">停止查找的元素（通常是 sender），不包括此元素本身</param>
    private static bool IsInsideScrollableControl(DependencyObject? element, DependencyObject? stopAt)
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

    #region IsolateParentScroll（Popup 打开时隔离外层页面滚动）

    /// <summary>
    /// 附加到 <see cref="Popup"/>：打开期间隔离最近的外层 <see cref="ScrollViewer"/>，避免弹层
    /// （如 ComboBox 下拉）打开/滚动时带动外层页面一起滚，干扰页面自身的滚动联动。三道防线：
    /// 1) <c>PreviewMouseWheel</c>——弹层打开期间外层不响应滚轮，滚轮转给弹层内可滚动控件；
    /// 2) <c>RequestBringIntoView</c>——拦截弹层内元素请求滚入视野，防止冒泡到外层；
    /// 3) <see cref="LockOuterScroll"/>——对前两道拦不住的滚动（主要是布局微调）事后恢复外层位置，
    ///    并置 <c>IsVerticalOffsetSyncSuspended</c> 暂停外层滚动联动的回写。
    /// </summary>
    public static readonly DependencyProperty IsolateParentScrollProperty =
        DependencyProperty.RegisterAttached(
            "IsolateParentScroll",
            typeof(bool),
            typeof(MouseWheelHelper),
            new PropertyMetadata(false, OnIsolateParentScrollChanged));

    public static bool GetIsolateParentScroll(DependencyObject element) =>
        (bool)element.GetValue(IsolateParentScrollProperty);

    public static void SetIsolateParentScroll(DependencyObject element, bool value) =>
        element.SetValue(IsolateParentScrollProperty, value);

    private static void OnIsolateParentScrollChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not Popup popup)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            var state = PopupIsolationStates.GetValue(popup, static p => new PopupScrollIsolationState(p));
            state.Attach();
        }
        else if (PopupIsolationStates.TryGetValue(popup, out var state))
        {
            state.Detach();
        }
    }

    /// <summary>
    /// 单个 Popup 的外层滚动隔离状态。
    /// </summary>
    private sealed class PopupScrollIsolationState
    {
        private readonly Popup _popup;
        private bool _attached;
        private bool _bringIntoViewHooked;
        private bool _outerScrollLocked;
        private bool _restoringOuterScroll;
        private double _lockedOuterScrollOffset;
        private ScrollViewer? _outerScrollViewer;
        private UIElement? _bringIntoViewTarget;

        public PopupScrollIsolationState(Popup popup)
        {
            _popup = popup;
        }

        public void Attach()
        {
            if (_attached)
            {
                return;
            }

            _popup.Opened += OnPopupOpened;
            _popup.Closed += OnPopupClosed;
            _attached = true;

            if (_popup.IsOpen)
            {
                OnPopupOpened(_popup, EventArgs.Empty);
            }
        }

        public void Detach()
        {
            if (!_attached)
            {
                return;
            }

            _popup.Opened -= OnPopupOpened;
            _popup.Closed -= OnPopupClosed;
            UnlockOuterScroll();
            UnhookBringIntoView();
            _attached = false;
        }

        private void OnPopupOpened(object? sender, EventArgs e)
        {
            HookBringIntoView();
            LockOuterScroll();
        }

        private void OnPopupClosed(object? sender, EventArgs e)
        {
            UnlockOuterScroll();
            UnhookBringIntoView();
        }

        private void HookBringIntoView()
        {
            UnhookBringIntoView();

            // Popup.Child 在打开后才稳定；拦截其 RequestBringIntoView，防止冒泡到页面 ScrollViewer
            if (_popup.Child is not UIElement child)
            {
                return;
            }

            child.AddHandler(
                FrameworkElement.RequestBringIntoViewEvent,
                new RequestBringIntoViewEventHandler(OnRequestBringIntoView),
                handledEventsToo: true);
            _bringIntoViewTarget = child;
            _bringIntoViewHooked = true;
        }

        private void UnhookBringIntoView()
        {
            if (!_bringIntoViewHooked || _bringIntoViewTarget == null)
            {
                return;
            }

            _bringIntoViewTarget.RemoveHandler(
                FrameworkElement.RequestBringIntoViewEvent,
                new RequestBringIntoViewEventHandler(OnRequestBringIntoView));
            _bringIntoViewTarget = null;
            _bringIntoViewHooked = false;
        }

        private static void OnRequestBringIntoView(object sender, RequestBringIntoViewEventArgs e)
        {
            // 阻断弹层内元素请求滚动到可视区域，避免外层页面跟着滚
            e.Handled = true;
        }

        private void LockOuterScroll()
        {
            var outer = FindParentScrollViewer(_popup);
            if (outer == null)
            {
                return;
            }

            if (_outerScrollLocked && ReferenceEquals(_outerScrollViewer, outer))
            {
                _lockedOuterScrollOffset = outer.VerticalOffset;
                return;
            }

            UnlockOuterScroll();

            _outerScrollViewer = outer;
            _lockedOuterScrollOffset = outer.VerticalOffset;

            // 下拉打开会触发外层 ScrollViewer 的微小滚动（change≈1，来自 ScrollViewer.OnLayoutUpdated
            // 的布局阶段，不是路由事件，无法用 Handled/Preview 拦截）。这里对这类拦不住的滚动采取
            // ｢事后恢复 + 期间暂停联动｣：订阅 ScrollChanged，一旦偏离打开前的位置就拉回；同时置
            // IsVerticalOffsetSyncSuspended，让外层滚动的双向绑定（ScrollViewerBinding→ScrollOffset→
            // 左侧导航 SelectedIndex）暂停回写，否则瞬时偏移会被记录成 ScrollOffset、错误改写高亮。
            _outerScrollViewer.ScrollChanged += OuterScrollViewer_ScrollChanged;
            _outerScrollViewer.PreviewMouseWheel += OuterScrollViewer_PreviewMouseWheel;
            ScrollViewerBinding.SetIsVerticalOffsetSyncSuspended(_outerScrollViewer, true);
            _outerScrollLocked = true;
        }

        private void UnlockOuterScroll()
        {
            if (!_outerScrollLocked || _outerScrollViewer == null)
            {
                _outerScrollLocked = false;
                _outerScrollViewer = null;
                return;
            }

            _outerScrollViewer.ScrollChanged -= OuterScrollViewer_ScrollChanged;
            _outerScrollViewer.PreviewMouseWheel -= OuterScrollViewer_PreviewMouseWheel;
            ScrollViewerBinding.SetIsVerticalOffsetSyncSuspended(_outerScrollViewer, false);
            _outerScrollLocked = false;
            _outerScrollViewer = null;
        }

        private void OuterScrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (!_outerScrollLocked || _restoringOuterScroll || _outerScrollViewer == null || !_popup.IsOpen)
            {
                return;
            }

            if (Math.Abs(e.VerticalChange) < 0.01 &&
                Math.Abs(_outerScrollViewer.VerticalOffset - _lockedOuterScrollOffset) < 0.01)
            {
                return;
            }

            _restoringOuterScroll = true;
            try
            {
                _outerScrollViewer.ScrollToVerticalOffset(_lockedOuterScrollOffset);
            }
            finally
            {
                _restoringOuterScroll = false;
            }
        }

        private void OuterScrollViewer_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (!_popup.IsOpen || e.Handled)
            {
                return;
            }

            // 弹层打开期间：外层页面一律不响应滚轮。
            // 若指针在弹层上，把滚轮转给弹层内第一个可滚动 ScrollViewer。
            if (IsMouseOverElement(_popup.Child as FrameworkElement))
            {
                var inner = FindDescendantScrollViewer(_popup.Child);
                if (inner != null)
                {
                    ScrollScrollViewer(inner, e.Delta);
                }
            }

            e.Handled = true;
        }
    }

    #endregion

    #region IsolateComboBoxScroll（为 ComboBox 模板内的 Popup 启用 IsolateParentScroll）

    // 为什么需要：设置页（SettingsView）的外层 ScrollViewer 用 ScrollViewerBinding.VerticalOffset
    // 把滚动位置双向绑到 ScrollOffset，并联动左侧导航 ListBox 的 SelectedIndex（滚动↔高亮分区）。
    // ComboBox 打开/滚动下拉时，会触发外层 ScrollViewer 多种瞬时滚动：滚轮冒泡、选中项 BringIntoView、
    // 以及下拉布局导致的 OnLayoutUpdated 微调。这些滚动在普通页面无害，但在设置页会被上述联动记录成
    // ScrollOffset 变化，错误改写左侧导航高亮，甚至把页面拉到顶。本属性自动为 ComboBox 模板内的
    // PART_Popup 启用 IsolateParentScroll 统一隔离；通过全局 ComboBox 默认 style 的 Setter 启用，
    // 无需逐个 ComboBox 设置，以后新增的也自动免疫。

    /// <summary>
    /// 附加到 <see cref="ComboBox"/>：模板应用后自动为其内部 <c>PART_Popup</c> 启用
    /// <see cref="IsolateParentScroll"/>，隔离下拉打开/滚动期间对设置页外层滚动联动的干扰。
    /// </summary>
    public static readonly DependencyProperty IsolateComboBoxScrollProperty =
        DependencyProperty.RegisterAttached(
            "IsolateComboBoxScroll",
            typeof(bool),
            typeof(MouseWheelHelper),
            new PropertyMetadata(false, OnIsolateComboBoxScrollChanged));

    /// <summary>
    /// Gets a value indicating whether combobox scroll isolation is enabled.
    /// </summary>
    /// <param name="element">The element.</param>
    /// <returns>True if enabled.</returns>
    [UsedImplicitly]
    public static bool GetIsolateComboBoxScroll(DependencyObject element) =>
        (bool)element.GetValue(IsolateComboBoxScrollProperty);

    /// <summary>
    /// Sets a value indicating whether combobox scroll isolation is enabled.
    /// </summary>
    /// <param name="element">The element.</param>
    /// <param name="value">The value.</param>
    public static void SetIsolateComboBoxScroll(DependencyObject element, bool value) =>
        element.SetValue(IsolateComboBoxScrollProperty, value);

    private static void OnIsolateComboBoxScrollChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not ComboBox comboBox)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            comboBox.Loaded += ComboBox_EnsurePopupIsolated;
            EnsurePopupIsolated(comboBox);
        }
        else
        {
            comboBox.Loaded -= ComboBox_EnsurePopupIsolated;
        }
    }

    private static void ComboBox_EnsurePopupIsolated(object? sender, RoutedEventArgs e)
    {
        if (sender is ComboBox cb)
        {
            EnsurePopupIsolated(cb);
        }
    }

    private static void EnsurePopupIsolated(ComboBox comboBox)
    {
        if (comboBox.Template?.FindName("PART_Popup", comboBox) is Popup popup)
        {
            SetIsolateParentScroll(popup, true);
        }
    }

    #endregion

    #region HandleMouseWheel（ScrollViewer 自行消化滚轮）

    /// <summary>
    /// 附加到 <see cref="ScrollViewer"/>：自行处理滚轮并标记 Handled，避免继续影响外层。
    /// </summary>
    public static readonly DependencyProperty HandleMouseWheelProperty =
        DependencyProperty.RegisterAttached(
            "HandleMouseWheel",
            typeof(bool),
            typeof(MouseWheelHelper),
            new PropertyMetadata(false, OnHandleMouseWheelChanged));

    public static bool GetHandleMouseWheel(DependencyObject element) =>
        (bool)element.GetValue(HandleMouseWheelProperty);

    public static void SetHandleMouseWheel(DependencyObject element, bool value) =>
        element.SetValue(HandleMouseWheelProperty, value);

    private static void OnHandleMouseWheelChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        if (d is not UIElement element)
        {
            return;
        }

        if ((bool)e.NewValue)
        {
            element.PreviewMouseWheel += HandleMouseWheel_OnPreviewMouseWheel;
        }
        else
        {
            element.PreviewMouseWheel -= HandleMouseWheel_OnPreviewMouseWheel;
        }
    }

    private static void HandleMouseWheel_OnPreviewMouseWheel(object sender, MouseWheelEventArgs e)
    {
        if (e.Handled)
        {
            return;
        }

        var scrollViewer = sender as ScrollViewer ?? FindDescendantScrollViewer(sender as DependencyObject);
        if (scrollViewer == null)
        {
            return;
        }

        ScrollScrollViewer(scrollViewer, e.Delta);
        e.Handled = true;
    }

    #endregion

    private static void ScrollScrollViewer(ScrollViewer scrollViewer, int delta)
    {
        scrollViewer.ScrollToVerticalOffset(scrollViewer.VerticalOffset - (delta / 3.0));
    }

    private static bool IsMouseOverElement(FrameworkElement? element)
    {
        if (element == null || !element.IsVisible)
        {
            return false;
        }

        try
        {
            var pos = Mouse.GetPosition(element);
            return pos.X >= 0 && pos.Y >= 0 && pos.X <= element.ActualWidth && pos.Y <= element.ActualHeight;
        }
        catch
        {
            return element.IsMouseOver;
        }
    }

    private static ScrollViewer? FindParentScrollViewer(DependencyObject? current)
    {
        // Popup 本身不在 PlacementTarget 的可视树中，优先从 PlacementTarget 向上找页面 ScrollViewer
        if (current is Popup popup)
        {
            current = popup.PlacementTarget as DependencyObject
                      ?? LogicalTreeHelper.GetParent(popup) as DependencyObject
                      ?? VisualTreeHelper.GetParent(popup);
        }

        while (current != null)
        {
            if (current is ScrollViewer scrollViewer)
            {
                return scrollViewer;
            }

            current = VisualTreeHelper.GetParent(current)
                      ?? LogicalTreeHelper.GetParent(current) as DependencyObject;
        }

        return null;
    }

    private static ScrollViewer? FindDescendantScrollViewer(DependencyObject? root)
    {
        if (root == null)
        {
            return null;
        }

        if (root is ScrollViewer scrollViewer)
        {
            return scrollViewer;
        }

        var count = VisualTreeHelper.GetChildrenCount(root);
        for (var i = 0; i < count; i++)
        {
            var result = FindDescendantScrollViewer(VisualTreeHelper.GetChild(root, i));
            if (result != null)
            {
                return result;
            }
        }

        return null;
    }
}
