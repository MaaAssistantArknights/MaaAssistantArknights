// <copyright file="CustomWebhookConfig.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Helper;
using MaaWpfGui.Services.Notification;
using static MaaWpfGui.Configuration.Single.Settings.ExternalNotification;

namespace MaaWpfGui.Models.ExternalNotification;

public class CustomWebhookConfig(string url = "", string headers = "", string body = "") : BaseConfig
{
    public CustomWebhookConfig()
        : this(string.Empty, string.Empty, string.Empty)
    {
    }

    public CustomWebhookConfig(CustomWebhook customWebhook)
        : this(SimpleEncryptionHelper.Decrypt(customWebhook.Url), SimpleEncryptionHelper.Decrypt(customWebhook.Headers), SimpleEncryptionHelper.Decrypt(customWebhook.Body))
    {
    }

    public string Url { get; set => SetAndNotify(ref field, value); } = url;

    public string Body { get; set => SetAndNotify(ref field, value); } = body;

    public string Headers { get; set => SetAndNotify(ref field, value); } = headers;

    public override CustomWebhook ToConfig() => new(SimpleEncryptionHelper.Encrypt(Url), SimpleEncryptionHelper.Encrypt(Headers), SimpleEncryptionHelper.Encrypt(Body));

    private string _selectedPresetTemplateId = "__custom__";

    public string SelectedPresetTemplateId
    {
        get => _selectedPresetTemplateId;
        set {
            if (!SetAndNotify(ref _selectedPresetTemplateId, value))
            {
                return;
            }

            if (value == "__custom__")
            {
                return;
            }

            var template = WebhookPresetTemplate.BuiltInTemplates.FirstOrDefault(t => t.Id == value);
            if (template == null)
            {
                return;
            }

            Url = template.Url;
            Body = template.Body;
            Headers = template.Headers;
        }
    }
}
