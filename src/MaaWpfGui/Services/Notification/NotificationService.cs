// <copyright file="NotificationService.cs" company="MaaAssistantArknights">
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
using System.Collections.ObjectModel;
using System.Windows.Threading;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.ViewModels.Items;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.Services.Notification;

// The only routing entry point for task logs. Producers provide events; transports
// deliver already selected payloads and do not call this service again.
public sealed class NotificationService
{
    private readonly Dictionary<NotificationChannel, NotificationFilter> _filters = new();
    private readonly NotificationHistory _history = new();
    private readonly NotificationSettings.Channel _systemPolicy = NotificationSettings.Channel.CreateDefault(NotificationChannel.SystemNotification);
    private readonly NotificationLogBuffer _taskQueueOverlay = new();
    private readonly NotificationLogBuffer _copilotOverlay = new();
    private readonly DispatcherTimer _trimTimer = new() { Interval = TimeSpan.FromSeconds(30) };

    public NotificationService()
    {
        foreach (var channel in Enum.GetValues<NotificationChannel>())
        {
            _filters.Add(channel, new NotificationFilter());
        }

        _trimTimer.Tick += (_, _) => TrimOverlays();
        _trimTimer.Start();
    }

    public ObservableCollection<LogItemViewModel> TaskQueueOverlay => _taskQueueOverlay.Items;

    public ObservableCollection<LogItemViewModel> CopilotOverlay => _copilotOverlay.Items;

    public void ProcessLog(NotificationEvent notification)
    {
        // All producers already marshal UI work. Keeping history and view
        // collections on one thread eliminates shared locks and ordering races.
        _history.Add(notification);
        var overlayPolicy = SettingsViewModel.NotificationSettings.Overlay.Effective;
        if (_filters[NotificationChannel.Overlay].ShouldSend(overlayPolicy, notification))
        {
            var overlay = notification.Source == NotificationSource.Copilot ? _copilotOverlay : _taskQueueOverlay;
            var item = new LogItemViewModel(notification.Content, notification.Color, notification.Weight, showTime: notification.ShowTime);
            overlay.Add(notification.Timestamp, item);
            overlay.Trim(overlayPolicy, notification.Timestamp);
        }

        if (SettingsViewModel.NotificationSettings.UseNotify
            && _filters[NotificationChannel.SystemNotification].ShouldSend(_systemPolicy, notification))
        {
            using var toast = new ToastNotification(notification.Message?.Title ?? notification.Content);
            if (notification.Message is { } message && message.Content != message.Title)
            {
                toast.AppendContentText(message.Content);
            }

            toast.Show();
        }

        var externalPolicy = SettingsViewModel.NotificationSettings.External.Effective;
        if (_filters[NotificationChannel.External].ShouldSend(externalPolicy, notification))
        {
            ExternalNotificationService.Send(
                notification.Message?.Title ?? notification.Content,
                _history.Bundle(notification, externalPolicy));
        }
    }

    public void Clear(NotificationSource source)
    {
        Execute.OnUIThread(() => {
            _history.Clear(source);
            (source == NotificationSource.Copilot ? _copilotOverlay : _taskQueueOverlay).Clear();
        });
    }

    private void TrimOverlays()
    {
        var policy = SettingsViewModel.NotificationSettings.Overlay.Effective;
        var now = DateTimeOffset.Now;
        _taskQueueOverlay.Trim(policy, now);
        _copilotOverlay.Trim(policy, now);
    }
}
