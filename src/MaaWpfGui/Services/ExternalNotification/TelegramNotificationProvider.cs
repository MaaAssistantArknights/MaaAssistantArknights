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
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Threading.Tasks;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.Services.Web;
using MaaWpfGui.ViewModels.UI;
using Serilog;

namespace MaaWpfGui.Services.ExternalNotification;

public class TelegramNotificationProvider(IHttpService httpService, TelegramConfig telegram) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<TelegramNotificationProvider>();

    /// <summary>
    /// sendMessage 的 text 上限，超出时接口返回 400 message is too long。
    /// </summary>
    private const int MaxTextLength = 4096;

    private const string TruncatedMark = "[...]\n";

    public async Task<bool> SendAsync(string title, string content)
    {
        var botToken = telegram.BotToken;
        var chatId = telegram.ChatId;
        var topicId = telegram.TopicId;

        var uri = $"https://api.telegram.org/bot{botToken}/sendMessage";

        var postContent = new TelegramPostContent
        {
            ChatId = chatId,
            Content = Truncate($"{title}: {content}"),
        };

        // Only add the topic ID if one is provided
        if (!string.IsNullOrEmpty(topicId))
        {
            postContent.TopicId = topicId;
        }

        try
        {
            var response = await httpService.PostAsync(new(uri), new StringContent(JsonSerializer.Serialize(postContent), Encoding.UTF8, "application/json"), uriPartial: UriPartial.Authority);
            var str = await response.Content.ReadAsStringAsync();
            if (!response.IsSuccessStatusCode)
            {
                // 失败原因只在响应体里（如 message is too long），不记下来无从排查
                _logger.Warning("Telegram API returned {StatusCode}: {Body}", (int)response.StatusCode, str);
                return false;
            }

            return !str.Contains("\"ok\":false");
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send POST request to {Uri}", new Uri(uri).GetLeftPart(UriPartial.Authority));
        }

        _logger.Warning("Failed to send message.");
        return false;
    }

    /// <summary>
    /// 把消息裁到 <see cref="MaxTextLength"/> 以内。开启 ｢通知包含详细日志｣ 后，完成通知会带上本轮全部日志，
    /// 长任务很容易超限而整条发不出去。保留末尾：用时、配置与出错清单等正文在日志之后。
    /// </summary>
    /// <param name="text">完整消息</param>
    /// <returns>不超过上限的消息</returns>
    private static string Truncate(string text)
    {
        if (text.Length <= MaxTextLength)
        {
            return text;
        }

        // 起点落在代理项对（如 emoji）中间时前半已被裁掉，剩下的低位代理项要一并丢掉，否则序列化成 JSON 会变成 U+FFFD
        var suffixStart = text.Length - (MaxTextLength - TruncatedMark.Length);
        if (char.IsLowSurrogate(text[suffixStart]))
        {
            suffixStart++;
        }

        return TruncatedMark + text[suffixStart..];
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
