// <copyright file="GameWindowRestoreState.cs" company="MaaAssistantArknights">
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

using MaaWpfGui.Models;

namespace MaaWpfGui.Configuration.Global;

public sealed class GameWindowRestoreState : NotifyPropertyChangedWithValue
{
    public string ExecutablePath { get; set; } = string.Empty;

    public int Left { get; set; }

    public int Top { get; set; }

    public int ClientWidth { get; set; }

    public int ClientHeight { get; set; }
}
