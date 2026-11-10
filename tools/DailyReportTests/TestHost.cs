// <copyright file="TestHost.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Configuration.Global;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Models;

// These hosts replace only GUI/Core boundaries. The collector, store, account model,
// report models, JSON serialization and file I/O are the linked production sources.
namespace MaaWpfGui.Models
{
    public class NotifyPropertyChangedWithValue;
}

namespace MaaWpfGui.Main
{
    public enum AsstMsg
    {
        SubTaskExtraInfo,
        SubTaskStart,
    }

    public static class Bootstrapper
    {
        public static bool IsDemoMode => false;
    }

    public static class Instances
    {
        public static AsstProxy AsstProxy { get; } = new();
    }

    public sealed class AsstProxy
    {
        public enum TaskType
        {
            Fight,
            StartUp,
        }

        public event Action<AsstMsg, AsstSubTaskMsg?>? AsstSubTaskMsgEvent;

        public event Action<int, TaskItemStatus>? OnTaskStatusChanged;

        public Dictionary<int, (TaskType Type, TaskItemStatus Status)> TasksStatus { get; } = [];

        public void SetTaskStatus(int id, TaskType type, TaskItemStatus status)
        {
            TasksStatus[id] = (type, status);
            OnTaskStatusChanged?.Invoke(id, status);
        }

        public void Publish(AsstMsg type, AsstSubTaskMsg? message)
        {
            AsstSubTaskMsgEvent?.Invoke(type, message);
        }
    }
}

namespace MaaWpfGui.Configuration.Factory
{
    public static class ConfigFactory
    {
        public static TestRoot Root { get; } = new();
    }

    public sealed class TestRoot
    {
        public TestGuiSettings Gui { get; } = new();
    }

    public sealed class TestGuiSettings
    {
        public DailyReportRetention DailyReportRetention { get; set; } = DailyReportRetention.Unlimited;

        public List<DailyReportAccount> DailyReportAccounts { get; set; } = [];

        public string? DailyReportAccountId { get; set; }
    }
}

namespace MaaWpfGui.ViewModels.UI
{
    public static class SettingsViewModel
    {
        public static Configuration.Factory.TestGuiSettings GuiSettings => Configuration.Factory.ConfigFactory.Root.Gui;
    }
}

namespace MaaWpfGui.Extensions
{
    public static class DateTimeExtension
    {
        // A different game day isolates each case without resetting collector internals.
        public static DateTime GameDay { get; set; } = new(2030, 1, 1);

        public static DateTime ToYjDate(this DateTime value) => GameDay;
    }
}

namespace MaaWpfGui.Helper
{
    public static class PathsHelper
    {
        public static string DataDir { get; } = Path.Combine(Path.GetTempPath(), "MaaDailyReportTests-" + Guid.NewGuid().ToString("N"));
    }
}
