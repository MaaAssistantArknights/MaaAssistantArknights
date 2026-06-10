// <copyright file="CustomWebhookTemplateParameter.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;

namespace MaaWpfGui.Services.Notification;

public class CustomWebhookTemplateParameter
{
    public string Key { get; set; } = string.Empty;

    public CustomWebhookLocalizedString Label { get; set; } = new();

    public CustomWebhookLocalizedString? Placeholder { get; set; }

    public bool Required { get; set; }

    [JsonIgnore]
    public string DisplayLabel => Label?.Resolved ?? Key;

    [JsonIgnore]
    public string? DisplayPlaceholder => Placeholder?.Resolved;
}
