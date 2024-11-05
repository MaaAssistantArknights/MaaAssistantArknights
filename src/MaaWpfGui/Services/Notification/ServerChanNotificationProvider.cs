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
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
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
            // 去掉 title 中的换行符
            title = title.Replace("\n", string.Empty);

            // 确保 title 的长度不超过 32 个字符
            if (title.Length > 32)
            {
                title = title[..32]; // 截取前 32 个字符
            }

            var sendKey = Instances.SettingsViewModel.ServerChanSendKey;

            try
            {
                var url = ConstructUrl(sendKey);
                var postData = $"text={Uri.EscapeDataString(title)}&desp={Uri.EscapeDataString(content)}";

                using var httpClient = new HttpClient();
                var request = new HttpRequestMessage(HttpMethod.Post, url)
                {
                    Content = new StringContent(postData, Encoding.UTF8, "application/x-www-form-urlencoded"),
                };

                var response = await httpClient.SendAsync(request);
                var responseContent = await response.Content.ReadAsStringAsync();

                var responseRoot = JsonDocument.Parse(responseContent).RootElement;
                if (responseRoot.TryGetProperty("code", out var codeElement) && codeElement.TryGetInt32(out var code))
                {
                    if (code == 0)
                    {
                        return true;
                    }

                    _logger.Warning($"Failed to send ServerChan notification, code: {code}");
                }
                else
                {
                    _logger.Warning($"Failed to send ServerChan notification, unknown response: {responseContent}");
                }
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Exception occurred while sending ServerChan notification.");
            }

            return false;
        }

        private static string ConstructUrl(string sendKey)
        {
            if (!sendKey.StartsWith("sctp"))
            {
                return $"https://sctapi.ftqq.com/{sendKey}.send";
            }

            // Server酱3
            var match = Regex.Match(sendKey, @"^sctp(\d+)t");
            if (!match.Success)
            {
                throw new ArgumentException("Invalid key format for sctp.");
            }

            var num = match.Groups[1].Value;
            return $"https://{num}.push.ft07.com/send/{sendKey}.send";
        }
    }
}
