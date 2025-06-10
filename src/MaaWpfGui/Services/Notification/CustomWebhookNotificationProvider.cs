// <copyright file="CustomWebhookNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
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
            _logger.Warning("Custom Webhook failed to send: URL or message body is empty");
            return false;
        }

        // 占位符替换
        string now = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
        string body = bodyTemplate
            .Replace("{title}", title.Replace("\r", string.Empty).Replace("\n", string.Empty))
            .Replace("{content}", content.Replace("\r", string.Empty).Replace("\n", "\\n"))
            .Replace("{time}", now);

        var requestContent = new StringContent(body, Encoding.UTF8, "application/json");
        var response = await httpService.PostAsync(new(webhookUrl), requestContent);

        if (response == null)
        {
            _logger.Warning("Custom Webhook failed to send: response is null");
            return false;
        }

        try
        {
            response.EnsureSuccessStatusCode();
            _logger.Information("Custom Webhook sent successfully, response: " + response);
            return true;
        }
        catch (HttpRequestException ex)
        {
            _logger.Warning("Custom Webhook failed to send: " + ex.Message);
            return false;
        }
    }
}
