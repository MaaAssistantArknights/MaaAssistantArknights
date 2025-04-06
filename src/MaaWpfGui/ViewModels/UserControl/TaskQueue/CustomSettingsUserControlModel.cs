// <copyright file="ReclamationSettingsUserControlModel.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
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
using MaaWpfGui.Constants;
using MaaWpfGui.Helper;

namespace MaaWpfGui.ViewModels.UserControl.TaskQueue;

public class CustomSettingsUserControlModel : TaskViewModel
{
    static CustomSettingsUserControlModel()
    {
        Instance = new();
    }

    public static CustomSettingsUserControlModel Instance { get; }

    private string _taskName = ConfigurationHelper.GetGlobalValue(ConfigurationKeys.DebugTaskName, string.Empty);

    public string TaskName
    {
        get => _taskName;
        set => SetAndNotify(ref _taskName, value);
    }
}
