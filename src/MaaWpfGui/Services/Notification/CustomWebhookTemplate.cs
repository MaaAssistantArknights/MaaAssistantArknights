// <copyright file="CustomWebhookTemplate.cs" company="MaaAssistantArknights">
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
using Newtonsoft.Json;

namespace MaaWpfGui.Services.Notification;

public class CustomWebhookTemplate
{
    public string Id { get; set; } = string.Empty;

    public CustomWebhookLocalizedString Name { get; set; } = new();

    public CustomWebhookLocalizedString? Description { get; set; }

    public string Url { get; set; } = string.Empty;

    public string Headers { get; set; } = string.Empty;

    public string BodyTemplate { get; set; } = string.Empty;

    public List<CustomWebhookTemplateParameter> UserParameters { get; set; } = [];

    [JsonIgnore]
    public string DisplayName => Name?.Resolved ?? Id;

    [JsonIgnore]
    public string? DisplayDescription => Description?.Resolved;
}
