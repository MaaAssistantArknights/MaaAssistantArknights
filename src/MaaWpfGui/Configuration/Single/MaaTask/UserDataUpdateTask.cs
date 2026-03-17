// <copyright file="UserDataUpdateTask.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Constants.Enums;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;

/// <summary>
/// 更新用户数据任务。
/// </summary>
public class UserDataUpdateTask : BaseTask
{
    public UserDataUpdateTask() => TaskType = TaskType.UserDataUpdate;

    public bool UpdateOperBox { get; set; } = true;

    public bool UpdateDepot { get; set; } = true;

    public UserDataUpdateTriggerInterval TriggerInterval { get; set; } = UserDataUpdateTriggerInterval.EveryTime;

    public bool IsTriggered => UpdateOperBox || UpdateDepot;
}
