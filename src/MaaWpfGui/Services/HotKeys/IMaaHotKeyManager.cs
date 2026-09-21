// <copyright file="IMaaHotKeyManager.cs" company="MaaAssistantArknights">
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

public interface IMaaHotKeyManager
{
    MaaHotKeyRegistrationResult TryRegister(MaaHotKeyAction action, MaaHotKey hotKey);

    void UnRegister(MaaHotKeyAction action);

    MaaHotKey GetOrNull(MaaHotKeyAction action);

    /// <summary>
    /// Gets a value indicating whether the last registration for the action failed (occupied or duplicated); the hotkey is kept in the mapping but inactive.
    /// </summary>
    /// <param name="action">The hotkey action to query.</param>
    /// <returns><c>true</c> if the last registration attempt for the action failed; otherwise, <c>false</c>.</returns>
    bool IsRegistrationFailed(MaaHotKeyAction action);

    void Release();
}
