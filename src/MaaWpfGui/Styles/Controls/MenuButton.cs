// <copyright file="MenuButton.cs" company="MaaAssistantArknights">
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
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Input;
using MaaWpfGui.Styles.Properties;

namespace MaaWpfGui.Styles.Controls;

/// <summary>
/// 带下拉的按钮：点击把 <see cref="PopupContent"/> 在按钮下方展开，交互与 ComboBox
/// 对齐——点一次打开、再点一次关闭、连击（含宏级高速连击）保持展开、点击窗口其他
/// 位置或选中内容后关闭。下拉用 Popup 而非 <see cref="ContextMenu"/> 承载：
/// ContextMenu 打开时强制建立子树鼠标捕获，点击捕获子树外会经捕获转移/广播被关闭，
/// 该路径不受 StaysOpen 接管控制，高速连击下 IsOpen 反复翻转导致上屏闪烁。
/// 点外部关闭由 <see cref="PopupDismissController"/> 以自管鼠标捕获实现；连击保持则在本类
/// 截断：捕获子树外（按钮在弹层独立窗口之外）的按下仍会照常路由到本按钮，判定窗口内的
/// 再次点击若不吞掉会经 Click 把弹层关闭，因此在预览阶段吞掉整次点击。
/// 刚关闭的判定窗口内的点击吞掉重开（保证关闭后再打开有间隔）。
/// 下拉内容的视觉（背景、边框、阴影、内边距等）由使用点在 XAML 里给出。
/// </summary>
public class MenuButton : Button
{
    // 最近一次下拉打开/关闭的时刻（Environment.TickCount64 毫秒）；null 表示从未发生。
    // 不能用 long.MinValue 之类的哨兵值：与 TickCount64 相减会溢出为负，导致判定恒真
    private long? _openedAt;

    private long? _closedAt;

    // 预览按下已吞掉点击（判定窗口内的连击）时置位，吞掉随后的 Click，避免切换关闭
    private bool _suppressNextClick;

    // 承载下拉内容的弹层（懒创建，复用）与开合判定控制器
    private Popup? _menuPopup;

    private PopupDismissController? _dismiss;

    /// <summary>
    /// Initializes a new instance of the <see cref="MenuButton"/> class.
    /// </summary>
    public MenuButton()
    {
        PreviewMouseLeftButtonDown += OnPreviewMouseLeftButtonDown;
    }

    /// <summary>
    /// Gets or sets 点击按钮时展开的下拉内容（视觉由使用点给出）。
    /// </summary>
    public object? PopupContent
    {
        get => GetValue(PopupContentProperty);
        set => SetValue(PopupContentProperty, value);
    }

    public static readonly DependencyProperty PopupContentProperty = DependencyProperty.Register(
        nameof(PopupContent), typeof(object), typeof(MenuButton), new PropertyMetadata(null));

    protected override void OnClick()
    {
        base.OnClick();
        if (PopupContent is null)
        {
            return;
        }

        if (_suppressNextClick)
        {
            _suppressNextClick = false;
            return;
        }

        // 下拉刚关闭，本次点击即关闭操作的一部分：不再重新打开
        if (_closedAt is { } closedAt
            && Environment.TickCount64 - closedAt < DropDownDismiss.SuppressIntervalMs)
        {
            return;
        }

        // 下拉着且判定窗口已过：正常关闭
        if (MenuPopup.IsOpen)
        {
            MenuPopup.IsOpen = false;
            return;
        }

        MenuPopup.Child = (UIElement)PopupContent;
        MenuPopup.IsOpen = true;
    }

    private void OnPreviewMouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        // 下拉着且刚打开：判定窗口内的连击，吞掉整次点击保持打开
        if (PopupContent is not null
            && MenuPopup.IsOpen
            && _openedAt is { } openedAt
            && Environment.TickCount64 - openedAt < DropDownDismiss.SuppressIntervalMs)
        {
            _suppressNextClick = true;
            e.Handled = true;
        }
    }

    private Popup MenuPopup
    {
        get
        {
            if (_menuPopup is null)
            {
                var popup = new Popup
                {
                    PlacementTarget = this,
                    Placement = PlacementMode.Bottom,

                    // 恒接管失活自动关闭，开合完全由 PopupDismissController 判定；
                    // 阴影等效果需要透明窗口承载，否则渲染为纯色边（阴影无法在窗口矩形外绘制）
                    StaysOpen = true,
                    AllowsTransparency = true,
                };
                popup.Opened += OnPopupOpened;
                popup.Closed += OnPopupClosed;
                _dismiss = new PopupDismissController(popup, this);
                _menuPopup = popup;
            }

            return _menuPopup;
        }
    }

    private void OnPopupOpened(object? sender, EventArgs e) => _openedAt = Environment.TickCount64;

    private void OnPopupClosed(object? sender, EventArgs e) => _closedAt = Environment.TickCount64;
}
