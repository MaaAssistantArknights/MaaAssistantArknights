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

using System;
using System.Runtime.InteropServices;

namespace MaaWpfGui.Utilities
{
    public static class SleepManagement
    {
        [DllImport("kernel32.dll")]
        private static extern ExecutionState SetThreadExecutionState(ExecutionState esFlags);

        [Flags]
        private enum ExecutionState : uint
        {
            SystemRequired = 0x01,
            DisplayRequired = 0x02,
            Continuous = 0x80000000,
        }

        public static void AllowSleep()
        {
            SetThreadExecutionState(ExecutionState.Continuous);
        }

        public static void BlockSleep(bool keepDisplayOn = true)
        {
            SetThreadExecutionState(keepDisplayOn ?
            ExecutionState.Continuous | ExecutionState.SystemRequired | ExecutionState.DisplayRequired :
            ExecutionState.Continuous | ExecutionState.SystemRequired);
        }

        public static void ResetIdle(bool keepDisplayOn = true)
        {
            SetThreadExecutionState(keepDisplayOn ?
            ExecutionState.SystemRequired | ExecutionState.DisplayRequired :
            ExecutionState.SystemRequired);
        }
    }
}
