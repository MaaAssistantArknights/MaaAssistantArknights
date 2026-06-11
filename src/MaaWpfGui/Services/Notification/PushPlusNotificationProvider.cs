// <copyright file="PushPlusNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class PushPlusNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<PushPlusNotificationProvider>();

    /// <summary>
    /// PushPlus API endpoint
    /// </summary>
    private const string PushPlusApiUrl = "https://www.pushplus.plus/send";

    /// <summary>
    /// Default template for PushPlus notifications
    /// </summary>
    public const string DefaultTemplate = "html";

    public async Task<bool> SendAsync(string title, string content)
    {
        var token = SettingsViewModel.ExternalNotificationSettings.PushPlusToken;
        if (string.IsNullOrWhiteSpace(token))
        {
            _logger.Warning("Failed to send PushPlus notification, token is empty");
            return false;
        }

        var template = SettingsViewModel.ExternalNotificationSettings.PushPlusTemplate;
        if (string.IsNullOrWhiteSpace(template))
        {
            template = DefaultTemplate;
        }

        try
        {
            var requestBody = new PushPlusPostContent
            {
                Token = token,
                Title = title,
                Content = content,
                Template = template,
            };

            var json = JsonSerializer.Serialize(requestBody);
            var response = await httpService.PostAsync(new(PushPlusApiUrl), new StringContent(json, Encoding.UTF8, "application/json"));

            if (!response.IsSuccessStatusCode)
            {
                _logger.Warning("PushPlus notification failed with HTTP status: {StatusCode}", response.StatusCode);
                return false;
            }

            var responseContent = await response.Content.ReadAsStringAsync();

            var responseRoot = JsonDocument.Parse(responseContent).RootElement;
            if (responseRoot.TryGetProperty("code", out var codeElement) && codeElement.TryGetInt32(out var code))
            {
                if (code == 200)
                {
                    _logger.Information("PushPlus notification sent successfully.");
                    return true;
                }

                var msg = responseRoot.TryGetProperty("msg", out var msgElement) ? msgElement.GetString() : "Unknown error";
                _logger.Warning("Failed to send PushPlus notification, code: {Code}, msg: {Message}", code, msg);
            }
            else
            {
                _logger.Warning("Failed to send PushPlus notification, unknown response: {ResponseContent}", responseContent);
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Exception occurred while sending PushPlus notification.");
        }

        return false;
    }

    private class PushPlusPostContent
    {
        // ReSharper disable UnusedAutoPropertyAccessor.Local
        // ReSharper disable UnusedMember.Local
        [System.Text.Json.Serialization.JsonPropertyName("token")]
        public string Token { get; set; } = string.Empty;

        [System.Text.Json.Serialization.JsonPropertyName("title")]
        public string Title { get; set; } = string.Empty;

        [System.Text.Json.Serialization.JsonPropertyName("content")]
        public string Content { get; set; } = string.Empty;

        [System.Text.Json.Serialization.JsonPropertyName("template")]
        public string Template { get; set; } = "html";

        // ReSharper restore UnusedAutoPropertyAccessor.Local
        // ReSharper restore UnusedMember.Local
    }
}