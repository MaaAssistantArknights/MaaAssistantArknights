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
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
using System.Windows.Threading;
using MaaWpfGui.Helper;
using Serilog;
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
    private static readonly ILogger _logger = Log.ForContext<OverlayWindow>();

    // CsWin32 在 AnyCPU 下无法生成 SetWindowLongPtr*（会触发 PInvoke005），但这里需要设置父窗口句柄。
    // 用兼容封装在 x86/x64 下分别调用 SetWindowLongW/SetWindowLongPtrW，避免对生成器/平台目标产生依赖。
    [DllImport("user32.dll", EntryPoint = "SetWindowLongPtrW", SetLastError = true)]
    private static extern IntPtr SetWindowLongPtrW(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

    [DllImport("user32.dll", EntryPoint = "SetWindowLongW", SetLastError = true)]
    private static extern int SetWindowLongW(IntPtr hWnd, int nIndex, int dwNewLong);

    private static IntPtr SetWindowLongPtrCompat(IntPtr hWnd, int nIndex, IntPtr dwNewLong)
    {
        return IntPtr.Size == 8
            ? SetWindowLongPtrW(hWnd, nIndex, dwNewLong)
            : new IntPtr(SetWindowLongW(hWnd, nIndex, dwNewLong.ToInt32()));
    }

    // Instance delegate to keep callback alive for this instance; avoids global mapping complexity
    private readonly WINEVENTPROC _winEventProc;
    private readonly DispatcherTimer _debounceTimer;

    public OverlayWindow()
    {
        InitializeComponent();
        DataContext = Instances.OverlayViewModel;
        Loaded += OnLoaded;
        Closed += OnClosed;

        // Bind instance win event delegate to prevent it from being GC'd while hooks are active.
        _winEventProc = WinEventProc;
        _debounceTimer = new DispatcherTimer(
            TimeSpan.FromMilliseconds(500),
            DispatcherPriority.Background,
            (_, _) =>
            {
                _debounceTimer?.Stop();
                UpdatePosition();
            },
            Dispatcher);
    }

    public void SetTargetHwnd(IntPtr hwnd)
    {
        try
        {
            if (hwnd == IntPtr.Zero)
            {
                return;
            }

            if (_overlayHwnd == IntPtr.Zero)
            {
                // 记录目标窗口，OnLoaded 会处理它
                _targetHwnd = hwnd;
                return;
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
        var exStyle = PInvoke.GetWindowLong((HWND)hwnd, WINDOW_LONG_PTR_INDEX.GWL_EXSTYLE);

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

        // 自动滚动到最新内容：订阅 ItemsControl 的 Items 集合变化
        try
        {
            if (FindName("LogItemsControl") is ItemsControl itemsCtrl &&
                FindName("LogScrollViewer") is ScrollViewer scroll &&
                itemsCtrl.Items is INotifyCollectionChanged coll)
            {
                Execute.OnUIThread(() => scroll.ScrollToVerticalOffset(scroll.ExtentHeight));
                coll.CollectionChanged += (s2, ev2) =>
                {
                    if (ev2.Action == NotifyCollectionChangedAction.Add)
                    {
                        Execute.OnUIThread(() => scroll.ScrollToVerticalOffset(scroll.ExtentHeight));
                    }
                };
                scroll.SizeChanged += (s2, ev2) =>
                {
                    Execute.OnUIThread(() => scroll.ScrollToVerticalOffset(scroll.ExtentHeight));
                };
            }
        }
        catch
        {
        }
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        StopWinEventHook();
    }

    #region Win32

    // Keep Win32 constants for clarity; underscore naming matches Win32 defines
#pragma warning disable SA1310 // Field names intentionally contain underscores for Win32 constants
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
    private uint _targetPid = 0;
    private IntPtr _overlayHwnd = IntPtr.Zero;

    // WinEventHook 用于即时订阅目标窗口的位置/大小变动
    private IntPtr _winEventHook = IntPtr.Zero;

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

            const uint WINEVENT_OUTOFCONTEXT = 0x0000;
            const uint WINEVENT_SKIPOWNPROCESS = 0x0002;

            const uint EVENT_OBJECT_LOCATIONCHANGE = 0x800B;

            // 获取目标窗口所属进程 id
            _ = PInvoke.GetWindowThreadProcessId((HWND)hwnd, out _targetPid);
            if (_targetPid == Environment.ProcessId)
            {
                return;
            }

            uint flags = WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS;

            // 订阅目标进程的 location change
            _winEventHook = (IntPtr)PInvoke.SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, HINSTANCE.Null, _winEventProc, _targetPid, 0, flags);
            if (_winEventHook == IntPtr.Zero)
            {
                _logger.Warning("SetWinEventHook for location change returned null for pid {Pid}", _targetPid);
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "StartWinEventHookForTarget failed");
        }
    }

    private void StopWinEventHook()
    {
        try
        {
            if (_winEventHook != IntPtr.Zero)
            {
                var toRemove = _winEventHook;
                try
                {
                    PInvoke.UnhookWinEvent((HWINEVENTHOOK)toRemove);
                }
                catch (Exception ex)
                {
                    _logger.Warning(ex, "UnhookWinEvent failed for hook {Hook}", toRemove);
                }

                _winEventHook = IntPtr.Zero;
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "StopWinEventHook failed");
        }
    }

    private int _invokePending;

    private void WinEventProc(HWINEVENTHOOK hWinEventHook, uint eventType, HWND hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
    {
        try
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

            Dispatcher.BeginInvoke(DispatcherPriority.Background, () =>
            {
                if (Interlocked.Exchange(ref _invokePending, 1) == 1)
                {
                    return;
                }

                _debounceTimer.Stop();
                _debounceTimer.Start();
                Interlocked.Exchange(ref _invokePending, 0);
            });
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Exception in WinEventProc (native callback)");
        }
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

            PInvoke.SetWindowPos(
                (HWND)_overlayHwnd, (HWND)IntPtr.Zero,
                newLeft, newTop, newWidth, newHeight,
                SET_WINDOW_POS_FLAGS.SWP_ASYNCWINDOWPOS |
                SET_WINDOW_POS_FLAGS.SWP_NOZORDER |
                SET_WINDOW_POS_FLAGS.SWP_NOACTIVATE |
                SET_WINDOW_POS_FLAGS.SWP_NOCOPYBITS);
        }

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
            border.MaxWidth = Math.Clamp(newWidthWpf - horizontalMargin, 0, 250);
        }
    }

    /// <summary>
    /// Show the overlay but ensure it is positioned according to the target window first.
    /// The window will be shown with Opacity=0 until positioned to avoid visible flicker.
    /// </summary>
    /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
    public async Task InitializeAndShowAsync()
    {
        Opacity = 0;
        Show();
        _ = SetWindowLongPtrCompat(_overlayHwnd, (int)WINDOW_LONG_PTR_INDEX.GWL_HWNDPARENT, _targetHwnd);
        UpdatePosition();
        Opacity = 1;
    }
}
