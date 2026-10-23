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
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using MaaWpfGui.ViewModels.UserControl.Settings;
using Stylet;

namespace MaaWpfGui.Services.Notification;

// The only routing entry point for task logs. Producers provide events; transports
// deliver already selected payloads and do not call this service again.
public sealed class NotificationService
{
    private readonly Dictionary<Guid, NotificationTrigger> _triggers = [];
    private readonly NotificationHistory _history = new();

    public NotificationService() => RunningState.Instance.StallOccurred += OnStalled;

    private void OnStalled(RunOwner owner, int initialMinutes, int accumulatedMinutes)
    {
        Execute.OnUIThread(() => {
            // The run may have ended while this timer callback waited for the UI thread.
            var state = RunningState.Instance;
            if (state.GetIdle() || state.Owner != owner || !state.EnableStallTimeout || state.StallTimeoutMinutes <= 0)
            {
                return;
            }

            var message = LocalizationHelper.GetStringFormat("TaskQueue.Stall.Warning", initialMinutes, accumulatedMinutes);
            Notify(owner == RunOwner.Copilot ? NotificationSource.Copilot : NotificationSource.TaskQueue,
                new(NotificationKind.Stalled, message, message), UiLogColor.Warning);
        });
    }

    public void PublishLog(NotificationSource source, string? content, Action display,
        string color = UiLogColor.Trace) => Publish(source, content, display, color);

    public void Notify(NotificationSource source, NotificationMessage message,
        string color = UiLogColor.Trace, string? logContent = null, Action? display = null)
    {
        if (message.Kind != NotificationKind.Stalled)
        {
            RunningState.Instance.NotifyOutputActivity();
        }

        var content = logContent ?? message.Content;
        Publish(source, content, display ?? (() => {
            if (source == NotificationSource.Copilot)
            {
                Instances.CopilotViewModel.DisplayLog(content, color);
            }
            else
            {
                Instances.TaskQueueViewModel.DisplayLog(content, color);
            }
        }), color, message: message);
    }

    public void TestSystemNotification()
    {
        ToastNotification.ShowDirect(LocalizationHelper.GetString("Toast.Test"));
        var (available, detail) = ToastNotification.ToastNotificationCheck();
        if (!available)
        {
            GrowlHelper.Error(LocalizationHelper.GetStringFormat("Toast.Unavailable", detail));
        }
    }

    public void ResetChannel(Guid id) => _triggers.Remove(id);

    public void ResetAllChannels() => _triggers.Clear();

    public void TestExternalNotification(Guid? id = null) => Execute.OnUIThread(() => {
        if (!SettingsViewModel.ExternalNotificationSettings.Enable)
        {
            return;
        }
        foreach (var channel in Channels.Where(item => item.DeliverySettings.Enable && (id is null || item.Id == id)))
        {
            SendExternal(channel, LocalizationHelper.GetString("ExternalNotification.SendTest.Title"),
                LocalizationHelper.GetString("ExternalNotification.SendTest.Content"), true);
        }
    });

    public void NotifyScheduledStart(string title, string content) => Execute.OnUIThread(() => {
        if (!SettingsViewModel.ExternalNotificationSettings.Enable)
        {
            return;
        }
        var notification = new NotificationEvent(DateTimeOffset.Now, NotificationSource.TaskQueue, content, UiLogColor.Trace);
        foreach (var channel in Channels.Where(item => item.DeliverySettings.Enable && item.DeliverySettings.SendBeforeScheduledStart))
        {
            ResetChannel(channel.Id);
            SendExternal(channel, title, _history.Bundle(notification, channel.Config.Delivery));
        }
    });

    private static NotificationChannelViewModel[] Channels => SettingsViewModel.ExternalNotificationSettings.ExternalNotificationConfigs.ToArray();

    private void Publish(NotificationSource source, string? content, Action display,
        string color, NotificationMessage? message = null)
    {
        Execute.OnUIThread(() => {
            if (!string.IsNullOrEmpty(content))
            {
                ProcessLog(new(DateTimeOffset.Now, source, content, color, message));
            }

            display();
        });
    }

    private void ProcessLog(NotificationEvent notification)
    {
        // Publication owns UI dispatch; a display callback never republishes the event.
        _history.Add(notification);
        if (SettingsViewModel.NotificationSettings.UseNotify
            && notification.Message?.Kind is NotificationKind.TaskError or NotificationKind.TaskComplete or NotificationKind.Stalled)
        {
            using var toast = new ToastNotification(notification.Message?.Title ?? notification.Content);
            if (notification.Message is { } message && message.Content != message.Title)
            {
                toast.AppendContentText(message.Content);
            }

            toast.Show();
        }

        if (!SettingsViewModel.ExternalNotificationSettings.Enable)
        {
            return;
        }
        foreach (var channel in Channels)
        {
            if (!_triggers.TryGetValue(channel.Id, out var trigger))
            {
                trigger = new NotificationTrigger();
                _triggers.Add(channel.Id, trigger);
            }
            if (trigger.ShouldSend(channel.Config.Delivery, notification, true))
            {
                trigger.Reset();
                SendExternal(channel, notification.Message?.Title ?? notification.Content,
                    _history.Bundle(notification, channel.Config.Delivery));
            }
        }
    }

    private static void SendExternal(NotificationChannelViewModel channel, string title, string content, bool isTest = false)
    {
        var descriptor = ExternalNotificationChannel.ForEditor(channel.Provider)!;
        var snapshot = descriptor.ReadConfig(channel.Provider.ToConfig());
        _ = ExternalNotificationService.SendAsync(snapshot, channel.DisplayName, title, content, isTest);
    }

    public void Clear(NotificationSource source) => Execute.OnUIThread(() => _history.Clear(source));
}
