// <copyright file="InfrastTask.cs" company="MaaAssistantArknights">
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
using MaaWpfGui.ViewModels.UserControl.TaskQueue;
using static MaaWpfGui.Main.AsstProxy;

namespace MaaWpfGui.Configuration.Single.MaaTask;
public class InfrastTask : BaseTask
{
    public InfrastTask() => TaskType = TaskType.Infrast;

    /// <summary>
    /// Gets or sets 基建切换模式
    /// </summary>
    public InfrastMode Mode { get; set; } = InfrastMode.Normal;


}
