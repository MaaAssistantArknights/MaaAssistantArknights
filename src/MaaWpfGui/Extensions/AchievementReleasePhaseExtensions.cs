// <copyright file="AchievementReleasePhaseExtensions.cs" company="MaaAssistantArknights">
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

using System;
using System.Collections.Generic;
using MaaWpfGui.Constants.Enums;

namespace MaaWpfGui.Extensions;

public static class AchievementReleasePhaseExtensions
{
    private static readonly Dictionary<AchievementReleasePhase, DateOnly> _releaseDates = new()
    {
        { AchievementReleasePhase.Phase1, new DateOnly(2025, 6, 10) },
        { AchievementReleasePhase.Phase2, new DateOnly(2026, 1, 5) },
        { AchievementReleasePhase.Phase3, new DateOnly(2026, 3, 27) },
    };

    public static DateOnly ReleaseDate(this AchievementReleasePhase phase)
        => _releaseDates.TryGetValue(phase, out var date) ? date : DateOnly.MinValue;
}
