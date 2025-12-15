// <copyright file="OverlayWindow.xaml.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Threading;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Stylet;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.UI.Accessibility;
using Windows.Win32.UI.WindowsAndMessaging;

#nullable enable

namespace MaaWpfGui.Views.UI;

/// <summary>
/// Represents a transparent overlay window that attaches to and tracks the position of a target window, providing
/// synchronized display and interaction capabilities.
/// </summary>
/// <remarks>OverlayWindow is designed to follow a specified target window, updating its position and visibility
/// in response to changes in the target. It is typically used to display information or controls over another
/// application's window without interfering with user input. The overlay supports dynamic attachment, automatic
/// resizing, and can be shown or hidden based on application state. Thread safety is not guaranteed; all interactions
/// should occur on the UI thread.</remarks>
public partial class OverlayWindow : Window
{
    public event EventHandler? LoadedCompleted;

    public OverlayWindow()
    {
        InitializeComponent();
        Loaded += OnLoaded;
        Closed += OnClosed;
    }

    public void SetTargetHwnd(IntPtr hwnd)
    {
        try
        {
            if (hwnd == IntPtr.Zero)
            {
                return;
            }

            // 如果窗口尚未加载完成，等待加载
            if (_overlayHwnd == IntPtr.Zero)
            {
                // 记录目标窗口,OnLoaded 会处理它
                _targetHwnd = hwnd;
                return;
            }

            bool restoreCollapsed = false;

            // If overlay is currently collapsed (not visible), temporarily make it visible but transparent
            if (Visibility == Visibility.Collapsed)
            {
                restoreCollapsed = true;
                try
                {
                    Opacity = 0;
                    Visibility = Visibility.Visible;
                }
                catch
                {
                }
            }

            _targetHwnd = hwnd;
            StopWinEventHook();
            StartWinEventHookForTarget(_targetHwnd);
            UpdatePosition();

            if (restoreCollapsed)
            {
                var idle = Instances.SettingsViewModel?.Idle ?? true;
                if (idle)
                {
                    Visibility = Visibility.Collapsed;
                }
                else
                {
                    Opacity = 1;
                }
            }
        }
        catch
        {
        }
    }

    private void OnLoaded(object? sender, RoutedEventArgs e)
    {
        var hwnd = new WindowInteropHelper(this).Handle;
        _overlayHwnd = hwnd;
        var exStyle = (int)PInvoke.GetWindowLong((HWND)hwnd, WINDOW_LONG_PTR_INDEX.GWL_EXSTYLE);

        // 鼠标穿透 + 分层窗口
        _ = PInvoke.SetWindowLong((HWND)hwnd, WINDOW_LONG_PTR_INDEX.GWL_EXSTYLE,
            exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);

        // 如果在 OnLoaded 触发时已有目标窗口（例如从配置恢复），立即设置钩子和位置
        if (_targetHwnd != IntPtr.Zero)
        {
            StopWinEventHook();
            StartWinEventHookForTarget(_targetHwnd);
            UpdatePosition();
        }

        // 订阅 Idle 状态变化以控制 Overlay 可见性
        var settings = Instances.SettingsViewModel;
        if (settings is INotifyPropertyChanged inpc)
        {
            inpc.PropertyChanged += Settings_PropertyChanged;
        }

        // 初始可见性
        UpdateIdleVisibility(Instances.SettingsViewModel?.Idle ?? true);

        // 自动滚动到最新内容：订阅 ItemsControl 的 Items 集合变化
        try
        {
            if (FindName("LogItemsControl") is ItemsControl itemsCtrl &&
                FindName("LogScrollViewer") is ScrollViewer scroll &&
                itemsCtrl.Items is INotifyCollectionChanged coll)
            {
                coll.CollectionChanged += (s2, ev2) =>
                {
                    if (ev2.Action == NotifyCollectionChangedAction.Add)
                    {
                        Dispatcher.InvokeAsync(() => scroll.ScrollToVerticalOffset(scroll.ExtentHeight));
                    }
                };
            }
        }
        catch
        {
        }

        // 通知已完成加载，可以恢复配置了
        LoadedCompleted?.Invoke(this, EventArgs.Empty);
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        if (Instances.SettingsViewModel is INotifyPropertyChanged inpc)
        {
            inpc.PropertyChanged -= Settings_PropertyChanged;
        }
    }

    private void Settings_PropertyChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (e.PropertyName == nameof(SettingsViewModel.Idle) || string.IsNullOrEmpty(e.PropertyName))
        {
            // 更新可见性：Idle 为 false 时显示
            var idle = Instances.SettingsViewModel?.Idle ?? true;
            Dispatcher.Invoke(() => UpdateIdleVisibility(idle));
        }
    }

    private void UpdateIdleVisibility(bool idle)
    {
        // 只有当 Idle 为 false 时才显示 overlay
        Visibility = idle ? Visibility.Collapsed : Visibility.Visible;
    }

    #region Win32

    // Keep Win32 constants for clarity; underscore naming matches Win32 defines
#pragma warning disable SA1310 // Field names intentionally contain underscores for Win32 constants
    private const int GWL_EXSTYLE = -20;
    private const int WS_EX_TRANSPARENT = 0x20;
    private const int WS_EX_LAYERED = 0x80000;
    private const int WS_EX_NOACTIVATE = 0x08000000;
#pragma warning restore SA1310

    // Use CsWin32 generated PInvoke wrappers instead of manual DllImport declarations.
    #endregion

    /// <summary>
    /// 目标窗口句柄（由用户选择）
    /// </summary>
    private IntPtr _targetHwnd = IntPtr.Zero;
    private IntPtr _overlayHwnd = IntPtr.Zero;

    // WinEventHook 用于即时订阅目标窗口的位置/大小变动
    private IntPtr _winEventHook = IntPtr.Zero;
    private IntPtr _winEventHookShowHide = IntPtr.Zero;
    private WINEVENTPROC? _winEventDelegate;

    private void StartWinEventHookForTarget(IntPtr hwnd)
    {
        try
        {
            if (hwnd == IntPtr.Zero)
            {
                return;
            }

            // 如果已经有钩子，先清理旧的
            if (_winEventHook != IntPtr.Zero)
            {
                StopWinEventHook();
            }

            // 获取目标窗口所属进程 id
            _ = PInvoke.GetWindowThreadProcessId((HWND)hwnd, out var pid);

            _winEventDelegate = WinEventProc;

            const uint WINEVENT_OUTOFCONTEXT = 0x0000;
            const uint WINEVENT_SKIPOWNPROCESS = 0x0002;

            const uint EVENT_OBJECT_LOCATIONCHANGE = 0x800B;
            const uint EVENT_OBJECT_SHOW = 0x8002;
            const uint EVENT_OBJECT_HIDE = 0x8003;

            // 订阅目标进程的 location change
            _winEventHook = (IntPtr)PInvoke.SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, HINSTANCE.Null, _winEventDelegate, (uint)pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

            // 单独订阅 show/hide 事件以处理窗口出现/隐藏
            _winEventHookShowHide = (IntPtr)PInvoke.SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, HINSTANCE.Null, _winEventDelegate, (uint)pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
        }
        catch
        {
            // ignored
        }
    }

    private void StopWinEventHook()
    {
        try
        {
            if (_winEventHook != IntPtr.Zero)
            {
                PInvoke.UnhookWinEvent((HWINEVENTHOOK)_winEventHook);
                _winEventHook = IntPtr.Zero;
                _winEventDelegate = null;
            }

            if (_winEventHookShowHide != IntPtr.Zero)
            {
                PInvoke.UnhookWinEvent((HWINEVENTHOOK)_winEventHookShowHide);
                _winEventHookShowHide = IntPtr.Zero;
            }
        }
        catch
        {
            // ignored
        }
    }

    private void WinEventProc(HWINEVENTHOOK hWinEventHook, uint eventType, HWND hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
    {
        // 仅处理顶层窗口位置变化
        if (hwnd == HWND.Null)
        {
            return;
        }

        if (_targetHwnd == IntPtr.Zero)
        {
            return;
        }

        if (hwnd != (HWND)_targetHwnd)
        {
            return;
        }

        Execute.OnUIThreadAsync(UpdatePosition);
    }

    private void UpdatePosition()
    {
        if (_targetHwnd == IntPtr.Zero)
        {
            return;
        }

        if (!PInvoke.GetWindowRect((HWND)_targetHwnd, out var rect))
        {
            return;
        }

        if (_overlayHwnd != IntPtr.Zero)
        {
            int newLeft = rect.left;
            int newTop = rect.top;
            int newWidth = Math.Max(0, rect.right - rect.left);
            int newHeight = Math.Max(0, rect.bottom - rect.top);

            PInvoke.SetWindowPos((HWND)_overlayHwnd, (HWND)IntPtr.Zero, newLeft, newTop, newWidth, newHeight, SET_WINDOW_POS_FLAGS.SWP_NOZORDER | SET_WINDOW_POS_FLAGS.SWP_NOACTIVATE);
        }

        Execute.OnUIThreadAsync(() =>
        {
            try
            {
                if (FindName("OuterBorder") is not Border border)
                {
                    return;
                }

                var source = PresentationSource.FromVisual(this);
                if (source?.CompositionTarget != null)
                {
                    var transform = source.CompositionTarget.TransformFromDevice;
                    var topLeft = transform.Transform(new Point(rect.left, rect.top));
                    var bottomRight = transform.Transform(new Point(rect.right, rect.bottom));
                    var newWidthWpf = Math.Max(0, bottomRight.X - topLeft.X);
                    double horizontalMargin = border.Margin.Left + border.Margin.Right;
                    var maxW = Math.Max(0, newWidthWpf - horizontalMargin);
                    border.MaxWidth = maxW;
                }
            }
            catch
            {
            }
        });
    }

    /// <summary>
    /// Show the overlay but ensure it is positioned according to the target window first.
    /// The window will be shown with Opacity=0 until positioned to avoid visible flicker.
    /// </summary>
    /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
    public async Task InitializeAndShowAsync()
    {
        try
        {
            // Make invisible while positioning
            Opacity = 0;
            Show();

            await Task.Delay(20).ConfigureAwait(true);
            int tries = 0;
            while (_targetHwnd == IntPtr.Zero && tries < 40)
            {
                await Task.Delay(50).ConfigureAwait(true);
                tries++;
            }

            UpdatePosition();
        }
        finally
        {
            Opacity = 1;
        }
    }
}
