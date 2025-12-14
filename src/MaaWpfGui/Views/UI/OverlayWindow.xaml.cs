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
using System.Diagnostics;
using System.Linq;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Collections.Specialized;
using System.Windows.Controls;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Threading;
using System.Windows.Media;
using System.Runtime.CompilerServices;

namespace MaaWpfGui.Views.UI;

public partial class OverlayWindow : Window
{
    public event EventHandler LoadedCompleted;

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
                return;

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
                catch { }
            }

            _targetHwnd = hwnd;

            // Stop any previous hooks
            StopWinEventHook();

            // Start WinEventHook and position tracking (no polling)
            StartWinEventHookForTarget(_targetHwnd);

            // Do an immediate position update once hooked
            UpdatePosition();

            // If we temporarily showed the overlay, restore collapsed state according to Idle
            if (restoreCollapsed)
            {
                try
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
                catch { }
            }
        }
        catch
        {
            // ignored
        }
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        var hwnd = new WindowInteropHelper(this).Handle;
        _overlayHwnd = hwnd;
        var exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        // 鼠标穿透 + 分层窗口
        SetWindowLong(hwnd, GWL_EXSTYLE,
            exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);

        // 启动附加与位置跟踪
        StartTracking();

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
            var itemsCtrl = FindName("LogItemsControl") as ItemsControl;
            var scroll = FindName("LogScrollViewer") as ScrollViewer;
            if (itemsCtrl != null && scroll != null)
            {
                if (itemsCtrl.Items is INotifyCollectionChanged coll)
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
        }
        catch
        {
            // ignored
        }

        // 通知已完成加载，可以恢复配置了
        LoadedCompleted?.Invoke(this, EventArgs.Empty);
    }

    private void OnClosed(object sender, EventArgs e)
    {
        StopTracking();

        if (Instances.SettingsViewModel is INotifyPropertyChanged inpc)
        {
            inpc.PropertyChanged -= Settings_PropertyChanged;
        }
    }

    private void Settings_PropertyChanged(object sender, PropertyChangedEventArgs e)
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

    private const int GWL_EXSTYLE = -20;
    private const int WS_EX_TRANSPARENT = 0x20;
    private const int WS_EX_LAYERED = 0x80000;
    private const int WS_EX_NOACTIVATE = 0x08000000;

    [DllImport("user32.dll")]
    private static extern int GetWindowLong(IntPtr hwnd, int index);

    [DllImport("user32.dll")]
    private static extern int SetWindowLong(IntPtr hwnd, int index, int value);

    #endregion

    // 目标窗口句柄（由用户选择）
    private IntPtr _targetHwnd = IntPtr.Zero;
    private DispatcherTimer _attachTimer; // 保留但不用于自动查找
    private IntPtr _overlayHwnd = IntPtr.Zero;
    // WinEventHook 用于即时订阅目标窗口的位置/大小变动
    private IntPtr _winEventHook = IntPtr.Zero;
    private IntPtr _winEventHookShowHide = IntPtr.Zero;
    private WinEventDelegate _winEventDelegate;

    // 每次窗口加载后启动定时器，尝试附加到 MuMu 模拟器窗口并跟踪其位置
    private void StartTracking()
    {
        // 不自动查找目标窗口。保留 attachTimer 以便未来扩展，但不启动自动搜索。
        _attachTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
    }

    private void StopTracking()
    {
        _attachTimer?.Stop();
        StopWinEventHook();
        _targetHwnd = IntPtr.Zero;
    }

    private void StartWinEventHookForTarget(IntPtr hwnd)
    {
        try
        {
            if (hwnd == IntPtr.Zero)
                return;

            // 如果已经有钩子，先清理旧的
            if (_winEventHook != IntPtr.Zero)
            {
                StopWinEventHook();
            }

            // 获取目标窗口所属进程 id
            GetWindowThreadProcessId(hwnd, out var pid);

            _winEventDelegate = WinEventProc;

            const uint WINEVENT_OUTOFCONTEXT = 0x0000;
            const uint WINEVENT_SKIPOWNPROCESS = 0x0002;

            const uint EVENT_OBJECT_LOCATIONCHANGE = 0x800B;
            const uint EVENT_OBJECT_SHOW = 0x8002;
            const uint EVENT_OBJECT_HIDE = 0x8003;

            // 订阅目标进程的 location change
            _winEventHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, IntPtr.Zero, _winEventDelegate, (uint)pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

            // 单独订阅 show/hide 事件以处理窗口出现/隐藏
            _winEventHookShowHide = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, IntPtr.Zero, _winEventDelegate, (uint)pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
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
                UnhookWinEvent(_winEventHook);
                _winEventHook = IntPtr.Zero;
                _winEventDelegate = null;
            }
            if (_winEventHookShowHide != IntPtr.Zero)
            {
                UnhookWinEvent(_winEventHookShowHide);
                _winEventHookShowHide = IntPtr.Zero;
            }
        }
        catch
        {
            // ignored
        }
    }

    private void WinEventProc(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
    {
        // 仅处理顶层窗口位置变化
        if (hwnd == IntPtr.Zero)
            return;

        if (_targetHwnd == IntPtr.Zero)
            return;

        if (hwnd != _targetHwnd)
            return;

        // 直接在 UI 线程更新位置，避免轮询延迟
        Dispatcher.InvokeAsync(() =>
        {
            try
            {
                UpdatePosition();
            }
            catch
            {
                // ignored
            }
        }, DispatcherPriority.Render);
    }

    private void UpdatePosition()
    {
        if (_targetHwnd == IntPtr.Zero)
            return;

        if (!GetWindowRect(_targetHwnd, out var rect))
            return;
        // Use native SetWindowPos for fast updates (device pixels)
        try
        {
            if (_overlayHwnd != IntPtr.Zero)
            {
                int newLeft = rect.Left;
                int newTop = rect.Top;
                int newWidth = Math.Max(0, rect.Right - rect.Left);
                int newHeight = Math.Max(0, rect.Bottom - rect.Top);

                const uint SWP_NOZORDER = 0x0004;
                const uint SWP_NOACTIVATE = 0x0010;

                SetWindowPos(_overlayHwnd, IntPtr.Zero, newLeft, newTop, newWidth, newHeight, SWP_NOZORDER | SWP_NOACTIVATE);

                // Update Border.MaxWidth on UI thread at Background priority (in WPF units)
                Dispatcher.BeginInvoke(() =>
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
                            var topLeft = transform.Transform(new Point(rect.Left, rect.Top));
                            var bottomRight = transform.Transform(new Point(rect.Right, rect.Bottom));
                            var newWidthWpf = Math.Max(0, bottomRight.X - topLeft.X);
                            double horizontalMargin = border.Margin.Left + border.Margin.Right;
                            var maxW = Math.Max(0, newWidthWpf - horizontalMargin);
                            border.MaxWidth = maxW;
                        }
                    }
                    catch { }
                }, DispatcherPriority.Background);
            }
        }
        catch
        {
            // ignored
        }
    }

    private static bool DoubleUtilAreClose(double a, double b)
    {
        return Math.Abs(a - b) < 0.5;
    }

    /// <summary>
    /// Show the overlay but ensure it is positioned according to the target window first.
    /// The window will be shown with Opacity=0 until positioned to avoid visible flicker.
    /// </summary>
    public async Task InitializeAndShowAsync()
    {
        try
        {
            // Make invisible while positioning
            Opacity = 0;
            Show();

            // Give the window a moment to initialize its presentation source
            await Task.Delay(20).ConfigureAwait(true);

            // Try to attach to target (may already be attached)
            int tries = 0;
            while (_targetHwnd == IntPtr.Zero && tries < 40)
            {
                await Task.Delay(50).ConfigureAwait(true);
                tries++;
            }

            // Update position once before making visible
            try
            {
                UpdatePosition();
            }
            catch
            {
                // ignored
            }

            // Make visible
            Opacity = 1;
        }
        catch
        {
            try
            {
                Opacity = 1;
            }
            catch
            {
                // ignored
            }
        }
    }

    #region Extra Win32 helpers for find window

    private delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    // WinEventHook APIs
    private delegate void WinEventDelegate(IntPtr hWinEventHook, uint eventType, IntPtr hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime);

    [DllImport("user32.dll")]
    private static extern IntPtr SetWinEventHook(uint eventMin, uint eventMax, IntPtr hmodWinEventProc, WinEventDelegate lpfnWinEventProc, uint idProcess, uint idThread, uint dwFlags);

    [DllImport("user32.dll")]
    private static extern bool UnhookWinEvent(IntPtr hWinEventHook);

    [StructLayout(LayoutKind.Sequential)]
    private struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    #endregion
}
