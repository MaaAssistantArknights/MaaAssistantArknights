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
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Stylet;

namespace MaaWpfGui.Services.Notification;

// The only routing entry point for task logs. Producers provide events; transports
// deliver already selected payloads and do not call this service again.
public sealed class NotificationService
{
    private readonly NotificationFilter _externalFilter = new();
    private readonly NotificationHistory _history = new();

    public NotificationService() => RunningState.Instance.StallOccurred += OnStalled;

    private void OnStalled(RunOwner owner, int initialMinutes, int accumulatedMinutes)
    {
        Execute.OnUIThread(() => {
            var message = LocalizationHelper.GetStringFormat("TaskStallWarning", initialMinutes, accumulatedMinutes);
            Notify(owner == RunOwner.Copilot ? NotificationSource.Copilot : NotificationSource.TaskQueue,
                new(NotificationTag.Stalled, message, message), UiLogColor.Warning);
        });
    }

    public void PublishLog(NotificationSource source, string? content, Action display,
        string color = UiLogColor.Trace) => Publish(source, content, display, color);

    public void Notify(NotificationSource source, NotificationMessage message,
        string color = UiLogColor.Trace, string? logContent = null, Action? display = null)
    {
        if (message.Tag != NotificationTag.Stalled)
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
        ToastNotification.ShowDirect(LocalizationHelper.GetString("ToastNotificationTest"));
        var (available, detail) = ToastNotification.ToastNotificationCheck();
        if (!available)
        {
            HandyControl.Controls.Growl.Error(LocalizationHelper.GetStringFormat("ToastNotificationUnavailable", detail));
        }
    }

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
            && notification.Message?.Tag is NotificationTag.TaskError or NotificationTag.TaskComplete or NotificationTag.Test)
        {
            using var toast = new ToastNotification(notification.Message?.Title ?? notification.Content);
            if (notification.Message is { } message && message.Content != message.Title)
            {
                toast.AppendContentText(message.Content);
            }

            toast.Show();
        }

        var externalPolicy = SettingsViewModel.ExternalNotificationSettings.ContentSettings.Effective;
        if (_externalFilter.ShouldSend(externalPolicy, notification))
        {
            ExternalNotificationService.Send(
                notification.Message?.Title ?? notification.Content,
                _history.Bundle(notification, externalPolicy));
        }
    }

    public void Clear(NotificationSource source) => Execute.OnUIThread(() => _history.Clear(source));
}
