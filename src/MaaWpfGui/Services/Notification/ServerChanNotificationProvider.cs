// <copyright file="ServerChanNotificationProvider.cs" company="MaaAssistantArknights">
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

            // 根据 sendkey 的前缀选择不同的 API URL
            var url = sendKey.StartsWith("sctp") 
                ? $"https://{sendKey}.push.ft07.com/send" 
                : $"https://sctapi.ftqq.com/{sendKey}.send";

            // 添加header，尝试修复 data format error
            var response = await _httpService.PostAsJsonAsync(
                new Uri(url),
                new ServerChanPostContent { Title = title, Content = content },
                headers);

            // 设置请求头，指定为 application/json;charset=utf-8
            var headers = new Dictionary<string, string>
            {
                { "Content-Type", "application/json;charset=utf-8" }
            };

            var responseRoot = JsonDocument.Parse(response).RootElement;
            var hasCodeProperty = responseRoot.TryGetProperty("code", out var codeElement);

            if (!hasCodeProperty)
            {
                _logger.Warning("Failed to send ServerChan notification, unknown response: {Response}", response);
                return false;
            }

            var hasCode = codeElement.TryGetInt32(out var code);
            if (!hasCode)
            {
                _logger.Warning("Failed to send ServerChan notification, unknown code in response: {Response}", response);
                return false;
            }

            if (code == 0)
            {
                return true;
            }

            _logger.Warning("Failed to send ServerChan notification, code: {Code}", code);
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
