// <copyright file="InfrastDronesUsageTests.cs" company="MaaAssistantArknights">
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

using System;
using System.Reflection;
using MaaWpfGui.Main;
using Newtonsoft.Json.Linq;

internal static class InfrastDronesUsageTests
{
    private static readonly MethodInfo _onExtraInfo = typeof(AsstProxy).GetMethod("ProcSubTaskExtraInfo", BindingFlags.NonPublic | BindingFlags.Static)!;
    private static readonly MethodInfo _takeReminder = typeof(AsstProxy).GetMethod("TakeTradeDronesUsageReminder", BindingFlags.NonPublic | BindingFlags.Static)!;

    [STAThread]
    private static void Main()
    {
        Require(!Take(1), "No reminder without a mismatch");
        Record(1);
        Record(1);
        Record(2);
        Require(Take(1, 2), "Multiple tasks and repeated callbacks produce one reminder");
        Require(!Take(1, 2), "All matching tasks must be consumed, not just the first");

        Record(3);
        Record(4);
        Require(Take(3), "A matching completed task triggers the reminder");
        Require(!Take(4), "Unmatched old task IDs must not leak into another run");

        Record(5);
        Require(!Take(6), "Unrelated completed tasks do not trigger reminders");
        Require(!Take(5), "Discard stale reminders even when no task matches");

        Record(7);
        Require(!(bool)_takeReminder.Invoke(null, new object?[] { null })!, "Missing completion data does not trigger reminders");
        Require(!Take(7), "Missing completion data still clears pending reminders");
        Console.WriteLine("Drone reminder lifecycle checks passed.");
    }

    private static void Record(int taskId)
        => _onExtraInfo.Invoke(null, new object[] { new JObject { ["taskchain"] = "Infrast", ["taskid"] = taskId, ["what"] = "TradeDronesUsageNotUsed" } });

    private static bool Take(params int[] taskIds)
        => (bool)_takeReminder.Invoke(null, new object[] { taskIds })!;

    private static void Require(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }
}
