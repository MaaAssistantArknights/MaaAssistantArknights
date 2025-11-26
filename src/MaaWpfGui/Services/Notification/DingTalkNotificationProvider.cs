// <copyright file="DingTalkNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class DingTalkNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private const string DingTalkWebhookEndpoint = "https://oapi.dingtalk.com/robot/send";
    private readonly ILogger _logger = Log.ForContext<DingTalkNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var accessToken = SettingsViewModel.ExternalNotificationSettings.DingTalkAccessToken;
        var secret = SettingsViewModel.ExternalNotificationSettings.DingTalkSecret;

        if (string.IsNullOrEmpty(accessToken))
        {
            _logger.Warning("Failed to send DingTalk notification: Access Token is empty");
            return false;
        }

        var webhook = BuildWebhookUrl(accessToken, secret);
        var requestBody = new DingTalkPostContent
        {
            MessageType = "text",
            Text = new DingTalkTextContent
            {
                Content = $"{title}: {content}"
            }
        };

        var response = await httpService.PostAsJsonAsync(new Uri(webhook), requestBody);
        if (response == null)
        {
            _logger.Warning("Failed to send DingTalk notification: response is null");
            return false;
        }

        try
        {
            var responseData = JsonSerializer.Deserialize<DingTalkResponse>(response);
            if (responseData?.ErrorCode == 0)
            {
                _logger.Information("DingTalk notification sent successfully.");
                return true;
            }

            _logger.Warning(
                "Failed to send DingTalk notification, error code: {ErrorCode}, error message: {ErrorMessage}",
                responseData?.ErrorCode,
                responseData?.ErrorMessage);
            return false;
        }
        catch (JsonException ex)
        {
            _logger.Warning(ex, "Failed to parse DingTalk response.");
            return false;
        }
    }

    private static string BuildWebhookUrl(string accessToken, string? secret)
    {
        var builder = new StringBuilder($"{DingTalkWebhookEndpoint}?access_token={accessToken}");

        if (string.IsNullOrEmpty(secret))
        {
            return builder.ToString();
        }

        var timestamp = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        var signBase = $"{timestamp}\n{secret}";
        using var hmac = new HMACSHA256(Encoding.UTF8.GetBytes(secret));
        var hash = hmac.ComputeHash(Encoding.UTF8.GetBytes(signBase));
        var sign = WebUtility.UrlEncode(Convert.ToBase64String(hash));
        builder.Append($"&timestamp={timestamp}&sign={sign}");

        return builder.ToString();
    }

    private class DingTalkPostContent
    {
        [JsonPropertyName("msgtype")]
        public string? MessageType { get; set; }

        [JsonPropertyName("text")]
        public DingTalkTextContent? Text { get; set; }
    }

    private class DingTalkTextContent
    {
        [JsonPropertyName("content")]
        public string? Content { get; set; }
    }

    private class DingTalkResponse
    {
        [JsonPropertyName("errcode")]
        public int ErrorCode { get; set; }

        [JsonPropertyName("errmsg")]
        public string? ErrorMessage { get; set; }
    }