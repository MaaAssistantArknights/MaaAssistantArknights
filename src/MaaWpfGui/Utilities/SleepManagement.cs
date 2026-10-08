// <copyright file="SleepManagement.cs" company="MaaAssistantArknights">
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
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using MaaWpfGui.Configuration.Factory;
using Serilog;

namespace MaaWpfGui.Utilities;

public static class SleepManagement
{
    [DllImport("kernel32.dll")]
    private static extern ExecutionState SetThreadExecutionState(ExecutionState esFlags);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern nint PowerCreateRequest(ref PowerRequestContextSimple context);

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool PowerSetRequest(nint powerRequest, PowerRequestType requestType);

    [DllImport("kernel32.dll")]
    private static extern bool PowerClearRequest(nint powerRequest, PowerRequestType requestType);

    [DllImport("kernel32.dll")]
    private static extern bool CloseHandle(nint hObject);

    private enum PowerRequestType
    {
        PowerRequestSystemRequired = 1,
    }

    // 用户态 REASON_CONTEXT 的 SIMPLE 分支：SimpleReasonString 就是普通 LPWSTR，
    // 不是内核态 COUNTED_REASON_CONTEXT 的 UNICODE_STRING——写成 UNICODE_STRING 布局
    // 会让系统把 Length/MaximumLength 字节当指针读，PowerCreateRequest 立即访问冲突崩溃
    [StructLayout(LayoutKind.Sequential)]
    private struct PowerRequestContextSimple
    {
        public uint Version;

        public uint Flags;

        [MarshalAs(UnmanagedType.LPWStr)]
        public string SimpleReasonString;
    }

    private static readonly ILogger _logger = Log.ForContext("SourceContext", "SleepManagement");
    private static bool _isBlockingSleep = false;
    private static readonly Lock _keepAwakeLock = new();
    private static int _keepAwakeGeneration;
    private static nint _keepAwakePowerRequest;

    [Flags]
    private enum ExecutionState : uint
    {
        SystemRequired = 0x01,
        DisplayRequired = 0x02,
        Continuous = 0x80000000,
    }

    public static void AllowSleep()
    {
        if (!_isBlockingSleep)
        {
            return;
        }

        _isBlockingSleep = false;

        _logger.Information("Allowing system to sleep");
        SetThreadExecutionState(ExecutionState.Continuous);
    }

    public static void BlockSleep(bool? allowBlockSleep = null, bool? blockSleepWithScreenOn = null)
    {
        if (!(allowBlockSleep ?? ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleep))
        {
            return;
        }

        _isBlockingSleep = true;

        bool keepDisplayOn = blockSleepWithScreenOn ?? ConfigFactory.CurrentConfig.Gui.RuntimeSettings.BlockSleepWithScreenOn;
        _logger.Information("Blocking system from sleeping");
        ExecutionState state = ExecutionState.Continuous | ExecutionState.SystemRequired |
            (keepDisplayOn ? ExecutionState.DisplayRequired : 0);
        SetThreadExecutionState(state);
    }

    /// <summary>
    /// 在指定时长内保持系统唤醒，到期自动失效；窗口未结束时再次调用会以新时长重新开窗。
    /// 走 PowerRequest 而非线程执行状态：进程级对象，与 <see cref="BlockSleep"/> 的
    /// SetThreadExecutionState 通道互不干扰，也不受调用线程影响，进程退出时由系统回收。
    /// </summary>
    /// <param name="duration">保持唤醒的时长。</param>
    public static void KeepAwakeFor(TimeSpan duration)
    {
        lock (_keepAwakeLock)
        {
            nint previous = _keepAwakePowerRequest;
            if (previous != 0)
            {
                _keepAwakePowerRequest = 0;
                PowerClearRequest(previous, PowerRequestType.PowerRequestSystemRequired);
                CloseHandle(previous);
            }

            int generation = ++_keepAwakeGeneration;

            var context = new PowerRequestContextSimple
            {
                Version = 0,
                Flags = 1,
                SimpleReasonString = "MAA keep awake after scheduled wake-up",
            };
            nint request = PowerCreateRequest(ref context);
            if (request == 0 || request == -1)
            {
                _logger.Warning("PowerCreateRequest failed: {ErrorCode}", Marshal.GetLastWin32Error());
                return;
            }

            if (!PowerSetRequest(request, PowerRequestType.PowerRequestSystemRequired))
            {
                _logger.Warning("PowerSetRequest failed: {ErrorCode}", Marshal.GetLastWin32Error());
                PowerClearRequest(request, PowerRequestType.PowerRequestSystemRequired);
                CloseHandle(request);
                return;
            }

            _keepAwakePowerRequest = request;
            _logger.Information("Keeping system awake for {Duration}", duration);

            _ = Task.Delay(duration).ContinueWith(_ => ClearKeepAwakeRequest(generation), TaskScheduler.Default);
        }
    }

    private static void ClearKeepAwakeRequest(int generation)
    {
        lock (_keepAwakeLock)
        {
            nint request = _keepAwakePowerRequest;
            if (request == 0 || generation != _keepAwakeGeneration)
            {
                return;
            }

            _keepAwakePowerRequest = 0;
            PowerClearRequest(request, PowerRequestType.PowerRequestSystemRequired);
            CloseHandle(request);
            _logger.Information("Keep-awake window expired");
        }
    }

    public static void ResetIdle(bool keepDisplayOn = true)
    {
        ExecutionState state = ExecutionState.SystemRequired |
            (keepDisplayOn ? ExecutionState.DisplayRequired : 0);
        SetThreadExecutionState(state);
    }
}
