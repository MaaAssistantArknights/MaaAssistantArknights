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
using System.Threading;
using MaaWpfGui.Constants;
using MaaWpfGui.Constants.Enums;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.ExternalNotification;
using MaaWpfGui.States;
using MaaWpfGui.ViewModels.UI;
using Stylet;
using ExternalNotificationDeliverySettings = MaaWpfGui.Configuration.Single.Settings.ExternalNotification.DeliverySettings;

namespace MaaWpfGui.Services.Notification;

// The only routing entry point for task logs. Producers provide events; transports
// deliver already selected payloads and do not call this service again.
public sealed class NotificationService
{
    private readonly NotificationTrigger _externalTrigger = new();
    private readonly NotificationHistory _history = new();
    private int _suppressErrorLog;

    public NotificationService() => RunningState.Instance.StallOccurred += OnStalled;

    /// <summary>
    /// 任务通知发布事件。订阅方（如企业微信机器人）自行决定转发策略，不得再次调用本服务。
    /// </summary>
    public event Action<NotificationSource, NotificationMessage>? MessagePublished;

    /// <summary>
    /// 错误级日志发布事件（仅界面日志，不含已经由 <see cref="Notify"/> 发出的任务错误，避免重复）。
    /// </summary>
    public event Action<NotificationSource, string>? ErrorLogPublished;

    private void OnStalled(RunOwner owner, int initialMinutes, int accumulatedMinutes)
    {
        Execute.OnUIThread(() => {
            // The run may have ended while this timer callback waited for the UI thread.
            var state = RunningState.Instance;
            if (state.GetIdle() || state.Owner != owner || !state.EnableStallTimeout || state.StallTimeoutMinutes <= 0)
            {
                return;
            }

            var message = LocalizationHelper.GetStringFormat("TaskStallWarning", initialMinutes, accumulatedMinutes);
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
        ToastNotification.ShowDirect(LocalizationHelper.GetString("ToastNotificationTest"));
        var (available, detail) = ToastNotification.ToastNotificationCheck();
        if (!available)
        {
            GrowlHelper.Error(LocalizationHelper.GetStringFormat("ToastNotificationUnavailable", detail));
        }
    }

    public void TestExternalNotification() => Execute.OnUIThread(() => {
        if (!CanSendExternal)
        {
            return;
        }

        SendExternal(LocalizationHelper.GetString("ExternalNotificationSendTestTitle"),
            LocalizationHelper.GetString("ExternalNotificationSendTestContent"), true);
    });

    public void NotifyScheduledStart(string title, string content) => Execute.OnUIThread(() => {
        var policy = ExternalPolicy;
        if (!CanSendExternal || !policy.SendBeforeScheduledStart)
        {
            return;
        }

        // A scheduled notification is not a new displayed log entry.
        var notification = new NotificationEvent(DateTimeOffset.Now, NotificationSource.TaskQueue, content, UiLogColor.Trace);
        SendExternal(title, _history.Bundle(notification, policy));
    });

    private static ExternalNotificationDeliverySettings ExternalPolicy => SettingsViewModel.ExternalNotificationSettings.DeliverySettings.Settings;

    private static bool CanSendExternal => ExternalPolicy.Enable && SettingsViewModel.ExternalNotificationSettings.ConfigCount > 0;

    private void Publish(NotificationSource source, string? content, Action display,
        string color, NotificationMessage? message = null)
    {
        Execute.OnUIThread(() => {
            if (!string.IsNullOrEmpty(content))
            {
                ProcessLog(new(DateTimeOffset.Now, source, content, color, message));
            }

            var suppress = message is not null;
            if (suppress)
            {
                Interlocked.Increment(ref _suppressErrorLog);
            }

            try
            {
                display();
            }
            finally
            {
                if (suppress)
                {
                    Interlocked.Decrement(ref _suppressErrorLog);
                }
            }

            if (message is not null)
            {
                MessagePublished?.Invoke(source, message);
            }
        });
    }

    /// <summary>
    /// 由共享 UI 日志入口调用，转发直接写入界面的错误日志。
    /// </summary>
    /// <param name="content">错误日志正文。</param>
    public void PublishUiError(string content)
    {
        if (Volatile.Read(ref _suppressErrorLog) == 0 && !string.IsNullOrWhiteSpace(content))
        {
            ErrorLogPublished?.Invoke(NotificationSource.TaskQueue, content);
        }
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

        var externalPolicy = ExternalPolicy;
        if (_externalTrigger.ShouldSend(externalPolicy, notification, SettingsViewModel.ExternalNotificationSettings.ConfigCount > 0))
        {
            SendExternal(
                notification.Message?.Title ?? notification.Content,
                _history.Bundle(notification, externalPolicy));
        }
    }

    private void SendExternal(string title, string content, bool isTest = false)
    {
        _externalTrigger.Reset();
        ExternalNotificationService.Send(title, content, isTest);
    }

    public void Clear(NotificationSource source) => Execute.OnUIThread(() => _history.Clear(source));
}
