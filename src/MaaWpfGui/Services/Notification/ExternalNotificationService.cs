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

using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification
{
    public static class ExternalNotificationService
    {
        private static readonly List<Task> _taskContainers = new List<Task>();

        private static readonly ILogger _logger = Log.Logger;

        private static async Task SendAsync(string title, string content, bool isTest = false)
        {
            var enabledProviders = SettingsViewModel.ExternalNotificationSettings.EnabledExternalNotificationProviderList;

            foreach (var enabledProvider in enabledProviders)
            {
                IExternalNotificationProvider provider = enabledProvider switch
                {
                    "ServerChan" => new ServerChanNotificationProvider(Instances.HttpService),
                    "Telegram" => new TelegramNotificationProvider(Instances.HttpService),
                    "Discord" => new DiscordNotificationProvider(Instances.HttpService),
                    "Discord Webhook" => new DiscordWebhookNotificationProvider(Instances.HttpService),
                    "Custom Webhook" => new CustomWebhookNotificationProvider(Instances.HttpService),
                    "SMTP" => new SmtpNotificationProvider(),
                    "Bark" => new BarkNotificationProvider(Instances.HttpService),
                    "Qmsg" => new QmsgNotificationProvider(Instances.HttpService),
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

                if (isTest is false && result)
                {
                    return;
                }

                ToastNotification.ShowDirect(
                    enabledProvider + " " +
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
            var task = SendAsync(title, content, isTest);
            _taskContainers.RemoveAll(x => x.Status != TaskStatus.Running);
            _taskContainers.Add(task);
        }
    }
}
