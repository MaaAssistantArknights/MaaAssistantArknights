// <copyright file="SplitButtonDropDown.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using System.Windows.Media;
using HandyControl.Controls;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 使 <see cref="SplitButton"/> 的下拉箭头交互与 ComboBox 对齐：点一次打开、再点一次关闭，
/// 但刚打开后的判定窗口内（快速双击/宏级连击）的再次点击不视为关闭，下拉保持打开。
/// 点外部关闭由 <see cref="PopupDismissController"/> 以自管鼠标捕获实现（模板 Popup 恒接管
/// 失活自动关闭，StaysOpen 置 true）；连击保持则在本类截断：捕获子树外（箭头在上、弹层内容
/// 在独立窗口中）的按下仍会照常路由到箭头 ToggleButton 使其翻转 IsChecked 关闭下拉，
/// 因此判定窗口内点击箭头时在预览阶段吞掉整次点击。
/// 判定窗口内下拉刚关闭时箭头若再被点击，同样吞掉以避免 ｢关闭后立刻又弹开｣，
/// 与 ComboBox ｢关闭后需放慢再点才重新打开｣ 的节奏一致。
/// </summary>
public static class SplitButtonDropDown
{
    // 上次下拉打开/关闭的时刻（Environment.TickCount64 毫秒），null 表示从未发生；仅内部使用。
    // 不能用 long.MinValue 之类的哨兵值：与 TickCount64 相减会溢出为负，导致判定恒真
    private static readonly DependencyProperty OpenedAtProperty = DependencyProperty.RegisterAttached(
        "OpenedAt", typeof(long?), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    private static readonly DependencyProperty ClosedAtProperty = DependencyProperty.RegisterAttached(
        "ClosedAt", typeof(long?), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    // 下拉开合判定控制器；模板 Popup 随模板重建变化时随之重建
    private static readonly DependencyProperty DismissControllerProperty = DependencyProperty.RegisterAttached(
        "DismissController", typeof(PopupDismissController), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    public static readonly DependencyProperty EnableToggleCloseProperty = DependencyProperty.RegisterAttached(
        "EnableToggleClose", typeof(bool), typeof(SplitButtonDropDown), new PropertyMetadata(false, OnEnableToggleCloseChanged));

    /// <summary>
    /// Gets a value indicating whether 启用点一次打开、再点一次关闭，且快速双击保持打开。
    /// </summary>
    public static bool GetEnableToggleClose(DependencyObject element)
        => (bool)element.GetValue(EnableToggleCloseProperty);

    public static void SetEnableToggleClose(DependencyObject element, bool value)
        => element.SetValue(EnableToggleCloseProperty, value);

    private static void OnEnableToggleCloseChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var splitButton = (SplitButton)d;
        var descriptor = DependencyPropertyDescriptor.FromProperty(SplitButton.IsDropDownOpenProperty, typeof(SplitButton));
        if ((bool)e.NewValue)
        {
            descriptor.AddValueChanged(splitButton, OnIsDropDownOpenChanged);
            splitButton.AddHandler(UIElement.PreviewMouseLeftButtonDownEvent, new MouseButtonEventHandler(OnPreviewMouseLeftButtonDown));
        }
        else
        {
            descriptor.RemoveValueChanged(splitButton, OnIsDropDownOpenChanged);
            splitButton.RemoveHandler(UIElement.PreviewMouseLeftButtonDownEvent, new MouseButtonEventHandler(OnPreviewMouseLeftButtonDown));
        }
    }

    private static void OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (sender is not SplitButton splitButton)
        {
            return;
        }

        // 下拉着且刚打开：快速连击，吞掉整次点击保持打开（阻止箭头 ToggleButton 翻转关闭下拉）
        if (splitButton.IsDropDownOpen
            && splitButton.GetValue(OpenedAtProperty) is long openedAt
            && Environment.TickCount64 - openedAt < DropDownDismiss.SuppressIntervalMs)
        {
            e.Handled = true;
            return;
        }

        // 下拉刚被关闭（判定窗口内）：本次点击是关闭动作的延续（或关闭后的立即再点），
        // 吞掉以阻止箭头 ToggleButton 立即翻转重开——否则 ｢关闭后立刻又弹开｣ 没有间隔，
        // 与 ComboBox ｢关闭后需放慢再点才重新打开｣ 的节奏不一致
        if (!splitButton.IsDropDownOpen
            && splitButton.GetValue(ClosedAtProperty) is long closedAt
            && Environment.TickCount64 - closedAt < DropDownDismiss.SuppressIntervalMs)
        {
            e.Handled = true;
        }
    }

    private static void OnIsDropDownOpenChanged(object? sender, EventArgs e)
    {
        if (sender is not SplitButton splitButton)
        {
            return;
        }

        if (!splitButton.IsDropDownOpen)
        {
            splitButton.SetValue(ClosedAtProperty, Environment.TickCount64);
            return;
        }

        splitButton.SetValue(OpenedAtProperty, Environment.TickCount64);

        if (FindDropDownPopup(splitButton) is not { } popup)
        {
            return;
        }

        // 恒接管失活自动关闭（模板默认 StaysOpen=False），点外部关闭交给判定控制器；
        // 模板 Popup 随模板重建变化时控制器随之重建
        popup.StaysOpen = true;
        if (splitButton.GetValue(DismissControllerProperty) is not PopupDismissController controller
            || !ReferenceEquals(controller.Popup, popup))
        {
            // 关闭走 SplitButton 的开关状态（模板绑定驱动 Popup 收起），不用 popup.IsOpen 直接触发
            controller = new PopupDismissController(
                popup,
                splitButton,
                () => splitButton.SetCurrentValue(SplitButton.IsDropDownOpenProperty, false));
            splitButton.SetValue(DismissControllerProperty, controller);
        }
    }

    // 模板未暴露 Popup 的名字，按类型在视觉树中查找下拉 Popup
    private static Popup? FindDropDownPopup(SplitButton splitButton)
    {
        var count = VisualTreeHelper.GetChildrenCount(splitButton);
        for (int i = 0; i < count; i++)
        {
            if (VisualTreeHelper.GetChild(splitButton, i) is not DependencyObject child)
            {
                continue;
            }

            if (child is Popup popup)
            {
                return popup;
            }

            if (FindPopupRecursive(child) is { } nested)
            {
                return nested;
            }
        }

        return null;
    }

    private static Popup? FindPopupRecursive(DependencyObject parent)
    {
        var count = VisualTreeHelper.GetChildrenCount(parent);
        for (int i = 0; i < count; i++)
        {
            var child = VisualTreeHelper.GetChild(parent, i);
            if (child is Popup popup)
            {
                return popup;
            }

            if (FindPopupRecursive(child) is { } nested)
            {
                return nested;
            }
        }

        return null;
    }
}
