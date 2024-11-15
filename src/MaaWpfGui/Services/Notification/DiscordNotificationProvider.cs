// <copyright file="DiscordNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class DiscordNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private const string DiscordApiVersion = "v9";

    private readonly ILogger _logger = Log.ForContext<DiscordNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var botToken = SettingsViewModel.ExternalNotificationDataContext.DiscordBotToken;
        var userId = SettingsViewModel.ExternalNotificationDataContext.DiscordUserId;

        var channelId = await CreateDmChannel(botToken, userId);

        if (channelId is null)
        {
            return false;
        }

        return await SendMessageAsync(botToken, channelId, content);
    }

    private async Task<string?> CreateDmChannel(string token, string recipientId)
    {
        const string Uri = $"https://discord.com/api/{DiscordApiVersion}/users/@me/channels";

        var response = await httpService.PostAsJsonAsync(
            new Uri(Uri),
            new ChannelsCreation { RecipientId = recipientId },
            new Dictionary<string, string>
            {
                { "Authorization", $"Bot {token}" },
                { "User-Agent", "DiscordBot" }, // browser related user-agent is not allowed.
            });

        if (response is null)
        {
            _logger.Warning("Failed to create DM channel.");
            return null;
        }

        var responseData = JsonSerializer.Deserialize<JsonElement>(response);
        if (responseData.ValueKind == JsonValueKind.Undefined || !responseData.TryGetProperty("id", out var channelId))
        {
            return null;
        }

        _logger.Debug($"DM Channel created successfully. Channel ID: {channelId}");
        return channelId.GetString();
    }

    public async Task<bool> SendMessageAsync(string token, string channelId, string message)
    {
        var uri = $"https://discord.com/api/{DiscordApiVersion}/channels/{channelId}/messages";

        var response = await httpService.PostAsFormUrlEncodedAsync(
            new Uri(uri),
            new Dictionary<string, string?> { { "content", message } },
            new Dictionary<string, string>
            {
                { "Authorization", $"Bot {token}" },
                { "User-Agent", "DiscordBot" }, // browser related user-agent is not allowed.
            });

        if (response is not null)
        {
            return true;
        }

        _logger.Warning("Failed to send message.");
        return false;
    }

    private class ChannelsCreation
    {
        // ReSharper disable UnusedMember.Local
        // ReSharper disable UnusedAutoPropertyAccessor.Local
        [JsonPropertyName("recipient_id")]
        public string? RecipientId { get; set; }

        [JsonPropertyName("access_tokens")]
        public string[]? AccessTokens { get; set; }

        [JsonPropertyName("nicks")]
        public string? Nicks { get; set; }

        // ReSharper restore UnusedMember.Local
        // ReSharper restore UnusedAutoPropertyAccessor.Local
    }
}
