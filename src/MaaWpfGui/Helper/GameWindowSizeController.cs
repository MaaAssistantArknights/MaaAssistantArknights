// <copyright file="GameWindowSizeController.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.Graphics.Gdi;
using Windows.Win32.UI.HiDpi;
using Windows.Win32.UI.WindowsAndMessaging;

namespace MaaWpfGui.Helper;

internal sealed class GameWindowSizeController
{
    private static readonly ILogger _logger = Log.ForContext<GameWindowSizeController>();

    // WinUser.h defines the per-monitor V2 context as the pseudo-handle -4.
    private static readonly DPI_AWARENESS_CONTEXT _perMonitorAwareV2 = (DPI_AWARENESS_CONTEXT)new IntPtr(-4);
    private SavedWindow? _original;

    internal enum Status
    {
        Resized,
        Restored,
        WindowedModeRequired,
        DisplayTooSmall,
        WindowClosed,
        Failed,
    }

    internal readonly record struct Result(Status Status, int Width = 0, int Height = 0, int Error = 0, bool WindowChanged = false);

    private sealed record SavedWindow(HWND Handle, uint ProcessId, RECT Bounds, RECT Client, string IdentityProperty);

    public bool CanRestore => _original is not null;

    internal static bool IsGameProcessWindow(IntPtr handle)
    {
        _ = PInvoke.GetWindowThreadProcessId((HWND)handle, out var processId);
        if (processId == 0)
        {
            return false;
        }

        try
        {
            using var process = Process.GetProcessById((int)processId);
            return string.Equals(process.ProcessName, "Arknights", StringComparison.OrdinalIgnoreCase);
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "Unable to verify game window process {ProcessId}", processId);
            return false;
        }
    }

    public unsafe Result Resize(IntPtr handle)
    {
        if (_original is not null)
        {
            return new(Status.Failed);
        }

        var previousContext = PInvoke.SetThreadDpiAwarenessContext(_perMonitorAwareV2);
        if (previousContext.IsNull)
        {
            return new(Status.Failed, Error: Marshal.GetLastWin32Error());
        }

        try
        {
            var hwnd = (HWND)handle;
            if (!PInvoke.IsWindow(hwnd))
            {
                return new(Status.WindowClosed);
            }

            var style = (WINDOW_STYLE)(uint)PInvoke.GetWindowLong(hwnd, WINDOW_LONG_PTR_INDEX.GWL_STYLE);
            if (PInvoke.IsIconic(hwnd) || (style & WINDOW_STYLE.WS_MAXIMIZE) != 0 || (style & WINDOW_STYLE.WS_CAPTION) != WINDOW_STYLE.WS_CAPTION)
            {
                return new(Status.WindowedModeRequired);
            }

            var monitor = new MONITORINFO { cbSize = (uint)Marshal.SizeOf<MONITORINFO>() };
            if (!PInvoke.GetMonitorInfo(PInvoke.MonitorFromWindow(hwnd, MONITOR_FROM_FLAGS.MONITOR_DEFAULTTONEAREST), ref monitor) ||
                !PInvoke.GetWindowRect(hwnd, out var bounds) || !PInvoke.GetClientRect(hwnd, out var client))
            {
                return new(Status.Failed, Error: Marshal.GetLastWin32Error());
            }

            var dpi = PInvoke.GetDpiForWindow(hwnd);
            var exStyle = (WINDOW_EX_STYLE)(uint)PInvoke.GetWindowLong(hwnd, WINDOW_LONG_PTR_INDEX.GWL_EXSTYLE);
            var hasMenu = !PInvoke.GetMenu(hwnd).IsNull;
            var width = 0;
            var height = 0;
            RECT target = default;
            foreach (var size in new[] { (Width: 1920, Height: 1080), (Width: 1280, Height: 720) })
            {
                target = new RECT { right = size.Width, bottom = size.Height };
                if (!PInvoke.AdjustWindowRectExForDpi(ref target, style, hasMenu, exStyle, dpi))
                {
                    return new(Status.Failed, Error: Marshal.GetLastWin32Error());
                }

                if (target.right - target.left <= monitor.rcWork.right - monitor.rcWork.left &&
                    target.bottom - target.top <= monitor.rcWork.bottom - monitor.rcWork.top)
                {
                    (width, height) = size;
                    break;
                }
            }

            if (width == 0)
            {
                return new(Status.DisplayTooSmall);
            }

            _ = PInvoke.GetWindowThreadProcessId(hwnd, out var processId);
            if (processId == 0)
            {
                return new(Status.WindowClosed);
            }

            // Window properties belong to the window lifetime, unlike a reusable HWND/PID pair.
            var identityProperty = $"MAA.WindowSize.{Guid.NewGuid():N}";
            fixed (char* propertyName = identityProperty)
            {
                if (!PInvoke.SetProp(hwnd, propertyName, (HANDLE)new IntPtr(1)))
                {
                    return new(Status.Failed, Error: Marshal.GetLastWin32Error());
                }
            }

            _original = new(hwnd, processId, bounds, client, identityProperty);
            AppDomain.CurrentDomain.ProcessExit += OnProcessExit;
            var outerWidth = target.right - target.left;
            var outerHeight = target.bottom - target.top;
            var error = 0;
            for (var attempt = 0; attempt < 3; attempt++)
            {
                target.left = monitor.rcWork.left + ((monitor.rcWork.right - monitor.rcWork.left - outerWidth) / 2);
                target.top = monitor.rcWork.top + ((monitor.rcWork.bottom - monitor.rcWork.top - outerHeight) / 2);
                target.right = target.left + outerWidth;
                target.bottom = target.top + outerHeight;
                if (MoveAndVerify(_original, target, width, height, out error))
                {
                    _logger.Information("Resized game window {Hwnd} to client size {Width}x{Height}", handle, width, height);
                    return new(Status.Resized, width, height, WindowChanged: GeometryChanged(hwnd, _original.Bounds, _original.Client));
                }

                // Read back the actual non-client dimensions instead of assuming a fixed title-bar height.
                if (!IsSameWindow(_original) || !PInvoke.GetClientRect(hwnd, out client) || !PInvoke.GetWindowRect(hwnd, out bounds))
                {
                    break;
                }

                outerWidth = bounds.right - bounds.left + width - (client.right - client.left);
                outerHeight = bounds.bottom - bounds.top + height - (client.bottom - client.top);
                if (outerWidth > monitor.rcWork.right - monitor.rcWork.left || outerHeight > monitor.rcWork.bottom - monitor.rcWork.top)
                {
                    break;
                }
            }

            // Keep the snapshot if rollback fails so that the restore button remains available.
            var saved = _original;
            _ = RestoreCore();
            return new(Status.Failed, Error: error, WindowChanged: saved is not null && GeometryChanged(saved.Handle, saved.Bounds, saved.Client));
        }
        finally
        {
            _ = PInvoke.SetThreadDpiAwarenessContext(previousContext);
        }
    }

    public Result Restore()
    {
        var previousContext = PInvoke.SetThreadDpiAwarenessContext(_perMonitorAwareV2);
        if (previousContext.IsNull)
        {
            return new(Status.Failed, Error: Marshal.GetLastWin32Error());
        }

        try
        {
            return RestoreCore();
        }
        finally
        {
            _ = PInvoke.SetThreadDpiAwarenessContext(previousContext);
        }
    }

    private Result RestoreCore()
    {
        var saved = _original;
        if (saved is null || !IsSameWindow(saved))
        {
            ClearSnapshot();
            return new(Status.WindowClosed);
        }

        var width = saved.Client.right - saved.Client.left;
        var height = saved.Client.bottom - saved.Client.top;
        if (!PInvoke.GetWindowRect(saved.Handle, out var beforeBounds) || !PInvoke.GetClientRect(saved.Handle, out var beforeClient))
        {
            return new(Status.Failed, Error: Marshal.GetLastWin32Error());
        }

        var style = (WINDOW_STYLE)(uint)PInvoke.GetWindowLong(saved.Handle, WINDOW_LONG_PTR_INDEX.GWL_STYLE);
        if ((PInvoke.IsIconic(saved.Handle) || (style & WINDOW_STYLE.WS_MAXIMIZE) != 0) &&
            !PInvoke.ShowWindowAsync(saved.Handle, SHOW_WINDOW_CMD.SW_SHOWNOACTIVATE))
        {
            return new(Status.Failed, Error: Marshal.GetLastWin32Error());
        }

        if (!MoveAndVerify(saved, saved.Bounds, width, height, out var error))
        {
            _logger.Warning("Failed to restore game window {Hwnd}, error: {Error}", saved.Handle, error);
            return new(Status.Failed, Error: error, WindowChanged: GeometryChanged(saved.Handle, beforeBounds, beforeClient));
        }

        ClearSnapshot();
        return new(Status.Restored, width, height, WindowChanged: GeometryChanged(saved.Handle, beforeBounds, beforeClient));
    }

    private void OnProcessExit(object? sender, EventArgs args) => ClearSnapshot();

    private unsafe void ClearSnapshot()
    {
        if (_original is { } saved && IsSameWindow(saved))
        {
            fixed (char* propertyName = saved.IdentityProperty)
            {
                _ = PInvoke.RemoveProp(saved.Handle, propertyName);
            }
        }

        _original = null;
        AppDomain.CurrentDomain.ProcessExit -= OnProcessExit;
    }

    private static bool GeometryChanged(HWND hwnd, RECT bounds, RECT client) =>
        !PInvoke.GetWindowRect(hwnd, out var actualBounds) || !PInvoke.GetClientRect(hwnd, out var actualClient) ||
        actualBounds.left != bounds.left || actualBounds.top != bounds.top || actualBounds.right != bounds.right || actualBounds.bottom != bounds.bottom ||
        actualClient.right != client.right || actualClient.bottom != client.bottom;

    private static unsafe bool IsSameWindow(SavedWindow saved)
    {
        _ = PInvoke.GetWindowThreadProcessId(saved.Handle, out var processId);
        fixed (char* propertyName = saved.IdentityProperty)
        {
            return PInvoke.IsWindow(saved.Handle) && processId == saved.ProcessId && PInvoke.GetProp(saved.Handle, propertyName) == (HANDLE)new IntPtr(1);
        }
    }

    private static bool MoveAndVerify(SavedWindow saved, RECT bounds, int clientWidth, int clientHeight, out int error)
    {
        error = 0;
        if (!IsSameWindow(saved))
        {
            return false;
        }

        if (!PInvoke.SetWindowPos(
                saved.Handle,
                HWND.Null,
                bounds.left,
                bounds.top,
                bounds.right - bounds.left,
                bounds.bottom - bounds.top,
                SET_WINDOW_POS_FLAGS.SWP_NOZORDER | SET_WINDOW_POS_FLAGS.SWP_NOACTIVATE | SET_WINDOW_POS_FLAGS.SWP_ASYNCWINDOWPOS))
        {
            error = Marshal.GetLastWin32Error();
            return false;
        }

        var matches = 0;
        for (var attempt = 0; attempt < 20; attempt++)
        {
            Thread.Sleep(50);
            if (!IsSameWindow(saved) || !PInvoke.GetClientRect(saved.Handle, out var client) || !PInvoke.GetWindowRect(saved.Handle, out var actual))
            {
                error = Marshal.GetLastWin32Error();
                return false;
            }

            if (client.right - client.left == clientWidth && client.bottom - client.top == clientHeight &&
                actual.left == bounds.left && actual.top == bounds.top && actual.right == bounds.right && actual.bottom == bounds.bottom)
            {
                if (++matches >= 3)
                {
                    return true;
                }
            }
            else
            {
                matches = 0;
            }
        }

        error = 1460; // ERROR_TIMEOUT: a queued resize was not observed at the requested bounds.
        return false;
    }
}
