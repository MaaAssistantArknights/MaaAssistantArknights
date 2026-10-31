// <copyright file="ExternalNotificationService.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.ExternalNotification;
using Serilog;

namespace MaaWpfGui.Services.ExternalNotification;

public static class ExternalNotificationService
{
    private static readonly ILogger _logger = Log.ForContext(typeof(ExternalNotificationService));

    public static async Task<bool> SendAsync(BaseConfig snapshot, string displayName, string title, string content, bool isTest = false)
    {
        var channel = ExternalNotificationChannel.ForEditor(snapshot);
        bool result = false;
        try
        {
            if (channel is not null)
            {
                result = await channel.CreateProvider(snapshot).SendAsync("[MAA] " + title, content).ConfigureAwait(false);
            }
        }
        catch (Exception ex)
        {
            _logger.Error(ex, "Failed to send external notification via {Channel}", displayName);
        }
        _logger.Information("External notification channel {Channel} returned {Success}", displayName, result);
        if (isTest || !result)
        {
            await Stylet.Execute.OnUIThreadAsync(() => ToastNotification.ShowDirect(displayName + " " +
                LocalizationHelper.GetString(result ? "ExternalNotification.Send.Success" : "ExternalNotification.Send.Failed"))).ConfigureAwait(false);
        }
        return result;
    }
}
