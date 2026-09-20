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

    public Task<bool> SendAsync(string title, string content)
    {
        return SendMessageAsync(Snapshot(), Truncate($"{title}: {content}"));
    }

    public async Task<bool> SendAsync(string title, string content, string details)
    {
        var target = Snapshot();
        var full = $"{title}: {details}{content}";
        if (full.Length <= MaxTextLength)
        {
            return await SendMessageAsync(target, full);
        }

        // 一条消息装不下详细日志：完整内容（含日志）作为 .txt 附件，消息退回不带详细日志的样子。
        // 附件发不出去时不能丢日志，消息仍带日志并裁到上限
        return await SendDocumentAsync(target, full)
            ? await SendMessageAsync(target, Truncate($"{title}: {content}"))
            : await SendMessageAsync(target, Truncate(full));
    }

    /// <summary>
    /// 一次通知的两个请求要发往同一个机器人、聊天与话题，所以开头就取一份，不能让用户中途改设置把它们拆开。
    /// </summary>
    /// <returns>当前配置的快照</returns>
    private Target Snapshot() => new(telegram.BotToken, telegram.ChatId, telegram.TopicId);

    private Task<bool> SendMessageAsync(Target target, string text)
    {
        var postContent = new TelegramPostContent
        {
            ChatId = target.ChatId,
            Content = text,
        };

        // Only add the topic ID if one is provided
        if (!string.IsNullOrEmpty(target.TopicId))
        {
            postContent.TopicId = target.TopicId;
        }

        return PostAsync(target, "sendMessage", new StringContent(JsonSerializer.Serialize(postContent), Encoding.UTF8, "application/json"));
    }

    /// <summary>
    /// 把完整消息作为 .txt 文件发出去。sendDocument 的文件上限是 50 MB，远大于任何一次任务的日志。
    /// </summary>
    /// <param name="target">发送目标</param>
    /// <param name="text">完整消息</param>
    /// <returns>是否发送成功</returns>
    private Task<bool> SendDocumentAsync(Target target, string text)
    {
        var form = new MultipartFormDataContent
        {
            { new StringContent(target.ChatId), "chat_id" },
            { new StringContent(text, Encoding.UTF8, "text/plain"), "document", $"MAA-{DateTime.Now:yyyyMMdd-HHmmss}.txt" },
        };

        if (!string.IsNullOrEmpty(target.TopicId))
        {
            form.Add(new StringContent(target.TopicId), "message_thread_id");
        }

        return PostAsync(target, "sendDocument", form);
    }

    private async Task<bool> PostAsync(Target target, string method, HttpContent content)
    {
        var uri = $"https://api.telegram.org/bot{target.BotToken}/{method}";

        try
        {
            var response = await httpService.PostAsync(new(uri), content, uriPartial: UriPartial.Authority);
            var str = await response.Content.ReadAsStringAsync();
            if (!response.IsSuccessStatusCode)
            {
                // 失败原因只在响应体里（如 message is too long），不记下来无从排查
                _logger.Warning("Telegram API {Method} returned {StatusCode}: {Body}", method, (int)response.StatusCode, str);
                return false;
            }

            return !str.Contains("\"ok\":false");
        }
        catch (Exception e)
        {
            _logger.Error(e, "Failed to send POST request to {Uri}", new Uri(uri).GetLeftPart(UriPartial.Authority));
            return false;
        }
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

    private readonly record struct Target(string BotToken, string ChatId, string TopicId);

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
