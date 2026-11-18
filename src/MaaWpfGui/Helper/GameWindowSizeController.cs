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
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using MaaWpfGui.Configuration.Global;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.Graphics.Gdi;
using Windows.Win32.UI.HiDpi;
using Windows.Win32.UI.WindowsAndMessaging;

namespace MaaWpfGui.Helper;

internal sealed class GameWindowSizeController : IDisposable
{
    private static readonly ILogger _logger = Log.ForContext<GameWindowSizeController>();

    // WinUser.h defines the per-monitor V2 context as the pseudo-handle -4.
    private static readonly DPI_AWARENESS_CONTEXT _perMonitorAwareV2 = (DPI_AWARENESS_CONTEXT)new IntPtr(-4);
    private SavedWindow? _original;
    private GameWindowRestoreState? _restoreState;
    private readonly string? _executablePath;
    private readonly Action<GameWindowRestoreState?>? _stateChanged;

    public GameWindowSizeController()
    {
    }

    public GameWindowSizeController(GameWindowRestoreState? restoreState, Action<GameWindowRestoreState?> stateChanged)
    {
        _restoreState = IsValidRestoreState(restoreState) ? restoreState : null;
        _stateChanged = stateChanged;
    }

    public GameWindowSizeController(GameWindowRestoreState? restoreState, Action<GameWindowRestoreState?> stateChanged, string executablePath)
        : this(restoreState, stateChanged)
    {
        _executablePath = NormalizeExecutablePath(executablePath) ?? throw new ArgumentException("An absolute executable path is required.", nameof(executablePath));
    }

    internal enum Status
    {
        Resized,
        AlreadyStandard,
        InstallationChanged,
        Restored,
        WindowedModeRequired,
        DisplayTooSmall,
        WindowClosed,
        Failed,
    }

    internal readonly record struct Result(Status Status, int Width = 0, int Height = 0, int Error = 0, bool WindowChanged = false);

    private sealed record SavedWindow(HWND Handle, uint ProcessId, RECT Bounds, RECT Client, string IdentityProperty);

    public bool CanRestore => _restoreState is not null;

    public GameWindowRestoreState? RestoreState => _restoreState;

    internal static bool IsValidRestoreState(GameWindowRestoreState? state) =>
        state is { ClientWidth: > 0 and <= 32767, ClientHeight: > 0 and <= 32767, Left: >= -1_000_000 and <= 1_000_000, Top: >= -1_000_000 and <= 1_000_000 };

    internal static string? NormalizeExecutablePath(string? executablePath)
    {
        if (string.IsNullOrWhiteSpace(executablePath) || !Path.IsPathFullyQualified(executablePath))
        {
            return null;
        }

        try
        {
            // Normalize Windows path spelling without binding records to a transient PID.
            var path = executablePath.StartsWith(@"\\?\UNC\", StringComparison.OrdinalIgnoreCase)
                ? @"\\" + executablePath[8..]
                : executablePath.StartsWith(@"\\?\", StringComparison.Ordinal) ? executablePath[4..] : executablePath;
            return Path.GetFullPath(path).ToUpperInvariant();
        }
        catch (Exception ex) when (ex is ArgumentException or NotSupportedException or PathTooLongException)
        {
            return null;
        }
    }

    internal static bool IsSameInstallation(GameWindowRestoreState state, string? executablePath)
    {
        var savedPath = NormalizeExecutablePath(state.ExecutablePath);
        return savedPath is not null && savedPath == NormalizeExecutablePath(executablePath);
    }

    internal static bool TryGetExecutablePath(IntPtr handle, uint expectedProcessId, out string? executablePath)
    {
        executablePath = null;
        try
        {
            using var process = Process.GetProcessById((int)expectedProcessId);
            var path = process.MainModule?.FileName;
            _ = PInvoke.GetWindowThreadProcessId((HWND)handle, out var processId);
            if (processId != expectedProcessId || !PInvoke.IsWindow((HWND)handle) || string.IsNullOrWhiteSpace(path) || !Path.IsPathFullyQualified(path))
            {
                return false;
            }

            executablePath = path;
            return true;
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "Unable to read game installation path for process {ProcessId}", expectedProcessId);
            return false;
        }
    }

    private void SetRestoreState(GameWindowRestoreState? state)
    {
        _restoreState = state;
        _stateChanged?.Invoke(state);
    }

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

    public unsafe Result Resize(IntPtr handle, uint expectedProcessId, bool allow720p)
    {
        if (CanRestore)
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
            _ = PInvoke.GetWindowThreadProcessId(hwnd, out var processId);
            if (!PInvoke.IsWindow(hwnd) || processId == 0 || processId != expectedProcessId)
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
            var sizes = allow720p ? new[] { (Width: 1920, Height: 1080), (Width: 1280, Height: 720) } : new[] { (Width: 1920, Height: 1080) };
            foreach (var size in sizes)
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
                return new(Status.DisplayTooSmall, allow720p ? 1280 : 1920, allow720p ? 720 : 1080);
            }

            if (client.right - client.left == width && client.bottom - client.top == height)
            {
                // No original size can be inferred when this process never adjusted the window.
                return new(Status.AlreadyStandard, width, height);
            }

            _ = PInvoke.GetWindowThreadProcessId(hwnd, out var currentProcessId);
            if (currentProcessId != expectedProcessId)
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
            var originalState = new GameWindowRestoreState {
                ExecutablePath = _executablePath ?? string.Empty,
                Left = bounds.left,
                Top = bounds.top,
                ClientWidth = client.right - client.left,
                ClientHeight = client.bottom - client.top,
            };
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
                    SetRestoreState(originalState);
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

            // A failed adjustment never creates persistent history; the UI guides manual recovery.
            var saved = _original;
            _ = RestoreCore(clearRestoreState: false);
            var changed = saved is not null && GeometryChanged(saved.Handle, saved.Bounds, saved.Client);
            ClearWindowBinding();
            return new(Status.Failed, Error: error, WindowChanged: changed);
        }
        finally
        {
            _ = PInvoke.SetThreadDpiAwarenessContext(previousContext);
        }
    }

    public unsafe Result Restore(IntPtr handle, uint expectedProcessId)
    {
        var previousContext = PInvoke.SetThreadDpiAwarenessContext(_perMonitorAwareV2);
        if (previousContext.IsNull)
        {
            return new(Status.Failed, Error: Marshal.GetLastWin32Error());
        }

        try
        {
            var state = _restoreState;
            if (!IsValidRestoreState(state))
            {
                return new(Status.WindowClosed);
            }

            var hwnd = (HWND)handle;
            _ = PInvoke.GetWindowThreadProcessId(hwnd, out var processId);
            if (!PInvoke.IsWindow(hwnd) || processId == 0 || processId != expectedProcessId)
            {
                return new(Status.WindowClosed);
            }

            if (_executablePath is not null && !IsSameInstallation(state!, _executablePath))
            {
                return new(Status.InstallationChanged);
            }

            var style = (WINDOW_STYLE)(uint)PInvoke.GetWindowLong(hwnd, WINDOW_LONG_PTR_INDEX.GWL_STYLE);
            if ((style & WINDOW_STYLE.WS_CAPTION) != WINDOW_STYLE.WS_CAPTION)
            {
                return new(Status.WindowedModeRequired);
            }

            var client = new RECT { right = state!.ClientWidth, bottom = state.ClientHeight };
            var bounds = client;
            var exStyle = (WINDOW_EX_STYLE)(uint)PInvoke.GetWindowLong(hwnd, WINDOW_LONG_PTR_INDEX.GWL_EXSTYLE);
            if (!PInvoke.AdjustWindowRectExForDpi(ref bounds, style & ~(WINDOW_STYLE.WS_MINIMIZE | WINDOW_STYLE.WS_MAXIMIZE), !PInvoke.GetMenu(hwnd).IsNull, exStyle, PInvoke.GetDpiForWindow(hwnd)))
            {
                return new(Status.Failed, Error: Marshal.GetLastWin32Error());
            }

            var outerWidth = bounds.right - bounds.left;
            var outerHeight = bounds.bottom - bounds.top;
            bounds = new RECT { left = state.Left, top = state.Top, right = state.Left + outerWidth, bottom = state.Top + outerHeight };
            var monitor = new MONITORINFO { cbSize = (uint)Marshal.SizeOf<MONITORINFO>() };
            if (!PInvoke.GetMonitorInfo(PInvoke.MonitorFromRect(in bounds, MONITOR_FROM_FLAGS.MONITOR_DEFAULTTONEAREST), ref monitor))
            {
                return new(Status.Failed, Error: Marshal.GetLastWin32Error());
            }

            bounds.left = Math.Clamp(bounds.left, monitor.rcWork.left, Math.Max(monitor.rcWork.left, monitor.rcWork.right - outerWidth));
            bounds.top = Math.Clamp(bounds.top, monitor.rcWork.top, Math.Max(monitor.rcWork.top, monitor.rcWork.bottom - outerHeight));
            bounds.right = bounds.left + outerWidth;
            bounds.bottom = bounds.top + outerHeight;

            // A persisted size is data, never permission to reuse an old HWND or PID.
            // Bind only the current uniquely selected game window for this operation.
            ClearWindowBinding();
            _ = PInvoke.GetWindowThreadProcessId(hwnd, out var currentProcessId);
            if (currentProcessId != expectedProcessId)
            {
                return new(Status.WindowClosed);
            }

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
            return RestoreCore();
        }
        finally
        {
            _ = PInvoke.SetThreadDpiAwarenessContext(previousContext);
        }
    }

    private Result RestoreCore(bool clearRestoreState = true)
    {
        var saved = _original;
        if (saved is null || !IsSameWindow(saved))
        {
            ClearWindowBinding();
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

        var target = saved.Bounds;
        var restored = false;
        var error = 0;
        for (var attempt = 0; attempt < 3; ++attempt)
        {
            if (MoveAndVerify(saved, target, width, height, out error))
            {
                restored = true;
                break;
            }

            // Moving onto another DPI may change the actual frame. Correct from readback, as Resize does.
            if (!IsSameWindow(saved) || !PInvoke.GetWindowRect(saved.Handle, out var actualBounds) || !PInvoke.GetClientRect(saved.Handle, out var actualClient))
            {
                break;
            }

            var outerWidth = actualBounds.right - actualBounds.left + width - (actualClient.right - actualClient.left);
            var outerHeight = actualBounds.bottom - actualBounds.top + height - (actualClient.bottom - actualClient.top);
            var corrected = new RECT { left = target.left, top = target.top, right = target.left + outerWidth, bottom = target.top + outerHeight };
            var monitor = new MONITORINFO { cbSize = (uint)Marshal.SizeOf<MONITORINFO>() };
            if (!PInvoke.GetMonitorInfo(PInvoke.MonitorFromRect(in target, MONITOR_FROM_FLAGS.MONITOR_DEFAULTTONEAREST), ref monitor))
            {
                error = Marshal.GetLastWin32Error();
                break;
            }

            corrected.left = Math.Clamp(corrected.left, monitor.rcWork.left, Math.Max(monitor.rcWork.left, monitor.rcWork.right - outerWidth));
            corrected.top = Math.Clamp(corrected.top, monitor.rcWork.top, Math.Max(monitor.rcWork.top, monitor.rcWork.bottom - outerHeight));
            corrected.right = corrected.left + outerWidth;
            corrected.bottom = corrected.top + outerHeight;
            if (corrected.left == target.left && corrected.top == target.top && corrected.right == target.right && corrected.bottom == target.bottom)
            {
                break;
            }

            target = corrected;
        }

        if (!restored)
        {
            _logger.Warning("Failed to restore game window {Hwnd}, error: {Error}", saved.Handle, error);
            return new(Status.Failed, Error: error, WindowChanged: GeometryChanged(saved.Handle, beforeBounds, beforeClient));
        }

        ClearWindowBinding();
        if (clearRestoreState)
        {
            SetRestoreState(null);
        }
        return new(Status.Restored, width, height, WindowChanged: GeometryChanged(saved.Handle, beforeBounds, beforeClient));
    }

    public void Dispose() => ClearWindowBinding();

    private void OnProcessExit(object? sender, EventArgs args) => ClearWindowBinding();

    private unsafe void ClearWindowBinding()
    {
        // Release runtime identity only; persisted geometry is cleared after a successful restore.
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
