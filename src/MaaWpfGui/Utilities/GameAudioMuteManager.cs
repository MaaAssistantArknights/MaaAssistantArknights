// <copyright file="GameAudioMuteManager.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.Graphics.Gdi;
using Windows.Win32.Media.Audio;
using Windows.Win32.System.Com;
using Windows.Win32.UI.WindowsAndMessaging;

namespace MaaWpfGui.Utilities;

internal static class GameAudioMuteManager
{
    private static readonly ILogger _logger = Log.ForContext(typeof(GameAudioMuteManager));
    private static readonly object _syncRoot = new();
    private static readonly List<AudioSessionMuteState> _mutedSessions = [];
    private static readonly HashSet<string> _mutedSessionIds = new(StringComparer.Ordinal);
    private static IntPtr _windowHwnd;
    private static uint _windowProcessId;
    private static uint _processId;
    private static bool _restoreMinimized;
    private static WINDOWPLACEMENT? _windowPlacement;
    private static long _version;
    private static bool _windowRestoreRequested;
    private static bool _windowRestoreRequestFailed;
    private static int _windowRestoreMatches;

    /// <summary>
    /// Retains the game window placement before connecting Core can move or restore it.
    /// </summary>
    /// <param name="hwnd">The game window handle.</param>
    public static void CaptureWindowPlacement(IntPtr hwnd)
    {
        if (hwnd == IntPtr.Zero || !PInvoke.IsWindow((HWND)hwnd))
        {
            return;
        }

        lock (_syncRoot)
        {
            _version++;
            if (_windowHwnd == hwnd && IsCapturedWindow())
            {
                // No native request was needed when the window already matched its original placement.
                // A posted request needs the confirmation worker; responsiveness alone is not completion.
                if (!_windowRestoreRequested && _windowRestoreMatches > 0)
                {
                    ClearWindowState();
                }
                else
                {
                    RestoreAudioCore();
                    ResetWindowRestoreRequest();
                    return;
                }
            }

            RestoreCore();
            _windowHwnd = hwnd;
            CaptureWindowPlacementCore(hwnd);
        }
    }

    /// <summary>
    /// Starts muting the process attached to the game window until the task ends.
    /// </summary>
    /// <param name="hwnd">The attached game window handle.</param>
    /// <param name="shouldContinue">Returns whether task-time muting is still required.</param>
    /// <returns>Whether muting was started.</returns>
    public static bool Start(IntPtr hwnd, Func<bool> shouldContinue)
    {
        if (hwnd == IntPtr.Zero)
        {
            return false;
        }

        _ = PInvoke.GetWindowThreadProcessId((HWND)hwnd, out var processId);
        if (processId == 0)
        {
            return false;
        }

        long version;
        lock (_syncRoot)
        {
            // Completion actions can start Core tasks while the GUI is already idle.
            if (!shouldContinue())
            {
                return false;
            }

            version = ++_version;
            RestoreAudioCore();
            if (_windowHwnd != hwnd)
            {
                RestoreWindowCore();
                _windowHwnd = hwnd;
                CaptureWindowPlacementCore(hwnd);
            }

            ResetWindowRestoreRequest();
            _processId = processId;
            MuteNewSessionsCore();
        }

        _ = Task.Run(() => MonitorAsync(version, shouldContinue));
        return true;
    }

    /// <summary>
    /// Restores the state retained by <see cref="Start"/>.
    /// This method is idempotent.
    /// </summary>
    /// <param name="restoreWindow">Whether to restore the retained window placement.</param>
    public static void Restore(bool restoreWindow = true)
    {
        lock (_syncRoot)
        {
            _version++;
            if (restoreWindow)
            {
                ResetWindowRestoreRequest();
                if (!RestoreWindowCore() && !_windowRestoreRequestFailed)
                {
                    // Void cleanup callers still need a confirmation path for their posted request.
                    _ = RestoreWindowWhenCoreIdleAsync(static () => false, _version, timeout: null);
                }
            }

            RestoreAudioCore();
        }
    }

    /// <summary>
    /// Restores audio immediately and the window after Core stops moving it.
    /// </summary>
    /// <param name="isCoreRunning">Returns whether Core is still running tasks.</param>
    /// <param name="timeout">Maximum caller wait; defaults to one minute. Pending window cleanup continues after timeout.</param>
    /// <returns>Whether this cleanup completed within the caller wait and was not superseded.</returns>
    public static async Task<bool> RestoreWhenCoreIdleAsync(Func<bool> isCoreRunning, TimeSpan? timeout = null)
    {
        long version;
        lock (_syncRoot)
        {
            version = ++_version;
            ResetWindowRestoreRequest();

            // Restore audio before completion actions can close the game process.
            RestoreAudioCore();
        }

        return await RestoreWindowWhenCoreIdleAsync(isCoreRunning, version, timeout ?? TimeSpan.FromMinutes(1)).ConfigureAwait(false);
    }

    private static async Task<bool> RestoreWindowWhenCoreIdleAsync(Func<bool> isCoreRunning, long version, TimeSpan? timeout)
    {
        var elapsed = Stopwatch.StartNew();
        try
        {
            while (true)
            {
                lock (_syncRoot)
                {
                    // A new capture, task, explicit restore or manual cleanup owns the newer version.
                    if (version != _version)
                    {
                        return false;
                    }

                    if (timeout is null && !IsCapturedWindow())
                    {
                        return false;
                    }

                    if (timeout is { } maxWait && elapsed.Elapsed >= maxWait)
                    {
                        _logger.Warning("Game window cleanup timed out before restoration was confirmed");
                        if (IsCapturedWindow())
                        {
                            // Keep one waiter for this version. Its result cannot overwrite newer operations.
                            _ = RestoreWindowWhenCoreIdleAsync(isCoreRunning, version, timeout: null);
                        }

                        return false;
                    }
                }

                if (isCoreRunning())
                {
                    await Task.Delay(50).ConfigureAwait(false);
                    continue;
                }

                // Core clears its running flag before its Win32 controller finishes restoration.
                await Task.Delay(100).ConfigureAwait(false);
                lock (_syncRoot)
                {
                    if (version != _version)
                    {
                        return false;
                    }

                    if (!isCoreRunning())
                    {
                        if (RestoreWindowCore())
                        {
                            return true;
                        }

                        if (_windowRestoreRequestFailed)
                        {
                            return false;
                        }
                    }
                }
            }
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "Failed while waiting for Core to finish restoring the game window");
            return false;
        }
    }

    private static bool IsCapturedWindow()
    {
        if (_windowHwnd == IntPtr.Zero || !PInvoke.IsWindow((HWND)_windowHwnd))
        {
            return false;
        }

        _ = PInvoke.GetWindowThreadProcessId((HWND)_windowHwnd, out var processId);
        return processId != 0 && processId == _windowProcessId;
    }

    private static void MuteNewSessionsCore()
    {
        if (_processId == 0)
        {
            return;
        }

        var previousCount = _mutedSessions.Count;
        try
        {
            MuteProcessSessions(_processId);
        }
        catch (Exception ex)
        {
            _logger.Warning(ex, "Failed to mute audio sessions for game process {ProcessId}", _processId);
        }

        var addedCount = _mutedSessions.Count - previousCount;
        if (addedCount != 0)
        {
            _logger.Information("Muted {Count} new audio session(s) for game process {ProcessId}", addedCount, _processId);
        }
    }

    private static async Task MonitorAsync(long version, Func<bool> shouldContinue)
    {
        try
        {
            while (true)
            {
                await Task.Delay(5000).ConfigureAwait(false);
                lock (_syncRoot)
                {
                    if (version != _version || !shouldContinue())
                    {
                        return;
                    }

                    MuteNewSessionsCore();
                }
            }
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "Failed while monitoring game audio sessions");
        }
    }

    private static void CaptureWindowPlacementCore(IntPtr hwnd)
    {
        ResetWindowRestoreRequest();
        _ = PInvoke.GetWindowThreadProcessId((HWND)hwnd, out _windowProcessId);
        _restoreMinimized = PInvoke.IsIconic((HWND)hwnd);
        _windowPlacement = null;
        var placement = new WINDOWPLACEMENT { length = (uint)Marshal.SizeOf<WINDOWPLACEMENT>(), };
        if (PInvoke.GetWindowPlacement((HWND)hwnd, ref placement))
        {
            _windowPlacement = placement;
        }
    }

    private static void MuteProcessSessions(uint processId)
    {
        IMMDeviceEnumerator? deviceEnumerator = null;
        IMMDeviceCollection? devices = null;

        try
        {
            deviceEnumerator = (IMMDeviceEnumerator)new MMDeviceEnumerator();
            deviceEnumerator.EnumAudioEndpoints(EDataFlow.eRender, DEVICE_STATE.DEVICE_STATE_ACTIVE, out devices);
            devices.GetCount(out var deviceCount);

            for (uint deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
            {
                IMMDevice? device = null;
                try
                {
                    devices.Item(deviceIndex, out device);
                    MuteDeviceSessions(device, processId);
                }
                catch (Exception ex)
                {
                    _logger.Debug(ex, "Failed to inspect an audio endpoint while muting process {ProcessId}", processId);
                }
                finally
                {
                    ReleaseComObject(device);
                }
            }
        }
        finally
        {
            ReleaseComObject(devices);
            ReleaseComObject(deviceEnumerator);
        }
    }

    private static unsafe void MuteDeviceSessions(IMMDevice device, uint processId)
    {
        IAudioSessionManager2? sessionManager = null;
        IAudioSessionEnumerator? sessionEnumerator = null;

        try
        {
            var sessionManagerGuid = typeof(IAudioSessionManager2).GUID;
            device.Activate(&sessionManagerGuid, CLSCTX.CLSCTX_ALL, null, out var sessionManagerObject);
            sessionManager = (IAudioSessionManager2)sessionManagerObject;
            sessionEnumerator = sessionManager.GetSessionEnumerator();
            sessionEnumerator.GetCount(out var sessionCount);

            for (var index = 0; index < sessionCount; index++)
            {
                IAudioSessionControl? sessionControl = null;
                try
                {
                    sessionEnumerator.GetSession(index, out sessionControl);
                    if (sessionControl is not IAudioSessionControl2 sessionControl2 ||
                        sessionControl is not ISimpleAudioVolume volume)
                    {
                        continue;
                    }

                    sessionControl2.GetProcessId(out var sessionProcessId);
                    if (sessionProcessId != processId)
                    {
                        continue;
                    }

                    PWSTR sessionInstanceIdPointer = default;
                    string? sessionInstanceId;
                    try
                    {
                        sessionControl2.GetSessionInstanceIdentifier(&sessionInstanceIdPointer);
                        sessionInstanceId = sessionInstanceIdPointer.ToString();
                    }
                    finally
                    {
                        if (sessionInstanceIdPointer.Value != null)
                        {
                            Marshal.FreeCoTaskMem((IntPtr)sessionInstanceIdPointer.Value);
                        }
                    }

                    if (string.IsNullOrEmpty(sessionInstanceId) || !_mutedSessionIds.Add(sessionInstanceId))
                    {
                        continue;
                    }

                    try
                    {
                        BOOL wasMuted = default;
                        volume.GetMute(&wasMuted);
                        var eventContext = Guid.Empty;
                        volume.SetMute(true, &eventContext);
                        _mutedSessions.Add(new(volume, wasMuted));
                        sessionControl = null;
                    }
                    catch
                    {
                        _mutedSessionIds.Remove(sessionInstanceId);
                        throw;
                    }
                }
                finally
                {
                    ReleaseComObject(sessionControl);
                }
            }
        }
        finally
        {
            ReleaseComObject(sessionEnumerator);
            ReleaseComObject(sessionManager);
        }
    }

    private static void RestoreCore()
    {
        RestoreWindowCore();
        RestoreAudioCore();
    }

    private static unsafe void RestoreAudioCore()
    {
        var hadAudioState = _mutedSessions.Count != 0;

        foreach (var state in _mutedSessions)
        {
            try
            {
                var eventContext = Guid.Empty;
                state.Volume.SetMute(state.WasMuted, &eventContext);
            }
            catch (Exception ex)
            {
                _logger.Debug(ex, "Failed to restore a game audio session's mute state");
            }
            finally
            {
                ReleaseComObject(state.Volume);
            }
        }

        _mutedSessions.Clear();
        _mutedSessionIds.Clear();
        if (hadAudioState)
        {
            _logger.Information("Restored game audio session mute state");
        }
    }

    private static bool RestoreWindowCore()
    {
        if (_windowHwnd == IntPtr.Zero)
        {
            return true;
        }

        if (!IsCapturedWindow() || (_windowPlacement is null && !_restoreMinimized))
        {
            ClearWindowState();
            return true;
        }

        // A placement can be queued or partly reflected in kernel state before the target processes it.
        // Bound this read-only WM_NULL acknowledgement instead of waiting indefinitely for its UI thread.
        if (!IsWindowResponsive())
        {
            _windowRestoreMatches = 0;
            return false;
        }

        var actual = new WINDOWPLACEMENT { length = (uint)Marshal.SizeOf<WINDOWPLACEMENT>() };
        var matches = _windowPlacement is { } expected
            ? PInvoke.GetWindowPlacement((HWND)_windowHwnd, ref actual) &&
              MatchesWindowPlacement(expected, actual)
            : (bool)PInvoke.IsIconic((HWND)_windowHwnd);
        if (matches)
        {
            if (++_windowRestoreMatches >= 3)
            {
                _logger.Information("Restored game window placement for HWND {Hwnd}", _windowHwnd);
                ClearWindowState();
                return true;
            }

            return false;
        }

        _windowRestoreMatches = 0;
        if (_windowRestoreRequested || _windowRestoreRequestFailed)
        {
            return false;
        }

        // Posting the request must not wait for an unresponsive game UI thread while holding _syncRoot.
        // Retain the placement until readback confirms completion; accepting a queued request is not success.
        if (_windowPlacement is { } placement)
        {
            placement.flags |= WINDOWPLACEMENT_FLAGS.WPF_ASYNCWINDOWPLACEMENT;
            _windowRestoreRequested = PInvoke.SetWindowPlacement((HWND)_windowHwnd, in placement);
        }
        else
        {
            _windowRestoreRequested = PInvoke.ShowWindowAsync((HWND)_windowHwnd, SHOW_WINDOW_CMD.SW_MINIMIZE);
        }

        _windowRestoreRequestFailed = !_windowRestoreRequested;
        if (_windowRestoreRequestFailed)
        {
            _logger.Warning("Failed to request game window restoration for HWND {Hwnd}", _windowHwnd);
        }

        return false;
    }

    private static bool MatchesWindowPlacement(WINDOWPLACEMENT expected, WINDOWPLACEMENT actual)
    {
        if (actual.showCmd != expected.showCmd)
        {
            return false;
        }

        var target = expected.rcNormalPosition;
        var current = actual.rcNormalPosition;
        if (current.left == target.left && current.top == target.top && current.right == target.right && current.bottom == target.bottom)
        {
            return true;
        }

        // SetWindowPlacement legitimately relocates a wholly off-screen saved rectangle after a
        // monitor disappears. Only accept that adjustment after issuing this version's restore.
        return _windowRestoreRequested &&
            current.right - current.left == target.right - target.left &&
            current.bottom - current.top == target.bottom - target.top &&
            PInvoke.MonitorFromRect(in target, MONITOR_FROM_FLAGS.MONITOR_DEFAULTTONULL).IsNull &&
            !PInvoke.MonitorFromRect(in current, MONITOR_FROM_FLAGS.MONITOR_DEFAULTTONULL).IsNull;
    }

    private static bool IsWindowResponsive() =>
        PInvoke.SendMessageTimeout((HWND)_windowHwnd, 0, default, default,
            SEND_MESSAGE_TIMEOUT_FLAGS.SMTO_ABORTIFHUNG | SEND_MESSAGE_TIMEOUT_FLAGS.SMTO_BLOCK, 50, out _) != IntPtr.Zero;

    private static void ResetWindowRestoreRequest()
    {
        _windowRestoreRequested = false;
        _windowRestoreRequestFailed = false;
        _windowRestoreMatches = 0;
    }

    private static void ClearWindowState()
    {
        _windowHwnd = IntPtr.Zero;
        _windowProcessId = 0;
        _processId = 0;
        _restoreMinimized = false;
        _windowPlacement = null;
        ResetWindowRestoreRequest();
    }

    private static void ReleaseComObject(object? value)
    {
        if (value is not null && Marshal.IsComObject(value))
        {
            _ = Marshal.FinalReleaseComObject(value);
        }
    }

    private sealed record AudioSessionMuteState(ISimpleAudioVolume Volume, bool WasMuted);
}
