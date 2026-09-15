// <copyright file="GameAudioMuteManager.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
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
    private static uint _processId;
    private static bool _restoreMinimized;
    private static WINDOWPLACEMENT? _windowPlacement;
    private static long _version;

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
            version = ++_version;
            RestoreCore();
            _windowHwnd = hwnd;
            _processId = processId;
            CaptureWindowPlacementCore(hwnd);
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
                RestoreWindowCore();
            }

            RestoreAudioCore();
        }
    }

    /// <summary>
    /// Restores the window and audio after Core stops moving the window.
    /// </summary>
    /// <param name="isCoreRunning">Returns whether Core is still running tasks.</param>
    /// <returns>A task representing the delayed window restoration.</returns>
    public static async Task RestoreWhenCoreIdleAsync(Func<bool> isCoreRunning)
    {
        long version;
        lock (_syncRoot)
        {
            if (_windowHwnd == IntPtr.Zero && _mutedSessions.Count == 0)
            {
                return;
            }

            version = ++_version;
        }

        const int MaxAttempts = 1200;
        try
        {
            for (var attempt = 0; attempt < MaxAttempts && isCoreRunning(); attempt++)
            {
                await Task.Delay(50).ConfigureAwait(false);
            }

            // Core sets its running flag before its Win32 controller finishes restoring the window.
            await Task.Delay(100).ConfigureAwait(false);
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "Failed while waiting for Core to finish restoring the game window");
        }

        lock (_syncRoot)
        {
            if (version == _version)
            {
                RestoreCore();
            }
        }
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
                if (!shouldContinue())
                {
                    return;
                }

                lock (_syncRoot)
                {
                    if (version != _version)
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

    private static void RestoreWindowCore()
    {
        if (_windowHwnd == IntPtr.Zero)
        {
            return;
        }

        var restored = false;
        if (_windowPlacement is { } placement)
        {
            restored = PInvoke.SetWindowPlacement((HWND)_windowHwnd, in placement);
        }
        else if (_restoreMinimized)
        {
            restored = PInvoke.ShowWindow((HWND)_windowHwnd, SHOW_WINDOW_CMD.SW_MINIMIZE);
        }

        if (_windowPlacement is not null || _restoreMinimized)
        {
            if (!restored)
            {
                _logger.Warning("Failed to restore game window placement for HWND {Hwnd}", _windowHwnd);
            }
            else
            {
                _logger.Information("Restored game window placement for HWND {Hwnd}", _windowHwnd);
            }
        }

        _windowHwnd = IntPtr.Zero;
        _processId = 0;
        _restoreMinimized = false;
        _windowPlacement = null;
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
