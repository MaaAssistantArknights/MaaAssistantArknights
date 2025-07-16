// <copyright file="TaskQueueViewModel.cs" company="MaaAssistantArknights">
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
using System.Linq;
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;
using MaaWpfGui.Models.AsstTasks;
using MaaWpfGui.Services;
using Newtonsoft.Json.Linq;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;
public class DGLabSettingsUserControlModel : TaskViewModel
{
    static DGLabSettingsUserControlModel()
    {
        Instance = new();
    }

    public static DGLabSettingsUserControlModel Instance { get; }

    private string _taskName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.DGLabTaskName, string.Empty);

    public string TaskName
    {
        get => _taskName;
        set
        {
            value = value.Replace("，", ",").Replace("；", ";");
            SetAndNotify(ref _taskName, value);
        }
    }

    public override (AsstTaskType Type, JObject Params) Serialize()
    {
        var task = new AsstDGLabTask();
        return task.Serialize();
    }
}
