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

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 使 <see cref="SplitButton"/> 的下拉交互与 ComboBox 对齐：点一次打开、再点一次关闭，
/// 但刚打开后的判定窗口内（快速双击/宏级连击）的再次点击不视为关闭，下拉保持打开；
/// 因点箭头关闭后的判定窗口内再次点击箭头（如双击的第二击）是同一关闭手势的延续，
/// 也不重新打开。判定范围只覆盖模板箭头（PART_Arrow）：主按钮不属于 ｢再点一次关闭｣，
/// 点击主按钮视同点击外部——首击只关下拉不执行命令，其后立即可正常执行。
/// 点外部关闭由 <see cref="PopupDismissController"/> 以自管鼠标捕获实现
/// （模板 Popup 恒接管失活自动关闭，StaysOpen 置 true，含关闭时点击吞除）；
/// 连击保持则在本类截断：捕获子树外（箭头在上、弹层内容在独立窗口中）的按下
/// 仍会照常路由到箭头 ToggleButton 使其翻转 IsChecked 关闭下拉，
/// 因此判定窗口内点击箭头时在预览阶段吞掉整次点击。
/// </summary>
public static class SplitButtonDropDown
{
    // 上次下拉打开的时刻与 ｢因点箭头关闭｣ 的时刻（Environment.TickCount64 毫秒），
    // null 表示从未发生；仅内部使用。
    // 不能用 long.MinValue 之类的哨兵值：与 TickCount64 相减会溢出为负，导致判定恒真
    private static readonly DependencyProperty OpenedAtProperty = DependencyProperty.RegisterAttached(
        "OpenedAt", typeof(long?), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    private static readonly DependencyProperty AnchorClickClosedAtProperty = DependencyProperty.RegisterAttached(
        "AnchorClickClosedAt", typeof(long?), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    // 下拉开合判定控制器；模板 Popup 或箭头随模板重建变化时随之重建
    private static readonly DependencyProperty DismissControllerProperty = DependencyProperty.RegisterAttached(
        "DismissController", typeof(PopupDismissController), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    // 最近一次打开时定位到的模板箭头元素，供关闭后判定窗口的光标落点检查
    private static readonly DependencyProperty ArrowElementProperty = DependencyProperty.RegisterAttached(
        "ArrowElement", typeof(FrameworkElement), typeof(SplitButtonDropDown), new PropertyMetadata(null));

    /// <summary>
    /// The enable toggle close property.
    /// </summary>
    public static readonly DependencyProperty EnableToggleCloseProperty = DependencyProperty.RegisterAttached(
        "EnableToggleClose", typeof(bool), typeof(SplitButtonDropDown), new PropertyMetadata(false, OnEnableToggleCloseChanged));

    /// <summary>
    /// Gets a value indicating whether 启用点一次打开、再点一次关闭，且快速双击保持打开。
    /// </summary>
    /// <param name="element">The <see cref="SplitButton"/> instance.</param>
    /// <returns>The property value.</returns>
    public static bool GetEnableToggleClose(DependencyObject element)
        => (bool)element.GetValue(EnableToggleCloseProperty);

    /// <summary>
    /// Sets a value indicating whether 启用点一次打开、再点一次关闭，且快速双击保持打开。
    /// </summary>
    /// <param name="element">The <see cref="SplitButton"/> instance.</param>
    /// <param name="value">The new property value.</param>
    public static void SetEnableToggleClose(DependencyObject element, bool value)
        => element.SetValue(EnableToggleCloseProperty, value);

    private static void OnEnableToggleCloseChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
    {
        var splitButton = (SplitButton)d;
        var descriptor = DependencyPropertyDescriptor.FromProperty(SplitButton.IsDropDownOpenProperty, typeof(SplitButton));
        if ((bool)e.NewValue)
        {
            descriptor.AddValueChanged(splitButton, OnIsDropDownOpenChanged);

            // handledEventsToo：SplitButton 的 HitMode=Hover/Focus 模式下其自身的类处理会先把
            // 预览按下标记已处理，仍需参与连击判定，保证各模式行为一致
            splitButton.AddHandler(
                UIElement.PreviewMouseLeftButtonDownEvent,
                new MouseButtonEventHandler(OnPreviewMouseLeftButtonDown),
                handledEventsToo: true);
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

        // 因点箭头关闭后的判定窗口内再次点击箭头（如双击的第二击）是同一关闭手势的延续：
        // 吞掉以阻止箭头 ToggleButton 翻转重新打开。窗口外的点击、以及落在主按钮上的点击
        // 不吞——主命令应照常执行，不受判定窗口影响
        if (!splitButton.IsDropDownOpen
            && splitButton.GetValue(AnchorClickClosedAtProperty) is long closedAt
            && Environment.TickCount64 - closedAt < DropDownDismiss.SuppressIntervalMs
            && IsCursorOnArrow(splitButton))
        {
            e.Handled = true;
            return;
        }

        // 下拉着且刚打开：快速连击，吞掉整次点击保持打开（阻止箭头 ToggleButton 翻转关闭下拉）。
        // 点主按钮不会走到这里：控制器在广播阶段已按 ｢点外部｣ 关闭下拉并吞掉整次点击
        if (splitButton.IsDropDownOpen
            && splitButton.GetValue(OpenedAtProperty) is long openedAt
            && Environment.TickCount64 - openedAt < DropDownDismiss.SuppressIntervalMs)
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
            return;
        }

        splitButton.SetValue(OpenedAtProperty, Environment.TickCount64);

        // 模板未暴露 Popup 的名字，按类型在视觉树中查找；箭头经模板名定位。
        // 找不到箭头（自定义模板未遵循 PART_Arrow）时不接管，避免把整个控件当锚点误吞主命令
        if (FindPopupRecursive(splitButton) is not { } popup
            || FindArrow(splitButton) is not { } arrow)
        {
            return;
        }

        splitButton.SetValue(ArrowElementProperty, arrow);

        // 恒接管失活自动关闭（模板默认 StaysOpen=False），点外部关闭交给判定控制器；
        // 模板 Popup 或箭头随模板重建变化时控制器随之重建。
        // 关闭走 SplitButton 的开关状态（模板绑定驱动 Popup 收起），不用 popup.IsOpen 直接触发；
        // 锚点是箭头而非整个 SplitButton——主按钮点击视同点外部（首击只关下拉，不记关闭时刻）
        popup.StaysOpen = true;
        if (splitButton.GetValue(DismissControllerProperty) is not PopupDismissController controller
            || !ReferenceEquals(controller.Popup, popup)
            || !ReferenceEquals(controller.Anchor, arrow))
        {
            controller = new PopupDismissController(
                popup,
                arrow,
                () => splitButton.SetCurrentValue(SplitButton.IsDropDownOpenProperty, false),
                onAnchorClickClose: () => splitButton.SetValue(
                    AnchorClickClosedAtProperty, Environment.TickCount64));
            splitButton.SetValue(DismissControllerProperty, controller);
        }
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

    // 模板箭头：开合判定与关闭时刻记录的作用区域，点击它之外（含主按钮）都视同点击外部
    private static FrameworkElement? FindArrow(SplitButton splitButton)
        => splitButton.Template?.FindName("PART_Arrow", splitButton) as FrameworkElement;

    private static bool IsCursorOnArrow(SplitButton splitButton)
    {
        if (splitButton.GetValue(ArrowElementProperty) is not FrameworkElement arrow)
        {
            return false;
        }

        try
        {
            var pos = Mouse.GetPosition(arrow);
            var size = arrow.RenderSize;
            return pos.X >= 0 && pos.Y >= 0 && pos.X <= size.Width && pos.Y <= size.Height;
        }
        catch (InvalidOperationException)
        {
            // 箭头尚未连接到演示源，按不在箭头上处理（不吞点击）
            return false;
        }
    }
}
