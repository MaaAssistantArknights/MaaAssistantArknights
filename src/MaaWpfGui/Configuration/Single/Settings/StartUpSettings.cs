// <copyright file="StartUpSettings.cs" company="MaaAssistantArknights">
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
using System.ComponentModel;

namespace MaaWpfGui.Configuration.Single.Settings;

public class StartUpSettings : INotifyPropertyChanged
{
    public event PropertyChangedEventHandler? PropertyChanged;

    public bool RunDirectly { get; set; }

    public bool StartEmulator { get; set; }

    public bool RestartEmulatorWhenAdbFailed { get; set; }

    public string EmulatorPath { get; set; } = string.Empty;

    public string EmulatorAddCommand { get; set; } = string.Empty;

    public int EmulatorWaitSeconds { get; set; } = 60;
}
