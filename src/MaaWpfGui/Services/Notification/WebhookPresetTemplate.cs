// <copyright file="WebhookPresetTemplate.cs" company="MaaAssistantArknights">
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
using System.Collections.Generic;
using MaaWpfGui.Helper;

namespace MaaWpfGui.Services.Notification;

public class WebhookPresetTemplate
{
    public string Id { get; init; } = string.Empty;

    public string NameResourceKey { get; init; } = string.Empty;

    public string Url { get; init; } = string.Empty;

    public string Headers { get; init; } = string.Empty;

    public string BodyTemplate { get; init; } = string.Empty;

    // 新增模板时，必须在所有语言的 Localizations/*.xaml 中添加对应的 NameResourceKey 字符串
    public string DisplayName => LocalizationHelper.GetString(NameResourceKey);

    private static readonly List<WebhookPresetTemplate> _builtInTemplates =
    [
        new()
        {
            Id = "__custom__",
            NameResourceKey = "ExternalNotificationCustomWebhook.TemplateCustom",
        },
        new()
        {
            Id = "meow",
            NameResourceKey = "ExternalNotificationCustomWebhook.TemplateMeoW",
            Url = "https://api.chuckfang.com/<nickname>",
            BodyTemplate = "{\"title\":\"{title}\",\"msg\":\"{content}\\n{time}\"}",
        },
    ];

    public static IReadOnlyList<WebhookPresetTemplate> BuiltInTemplates => _builtInTemplates;
}
