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
using System.IO;
using System.Net.Http;
using System.Text;
using System.Text.RegularExpressions;
using System.Text.Json;
using System.Threading.Tasks;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Serilog;
using MaaWpfGui.Services.Web;

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

            try
            {
                var url = ConstructUrl(sendKey);
                var postData = $"text={Uri.EscapeDataString(title)}&desp={Uri.EscapeDataString(content)}";

                using var httpClient = new HttpClient();
                var request = new HttpRequestMessage(HttpMethod.Post, url)
                {
                    Content = new StringContent(postData, Encoding.UTF8, "application/x-www-form-urlencoded")
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
                    _logger.Warning("Failed to send ServerChan notification, code: {Code}", code);
                }
                else
                {
                    _logger.Warning("Failed to send ServerChan notification, unknown response: {Response}", responseContent);
                }
            }
            catch (Exception ex)
            {
                _logger.Error(ex, "Exception occurred while sending ServerChan notification.");
            }

            return false;
        }

        private string ConstructUrl(string sendKey)
        {
            if (sendKey.StartsWith("sctp"))
            { // Server酱3
                var match = Regex.Match(sendKey, @"^sctp(\d+)t");
                if (match.Success)
                {
                    var num = match.Groups[1].Value;
                    return $"https://{num}.push.ft07.com/send/{sendKey}.send";
                }
                else
                {
                    throw new ArgumentException("Invalid key format for sctp.");
                }
            }
            else
            {
                return $"https://sctapi.ftqq.com/{sendKey}.send";
            }
        }
    }
}
