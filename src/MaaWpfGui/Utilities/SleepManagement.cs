// <copyright file="SleepManagement.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Serilog;

namespace MaaWpfGui.Utilities
{
    public static class SleepManagement
    {
        [DllImport("kernel32.dll")]
        private static extern ExecutionState SetThreadExecutionState(ExecutionState esFlags);

        private static readonly ILogger _logger = Log.ForContext("SourceContext", "SleepManagement");

        private static bool _allowBlockSleep = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleep, bool.FalseString));
        private static bool _blockSleepWithScreenOn = Convert.ToBoolean(ConfigurationHelper.GetValue(ConfigurationKeys.BlockSleepWithScreenOn, bool.TrueString));
        private static bool _isBlockingSleep = false;

        public static void SetBlockSleep(bool allowBlockSleep)
        {
            _allowBlockSleep = allowBlockSleep;
        }

        public static void SetBlockSleepWithScreenOn(bool blockSleepWithScreenOn)
        {
            _blockSleepWithScreenOn = blockSleepWithScreenOn;
        }

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
            if (!(allowBlockSleep ?? _allowBlockSleep))
            {
                return;
            }

            _isBlockingSleep = true;

            bool keepDisplayOn = blockSleepWithScreenOn ?? _blockSleepWithScreenOn;
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
}
