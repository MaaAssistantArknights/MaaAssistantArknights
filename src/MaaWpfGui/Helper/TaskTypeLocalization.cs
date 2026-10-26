// <copyright file="TaskTypeLocalization.cs" company="MaaAssistantArknights">
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
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Helper;

/// <summary>
/// Converts the stable task-type identifier to a localized display name.
/// </summary>
public static class TaskTypeLocalization
{
    public static string GetName(TaskType type) => type switch
    {
        TaskType.StartUp => LocalizationHelper.GetString("TaskQueue.StartUp"),
        TaskType.CloseDown => LocalizationHelper.GetString("TaskQueue.ExitArknights"),
        TaskType.Fight => LocalizationHelper.GetString("TaskQueue.Fight"),
        TaskType.OperProgress => LocalizationHelper.GetString("TaskQueue.OperProgress"),
        TaskType.Recruit => LocalizationHelper.GetString("TaskQueue.Recruit"),
        TaskType.Infrast => LocalizationHelper.GetString("TaskQueue.Infrast"),
        TaskType.Mall => LocalizationHelper.GetString("TaskQueue.Mall"),
        TaskType.Award => LocalizationHelper.GetString("TaskQueue.Award"),
        TaskType.Roguelike => LocalizationHelper.GetString("TaskQueue.Roguelike"),
        TaskType.RecruitCalc => LocalizationHelper.GetString("Recruitment.Recognition"),
        TaskType.Copilot => LocalizationHelper.GetString("Copilot.Title"),
        TaskType.Depot => LocalizationHelper.GetString("Depot.Title"),
        TaskType.OperBox => LocalizationHelper.GetString("Operator.Title"),
        TaskType.Gacha => LocalizationHelper.GetString("Gacha.Title"),
        TaskType.Reclamation => LocalizationHelper.GetString("TaskQueue.Reclamation"),
        TaskType.UserDataUpdate => LocalizationHelper.GetString("TaskQueue.UserDataUpdate"),
        TaskType.DepotMaintain => LocalizationHelper.GetString("Depot.Maintain"),
        TaskType.SwitchTheme => LocalizationHelper.GetString("Appearance.SwitchTheme"),
        TaskType.MiniGame => LocalizationHelper.GetString("TaskQueue.MiniGame"),
        TaskType.Custom => LocalizationHelper.GetString("TaskQueue.Custom"),
        _ => throw new ArgumentOutOfRangeException(nameof(type), type, "Unknown task type"),
    };
}
