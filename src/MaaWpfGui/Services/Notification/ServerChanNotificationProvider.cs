// <copyright file="ServerChanNotificationProvider.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

using System;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using Serilog;

namespace MaaWpfGui.Services.Notification
{
    public class ServerChanNotificationProvider : IExternalNotificationProvider
    {
        private readonly IHttpService _httpService;

        private readonly ILogger _logger = Log.ForContext<ServerChanNotificationProvider>();

        public ServerChanNotificationProvider(IHttpService httpService)
        {
            _httpService = httpService;
        }

        public async Task<bool> SendAsync(string title, string content)
        {
            var sendKey = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationServerChanSendKey, string.Empty);
            var url = $"https://sctapi.ftqq.com/{sendKey}.send";

            var response = await _httpService.PostAsJsonAsync(
                new Uri(url),
                new ServerChanPostContent { Title = title, Content = content, });

            var responseRoot = JsonDocument.Parse(response).RootElement;
            var hasCodeProperty = responseRoot.TryGetProperty("code", out var codeElement);
            if (hasCodeProperty is false)
            {
                _logger.Warning("Failed to send ServerChan notification, unknown response, {Response}", response);
                return false;
            }

            var hasCode = codeElement.TryGetInt32(out var code);
            if (hasCode is false)
            {
                _logger.Warning("Failed to send ServerChan notification, unknown response {Response}", response);
                return false;
            }

            if (code == 0)
            {
                return true;
            }

            _logger.Warning("Failed to send ServerChan notification, code {Code}", code);
            return false;
        }

        private class ServerChanPostContent
        {
            // 这两个没用过，不知道有没有用，之后再看看
            [JsonPropertyName("title")]

            // ReSharper disable once UnusedAutoPropertyAccessor.Local
            public string Title { get; set; }

            [JsonPropertyName("desp")]

            // ReSharper disable once UnusedAutoPropertyAccessor.Local
            public string Content { get; set; }
        }
    }
}
