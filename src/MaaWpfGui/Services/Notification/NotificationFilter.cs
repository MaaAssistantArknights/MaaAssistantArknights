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
using System.Diagnostics;
using System.Linq;
using System.Text.RegularExpressions;
using Serilog;

namespace MaaWpfGui.Services.Notification;

// External notification rules are cached. User expressions always have a timeout.
public sealed class NotificationFilter
{
    private static readonly ILogger _logger = Log.ForContext<NotificationFilter>();
    private static readonly TimeSpan _matchTimeout = TimeSpan.FromMilliseconds(50);

    private string? _patternText;
    private Regex[] _patterns = [];
    private bool _valid = true;

    // Null means that the expression is invalid or timed out. Callers can handle
    // invalid trigger rules and invalid attachment rules independently.
    public bool? Matches(string patterns, string content)
    {
        if (!UpdatePatterns(patterns))
        {
            return null;
        }

        try
        {
            var started = Stopwatch.GetTimestamp();
            foreach (var pattern in _patterns)
            {
                if (Stopwatch.GetElapsedTime(started) > _matchTimeout)
                {
                    _logger.Warning("Notification filter exceeded the matching time budget");
                    return null;
                }

                if (pattern.IsMatch(content))
                {
                    return true;
                }
            }

            return false;
        }
        catch (RegexMatchTimeoutException ex)
        {
            _logger.Warning(ex, "Notification filter timed out");
            return null;
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
        .Select(pattern => new Regex(pattern, RegexOptions.CultureInvariant, _matchTimeout))
        .ToArray();
}
