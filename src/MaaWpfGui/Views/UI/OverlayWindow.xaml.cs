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

namespace MaaWpfGui.Views.UI;

public partial class OverlayWindow : Window
{
    public OverlayWindow()
    {
        InitializeComponent();
        Loaded += OnLoaded;
        Closed += OnClosed;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        var hwnd = new WindowInteropHelper(this).Handle;
        var exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

        // 鼠标穿透 + 分层窗口
        SetWindowLong(hwnd, GWL_EXSTYLE,
            exStyle | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);

        // 启动附加与位置跟踪
        StartTracking();

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

    // 目标模拟器窗口句柄
    private IntPtr _targetHwnd = IntPtr.Zero;
    private DispatcherTimer _attachTimer;
    private DispatcherTimer _posTimer;
    // WinEventHook 用于即时订阅目标窗口的位置/大小变动
    private IntPtr _winEventHook = IntPtr.Zero;
    private WinEventDelegate _winEventDelegate;

    // 每次窗口加载后启动定时器，尝试附加到 MuMu 模拟器窗口并跟踪其位置
    private void StartTracking()
    {
        _attachTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(1) };
        _attachTimer.Tick += (s, e) =>
        {
            if (_targetHwnd == IntPtr.Zero)
            {
                if (TryAttachToMuMuWindow())
                {
                    _attachTimer.Stop();
                    // 开始位置跟踪
                    _posTimer?.Stop();
                    _posTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
                    _posTimer.Tick += (s2, e2) => UpdatePosition();
                    _posTimer.Start();
                }
            }
        };
        _attachTimer.Start();
    }

    private void StopTracking()
    {
        _attachTimer?.Stop();
        _posTimer?.Stop();
        StopWinEventHook();
        _targetHwnd = IntPtr.Zero;
    }

    private bool TryAttachToMuMuWindow()
    {
        var patterns = new[] { "MuMuNxDevice" };
        foreach (var proc in Process.GetProcesses())
        {
            try
            {
                var name = proc.ProcessName ?? string.Empty;
                var title = proc.MainWindowTitle ?? string.Empty;
                if (patterns.Any(p => name.IndexOf(p, StringComparison.OrdinalIgnoreCase) >= 0) ||
                    patterns.Any(p => title.IndexOf(p, StringComparison.OrdinalIgnoreCase) >= 0))
                {
                    var hwnd = proc.MainWindowHandle;
                    if (hwnd == IntPtr.Zero)
                        hwnd = FindWindowForProcess(proc.Id);

                    if (hwnd != IntPtr.Zero)
                    {
                        _targetHwnd = hwnd;
                        // 为目标窗口创建 WinEventHook，以便在窗口移动/调整大小时即时更新
                        StartWinEventHookForTarget(_targetHwnd);
                        return true;
                    }
                }
            }
            catch
            {
                // 忽略进程无法访问的情况
            }
        }

        return false;
    }

    private void StartWinEventHookForTarget(IntPtr hwnd)
    {
        try
        {
            if (hwnd == IntPtr.Zero)
                return;

            if (_winEventHook != IntPtr.Zero)
                return; // already hooked

            // 获取目标窗口所属进程 id
            GetWindowThreadProcessId(hwnd, out var pid);

            _winEventDelegate = WinEventProc;

            const uint WINEVENT_OUTOFCONTEXT = 0x0000;
            const uint WINEVENT_SKIPOWNPROCESS = 0x0002;

            const uint EVENT_OBJECT_LOCATIONCHANGE = 0x800B;
            const uint EVENT_OBJECT_SHOW = 0x8002;
            const uint EVENT_OBJECT_HIDE = 0x8003;

            // 仅订阅目标进程的 location change / show / hide
            _winEventHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, IntPtr.Zero, _winEventDelegate, (uint)pid, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

            // 也订阅 show/hide 以处理窗口出现/隐藏情况
            // 可以为这些事件创建额外的 hook，但这里尽量简洁：如果需要更全面，后续可扩展
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
        });
    }

    private void UpdatePosition()
    {
        if (_targetHwnd == IntPtr.Zero)
            return;

        if (!GetWindowRect(_targetHwnd, out var rect))
            return;

        // 将像素坐标转换为 WPF 设备独立像素
        var source = PresentationSource.FromVisual(this);
        if (source?.CompositionTarget == null)
            return;

        var transform = source.CompositionTarget.TransformFromDevice;
        var topLeft = transform.Transform(new Point(rect.Left, rect.Top));
        var bottomRight = transform.Transform(new Point(rect.Right, rect.Bottom));

        var newLeft = topLeft.X;
        var newTop = topLeft.Y + 40;
        var newWidth = Math.Max(0, bottomRight.X - topLeft.X);
        var newHeight = Math.Max(0, bottomRight.Y - topLeft.Y - 40);

        // 更新 Overlay 窗口位置和大小
        if (!DoubleUtilAreClose(Left, newLeft)) Left = newLeft;
        if (!DoubleUtilAreClose(Top, newTop)) Top = newTop;
        if (!DoubleUtilAreClose(Width, newWidth)) Width = newWidth;
        if (!DoubleUtilAreClose(Height, newHeight)) Height = newHeight;

        // 将 Border 的 MaxWidth 限制为模拟器窗口宽度减去左右 Margin（留出间距）
        try
        {
            var border = FindName("OuterBorder") as System.Windows.Controls.Border;
            if (border != null)
            {
                // Border 的 Margin 左右之和
                double horizontalMargin = border.Margin.Left + border.Margin.Right;
                var maxW = Math.Max(0, newWidth - horizontalMargin);
                border.MaxWidth = maxW;
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
                TryAttachToMuMuWindow();
                if (_targetHwnd != IntPtr.Zero)
                    break;
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

    private IntPtr FindWindowForProcess(int pid)
    {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) =>
        {
            if (!IsWindowVisible(hWnd))
                return true; // continue

            GetWindowThreadProcessId(hWnd, out var windowPid);
            if (windowPid == pid)
            {
                found = hWnd;
                return false; // stop enumeration
            }

            return true;
        }, IntPtr.Zero);

        return found;
    }

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

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
