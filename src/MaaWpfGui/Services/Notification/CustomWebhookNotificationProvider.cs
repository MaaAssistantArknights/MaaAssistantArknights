// <copyright file="CustomWebhookNotificationProvider.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using System.Text;
using System.Threading.Tasks;
using System.Net.Http;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class CustomWebhookNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<CustomWebhookNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var webhookUrl = SettingsViewModel.ExternalNotificationSettings.CustomWebhookUrl;
        var bodyTemplate = SettingsViewModel.ExternalNotificationSettings.CustomWebhookBody;
        if (string.IsNullOrEmpty(webhookUrl) || string.IsNullOrEmpty(bodyTemplate))
        {
            _logger.Warning("自定义Webhook发送失败，URL或消息体为空");
            return false;
        }

        // 占位符替换
        string now = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
        string body = bodyTemplate
            .Replace("{title}", title)
            .Replace("{content}", content)
            .Replace("{time}", now);

        var requestContent = new StringContent(body, Encoding.UTF8, "application/json");
        var response = await httpService.PostAsync(new Uri(webhookUrl), requestContent, null);

        if (response == null)
        {
            _logger.Warning("自定义Webhook发送失败，response为null");
            return false;
        }

        // 只要返回200/204等都算成功
        _logger.Information("自定义Webhook发送完成，返回：" + response);
        return true;
    }
} 