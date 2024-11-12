// <copyright file="BarkNotificationProvider.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification
{
    public class BarkNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
    {
        private readonly ILogger _logger = Log.ForContext<BarkNotificationProvider>();

        public async Task<bool> SendAsync(string title, string content)
        {
            var sendKey = SettingsViewModel.ExternalNotificationDataContext.BarkSendKey;
            if (string.IsNullOrWhiteSpace(sendKey))
            {
                _logger.Warning("Failed to send Bark notification, Bark send key is empty");
                return false;
            }

            var apiBase = SettingsViewModel.ExternalNotificationDataContext.BarkServer;
            if (string.IsNullOrWhiteSpace(apiBase))
            {
                _logger.Warning("Failed to send Bark notification, Bark server address is empty");
                return false;
            }

            var response = await httpService.PostAsJsonAsync(
                new Uri(new Uri(apiBase), "/push"),
                new BarkPostContent { Title = title, Content = content, SendKey = sendKey });
            if (response == null)
            {
                _logger.Warning("Failed to send Bark notification, response is null");
                return false;
            }

            var data = JsonSerializer.Deserialize<BarkResponse>(response);
            if (data?.Code == 200)
            {
                return true;
            }

            _logger.Warning("Failed to send Bark notification: {Code} {Message}", data?.Code, data?.Message);
            return false;
        }

        private class BarkPostContent
        {
            // ReSharper disable UnusedAutoPropertyAccessor.Local
            // ReSharper disable UnusedMember.Local
            [JsonPropertyName("device_key")]
            public string? SendKey { get; set; }

            [JsonPropertyName("title")]
            public string? Title { get; set; }

            [JsonPropertyName("body")]
            public string? Content { get; set; }

            [JsonPropertyName("icon")]
            public static string Icon { get => "https://cdn.jsdelivr.net/gh/MaaAssistantArknights/design@main/logo/maa-logo_256x256.png"; }

            // ReSharper restore UnusedAutoPropertyAccessor.Local
            // ReSharper restore UnusedMember.Local
        }

        private class BarkResponse
        {
            [JsonPropertyName("code")]
            public int? Code { get; init; }

            [JsonPropertyName("message")]
            public string? Message { get; init; }

            [JsonPropertyName("timestamp")]
            public long? Timestamp { get; init; }
        }
    }
}
