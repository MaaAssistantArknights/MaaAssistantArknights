// <copyright file="PopupDismissController.cs" company="MaaAssistantArknights">
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
using System.Windows.Controls.Primitives;
using System.Windows.Input;

namespace MaaWpfGui.Styles.Properties;

/// <summary>
/// 下拉弹层的外部点击关闭控制器，配合 <see cref="Popup"/>（StaysOpen 恒 true）使用，
/// 供 MenuButton、TreeComboBox、SplitButtonDropDown 共用，保持开合手感一致。
/// 交互规格与 ComboBox 对齐：点一次打开、再点一次关闭、快速连击（判定窗口内重复点击触发控件）
/// 保持打开、点击窗口其他位置或选中内容后关闭。
/// 机制：打开时把鼠标捕获挂到弹层内容子树，捕获子树外的按下会以
/// <see cref="Mouse.PreviewMouseDownOutsideCapturedElementEvent"/> 广播过来——
/// 按下位置在触发控件上且处于判定窗口内视为连击保持打开，其余（含判定窗口内点击外部）一律关闭。
/// 不用 StaysOpen=false 的失活自动关闭实现 ｢点外部关闭｣：它无法在判定窗口内区分
/// ｢连击触发控件（应保持）｣ 与 ｢点击外部（应关闭）｣——若像连击一样暂时接管（StaysOpen 置 true），
/// 判定窗口内点击外部会一并被吞掉，导致打开后立刻点外部无法关闭。
/// </summary>
internal sealed class PopupDismissController
{
    private readonly Popup _popup;

    // 触发开合的控件（下拉按钮、箭头等），判定窗口内点击它保持打开
    private readonly UIElement _anchor;

    // 关闭动作：默认直接收起 Popup；宿主控件的开关状态与 Popup 非直接绑定时由构造方提供
    private readonly Action _close;

    // 弹层内容作为捕获与广播事件宿主；Popup.Child 在打开前可能未赋值，打开时按需挂接
    private UIElement? _hookedChild;

    private Window? _hostWindow;

    private long _openedAt;

    // 捕获恢复的异步排队标记，避免 LostCapture 高频触发重复排队
    private bool _captureRestoreQueued;

    /// <summary>
    /// Gets 受控弹层，供使用方比对模板重建后弹层是否变化以决定重建本控制器。
    /// </summary>
    internal Popup Popup => _popup;

    /// <summary>
    /// Initializes a new instance of the <see cref="PopupDismissController"/> class.
    /// </summary>
    /// <param name="popup">受控弹层，需 StaysOpen 保持 true，关闭时机完全由本类接管。</param>
    /// <param name="anchor">触发控件。</param>
    /// <param name="close">关闭动作，null 表示直接置 <c>popup.IsOpen = false</c>。</param>
    public PopupDismissController(Popup popup, UIElement anchor, Action? close = null)
    {
        _popup = popup;
        _anchor = anchor;
        _close = close ?? (() => popup.IsOpen = false);
        popup.Opened += OnPopupOpened;
        popup.Closed += OnPopupClosed;
    }

    private void OnPopupOpened(object? sender, EventArgs e)
    {
        _openedAt = Environment.TickCount64;

        // 宿主窗口失活（点击其他应用等收不到鼠标广播的场景）兜底关闭；懒取且只挂一次
        if (_hostWindow is null)
        {
            _hostWindow = Window.GetWindow(_anchor);
            if (_hostWindow is not null)
            {
                _hostWindow.Deactivated += OnHostDeactivated;
            }
        }

        if (_popup.Child is { } child && !ReferenceEquals(child, _hookedChild))
        {
            _hookedChild?.RemoveHandler(Mouse.PreviewMouseDownOutsideCapturedElementEvent, (MouseButtonEventHandler)OnMouseOutside);
            _hookedChild?.RemoveHandler(UIElement.LostMouseCaptureEvent, (RoutedEventHandler)OnLostMouseCapture);
            child.AddHandler(Mouse.PreviewMouseDownOutsideCapturedElementEvent, (MouseButtonEventHandler)OnMouseOutside);
            child.AddHandler(UIElement.LostMouseCaptureEvent, (RoutedEventHandler)OnLostMouseCapture);
            _hookedChild = child;
        }

        Capture();
    }

    private void Capture() => Mouse.Capture(_popup.Child, CaptureMode.SubTree);

    private void OnMouseOutside(object sender, MouseButtonEventArgs e)
    {
        // 广播事件标记 Handled 对原始按下事件的路由没有影响：捕获子树外的按下仍会照常
        // 路由到实际命中元素，触发控件侧需自行吞击（连击保持）——见各控件的 Preview 处理
        if (IsCursorOnAnchor()
            && Environment.TickCount64 - _openedAt < DropDownDismiss.SuppressIntervalMs)
        {
            // 判定窗口内连击触发控件：保持打开（与 ComboBox 双击不关闭的手感一致），
            // 是否关闭的后续交由触发控件的预览吞击截断（箭头翻转/按钮 Click 会关闭弹层）
            return;
        }

        _close();
    }

    private void OnPopupClosed(object? sender, EventArgs e)
    {
        if (Mouse.Captured == _popup.Child)
        {
            Mouse.Capture(null);
        }
    }

    private void OnLostMouseCapture(object? sender, RoutedEventArgs e)
    {
        // 弹层内滚动条拖动等瞬时捕获转移后恢复捕获，保证其后点击外部仍能关闭
        if (_captureRestoreQueued || !_popup.IsOpen)
        {
            return;
        }

        _captureRestoreQueued = true;
        _popup.Dispatcher.BeginInvoke(() =>
        {
            _captureRestoreQueued = false;
            if (_popup.IsOpen && Mouse.Captured is null)
            {
                Capture();
            }
        });
    }

    private void OnHostDeactivated(object? sender, EventArgs e)
    {
        if (_popup.IsOpen)
        {
            _close();
        }
    }

    private bool IsCursorOnAnchor()
    {
        try
        {
            var pos = Mouse.GetPosition(_anchor);
            var size = _anchor.RenderSize;
            return pos.X >= 0 && pos.Y >= 0 && pos.X <= size.Width && pos.Y <= size.Height;
        }
        catch (InvalidOperationException)
        {
            // anchor 尚未连接到演示源（理论上打开状态下不会发生），按不在锚点上处理走关闭
            return false;
        }
    }
}
