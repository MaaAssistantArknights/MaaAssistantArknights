// <copyright file="NotificationFilter.cs" company="MaaAssistantArknights">
// Part of the MaaWpfGui project, maintained by the MaaAssistantArknights team (Maa Team)
// Copyright (C) 2021-2026 MaaAssistantArknights Contributors
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
using System.Text.RegularExpressions;
using MaaWpfGui.Configuration.Single.Settings;
using MaaWpfGui.Constants.Enums;
using Serilog;

namespace MaaWpfGui.Services.Notification;

// Each channel owns one cache. User expressions never run without a timeout.
public sealed class NotificationFilter
{
    private static readonly ILogger _logger = Log.ForContext<NotificationFilter>();

    private string? _patternText;
    private Regex[] _patterns = [];
    private bool _valid = true;

    public bool ShouldSend(NotificationSettings.Channel policy, NotificationEvent notification)
    {
        if (!policy.Enable)
        {
            return false;
        }

        if (policy.FilterMode == NotificationFilterMode.None)
        {
            return true;
        }

        if (!UpdatePatterns(policy.FilterList))
        {
            return false;
        }

        try
        {
            var matches = _patterns.Any(pattern => pattern.IsMatch(notification.FilterContent));
            return policy.FilterMode == NotificationFilterMode.Blacklist ? !matches : matches;
        }
        catch (RegexMatchTimeoutException ex)
        {
            _logger.Warning(ex, "Notification filter timed out");
            return false;
        }
    }

    public static bool IsValid(string patterns)
    {
        try
        {
            Parse(patterns);
            return true;
        }
        catch (ArgumentException)
        {
            return false;
        }
    }

    private bool UpdatePatterns(string patterns)
    {
        if (_patternText == patterns)
        {
            return _valid;
        }

        _patternText = patterns;
        try
        {
            _patterns = Parse(patterns);
            _valid = true;
        }
        catch (ArgumentException ex)
        {
            _patterns = [];
            _valid = false;
            _logger.Warning(ex, "Invalid notification filter");
        }

        return _valid;
    }

    private static Regex[] Parse(string patterns) => patterns
        .Split(['\r', '\n'], StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
        .Select(pattern => new Regex(pattern, RegexOptions.CultureInvariant, TimeSpan.FromMilliseconds(50)))
        .ToArray();
}
