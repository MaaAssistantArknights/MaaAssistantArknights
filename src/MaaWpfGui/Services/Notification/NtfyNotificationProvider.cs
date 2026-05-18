// <copyright file="NtfyNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class NtfyNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<NtfyNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var server = SettingsViewModel.ExternalNotificationSettings.NtfyServer;
        var topic = SettingsViewModel.ExternalNotificationSettings.NtfyTopic;

        if (string.IsNullOrWhiteSpace(server))
        {
            _logger.Warning("Failed to send Ntfy notification, server URL is empty");
            return false;
        }

        server = server.Trim();
        if (!Uri.TryCreate(server, UriKind.Absolute, out var baseUri) ||
            (baseUri.Scheme != Uri.UriSchemeHttp && baseUri.Scheme != Uri.UriSchemeHttps))
        {
            return false;
        }

        if (string.IsNullOrWhiteSpace(topic))
        {
            _logger.Warning("Failed to send Ntfy notification, topic is empty");
            return false;
        }

        var token = SettingsViewModel.ExternalNotificationSettings.NtfyToken;
        Dictionary<string, string>? headers = null;

        if (!string.IsNullOrWhiteSpace(token))
        {
            headers = new Dictionary<string, string>
            {
                { "Authorization", $"Bearer {token.Trim()}" },
            };
        }

        try
        {
            var response = await httpService.PostAsJsonAsync(
                baseUri,
                new NtfyMessage { Topic = topic.Trim(), Title = title, Message = content },
                headers);

            if (string.IsNullOrEmpty(response))
            {
                _logger.Warning("Failed to send Ntfy notification, response is null");
                return false;
            }

            _logger.Information("Ntfy notification sent successfully");
            return true;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to send Ntfy notification");
            return false;
        }
    }

    private class NtfyMessage
    {
        [JsonPropertyName("topic")]
        public string Topic { get; set; } = string.Empty;

        [JsonPropertyName("title")]
        public string Title { get; set; } = string.Empty;

        [JsonPropertyName("message")]
        public string Message { get; set; } = string.Empty;
    }
}
