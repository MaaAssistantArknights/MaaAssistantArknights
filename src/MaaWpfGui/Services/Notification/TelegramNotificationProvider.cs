// <copyright file="TelegramNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public class TelegramNotificationProvider(IHttpService httpService) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<TelegramNotificationProvider>();

    private const int MaxMessageLength = 3900;

    // Matches "[timestamp][color]" at the start of a log entry line
    private static readonly Regex LogEntryRegex = new(@"^\[[^\]]+\]\[[^\]]+\]", RegexOptions.Compiled | RegexOptions.Multiline);

    public async Task<bool> SendAsync(string title, string content)
    {
        var botToken = SettingsViewModel.ExternalNotificationSettings.TelegramBotToken;
        var chatId = SettingsViewModel.ExternalNotificationSettings.TelegramChatId;
        var topicId = SettingsViewModel.ExternalNotificationSettings.TelegramTopicId;

        var uri = $"https://api.telegram.org/bot{botToken}/sendMessage";
        var fullMessage = $"{title}: {content}";

        // Short enough — send as-is
        if (fullMessage.Length <= MaxMessageLength)
        {
            return await SendMessageAsync(uri, chatId, topicId, fullMessage);
        }

        // Too long — split by log entry boundaries ([timestamp][color])
        var maxContentLength = MaxMessageLength - title.Length - ": ".Length;
        var parts = SplitByLogEntries(content, maxContentLength);
        var allSuccess = true;
        foreach (var part in parts)
        {
            if (!await SendMessageAsync(uri, chatId, topicId, $"{title}: {part}"))
            {
                allSuccess = false;
            }
        }

        return allSuccess;
    }

    /// <summary>
    /// Split content into sub-messages by log entry boundaries ([timestamp][color]).
    /// Each sub-message fits within maxLength. Only splits — does not modify content.
    /// </summary>
    private static List<string> SplitByLogEntries(string segment, int maxLength)
    {
        // Find entry start positions
        var entryStarts = new List<int>();
        foreach (Match match in LogEntryRegex.Matches(segment))
        {
            // Align to line start
            var lineStart = segment.LastIndexOf('\n', match.Index);
            lineStart = lineStart < 0 ? 0 : lineStart + 1;
            if (entryStarts.Count == 0 || entryStarts[^1] != lineStart)
            {
                entryStarts.Add(lineStart);
            }
        }

        if (entryStarts.Count == 0)
        {
            return [segment];
        }

        // Group consecutive entries into chunks that fit maxLength
        var parts = new List<string>();
        var chunkStart = entryStarts[0];
        for (var i = 0; i < entryStarts.Count; i++)
        {
            var entryEnd = i + 1 < entryStarts.Count ? entryStarts[i + 1] : segment.Length;
            var chunkEnd = entryEnd;

            // If adding this entry would exceed limit, flush current chunk
            if (chunkEnd - chunkStart > maxLength && chunkStart < entryStarts[i])
            {
                parts.Add(segment[chunkStart..entryStarts[i]]);
                chunkStart = entryStarts[i];
            }
        }

        // Flush remaining
        if (chunkStart < segment.Length)
        {
            parts.Add(segment[chunkStart..]);
        }

        return parts;
    }

    private async Task<bool> SendMessageAsync(string uri, string chatId, string topicId, string message)
    {
        var postContent = new TelegramPostContent
        {
            ChatId = chatId,
            Content = message,
        };

        if (!string.IsNullOrEmpty(topicId))
        {
            postContent.TopicId = topicId;
        }

        try
        {
            var response = await httpService.PostAsync(new(uri), new StringContent(JsonSerializer.Serialize(postContent), Encoding.UTF8, "application/json"), uriPartial: UriPartial.Authority);
            response.EnsureSuccessStatusCode();
            var str = await response.Content.ReadAsStringAsync();
            if (response is not null)
            {
                return !str.Contains("\"ok\":false");
            }
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send POST request to {Uri}", uri);
        }

        _logger.Warning("Failed to send message.");
        return false;
    }

    private class TelegramPostContent
    {
        [JsonPropertyName("chat_id")]
        public string? ChatId { get; set; }

        [JsonPropertyName("text")]
        public string? Content { get; set; }

        [JsonPropertyName("message_thread_id")]
        public string? TopicId { get; set; }
    }
}
