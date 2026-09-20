// <copyright file="ToolboxSettingsViewModel.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using MaaWpfGui.Configuration.Factory;
using MaaWpfGui.Configuration.Single.Settings;
using Stylet;

namespace MaaWpfGui.ViewModels.UserControl.Settings;

public class ToolboxSettingsViewModel : PropertyChangedBase
{
    // A profile switch restarts the GUI. Keep late callbacks bound to the original profile.
    private readonly Toolbox _config = ConfigFactory.CurrentConfig.Toolbox;

    public ToolboxSettingsViewModel()
    {
        MaterialCraftQueue = _config.MaterialCraftQueue ?? [];
        MaterialCraftStationOperators = _config.MaterialCraftStationOperators;
    }

    public List<MaterialCraftQueueItem> MaterialCraftQueue
    {
        get; set {
            SetAndNotify(ref field, value);
            _config.MaterialCraftQueue = value;
        }
    } = [];

    public bool MaterialCraftStationOperators
    {
        get; set {
            SetAndNotify(ref field, value);
            _config.MaterialCraftStationOperators = value;
        }
    }
}
