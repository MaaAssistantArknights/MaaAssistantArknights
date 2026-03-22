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
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Interop;
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
    private static readonly HWND HwndTop = (HWND)IntPtr.Zero;
    private static readonly HWND HwndTopmost = (HWND)new IntPtr(-1);
    private static readonly HWND HwndNotTopmost = (HWND)new IntPtr(-2);
    private const double OverlayMarginLeft = 8;
    private const double OverlayMarginTop = 48;
    private const double OverlayMarginRight = 8;
    private const double OverlayMarginBottom = 8;
    private const double OverlayMaxWidth = 250;
    private const int SecondaryZOrderVerificationDelayMs = 75;
    private const uint WineventOutOfContext = 0x0000;
    private const uint WineventSkipOwnProcess = 0x0002;
    private const uint EventSystemForeground = 0x0003;
    private const uint EventSystemMinimizeStart = 0x0016;
    private const uint EventSystemMinimizeEnd = 0x0017;
    private const uint EventObjectDestroy = 0x8001;
    private const uint EventObjectShow = 0x8002;
    private const uint EventObjectHide = 0x8003;
    private const uint EventObjectLocationChange = 0x800B;
    private const int ObjidWindow = 0;

    // Instance delegate to keep callback alive for this instance; avoids global mapping complexity
    private readonly WINEVENTPROC _winEventProc;

    public OverlayWindow()
    {
        InitializeComponent();
        DataContext = Instances.OverlayViewModel;
        Loaded += OnLoaded;
        Closed += OnClosed;

        // Bind instance win event delegate to prevent it from being GC'd while hooks are active.
        _winEventProc = WinEventProc;
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
            StopWinEventHooks();
            StartWinEventHooksForTarget(_targetHwnd);
            SyncOverlayToTargetState(forceRecalculateSize: true);
        }

        // 自动滚动到最新内容：订阅 ItemsControl 的 Items 集合变化
        try
        {
            if (FindName("LogItemsControl") is ItemsControl itemsCtrl &&
                FindName("LogScrollViewer") is ScrollViewer scroll &&
                itemsCtrl.Items is INotifyCollectionChanged coll)
            {
                Execute.OnUIThread(() => scroll.ScrollToVerticalOffset(scroll.ExtentHeight));
                coll.CollectionChanged += (s2, ev2) => {
                    if (ev2.Action == NotifyCollectionChangedAction.Add)
                    {
                        Execute.OnUIThread(() => {
                            scroll.ScrollToVerticalOffset(scroll.ExtentHeight);
                            RequestUpdatePosition(forceRecalculateSize: true);
                        });
                    }
                };
                scroll.SizeChanged += (s2, ev2) => {
                    Execute.OnUIThread(() => {
                        scroll.ScrollToVerticalOffset(scroll.ExtentHeight);
                        RequestUpdatePosition(forceRecalculateSize: true);
                    });
                };
            }
        }
        catch
        {
        }
    }

    private void OnClosed(object? sender, EventArgs e)
    {
        StopWinEventHooks();
        Interlocked.Increment(ref _zOrderVerificationVersion);
        _zOrderVerificationCts?.Cancel();
        _zOrderVerificationCts?.Dispose();
        _zOrderVerificationCts = null;
        _overlayHwnd = IntPtr.Zero;
        _targetHwnd = IntPtr.Zero;
        _targetPid = 0;
    }

    #region Win32

#pragma warning disable SA1310 // Field names intentionally contain underscores for Win32 constants
    private const int WS_EX_TRANSPARENT = 0x20;
    private const int WS_EX_LAYERED = 0x80000;
    private const int WS_EX_NOACTIVATE = 0x08000000;
#pragma warning restore SA1310

    // Use CsWin32 generated PInvoke wrappers where possible.
    #endregion

    /// <summary>
    /// 目标窗口句柄（由用户选择）
    /// </summary>
    private IntPtr _targetHwnd = IntPtr.Zero;
    private uint _targetPid = 0;
    private IntPtr _overlayHwnd = IntPtr.Zero;
    private int _lastTargetWidth = -1;
    private int _lastTargetHeight = -1;
    private int _overlayWidth = 1;
    private int _overlayHeight = 1;
    private int _positionUpdateVersion;
    private int _lastAppliedPositionUpdateVersion;
    private int _positionUpdateScheduled;
    private int _forceRecalculateSizeRequested;
    private int _zOrderVerificationVersion;
    private bool _overlayHiddenByTargetState;
    private CancellationTokenSource? _zOrderVerificationCts = new();

    private IntPtr _locationChangeHook = IntPtr.Zero;
    private IntPtr _foregroundHook = IntPtr.Zero;
    private IntPtr _minimizeHook = IntPtr.Zero;
    private IntPtr _showHideHook = IntPtr.Zero;
    private IntPtr _destroyHook = IntPtr.Zero;

    private void StartWinEventHooksForTarget(IntPtr hwnd)
    {
        try
        {
            if (hwnd == IntPtr.Zero)
            {
                return;
            }

            // 获取目标窗口所属进程 id
            _ = PInvoke.GetWindowThreadProcessId((HWND)hwnd, out _targetPid);
            if (_targetPid == Environment.ProcessId)
            {
                return;
            }

            uint globalFlags = WineventOutOfContext;
            uint targetProcessFlags = WineventOutOfContext | WineventSkipOwnProcess;

            StopWinEventHooks();
            _foregroundHook = RegisterWinEventHook(EventSystemForeground, EventSystemForeground, 0, globalFlags, "foreground");
            _minimizeHook = RegisterWinEventHook(EventSystemMinimizeStart, EventSystemMinimizeEnd, _targetPid, targetProcessFlags, "minimize");
            _showHideHook = RegisterWinEventHook(EventObjectShow, EventObjectHide, _targetPid, targetProcessFlags, "show/hide");
            _destroyHook = RegisterWinEventHook(EventObjectDestroy, EventObjectDestroy, _targetPid, targetProcessFlags, "destroy");
            _locationChangeHook = RegisterWinEventHook(EventObjectLocationChange, EventObjectLocationChange, _targetPid, targetProcessFlags, "location change");
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "StartWinEventHooksForTarget failed");
        }
    }

    private IntPtr RegisterWinEventHook(uint eventMin, uint eventMax, uint processId, uint flags, string name)
    {
        var hook = (IntPtr)PInvoke.SetWinEventHook(eventMin, eventMax, HINSTANCE.Null, _winEventProc, processId, 0, flags);
        if (hook == IntPtr.Zero)
        {
            _logger.Warning("SetWinEventHook for {Name} returned null for pid {Pid}", name, processId);
        }

        return hook;
    }

    private void StopWinEventHooks()
    {
        try
        {
            UnhookWinEvent(ref _foregroundHook);
            UnhookWinEvent(ref _minimizeHook);
            UnhookWinEvent(ref _showHideHook);
            UnhookWinEvent(ref _destroyHook);
            UnhookWinEvent(ref _locationChangeHook);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "StopWinEventHooks failed");
        }
    }

    private void UnhookWinEvent(ref IntPtr hook)
    {
        if (hook == IntPtr.Zero)
        {
            return;
        }

        var toRemove = hook;
        try
        {
            PInvoke.UnhookWinEvent((HWINEVENTHOOK)toRemove);
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "UnhookWinEvent failed for hook {Hook}", toRemove);
        }

        hook = IntPtr.Zero;
    }

    private void WinEventProc(HWINEVENTHOOK hWinEventHook, uint eventType, HWND hwnd, int idObject, int idChild, uint dwEventThread, uint dwmsEventTime)
    {
        try
        {
            if (hwnd == HWND.Null)
            {
                return;
            }

            if (_targetHwnd == IntPtr.Zero)
            {
                return;
            }

            if (eventType >= EventObjectDestroy && idObject != ObjidWindow)
            {
                return;
            }

            switch (eventType)
            {
                case EventSystemForeground:
                    Execute.OnUIThread(() => {
                        UpdateOverlayZOrder(hwnd);
                        ScheduleSecondaryZOrderVerification();
                    });
                    break;

                case EventSystemMinimizeStart:
                case EventObjectHide:
                    if (hwnd == (HWND)_targetHwnd)
                    {
                        Execute.OnUIThread(HideOverlayForTargetState);
                    }

                    break;

                case EventSystemMinimizeEnd:
                case EventObjectShow:
                    if (hwnd == (HWND)_targetHwnd)
                    {
                        Execute.OnUIThread(() => {
                            SyncOverlayToTargetState(forceRecalculateSize: true);
                            ScheduleSecondaryZOrderVerification();
                        });
                    }

                    break;

                case EventObjectDestroy:
                    if (hwnd == (HWND)_targetHwnd)
                    {
                        Execute.OnUIThread(HandleTargetDestroyed);
                    }

                    break;

                case EventObjectLocationChange:
                    if (hwnd == (HWND)_targetHwnd)
                    {
                        RequestUpdatePosition();
                    }

                    break;
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Exception in WinEventProc (native callback)");
        }
    }

    private void RequestUpdatePosition(bool forceRecalculateSize = false)
    {
        if (forceRecalculateSize)
        {
            Interlocked.Exchange(ref _forceRecalculateSizeRequested, 1);
        }

        Interlocked.Increment(ref _positionUpdateVersion);
        if (Interlocked.Exchange(ref _positionUpdateScheduled, 1) != 0)
        {
            return;
        }

        Execute.OnUIThread(ProcessPendingPositionUpdates);
    }

    private void ProcessPendingPositionUpdates()
    {
        try
        {
            while (true)
            {
                int latestVersion = Volatile.Read(ref _positionUpdateVersion);
                if (latestVersion == _lastAppliedPositionUpdateVersion)
                {
                    break;
                }

                bool forceRecalculateSize = Interlocked.Exchange(ref _forceRecalculateSizeRequested, 0) != 0;
                UpdatePosition(forceRecalculateSize);
                _lastAppliedPositionUpdateVersion = latestVersion;

                if (latestVersion == Volatile.Read(ref _positionUpdateVersion))
                {
                    break;
                }
            }
        }
        finally
        {
            Interlocked.Exchange(ref _positionUpdateScheduled, 0);
            if (Volatile.Read(ref _positionUpdateVersion) != _lastAppliedPositionUpdateVersion &&
                Interlocked.Exchange(ref _positionUpdateScheduled, 1) == 0)
            {
                Execute.OnUIThread(ProcessPendingPositionUpdates);
            }
        }
    }

    private void SyncOverlayToTargetState(bool forceRecalculateSize = false)
    {
        if (!ShouldShowOverlay())
        {
            HideOverlayForTargetState();
            return;
        }

        ShowOverlayForTargetState();
        UpdatePosition(forceRecalculateSize);
        UpdateOverlayZOrder(PInvoke.GetForegroundWindow());
    }

    private void ScheduleSecondaryZOrderVerification()
    {
        var cts = _zOrderVerificationCts;
        if (cts == null)
        {
            return;
        }

        int version = Interlocked.Increment(ref _zOrderVerificationVersion);
        _ = Task.Run(async () => {
            try
            {
                await Task.Delay(SecondaryZOrderVerificationDelayMs, cts.Token).ConfigureAwait(false);
            }
            catch (OperationCanceledException)
            {
                return;
            }

            if (cts.IsCancellationRequested)
            {
                return;
            }

            if (version != Volatile.Read(ref _zOrderVerificationVersion))
            {
                return;
            }

            Execute.OnUIThread(() => {
                if (cts.IsCancellationRequested || _overlayHwnd == IntPtr.Zero)
                {
                    return;
                }

                if (version != Volatile.Read(ref _zOrderVerificationVersion))
                {
                    return;
                }

                UpdateOverlayZOrder(PInvoke.GetForegroundWindow());
            });
        });
    }

    private bool ShouldShowOverlay()
    {
        return _overlayHwnd != IntPtr.Zero &&
               _targetHwnd != IntPtr.Zero &&
               PInvoke.IsWindowVisible((HWND)_targetHwnd) &&
               !PInvoke.IsIconic((HWND)_targetHwnd);
    }

    private void ShowOverlayForTargetState()
    {
        if (_overlayHwnd == IntPtr.Zero || !_overlayHiddenByTargetState)
        {
            return;
        }

        _overlayHiddenByTargetState = false;
        PInvoke.ShowWindow((HWND)_overlayHwnd, SHOW_WINDOW_CMD.SW_SHOWNOACTIVATE);
    }

    private void HideOverlayForTargetState()
    {
        if (_overlayHwnd == IntPtr.Zero || _overlayHiddenByTargetState)
        {
            return;
        }

        _overlayHiddenByTargetState = true;
        PInvoke.ShowWindow((HWND)_overlayHwnd, SHOW_WINDOW_CMD.SW_HIDE);
    }

    private void UpdateOverlayZOrder(HWND foregroundWindow)
    {
        if (_overlayHwnd == IntPtr.Zero || _targetHwnd == IntPtr.Zero || _overlayHiddenByTargetState)
        {
            return;
        }

        if (foregroundWindow == (HWND)_targetHwnd)
        {
            SetOverlayTopmostState(true);
            return;
        }

        if (foregroundWindow == HWND.Null || foregroundWindow == (HWND)_overlayHwnd)
        {
            return;
        }

        SetOverlayTopmostState(false);
        PInvoke.SetWindowPos(foregroundWindow, HwndTop, 0, 0, 0, 0, GetZOrderOnlyFlags());
    }

    private void SetOverlayTopmostState(bool topmost)
    {
        if (_overlayHwnd == IntPtr.Zero)
        {
            return;
        }

        PInvoke.SetWindowPos(
            (HWND)_overlayHwnd,
            topmost ? HwndTopmost : HwndNotTopmost,
            0,
            0,
            0,
            0,
            GetZOrderOnlyFlags());
    }

    private static SET_WINDOW_POS_FLAGS GetZOrderOnlyFlags()
    {
        return SET_WINDOW_POS_FLAGS.SWP_NOMOVE |
               SET_WINDOW_POS_FLAGS.SWP_NOSIZE |
               SET_WINDOW_POS_FLAGS.SWP_NOACTIVATE;
    }

    private void HandleTargetDestroyed()
    {
        HideOverlayForTargetState();
        StopWinEventHooks();
        _targetHwnd = IntPtr.Zero;
        _targetPid = 0;
    }

    private void UpdatePosition(bool forceRecalculateSize = false)
    {
        if (!ShouldShowOverlay())
        {
            HideOverlayForTargetState();
            return;
        }

        ShowOverlayForTargetState();

        if (_targetHwnd == IntPtr.Zero)
        {
            return;
        }

        if (!PInvoke.GetWindowRect((HWND)_targetHwnd, out var rect))
        {
            return;
        }

        int targetWidth = rect.right - rect.left;
        int targetHeight = rect.bottom - rect.top;
        bool updateSize = forceRecalculateSize ||
                          targetWidth != _lastTargetWidth ||
                          targetHeight != _lastTargetHeight;

        if (updateSize)
        {
            RecalculateOverlaySize(rect);
        }

        MoveOverlay(rect, updateSize);
        UpdateOverlayZOrder(PInvoke.GetForegroundWindow());

        _lastTargetWidth = targetWidth;
        _lastTargetHeight = targetHeight;
    }

    private void RecalculateOverlaySize(RECT rect)
    {
        if (FindName("OuterBorder") is not Border border)
        {
            return;
        }

        _overlayWidth = Math.Max(1, (int)Math.Ceiling(border.ActualWidth));
        _overlayHeight = Math.Max(1, (int)Math.Ceiling(border.ActualHeight));

        var source = PresentationSource.FromVisual(this);
        if (source?.CompositionTarget == null)
        {
            return;
        }

        var transform = source.CompositionTarget.TransformFromDevice;
        var transformToDevice = source.CompositionTarget.TransformToDevice;
        var topLeft = transform.Transform(new Point(rect.left, rect.top));
        var bottomRight = transform.Transform(new Point(rect.right, rect.bottom));
        var newWidthWpf = Math.Max(0, bottomRight.X - topLeft.X);
        var newHeightWpf = Math.Max(0, bottomRight.Y - topLeft.Y);
        double availableWidth = Math.Max(0, newWidthWpf - OverlayMarginLeft - OverlayMarginRight);
        double availableHeight = Math.Max(0, newHeightWpf - OverlayMarginTop - OverlayMarginBottom);
        border.MaxWidth = Math.Clamp(availableWidth, 0, OverlayMaxWidth);
        border.MaxHeight = availableHeight;

        border.Measure(new Size(border.MaxWidth, border.MaxHeight));
        var desiredSizeInPixels = transformToDevice.Transform(new Point(border.DesiredSize.Width, border.DesiredSize.Height));
        _overlayWidth = Math.Max(1, (int)Math.Ceiling(desiredSizeInPixels.X));
        _overlayHeight = Math.Max(1, (int)Math.Ceiling(desiredSizeInPixels.Y));
    }

    private void MoveOverlay(RECT rect, bool updateSize)
    {
        if (_overlayHwnd == IntPtr.Zero)
        {
            return;
        }

        var marginInPixels = GetOverlayMarginInPixels();
        int newLeft = rect.left + marginInPixels.Left;
        int newTop = rect.top + marginInPixels.Top;
        var flags = SET_WINDOW_POS_FLAGS.SWP_NOZORDER |
                    SET_WINDOW_POS_FLAGS.SWP_NOACTIVATE |
                    SET_WINDOW_POS_FLAGS.SWP_NOCOPYBITS;
        if (!updateSize)
        {
            flags |= SET_WINDOW_POS_FLAGS.SWP_NOSIZE;
        }

        PInvoke.SetWindowPos(
            (HWND)_overlayHwnd, (HWND)IntPtr.Zero,
            newLeft, newTop, _overlayWidth, _overlayHeight,
            flags);
    }

    private (int Left, int Top) GetOverlayMarginInPixels()
    {
        var source = PresentationSource.FromVisual(this);
        if (source?.CompositionTarget == null)
        {
            return ((int)Math.Round(OverlayMarginLeft), (int)Math.Round(OverlayMarginTop));
        }

        var margin = source.CompositionTarget.TransformToDevice.Transform(new Point(OverlayMarginLeft, OverlayMarginTop));
        return ((int)Math.Round(margin.X), (int)Math.Round(margin.Y));
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
        SyncOverlayToTargetState(forceRecalculateSize: true);
        Opacity = 1;
    }
}
