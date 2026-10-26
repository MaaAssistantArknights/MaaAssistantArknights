// <copyright file="OperatorRoleLocalization.cs" company="MaaAssistantArknights">
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
/// Converts an operator role identifier to the corresponding display name.
/// </summary>
public static class OperatorRoleLocalization
{
    public static string GetName(OperatorRole role) => role switch
    {
        OperatorRole.Unknown => string.Empty,
        OperatorRole.Warrior => LocalizationHelper.GetString("Copilot.Warrior"),
        OperatorRole.Pioneer => LocalizationHelper.GetString("Copilot.Pioneer"),
        OperatorRole.Medic => LocalizationHelper.GetString("Copilot.Medic"),
        OperatorRole.Tank => LocalizationHelper.GetString("Copilot.Tank"),
        OperatorRole.Sniper => LocalizationHelper.GetString("Copilot.Sniper"),
        OperatorRole.Caster => LocalizationHelper.GetString("Copilot.Caster"),
        OperatorRole.Support => LocalizationHelper.GetString("Copilot.Support"),
        OperatorRole.Special => LocalizationHelper.GetString("Copilot.Special"),
        OperatorRole.Drone => LocalizationHelper.GetString("Copilot.Drone"),
        _ => ReportUnknownRole(role),
    };

    private static string ReportUnknownRole(OperatorRole role)
    {
        Log.Error("Missing operator role localization: {Role}", role);
        return $"{{{{ {role} }}}}";
    }
}
