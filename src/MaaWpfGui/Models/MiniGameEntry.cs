// <copyright file="MiniGameEntry.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Models;

/// <summary>
/// Represents a mini-game entry as provided by StageActivityV2.json.
/// Kept separate from CombinedData to avoid changing existing UI model.
/// </summary>
public class MiniGameEntry
{
    /// <summary>
    /// Gets or sets display text (may be non-localized). Client code should prefer localized value when available.
    /// </summary>
    public string Display { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets localization key for Display (optional).
    /// </summary>
    public string? DisplayKey { get; set; }

    /// <summary>
    /// Gets or sets internal action/value for this entry.
    /// </summary>
    public string Value { get; set; } = string.Empty;

    /// <summary>
    /// Gets or sets Tip text (may be non-localized).
    /// </summary>
    public string? Tip { get; set; }

    /// <summary>
    /// Gets or sets localization key for Tip (optional).
    /// </summary>
    public string? TipKey { get; set; }

    /// <summary>
    /// Gets or sets minimum required client version (SemVer string) for this mini-game entry.
    /// </summary>
    public string? MinimumRequired { get; set; }

    /// <summary>
    /// Gets or sets the activity UTC start time for this mini-game entry.
    /// Stored as DateTime.Utc; default is DateTime.MinValue when not provided or parse fails.
    /// </summary>
    public DateTime UtcStartTime { get; set; } = DateTime.MinValue;

    /// <summary>
    /// Gets or sets the activity UTC expire time for this mini-game entry.
    /// </summary>
    public DateTime UtcExpireTime { get; set; } = DateTime.MinValue;

    /// <summary>
    /// Gets a value indicating whether gets whether activity is currently open (based on UtcStartTime/UtcExpireTime).
    /// </summary>
    public bool BeingOpen => !NotOpenYet && !IsExpired;

    /// <summary>
    /// Gets a value indicating whether gets whether activity is expired.
    /// </summary>
    public bool IsExpired => DateTime.UtcNow >= UtcExpireTime;

    /// <summary>
    /// Gets a value indicating whether gets whether activity has not opened yet.
    /// </summary>
    public bool NotOpenYet => DateTime.UtcNow <= UtcStartTime;
}
