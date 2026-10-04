// <copyright file="ExternalNotificationService.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.ExternalNotification;

public static class ExternalNotificationService
{
    private static readonly ILogger _logger = Log.ForContext(typeof(ExternalNotificationService));

    private static async Task SendAsync(string title, string content, BaseConfig[] notificationList, bool isTest)
    {
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
            _logger.Debug("Sending external notification via {Provider} (test: {IsTest}, title length: {TitleLength}, content length: {ContentLength})",
                config.GetType().Name, isTest, title.Length, content.Length);
            try
            {
                result = await provider.SendAsync(title, content).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Failed to send External Notifications");
            }

            _logger.Information("External notification provider {Provider} returned {Success}", config.GetType().Name, result);
            if (!isTest && result)
            {
                continue;
            }

            await Stylet.Execute.OnUIThreadAsync(() => {
                // 渠道显示名与设置页各渠道卡片标题一致：品牌名无需本地化，自定义 Webhook 用本地化 key
                var providerName = config switch {
                    ServerChanConfig => "Server Chan",
                    TelegramConfig => "Telegram",
                    DiscordConfig => "Discord",
                    DingTalkConfig => "DingTalk",
                    SmtpConfig => "SMTP",
                    BarkConfig => "Bark",
                    QmsgConfig => "Qmsg",
                    GotifyConfig => "Gotify",
                    CustomWebhookConfig => LocalizationHelper.GetString("ExternalNotificationCustomWebhook"),
                    _ => config.GetType().Name,
                };

                ToastNotification.ShowDirect(
                    providerName + " " +
                    LocalizationHelper.GetString(result ? "ExternalNotificationSendSuccess" : "ExternalNotificationSendFail"));
            }).ConfigureAwait(false);
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
        // Snapshot on the UI thread before asynchronous provider delivery.
        var configurations = SettingsViewModel.ExternalNotificationSettings.ExternalNotificationConfigs.ToArray();
        _ = SendAsync("[MAA] " + title, content, configurations, isTest);
    }
}
