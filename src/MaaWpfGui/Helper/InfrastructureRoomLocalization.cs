// <copyright file="InfrastructureRoomLocalization.cs" company="MaaAssistantArknights">
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

using System;
using MaaWpfGui.Constants.Enums;
using Serilog;

namespace MaaWpfGui.Helper;

/// <summary>
/// Converts infrastructure room identifiers to display names.
/// </summary>
public static class InfrastructureRoomLocalization
{
    public static string GetName(InfrastRoomType room) => room switch
    {
        InfrastRoomType.Mfg => LocalizationHelper.GetString("Infrastructure.Mfg"),
        InfrastRoomType.Trade => LocalizationHelper.GetString("Infrastructure.Trade"),
        InfrastRoomType.Control => LocalizationHelper.GetString("Infrastructure.Control"),
        InfrastRoomType.Power => LocalizationHelper.GetString("Infrastructure.Power"),
        InfrastRoomType.Reception => LocalizationHelper.GetString("Infrastructure.Reception"),
        InfrastRoomType.Office => LocalizationHelper.GetString("Infrastructure.Office"),
        InfrastRoomType.Recycle => LocalizationHelper.GetString("Infrastructure.Recycle"),
        InfrastRoomType.Dorm => LocalizationHelper.GetString("Infrastructure.Dorm"),
        InfrastRoomType.Processing => LocalizationHelper.GetString("Infrastructure.Processing"),
        InfrastRoomType.Training => LocalizationHelper.GetString("Infrastructure.Training"),
        InfrastRoomType.AssistantChange => LocalizationHelper.GetString("Settings.AssistantChange"),
        _ => throw new ArgumentOutOfRangeException(nameof(room), room, "Unknown infrastructure room"),
    };

    public static string GetCoreFacilityName(string facility)
    {
        if (Enum.TryParse<InfrastRoomType>(facility, out var room) && Enum.IsDefined(room))
        {
            return GetName(room);
        }

        Log.Error("Unknown core facility localization identifier: {Facility}", facility);
        return $"{{{{ {facility} }}}}";
    }
}
