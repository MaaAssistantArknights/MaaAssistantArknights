// <copyright file="MeoWNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Text.Json;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class MeoWNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private const string MeoWApiBase = "https://api.chuckfang.com";

    private readonly ILogger _logger = Log.ForContext<MeoWNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var nickname = SettingsViewModel.ExternalNotificationSettings.MeoWNickname;
        if (string.IsNullOrWhiteSpace(nickname))
        {
            _logger.Warning("Failed to send MeoW notification, nickname is empty");
            return false;
        }

        var url = $"{MeoWApiBase}/{Uri.EscapeDataString(nickname)}";

        try
        {
            var response = await httpService.PostAsJsonAsync(new Uri(url), new { title, msg = content });
            if (response == null)
            {
                _logger.Warning("Failed to send MeoW notification, response is null");
                return false;
            }

            using var json = JsonDocument.Parse(response);
            var root = json.RootElement;
            var isSuccess = root.TryGetProperty("status", out var statusElement)
                && statusElement.TryGetInt32(out var status)
                && status == 200;

            if (isSuccess)
            {
                return true;
            }

            var statusValue = statusElement.ValueKind == JsonValueKind.Undefined ? "missing" : statusElement.GetRawText();
            _logger.Warning("Failed to send MeoW notification, status: {Status}, response: {Response}", statusValue, response);
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Exception occurred while sending MeoW notification.");
        }

        return false;
    }
}
