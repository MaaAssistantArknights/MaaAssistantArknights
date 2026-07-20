// <copyright file="ExternalNotificationService.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public static class ExternalNotificationService
{
    private static readonly List<Task> _taskContainers = [];

    private static readonly ILogger _logger = Log.Logger;

    private static async Task SendAsync(string title, string content, bool isTest = false)
    {
        var notificationList = SettingsViewModel.ExternalNotificationSettings.ExternalNotificationConfigs.AsReadOnly();
        foreach (var config in notificationList)
        {
            IExternalNotificationProvider provider = config switch {
                GotifyConfig gotify => new GotifyNotificationProvider(Instances.HttpService, gotify),
                ServerChanConfig serverChan => new ServerChanNotificationProvider(Instances.HttpService, serverChan),
                TelegramConfig telegram => new TelegramNotificationProvider(Instances.HttpService, telegram),
                DiscordConfig discord => new DiscordNotificationProvider(Instances.HttpService, discord),
                DingTalkConfig dingTalk => new DingTalkNotificationProvider(Instances.HttpService, dingTalk),
                CustomWebhookConfig custom => new CustomWebhookNotificationProvider(Instances.HttpService, custom),
                SmtpConfig smtp => new SmtpNotificationProvider(smtp),
                BarkConfig bark => new BarkNotificationProvider(Instances.HttpService, bark),
                QmsgConfig qmsg => new QmsgNotificationProvider(Instances.HttpService, qmsg),
                _ => new DummyNotificationProvider(),
            };

            var result = false;
            try
            {
                result = await provider.SendAsync(title, content);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to send External Notifications");
            }

            if (!isTest && result)
            {
                continue;
            }

            ToastNotification.ShowDirect(
                config.GetType().Name + " " +
                LocalizationHelper.GetString(result ? "ExternalNotificationSendSuccess" : "ExternalNotificationSendFail"));
        }
    }

    /// <summary>
    ///     Send notification
    /// </summary>
    /// <param name="title">The title of the notification</param>
    /// <param name="content">The content of the notification</param>
    /// <param name="isTest">Indicate if it is a test or not.</param>
    public static void Send(string title, string content, bool isTest = false)
    {
        var task = SendAsync("[MAA] " + title, content, isTest);
        _taskContainers.RemoveAll(x => x.Status != TaskStatus.Running);
        _taskContainers.Add(task);
    }

    public static class Event
    {
        public static void AllTaskComplete(string title, string content, string? sanityReport)
        {
            if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationSendWhenComplete)
            {
                var logs = string.Empty;
                if (SettingsViewModel.ExternalNotificationSettings.ExternalNotificationEnableDetails)
                {
                    logs = string.Join("\n", Instances.TaskQueueViewModel.LogItemViewModels.Select(logItem => $"[{logItem.Time}][{logItem.Color}]{logItem.Content}"));
                }
                logs += content;
                if (!string.IsNullOrEmpty(sanityReport))
                {
                    logs += Environment.NewLine + sanityReport;
                }

                Send(title, logs);
            }
        }
    }
}
