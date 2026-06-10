// <copyright file="CustomWebhookTemplateManager.cs" company="MaaAssistantArknights">
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
using System.IO;
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;
using Serilog;

namespace MaaWpfGui.Services.Notification;

public static class CustomWebhookTemplateManager
{
    private static List<CustomWebhookTemplate>? _templates;
    private static readonly object _lock = new();

    private static string TemplateDir =>
        Path.Combine(PathsHelper.ResourceDir, "webhook_template");

    public static List<CustomWebhookTemplate> LoadTemplates()
    {
        if (_templates != null)
        {
            return _templates;
        }

        lock (_lock)
        {
            if (_templates != null)
            {
                return _templates;
            }

            var list = new List<CustomWebhookTemplate>
            {
                new()
                {
                    Id = "__custom__",
                    Name = new CustomWebhookLocalizedString
                    {
                        Default = "en-us",
                        Content = new()
                        {
                            ["zh-cn"] = "自定义",
                            ["en-us"] = "Custom",
                            ["zh-tw"] = "自訂",
                            ["ja-jp"] = "カスタム",
                            ["ko-kr"] = "사용자 정의",
                        },
                    },
                },
            };

            if (Directory.Exists(TemplateDir))
            {
                foreach (var file in Directory.GetFiles(TemplateDir, "*.json"))
                {
                    try
                    {
                        var json = File.ReadAllText(file);
                        var template = JsonConvert.DeserializeObject<CustomWebhookTemplate>(json);
                        if (template != null && !string.IsNullOrEmpty(template.Id))
                        {
                            list.Add(template);
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "Failed to load webhook template: {File}", file);
                    }
                }
            }

            _templates = list;
            return list;
        }
    }

    public static CustomWebhookTemplate? FindTemplate(string id)
    {
        return LoadTemplates().FirstOrDefault(t => t.Id == id);
    }

    public static string ApplyParameters(string raw, Dictionary<string, string> parameters)
    {
        if (string.IsNullOrEmpty(raw) || parameters == null)
        {
            return raw;
        }

        foreach (var (key, value) in parameters)
        {
            raw = raw.Replace($"{{{key}}}", value ?? string.Empty);
        }

        return raw;
    }

    public static Dictionary<string, string> GetAllParameters(string templateId)
    {
        var stored = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookTemplateParams, "{}");
        try
        {
            var all = JsonConvert.DeserializeObject<Dictionary<string, Dictionary<string, string>>>(stored);
            return all?.GetValueOrDefault(templateId) ?? [];
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to deserialize webhook template parameters for {TemplateId}", templateId);
            return [];
        }
    }

    public static void SetParameter(string templateId, string key, string value)
    {
        var stored = ConfigurationHelper.GetValue(ConfigurationKeys.ExternalNotificationCustomWebhookTemplateParams, "{}");
        try
        {
            var all = JsonConvert.DeserializeObject<Dictionary<string, Dictionary<string, string>>>(stored)
                      ?? [];
            if (!all.ContainsKey(templateId))
            {
                all[templateId] = [];
            }

            all[templateId][key] = value;
            ConfigurationHelper.SetValue(
                ConfigurationKeys.ExternalNotificationCustomWebhookTemplateParams,
                JsonConvert.SerializeObject(all));
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to serialize webhook template parameters for {TemplateId}", templateId);
        }
    }

    public static void Invalidate()
    {
        _templates = null;
    }
}
