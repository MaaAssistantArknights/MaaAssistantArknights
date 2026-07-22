// <copyright file="SleepManagement.cs" company="MaaAssistantArknights">
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
using System.Runtime.InteropServices;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Serilog;

namespace MaaWpfGui.Utilities;

public static class SleepManagement
{
    [DllImport("kernel32.dll")]
    private static extern ExecutionState SetThreadExecutionState(ExecutionState esFlags);

    private static readonly ILogger _logger = Log.ForContext("SourceContext", "SleepManagement");
    private static bool _isBlockingSleep = false;

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
        if (!(allowBlockSleep ?? ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.BlockSleep))
        {
            return;
        }

        _isBlockingSleep = true;

        bool keepDisplayOn = blockSleepWithScreenOn ?? ConfigFactory.CurrentConfig.WpfSettings.RuntimeSettings.BlockSleepWithScreenOn;
        _logger.Information("Blocking system from sleeping");
        ExecutionState state = ExecutionState.Continuous | ExecutionState.SystemRequired |
            (keepDisplayOn ? ExecutionState.DisplayRequired : 0);
        SetThreadExecutionState(state);
    }

    public static void ResetIdle(bool keepDisplayOn = true)
    {
        ExecutionState state = ExecutionState.SystemRequired |
            (keepDisplayOn ? ExecutionState.DisplayRequired : 0);
        SetThreadExecutionState(state);
    }
}
