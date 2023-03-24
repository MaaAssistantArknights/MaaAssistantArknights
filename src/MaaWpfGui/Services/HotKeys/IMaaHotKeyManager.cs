// <copyright file="IMaaHotKeyManager.cs" company="MaaAssistantArknights">
// MaaWpfGui - A part of the MaaCoreArknights project
// Copyright (C) 2021 MistEO and Contributors
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY
// </copyright>

namespace MaaWpfGui.Services.HotKeys
{
    public interface IMaaHotKeyManager
    {
        bool TryRegister(MaaHotKeyAction action, MaaHotKey hotKey);

        void Unregister(MaaHotKeyAction action);

        MaaHotKey GetOrNull(MaaHotKeyAction action);
    }
}
