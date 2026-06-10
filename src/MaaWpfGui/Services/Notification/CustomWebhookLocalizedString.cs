// <copyright file="CustomWebhookLocalizedString.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using Newtonsoft.Json;

namespace MaaWpfGui.Services.Notification;

public class CustomWebhookLocalizedString
{
    public string Default { get; set; } = "en-us";

    public Dictionary<string, string> Content { get; set; } = [];

    [JsonIgnore]
    public string Resolved => Resolve();

    private string Resolve()
    {
        var lang = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.Localization, LocalizationHelper.DefaultLanguage);
        if (Content.TryGetValue(lang, out var val))
        {
            return val;
        }

        if (Content.TryGetValue(Default, out val))
        {
            return val;
        }

        return Content.Values.FirstOrDefault() ?? string.Empty;
    }
}
