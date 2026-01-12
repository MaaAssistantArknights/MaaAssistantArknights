// <copyright file="EmulatorPathSelectionViewModel.cs" company="MaaAssistantArknights">
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

using System.Collections.Generic;
using Stylet;

namespace MaaWpfGui.ViewModels;

/// <summary>
/// ViewModel for EmulatorPathSelectionWindow
/// </summary>
public class EmulatorPathSelectionViewModel : PropertyChangedBase
{
    /// <summary>
    /// Initializes a new instance of the <see cref="EmulatorPathSelectionViewModel"/> class.
    /// </summary>
    /// <param name="paths">可选路径列表</param>
    public EmulatorPathSelectionViewModel(List<string> paths)
    {
        Paths = paths;
        if (paths.Count > 0)
        {
            SelectedPath = paths[0];
        }
    }

    /// <summary>
    /// Gets the list of paths.
    /// </summary>
    public List<string> Paths { get; }

    private string? _selectedPath;

    /// <summary>
    /// Gets or sets the selected path.
    /// </summary>
    public string? SelectedPath
    {
        get => _selectedPath;
        set => SetAndNotify(ref _selectedPath, value);
    }
}
