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
using MaaWpfGui.Models;
using Serilog;
using Windows.Win32;
using Windows.Win32.Foundation;
using Windows.Win32.UI.WindowsAndMessaging;

namespace MaaWpfGui.Utilities;

internal static class GameAudioMuteManager
{
    private static readonly ILogger _logger = Log.ForContext(typeof(GameAudioMuteManager));
    private static readonly object _syncRoot = new();
    private static readonly List<AudioSessionMuteState> _mutedSessions = [];
    private static IntPtr _windowHwnd;
    private static bool _restoreMinimized;
    private static WindowPlacement? _windowPlacement;
    private static long _windowStateVersion;

    /// <summary>
    /// Mutes every audio session owned by the process associated with <paramref name="hwnd"/>.
    /// The original mute state is retained and can later be restored with <see cref="Restore"/>.
    /// </summary>
    /// <param name="hwnd">The attached game window handle.</param>
    /// <returns>Whether at least one matching audio session was muted.</returns>
    public static bool MuteWindow(IntPtr hwnd)
    {
        if (hwnd == IntPtr.Zero)
        {
            return false;
        }

        _ = PInvoke.GetWindowThreadProcessId((HWND)hwnd, out var processId);
        if (processId == 0)
        {
            _logger.Warning("Unable to get process ID for game window {Hwnd}", hwnd);
            return false;
        }

        lock (_syncRoot)
        {
            RestoreCore();
            _windowHwnd = hwnd;
            _restoreMinimized = PInvoke.IsIconic((HWND)hwnd);
            var placement = new WindowPlacement { Length = Marshal.SizeOf<WindowPlacement>(), };
            if (GetWindowPlacement(hwnd, ref placement))
            {
                _windowPlacement = placement;
            }

            _windowStateVersion++;

            try
            {
                MuteProcessSessions(processId);
            }
            catch (Exception ex)
            {
                _logger.Warning(ex, "Failed to mute audio sessions for game process {ProcessId}", processId);
            }

            if (_mutedSessions.Count == 0)
            {
                _logger.Warning("No audio session found for game process {ProcessId}", processId);
                return false;
            }

            _logger.Information("Muted {Count} audio session(s) for game process {ProcessId}", _mutedSessions.Count, processId);
            return true;
        }
    }

    /// <summary>
    /// Restores the mute state that each game audio session had before <see cref="MuteWindow"/> was called.
    /// This method is idempotent.
    /// </summary>
    public static void Restore()
    {
        lock (_syncRoot)
        {
            RestoreCore();
        }
    }

    /// <summary>
    /// Restores audio immediately, then restores the original window placement after Core stops moving the window.
    /// </summary>
    /// <param name="isCoreRunning">Returns whether Core is still running tasks.</param>
    /// <returns>A task representing the delayed window restoration.</returns>
    public static async Task RestoreWhenCoreIdleAsync(Func<bool> isCoreRunning)
    {
        long stateVersion;
        lock (_syncRoot)
        {
            RestoreAudioCore();
            stateVersion = _windowStateVersion;
        }

        const int MaxAttempts = 1200;
        try
        {
            for (var attempt = 0; attempt < MaxAttempts && isCoreRunning(); attempt++)
            {
                await Task.Delay(50);
            }

            // Core sets its running flag before its Win32 controller finishes restoring the window.
            await Task.Delay(100);
        }
        catch (Exception ex)
        {
            _logger.Debug(ex, "Failed while waiting for Core to finish restoring the game window");
        }

        lock (_syncRoot)
        {
            if (stateVersion == _windowStateVersion)
            {
                RestoreWindowCore();
            }
        }
    }

    /// <summary>
    /// Attempts to mute the attached window again when the audio session was created after attachment.
    /// </summary>
    public static void EnsureMuted()
    {
        lock (_syncRoot)
        {
            if (_windowHwnd == IntPtr.Zero || _mutedSessions.Count != 0)
            {
                return;
            }

            _ = PInvoke.GetWindowThreadProcessId((HWND)_windowHwnd, out var processId);
            if (processId == 0)
            {
                return;
            }

            try
            {
                MuteProcessSessions(processId);
            }
            catch (Exception ex)
            {
                _logger.Warning(ex, "Failed to retry muting audio sessions for game process {ProcessId}", processId);
            }

            if (_mutedSessions.Count != 0)
            {
                _logger.Information("Muted {Count} audio session(s) for game process {ProcessId} after task start", _mutedSessions.Count, processId);
            }
        }
    }

    private static void MuteProcessSessions(uint processId)
    {
        IMMDeviceEnumerator? deviceEnumerator = null;
        IMMDeviceCollection? devices = null;

        try
        {
            deviceEnumerator = (IMMDeviceEnumerator)new MMDeviceEnumeratorComObject();
            ThrowIfFailed(deviceEnumerator.EnumAudioEndpoints(AudioDataFlow.Render, (uint)DeviceState.Active, out devices));
            ThrowIfFailed(devices.GetCount(out var deviceCount));

            for (uint deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++)
            {
                IMMDevice? device = null;
                try
                {
                    ThrowIfFailed(devices.Item(deviceIndex, out device));
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

    private static void MuteDeviceSessions(IMMDevice device, uint processId)
    {
        IAudioSessionManager2? sessionManager = null;
        IAudioSessionEnumerator? sessionEnumerator = null;

        try
        {
            var sessionManagerGuid = typeof(IAudioSessionManager2).GUID;
            ThrowIfFailed(device.Activate(ref sessionManagerGuid, ClsCtx.All, IntPtr.Zero, out var sessionManagerObject));
            sessionManager = (IAudioSessionManager2)sessionManagerObject;
            ThrowIfFailed(sessionManager.GetSessionEnumerator(out sessionEnumerator));
            ThrowIfFailed(sessionEnumerator.GetCount(out var sessionCount));

            for (var index = 0; index < sessionCount; index++)
            {
                IAudioSessionControl? sessionControl = null;
                try
                {
                    ThrowIfFailed(sessionEnumerator.GetSession(index, out sessionControl));
                    if (sessionControl is not IAudioSessionControl2 sessionControl2 ||
                        sessionControl2.GetProcessId(out var sessionProcessId) < 0 ||
                        sessionProcessId != processId ||
                        sessionControl is not ISimpleAudioVolume volume)
                    {
                        continue;
                    }

                    ThrowIfFailed(volume.GetMute(out var wasMuted));
                    var eventContext = Guid.Empty;
                    ThrowIfFailed(volume.SetMute(true, ref eventContext));
                    _mutedSessions.Add(new(volume, wasMuted));
                    sessionControl = null;
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
        RestoreAudioCore();
        RestoreWindowCore();
    }

    private static void RestoreAudioCore()
    {
        var hadAudioState = _mutedSessions.Count != 0;

        foreach (var state in _mutedSessions)
        {
            try
            {
                var eventContext = Guid.Empty;
                ThrowIfFailed(state.Volume.SetMute(state.WasMuted, ref eventContext));
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
            restored = SetWindowPlacement(_windowHwnd, ref placement);
        }
        else if (_restoreMinimized)
        {
            restored = PInvoke.ShowWindow((HWND)_windowHwnd, SHOW_WINDOW_CMD.SW_MINIMIZE);
        }

        if (!restored)
        {
            _logger.Warning("Failed to restore game window placement for HWND {Hwnd}", _windowHwnd);
        }
        else
        {
            _logger.Information("Restored game window placement for HWND {Hwnd}", _windowHwnd);
        }

        _windowHwnd = IntPtr.Zero;
        _restoreMinimized = false;
        _windowPlacement = null;
    }

    private static void ThrowIfFailed(int hresult)
    {
        if (hresult < 0)
        {
            Marshal.ThrowExceptionForHR(hresult);
        }
    }

    private static void ReleaseComObject(object? value)
    {
        if (value is not null && Marshal.IsComObject(value))
        {
            _ = Marshal.FinalReleaseComObject(value);
        }
    }

    private sealed record AudioSessionMuteState(ISimpleAudioVolume Volume, bool WasMuted);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetWindowPlacement(IntPtr window, ref WindowPlacement placement);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool SetWindowPlacement(IntPtr window, [In] ref WindowPlacement placement);

    private enum AudioDataFlow
    {
        Render,
        Capture,
        All,
    }

    private enum AudioRole
    {
        Console,
        Multimedia,
        Communications,
    }

    [Flags]
    private enum DeviceState : uint
    {
        Active = 0x1,
    }

    [Flags]
    private enum ClsCtx : uint
    {
        InprocServer = 0x1,
        InprocHandler = 0x2,
        LocalServer = 0x4,
        RemoteServer = 0x10,
        All = InprocServer | InprocHandler | LocalServer | RemoteServer,
    }

    [ComImport]
    [Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")]
    private class MMDeviceEnumeratorComObject
    {
    }

    [ComImport]
    [Guid("A95664D2-9614-4F35-A746-DE8DB63617E6")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IMMDeviceEnumerator
    {
        [PreserveSig]
        int EnumAudioEndpoints(AudioDataFlow dataFlow, uint stateMask, out IMMDeviceCollection devices);

        [PreserveSig]
        int GetDefaultAudioEndpoint(AudioDataFlow dataFlow, AudioRole role, out IMMDevice device);

        [PreserveSig]
        int GetDevice([MarshalAs(UnmanagedType.LPWStr)] string id, out IMMDevice device);

        [PreserveSig]
        int RegisterEndpointNotificationCallback(IntPtr client);

        [PreserveSig]
        int UnregisterEndpointNotificationCallback(IntPtr client);
    }

    [ComImport]
    [Guid("0BD7A1BE-7A1A-44DB-8397-CC5392387B5E")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IMMDeviceCollection
    {
        [PreserveSig]
        int GetCount(out uint deviceCount);

        [PreserveSig]
        int Item(uint deviceIndex, out IMMDevice device);
    }

    [ComImport]
    [Guid("D666063F-1587-4E43-81F1-B948E807363F")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IMMDevice
    {
        [PreserveSig]
        int Activate(ref Guid interfaceId, ClsCtx classContext, IntPtr activationParams, [MarshalAs(UnmanagedType.IUnknown)] out object interfaceObject);

        [PreserveSig]
        int OpenPropertyStore(uint storageAccessMode, out IntPtr properties);

        [PreserveSig]
        int GetId(out IntPtr id);

        [PreserveSig]
        int GetState(out uint state);
    }

    [ComImport]
    [Guid("77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IAudioSessionManager2
    {
        [PreserveSig]
        int GetAudioSessionControl(IntPtr sessionGuid, uint streamFlags, out IntPtr sessionControl);

        [PreserveSig]
        int GetSimpleAudioVolume(IntPtr sessionGuid, uint streamFlags, out IntPtr audioVolume);

        [PreserveSig]
        int GetSessionEnumerator(out IAudioSessionEnumerator sessionEnumerator);

        [PreserveSig]
        int RegisterSessionNotification(IntPtr sessionNotification);

        [PreserveSig]
        int UnregisterSessionNotification(IntPtr sessionNotification);

        [PreserveSig]
        int RegisterDuckNotification([MarshalAs(UnmanagedType.LPWStr)] string sessionId, IntPtr duckNotification);

        [PreserveSig]
        int UnregisterDuckNotification(IntPtr duckNotification);
    }

    [ComImport]
    [Guid("E2F5BB11-0570-40CA-ACDD-3AA01277DEE8")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IAudioSessionEnumerator
    {
        [PreserveSig]
        int GetCount(out int sessionCount);

        [PreserveSig]
        int GetSession(int sessionIndex, out IAudioSessionControl sessionControl);
    }

    [ComImport]
    [Guid("F4B1A599-7266-4319-A8CA-E70ACB11E8CD")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IAudioSessionControl
    {
        [PreserveSig]
        int GetState(out int state);

        [PreserveSig]
        int GetDisplayName(out IntPtr displayName);

        [PreserveSig]
        int SetDisplayName([MarshalAs(UnmanagedType.LPWStr)] string displayName, ref Guid eventContext);

        [PreserveSig]
        int GetIconPath(out IntPtr iconPath);

        [PreserveSig]
        int SetIconPath([MarshalAs(UnmanagedType.LPWStr)] string iconPath, ref Guid eventContext);

        [PreserveSig]
        int GetGroupingParam(out Guid groupingId);

        [PreserveSig]
        int SetGroupingParam(ref Guid groupingId, ref Guid eventContext);

        [PreserveSig]
        int RegisterAudioSessionNotification(IntPtr client);

        [PreserveSig]
        int UnregisterAudioSessionNotification(IntPtr client);
    }

    [ComImport]
    [Guid("BFB7FF88-7239-4FC9-8FA2-07C950BE9C6D")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface IAudioSessionControl2
    {
        [PreserveSig]
        int GetState(out int state);

        [PreserveSig]
        int GetDisplayName(out IntPtr displayName);

        [PreserveSig]
        int SetDisplayName([MarshalAs(UnmanagedType.LPWStr)] string displayName, ref Guid eventContext);

        [PreserveSig]
        int GetIconPath(out IntPtr iconPath);

        [PreserveSig]
        int SetIconPath([MarshalAs(UnmanagedType.LPWStr)] string iconPath, ref Guid eventContext);

        [PreserveSig]
        int GetGroupingParam(out Guid groupingId);

        [PreserveSig]
        int SetGroupingParam(ref Guid groupingId, ref Guid eventContext);

        [PreserveSig]
        int RegisterAudioSessionNotification(IntPtr client);

        [PreserveSig]
        int UnregisterAudioSessionNotification(IntPtr client);

        [PreserveSig]
        int GetSessionIdentifier(out IntPtr sessionIdentifier);

        [PreserveSig]
        int GetSessionInstanceIdentifier(out IntPtr sessionInstanceIdentifier);

        [PreserveSig]
        int GetProcessId(out uint processId);

        [PreserveSig]
        int IsSystemSoundsSession();

        [PreserveSig]
        int SetDuckingPreference([MarshalAs(UnmanagedType.Bool)] bool optOut);
    }

    [ComImport]
    [Guid("87CE5498-68D6-44E5-9215-6DA47EF883D8")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    private interface ISimpleAudioVolume
    {
        [PreserveSig]
        int SetMasterVolume(float level, ref Guid eventContext);

        [PreserveSig]
        int GetMasterVolume(out float level);

        [PreserveSig]
        int SetMute([MarshalAs(UnmanagedType.Bool)] bool muted, ref Guid eventContext);

        [PreserveSig]
        int GetMute([MarshalAs(UnmanagedType.Bool)] out bool muted);
    }
}
