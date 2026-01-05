// <copyright file="GotifyNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public partial class GotifyNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<GotifyNotificationProvider>();

    [GeneratedRegex(@"\[(.*?)\]\[(.*?)\]([\s\S]*?)(?=\n\[|$)")]
    private static partial Regex ContentRegex();

    private static string ProcessContent(string content)
    {
        var matches = ContentRegex().Matches(content);
        if (matches.Count == 0)
        {
            return content;
        }

        var resultContent = new StringBuilder();
        foreach (Match match in matches)
        {
            string time = match.Groups[1].Value;
            string contentText = match.Groups[3].Value;
            resultContent.Append($"[{time}]{contentText.TrimEnd('\n', '\r')}");
            resultContent.Append('\n');
        }

        return resultContent.ToString().TrimEnd('\n', '\r');
    }

    public async Task<bool> SendAsync(string title, string content)
    {
        var server = SettingsViewModel.ExternalNotificationSettings.GotifyServer;
        var token = SettingsViewModel.ExternalNotificationSettings.GotifyToken;

        if (string.IsNullOrWhiteSpace(server))
        {
            _logger.Warning("Failed to send Gotify notification, server URL is empty");
            return false;
        }

        server = server.Trim();
        if (!Uri.TryCreate(server, UriKind.Absolute, out var baseUri) ||
            (baseUri.Scheme != Uri.UriSchemeHttp && baseUri.Scheme != Uri.UriSchemeHttps))
        {
            return false;
        }

        if (string.IsNullOrWhiteSpace(token))
        {
            _logger.Warning("Failed to send Gotify notification, application token is empty");
            return false;
        }

        // 处理内容，去掉时间戳和颜色标记
        content = ProcessContent(content);

        try
        {
            var messageUri = new Uri(baseUri, "/message");

            var headers = new Dictionary<string, string>
            {
                { "X-Gotify-Key", token },
            };

            var response = await httpService.PostAsJsonAsync(
                messageUri,
                new GotifyMessage { Title = title, Message = content },
                headers);

            if (string.IsNullOrEmpty(response))
            {
                _logger.Warning("Failed to send Gotify notification, response is null");
                return false;
            }

            using var doc = JsonDocument.Parse(response);
            var responseRoot = doc.RootElement;
            if (responseRoot.TryGetProperty("id", out var idElement))
            {
                _logger.Information("Gotify notification sent successfully, message ID: {Id}", idElement.GetInt32());
                return true;
            }

            _logger.Warning("Failed to send Gotify notification, unknown response: {Response}", response);
            return false;
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to send Gotify notification");
            return false;
        }
    }

    private class GotifyMessage
    {
        [JsonPropertyName("title")]
        public string Title { get; set; } = string.Empty;

        [JsonPropertyName("message")]
        public string Message { get; set; } = string.Empty;
    }
}
