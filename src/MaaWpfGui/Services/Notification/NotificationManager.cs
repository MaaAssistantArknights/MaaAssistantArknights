// <copyright file="NotificationManager.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text.RegularExpressions;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UserControl.Settings;

namespace MaaWpfGui.Services.Notification;

public enum NotificationType
{
    Overlay,
    SystemNotification,
    External,
    TaskQueueLog,
}

public static class NotificationManager
{
    private static NotificationSettingsUserControlModel Settings => NotificationSettingsUserControlModel.Instance;

    public static readonly ObservableCollection<LogItemViewModel> OverlayLogItems = [];

    public static readonly ObservableCollection<LogItemViewModel> FilteredTaskQueueLogItems = [];

    private static readonly Dictionary<LogItemViewModel, DateTime> _itemTimestamps = [];

    private static readonly object _lock = new();

    private static readonly Dictionary<NotificationType, DefaultProfile> _defaultProfiles = new()
    {
        [NotificationType.TaskQueueLog] = new DefaultProfile
        {
            Enable = true,
            EnableBlacklist = false,
            EnableWhitelist = false,
            FilterList = string.Empty,
        },
        [NotificationType.Overlay] = new DefaultProfile
        {
            Enable = true,
            EnableBlacklist = false,
            EnableWhitelist = false,
            FilterList = string.Empty,
            MaxEntries = 100,
            TimeMinutes = 60,
        },
        [NotificationType.SystemNotification] = new DefaultProfile
        {
            Enable = true,
            EnableBlacklist = false,
            EnableWhitelist = true,
            FilterList = @"\[TaskError\]|\[TaskComplete\]|\[Test\]",
        },
        [NotificationType.External] = new DefaultProfile
        {
            Enable = true,
            EnableBlacklist = false,
            EnableWhitelist = true,
            FilterList = @"\[TaskError\]|\[TaskComplete\]|\[Stalled\]",
            MaxEntries = 100,
            TimeMinutes = 60,
        },
    };

    private record DefaultProfile
    {
        public bool Enable { get; init; } = true;
        public bool EnableBlacklist { get; init; }
        public bool EnableWhitelist { get; init; }
        public string FilterList { get; init; } = string.Empty;
        public int MaxEntries { get; init; } = 100;
        public int TimeMinutes { get; init; } = 60;
    }

    /// <summary>
    /// Check whether the given notification channel should show this content.
    /// </summary>
    public static bool ShouldShow(NotificationType type, string content, string? tag = null)
    {
        var settings = GetSettings(type);
        bool useIndependent = settings.UseIndependent;

        bool enable;
        bool enableBlacklist;
        bool enableWhitelist;
        string filterList;

        if (useIndependent)
        {
            enable = settings.Enable;
            enableBlacklist = settings.EnableBlacklist;
            enableWhitelist = settings.EnableWhitelist;
            filterList = settings.FilterList;
        }
        else
        {
            var def = _defaultProfiles[type];
            enable = def.Enable;
            enableBlacklist = def.EnableBlacklist;
            enableWhitelist = def.EnableWhitelist;
            filterList = def.FilterList;
        }

        if (!enable)
        {
            return false;
        }

        if (!enableBlacklist && !enableWhitelist)
        {
            return true;
        }

        if (string.IsNullOrWhiteSpace(filterList))
        {
            return true;
        }

        var lines = filterList.Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries);
        bool anyMatch = lines.Any(line =>
        {
            try
            {
                return Regex.IsMatch(content, line.Trim());
            }
            catch (RegexParseException)
            {
                return false;
            }
        });

        if (enableBlacklist)
        {
            return !anyMatch;
        }

        if (enableWhitelist)
        {
            return anyMatch;
        }

        return true;
    }

    /// <summary>
    /// Get the effective MaxEntries (from user config or default).
    /// </summary>
    public static int GetMaxEntries(NotificationType type)
    {
        var settings = GetSettings(type);
        if (settings.UseIndependent)
        {
            return settings.MaxEntries;
        }

        return _defaultProfiles.TryGetValue(type, out var def) ? def.MaxEntries : 100;
    }

    /// <summary>
    /// Get the effective TimeMinutes (from user config or default).
    /// </summary>
    public static int GetTimeMinutes(NotificationType type)
    {
        var settings = GetSettings(type);
        if (settings.UseIndependent)
        {
            return settings.TimeMinutes;
        }

        return _defaultProfiles.TryGetValue(type, out var def) ? def.TimeMinutes : 60;
    }

    /// <summary>
    /// Called from AddLog to federate a log item to all channels.
    /// </summary>
    public static void ProcessLog(LogItemViewModel logItem, string? notificationTag)
    {
        var content = notificationTag != null ? $"[{notificationTag}] {logItem.Content}" : logItem.Content;
        var now = DateTime.Now;

        lock (_lock)
        {
            _itemTimestamps[logItem] = now;

            if (ShouldShow(NotificationType.Overlay, content, notificationTag))
            {
                OverlayLogItems.Add(logItem);
                TrimCollection(OverlayLogItems, NotificationType.Overlay);
            }

            if (ShouldShow(NotificationType.TaskQueueLog, content, notificationTag))
            {
                FilteredTaskQueueLogItems.Add(logItem);
                TrimCollection(FilteredTaskQueueLogItems, NotificationType.TaskQueueLog);
            }
        }
    }

    /// <summary>
    /// Trim a collection to respect MaxEntries and TimeMinutes limits.
    /// Remove oldest items when exceeding the limit, and items older than the time window.
    /// </summary>
    public static void TrimCollection(ObservableCollection<LogItemViewModel> collection, NotificationType type)
    {
        var maxEntries = GetMaxEntries(type);
        var timeMinutes = GetTimeMinutes(type);

        if (maxEntries <= 0)
        {
            maxEntries = 100;
        }

        if (timeMinutes <= 0)
        {
            timeMinutes = 60;
        }

        var cutoff = DateTime.Now.AddMinutes(-timeMinutes);

        // 按时间顺序排列（最早的在前面），移除超出时间窗口的
        var ordered = collection
            .Select((item, idx) => new { Item = item, Time = _itemTimestamps.GetValueOrDefault(item, DateTime.MinValue) })
            .OrderBy(x => x.Time)
            .ToList();

        // 从前面移除超出时间和数量的条目
        int removeCount = Math.Max(0, ordered.Count - maxEntries);

        for (int i = 0; i < ordered.Count; i++)
        {
            bool expired = ordered[i].Time < cutoff;
            if (expired || i < removeCount)
            {
                collection.Remove(ordered[i].Item);
                _itemTimestamps.Remove(ordered[i].Item);
            }
        }
    }

    /// <summary>
    /// Get the default filter list for display (what would be in effect without independent settings).
    /// </summary>
    public static string GetDefaultFilterList(NotificationType type) =>
        _defaultProfiles.TryGetValue(type, out var def) ? def.FilterList : string.Empty;

    public static bool GetDefaultEnableBlacklist(NotificationType type) =>
        _defaultProfiles.TryGetValue(type, out var def) && def.EnableBlacklist;

    public static bool GetDefaultEnableWhitelist(NotificationType type) =>
        _defaultProfiles.TryGetValue(type, out var def) && def.EnableWhitelist;

    public static int GetDefaultMaxEntries(NotificationType type) =>
        _defaultProfiles.TryGetValue(type, out var def) ? def.MaxEntries : 100;

    public static int GetDefaultTimeMinutes(NotificationType type) =>
        _defaultProfiles.TryGetValue(type, out var def) ? def.TimeMinutes : 60;

    public static NotificationSettingsItem GetSettings(NotificationType type) => type switch
    {
        NotificationType.Overlay => Settings.Overlay,
        NotificationType.SystemNotification => Settings.SystemNotification,
        NotificationType.External => Settings.External,
        NotificationType.TaskQueueLog => Settings.TaskQueueLog,
        _ => Settings.Overlay,
    };

    public static void Clear()
    {
        OverlayLogItems.Clear();
        FilteredTaskQueueLogItems.Clear();
        lock (_lock)
        {
            _itemTimestamps.Clear();
        }
    }
}
