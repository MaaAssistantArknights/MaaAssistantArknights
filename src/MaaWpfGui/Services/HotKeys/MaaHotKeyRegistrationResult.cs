// <copyright file="MaaHotKeyRegistrationResult.cs" company="MaaAssistantArknights">
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

namespace MaaWpfGui.Services.HotKeys;

/// <summary>
/// Indicates why a hotkey registration attempt succeeded or failed.
/// </summary>
public enum MaaHotKeyRegistrationResult
{
    /// <summary>
    /// The hotkey was registered successfully.
    /// </summary>
    Success,

    /// <summary>
    /// The hotkey duplicates another MAA hotkey.
    /// </summary>
    DuplicateHotKey,

    /// <summary>
    /// The system rejected the registration, likely because the key combination is already taken by another program.
    /// </summary>
    OccupiedByOtherApp,
}
