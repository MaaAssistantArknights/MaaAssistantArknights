// <copyright file="DiscordWebhookNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class DiscordWebhookNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<DiscordWebhookNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var webhookUrl = SettingsViewModel.ExternalNotificationSettings.DiscordWebhookUrl;
        if (string.IsNullOrEmpty(webhookUrl))
        {
            _logger.Warning("Failed to send Discord Webhook, Webhook URL is empty");
            return false;
        }

        var response = await httpService.PostAsJsonAsync(
            new Uri(webhookUrl),
            new DiscordWebhookPostContent { Content = content });

        // httpService failed to post返回null
        if (response == null)
        {
            _logger.Warning("Failed to send Discord Webhook, response is null");
            return false;
        }

        // 成功時Discord Webhook返回204 No Content
        if (string.IsNullOrEmpty(response))
        {
            _logger.Information("Discord Webhook sent successfully.");
            return true;
        }

        // 輸入非Discord Webhook的URL會返回HTML
        if (!IsValidJson(response))
        {
            _logger.Error("Failed to send Discord Webhook, received an invalid JSON response.");
            return false;
        }

        var responseData = JsonSerializer.Deserialize<DiscordWebhookResponse>(response);
        _logger.Warning($"Failed to send Discord Webhook, {responseData?.Message}");

        return false;
    }

    private static bool IsValidJson(string input)
    {
        try
        {
            JsonDocument.Parse(input);
            return true;
        }
        catch
        {
            return false;
        }
    }

    private class DiscordWebhookPostContent
    {
        [JsonPropertyName("content")]
        public string? Content { get; set; }
    }

    private class DiscordWebhookResponse
    {
        [JsonPropertyName("message")]
        public string? Message { get; set; }

        [JsonPropertyName("code")]
        public int? ErrorCode { get; set; }
    }
}
