// <copyright file="CustomWebhookNotificationProvider.cs" company="MaaAssistantArknights">
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
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using MaaWpfGui.Models.ExternalNotification;
using MaaWpfGui.Services.Web;
using Serilog;

namespace MaaWpfGui.Services.ExternalNotification;

public class CustomWebhookNotificationProvider(IHttpService httpService, CustomWebhookConfig custom) : IExternalNotificationProvider
{
    private readonly ILogger _logger = Log.ForContext<CustomWebhookNotificationProvider>();

    public async Task<bool> SendAsync(string title, string content)
    {
        var webhookUrl = custom.Url;
        var webhookHeaders = custom.Headers;
        var bodyTemplate = custom.Body;
        if (string.IsNullOrEmpty(webhookUrl) || string.IsNullOrEmpty(bodyTemplate))
        {
            _logger.Warning("Custom Webhook failed to send: URL or message body is empty");
            return false;
        }

        // 占位符替换；标题和内容会原样嵌入 JSON 模板的字符串字面量，
        // 需转义反斜杠、引号和换行，否则含这些字符的任务日志会破坏 JSON 结构
        string now = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
        string body = bodyTemplate
            .Replace("{title}", EscapeJsonString(title))
            .Replace("{content}", EscapeJsonString(content))
            .Replace("{time}", now);

        var requestContent = new StringContent(body, Encoding.UTF8, "application/json");
        var headers = webhookHeaders.Replace("\r", string.Empty).Split('\n', StringSplitOptions.RemoveEmptyEntries)
            .Select(line => line.Split(':', 2))
            .Where(parts => parts.Length == 2)
            .ToDictionary(p => p[0].Trim(), p => p[1].Trim());
        var response = await httpService.PostAsync(new(webhookUrl), requestContent, headers);

        if (response == null)
        {
            _logger.Warning("Custom Webhook failed to send: response is null");
            return false;
        }

        try
        {
            response.EnsureSuccessStatusCode();
            _logger.Information("Custom Webhook sent successfully, response: " + response);
            return true;
        }
        catch (HttpRequestException ex)
        {
            _logger.Warning("Custom Webhook failed to send: " + ex.Message);
            return false;
        }
    }

    /// <summary>
    /// 转义嵌入 JSON 字符串字面量的特殊字符：反斜杠、引号；换行转为 \n 字面量，\r 丢弃。
    /// </summary>
    private static string EscapeJsonString(string value) => value
        .Replace("\\", "\\\\")
        .Replace("\"", "\\\"")
        .Replace("\r", string.Empty)
        .Replace("\n", "\\n");
}
